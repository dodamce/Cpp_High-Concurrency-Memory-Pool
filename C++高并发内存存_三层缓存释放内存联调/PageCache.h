#pragma once

#include"Common.h"

class PageCache {
private:
	SpanList _SpanList[NPAGE];
	static PageCache _sInst;

	//页号与Span链表的映射,方便归还内存时直接通过内存找页号找到这块内存是那个Span
	std::unordered_map<PAGE_ID, Span*>IdSpanMap;

	PageCache() {}
public:
	std::mutex _PageMtx;

	PageCache(const PageCache&) = delete;

	//获取这个内存是那个Span
	Span* MapObjToSpan(void* obj);

	static PageCache* GetInst() {
		return &_sInst;
	}

	//将CentralCache的Span回收,合并相邻页
	void RetrunPageCache(Span* span);

	//获取NumPage页的Span
	Span* NewSpan(size_t NumPage);
};