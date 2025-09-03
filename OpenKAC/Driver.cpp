/*++

Module Name:

	driver.c

Abstract:

	This file contains the driver entry points and callbacks.

Environment:

	Kernel-mode Driver Framework

--*/

// greate writeup https://secret.club/2020/04/13/how-anti-cheats-detect-system-emulation.html
#define NT_DEVICE_NAME      L"\\Device\\OpenKAC"
#define DOS_DEVICE_NAME     L"\\DosDevices\\OpenKAC"


#include "driver.hpp"
#include "ntoskrnl.hpp"

//#include "driver.tmh"
#include "ioctls.hpp"
extern "C" {
#include <intsafe.h>
#include <handleapi.h>
#include "BlackBone/BlackBoneDef.h"
#include "BlackBone/Remap.h"
#include "BlackBone/Loader.h"
#include "BlackBone/Utils.h"
#include "BlackBone/Routines.h"
}
#include <skCrypter.h>
#include "AntiCheat.hpp"
#include "DriverVerification.hpp"
#include "Integrity.hpp"
#include <runtime/StackMaster.hpp>
#include <runtime/Helpers.hpp>
#include <intrin.h>
#include <stdlib.h>
#include <Solar/Hardware/Solar_PCIE.hpp>
//#include <corecrt_startup.h>
extern "C" DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH CreateClose;

DRIVER_UNLOAD UnloadDriver;

extern "C" NTSTATUS BBInitDynamicData(IN OUT PDYNAMIC_DATA pData);
extern "C" NTSTATUS BBScanSection(IN const char* section, IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len, OUT PVOID* ppFound);
extern "C" NTSTATUS BBGetBuildNO(OUT PULONG pBuildNo);

#pragma alloc_text(INIT, BBInitDynamicData)
#pragma alloc_text(INIT, BBScanSection)
#pragma alloc_text(INIT, BBGetBuildNO)

extern "C" {
    DYNAMIC_DATA dynData;
}
extern "C" NTSTATUS NTAPI MmCopyVirtualMemory(PEPROCESS srcProc, PVOID srcAddr, PEPROCESS targProc,
	PVOID targAddr, SIZE_T bufSize, KPROCESSOR_MODE prevMode, PSIZE_T retSize);

PVOID registrationHandle = NULL;
TD_CALLBACK_REGISTRATION  CBCallbackRegistration = { 0 };

inline bool CheckMSR() {
	__try {
		__readmsr(0x40000000);
	}
	__except (1) {
		return false;
	}
	KdPrintEx((0, 0, "[OpenKAC] VM detected\n"));
	return true;
}
KGUARDED_MUTEX TdCallbacksMutex;
extern "C"
{


    typedef void(__cdecl* _PVFV)(void);
    typedef int(__cdecl* _PIFV)(void);
    typedef void(__cdecl* _PVFI)(int);

    static void __cdecl _initterm(
        _In_reads_(_Last - _First) _In_ _PVFV* _First,
        _In_                            _PVFV* _Last
    ) {
        while (_First < _Last)
        {
            if (*_First != 0)
                (**_First)();
            ++_First;
        }
    }

    static int  __cdecl _initterm_e(
        _In_reads_(_Last - _First)      _PIFV* _First,
        _In_                            _PIFV* _Last
    ) {
        int ret = 0;

        while (_First < _Last && ret == 0)
        {
            // if current table entry is non-NULL, call thru it.
            if (*_First != 0)
                ret = (**_First)();
            ++_First;
        }

        return ret;
    }


}
//#pragma const_seg(".CRT$XCA")
//	_PVFV __xc_a = 0;
//#pragma const_seg(".CRT$XCZ")
//	_PVFV __xc_z = 0;
//
//#pragma const_seg(".CRT$XIA")
//	_PIFV __xi_a = 0;
//#pragma const_seg(".CRT$XIZ")
//	_PIFV __xi_z = 0;
//
//
//#pragma const_seg(".CRT$XCA")
//    _PVFV __xc_a = 0;
//#pragma const_seg(".CRT$XCZ")
//    _PVFV __xc_z = 0;
//
//#pragma const_seg(".CRT$XIA")
//    _PIFV __xt_a = 0;
//#pragma const_seg("..CRT$XTZ")
//    _PIFV __xt_z = 0;


