#pragma once
#define LONESHA256_STATIC
#ifdef __cplusplus
#include <cstdarg>
extern "C" {
#else
#include <stdarg.h>
#endif // __cplusplus


#include <ntifs.h>
#include <ntdef.h>
#include <wdf.h>
#include <wdm.h>
#include <ntimage.h>
#include "std_includes.h"
#include "runtime/lonesha256.h"
#ifdef __cplusplus
}
#endif
typedef unsigned long DWORD;
typedef unsigned char BYTE, * PBYTE;

#if DBG
#ifndef STD_INC_H
#define	kdprint(...) _kdprint(__VA_ARGS__)


inline ULONG _kdprint(PCCH s, ...) {
	va_list(args);
	va_start(args, s); 
	auto ret = vDbgPrintExWithPrefix("[OpenKAC] ", 0, 0, s, args);
	va_end(args);
	return ret;
}
#endif
#else
#define	kdprint(...)
#endif