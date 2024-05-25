#include <windows.h>
#include <winsvc.h>
#include <versionhelpers.h>
#include <process.h>
#include <iostream>
#include <filesystem>

#include "def.hpp"

DWORD ServiceHandlerProc(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext)
{
	UNREFERENCED_PARAMETER(dwControl);
	UNREFERENCED_PARAMETER(dwEventType);
	UNREFERENCED_PARAMETER(lpEventData);
	UNREFERENCED_PARAMETER(lpContext);

	return 0;
}

bool CreateKACService() {
	std::filesystem::path sysFile("C:\\Program Files\\OpenKAC\\OpenKAC.sys");
	if (std::filesystem::exists(sysFile) && std::filesystem::is_regular_file(sysFile)) {
		auto scm = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
		if (scm) {
			auto svs = CreateServiceA(scm, KAC_SERVICE_NAME, KAC_SERVICE_DISPLAY_NAME, SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
				sysFile.string().c_str(), NULL, NULL, NULL, NULL, NULL
			);
			if (svs) {
				//TODO:
			}
			else {
				std::cout << "Failed to create service";
				CloseServiceHandle(scm);
			}

		}
		else return false;
	}
	else
		return false;

}
 
BYTE ServiceExists() {
	auto scm = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	if (scm) {
		auto svs = OpenServiceA(scm, KAC_SERVICE_NAME, SC_MANAGER_ALL_ACCESS);
		if (svs == 0 && GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST ) {
			return FALSE;
		}
		else if(svs == 0)
			return KAC_OPENSERVICE_FAIL;
		else
		{
			return TRUE;
		}
	}
	else
		return KAC_SCMANGER_FAIL;
}
void ServiceProc(DWORD dwNumServicesArgs, LPSTR* lpServiceArgVectors) {

	UNREFERENCED_PARAMETER(dwNumServicesArgs);
	UNREFERENCED_PARAMETER(lpServiceArgVectors);

	SERVICE_STATUS_HANDLE status = RegisterServiceCtrlHandlerExA(KAC_SERVICE_PROC_NAME,&ServiceHandlerProc,0);
	if (status != 0) {

	}
	if (ServiceExists() == FALSE) {
		CreateKACService();
	}
	else {
		// why are we here, we shouldn't be. 
	
	}
}





int main(int argc, char** argv) {

	if (!IsWindows10OrGreater()) {
		MessageBoxA(0, "Only supports Windows 10 or greater", "Error", MB_OK);
		return 0;
	}
	SERVICE_TABLE_ENTRYA ste;
	ste.lpServiceName = KAC_SERVICE_PROC_NAME;
	ste.lpServiceProc = &ServiceProc;
	if (StartServiceCtrlDispatcherA(&ste)) {
		//TODO: error handling
		return 0;
	}

}