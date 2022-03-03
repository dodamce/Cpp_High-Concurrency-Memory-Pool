#pragma once

#include"Common.h"
#include"ThreadCache.h"

//线程调用申请ThreadCache空间
static void* ConcurrentAlloc(size_t size) {
	//获取线程自己的ThreadCache
	if (tls_threadcache == nullptr) {
		tls_threadcache = new ThreadCache;
	}
	cout << std::this_thread::get_id()<<" "<<tls_threadcache<< endl;
	return tls_threadcache->ApplySpace(size);
}

static void ConcurrentFree(void* ptr, size_t size) {
	//释放时每个线程一定有tls_threadcache
	assert(tls_threadcache != nullptr);
	tls_threadcache->ReleaseSpace(ptr,size);
}