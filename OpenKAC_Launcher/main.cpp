#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <string>
#include <thread>
#include <Windows.h>
#include <psapi.h>
#include <tchar.h>

#include <argh.h>
#include "EasyPDB.hpp"

#include "def.hpp"
#include "ioctls.hpp"
#define DRIVER_FILE_LOC "C:\\Program Files\\OpenKAC\\"

bool SendProcID(HANDLE driver, HANDLE proc) {
	ioctls::Rqdata data = {};
	data.sendbuf = new UINT64();
	data.receivebuf = new UINT64();
	data.size = sizeof(UINT64);
	data.ret = 0;
	(*(UINT64*)data.sendbuf) = (LONG_PTR)proc;
	std::cout << "created data buffer " << std::hex << &data << std::dec << std::endl;

	auto ret = DeviceIoControl(driver, ioctls::setProcess, &data, sizeof(data), &data, sizeof(data), 0, 0);
	if (*(UINT64*)data.receivebuf == 1) {
	} {
	}
	return ret;
}
std::filesystem::path getModuleFromID(std::string id) {

	UNREFERENCED_PARAMETER(id);
	//TODO:
	return (std::filesystem::path(DRIVER_FILE_LOC) /= "OpenKAC_Module.dll");
}
bool SendACModule(HANDLE driver, std::string gameId) {

	std::filesystem::path file = getModuleFromID(gameId);
	if (std::filesystem::exists(file) && std::filesystem::is_regular_file(file)) {

		std::cout << "Reading file:" << file << std::endl;

		auto length = std::filesystem::file_size(file);
		char* buffer = (char*)VirtualAlloc(0, length, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		std::ifstream inputFile(file, std::ios_base::binary);
		inputFile.seekg(0, std::ios::beg);
		inputFile.read(buffer, length);
		inputFile.close();


		ioctls::Rqdata data = {};
		data.sendbuf = buffer;
		data.receivebuf = new UINT64();
		data.size = length;
		data.ret = 0;
		std::cout << "created data buffer " << std::hex << &data << std::dec << std::endl;

		auto ret = DeviceIoControl(driver, ioctls::umACmodule, &data, sizeof(data), &data, sizeof(data), 0, 0);

		return true;
	}
	return false;

}

bool SendPDB(HANDLE driver, std::string pdbName,void* fileData, UINT32 size) {

	//std::filesystem::path file = getModuleFromID(gameId);

	//if (std::filesystem::exists(file) && std::filesystem::is_regular_file(file)) {

		//std::cout << "Reading file:" << file << std::endl;

		//auto length = std::filesystem::file_size(file);
		//char* buffer = (char*)VirtualAlloc(0, length, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		//std::ifstream inputFile(file, std::ios_base::binary);
		//inputFile.seekg(0, std::ios::beg);
		//inputFile.read(buffer, length);
		//inputFile.close();


		ioctls::Rqdata data = {};
		ioctls::PPDB_Info pdbInfo = (ioctls::PPDB_Info)VirtualAlloc(0, sizeof(ioctls::PDB_Info), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		pdbInfo->dataPtr = fileData;
		pdbInfo->pdbLen = size;
		pdbInfo->moduleName = (char*)pdbName.c_str();
		pdbInfo->nameLen = pdbName.size();
		data.sendbuf = pdbInfo;
		data.receivebuf = new UINT64();
		data.size = sizeof(ioctls::PDB_Info);
		data.ret = 0;
		std::cout << "created data buffer " << std::hex << &data << std::dec << std::endl;

		auto ret = DeviceIoControl(driver, ioctls::umACmodule, &data, sizeof(data), &data, sizeof(data), 0, 0);

		return true;
	//}
	//return false;

}

bool hasSecurebootEnabled() {
	const char* group = "SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State";

	const char* key =  "UEFISecureBootEnabled";
	HKEY keyRes;

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, group, 0, KEY_QUERY_VALUE, &keyRes)) {
		DWORD type = REG_DWORD;
		DWORD res = 0;
		DWORD sz = 0;
		if(auto status = RegQueryValueExA(keyRes, key,0, &type,(unsigned char*) & res,&sz); status != 0 && sz == 4){
			if (status == ERROR_MORE_DATA) {
				//todo WTF, the value is now not a dword
			}
		}
	}
	return false;
}

BYTE ServiceExists() {
	auto scm = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	if (scm) {
		auto svs = OpenServiceA(scm, KAC_SERVICE_NAME, SC_MANAGER_ALL_ACCESS);
		if (svs == 0 && GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
			return FALSE;
		}
		else if (svs == 0)
			return KAC_OPENSERVICE_FAIL;
		else
		{
			return TRUE;
		}
	}
	else
		return KAC_SCMANGER_FAIL;
}

