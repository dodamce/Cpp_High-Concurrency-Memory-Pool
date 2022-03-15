#pragma once

#include<iostream>
#include<vector>
#include<time.h>
#include<assert.h>
#include<thread>
#include<mutex>
#include<algorithm>
#include<unordered_map>

#include<windows.h>

// 直接去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux下brk mmap等
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

//释放直接向堆申请的空间
inline static void SystemFree(void* ptr) {
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
#endif
}

using std::cout; using std::endl;

static const size_t MAX_BYTE = 256 * 1024;//如果线程申请超过256KB不能直接向ThreadCache申请空间

static const size_t NUMLIST = 208;//ThreadCache中哈希桶的个数

static const size_t NPAGE = 129;//PageCache中哈希桶的个数,0号桶空开从1号桶开始

static const size_t PAGESIZE = 13;//定义一页的大小为2^13（8K）

#ifdef _WIN64
typedef unsigned long long PAGE_ID;//64位下的页号
#elif _WIN32
typedef size_t PAGE_ID;
#else 
	//Linux
#endif // _WIN32


//获取自由链表下一个节点
static void*& NextObj(void* obj) {
	return *(void**)obj;
}

//管理切分好内存的自由链表
class FreeList {
public:
	FreeList() :_freeList(nullptr), MaxSize(1), size(0) {}
	void Push(void* obj) {
		//头插
		assert(obj);
		NextObj(obj) = _freeList;
		_freeList = obj;
		++size;
	}

	void* Pop() {
		//头删
		assert(_freeList);
		void* obj = _freeList;
		_freeList = NextObj(obj);
		--size;
		return obj;
	}

	void PushList(void* begin, void* end, size_t len) {
		//头插链表
		NextObj(end) = _freeList;
		_freeList = begin;
		size += len;
	}

	void PopList(void*& begin, void*& end, size_t len) {//获得len个内存节点
		assert(len <= size);//要弹出的节点个数小于现在freeList中的节点个数
		begin = _freeList; end = begin;
		for (size_t i = 0; i < len - 1; i++) {
			end = NextObj(end);
		}
		_freeList = NextObj(end);
		NextObj(end) = nullptr;
		size -= len;
	}

	bool Empty() { return _freeList == nullptr; }

	size_t GetSize() { return size; }
	size_t& GetMaxSize() { return MaxSize; }
private:
	void* _freeList;
	size_t MaxSize;//记录freeList链表要向中心缓存层申请多少个内存节点

	size_t size;
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
			return _RoundUp(size, 1 << PAGESIZE);//8KB对其
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

	//ThreadCache一个Span中链表挂多少个空间节点
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

	//计算PageCache向系统申请页的数目
	static size_t NumForPage(size_t size) {
		size_t NumForMemory = ForMemory(size);//计算中心缓存层一个Span最多要多少节点

		size_t Byte = NumForMemory * size;//总共的字节数

		size_t NumPage = (Byte >> PAGESIZE);//计算申请几页
		if (NumPage == 0) {
			NumPage = 1;//至少给一页空间
		}
		return NumPage;
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
	PAGE_ID _PageID;//记录是第几页
	size_t _Num;//记录Span里面有多少页
	Span* _next;//双向链表
	Span* _prev;

	size_t use_count;//记录分配了多少个对象给ThreadCahce

	void* FreeList;//切好的小块内存空间

	bool IsUse;//标记这块Span是否被使用

	size_t ObjectSize;//每个切好的小块内存空间大小

	Span() :_PageID(0), _Num(0), _next(nullptr), _prev(nullptr), use_count(0), FreeList(nullptr), IsUse(false), ObjectSize(0) {}
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

	Span* Pop() {
		Span* front = _head->_next;//带头双向循环链表
		Erase(_head->_next);
		return front;
	}

	void Erase(Span* pos) {//删除SpanList上的节点
		assert(pos != _head);
		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

	Span* begin() { return _head->_next; }
	Span* end() { return _head; }

	bool Empty() { return _head->_next == _head; }
};