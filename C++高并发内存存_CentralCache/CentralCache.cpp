#include"CentralCache.h"

CentralCache CentralCache::_sInst;//������һ������

size_t CentralCache::GiveThreadCache(void*& start, void*& end, size_t Num, size_t size) {
	size_t index = SizeClass::Index(size);
	//Ҫ��num���ڴ�ڵ㣬��Ҫ��Span��FreeList�սڵ����
	_SpanList[index]._mtx.lock();//����

	Span* span = GetSpan(_SpanList[index], size);
	assert(span != nullptr && span->FreeList != nullptr);

	start = span->FreeList; end = start;
	//����Num�Σ���Num���ڵ�
	size_t actualNum = 1;//ʵ�ʻ�ü����ڴ�ڵ�
	for (int i = 0; i < Num - 1; i++) {
		if (NextObj(end) == nullptr) break;//����ߵ�span��FreeList�Ŀգ�˵��span���ڴ治��Num�������ʱ���ж��ٷ��ض���
		actualNum += 1;
		end = NextObj(end);
	}
	span->FreeList = NextObj(end);
	//Ϊ�ֳ�ȥ���ڴ�ڵ��������nullptr
	NextObj(end) = nullptr;

	_SpanList[index]._mtx.unlock();
	return actualNum;
}

Span* CentralCache::GetSpan(SpanList& List, size_t size) {
	//�ڹ�ϣͰ��Ӧλ��Span���������Ƿ���Span��û�о���PageCache����ռ�
	return nullptr;
}