#pragma const_seg(".CRT$XIA")
    extern "C" _PIFV __xi_a = 0;
#pragma const_seg(".CRT$XIZ")
    extern "C" _PIFV __xi_z = 0;    /* C initializers */
#pragma const_seg(".CRT$XCA") 
    extern "C" _PVFV __xc_a = 0;
#pragma const_seg(".CRT$XCZ")
    extern "C" _PVFV __xc_z = 0;    /* C++ initializers */
#pragma const_seg(".CRT$XPA") 
    extern "C" _PIFV __xp_a = 0;
#pragma const_seg(".CRT$XPZ") 
    extern "C" _PIFV __xp_z = 0;    /* C pre-terminators */
#pragma const_seg(".CRT$XTA") 
    extern "C" _PVFV __xt_a = 0;
#pragma const_seg(".CRT$XTZ") 
    extern "C" _PVFV __xt_z = 0;    /* C terminators */

#pragma code_seg(push)
#pragma code_seg("INIT")
NTSTATUS
main(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

extern "C" {
	NTSTATUS DriverEntry(
		_In_ PDRIVER_OBJECT  DriverObject,
		_In_ PUNICODE_STRING RegistryPath
	) {
		/*
	 * do initializations
	 */
		int initret = _initterm_e(&__xi_a, &__xi_z);
        if (initret != 0) {
            KdPrintEx((0, 0, "[OpenKAC] _initterm_e failed with %d\n", initret));
            return STATUS_FAILED_DRIVER_ENTRY;
        }

		/*
		 * do C++ initializations
		 */
		_initterm(&__xc_a, &__xc_z);
		return main(DriverObject, RegistryPath);
	}
}
NTSTATUS
main(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	KMUTEX  acLock = {};
	UNREFERENCED_PARAMETER(acLock);
	NTSTATUS status;
	UNICODE_STRING  ntUnicodeString;
	UNICODE_STRING  ntWin32NameString;
	PDEVICE_OBJECT  deviceObject = NULL;
	UNICODE_STRING altitude = { 0 };
	OB_CALLBACK_REGISTRATION  obRegistration = { };
	OB_OPERATION_REGISTRATION operationRegistrations[3] = { { },{ } };
	UNREFERENCED_PARAMETER(RegistryPath);
	PAGED_CODE();
	RtlInitUnicodeString(&ntUnicodeString, NT_DEVICE_NAME);
	//
	// Initialize WPP Tracing
	//
	//WPP_INIT_TRACING(DriverObject, RegistryPath); // This broke cba too fix

	KdPrintEx((0, 0, "%s Entry\n", __FUNCTION__));
	InitializeDebuggerBlock();
	status = BBInitDynamicData(&dynData);

	status = klib::IoCreateDevice(
		DriverObject,                   // Our Driver Object
		0,                              // We don't use a device extension
		&ntUnicodeString,               // Device name
		FILE_DEVICE_UNKNOWN,            // Device type
		FILE_DEVICE_SECURE_OPEN,     // Device characteristics
		FALSE,                          // Not an exclusive device
		&deviceObject);                // Returned ptr to Device Object

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(0, 0, "IoCreateDevice failed %lu", status);
		//KdPrintEx((0, 0, "IoCreateDevice failed %lu", status));
		//WPP_CLEANUP(DriverObject);
		return status;
	}
	status = BBInitLdrData((PKLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection);
	if (!NT_SUCCESS(status))
		return status;
	//MSR hv detect
	KdPrintEx((0, 0, "[OpenKAC] reading MSR\n"));

	//OpenKAC::Integrity::InitHashGen();
	if (CheckMSR()) {
		//  IoDeleteDevice(deviceObject);
	}
	//
    // Globals init
    //
	InitializeListHead(&g_PhysProcesses);
	RtlInitializeGenericTableAvl(&g_ProcessPageTables, &AvlCompare, &AvlAllocate, &AvlFree, NULL);
	KeInitializeGuardedMutex(&g_globalLock);
    OpenKAC::Solar::PCIE::PCIE_Device::PCIE_Enumorate();
	status = PsSetCreateProcessNotifyRoutine(BBProcessNotify, FALSE);
	if (!NT_SUCCESS(status))
	{
		DPRINT("BlackBone: %s: Failed to setup notify routine with staus 0x%X\n", __FUNCTION__, status);
		return status;
	}

	SetFlag(deviceObject->Flags, DO_BUFFERED_IO);

	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctls::DeviceControl;
	DriverObject->DriverUnload = UnloadDriver;

	ClearFlag(deviceObject->Flags, DO_DEVICE_INITIALIZING);
	ULONG64 EnabledFeatures;  EnabledFeatures = RtlGetEnabledExtendedFeatures(-1);
	if ((EnabledFeatures & XSTATE_MASK_GSSE) == 0) {
		//TODO show to usermode
		KdPrintEx((0, 0, "No AVX support\n"));
		return STATUS_SUCCESS;
	}

	//KeInitializeMutex(&acLock, 0);
	//status =
	//    KeWaitForMutexObject(
	//        &acLock,
	//        Executive,
	//        KernelMode,
	//        FALSE,
	//        0);
    OpenKAC::AntiCheat::ac = new OpenKAC::AntiCheat();
	if (OpenKAC::AntiCheat::ac == nullptr) {
		KdBreakPoint();
		KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED, 0, 0, 0, 0);
	}
	//KeAcquireGuardedMutex(&TdCallbacksMutex);
	RtlInitUnicodeString(&altitude, L"327460");
	operationRegistrations[0].ObjectType = PsProcessType;
	operationRegistrations[0].Operations |= OB_OPERATION_HANDLE_CREATE;
	operationRegistrations[0].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
	operationRegistrations[0].PreOperation = OpenKAC::AntiCheat::ObPreOperation;
	operationRegistrations[0].PostOperation = OpenKAC::AntiCheat::ObPostOperation;

	operationRegistrations[1].ObjectType = PsThreadType;
	operationRegistrations[1].Operations |= OB_OPERATION_HANDLE_CREATE;
	operationRegistrations[1].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
	operationRegistrations[1].PreOperation = OpenKAC::AntiCheat::ObPreOperation;
	operationRegistrations[1].PostOperation = OpenKAC::AntiCheat::ObPostOperation;

	operationRegistrations[2].ObjectType = ExDesktopObjectType;
	operationRegistrations[2].Operations |= OB_OPERATION_HANDLE_CREATE;
	operationRegistrations[2].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
	operationRegistrations[2].PreOperation = OpenKAC::AntiCheat::ObPreOperation;
	operationRegistrations[2].PostOperation = OpenKAC::AntiCheat::ObPostOperation;

	obRegistration.Version = OB_FLT_REGISTRATION_VERSION;
	obRegistration.OperationRegistrationCount = 3;
	obRegistration.Altitude = altitude;
	obRegistration.RegistrationContext = &CBCallbackRegistration;
	obRegistration.OperationRegistration = operationRegistrations;

	RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);

	//status = ObRegisterCallbacks(
	//	&obRegistration,
	//	&registrationHandle       // save the registration handle to remove callbacks later
	//);
	//KeReleaseGuardedMutex(&TdCallbacksMutex);
	OpenKAC::DriverVerification::CheckWdFilter();
	if (!NT_SUCCESS(status))
	{
		//
		// Delete everything that this routine has allocated.
		//
		KdPrintEx((0, 0, "ObRegisterCallbacks failed %x!\n", status));

		//IoDeleteDevice(deviceObject);
		return status;
	}
	status = IoCreateSymbolicLink(
		&ntWin32NameString, &ntUnicodeString);

	if (!NT_SUCCESS(status))
	{
		//
		// Delete everything that this routine has allocated.
		//
		KdPrintEx((0, 0, "IoCreateSymbolicLink failed %x!\n", status));

		IoDeleteDevice(deviceObject);
	}
	OpenKAC::SysModule mod = {};
	OpenKAC::Modules::GetSystemModuleBase(&mod, skCrypt(L"WdFilter.sys"));
	if (mod.addr == 0) {
		KdPrintEx((0, 0, "Cannot get WdFilter.sys module\n"));
	}
	else {
		//OpenKAC::Integrity::CheckHashesOfDriver(&mod, nullptr);

	}
	//TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

	return status;
}
#pragma code_seg(pop)

