#include <windows.h>
#include <winsvc.h>
#include <versionhelpers.h>
#include <process.h>
#include <iostream>
#include <filesystem>
#include <tchar.h>
#include <strsafe.h>
SERVICE_STATUS svcStatus;
SERVICE_STATUS_HANDLE svcStatusHandle;
HANDLE svsEvent = 0 ;
#include "def.hpp"
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint);

void WINAPI ServiceHandlerProc(DWORD dwControl)
{
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(svsEvent);
		ReportSvcStatus(svcStatus.dwCurrentState, NO_ERROR, 0);

		return; 

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

	return;
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

VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	svcStatus.dwCurrentState = dwCurrentState;
	svcStatus.dwWin32ExitCode = dwWin32ExitCode;
	svcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		svcStatus.dwControlsAccepted = 0;
	else svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		svcStatus.dwCheckPoint = 0;
	else svcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(svcStatusHandle, &svcStatus);
}

void ServiceProc(DWORD dwNumServicesArgs, LPSTR* lpServiceArgVectors) {

	UNREFERENCED_PARAMETER(dwNumServicesArgs);
	UNREFERENCED_PARAMETER(lpServiceArgVectors);

	svcStatusHandle = RegisterServiceCtrlHandlerA(KAC_SERVICE_NAME,ServiceHandlerProc);
	if (svcStatusHandle != 0) {
		svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		svcStatus.dwServiceSpecificExitCode = 0;
	}

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	svsEvent = CreateEventA(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name
	if (svsEvent == 0) {
		std::cout << "Can not create service event" << std::endl;
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}
	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	while (1)
	{
	
		WaitForSingleObject(svsEvent, INFINITE);

		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);

		return;

	}
}

VOID SvsReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCSTR lpszStrings[2];
	CHAR Buffer[80];

	hEventSource = RegisterEventSourceA(NULL, KAC_SERVICE_NAME);

	if (NULL != hEventSource)
	{
		StringCchPrintfA(Buffer, 80,"%s failed with %d", szFunction, GetLastError());

		lpszStrings[0] = KAC_SERVICE_NAME;
		lpszStrings[1] = Buffer;

		ReportEventA(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			1,					 // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}
//https://stackoverflow.com/questions/55906598/iswindows10orgreater-always-returns-false-even-with-manifest-file
//normal IsWindows10OrGreater always returned false even when on windows 10.
inline bool isWindows10OrGreater()
{
	
	INT32(NTAPI * RtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation) = nullptr;
	HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
	if (ntdll == NULL)
		return false;
	*reinterpret_cast<FARPROC*>(&RtlGetVersion) = GetProcAddress(ntdll, "RtlGetVersion");
	if (RtlGetVersion == nullptr)
		return false;
	OSVERSIONINFOEX versionInfo{ sizeof(OSVERSIONINFOEX) };
	if (RtlGetVersion(reinterpret_cast<LPOSVERSIONINFO>(&versionInfo)) < 0)
		return false;

	if (versionInfo.dwMajorVersion > HIBYTE(_WIN32_WINNT_WIN10))
		return true;

	if (versionInfo.dwMajorVersion == HIBYTE(_WIN32_WINNT_WIN10))
	{
		if (versionInfo.dwMinorVersion > LOBYTE(_WIN32_WINNT_WIN10))
			return true;

		if (versionInfo.dwMinorVersion == LOBYTE(_WIN32_WINNT_WIN10) && versionInfo.dwBuildNumber >= 19041)
			return true;
	}

	return false;
}


int main(int argc, char** argv) {

	if (!isWindows10OrGreater()) {
		MessageBoxA(0, "Only supports Windows 10 or greater", "Error", MB_OK);
		return 0;
	}	
	if (ServiceExists() == FALSE) {
		CreateKACService();
	}

	auto scm = OpenSCManagerA(0, 0, SC_MANAGER_ALL_ACCESS);
	if (scm) {
		auto svs = OpenServiceA(scm, KAC_SERVICE_NAME, SC_MANAGER_ALL_ACCESS);
		if (svs == 0 && GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
			std::cout << "Can't get OpenKAC service. It doesn't exsist" << std::endl;
			return ERROR_SERVICE_DOES_NOT_EXIST;
		}
		else if (svs == 0)
			std::cout << "Can't get OpenKAC service" << std::endl;
		std::cout << "Starting Service" << std::endl;
		if (StartServiceA(svs, 0, 0) == 0) {
			auto err = GetLastError();
			std::cout << "StartService failed: " << err << std::endl;
			return err;
		}
	}
	else
		std::cout << "Can't open services manager" << std::endl;
	SERVICE_TABLE_ENTRYA ste[] = { {KAC_SERVICE_NAME, (LPSERVICE_MAIN_FUNCTIONA)ServiceProc},{0,0} };
	if (StartServiceCtrlDispatcherA(ste)) {
		auto err = GetLastError();
		std::cout << "StartServiceCtrlDispatcher error: "<< err << std::endl;

		return err;
	}

}