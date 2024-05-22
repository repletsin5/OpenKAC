#include <argh.h>
#include "ioctls.h"
#include <iostream>
#include <filesystem>

bool SendProcID(HANDLE driver,HANDLE proc) {
	ioctls::Rqdata data = {};
	data.sendbuf = new INT64();
	data.reseivebuf = new INT64();
	data.size = sizeof(INT64);
	data.ret = 0;
	(*(INT64*)data.sendbuf) = (LONG_PTR)proc;
	std::cout << "created data buffer " << std::hex << proc << std::dec << std::endl;

	auto ret = DeviceIoControl(driver, ioctls::setProcess, &data, sizeof(data), &data, sizeof(data), 0, 0);
	std::cout << std::hex << (*(INT64*)data.reseivebuf) << std::endl;
	return ret;
}
#define DRIVER_FILE_LOC "C:\Program Files\OpenKAC\"
int main(int argc, char** argv) {

	argh::parser cmdl(argc,argv);

	if (cmdl[{"-p", "--process"}]) {

	}
	
	//std::filsy
		

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