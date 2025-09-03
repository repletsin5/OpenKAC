#include "AntiCheat.hpp"
#include "AntiCheat.hpp"
#include <exception>
#include <ctime>
#include <thread>

AntiCheat::AntiCheat()
{
	drvHandle = CreateFile(L"\\\\.\\OpenKAC", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (drvHandle == INVALID_HANDLE_VALUE)
		throw std::exception("AntiCheat is not open");
}

AntiCheat::~AntiCheat()
{
	if (drvHandle != 0 && drvHandle != INVALID_HANDLE_VALUE)
		CloseHandle(drvHandle);
}

bool AntiCheat::Heartbeat()
{
	return false;
}

void AntiCheat::CreateHandleVerifyThread()
{
	std::thread t([&] {
		WaitForSingleObject(drvHandle, INFINITE);
		shouldKillProcess = true;
		});
	t.detach();
}

void AntiCheat::CreateProcessKillingThread()
{
	std::thread t([&] {
		while (!shouldKillProcess) {
			auto rnd = (rand() % (30 - 15 + 1) + 15);
			std::this_thread::sleep_for(std::chrono::milliseconds(rnd));
		}
		exit(0);
		});
	t.detach();
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved)  // reserved
{
	// Perform actions based on the reason for calling.
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:

		if (lpvReserved != nullptr)
		{
			break; // do not do cleanup if process termination scenario
		}

		// Perform any necessary cleanup.
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}