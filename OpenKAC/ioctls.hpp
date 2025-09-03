#pragma once

extern "C" {
#include <ntifs.h>
}
//#include "Trace.h"
//#include "driver.tmh"

#define KAC_INCORRECT_DATA_SIZE 0xff0f1
#define KAC_INVALID_DATA_ADDRESS 0xff0f2
#define KAC_PROCCESS_ALREADY_SET 0xff0f3
#define KAC_INVALID_SEND_INFO 0xff0f4
namespace ioctls {
	inline constexpr ULONG setProcess =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f00, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	inline constexpr ULONG heartbeat =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f01, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	inline constexpr ULONG detectstatus =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f02, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	inline constexpr ULONG umACmodule =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f03, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	inline constexpr ULONG pdbFiles =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f04, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	struct Rqdata{
		PVOID sendbuf;
		PVOID receivebuf;
		SIZE_T size;
		SIZE_T ret;
	};

	typedef struct _PDB_Info{
		UINT16 nameLen;
		char* moduleName;
		UINT32 pdbLen;
		void* dataPtr;
	}PDB_Info, *PPDB_Info;
	static PEPROCESS proc = 0;
	_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
		DRIVER_DISPATCH DeviceControl;
}