#include "ioctls.hpp"
extern "C" {
#include <handleapi.h>
}
#include "AntiCheat.hpp"
#include "Driver.hpp"
#include <climits>
extern "C" NTSTATUS NTAPI MmCopyVirtualMemory(PEPROCESS srcProc, PVOID srcAddr, PEPROCESS targProc,
    PVOID targAddr, SIZE_T bufSize, KPROCESSOR_MODE prevMode, PSIZE_T retSize);
#pragma code_seg(push)
#pragma code_seg()
void CreateDriverCppObject(PEPROCESS& proc) {

    OpenKAC::AntiCheat::ac->SetProcess(proc);
    KdPrintEx((0, 0, "[OpenKAC] AC addr is: 0x%x\n", OpenKAC::AntiCheat::ac));
    KdPrintEx((0, 0, "[OpenKAC] AC proc is: 0x%x\n", OpenKAC::AntiCheat::ac->GetProcess()));
    OpenKAC::AntiCheat::ac->CreateThread();
    OpenKAC::AntiCheat::ac->StripHandles();

}
#pragma code_seg(pop)

#pragma code_seg(push)
#pragma code_seg("PAGE")
static NTSTATUS Sendback(UINT64 sendValue, ioctls::Rqdata** pdata, HANDLE caller, PIRP& Irp) {
    PAGED_CODE();

    ioctls::Rqdata* data = *pdata;
    size_t bytes;
    PEPROCESS callerprocess;
    PsLookupProcessByProcessId(caller, &callerprocess);
    data->ret = sizeof(UINT64);
    auto status = MmCopyVirtualMemory((PEPROCESS)PsGetCurrentProcess(), (void*)&sendValue, callerprocess, (void*)data->receivebuf, data->ret, KernelMode, &bytes);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0,0, "MmCopyVirtualMemory Failed %lu", status);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
#pragma code_seg(pop)

#define SENDBACK(sendValue) Sendback(sendValue,&data,caller, Irp);
#pragma code_seg(push)
#pragma code_seg("PAGE")
NTSTATUS
ioctls::DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
) {
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();
    KdPrintEx((0,0,"%s: Device Control\n",__FUNCTION__));


    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
    ioctls::Rqdata* data = reinterpret_cast<ioctls::Rqdata*>(Irp->AssociatedIrp.SystemBuffer);


    Irp->IoStatus.Status = status;

    if (data == nullptr || stackLocation == nullptr) {
        //DbgPrintEx(0, 0, "[OpenKAC] data or stack location is null\n");
        KdPrintEx((0,0, "[OpenKAC] data or stackLocation is null %lu\n", status));
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    }
    //Weird api difference, it returns a HANDLE as a ULONG.
    //Will check for ULONG_MAX value but extremely unlikely to go past ULONG_MAX.
    HANDLE caller = (HANDLE)(LONG_PTR)IoGetRequestorProcessId(Irp);
    if (caller == ((HANDLE)(LONG_PTR)ULONG_MAX) || caller == INVALID_HANDLE_VALUE || caller == 0) {
        KdPrintEx((0,0, "[OpenKAC] Your HANDLE count is very high. Returing due to API only supporting ULONG: %lu!", status));
        KdBreakPoint();
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED, 0, 0, 0, 0);
         return STATUS_UNSUCCESSFUL;
    }
    
    const ULONG ctrlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    //check for invalid address
    if ((UINT64)data->receivebuf > (UINT64)0x7FFFFFFFFFFF || (UINT64)data->receivebuf == (UINT64)0) {
        KdPrintEx((0, 0, "[OpenKAC] Invalid Address of send buffer is: 0x%llx\n", (UINT64)data->receivebuf));
        KdBreakPoint();
        return SENDBACK(KAC_INVALID_DATA_ADDRESS);
    }
    switch (ctrlCode)
    {
    case ioctls::setProcess:
        if (proc != 0) {
            KdPrintEx((0, 0, "[OpenKAC] Process already set\n"));
            return SENDBACK(KAC_PROCCESS_ALREADY_SET);
        }
        if (data->size != sizeof(INT64)) {
            return SENDBACK(KAC_INCORRECT_DATA_SIZE);
        }
        else {
            KdPrintEx((0, 0, "[OpenKAC] Setting process to: 0x%llx\n", (*((UINT64*)data->sendbuf))));
            status = PsLookupProcessByProcessId((HANDLE)(*((UINT64*)data->sendbuf)), &proc);
            if (!NT_SUCCESS(status)) {
                KdPrintEx((0, 0, "[OpenKAC] PsLookupProcessByProcessId failed: 0x%x", status));
                return SENDBACK(0);
            }
            //TODO protect process e.g. hide thread names
            CreateDriverCppObject(proc);
            return SENDBACK(1);
        }

        break;
    case ioctls::heartbeat:
        if (proc == 0) {
            KdPrintEx((0, 0, "[OpenKAC] No process for heartbeat: 0x%llx\n", *(UINT64*)data->sendbuf));
            //TODO new error value
            return SENDBACK(0);
        }
        if (data->size != sizeof(INT64)) {
            return SENDBACK(KAC_INCORRECT_DATA_SIZE);
        }
        if (caller != PsGetProcessId(proc)) {
            return SENDBACK(0);
            //TODO:
            KdPrintEx((0, 0, "[OpenKAC] Other process called heartbeat killing both\n"));
            auto hdl = PsGetProcessId(proc);
            ZwTerminateProcess(hdl, 0);
            ZwTerminateProcess(caller, 0);
            KdPrintEx((0, 0, "[OpenKAC] Unloading\n"));
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
#pragma code_seg(pop)