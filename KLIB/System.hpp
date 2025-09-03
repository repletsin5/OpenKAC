#pragma once
#include "std_includes.h"
#include "KString.hpp"

EXTERN_C NTSYSAPI void* RtlLookupFunctionEntry(
	DWORD64               ControlPc,
	PDWORD64              ImageBase,
	PUNWIND_HISTORY_TABLE HistoryTable
);


typedef struct _EX_PUSH_LOCK
{
	union
	{
		struct
		{
			ULONGLONG Locked : 1;                                             //0x0
			ULONGLONG Waiting : 1;                                            //0x0
			ULONGLONG Waking : 1;                                             //0x0
			ULONGLONG MultipleShared : 1;                                     //0x0
			ULONGLONG Shared : 60;                                            //0x0
		};
		ULONGLONG Value;                                                    //0x0
		VOID* Ptr;                                                          //0x0
	};
} __EX_PUSH_LOCK, * P__EX_PUSH_LOCK;

struct _DEVICE_MAP
{
	struct _OBJECT_DIRECTORY* DosDevicesDirectory;                          //0x0
	struct _OBJECT_DIRECTORY* GlobalDosDevicesDirectory;                    //0x8
	VOID* DosDevicesDirectoryHandle;                                        //0x10
	volatile LONG ReferenceCount;                                           //0x18
	ULONG DriveMap;                                                         //0x1c
	UCHAR DriveType[32];                                                    //0x20
	void* ServerSilo;                                               //0x40
};

typedef struct _OBJECT_DIRECTORY_ENTRY
{
	struct _OBJECT_DIRECTORY_ENTRY* ChainLink;                              //0x0
	VOID* Object;                                                           //0x8
	ULONG HashValue;                                                        //0x10
}OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

//0x158 bytes (sizeof)
typedef struct _OBJECT_DIRECTORY
{
	struct _OBJECT_DIRECTORY_ENTRY* HashBuckets[37];                        //0x0
	EX_PUSH_LOCK Lock;                                              //0x128
	struct _DEVICE_MAP* DeviceMap;                                          //0x130
	struct _OBJECT_DIRECTORY* ShadowDirectory;                              //0x138
	VOID* NamespaceEntry;                                                   //0x140
	VOID* SessionObject;                                                    //0x148
	ULONG Flags;                                                            //0x150
	ULONG SessionId;                                                        //0x154

} OBJECT_DIRECTORY, * POBJECT_DIRECTORY;

namespace klib {
	namespace std {
		template<typename T>
		class array;
	}
	typedef struct _SysModule {
		UINT64 addr;
		ULONG size;
		UINT64 FindPattern(const char* bMask, const char* szMask);
		bool IsWithinModule(UINT64 addr) {
			return ((UINT64)addr < this->addr || (UINT64)addr >(this->addr + this->size));
		}

		template<typename T>
		T GetFunction(char* name) {
			return T(0);
		}

	} SysModule, * PSysModule;

	class Modules {
	public:
		static __forceinline INT64 GetOffset(UINT64 start, short instruLen)
		{
			return *(INT32*)(start + instruLen);

		}
		static UINT64 FindPattern(PSysModule mod, const char* bMask, const char* szMask);
		static UINT64 FindPatternIDA(klib::PSysModule mod, const char* pattern, size_t patternSize);
		static void GetSystemModuleBase(IN OUT PSysModule mod, IN const wchar_t* name);
		static void GetDriverObjecFromDeviceName(IN OUT PDRIVER_OBJECT* obj, IN klib::ukString& name);
		static void GetDriverObjects(IN OUT klib::std::array<PDRIVER_OBJECT> &objs);

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

};
