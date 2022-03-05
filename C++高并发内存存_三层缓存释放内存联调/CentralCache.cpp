#include"CentralCache.h"
#include"PageCache.h"

CentralCache CentralCache::_sInst;//定义了一个对象

size_t CentralCache::GiveThreadCache(void*& start, void*& end, size_t Num, size_t size) {
	size_t index = SizeClass::Index(size);
	//要求num个内存节点，需要计Span中FreeList空节点个数
	_SpanList[index]._mtx.lock();//加锁

	Span* span = GetSpan(_SpanList[index], size);
	assert(span != nullptr && span->FreeList != nullptr);

	start = span->FreeList; end = start;
	//变量Num次，找Num个节点
	size_t actualNum = 1;//实际获得几个内存节点
	for (int i = 0; i < Num - 1; i++) {
		if (NextObj(end) == nullptr) break;//如果走到span中FreeList的空，说明span中内存不够Num个，这个时候有多少返回多少
		actualNum += 1;
		end = NextObj(end);
	}
	span->FreeList = NextObj(end);
	//为分出去的内存节点链表添加nullptr
	NextObj(end) = nullptr;

	span->use_count += actualNum;

	_SpanList[index]._mtx.unlock();
	return actualNum;
}

Span* CentralCache::GetSpan(SpanList& List, size_t size) {
	//在哈希桶对应位置Span链表中找是否有Span，没有就向PageCache申请空间

	//遍历桶的Span链表
	Span* it = List.begin();
	while (it != List.end()) {
		if (it->FreeList != nullptr) {
			return it;//这个Span有空间
		}
		else {
			//Span没有空间，继续找下一个链表Span
			it = it->_next;
		}
	}
	//先把CentralCache的桶锁解开，如果其他线程释放内存不会阻塞
	List._mtx.unlock();
	//没有空闲的Span只能找PageCache,需要加锁，PageCache只能由一个线程访问
	//size是单个对象的大小
	PageCache::GetInst()->_PageMtx.lock();
	Span* span=PageCache::GetInst()->NewSpan(SizeClass::NumForPage(size));
	span->IsUse = true;
	PageCache::GetInst()->_PageMtx.unlock();

	//获得了一块大Span，这块Span这时被线程单独看到，不需要加锁（没有挂到桶上）
	//Span起始地址
	char* start = (char*)((span->_PageID) << PAGESIZE);
	size_t ByteSize = (span->_Num) << PAGESIZE;
	char* end = start + ByteSize;
	//把Span内部大块内存切成自由链表链接起来
	span->FreeList = start;
	start += size;//自由链表的头节点
	void* tail = span->FreeList;
	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	List._mtx.lock();
	List.Insert(List.begin(), span);//将Span挂到桶上，此时需要加桶锁
	return span;
}

void CentralCache::MergeSpan(void* start, size_t size) {
	size_t index = SizeClass::Index(size);
	_SpanList[index]._mtx.lock();

	while (start != nullptr) {
		void* next = NextObj(start);
		Span* span = PageCache::GetInst()->MapObjToSpan(start);
		//将这个节点头插到这个Span上
		NextObj(start) = span->FreeList;
		span->FreeList = start;

		span->use_count--;//每回收一块内存，记录分出去的内存use_count就-1

		if (span->use_count == 0) {
			//Span所有小块内存都回来了，这个Span就可以给Page Cache回收。PageCache做前后页的合并
			_SpanList[index].Erase(span);

			//通过页号就可以再次找到这块内存
			span->FreeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			//页号与页数不能动，PageCache通过这两个数据找到这大块内存,此时可以解开桶锁，让其他线程访问这个桶
			_SpanList[index]._mtx.unlock();
			//此时要访问PageCache需要加PageCache的锁
			PageCache::GetInst()->_PageMtx.lock();
			PageCache::GetInst()->RetrunPageCache(span);
			PageCache::GetInst()->_PageMtx.unlock();
			_SpanList[index]._mtx.lock();
		}
		start = next;
	}

	_SpanList[index]._mtx.unlock();
}