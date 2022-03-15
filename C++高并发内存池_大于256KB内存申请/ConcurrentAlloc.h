#pragma once

#include"Common.h"
#include"ThreadCache.h"
#include"PageCache.h"

//线程调用申请ThreadCache空间
static void* ConcurrentAlloc(size_t size) {
	if (size > MAX_BYTE) {//大于256KB内存
		size_t AlignSize = SizeClass::RoundUp(size);//计算对其大小
		//直接向PageCache索要K页的内存
		size_t K = AlignSize >> PAGESIZE;
		PageCache::GetInst()->_PageMtx.lock();
		Span*span=PageCache::GetInst()->NewSpan(K);
		PageCache::GetInst()->_PageMtx.unlock();
		void* ptr = (void*)(span->_PageID << PAGESIZE);//获取这块内存的地址
		return ptr;
	}
	else {
		//获取线程自己的ThreadCache
		if (tls_threadcache == nullptr) {
			tls_threadcache = new ThreadCache;
		}
		cout << std::this_thread::get_id() << " " << tls_threadcache << endl;
		return tls_threadcache->ApplySpace(size);
	}
}

static void ConcurrentFree(void* ptr, size_t size) {
	if(size>MAX_BYTE){
		Span* span = PageCache::GetInst()->MapObjToSpan(ptr);//计算要释放的大空间属于那个Span

		PageCache::GetInst()->_PageMtx.lock();
		PageCache::GetInst()->RetrunPageCache(span);//将内存挂到PageCache桶上，需要修改桶，所以要加锁
		PageCache::GetInst()->_PageMtx.unlock();
	}
	else {
		//释放时每个线程一定有tls_threadcache
		assert(tls_threadcache != nullptr);
		tls_threadcache->ReleaseSpace(ptr, size);
	}
}