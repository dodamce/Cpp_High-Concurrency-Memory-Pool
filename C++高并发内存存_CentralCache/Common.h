#pragma once

#include<iostream>
#include<vector>
#include<time.h>
#include<assert.h>
#include<thread>
#include<mutex>
#include<algorithm>

using std::cout; using std::endl;

static const size_t MAX_BYTE = 126 * 1024;//����߳����볬��126KB����ֱ����ThreadCache����ռ�

static const size_t NUMLIST = 208;//ThreadCache�й�ϣͰ�ĸ���

#ifdef _WIN64
typedef unsigned long long PAGE_ID;//64λ�µ�ҳ��
#elif _WIN32
typedef size_t PAGE_ID;
#endif // _WIN32


//��ȡ����������һ���ڵ�
static void*& NextObj(void* obj) {
	return *(void**)obj;
}

//�����зֺ��ڴ����������
class FreeList {
public:
	FreeList() :_freeList(nullptr), MaxSize(1) {}
	void Push(void* obj) {
		//ͷ��
		assert(obj);
		NextObj(obj) = _freeList;
		_freeList = obj;
	}

	void* Pop() {
		//ͷɾ
		assert(_freeList);
		void* obj = _freeList;
		_freeList = NextObj(obj);
		return obj;
	}

	void PushList(void* begin, void* end) {
		//ͷ������
		NextObj(end) = _freeList;
		_freeList = begin;
	}

	bool Empty() { return _freeList == nullptr; }

	size_t& GetMaxSize() { return MaxSize; }
private:
	void* _freeList;
	size_t MaxSize;//��¼freeList����Ҫ�����Ļ����������ٸ��ڴ�ڵ�
};

//����ThreadCache�й�ϣͰ��Ͱ����
class SizeClass {
public:
	static inline size_t RoundUp(size_t size) {
		if (size <= 128) return _RoundUp(size, 8);
		else if (size <= 1024) return _RoundUp(size, 16);
		else if (size <= 8 * 1024) return _RoundUp(size, 128);
		else if (size < 64 * 1024) return _RoundUp(size, 1024);
		else if (size < 256 * 1024) return _RoundUp(size, 8 * 1024);
		else{
			assert(false);
			return -1;
		}
	}
	//�����ڹ�ϣͰ���Ǹ�λ��
	static inline size_t Index(size_t size) {
		assert(size <= MAX_BYTE);
		static int Group[4] = { 16,56,56,56 };
		if (size <= 128) return _Index(size, 8);
		else if (size <= 1024) return _Index(size - 128, 16) + Group[0];
		else if (size <= 8 * 1024) return _Index(size - 1024, 128) + Group[1] + Group[0];
		else if (size < 64 * 1024) return _Index(size - 8 * 1024, 1024) + Group[2] + Group[1] + Group[0];
		else if (size < 256 * 1024) return _Index(size - 64 * 1024, 8 * 1024) + Group[3] + Group[2] + Group[1] + Group[0];
		else {
			assert(false);
			return -1;
		}
	}

	//ThreadCache�����Ļ��������ҳ������
	static size_t ForMemory(size_t size) {
		assert(size > 0);
		size_t num = MAX_BYTE / size;
		if (num < 2) {
			num = 2;//�������ܴ�һ���ٸ�ThreadCacheһЩ��
		}
		else if (num > 512) {//��������С��һ�ζ��ThreadCacheһЩ
			num = 512;
		}
		return num;
	}
private:
	static inline size_t _RoundUp(size_t size, size_t AlignNum) {
		size_t AligSize = size;
		if (size % AlignNum != 0) {
			AligSize = (size / AlignNum + 1) * AlignNum;
		}
		return AligSize;
	}
	static inline size_t _Index(size_t size, size_t AlignNum) {
		size_t Pos = 0;
		if (size % AlignNum == 0) {
			Pos = size / AlignNum - 1;
		}
		else {
			Pos = size / AlignNum;
		}
		return Pos;
	}
};

//������ҳΪ��λ�Ĵ��ռ�ṹ
struct Span {
	PAGE_ID _PageID;//����ڴ�ҳ��
	size_t _Num;//ҳ����
	Span* _next;//˫������
	Span* _prev;

	size_t use_count;//��¼�����˶��ٸ������ThreadCahce

	void* FreeList;//�кõ�С���ڴ�ռ�

	Span() :_PageID(0), _Num(0), _next(nullptr), _prev(nullptr), use_count(0), FreeList(nullptr) {}
};

class SpanList {
private:
	Span* _head;
public:
	std::mutex _mtx;//Ͱ��
	SpanList() {//��ͷ˫��ѭ������
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Insert(Span* pos, Span* NewSpan) {
		//posλ��ǰ����NewSpan
		assert(pos != nullptr && NewSpan != nullptr);
		Span* prev = pos->_prev;//ǰһ��
		prev->_next = NewSpan;
		NewSpan->_prev = prev;
		NewSpan->_next = pos;
		pos->_prev = NewSpan;
	}

	void Erase(Span* pos) {
		assert(pos != _head);
		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
		//������һ���PageCache
	}
};