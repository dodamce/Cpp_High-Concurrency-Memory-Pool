#pragma once

#include"Common.h"

//���Ļ���ֻ��һ����ÿ���̶߳����Կ������������Ļ�������õ���ģʽ(����ģʽ)
class CentralCache {
private:
	SpanList _SpanList[NUMLIST];//��ThreadCacheͰ�Ĵ�С��ͬ
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
	//static���εľ�̬���������ͷ�ļ��г�ʼ��,���ͷ�ļ����������ٴ�,�����̬�����ͻᶨ����ٴ�.����Ļ����ܻ�����ض��������.
	//�������Դ�ļ���,��ֻ�ܱ���ǰ�ļ�����.���Ƕ�����ͷ�ļ���,�ĸ�Դ�ļ�����,�ĸ�Դ�ļ��Ϳ��Կ���
public:
	CentralCache() {}
	static CentralCache* GetCentralCache() {
		return &_sInst;
	}

	//��ȡһ���ǿ�Span,��Ҫ��PageCache����ռ䣬�ȿտ�
	Span* GetSpan(SpanList& List, size_t size);

	size_t GiveThreadCache(void*& start, void*& end, size_t Num, size_t Size);
};