#pragma once

#include"Common.h"
#include"CentralCache.h"

class ThreadCache {
private:
	FreeList _FreeList[NUMLIST];
public:
	//�����ͷſռ�
	void* ApplySpace(size_t size) {
		assert(size <= MAX_BYTE);
		size_t AligSize = SizeClass::RoundUp(size);
		//����Ͱλ��
		size_t index = SizeClass::Index(AligSize);
		if (!_FreeList[index].Empty()) {
			return _FreeList[index].Pop();
		}
		else {
			//thread cacheû���ڴ���central cacheҪ�ռ�
			return RequestFromCentralCache(index, AligSize);
		}
	}
	void ReleaseSpace(void* ptr, size_t size) {
		assert(size <= MAX_BYTE && ptr != nullptr);
		size_t index = SizeClass::Index(size);//�ҵ��ڼ���Ͱ
		//������ռ�ͷ�嵽����������
		_FreeList[index].Push(ptr);
	}

	//�����Ļ�������ռ�
	void* RequestFromCentralCache(size_t index, size_t size) { 
		//����ʼ�����㷨
		size_t Num = std::min(SizeClass::ForMemory(size),_FreeList[index].GetMaxSize());//�������Ĳ���̻߳�����ٸ��ռ�ڵ�;
		if (Num == _FreeList[index].GetMaxSize()) {
			_FreeList[index].GetMaxSize() += 2;//������,ÿ�������´λ���ThreadCache�ռ�
		}
		//���ε���SizeClass::ForMemory(size)���������ޣ�һ���������������������
		void* begin = nullptr; void* end = nullptr;
		size_t actualNum = CentralCache::GetCentralCache()->GiveThreadCache(begin, end, Num, size);
		assert(actualNum > 1);//���ٻ����뵽һ���ռ�ڵ㡣
		if (actualNum == 1) {//ʵ�ʾͻ����һ���ڵ�,ֱ�ӽ�����ڵ㷵��
			return begin;
		}
		else {
			//����˶���ڵ㣬Ҫ����Щ�ڵ㶼���뵽ThreadCache�Ĺ�ϣͰ��
			_FreeList[index].PushList(NextObj(begin), end);//���������һ���ڵ����Ͱ�У�ͷ�ڵ㷵�ظ�ThreadCache
			return begin;
		}
	}
};

//ÿ���̶߳���ӵ���Լ���ThreadCache
static __declspec(thread) ThreadCache* tls_threadcache = nullptr;//��static����ֻ�ڵ�ǰ�ļ��ɼ