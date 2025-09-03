#include "AntiCheat.hpp"
#include <intrin.h>
#include <handleapi.h>
#include <aux_klib.h>
#include "Driver.hpp"
#include "Defs.h"
#include "skCrypter.h"

//#include "driver.tmh"
#define RT_TAG 'okac'

#pragma code_seg(push)
#pragma code_seg()
OpenKAC::AntiCheat::AntiCheat()
{

	//PAGED_CODE();
}
OpenKAC::AntiCheat::~AntiCheat()
{
	if (acThreadHandle != 0 && acThreadHandle != INVALID_HANDLE_VALUE && acThread != 0) {
		ZwClose(acThreadHandle);

		KeWaitForSingleObject(acThread,
			Executive,
			KernelMode,
			FALSE,
			NULL);
		ObDereferenceObject(acThread);

	}
}	
PEPROCESS OpenKAC::AntiCheat::GetProcess()
{
//	PAGED_CODE();
	return this->proc;
}
void OpenKAC::AntiCheat::SetProcess(PEPROCESS passedProc)
{
	//	PAGED_CODE();
	this->proc = passedProc;
}

#pragma code_seg(pop)
#pragma code_seg(push)
#pragma code_seg("PAGE")

KSTART_ROUTINE KstartRoutine;
void KstartRoutine(PVOID StartContext) {
	PAGED_CODE();

	KdPrintEx((0, 0, "[OpenKAC] AC thread started\n"));

		
	PsTerminateSystemThread(0);
}
void OpenKAC::AntiCheat::CreateThread()
{
	PAGED_CODE();
	OBJECT_ATTRIBUTES p = {};
	InitializeObjectAttributes(&p, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	NTSTATUS status = PsCreateSystemThread(&acThreadHandle, STANDARD_RIGHTS_ALL,&p, 0, 0, &KstartRoutine, 0);
	UNREFERENCED_PARAMETER(status);
	
	//if(NT_SUCCESS(status))
	//	DbgPrintEx(0, 0, "[OpenKAC] AC yay\n");
	ObReferenceObjectByHandle(acThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		(void**)&acThread,
		NULL);

	//ZwClose(acThreadHandle);
	//if (acThread) {
	//	KeWaitForSingleObject(acThread,
	//		Executive,
	//		KernelMode,
	//		FALSE,
	//		NULL);
	//	ObDereferenceObject(acThread);
	//}
}
void OpenKAC::AntiCheat::StripHandles()
{
	PAGED_CODE();

}
OB_PREOP_CALLBACK_STATUS OpenKAC::AntiCheat::ObPreOperation(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION PreInfo)
{
	PAGED_CODE();

	PTD_CALLBACK_REGISTRATION CallbackRegistration;

	ACCESS_MASK AccessBitsToClear = 0;
	ACCESS_MASK AccessBitsToSet = 0;
	ACCESS_MASK InitialDesiredAccess = 0;
	ACCESS_MASK OriginalDesiredAccess = 0;


	PACCESS_MASK DesiredAccess = NULL;

	LPCWSTR ObjectTypeName = NULL;
	LPCWSTR OperationName = NULL;
	CallbackRegistration = (PTD_CALLBACK_REGISTRATION)RegistrationContext;

	//if (ac->GetProcess() != 0 && PsGetProcessExitStatus(ac->GetProcess()) == STATUS_PENDING) {
	//	if (PreInfo->ObjectType == *PsProcessType) {
	//		HANDLE ProcessIdOfTargetThread = PsGetThreadProcessId((PETHREAD)PreInfo->Object);


	//		// Threads which try to open/duplicate from the protected process itself
	//		if (ProcessIdOfTargetThread == PsGetProcessId(ac->GetProcess())) {
	//			//KdPrintEx((0, 0, "[OpenKAC] ObPreOperation: ignore proccess open/duplicate from the protected process itself\n"));
	//			ObjectTypeName = L"PsProcessType";
	//			AccessBitsToClear = 0;
	//			AccessBitsToSet = 0;
	//		}
	//		return OB_PREOP_SUCCESS;

	//	}
	//	else if (PsGetProcessExitStatus(ac->GetProcess()) == STATUS_PENDING && PreInfo->ObjectType == *PsProcessType) {
	//		HANDLE ProcessIdOfTargetThread = PsGetThreadProcessId((PETHREAD)PreInfo->Object);

	//		//
	//		// Ignore requests for threads belonging to processes other than our
	//		// target process.
	//		//

	//		// if (CallbackRegistration->TargetProcess   != NULL &&
	//		//     CallbackRegistration->TargetProcessId != ProcessIdOfTargetThread)


	//		//
	//		// Also ignore requests for threads belonging to the current processes.
	//		//

	//		if (ProcessIdOfTargetThread == PsGetProcessId(ac->GetProcess())) {
	//			//DbgPrintEx(
	//				//0, 0,
	//				//"ObCallbackTest: CBTdPreOperationCallback: ignore thread open/duplicate from the protected process itself\n");
	//			return OB_PREOP_SUCCESS;
	//		}

	//		ObjectTypeName = L"PsThreadType";
	//		AccessBitsToClear = 0x1;
	//		AccessBitsToSet = 0;
	//	}
	//}
	switch (PreInfo->Operation) {
	case OB_OPERATION_HANDLE_CREATE:
		DesiredAccess = &PreInfo->Parameters->CreateHandleInformation.DesiredAccess;
		OriginalDesiredAccess = PreInfo->Parameters->CreateHandleInformation.OriginalDesiredAccess;

		OperationName = L"OB_OPERATION_HANDLE_CREATE";
		break;

	case OB_OPERATION_HANDLE_DUPLICATE:
		DesiredAccess = &PreInfo->Parameters->DuplicateHandleInformation.DesiredAccess;
		OriginalDesiredAccess = PreInfo->Parameters->DuplicateHandleInformation.OriginalDesiredAccess;

		OperationName = L"OB_OPERATION_HANDLE_DUPLICATE";
		break;
	}
	if (DesiredAccess) {
		InitialDesiredAccess = *DesiredAccess;

		if (PreInfo->KernelHandle != 1) {
			*DesiredAccess &= ~AccessBitsToClear;
			*DesiredAccess |= AccessBitsToSet;
		}

		kdprint(0, 0,
			"PreOperationCallback ,\n"
			"    Client Id:    %p:%p\n"
			"    Object:       %p\n"
			"    Type:         %ls\n"
			"    Operation:    %ls (KernelHandle=%d)\n"
			"    OriginalDesiredAccess: 0x%x\n"
			"    DesiredAccess (in):    0x%x\n"
			"    DesiredAccess (out):   0x%x\n"
			"    =========================================================\n",
			PsGetCurrentProcessId(),
			PsGetCurrentThreadId(),
			PreInfo->Object,
			ObjectTypeName,
			OperationName,
			PreInfo->KernelHandle,
			OriginalDesiredAccess,
			InitialDesiredAccess,
			*DesiredAccess
		);
	}
	return OB_PREOP_SUCCESS;

}
void OpenKAC::AntiCheat::ObPostOperation(PVOID RegistrationContext, POB_POST_OPERATION_INFORMATION PostInfo)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(RegistrationContext);
	UNREFERENCED_PARAMETER(PostInfo);
}
#pragma code_seg(pop)
