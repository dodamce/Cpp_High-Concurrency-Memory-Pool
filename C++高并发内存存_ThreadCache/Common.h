#pragma once

#include<iostream>
#include<vector>
#include<time.h>
#include<assert.h>
#include<thread>

using std::cout; using std::endl;

static const size_t MAX_BYTE = 126 * 1024;//����߳����볬��126KB����ֱ����ThreadCache����ռ�

static const size_t NUMLIST = 208;//ThreadCache�й�ϣͰ�ĸ���

//��ȡ����������һ���ڵ�
void*& NextObj(void* obj) {
	return *(void**)obj;
}

//�����зֺ��ڴ����������
class FreeList {
public:
	FreeList() :_freeList(nullptr) {}
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

	bool Empty() { return _freeList == nullptr; }
private:
	void* _freeList;
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