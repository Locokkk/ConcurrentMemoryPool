#pragma once

#include"ThreadCache.h"
#include"PageCache.h"

void* ConcurrentMalloc(size_t size)
{
	if (size <= MAX_SIZE)  //[1byte, 64kb]
	{
		if (pThreadCache == nullptr)
		{
			pThreadCache = new ThreadCache;
			cout << std::this_thread::get_id() << "->" << pThreadCache << endl;//获取各个线程的ID
		}
		return pThreadCache->Allocte(size);
	}
	else if (size <= ((MAX_PAGES-1) << PAGE_SHIFT))  //(64kb, 128*4kb]
	{
		size_t align_size = SizeClass::_RoundUp(size, 1<<PAGE_SHIFT);
		size_t pagenum = (align_size >> PAGE_SHIFT);
		Span* span = pageCacheInst.NewSpan(pagenum);
		span->_objSize = align_size;
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else  //[128*4kb, -]
	{
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT);
		size_t pagenum = (align_size >> PAGE_SHIFT);
		return SystemAlloc(pagenum);
	}
}

void ConcurrentFree(void* ptr)
{
	size_t pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = pageCacheInst.GetIdToSpan(pageid);
	if (span == nullptr)   //[128*4kb, -]
	{
		SystemFree(ptr);
		return;
	}
	size_t size = span->_objSize;
	if (size <= MAX_SIZE)  //[1byte, 64kb]
	{
		pThreadCache->Deallocte(ptr, size);
	}
	else if (size <= ((MAX_PAGES - 1) << PAGE_SHIFT))  //(64kb, 128*4kb]
	{
		pageCacheInst.ReleaseSpanToPageCache(span);
	}
}
