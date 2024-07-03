#pragma once

EXTERN_C_START
#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include "trace.h"
//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EXTERN_C_END
static PEPROCESS proc = 0;
