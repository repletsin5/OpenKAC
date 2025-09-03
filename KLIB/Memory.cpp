#include "Memory.hpp"
#pragma code_seg("PAGE")
#pragma push("new")
void* __cdecl operator new(size_t size) {
	PAGED_CODE();
	if (size == 0) size = 1;
	
	if (auto memptr = ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, size, RT_TAG))
	{
		return memptr;
	}
	KdBreakPoint();
	KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED, KAC_BC_POOL_ALLOC_FAILED,0,0,0);
}



void* __cdecl operator new[](size_t size) {
	PAGED_CODE();

	if (size == 0) size = 1;

	if (auto memptr = ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, size, RT_TAG))
	{
		return memptr;
	}
	KdBreakPoint();
	KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED, KAC_BC_POOL_ALLOC_FAILED, 0, 0, 0);
}

#pragma pop("new")
#pragma push("delete")

void __cdecl operator delete(void* memptr)
{
	PAGED_CODE();

	if (memptr == nullptr) return;
	ExFreePoolWithTag(memptr,RT_TAG);
}
void __cdecl operator delete(void* memptr,size_t)
{
	PAGED_CODE();

	if (memptr == nullptr) return;
	ExFreePoolWithTag(memptr, RT_TAG);
}


void __cdecl operator delete[](void* memptr)
{
	PAGED_CODE();

	if (memptr == nullptr) return;
	ExFreePoolWithTag(memptr, RT_TAG);
}
#pragma pop("delete")
extern "C" void __cdecl __std_terminate()
{
	PAGED_CODE();

	KdBreakPoint();
	KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED, KAC_BC_TERMINATE, 0, 0, 0);
}