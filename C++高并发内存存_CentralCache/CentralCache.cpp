#include"CentralCache.h"

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

	_SpanList[index]._mtx.unlock();
	return actualNum;
}

Span* CentralCache::GetSpan(SpanList& List, size_t size) {
	//在哈希桶对应位置Span链表中找是否有Span，没有就向PageCache申请空间
	return nullptr;
}