/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_OpenKAC,
    0x917e1021,0xaaa7,0x43f9,0x92,0xd4,0x55,0xa6,0x43,0x32,0x4c,0xb9);
// {917e1021-aaa7-43f9-92d4-55a643324cb9}
