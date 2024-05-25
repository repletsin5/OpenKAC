/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/
#define NT_DEVICE_NAME      L"\\Device\\OpenKAC"
#define DOS_DEVICE_NAME     L"\\DosDevices\\OpenKAC"

#include <ntifs.h>
#include <ntdef.h>

#include "driver.h"
#include "driver.tmh"
#include "ioctls.h"
#include <intsafe.h>
#include <handleapi.h>
extern "C" {
    DRIVER_INITIALIZE DriverEntry;

    _Dispatch_type_(IRP_MJ_CREATE)
        _Dispatch_type_(IRP_MJ_CLOSE)
        DRIVER_DISPATCH CreateClose;

    _Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
        DRIVER_DISPATCH DeviceControl;

    DRIVER_UNLOAD UnloadDriver;
#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text(PAGE, CreateClose)
#pragma alloc_text(PAGE, DeviceControl)
#pragma alloc_text(PAGE, UnloadDriver)
#endif
}
extern "C" NTSTATUS NTAPI MmCopyVirtualMemory(PEPROCESS srcProc, PVOID srcAddr, PEPROCESS targProc,
    PVOID targAddr, SIZE_T bufSize, KPROCESSOR_MODE prevMode, PSIZE_T retSize);





NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{

    NTSTATUS status;
    UNICODE_STRING  ntUnicodeString;
    UNICODE_STRING  ntWin32NameString;  
    PDEVICE_OBJECT  deviceObject = NULL;   

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&ntUnicodeString, NT_DEVICE_NAME);
    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        0,                              // We don't use a device extension
        &ntUnicodeString,               // Device name 
        FILE_DEVICE_UNKNOWN,            // Device type
        FILE_DEVICE_SECURE_OPEN,     // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject);                // Returned ptr to Device Object

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "IoCreateDevice failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    SetFlag(deviceObject->Flags, DO_BUFFERED_IO);

    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload = UnloadDriver;

    ClearFlag(deviceObject->Flags, DO_DEVICE_INITIALIZING);

    RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);
    //
    // Create a symbolic link between our device name  and the Win32 name
    //

    status = IoCreateSymbolicLink(
        &ntWin32NameString, &ntUnicodeString);

    if (!NT_SUCCESS(status))
    {
        //
        // Delete everything that this routine has allocated.
        //
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "IoCreateSymbolicLink failed %!STATUS!", status);

        IoDeleteDevice(deviceObject);
    }

    RtlFreeUnicodeString(&ntWin32NameString);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

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
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    PAGED_CODE();

    //
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

    RtlFreeUnicodeString(&uniWin32NameString);

}


NTSTATUS Sendback(int sendValue, ioctls::Rqdata** pdata,HANDLE caller, PIRP &Irp) {
    ioctls::Rqdata* data = *pdata;
    size_t bytes; 
    PEPROCESS callerprocess; 
    PsLookupProcessByProcessId(caller, &callerprocess); 
    data->ret = sizeof(int); 
    auto status = MmCopyVirtualMemory((PEPROCESS)PsGetCurrentProcess(), (void*)&sendValue, callerprocess, (void*)data->sendbuf, data->ret, KernelMode, &bytes); 
    if (!NT_SUCCESS(status)) {        
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "MmCopyVirtualMemory Failed %!STATUS!", status); 
        return STATUS_UNSUCCESSFUL; 
        IoCompleteRequest(Irp, IO_NO_INCREMENT); 
    }
    IoCompleteRequest(Irp, IO_NO_INCREMENT); 
    return STATUS_SUCCESS;
}

#define SENDBACK(sendValue) Sendback(sendValue,&data,caller, Irp);



NTSTATUS
DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
) {
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Device Control");


    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
    ioctls::Rqdata* data = reinterpret_cast<ioctls::Rqdata*>(Irp->AssociatedIrp.SystemBuffer);


    Irp->IoStatus.Status = status;

    if (data == nullptr || stackLocation == nullptr) {
        DbgPrintEx(0, 0, "[OpenKAC] data or stack location is null\n");
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "data or stackLocation is null %!STATUS!", status);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    }
    //Weird api difference, it returns a HANDLE as a ULONG.
    //Will check for ULONG_MAX value but extremely unlikely to go past ULONG_MAX.
    HANDLE caller = (HANDLE)(LONG_PTR)IoGetRequestorProcessId(Irp);
    if (caller == ((HANDLE)(LONG_PTR)ULONG_MAX) || caller == INVALID_HANDLE_VALUE || caller == 0) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Your HANDLE count is very high. Returing due to API only supporting ULONG %!STATUS!", status);
        return STATUS_UNSUCCESSFUL;
    }
                          
    const ULONG ctrlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    //check for invalid address
    if ((UINT64)data->receivebuf > (UINT64)0x7FFFFFFFFFFF || (UINT64)data->receivebuf == (UINT64)0) {
        DbgPrintEx(0, 0, "[OpenKAC] Invalid Address of send buffer is: 0x%x\n", data->receivebuf);
        SENDBACK(KAC_INVALID_DATA_ADDRESS);
    }
    switch (ctrlCode)
    {
    case ioctls::setProcess:
        if (proc != 0) {
            DbgPrintEx(0, 0, "[OpenKAC] Process Already set\n");
            SENDBACK(KAC_PROCCESS_ALREADY_SET);
        }
        if (data->size != sizeof(INT64)) {
            SENDBACK(KAC_INCORRECT_DATA_SIZE);
        }
        else {
            DbgPrintEx(0, 0, "[OpenKAC] Setting process to: 0x%x\n", data->receivebuf);
            PsLookupProcessByProcessId(data->receivebuf, &proc);
            //TODO protect process e.g. hide thread names
            SENDBACK(0);
        }
    
        break;
    case ioctls::heartbeat:
        if (proc == 0) {
            DbgPrintEx(0, 0, "[OpenKAC] No process for heartbeat: 0x%x\n", data->receivebuf);
            SENDBACK(0);
        }
        if (data->size != sizeof(INT64)) {
            SENDBACK(KAC_INCORRECT_DATA_SIZE);
        }
        if (caller != PsGetProcessId(proc)) {
            DbgPrintEx(0, 0, "[OpenKAC] Other process called heartbeat killing both\n");
            auto hdl = PsGetProcessId(proc);
            ZwTerminateProcess(hdl,0);
            ZwTerminateProcess(caller,0);
            DbgPrintEx(0, 0, "[OpenKAC] Unloading\n");
            //ZwUnloadDriver()
        }
        break;
    case ioctls::detectstatus:
        break;
    default:
        break;
    }
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}