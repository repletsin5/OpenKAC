#pragma once
#include <ntifs.h>
#define KAC_INCORRECT_DATA_SIZE 0xff0f1
#define KAC_INVALID_DATA_ADDRESS 0xff0f2
#define KAC_PROCCESS_ALREADY_SET 0xff0f3
namespace ioctls {
	inline constexpr ULONG setProcess =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f00, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	inline constexpr ULONG heartbeat =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f01, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	inline constexpr ULONG detectstatus =
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9f8f02, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

	struct Rqdata{
		PVOID sendbuf;
		PVOID receivebuf;
		SIZE_T size;
		SIZE_T ret;
	};
}