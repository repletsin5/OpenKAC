#include "DriverVerification.hpp"
#include <intrin.h>
#include <runtime/Helpers.hpp>
#include <skCrypter.h>
using namespace OpenKAC;
#pragma code_seg(push)
#pragma code_seg("PAGE")
void OpenKAC::DriverVerification::CheckWdFilter()
{
	PAGED_CODE();

	//TODO: Only check on OS ver that 100% we know it works.

	SysModule mod = {};
	Modules::GetSystemModuleBase(&mod, skCrypt(L"WdFilter.sys"));
	if (mod.addr == 0) {
		KdPrintEx((0, 0, "Cannot get WdFilter.sys module\n"));
		return;
	}
	//https://www.unknowncheats.me/forum/anti-cheat-bypass/509917-eac-taking-advantage-wdfilter-driver.html
	// 48 8B 0D ? ? ? ? FF 05
	auto runtimeDriversList = mod.FindPattern(skCrypt("\x48\x8B\x0D\x0\x0\x0\x0\xFF\x05"), skCrypt("xxx????xx"));
	if (runtimeDriversList == 0) {
		kdprint("Cannot get runtimeDriversList pattern in WdFilter.sys module\n");
		return;
	}
	//seems to overflow to go back instead of subtracting :)
	//INT64 offset = 0xffffffff00000000 | *iptr;
	INT64 offset =  Modules::GetOffset(runtimeDriversList, (short)3);
	KdPrintEx((0, 0, "[OpenKAC] rtdl instruc %llx\n", runtimeDriversList));
	KdPrintEx((0, 0, "[OpenKAC] rtdl off low %08x\n", (INT32)offset));

	KdPrintEx((0, 0, "[OpenKAC] rtdl offset %llx\n", offset));
	KdPrintEx((0, 0, "[OpenKAC] rtdl addr %llx\n", runtimeDriversList + 0x7 + offset));

	//Explaination: RIP + offset - sizeof(UINT64)
	auto head = (PLIST_ENTRY)((runtimeDriversList + 0x7) + (offset)-0x8);

	//FF 05 ? ? ? ? 48 39 11
	//auto runtimeDriversListCount = FindPattern(&mod, "\xFF\x05\x00\x00\x00\x00\x48\x39\x11", "xx????xxx");
	//if (runtimeDriversList == 0) {
	//	KdPrintEx((0, 0, "Cannot get runtimeDriversListCount pattern in WdFilter.sys module\n"));
	//	return;
	//}

	//Check if somehow this fucked up. We do not wont to BSOD
	//if ((UINT64)head < mod.addr || (UINT64)head >(mod.addr + mod.size)) {
	if (!mod.IsWithinModule((UINT64)head)) {
		KdPrintEx((0, 0, "[OpenKAC] rtdl head addr seem to be invalid, returning to avoid BSOD:\n\tHead:0x%llx\n\tRTDL:0x%llx ", (UINT64)head, (UINT64)runtimeDriversList));
		return;
	}
	KdPrintEx((0, 0, "[OpenKAC] Runtime driver list:\n"));


	for (PLIST_ENTRY pListEntry = head->Flink; pListEntry != head; pListEntry = pListEntry->Flink)
	{
		PMP_DRIVER_INFO pEntry = CONTAINING_RECORD(pListEntry, MP_DRIVER_INFO, DriverInfoList);
		KdPrintEx((0, 0, "%wZ\n", pEntry->ImageName));
		//TODO: Do actual checks
	}
}
void OpenKAC::DriverVerification::CheckHashBucketList() {
	SysModule mod = {};
	Modules::GetSystemModuleBase(&mod, skCrypt(L"CI.dll"));

	auto bucketListSig = mod.FindPattern(skCrypt("\x48\x8B\x1D\x00\x00\x00\x00\xEB\x00\xF7\x43\x40\x00\x20\x00\x00"), skCrypt("xxx????x?xxxxxxx"));
	auto bucketListLockSig = mod.FindPattern(skCrypt("\x48\x8D\x0D"), skCrypt("xxx"));
	if (!bucketListLockSig || !bucketListSig) {
		KdPrintEx((0,0,"[OpenKAC] Can't Find Hash bucket offsets"));
		return;
	}

	UINT64 bucketList = Modules::GetOffset(bucketListSig + 0x3, (short)7);
	UINT64 bucketListLock = Modules::GetOffset(bucketListLockSig + 0x3, (short)7);

	if (!mod.IsWithinModule(bucketList))
	{

	}
	if (!mod.IsWithinModule(bucketListLock)) {

	}

	ExAcquireResourceExclusiveLite((PERESOURCE)bucketListLock,true);
}
void OpenKAC::DriverVerification::TimingCheck()
{
	PAGED_CODE();
	//TODO: NOT DONE
	auto a = __rdtsc();
	__cpuid(0, 0);
	auto b = __rdtsc();
	auto c = b - a;
	LARGE_INTEGER delay;
	delay.QuadPart = -(10000 * 500);
	UINT64 avg = 0;
	for (int i = 0; i < 10; i++) {
		auto a2 = __rdtsc();
		UNREFERENCED_PARAMETER(a2);
		__cpuid(0, 0);
		auto b2 = __rdtsc();
		UNREFERENCED_PARAMETER(b2);
		avg += c;
		KeDelayExecutionThread(KernelMode, FALSE, &delay);

	}
	avg = avg / 10;
	bool s = (avg < 1000 && avg > 0) ? FALSE : TRUE;
	UNREFERENCED_PARAMETER(s);
}
void OpenKAC::DriverVerification::CheckTPM2dot0()
{
}
#pragma code_seg(pop)