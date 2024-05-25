#include <argh.h>
#include "ioctls.h"
#include <iostream>
#include <filesystem>
#include <set>
#include <psapi.h>
#include <tchar.h>
#include "def.h"

bool SendProcID(HANDLE driver,HANDLE proc) {
	ioctls::Rqdata data = {};
	data.sendbuf = new INT64();
	data.receivebuf = new INT64();
	data.size = sizeof(INT64);
	data.ret = 0;
	(*(INT64*)data.sendbuf) = (LONG_PTR)proc;
	std::cout << "created data buffer " << std::hex << proc << std::dec << std::endl;

	auto ret = DeviceIoControl(driver, ioctls::setProcess, &data, sizeof(data), &data, sizeof(data), 0, 0);
	std::cout << std::hex << (*(INT64*)data.receivebuf) << std::endl;
	return ret;
}
#define DRIVER_FILE_LOC "C:\\Program Files\\OpenKAC\\"


int BlackListedDriver() {

	DWORD bytesNeeded = 0;
	DWORD numServices = 0;
	DWORD resumeHandle = 0;

	SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (!scm) {
		std::cout << "Can not get scm manager" << std::endl;
		return GetLastError();
	}

	if(!EnumServicesStatusA(scm, SERVICE_DRIVER, SERVICE_ACTIVE, NULL, 0, &bytesNeeded, &numServices, 0))
	{
		auto ret = GetLastError();
		if (ret != ERROR_INSUFFICIENT_BUFFER && ret != ERROR_MORE_DATA) {
			std::cout << "Can not get enumiration size for driver services: " <<  ret  << std::endl;
			return ret;
		}
	}
	auto buf = new byte[bytesNeeded];
	LPENUM_SERVICE_STATUSA pEnum = reinterpret_cast<LPENUM_SERVICE_STATUSA>(buf);

	if (!EnumServicesStatusA(scm, SERVICE_DRIVER, SERVICE_ACTIVE, pEnum, bytesNeeded, &bytesNeeded, &numServices, 0)) {
		auto ret = GetLastError();
		std::cout << "Can not enumirate driver services: 0x" << std::hex << ret << std::dec << std::endl;
		return ret;
	}
	// quick check before loading driver and you could let the user know of said driver running.
	std::set<std::string> drvs = { "KProcessHacker3" };
	for (DWORD i = 0; i < numServices; ++i)
	{
#ifdef LIST_RUNNING_DRIVERS
		std::cout << pEnum[i].lpServiceName << " (" << pEnum[i].lpDisplayName << ")" << std::endl;
		std::cout << "status: 0x" << std::hex << pEnum[i].ServiceStatus.dwCurrentState << std::dec << std::endl;
#endif
		if (std::find(drvs.begin(), drvs.end(), std::string(pEnum[i].lpServiceName)) != drvs.end()) 
		{
			std::cout << "found: " << pEnum[i].lpServiceName << std::endl;
			CloseServiceHandle(scm);
			return KAC_ENUM_DRV_BLACKLISTED;
		}
	}
	CloseServiceHandle(scm);
	return KAC_ENUM_DRV_PASS;

}


int main(int argc, char** argv) {

	argh::parser cmdl(argc,argv);

	if (cmdl[{"-p", "--process"}]) {

	}

	//std::filsy
	auto bldRes = BlackListedDriver();
	switch (bldRes)
	{
	case KAC_ENUM_DRV_PASS:
		break;
	case KAC_ENUM_DRV_BLACKLISTED:
		// TODO send info to user
		exit(0);
		break;
	default:
		std::cout << "Checking for backlisted driver failed: " << std::hex << bldRes << std::dec << std::endl;
		break;
	}
	//TODO call execute OpenKAC_Service.

	auto driver = CreateFile(L"\\\\.\\OpenKAC",GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (driver == INVALID_HANDLE_VALUE) {
		std::cin.get();
		exit(0);
	}
	HANDLE proc = (HANDLE)GetCurrentProcessId();
	std::cout << "sending proc: " << std::hex << proc << std::dec << std::endl;
	SendProcID(driver, proc);
	CloseHandle(driver);
	return 0;
}
