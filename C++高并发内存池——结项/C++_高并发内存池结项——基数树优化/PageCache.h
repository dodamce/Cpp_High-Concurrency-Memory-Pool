#pragma once

#include"Common.h"
#include"ObjectPool.h"
#include"PageMap.h"

class PageCache {
private:
	SpanList _SpanList[NPAGE];
	static PageCache _sInst;

	////ҳ����Span�����ӳ��,����黹�ڴ�ʱֱ��ͨ���ڴ���ҳ���ҵ�����ڴ����Ǹ�Span
	//std::unordered_map<PAGE_ID, Span*>IdSpanMap;

	//�����ڴ��������Span����
	ObjectPool<Span>_SpanPool;

	TCMalloc_PageMap1<32 - PAGESIZE>IdSpanMap;//����һ�������

	PageCache() {}
public:
	std::mutex _PageMtx;

	PageCache(const PageCache&) = delete;

	//��ȡ����ڴ����Ǹ�Span
	Span* MapObjToSpan(void* obj);

	static PageCache* GetInst() {
		return &_sInst;
	}

	//��CentralCache��Span����,�ϲ�����ҳ
	void RetrunPageCache(Span* span);

	//��ȡNumPageҳ��Span
	Span* NewSpan(size_t NumPage);
};