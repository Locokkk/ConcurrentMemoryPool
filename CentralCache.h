#pragma once

#include"Common.h"
#include"PageCache.h"

class CentralCache
{
public:
	//从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);//num是对象数,size是字节数

	//将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t size);

	//从spanlist或者page cache获取一个span
	Span* GetOneSpan(size_t size);
private:
	SpanList _spanLists[NFREELIST];
};

static CentralCache centralcacheInst;//静态区，多个线程共享
