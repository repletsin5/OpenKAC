#pragma once
#define WDF_DEVICE_NO_WDMSEC_H
extern "C" {
#include <ntifs.h>
#include <ntdef.h>
#include <wdf.h>
#include <ntimage.h>
    typedef struct _KLDR_DATA_TABLE_ENTRY
    {
        _LIST_ENTRY InLoadOrderLinks;                                    //0x0
        VOID* ExceptionTable;                                                   //0x8
        ULONG ExceptionTableSize;                                               //0xc
        VOID* GpValue;                                                          //0x10
        _NON_PAGED_DEBUG_INFO* NonPagedDebugInfo;                        //0x14
        VOID* DllBase;                                                          //0x18
        VOID* EntryPoint;                                                       //0x1c
        ULONG SizeOfImage;                                                      //0x20
        _UNICODE_STRING FullDllName;                                     //0x24
        _UNICODE_STRING BaseDllName;                                     //0x2c
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
    } KLDR_DATA_TABLE_ENTRY, *PKLDR_DATA_TABLE_ENTRY;
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
}
//#include "Driver.hpp"
//#include "Trace.h"
//#include "driver.tmh"

namespace OpenKAC {
    typedef struct _SysModule {
        UINT64 addr;
        ULONG size;
    } SysModule, *PSysModule;

	class AntiCheat {

	public:
        static UINT64 FindPattern(PSysModule mod,const char* bMask,const char* szMask);
		AntiCheat();
		~AntiCheat();
		inline static OpenKAC::AntiCheat* ac = nullptr;

		PEPROCESS GetProcess();
		void SetProcess(PEPROCESS);
		void CheckWdFilter();
		void CreateThread();
		void StripHandles();
		static OB_PREOP_CALLBACK_STATUS ObPreOperation(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION PreInfo);
		static void ObPostOperation(PVOID RegistrationContext, POB_POST_OPERATION_INFORMATION PostInfo);
		static void GetSystemModuleBase(IN OUT PSysModule mod, IN const wchar_t* name);
	private:
		PETHREAD acThread = {};
		HANDLE acThreadHandle = {};

		PEPROCESS proc = 0;



	};
}