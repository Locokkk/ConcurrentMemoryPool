#include"CentralCache.h"

//从spanlist或者page cache获取一个span，优先从spanlist中获取
Span* CentralCache::GetOneSpan(size_t size)
{
	//获取一个有对象的Span
	size_t index = SizeClass::ListIndex(size);//计算下标
	//通过下标遍历SpanList,找到对应的Span
	SpanList& spanlist = _spanLists[index];
	Span* it = spanlist.Begin();
	while (it != spanlist.End())
	{
		if (!it->_freelist.Empty())
		{
			//找到直接取
			return it;//当前Span
		}
		else
		{
			it = it->_next;
		}
	}

	//没有Span，向PageCache获取
	size_t numpage = SizeClass::NumMovePage(size);
	Span* span = pageCacheInst.NewSpan(numpage);
	//把span对象切成对应大小挂到span的freelist中

	char* start = (char*)(span->_pageid <<12);
	char* end = start + (span->_pagesize << 12);
	while (start < end)
	{
		char* obj = start;
		start += size;

		span->_freelist.Push(obj);
	}
	span->_objSize = size;
	spanlist.PushBack(span);

	return span;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size)//num是对象数,size是字节数
{
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	Span* span = GetOneSpan(size);//上面的代码保证这里拿到的是切好的对象
	FreeList& freelist = span->_freelist;
	size_t actualNum = freelist.PopRange(start, end, num);
	span->_use_count += actualNum;

	spanlist.Unlock();

	return actualNum;
}

void CentralCache::ReleaseListToSpans(void * start, size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	//这里给一个map<ptr, span*>,要还的对象的指针直接映射要还的span
	//再优化：map<pageid, span*>,不用知道这个对象属于哪个span，直接根据页号算出对应span,不用知道这个span在哪个list或是哪个地方，直接根据指针挂到list中
	while (start)
	{
		void* next = NextObj(start);//注意next的提前保存，避免头插后找不到原有的next
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		Span* span = pageCacheInst.GetIdToSpan(id);
		span->_freelist.Push(start);
		span->_use_count--;

		//表示当前span切出去的对象全部返回，可以将span还给page cache，进行合并，减少内存碎片问题
		if (span->_use_count == 0)
		{
			size_t index = SizeClass::ListIndex(span->_objSize);
			_spanLists[index].Erase(span);
			span->_freelist.Clear();//防止切过的内存再切再挂一次
			pageCacheInst.ReleaseSpanToPageCache(span);
		}
		start = next;
	}

	spanlist.Unlock();
}
