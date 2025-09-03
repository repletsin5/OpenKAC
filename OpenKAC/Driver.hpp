#pragma once
#include "AntiCheat.hpp"
EXTERN_C_START
#include <crtdefs.h>
#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EXTERN_C_END

#define TD_CALLBACK_REGISTRATION_TAG  '0bCO' // TD_CALLBACK_REGISTRATION structure.
#define TD_CALL_CONTEXT_TAG           '1bCO' // TD_CALL_CONTEXT structure.

typedef struct _TD_CALLBACK_PARAMETERS {
    ACCESS_MASK AccessBitsToClear;
    ACCESS_MASK AccessBitsToSet;
}
TD_CALLBACK_PARAMETERS, * PTD_CALLBACK_PARAMETERS;

//
// TD_CALLBACK_REGISTRATION
//

typedef struct _TD_CALLBACK_REGISTRATION {

    //
    // Handle returned by ObRegisterCallbacks.
    //

    PVOID RegistrationHandle;

    //
    // If not NULL, filter only requests to open/duplicate handles to this
    // process (or one of its threads).
    //

    PVOID TargetProcess;
    HANDLE TargetProcessId;


    //
    // Currently each TD_CALLBACK_REGISTRATION has at most one process and one
    // thread callback. That is, we can't register more than one callback for
    // the same object type with a single ObRegisterCallbacks call.
    //

    TD_CALLBACK_PARAMETERS ProcessParams;
    TD_CALLBACK_PARAMETERS ThreadParams;

    ULONG RegistrationId;        // Index in the global TdCallbacks array.

}
TD_CALLBACK_REGISTRATION, * PTD_CALLBACK_REGISTRATION;
//#include "trace.h"