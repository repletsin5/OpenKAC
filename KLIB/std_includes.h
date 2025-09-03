#ifndef STD_INC_H
#define STD_INC_H
#define LONESHA256_STATIC
#ifdef __cplusplus
#include <cstdarg>
extern "C" {
#else
#include <stdarg.h>
#endif // __cplusplus


#include <ntifs.h>
#include <ntdef.h>
#include <wdf.h>
#include <wdm.h>
#include <ntimage.h>
#include <ntstrsafe.h>
typedef struct _KLDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;                                    //0x0
	VOID* ExceptionTable;                                                   //0x8
	ULONG ExceptionTableSize;                                               //0xc
	VOID* GpValue;                                                          //0x10
	NON_PAGED_DEBUG_INFO* NonPagedDebugInfo;                        //0x14
	VOID* DllBase;                                                          //0x18
	VOID* EntryPoint;                                                       //0x1c
	ULONG SizeOfImage;                                                      //0x20
	UNICODE_STRING FullDllName;                                     //0x24
	UNICODE_STRING BaseDllName;                                     //0x2c
	ULONG Flags;                                                            //0x34
	USHORT LoadCount;                                                       //0x38
	union
	{
		USHORT SignatureLevel : 4;                                            //0x3a
		USHORT SignatureType : 3;                                             //0x3a
		USHORT Unused : 9;                                                    //0x3a
		USHORT EntireField;                                                 //0x3a
	} u1;                                                                   //0x3a
	VOID* SectionPointer;                                                   //0x3c
	ULONG CheckSum;                                                         //0x40
	ULONG CoverageSectionSize;                                              //0x44
	VOID* CoverageSection;                                                  //0x48
	VOID* LoadedImports;                                                    //0x4c
	VOID* Spare;                                                            //0x50
	ULONG SizeOfImageNotRounded;                                            //0x54
	ULONG TimeDateStamp;                                                    //0x58
} KLDR_DATA_TABLE_ENTRY, * PKLDR_DATA_TABLE_ENTRY;
typedef struct _MP_DRIVER_INFO
{
	LIST_ENTRY DriverInfoList;
	UNICODE_STRING ImageName;
	UNICODE_STRING DriverRegistryPath;
	UNICODE_STRING CertPublisher;
	UNICODE_STRING CertIssuer;
	PVOID ImageHash;
	INT ImageHashAlgorithm;
	INT ImageHashLength;
	PVOID CertThumbprint;
	INT ThumbprintHashAlgorithm;
	INT CertificateThumbprintLength;
	PVOID ImageBase;
	INT64 ImageSize;
	INT ImageFlags;
	INT DriverClassification;
	INT ModuleEntryEnd;
} MP_DRIVER_INFO, * PMP_DRIVER_INFO;
#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _UNWIND_HISTORY_TABLE_ENTRY {
	ULONG_PTR ImageBase;
	void* FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, * PUNWIND_HISTORY_TABLE_ENTRY;
typedef struct _UNWIND_HISTORY_TABLE {

	DWORD Count;
	BYTE  LocalHint;
	BYTE  GlobalHint;
	BYTE  Search;
	BYTE  Once;
	ULONG_PTR LowAddress;
	ULONG_PTR HighAddress;
	UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];

} UNWIND_HISTORY_TABLE, * PUNWIND_HISTORY_TABLE;

#ifdef __cplusplus
}
#endif
typedef unsigned long DWORD;
typedef unsigned char BYTE, * PBYTE;

#if DBG
#define	kdprint(...) _kdprint(__VA_ARGS__)


inline ULONG _kdprint(PCCH s, ...) {
	va_list(args);
	va_start(args, s); 
	auto ret = vDbgPrintExWithPrefix("[OpenKAC] ", 0, 0, s, args);
	va_end(args);
	return ret;
}
#else
#define	kdprint(...)
#endif
#endif