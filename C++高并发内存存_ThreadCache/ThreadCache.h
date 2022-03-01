#pragma once

#include"Common.h"

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

	//�����Ļ�������ռ䣬�ȿտ�
	void* RequestFromCentralCache(size_t index, size_t size) { return nullptr; }
};

//ÿ���̶߳���ӵ���Լ���ThreadCache
static __declspec(thread) ThreadCache* tls_threadcache = nullptr;//��static����ֻ�ڵ�ǰ�ļ��ɼ