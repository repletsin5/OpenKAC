#include "Solar_PCIE.hpp"
#include <Wdf.h>
#include <KString.hpp>
#include <System.hpp>
#include <Array.hpp>

#pragma code_seg(push)
#pragma code_seg("PAGE")
OpenKAC::Solar::Status OpenKAC::Solar::PCIE::PCIE_Device::InitFilter(PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath)
{


	PAGED_CODE();
}


OpenKAC::Solar::Status OpenKAC::Solar::PCIE::PCIE_Device::PCIE_Enumorate()
{
	PAGED_CODE();
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK ioStatusBlock;
	PIO_STACK_LOCATION irpStack;
	PDEVICE_OBJECT targetObject;

	OpenKAC::Solar::Status  status = OpenKAC::Solar::Status::STATUS_SUCCESSFUL;
	klib::ukString PCIdriverName(L"\\Driver\\pci");
	PDRIVER_OBJECT PCIdriverObject = 0;
	klib::Modules::GetDriverObjecFromDeviceName(&PCIdriverObject, PCIdriverName);

	if (!PCIdriverObject) {
		//TODO:
		return STATUS_NULLPTR_OBJECT;
	}
	DWORD count = 0;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ntStatus = IoEnumerateDeviceObjectList(PCIdriverObject, NULL, 0, &count);

	klib::std::array<PDEVICE_OBJECT> list(count);

	ntStatus = IoEnumerateDeviceObjectList(
		PCIdriverObject,
		list.data(),
		count * sizeof(PDEVICE_OBJECT),
		&count);

	list.setCount(count);



	for (auto& obj : list) {
		if (obj != 0) {
			KdPrintEx((0, 0, "[OpenKAC] found pci device obj at 0x%I64x\n", (UINT64)obj));

			KeInitializeEvent(&event, NotificationEvent, FALSE);
			targetObject = IoGetAttachedDeviceReference(obj);
			irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
				targetObject,
				NULL,
				0,
				NULL,
				&event,
				&ioStatusBlock);
			if (irp == nullptr) {

				KdPrintEx((0, 0, "[OpenKAC] pci device irp is null\n"));
				ObDereferenceObject(obj);
				return STATUS_FAIL;
			}
			char* buf = new char[4096];
			irpStack = IoGetNextIrpStackLocation(irp);
			irpStack->MinorFunction = IRP_MN_READ_CONFIG;
			irpStack->Parameters.ReadWriteConfig.WhichSpace = PCI_WHICHSPACE_CONFIG;
			irpStack->Parameters.ReadWriteConfig.Buffer = buf;
			irpStack->Parameters.ReadWriteConfig.Offset = 0;
			irpStack->Parameters.ReadWriteConfig.Length = 4096;
			irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

			ntStatus = IoCallDriver(targetObject, irp);
			if (ntStatus == STATUS_PENDING) {
				KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
				ntStatus = ioStatusBlock.Status;
			}
			PPCI_CONFIG_SPACE cfg = (PPCI_CONFIG_SPACE)buf;
			KdPrintEx((0, 0, "[OpenKAC] vender id: 0x%04x, device id: 0x%04x\n", cfg->venderID, cfg->deviceID));
			delete buf;
			ObDereferenceObject(obj);
		}
	}
	return status;
}
#pragma code_seg(pop)
