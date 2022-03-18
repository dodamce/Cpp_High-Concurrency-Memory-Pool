#include"PageCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t NumPage) {//NumPage��ҳ��
	assert(NumPage > 0);

	if (NumPage >= NPAGE) {//���Ҫ������ڴ����Ͱ�ĸ�����ֱ���������ռ�
		void* ptr = SystemAlloc(NumPage);
		Span* span = _SpanPool.New();
		span->_PageID = (PAGE_ID)ptr >> PAGESIZE;
		span->_Num = NumPage;
		//������ʼҳ�ţ������ͷ��ڴ�
		//IdSpanMap[span->_PageID]=span;
		IdSpanMap.set(span->_PageID, span);
		return span;
	}
	else {
		//����ǰλ��Ͱ���Ƿ���Span
		if (!_SpanList[NumPage].Empty()) {
			Span*NumPageSpan =_SpanList[NumPage].Pop();
			for (PAGE_ID i = 0; i < NumPageSpan->_Num; i++) {//����Numҳ
				IdSpanMap.set(NumPageSpan->_PageID, NumPageSpan);
			}
			return NumPageSpan;
		}
		//��Ӧλ�õ�Ͱ�ǿգ�������Ͱ����û��Span,����Span�зֳ�СSpan
		for (size_t i = NumPage + 1; i < NPAGE; i++) {
			if (!_SpanList[i].Empty()) {//��һ��Ͱ���ڣ��зִ�Span��NumPageҳSpan��N-NumPageҳ��Span
				Span* NumPageSpan = _SpanPool.New();
				Span* NSpan = _SpanList[i].Pop();
				//ͷ��
				NumPageSpan->_PageID = NSpan->_PageID;
				NumPageSpan->_Num = NumPage;
				NSpan->_PageID += NumPage;
				NSpan->_Num -= NumPage;

				//�����µ�Span�ҵ�����Ͱ��
				_SpanList[NSpan->_Num].Insert(_SpanList[NSpan->_Num].begin(), NSpan);
				//����NSpanǰ��ҳ��ӳ���ϵ���������
				IdSpanMap.set(NSpan->_PageID, NSpan);
				IdSpanMap.set(NSpan->_PageID + NSpan->_Num - 1, NSpan);//�м�ҳû��Ҫ����ӳ���У�ǰ����ҳΪ�˷������
				//����PAGE_ID��Span*ӳ����
				for (PAGE_ID i = 0; i < NumPageSpan->_Num; i++) {//����Numҳ
					IdSpanMap.set(NumPageSpan->_PageID + i, NumPageSpan);
				}

				return NumPageSpan;
			}
		}
		//����Ͱ��û��Span,ֱ�����������һ���ռ䣬�����ռ�ҵ����һ��Ͱ��
		Span* BigSpan = _SpanPool.New();
		void* ptr = SystemAlloc(NPAGE - 1);
		BigSpan->_PageID = (PAGE_ID)ptr >> PAGESIZE;
		BigSpan->_Num = NPAGE - 1;
		_SpanList[BigSpan->_Num].Insert(_SpanList[BigSpan->_Num].begin(), BigSpan);
		//�ڵ����Լ���Central Cache���Ϳռ�
		return NewSpan(NumPage);
	}
}

Span* PageCache::MapObjToSpan(void* obj) {

	//std::unique_lock<std::mutex>lock(_PageMtx);//ӳ�䱻����̷߳�����Ҫ������ֹ�̰߳�ȫ�����˺������Զ��ͷ�
	// ʹ�û���������Ҫ����
	//����obj��ҳ��
	PAGE_ID pageId = (PAGE_ID)obj >> PAGESIZE;
	//��ȡ����ڴ����Ǹ�Span
	Span* ret =(Span*)IdSpanMap.get(pageId);
	assert(ret != nullptr);
	return ret;
}

//��CentralCache��Span����,�ϲ�����ҳ
void PageCache::RetrunPageCache(Span* span) {
	if (span->_Num >= NPAGE) {
		//ֱ���������Ŀռ����128ҳ�Ļ���ϵͳ������Ĺ���Ͱ��
		void* ptr = (void*)(span->_PageID << PAGESIZE);
		SystemFree(ptr);
		_SpanPool.Delete(span);
	}
	else {
		//��Spanǰ��ҳ���кϲ��������ڴ���Ƭ����Ƭ����
		while (true) { //��ǰ�ϲ�
			PAGE_ID prevId = span->_PageID - 1;
			Span* ret = (Span*)IdSpanMap.get(prevId);
			if (ret == nullptr) {
				break;
			}
			else {
				if (ret->IsUse == true) {
					//ǰ������ҳ��Span����ʹ�õ��ڴ治�ϲ�
					break;
				}
				//��ǰSpan�ϲ���С����128KB��ֹͣ�ϲ�,�޷�����
				else if (ret->_Num + span->_Num >= NPAGE) {
					break;
				}
				else {
					//�ϲ�
					span->_PageID = ret->_PageID;
					span->_Num += ret->_Num;
					_SpanList[ret->_Num].Erase(ret);
					_SpanPool.Delete(ret);
				}
			}
		}
		//���ϲ�
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
		//���ϲ���Span�������ӳ��
		_SpanList[span->_Num].Insert(_SpanList[span->_Num].begin(), span);
		span->IsUse = false;
		IdSpanMap.set(span->_PageID, span);
		IdSpanMap.set(span->_PageID + span->_Num - 1, span);
	}
}