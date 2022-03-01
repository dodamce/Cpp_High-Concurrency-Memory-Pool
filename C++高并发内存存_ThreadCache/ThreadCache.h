#pragma once

#include"Common.h"

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

	//向中心缓存申请空间，先空开
	void* RequestFromCentralCache(size_t index, size_t size) { return nullptr; }
};

//每个线程都会拥有自己的ThreadCache
static __declspec(thread) ThreadCache* tls_threadcache = nullptr;//用static修饰只在当前文件可见