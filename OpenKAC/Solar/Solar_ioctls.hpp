#pragma once
#include <Defs.h>


namespace ioctls {
	struct Rqdata;
}

namespace OpenKAC::Solar 
{

	NTSTATUS ioctl_handler(PIRP Irp, ioctls::Rqdata* data, HANDLE caller);
};