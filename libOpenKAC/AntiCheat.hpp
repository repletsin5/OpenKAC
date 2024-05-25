#pragma once
#include <Windows.h>


class AntiCheat {

public:
	AntiCheat();

	~AntiCheat();

	AntiCheat(const AntiCheat&) = delete;
	AntiCheat& operator=(AntiCheat const&) = delete;

	bool Heartbeat();

	void CreateHandleVerifyThread();

	void CreateProcessKillingThread();
private:
	HANDLE drvHandle = 0;
	bool shouldKillProcess = false;
};