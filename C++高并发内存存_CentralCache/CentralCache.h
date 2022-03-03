#pragma once

#include"Common.h"

//中心缓存只有一个，每个线程都可以看见，所以中心缓存类采用单例模式(饿汉模式)
class CentralCache {
private:
	SpanList _SpanList[NUMLIST];//与ThreadCache桶的大小相同
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
	//static修饰的静态变量如果在头文件中初始化,则该头文件被包含多少次,这个静态变量就会定义多少次.编译的话可能会出现重定义的问题.
	//如果是在源文件中,是只能被当前文件看到.但是定义在头文件中,哪个源文件包含,哪个源文件就可以看到
public:
	CentralCache() {}
	static CentralCache* GetCentralCache() {
		return &_sInst;
	}

	//获取一个非空Span,需要向PageCache申请空间，先空开
	Span* GetSpan(SpanList& List, size_t size);

	size_t GiveThreadCache(void*& start, void*& end, size_t Num, size_t Size);
};