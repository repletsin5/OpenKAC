#include "ioctls.hpp"
#include <runtime/Helpers.h>

extern "C" {
#include <handleapi.h>
#include <BlackBone/BlackBoneDef.h>
#include <BlackBone/Routines.h>
#include <BlackBone/Loader.h>
}
#include "AntiCheat.hpp"
#include "Solar/Solar_ioctls.hpp"
#include "Driver.hpp"
#include <climits>
#include <ntstrsafe.h>
#include <ntifs.h>

#pragma code_seg(push)
#pragma code_seg()
void CreateDriverCppObject(PEPROCESS& proc) {
	OpenKAC::AntiCheat::ac->SetProcess(proc);
	KdPrintEx((0, 0, "[OpenKAC] AC addr is: 0x%llx\n", (UINT64)OpenKAC::AntiCheat::ac));
	KdPrintEx((0, 0, "[OpenKAC] AC proc is: 0x%llx\n", (UINT64)OpenKAC::AntiCheat::ac->GetProcess()));
	OpenKAC::AntiCheat::ac->CreateThread();
	OpenKAC::AntiCheat::ac->StripHandles();
}
#pragma code_seg(pop)

#pragma code_seg(push)
#pragma code_seg("PAGE")
static __forceinline NTSTATUS Sendback(UINT64 sendValue, ioctls::Rqdata** pdata, HANDLE caller, PIRP& Irp) {
	PAGED_CODE();

	ioctls::Rqdata* data = *pdata;
	size_t bytes;
	PEPROCESS callerprocess;
	PsLookupProcessByProcessId(caller, &callerprocess);
	data->ret = sizeof(UINT64);	
	//VOID* userBuffer = 0;
	//NtAllocateVirtualMemory(caller, &userBuffer, 0, &data->ret, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	//ProbeForWrite(src, size, allignment); //TODO: do check before writing to mem

	//auto status = MmCopyVirtualMemory((PEPROCESS)PsGetCurrentProcess(), (void*)&sendValue, callerprocess, (void*)userBuffer, data->ret, KernelMode, &bytes);
	auto status = MmCopyVirtualMemory((PEPROCESS)PsGetCurrentProcess(), (void*)&sendValue, callerprocess, (void*)data->receivebuf, data->ret, KernelMode, &bytes);

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(0, 0, "MmCopyVirtualMemory Failed %llx", status);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_UNSUCCESSFUL;
	}
	//data->receivebuf = userBuffer;
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
	//KdPrintEx((0, 0, "%s: Device Control\n", __FUNCTION__));

	PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
	ioctls::Rqdata* data =(ioctls::Rqdata*)Irp->AssociatedIrp.SystemBuffer;

	Irp->IoStatus.Status = status;

	if (data == nullptr || stackLocation == nullptr) {
		KdPrintEx((0, 0, "[OpenKAC] data or stackLocation is null %lu\n", status));
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
	//Weird api difference, it returns a HANDLE as a ULONG.
	//Will check for ULONG_MAX value but extremely unlikely to go past ULONG_MAX.
	HANDLE caller = (HANDLE)(LONG_PTR)IoGetRequestorProcessId(Irp);
	if (caller == ((HANDLE)(LONG_PTR)ULONG_MAX) || caller == INVALID_HANDLE_VALUE || caller == 0) {
		KdPrintEx((0, 0, "[OpenKAC] Your HANDLE count is very high. Returing due to API only supporting ULONG: %lu!", status));
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
	PINJECT_DLL dlldata;
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
			KdPrintEx((0, 0, "[OpenKAC] Setting process to: 0x%I64x\n", (*((UINT64*)data->sendbuf))));
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
	case ioctls::umACmodule:
		//TODO check who sent and validate dll image
		if (proc == 0) {
			KdPrintEx((0, 0, "[OpenKAC] No process\n"));
			//TODO new error value
			return SENDBACK(0);
		}
		if (data->size < 8*1024) { //just just make sure it will not send obviously corrupt file.
			return SENDBACK(KAC_INCORRECT_DATA_SIZE);
		}
		dlldata = new INJECT_DLL();
		dlldata->pid = PsGetProcessId(proc);
		dlldata->asImage = false;
		dlldata->type = IT_MMap;
		dlldata->initRVA = true;
		dlldata->unlink = true;
		dlldata->wait = true;
		dlldata->imageBase = (UINT64)data->sendbuf;
		dlldata->imageSize = data->size;
		//RtlStringCchCopyW(data->FullDllPath, 512, L"awdawdwad");
		memset(dlldata->FullDllPath, 0, 512);
		RtlStringCchCopyW(dlldata->initArg, 512, L"");
		KdPrintEx((0, 0, "[OpenKAC] Injecting DLL\n"));
		BBInjectDll(dlldata);
		return SENDBACK(0);
		break;	
	case ioctls::pdbFiles:
		//TODO verify who sent and validate pdb
		if (data->size != sizeof(PDB_Info)) {
			return SENDBACK(KAC_INCORRECT_DATA_SIZE);
		}
		if (data->sendbuf == 0) {
			return SENDBACK(KAC_INVALID_SEND_INFO);
		}
		MEMORY_BASIC_INFORMATION  info;
		ZwQueryVirtualMemory(caller, data->sendbuf, MemoryBasicInformation, &info, sizeof(MEMORY_BASIC_INFORMATION), 0);
		if (info.State == MEM_FREE && info.RegionSize < data->size ) {
			return SENDBACK(KAC_INVALID_SEND_INFO);
		}
		//PPDB_Info pdbInfo = (PPDB_Info)data->sendbuf;
		//VOID* pdbInfoCopy = 0;
		//SIZE_T sizeAllocated = data->size;
		//ZwAllocateVirtualMemory(PsGetCurrentProcess(), &pdbInfoCopy, 0, &sizeAllocated, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		//pdbInfo->dataPtr;
		break;
		
	default:
		break;
	}

	return OpenKAC::Solar::ioctl_handler(Irp, data, caller);
}
#pragma code_seg(pop)