#pragma code_seg(push)
#pragma code_seg("PAGE")
NTSTATUS
CreateClose(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PAGED_CODE();

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
VOID
UnloadDriver(
	_In_ PDRIVER_OBJECT  DriverObject
)
{
	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
	UNICODE_STRING uniWin32NameString;

	PAGED_CODE();
	//KeAcquireGuardedMutex(&TdCallbacksMutex);

	if (registrationHandle)
		ObUnRegisterCallbacks(registrationHandle);
	//KeReleaseGuardedMutex(&TdCallbacksMutex);

	PsSetCreateProcessNotifyRoutine(BBProcessNotify, TRUE);

	// Cleanup physical regions
	BBCleanupProcessPhysList();

	// Cleanup process mapping info
	BBCleanupProcessTable();

	// Create counted string version of our Win32 device name.
	//

	RtlInitUnicodeString(&uniWin32NameString, DOS_DEVICE_NAME);

	//
	// Delete the link from our device name to a name in the Win32 namespace.
	//

	IoDeleteSymbolicLink(&uniWin32NameString);

	if (deviceObject != NULL)
	{
		IoDeleteDevice(deviceObject);
	}
	//    RtlFreeUnicodeString(&uniWin32NameString);
	if (OpenKAC::AntiCheat::ac != nullptr) {
        OpenKAC::AntiCheat::ac->~AntiCheat();
        OpenKAC::AntiCheat::ac = nullptr;
	}
    //OpenKAC::Integrity::UnInitHashGen();

    int initret = _initterm_e(&__xp_a, &__xp_z);
    if (initret != 0) {
        KdPrintEx((0, 0, "[OpenKAC] _initterm_e failed with %d\n", initret));
    }

    /*
     * do C++ deitializations
     */
    _initterm(&__xt_a, &__xt_z);
    //exit(0);
}

#pragma code_seg(pop)



/// <summary>
/// Find pattern in kernel PE section
/// </summary>
/// <param name="section">Section name</param>
/// <param name="pattern">Pattern data</param>
/// <param name="wildcard">Pattern wildcard symbol</param>
/// <param name="len">Pattern length</param>
/// <param name="ppFound">Found address</param>
/// <returns>Status code</returns>
NTSTATUS BBScanSection(IN const char* section, IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len, OUT PVOID* ppFound)
{
    ASSERT(ppFound != NULL);
    if (ppFound == NULL)
        return STATUS_INVALID_PARAMETER;

    PVOID base = GetKernelBase(NULL);
    if (!base)
        return STATUS_NOT_FOUND;

    PIMAGE_NT_HEADERS pHdr = RtlImageNtHeader(base);
    if (!pHdr)
        return STATUS_INVALID_IMAGE_FORMAT;

    PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(pHdr + 1);
    for (PIMAGE_SECTION_HEADER pSection = pFirstSection; pSection < pFirstSection + pHdr->FileHeader.NumberOfSections; pSection++)
    {
        ANSI_STRING s1, s2;
        RtlInitAnsiString(&s1, section);
        RtlInitAnsiString(&s2, (PCCHAR)pSection->Name);
        if (RtlCompareString(&s1, &s2, TRUE) == 0)
        {
            PVOID ptr = NULL;
            NTSTATUS status = BBSearchPattern(pattern, wildcard, len, (PUCHAR)base + pSection->VirtualAddress, pSection->Misc.VirtualSize, &ptr);
            if (NT_SUCCESS(status))
                *(PULONG)ppFound = (ULONG)((PUCHAR)ptr - (PUCHAR)base);

            return status;
        }
    }

    return STATUS_NOT_FOUND;
}

/// <summary>
/// Get kernel build number
/// </summary>
/// <param name="pBuildNO">Build number.</param>
/// <returns>Status code</returns>
NTSTATUS BBGetBuildNO(OUT PULONG pBuildNo)
{
    ASSERT(pBuildNo != NULL);
    if (pBuildNo == NULL)
        return STATUS_INVALID_PARAMETER;

    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING strRegKey = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion");
    UNICODE_STRING strRegValue = RTL_CONSTANT_STRING(L"BuildLabEx");
    UNICODE_STRING strRegValue10 = RTL_CONSTANT_STRING(L"UBR");
    UNICODE_STRING strVerVal = { 0 };
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES keyAttr = { 0 };

    InitializeObjectAttributes(&keyAttr, &strRegKey, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenKey(&hKey, KEY_READ, &keyAttr);
    if (NT_SUCCESS(status))
    {
        PKEY_VALUE_FULL_INFORMATION pValueInfo =(PKEY_VALUE_FULL_INFORMATION)ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, BB_POOL_TAG);
        ULONG bytes = 0;

        if (pValueInfo)
        {
            // Try query UBR value
            status = ZwQueryValueKey(hKey, &strRegValue10, KeyValueFullInformation, pValueInfo, PAGE_SIZE, &bytes);
            if (NT_SUCCESS(status))
            {
                *pBuildNo = *(PULONG)((PUCHAR)pValueInfo + pValueInfo->DataOffset);
            }else if (status = ZwQueryValueKey(hKey, &strRegValue, KeyValueFullInformation, pValueInfo, PAGE_SIZE, &bytes); NT_SUCCESS(status))
            {
                PWCHAR pData = (PWCHAR)((PUCHAR)pValueInfo->Name + pValueInfo->NameLength);
                for (ULONG i = 0; i < pValueInfo->DataLength; i++)
                {
                    if (pData[i] == L'.')
                    {
                        for (ULONG j = i + 1; j < pValueInfo->DataLength; j++)
                        {
                            if (pData[j] == L'.')
                            {
                                strVerVal.Buffer = &pData[i] + 1;
                                strVerVal.Length = strVerVal.MaximumLength = (USHORT)((j - i) * sizeof(WCHAR));
                                status = RtlUnicodeStringToInteger(&strVerVal, 10, pBuildNo);

                            }
                        }
                    }
                }

            }

            ExFreePoolWithTag(pValueInfo, BB_POOL_TAG);
        }
        else
            status = STATUS_NO_MEMORY;

        ZwClose(hKey);
    }
    else
        DPRINT("BlackBone: %s: ZwOpenKey failed with status 0x%X\n", __FUNCTION__, status);

    return status;

}

/// <summary>
/// Initialize dynamic data.
/// </summary>
/// <param name="pData">Data to initialize</param>
/// <returns>Status code</returns>
NTSTATUS BBInitDynamicData(IN OUT PDYNAMIC_DATA pData)
{
    NTSTATUS status = STATUS_SUCCESS;
    RTL_OSVERSIONINFOEXW verInfo = { 0 };

    if (pData == NULL)
        return STATUS_INVALID_ADDRESS;

    RtlZeroMemory(pData, sizeof(DYNAMIC_DATA));
    pData->DYN_PDE_BASE = PDE_BASE;
    pData->DYN_PTE_BASE = PTE_BASE;

    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&verInfo);

    if (status == STATUS_SUCCESS)
    {
        ULONG ver_short = (verInfo.dwMajorVersion << 8) | (verInfo.dwMinorVersion << 4) | verInfo.wServicePackMajor;
        pData->ver = (WinVer)ver_short;

        DPRINT(
            "[OpenKAC] OS version %d.%d.%d.%d.%d - 0x%x\n",
            verInfo.dwMajorVersion,
            verInfo.dwMinorVersion,
            verInfo.dwBuildNumber,
            verInfo.wServicePackMajor,
            pData->buildNo,
            ver_short
        );

        // Validate current driver version
        pData->correctBuild = TRUE;
#if defined(_WIN7_)
        if (ver_short != WINVER_7 && ver_short != WINVER_7_SP1)
            return STATUS_NOT_SUPPORTED;
#elif defined(_WIN8_)
        if (ver_short != WINVER_8)
            return STATUS_NOT_SUPPORTED;
#elif defined (_WIN81_)
        if (ver_short != WINVER_81)
            return STATUS_NOT_SUPPORTED;
#elif defined (_WIN10_)
        if (ver_short < WINVER_10 || WINVER_10_20H1 < ver_short)
            return STATUS_NOT_SUPPORTED;
#endif


        VOID* p = &pData->ExRemoveTable;
        switch (ver_short)
        {
            // Windows 7
            // Windows 7 SP1
        case WINVER_7:
        case WINVER_7_SP1:
            pData->KExecOpt = 0x0D2;
            pData->Protection = 0x43C;  // Bitfield, bit index - 0xB
            pData->ObjTable = 0x200;
            pData->VadRoot = 0x448;
            pData->NtProtectIndex = 0x04D;
            pData->NtCreateThdExIndex = 0x0A5;
            pData->NtTermThdIndex = 0x50;
            pData->PrevMode = 0x1F6;
            pData->ExitStatus = 0x380;
            pData->MiAllocPage = (ver_short == WINVER_7_SP1) ? 0 : 0;
            if (ver_short == WINVER_7_SP1)
            {
                if (NT_SUCCESS( BBScanSection("PAGE", (PCUCHAR)"\x48\x8D\x56\x20\x48\x8B\x42\x08", 0xCC, 8, (PVOID*)&pData->ExRemoveTable)))
                    pData->ExRemoveTable -= 0x36;
            }
            else
                pData->ExRemoveTable = 0x32D404;
            break;

            // Windows 8
        case WINVER_8:
            pData->KExecOpt = 0x1B7;
            pData->Protection = 0x648;
            pData->ObjTable = 0x408;
            pData->VadRoot = 0x590;
            pData->NtProtectIndex = 0x04E;
            pData->NtCreateThdExIndex = 0x0AF;
            pData->NtTermThdIndex = 0x51;
            pData->PrevMode = 0x232;
            pData->ExitStatus = 0x450;
            pData->MiAllocPage = 0x3AF374;
            pData->ExRemoveTable = 0x487518;
            break;

            // Windows 8.1
        case WINVER_81:
            pData->KExecOpt = 0x1B7;
            pData->Protection = 0x67A;
            pData->EProcessFlags2 = 0x2F8;
            pData->ObjTable = 0x408;
            pData->VadRoot = 0x5D8;
            pData->NtCreateThdExIndex = 0xB0;
            pData->NtTermThdIndex = 0x52;
            pData->PrevMode = 0x232;
            pData->ExitStatus = 0x6D8;
            pData->MiAllocPage = 0;
            pData->ExRemoveTable = 0x432A88; // 0x38E320;
            if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x8D\x7D\x18\x48\x8B", 0xCC, 6, &p)))
                pData->ExRemoveTable -= 0x5E;
            break;

            // Windows 10, build 16299/15063/14393/10586
        case WINVER_10:
            if (verInfo.dwBuildNumber == 10586)
            {
                pData->KExecOpt = 0x1BF;
                pData->Protection = 0x6B2;
                pData->EProcessFlags2 = 0x300;
                pData->ObjTable = 0x418;
                pData->VadRoot = 0x610;
                pData->NtCreateThdExIndex = 0xB4;
                pData->NtTermThdIndex = 0x53;
                pData->PrevMode = 0x232;
                pData->ExitStatus = 0x6E0;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x8D\x7D\x18\x48\x8B", 0xCC, 6, &p)))
                    pData->ExRemoveTable -= 0x5C;
                break;
            }
            else if (verInfo.dwBuildNumber == 14393)
            {
                pData->ver = WINVER_10_RS1;
                pData->KExecOpt = 0x1BF;
                pData->Protection = pData->buildNo >= 447 ? 0x6CA : 0x6C2;
                pData->EProcessFlags2 = 0x300;
                pData->ObjTable = 0x418;
                pData->VadRoot = 0x620;
                pData->NtCreateThdExIndex = 0xB6;
                pData->NtTermThdIndex = 0x53;
                pData->PrevMode = 0x232;
                pData->ExitStatus = 0x6F0;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x8D\x7D\x18\x48\x8B", 0xCC, 6, &p)))
                    pData->ExRemoveTable -= 0x60;
                break;
            }
            else if (verInfo.dwBuildNumber == 15063)
            {
                pData->ver = WINVER_10_RS2;
                pData->KExecOpt = 0x1BF;
                pData->Protection = 0x6CA;
                pData->EProcessFlags2 = 0x300;
                pData->ObjTable = 0x418;
                pData->VadRoot = 0x628;
                pData->NtCreateThdExIndex = 0xB9;
                pData->NtTermThdIndex = 0x53;
                pData->PrevMode = 0x232;
                pData->ExitStatus = 0x6F8;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x8B\x47\x20\x48\x83\xC7\x18", 0xCC, 8, &p)))
                    pData->ExRemoveTable -= 0x34;
                break;
            }
            else if (verInfo.dwBuildNumber == 16299)
            {
                pData->ver = WINVER_10_RS3;
                pData->KExecOpt = 0x1BF;
                pData->Protection = 0x6CA;
                pData->EProcessFlags2 = 0x828;    // MitigationFlags offset
                pData->ObjTable = 0x418;
                pData->VadRoot = 0x628;
                pData->NtCreateThdExIndex = 0xBA;
                pData->NtTermThdIndex = 0x53;
                pData->PrevMode = 0x232;
                pData->ExitStatus = 0x700;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x83\xC7\x18\x48\x8B\x17", 0xCC, 7, &p)))
                    pData->ExRemoveTable -= 0x34;
                break;
            }
            else if (verInfo.dwBuildNumber == 17134)
            {
                pData->ver = WINVER_10_RS4;
                pData->KExecOpt = 0x1BF;
                pData->Protection = 0x6CA;
                pData->EProcessFlags2 = 0x828;    // MitigationFlags offset
                pData->ObjTable = 0x418;
                pData->VadRoot = 0x628;
                pData->NtCreateThdExIndex = 0xBB;
                pData->NtTermThdIndex = 0x53;
                pData->PrevMode = 0x232;
                pData->ExitStatus = 0x700;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x83\xC7\x18\x48\x8B\x17", 0xCC, 7, &p)))
                    pData->ExRemoveTable -= 0x34;
                break;
            }
            else if (verInfo.dwBuildNumber == 17763)
            {
                pData->ver = WINVER_10_RS5;
                pData->KExecOpt = 0x1BF;
                pData->Protection = 0x6CA;
                pData->EProcessFlags2 = 0x820;    // MitigationFlags offset
                pData->ObjTable = 0x418;
                pData->VadRoot = 0x628;
                pData->NtCreateThdExIndex = 0xBC;
                pData->NtTermThdIndex = 0x53;
                pData->PrevMode = 0x232;
                pData->ExitStatus = 0x700;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x83\xC7\x18\x48\x8B\x17", 0xCC, 7, &p)))
                    pData->ExRemoveTable -= 0x34;
                break;
            }
            else if (verInfo.dwBuildNumber == 18362 || verInfo.dwBuildNumber == 18363)
            {
                pData->ver = verInfo.dwBuildNumber == 18362 ? WINVER_10_19H1 : WINVER_10_19H2;
                pData->KExecOpt = 0x1C3;
                pData->Protection = 0x6FA;
                pData->EProcessFlags2 = 0x850;    // MitigationFlags offset
                pData->ObjTable = 0x418;
                pData->VadRoot = 0x658;
                pData->NtCreateThdExIndex = 0xBD;
                pData->NtTermThdIndex = 0x53;
                pData->PrevMode = 0x232;
                pData->ExitStatus = 0x710;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x83\xC7\x18\x48\x8B\x17", 0xCC, 7, &p)))
                    pData->ExRemoveTable -= 0x34;
                break;
            }
            else if (verInfo.dwBuildNumber <= 19041 && verInfo.dwBuildNumber < 19044)
            {
                pData->ver = WINVER_10_20H1;
                // KP
                pData->KExecOpt = 0x283;
                // EP
                pData->Protection = 0x87A;
                pData->EProcessFlags2 = 0x9D4;    // MitigationFlags offset
                pData->ObjTable = 0x570;
                pData->VadRoot = 0x7D8;
                // KT
                pData->PrevMode = 0x232;
                // ET
                pData->ExitStatus = 0x548;
                // SSDT
                pData->NtCreateThdExIndex = 0xC1;
                pData->NtTermThdIndex = 0x53;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x83\xC7\x18\x48\x8B\x17", 0xCC, 7, &p)))
                    pData->ExRemoveTable -= 0x34;
                break;
            }
            else if (verInfo.dwBuildNumber >= 19044)
            {
                pData->ver = WINVER_10_21H2;
                // KP
                pData->KExecOpt = 0x283;
                // EP
                pData->Protection = 0x87A;
                pData->EProcessFlags2 = 0x9D4;    // MitigationFlags offset
                pData->ObjTable = 0x570;
                pData->VadRoot = 0x7D8;
                // KT
                pData->PrevMode = 0x232;
                // ET
                pData->ExitStatus = 0x548;
                // SSDT
                pData->NtCreateThdExIndex = 0xC2;
                pData->NtTermThdIndex = 0x53;
                pData->MiAllocPage = 0;
                if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x83\xC7\x18\x48\x8B\x17", 0xCC, 7, &p)))
                    pData->ExRemoveTable -= 0x34;
                break;
                }
            else
            {
                return STATUS_NOT_SUPPORTED;
            }
        default:
            break;
        }

        if (pData->ExRemoveTable != 0)
            pData->correctBuild = TRUE;

        DPRINT(
            "BlackBone: Dynamic search status: SSDT - %s, ExRemoveTable - %s\n",
            GetSSDTBase() != NULL ? "SUCCESS" : "FAIL",
            pData->ExRemoveTable != 0 ? "SUCCESS" : "FAIL"
        );

        if (pData->ver >= WINVER_10_RS1)
        {
            DPRINT(
                "BlackBone: %s: g_KdBlock->KernBase: %p, GetKernelBase() = 0x%p \n",
                __FUNCTION__, g_KdBlock.KernBase, GetKernelBase(NULL)
            );

            ULONGLONG mask = (1ll << (PHYSICAL_ADDRESS_BITS - 1)) - 1;
            dynData.DYN_PTE_BASE = (ULONG_PTR)g_KdBlock.PteBase;
            dynData.DYN_PDE_BASE = (ULONG_PTR)((g_KdBlock.PteBase & ~mask) | ((g_KdBlock.PteBase >> 9) & mask));
        }

        DPRINT("BlackBone: PDE_BASE: %p, PTE_BASE: %p\n", pData->DYN_PDE_BASE, pData->DYN_PTE_BASE);
        if (pData->DYN_PDE_BASE < MI_SYSTEM_RANGE_START || pData->DYN_PTE_BASE < MI_SYSTEM_RANGE_START)
        {
            DPRINT("BlackBone: Invalid PDE/PTE base, aborting\n");
            return STATUS_UNSUCCESSFUL;
        }

        return (pData->VadRoot != 0 ? status : STATUS_INVALID_KERNEL_INFO_VERSION);
    }

    return status;
}