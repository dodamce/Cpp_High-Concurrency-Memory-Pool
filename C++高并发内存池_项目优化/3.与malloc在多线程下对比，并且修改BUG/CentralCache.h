#pragma once

#include"Common.h"

//���Ļ���ֻ��һ����ÿ���̶߳����Կ������������Ļ�������õ���ģʽ(����ģʽ)
class CentralCache {
private:
	SpanList _SpanList[NUMLIST];//��ThreadCacheͰ�Ĵ�С��ͬ
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
public:
	CentralCache() {}
	static CentralCache* GetCentralCache() {
		return &_sInst;
	}

	//��ȡһ���ǿ�Span,��Ҫ��PageCache����ռ�
	Span* GetSpan(SpanList& List, size_t size);

	size_t GiveThreadCache(void*& start, void*& end, size_t Num, size_t Size);

	//��һ���ռ�����ϲ���Span�ҵ�Ͱ�ϣ�ʵ���ͷ��ڴ�Ĺ���
	void MergeSpan(void* start, size_t size);
};