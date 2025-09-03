#pragma once
#include <Defs.h>
#include <guiddef.h>
#include <wdmguid.h>
#include <Solar/Solar_Status.hpp>

namespace OpenKAC::Solar::PCIE {
#pragma pack(push, 1)

	typedef struct _PCI_CONFIG_SPACE {
		UINT16 venderID;
		UINT16 deviceID;
		UINT16 Command;
		UINT16 Status;
		//TODO: fill
	}PCI_CONFIG_SPACE, * PPCI_CONFIG_SPACE;
#pragma pack(pop)

	class PCIE_Device {
	public:
		static OpenKAC::Solar::Status InitFilter(PDRIVER_OBJECT DriverObject,PUNICODE_STRING RegistryPat);
		static OpenKAC::Solar::Status PCIE_Enumorate();
	};
}