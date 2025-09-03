#pragma once
extern "C" {
#include "Helpers.h"
}
namespace OpenKAC {
	typedef struct _SysModule {
		UINT64 addr;
		ULONG size;
		UINT64 FindPattern(const char* bMask, const char* szMask);
		bool IsWithinModule(UINT64 addr) {
			return ((UINT64)addr > this->addr && (UINT64)addr <(this->addr + this->size));
		}
		template<typename T>
		T GetFunction(char* name) {

		}
	} SysModule, * PSysModule;
	class Modules {
	public:
		static __forceinline INT64 GetOffset(UINT64 start, short instruLen)
		{
			return *(INT32*)(start + instruLen);

		}
		static UINT64 FindPattern(PSysModule mod, const char* bMask, const char* szMask);
		static void GetSystemModuleBase(IN OUT PSysModule mod, IN const wchar_t* name);

		//refrerence https://www.unknowncheats.me/forum/3238153-post14.html -- fixed issue with it
		static inline UINT64 GetNtosImageBase(ULONG* Size)
		{
			UINT64 NtosImageBase;
			UNWIND_HISTORY_TABLE tbl;
			RtlLookupFunctionEntry((DWORD64)&MmCopyMemory, (PDWORD64)&NtosImageBase, &tbl);
			*Size = ((IMAGE_NT_HEADERS64*)((UINT64)NtosImageBase + ((IMAGE_DOS_HEADER*)NtosImageBase)->e_lfanew))->OptionalHeader.SizeOfImage;
			return NtosImageBase;
		}
	};

}