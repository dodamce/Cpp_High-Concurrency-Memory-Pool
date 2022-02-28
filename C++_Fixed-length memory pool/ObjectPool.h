#pragma once

#include"Common.h"

template<class T>//�����ڴ��
class ObjectPool {
private:
	char* _memory;//ָ�����ڴ��ָ��
	//���ص��ڴ�����ʽ�ṹ����
	void* _freeList;
	size_t _overage;//����ڴ�ʣ��ռ��С
public:
	ObjectPool() :_memory(nullptr), _freeList(nullptr),_overage(0) {}

	T* New() {
		T* obj = nullptr;
		if (_freeList != nullptr) {
			//���Ȱѹ黹���ڴ��ظ�����
			//����ͷɾ
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
		//���ö�λnew���̿ռ��ʼ��
		new(obj)T;
		return obj;
	}

	void Delete(T* obj) {
		obj->~T();//��ʾ���������������������
		//ͷ��
		*(void**)obj = _freeList;
		_freeList = obj;
	}
};