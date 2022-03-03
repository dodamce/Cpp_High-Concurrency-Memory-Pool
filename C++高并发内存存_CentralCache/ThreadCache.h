#pragma once

#include"Common.h"
#include"CentralCache.h"

class ThreadCache {
private:
	FreeList _FreeList[NUMLIST];
public:
	//申请释放空间
	void* ApplySpace(size_t size) {
		assert(size <= MAX_BYTE);
		size_t AligSize = SizeClass::RoundUp(size);
		//计算桶位置
		size_t index = SizeClass::Index(AligSize);
		if (!_FreeList[index].Empty()) {
			return _FreeList[index].Pop();
		}
		else {
			//thread cache没有内存向central cache要空间
			return RequestFromCentralCache(index, AligSize);
		}
	}
	void ReleaseSpace(void* ptr, size_t size) {
		assert(size <= MAX_BYTE && ptr != nullptr);
		size_t index = SizeClass::Index(size);//找到第几个桶
		//将这个空间头插到自由链表上
		_FreeList[index].Push(ptr);
	}

	//向中心缓存申请空间
	void* RequestFromCentralCache(size_t index, size_t size) { 
		//慢开始调节算法
		size_t Num = std::min(SizeClass::ForMemory(size),_FreeList[index].GetMaxSize());//计算中心层给线程缓存多少个空间节点;
		if (Num == _FreeList[index].GetMaxSize()) {
			_FreeList[index].GetMaxSize() += 2;//慢增长,每次申请下次会多给ThreadCache空间
		}
		//依次递增SizeClass::ForMemory(size)是申请上限，一次申请数量不会比它还大
		void* begin = nullptr; void* end = nullptr;
		size_t actualNum = CentralCache::GetCentralCache()->GiveThreadCache(begin, end, Num, size);
		assert(actualNum > 1);//至少会申请到一个空间节点。
		if (actualNum == 1) {//实际就获得了一个节点,直接将这个节点返回
			return begin;
		}
		else {
			//获得了多个节点，要把这些节点都插入到ThreadCache的哈希桶上
			_FreeList[index].PushList(NextObj(begin), end);//将链表的下一个节点插入桶中，头节点返回给ThreadCache
			return begin;
		}
	}
};

//每个线程都会拥有自己的ThreadCache
static __declspec(thread) ThreadCache* tls_threadcache = nullptr;//用static修饰只在当前文件可见