#pragma once

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
#define RT_TAG 'kLiB'
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
inline static bool isValidKernelAddress(UINT64 address) {
	
	return !(address == 0 || address == 0xCCCCCCCCCCCCCCCC || address == 0xFFFFFFFFFFFFFFFF || !(address >= 0x400000 || address < 0x7FFFFFFFFFFFFFFF));


}
inline static bool isValidUsermodeAddress(UINT64 address) {

	return !(address == 0 || address == 0xCCCCCCCCCCCCCCCC || address == 0xFFFFFFFFFFFFFFFF || !(address <= 0x400000 || address > 0x7FFFFFFFFFFFFFFF));


}
#pragma code_seg(pop)