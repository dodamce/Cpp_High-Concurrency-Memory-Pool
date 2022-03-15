#pragma once

#include"Common.h"

//中心缓存只有一个，每个线程都可以看见，所以中心缓存类采用单例模式(饿汉模式)
class CentralCache {
private:
	SpanList _SpanList[NUMLIST];//与ThreadCache桶的大小相同
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
public:
	CentralCache() {}
	static CentralCache* GetCentralCache() {
		return &_sInst;
	}

	//获取一个非空Span,需要向PageCache申请空间
	Span* GetSpan(SpanList& List, size_t size);

	size_t GiveThreadCache(void*& start, void*& end, size_t Num, size_t Size);

	//将一条空间链表合并成Span挂到桶上，实现释放内存的过程
	void MergeSpan(void* start, size_t size);
};