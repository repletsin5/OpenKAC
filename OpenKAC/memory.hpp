#pragma once
#include <Driver.hpp>

extern "C"
{
#include <ntddk.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
}
#define	KAC_BC_POOL_ALLOC_FAILED 0x02
#define	KAC_BC_TERMINATE 0xff
#define RT_TAG 'okac'
#pragma code_seg(push)
#pragma code_seg("PAGE")
template <typename T>
inline static T* __cdecl Allocate(size_t size, POOL_FLAGS flags) {
	PAGED_CODE();
	if (size == 0) size = 1;

	if (auto memptr = (T*)ExAllocatePool2(flags, size * sizeof(T), RT_TAG))
	{
		return memptr;
	}
	KdBreakPoint();
	KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED, KAC_BC_POOL_ALLOC_FAILED, 0, 0, 0);
}

template <typename T>
inline static T* __cdecl Allocate(POOL_FLAGS flags) {
	PAGED_CODE();

	if (auto memptr = (T*)ExAllocatePool2(flags, sizeof(T), RT_TAG))
	{
		return memptr;
	}
	KdBreakPoint();
	KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED, KAC_BC_POOL_ALLOC_FAILED, 0, 0, 0);
}
template <typename T>

inline static void __cdecl Free(T* ptr) {
	PAGED_CODE();
	if (ptr == 0) return;

	ExFreePoolWithTag((void*)ptr, RT_TAG);

}
#pragma code_seg(pop)