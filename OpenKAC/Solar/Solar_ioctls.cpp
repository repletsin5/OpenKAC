#include "Solar_ioctls.hpp"
#include <BlackBone/Imports.h>
#include "ioctls.hpp"


#pragma code_seg(push)
#pragma code_seg("PAGE")

NTSTATUS OpenKAC::Solar::ioctl_handler(PIRP Irp, ioctls::Rqdata* data, HANDLE caller)
{
	PAGED_CODE();

	return STATUS_SUCCESS;
}

#pragma code_seg(pop)
