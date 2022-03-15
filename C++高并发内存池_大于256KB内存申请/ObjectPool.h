#pragma once

#include"Common.h"

template<class T>//定长内存池
class ObjectPool {
private:
	char* _memory;//指向大块内存的指针
	//返回的内存用链式结构管理
	void* _freeList;
	size_t _overage;//大块内存剩余空间大小
public:
	ObjectPool() :_memory(nullptr), _freeList(nullptr),_overage(0) {}

	T* New() {
		T* obj = nullptr;
		if (_freeList != nullptr) {
			//优先把归还的内存重复利用
			//链表头删
			void* next = *((void**)_freeList);
			obj = (T*)_freeList;
			_freeList = next;
		}
		else
		{
			if (_overage < sizeof(T)) {
				_overage = 100 * 1024;
				_memory = (char*)malloc(_overage);
				if (_memory == nullptr) {
					throw std::bad_alloc();
				}
			}
			obj = (T*)_memory;
			size_t SizeT = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
			_memory += SizeT;
			_overage -= SizeT;
		}
		//调用定位new进程空间初始化
		new(obj)T;
		return obj;
	}

	void Delete(T* obj) {
		obj->~T();//显示调用析构函数，清理对象
		//头插
		*(void**)obj = _freeList;
		_freeList = obj;
	}
};