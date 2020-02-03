#pragma once

#include"Common.h"

class ThreadCache
{
public:
	//申请内存和释放内存
	void* Allocte(size_t size);
	void Deallocte(void* ptr, size_t size);

	//从中心缓存获取对象
	//这个接口用于threadcache中没有内存，去centralcache中申请，此时要加锁，而且是批量申请
	void* FetchFromCentralCache(size_t index);
	//如果自由链表中对象超过一定长度就要释放给中心缓存
	void ListTooLong(FreeList& freeList, size_t num, size_t size);

private:
	FreeList _freeLists[NFREELIST];

	//ThreadCache* _next;
	//int threadid;
};

//ThreadCache* tclist = nullptr;
//线程TLS Thread Local Storage

//static ThreadCache* pThreadCache = nullptr;//结果是只有一个线程
_declspec (thread) static ThreadCache* pThreadCache = nullptr;//实现一个静态的TLS，结果是有两个线程
//静态的全局变量
