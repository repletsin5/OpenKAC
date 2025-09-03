#pragma once
//#define HIDE_IMPORTS
#ifdef HIDE_IMPORTS
#include "std_includes.h"
#include "ImportHider.hpp"
#endif // HIDE_IMPORTS




namespace klib {

    __forceinline _IRQL_requires_max_(APC_LEVEL)
    _Ret_range_(<= , 0)     
    NTSTATUS
    IoCreateDevice(
            _In_  PDRIVER_OBJECT DriverObject,
            _In_  ULONG DeviceExtensionSize,
            _In_opt_ PUNICODE_STRING DeviceName,
            _In_  DEVICE_TYPE DeviceType,
            _In_  ULONG DeviceCharacteristics,
            _In_  BOOLEAN Exclusive,
            _Outptr_result_nullonfailure_
            _At_(*DeviceObject,
                __drv_allocatesMem(Mem)
                _When_((((_In_function_class_(DRIVER_INITIALIZE))
                    || (_In_function_class_(DRIVER_DISPATCH)))),
                    __drv_aliasesMem))
            PDEVICE_OBJECT* DeviceObject
        ) {
#ifdef HIDE_IMPORTS
        //TODO
        return ::IoCreateDevice(DriverObject, DeviceExtensionSize, DeviceName, DeviceType, DeviceCharacteristics, Exclusive, DeviceObject);
        #else
        return ::IoCreateDevice(DriverObject, DeviceExtensionSize, DeviceName, DeviceType, DeviceCharacteristics, Exclusive, DeviceObject);
#endif // HIDE_IMPORTS

    }
}