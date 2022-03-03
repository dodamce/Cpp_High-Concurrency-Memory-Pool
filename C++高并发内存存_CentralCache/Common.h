#pragma once

#include<iostream>
#include<vector>
#include<time.h>
#include<assert.h>
#include<thread>
#include<mutex>
#include<algorithm>

using std::cout; using std::endl;

static const size_t MAX_BYTE = 126 * 1024;//如果线程申请超过126KB不能直接向ThreadCache申请空间

static const size_t NUMLIST = 208;//ThreadCache中哈希桶的个数

#ifdef _WIN64
typedef unsigned long long PAGE_ID;//64位下的页号
#elif _WIN32
typedef size_t PAGE_ID;
#endif // _WIN32


//获取自由链表下一个节点
static void*& NextObj(void* obj) {
	return *(void**)obj;
}

//管理切分好内存的自由链表
class FreeList {
public:
	FreeList() :_freeList(nullptr), MaxSize(1) {}
	void Push(void* obj) {
		//头插
		assert(obj);
		NextObj(obj) = _freeList;
		_freeList = obj;
	}

	void* Pop() {
		//头删
		assert(_freeList);
		void* obj = _freeList;
		_freeList = NextObj(obj);
		return obj;
	}

	void PushList(void* begin, void* end) {
		//头插链表
		NextObj(end) = _freeList;
		_freeList = begin;
	}

	bool Empty() { return _freeList == nullptr; }

	size_t& GetMaxSize() { return MaxSize; }
private:
	void* _freeList;
	size_t MaxSize;//记录freeList链表要向中心缓存层申请多少个内存节点
};

//计算ThreadCache中哈希桶的桶个数
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
	//计算在哈希桶的那个位置
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

	//ThreadCache向中心缓存申请的页的数量
	static size_t ForMemory(size_t size) {
		assert(size > 0);
		size_t num = MAX_BYTE / size;
		if (num < 2) {
			num = 2;//如果对象很大，一次少给ThreadCache一些。
		}
		else if (num > 512) {//如果对象很小，一次多给ThreadCache一些
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

//管理以页为单位的大块空间结构
struct Span {
	PAGE_ID _PageID;//大块内存页号
	size_t _Num;//页数量
	Span* _next;//双向链表
	Span* _prev;

	size_t use_count;//记录分配了多少个对象给ThreadCahce

	void* FreeList;//切好的小块内存空间

	Span() :_PageID(0), _Num(0), _next(nullptr), _prev(nullptr), use_count(0), FreeList(nullptr) {}
};

class SpanList {
private:
	Span* _head;
public:
	std::mutex _mtx;//桶锁
	SpanList() {//带头双向循环链表
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Insert(Span* pos, Span* NewSpan) {
		//pos位置前插入NewSpan
		assert(pos != nullptr && NewSpan != nullptr);
		Span* prev = pos->_prev;//前一个
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
		//还给下一层的PageCache
	}
};