#include"PageCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t NumPage) {//NumPage是页数
	assert(NumPage > 0);

	if (NumPage >= NPAGE) {//如果要申请的内存大于桶的个数，直接向堆申请空间
		void* ptr = SystemAlloc(NumPage);
		Span* span = _SpanPool.New();
		span->_PageID = (PAGE_ID)ptr >> PAGESIZE;
		span->_Num = NumPage;
		//保存起始页号，方便释放内存
		//IdSpanMap[span->_PageID]=span;
		IdSpanMap.set(span->_PageID, span);
		return span;
	}
	else {
		//看当前位置桶中是否有Span
		if (!_SpanList[NumPage].Empty()) {
			Span*NumPageSpan =_SpanList[NumPage].Pop();
			for (PAGE_ID i = 0; i < NumPageSpan->_Num; i++) {//切了Num页
				IdSpanMap.set(NumPageSpan->_PageID, NumPageSpan);
			}
			return NumPageSpan;
		}
		//对应位置的桶是空，检查后面桶里有没有Span,将大Span切分成小Span
		for (size_t i = NumPage + 1; i < NPAGE; i++) {
			if (!_SpanList[i].Empty()) {//有一个桶存在，切分大Span成NumPage页Span和N-NumPage页的Span
				Span* NumPageSpan = _SpanPool.New();
				Span* NSpan = _SpanList[i].Pop();
				//头切
				NumPageSpan->_PageID = NSpan->_PageID;
				NumPageSpan->_Num = NumPage;
				NSpan->_PageID += NumPage;
				NSpan->_Num -= NumPage;

				//将切下的Span挂到其他桶上
				_SpanList[NSpan->_Num].Insert(_SpanList[NSpan->_Num].begin(), NSpan);
				//保存NSpan前后页的映射关系，方便回收
				IdSpanMap.set(NSpan->_PageID, NSpan);
				IdSpanMap.set(NSpan->_PageID + NSpan->_Num - 1, NSpan);//中间页没必要加入映射中，前后两页为了方便回收
				//填入PAGE_ID与Span*映射中
				for (PAGE_ID i = 0; i < NumPageSpan->_Num; i++) {//切了Num页
					IdSpanMap.set(NumPageSpan->_PageID + i, NumPageSpan);
				}

				return NumPageSpan;
			}
		}
		//所有桶都没有Span,直接向堆中申请一大块空间，将这块空间挂到最后一个桶上
		Span* BigSpan = _SpanPool.New();
		void* ptr = SystemAlloc(NPAGE - 1);
		BigSpan->_PageID = (PAGE_ID)ptr >> PAGESIZE;
		BigSpan->_Num = NPAGE - 1;
		_SpanList[BigSpan->_Num].Insert(_SpanList[BigSpan->_Num].begin(), BigSpan);
		//在调用自己向Central Cache发送空间
		return NewSpan(NumPage);
	}
}

Span* PageCache::MapObjToSpan(void* obj) {

	//std::unique_lock<std::mutex>lock(_PageMtx);//映射被多个线程访问需要加锁防止线程安全，出了函数锁自动释放
	// 使用基数树后不需要加锁
	//计算obj的页号
	PAGE_ID pageId = (PAGE_ID)obj >> PAGESIZE;
	//获取这个内存是那个Span
	Span* ret =(Span*)IdSpanMap.get(pageId);
	assert(ret != nullptr);
	return ret;
}

//将CentralCache的Span回收,合并相邻页
void PageCache::RetrunPageCache(Span* span) {
	if (span->_Num >= NPAGE) {
		//直接向堆申请的空间大于128页的还给系统，其余的挂在桶上
		void* ptr = (void*)(span->_PageID << PAGESIZE);
		SystemFree(ptr);
		_SpanPool.Delete(span);
	}
	else {
		//对Span前后页进行合并，缓解内存碎片外碎片问题
		while (true) { //向前合并
			PAGE_ID prevId = span->_PageID - 1;
			Span* ret = (Span*)IdSpanMap.get(prevId);
			if (ret == nullptr) {
				break;
			}
			else {
				if (ret->IsUse == true) {
					//前面相邻页的Span正在使用的内存不合并
					break;
				}
				//和前Span合并大小超过128KB则停止合并,无法管理
				else if (ret->_Num + span->_Num >= NPAGE) {
					break;
				}
				else {
					//合并
					span->_PageID = ret->_PageID;
					span->_Num += ret->_Num;
					_SpanList[ret->_Num].Erase(ret);
					_SpanPool.Delete(ret);
				}
			}
		}
		//向后合并
		while (true) {
			PAGE_ID nextId = span->_PageID + span->_Num;
			Span* ret = (Span*)IdSpanMap.get(nextId);
			if (ret == nullptr) {
				break;
			}
			else {
				Span* nextSpan = ret;
				if (nextSpan->IsUse == true) {
					break;
				}
				else if (nextSpan->_Num + span->_Num >= NPAGE) {
					break;
				}
				else {
					span->_Num += nextSpan->_Num;
					_SpanList[nextSpan->_Num].Erase(nextSpan);
					_SpanPool.Delete(nextSpan);
				}
			}
		}
		//将合并的Span挂起并添加映射
		_SpanList[span->_Num].Insert(_SpanList[span->_Num].begin(), span);
		span->IsUse = false;
		IdSpanMap.set(span->_PageID, span);
		IdSpanMap.set(span->_PageID + span->_Num - 1, span);
	}
}