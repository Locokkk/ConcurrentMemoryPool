#include"ThreadCache.h"
#include"CentralCache.h"

void* ThreadCache::Allocte(size_t size)
{
	size_t index = SizeClass::ListIndex(size);//Index(size);
	FreeList& freelist = _freeLists[index];
	if (!freelist.Empty())//不直接操作指针，而是用类进行封装再判断
	{//有内存
		return freelist.Pop();
	}
	else
	{//无内存，去centralcache申请
		//size_t num = 20;//以后调整，和单个对象的大小有关
		return FetchFromCentralCache(SizeClass::RoundUp(size));//num控制批量申请,start接受申请的新空间（链表）,对齐后再传过去
	}
}

void ThreadCache::Deallocte(void* ptr, size_t size)
{//还的内存挂起来
	size_t index = SizeClass::ListIndex(size);//?
	FreeList& freeList = _freeLists[index];
	freeList.Push(ptr);

	//释放的两个时机：对象个数满足一定条件  内存大小
	size_t num = SizeClass::NumMoveSize(size);
	if (freeList.Num() >= num)
	{
		//链得太多了要释放给centralcache
		ListTooLong(freeList, num, size);
	}
}

void ThreadCache::ListTooLong(FreeList& freeList, size_t num, size_t size)
{
	void* start = nullptr, *end = nullptr;
	freeList.PopRange(start, end, num);

	NextObj(end) = nullptr;//删掉的一段尾要置空处理
	centralcacheInst.ReleaseListToSpans(start, size);
}

////独立测试thread cache
//void* ThreadCache::FetchFromCentralCache(size_t size)
//{
//	//从centralcache批量申请空间，返回的时候是链起来返回的，num控制批量申请
//	size_t num = 20;//以后调整，这个接口先暂时从centralcache获取一个对象
//
//	//模拟取内存对象的代码，测试ThreadCache的逻辑
//	size_t size = (index + 1) * 8;//单个对象大小
//	char* start = (char*)malloc(size*num);//num是对象个数
//	char* cur = start;
//	for (size_t i = 0; i < num; ++i)
//	{//开的num个空间要链起来返回
//		char* next = cur + size;
//		NextObj(cur) = next;
//
//		cur = next;
//	}
//
//	NextObj(cur) = nullptr;//最后一个位置置空
//
//	void* head = NextObj(start);
//	void* tail = cur;
//
//	_freeLists[index].PushRange(head, tail);
//
//	return start;
//}


//独立测试thread cache
void* ThreadCache::FetchFromCentralCache(size_t size)
{
	size_t num = SizeClass::NumMoveSize(size);

	//从CentralCache中获取内存
	void* start = nullptr, *end = nullptr;
	size_t actualNum = centralcacheInst.FetchRangeObj(start, end, num, size);//num要的num,actualNum实际返回的num

	if (actualNum == 1)
	{
		return start;
	}
	else
	{
		size_t index = SizeClass::ListIndex(size);
		FreeList& list = _freeLists[index];
		list.PushRange(NextObj(start), end, actualNum - 1);

		return start;
	}
}
