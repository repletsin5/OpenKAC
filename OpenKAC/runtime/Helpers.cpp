#include "Helpers.hpp"

EXTERN_C PLIST_ENTRY PsLoadedModuleList;
#pragma code_seg(push)
#pragma code_seg("PAGE")
//https://www.unknowncheats.me/forum/1897834-post25.html
__forceinline BOOLEAN bDataCompare(const char* data, const char* bMask, const char* szMask) // no null checks here as it should fail before here
{
	//PAGED_CODE();
	
	for (; *szMask; ++szMask, ++data, ++bMask)
		if (*szMask == 'x' && *data != *bMask)
			return 0;

	return (*szMask) == 0;
}

UINT64 OpenKAC::Modules::FindPattern(PSysModule mod, const char* bMask, const char* szMask)
{
	PAGED_CODE();
	if (mod == 0) {
		KdPrintEx((0, 0, "mod is nullptr, please fix"));
		KdBreakPoint();
		return 0;
	}
	if (bMask == 0) {
		KdPrintEx((0, 0, "mask is nullptr, please fix"));
		KdBreakPoint();
		return 0;
	}
	if (szMask == 0) {
		KdPrintEx((0, 0, "mask pattern is nullptr, please fix"));
		KdBreakPoint();
		return 0;
	}
	for (UINT64 i = 0; i < mod->size; i++) {
		if (bDataCompare((const char*)mod->addr + i, bMask, szMask))
			return (UINT64)(mod->addr + i);
	}
	return 0;
}


void OpenKAC::Modules::GetSystemModuleBase(IN OUT PSysModule mod, IN const wchar_t* name)
{
	PAGED_CODE();
	if (mod == 0) {
		KdPrintEx((0, 0, "mod is nullptr, please fix"));
		KdBreakPoint();
		return;
	}
	if (name == 0) {
		KdPrintEx((0, 0, "name is nullptr, please fix"));
		KdBreakPoint();
		return;
	}
	mod->addr = 0;
	mod->size = 0;
	UNICODE_STRING str;
	RtlInitUnicodeString(&str, name);
	if (IsListEmpty(PsLoadedModuleList))
		return;
	for (PLIST_ENTRY pListEntry = PsLoadedModuleList->Flink; pListEntry != PsLoadedModuleList; pListEntry = pListEntry->Flink)
	{
		PKLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if ((&str && RtlCompareUnicodeString(&pEntry->BaseDllName, &str, TRUE) == 0))
		{
			KdPrintEx((0, 0, "[OpenKAC] Found %ws at: 0x%I64x\n", name, (UINT64)(pEntry->DllBase)));
			mod->addr = (UINT64)pEntry->DllBase;
			mod->size = pEntry->SizeOfImage;
			return;
		}
	}
}

#pragma code_seg(pop)

UINT64 OpenKAC::_SysModule::FindPattern(const char* bMask, const char* szMask)
{
	return Modules::FindPattern(this, bMask, szMask);
}