bool CreateKACService() {
	std::filesystem::path File("C:\\Program Files\\OpenKAC\\OpenKAC_Service.exe");
	if (std::filesystem::exists(File) && std::filesystem::is_regular_file(File)) {
		auto scm = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
		if (scm) {
			auto svs = CreateServiceA(scm, KAC_SERVICE_NAME, KAC_SERVICE_DISPLAY_NAME, SERVICE_ALL_ACCESS,
				SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
				File.string().c_str(), NULL, NULL, NULL, NULL, NULL
			);
			if (svs) {
				std::cout << "Created service" << std::endl;
				return true;
			}
			else {
				std::cout << "Failed to create service" << std::endl;
				CloseServiceHandle(scm);
			}
		}
		else return false;
	}
	else
		return false;

}
int BlackListedDriver() {
	DWORD bytesNeeded = 0;
	DWORD numServices = 0;
	DWORD resumeHandle = 0;

	SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (!scm) {
		std::cout << "Can not get scm manager" << std::endl;
		return GetLastError();
	}
	
	if (!EnumServicesStatusA(scm, SERVICE_DRIVER, SERVICE_ACTIVE, NULL, 0, &bytesNeeded, &numServices, 0))
	{
		auto ret = GetLastError();
		if (ret != ERROR_INSUFFICIENT_BUFFER && ret != ERROR_MORE_DATA) {
			std::cout << "Can not get enumiration size for driver services: " << ret << std::endl;
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
	std::set<std::string> drvs = { "KProcessHacker2" };
	for (DWORD i = 0; i < (numServices - 1); ++i)
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
	argh::parser cmdl(argc, argv);

	if (cmdl[{"-p", "--process"}]) {
		//TODO
	}
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	auto bldRes = BlackListedDriver();
	switch (bldRes)
	{
	case KAC_ENUM_DRV_PASS:
		break;
	case KAC_ENUM_DRV_BLACKLISTED:
		// TODO send info to user
		std::cout << "Found backlisted Driver" << std::endl;
		exit(0);
		break;
	default:
		std::cout << "Checking for backlisted driver failed: " << std::hex << bldRes << std::dec << std::endl;
		break;
	}
	if (ServiceExists() == FALSE) {
		CreateKACService();
	}

	//TODO call execute OpenKAC_Service.
	//TODO check if file exists
	auto scm = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	if (scm) {
		auto svs = OpenServiceA(scm, KAC_SERVICE_NAME, SC_MANAGER_ALL_ACCESS);
		if (svs == 0 && GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
			std::cout << "Can't get OpenKAC service. It doesn't exsist" << std::endl;
			return ERROR_SERVICE_DOES_NOT_EXIST;
		}
		else if (svs == 0) {
			std::cout << "Can't get OpenKAC service" << std::endl;
			return -1;
		}
		std::cout << "Starting Service" << std::endl;
		if (StartServiceA(svs, 0, 0) == 0) {
			auto err = GetLastError();
			std::cout << "StartService failed: " << err << std::endl;
			return err;
		}
		CloseServiceHandle(svs);
		//CloseServiceHandle(scm);
	}
	else {
		std::cout << "Can't open services manager" << std::endl;
		return 0;
	}
	auto driver = CreateFile(L"\\\\.\\OpenKAC", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (driver != INVALID_HANDLE_VALUE) {
		std::cout << "Driver already running please close the game that is using it." << std::endl;
		std::cin.get();
		//exit(0);
	}
	//std::this_thread::sleep_for(std::chrono::milliseconds(5));
	int msTime = 0;
	while (driver == INVALID_HANDLE_VALUE && msTime < 5000) {
		constexpr static int ms = 5;
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		msTime += ms;
		driver = CreateFile(L"\\\\.\\OpenKAC", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	}
	if (driver == INVALID_HANDLE_VALUE) {
		std::cout << "Driver not started. Please re-install, if you still get this error report on the github." << std::endl;
		std::cin.get();
		exit(0);
	}

	HANDLE proc = (HANDLE)GetCurrentProcessId();
	std::cout << "sending proc: " << std::hex << proc << std::dec << std::endl;
	SendProcID(driver, proc);
	std::string ci_path = std::string(std::getenv("systemroot")) + "\\System32\\ci.dll";
	ez::pdb ntos_pdb = ez::pdb(ci_path, "https://msdl.szdyg.cn/download/symbols");
	ntos_pdb.init();

	std::ifstream file(ci_path, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	char* buffer = (char*)VirtualAlloc(0,size,MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);


	if (!file.read(buffer, size) || size == 0)
	{
		SetLastError(ERROR_ACCESS_DENIED);
	}
	SendPDB(driver, "ci.dll", buffer,size);
	std::cout << "Sending DLL" << std::endl;
	SendACModule(driver, "");
	std::cin.get();
	MessageBoxA(GetActiveWindow(), "rtgnergieirg", "AAAAAAAAAAAAAAAAAA", MB_OK);
	CloseHandle(driver);
	return 0;
}