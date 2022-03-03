#pragma once

#include"Common.h"
#include"ThreadCache.h"

//�̵߳�������ThreadCache�ռ�
static void* ConcurrentAlloc(size_t size) {
	//��ȡ�߳��Լ���ThreadCache
	if (tls_threadcache == nullptr) {
		tls_threadcache = new ThreadCache;
	}
	cout << std::this_thread::get_id()<<" "<<tls_threadcache<< endl;
	return tls_threadcache->ApplySpace(size);
}

static void ConcurrentFree(void* ptr, size_t size) {
	//�ͷ�ʱÿ���߳�һ����tls_threadcache
	assert(tls_threadcache != nullptr);
	tls_threadcache->ReleaseSpace(ptr,size);
}