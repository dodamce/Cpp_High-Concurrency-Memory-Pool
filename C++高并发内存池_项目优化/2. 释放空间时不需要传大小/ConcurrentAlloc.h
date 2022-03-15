#pragma once

#include"Common.h"
#include"ThreadCache.h"
#include"PageCache.h"
#include"ObjectPool.h"

//�̵߳�������ThreadCache�ռ�
static void* ConcurrentAlloc(size_t size) {
	if (size > MAX_BYTE) {//����256KB�ڴ�
		size_t AlignSize = SizeClass::RoundUp(size);//��������С
		//ֱ����PageCache��ҪKҳ���ڴ�
		size_t K = AlignSize >> PAGESIZE;
		PageCache::GetInst()->_PageMtx.lock();
		Span*span=PageCache::GetInst()->NewSpan(K);

		span->ObjectSize = size;//û�о���CentralCacheֱ�ӻ�ȡ��Span��дObjectSize,Ϊ��ConcurrentFree����size����

		PageCache::GetInst()->_PageMtx.unlock();
		void* ptr = (void*)(span->_PageID << PAGESIZE);//��ȡ����ڴ�ĵ�ַ
		return ptr;
	}
	else {
		//��ȡ�߳��Լ���ThreadCache
		if (tls_threadcache == nullptr) {
			static ObjectPool<ThreadCache>TcPool;
			tls_threadcache = TcPool.New();
		}
		cout << std::this_thread::get_id() << " " << tls_threadcache << endl;
		return tls_threadcache->ApplySpace(size);
	}
}

static void ConcurrentFree(void* ptr) {
	Span* span = PageCache::GetInst()->MapObjToSpan(ptr);//����Ҫ�ͷŵĴ�ռ������Ǹ�Span
	size_t size = span->ObjectSize;

	if(size>MAX_BYTE){

		PageCache::GetInst()->_PageMtx.lock();
		PageCache::GetInst()->RetrunPageCache(span);//���ڴ�ҵ�PageCacheͰ�ϣ���Ҫ�޸�Ͱ������Ҫ����
		PageCache::GetInst()->_PageMtx.unlock();
	}
	else {
		//�ͷ�ʱÿ���߳�һ����tls_threadcache
		assert(tls_threadcache != nullptr);
		tls_threadcache->ReleaseSpace(ptr, size);
	}
}