#include"CentralCache.h"
#include"PageCache.h"

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

	span->use_count += actualNum;

	_SpanList[index]._mtx.unlock();
	return actualNum;
}

Span* CentralCache::GetSpan(SpanList& List, size_t size) {
	//�ڹ�ϣͰ��Ӧλ��Span���������Ƿ���Span��û�о���PageCache����ռ�

	//����Ͱ��Span����
	Span* it = List.begin();
	while (it != List.end()) {
		if (it->FreeList != nullptr) {
			return it;//���Span�пռ�
		}
		else {
			//Spanû�пռ䣬��������һ������Span
			it = it->_next;
		}
	}
	//�Ȱ�CentralCache��Ͱ���⿪����������߳��ͷ��ڴ治������
	List._mtx.unlock();
	//û�п��е�Spanֻ����PageCache,��Ҫ������PageCacheֻ����һ���̷߳���
	//size�ǵ�������Ĵ�С
	PageCache::GetInst()->_PageMtx.lock();
	Span* span=PageCache::GetInst()->NewSpan(SizeClass::NumForPage(size));
	span->IsUse = true;
	PageCache::GetInst()->_PageMtx.unlock();

	//�����һ���Span�����Span��ʱ���̵߳�������������Ҫ������û�йҵ�Ͱ�ϣ�
	//Span��ʼ��ַ
	char* start = (char*)((span->_PageID) << PAGESIZE);
	size_t ByteSize = (span->_Num) << PAGESIZE;
	char* end = start + ByteSize;
	//��Span�ڲ�����ڴ��г�����������������
	span->FreeList = start;
	start += size;//���������ͷ�ڵ�
	void* tail = span->FreeList;
	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	List._mtx.lock();
	List.Insert(List.begin(), span);//��Span�ҵ�Ͱ�ϣ���ʱ��Ҫ��Ͱ��
	return span;
}

void CentralCache::MergeSpan(void* start, size_t size) {
	size_t index = SizeClass::Index(size);
	_SpanList[index]._mtx.lock();

	while (start != nullptr) {
		void* next = NextObj(start);
		Span* span = PageCache::GetInst()->MapObjToSpan(start);
		//������ڵ�ͷ�嵽���Span��
		NextObj(start) = span->FreeList;
		span->FreeList = start;

		span->use_count--;//ÿ����һ���ڴ棬��¼�ֳ�ȥ���ڴ�use_count��-1

		if (span->use_count == 0) {
			//Span����С���ڴ涼�����ˣ����Span�Ϳ��Ը�Page Cache���ա�PageCache��ǰ��ҳ�ĺϲ�
			_SpanList[index].Erase(span);

			//ͨ��ҳ�žͿ����ٴ��ҵ�����ڴ�
			span->FreeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			//ҳ����ҳ�����ܶ���PageCacheͨ�������������ҵ������ڴ�,��ʱ���Խ⿪Ͱ�����������̷߳������Ͱ
			_SpanList[index]._mtx.unlock();
			//��ʱҪ����PageCache��Ҫ��PageCache����
			PageCache::GetInst()->_PageMtx.lock();
			PageCache::GetInst()->RetrunPageCache(span);
			PageCache::GetInst()->_PageMtx.unlock();
			_SpanList[index]._mtx.lock();
		}
		start = next;
	}

	_SpanList[index]._mtx.unlock();
}