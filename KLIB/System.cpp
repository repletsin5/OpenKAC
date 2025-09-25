#include "System.hpp"
#include "kString.hpp"
#include "Memory.hpp"
#include "Array.hpp"


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

	UINT64 klib::Modules::FindPattern(klib::PSysModule mod, const char* bMask, const char* szMask)
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
	UINT64 klib::Modules::FindPatternIDA(klib::PSysModule mod, const char* pattern, size_t patternSize) {

		return 0;
	}


	void klib::Modules::GetSystemModuleBase(IN OUT klib::PSysModule mod, IN const wchar_t* name)
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
		//UNICODE_STRING str;
		//RtlInitUnicodeString(&str, name);
		klib::ukString str(name);
		if (IsListEmpty(PsLoadedModuleList))
			return;
		for (PLIST_ENTRY pListEntry = PsLoadedModuleList->Flink; pListEntry != PsLoadedModuleList; pListEntry = pListEntry->Flink)
		{
			PKLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

			if ((&str && RtlCompareUnicodeString(&pEntry->BaseDllName, str.str(), TRUE) == 0))
			{
				KdPrintEx((0, 0, "[kLib] Found %ws at: 0x%I64x\n", name, (UINT64)(pEntry->DllBase)));
				mod->addr = (UINT64)pEntry->DllBase;
				mod->size = pEntry->SizeOfImage;
				return;
			}
		}
	}

	void klib::Modules::GetDriverObjecFromDeviceName(OUT ::PDRIVER_OBJECT* obj, IN klib::ukString& name)
	{
		OBJECT_ATTRIBUTES attributes;
		HANDLE directoryHandle = 0;
		POBJECT_DIRECTORY driverRootDir;
		PDRIVER_OBJECT driver = NULL;
		NTSTATUS status = STATUS_SUCCESS;

		if (name.str() == nullptr) {
			obj = nullptr;
			return;
		}
		klib::ukString baseDirName(L"\\driver");
		InitializeObjectAttributes(&attributes, baseDirName.str(), OBJ_CASE_INSENSITIVE, NULL, NULL);

		status = ZwOpenDirectoryObject(&directoryHandle, DIRECTORY_ALL_ACCESS, &attributes);
		if (!NT_SUCCESS(status)) {
			KdPrintEx((0,0,"ZwOpenDirectoryObject failed with status %x\n", status));
			return;
		}
		status = ObReferenceObjectByHandle(directoryHandle, DIRECTORY_ALL_ACCESS, 0, KernelMode, (void**)&driverRootDir, 0);
		if (!NT_SUCCESS(status)) {
			KdPrintEx((0,0,"ObReferenceObjectByHandle failed with status %x\n", status));
			ZwClose(directoryHandle);
			return;
		}
		if (driverRootDir == nullptr) {
			
			KdPrintEx((0,0,"[kLib] driverRootDir is null\n"));
			ZwClose(directoryHandle);
			return;
		}
		KeEnterCriticalRegion();
		{
			ExAcquirePushLockExclusiveEx(&driverRootDir->Lock, NULL);

			int size = sizeof(driverRootDir->HashBuckets) / sizeof(_OBJECT_DIRECTORY_ENTRY*);

			for (int i = 0; i < size - 1; i++) {

				auto entry = driverRootDir->HashBuckets[i];
				if (!isValidKernelAddress((UINT64)entry)) {
					KdPrintEx((0, 0, "[kLib] Invalid driver object hash entry 0x%I64x\n", (UINT64)entry));
				}
				while (isValidKernelAddress((UINT64)entry)) {
					driver = (PDRIVER_OBJECT)entry->Object;

					if (!RtlCompareUnicodeString(
						name.str(),
						&driver->DriverName,
						FALSE)) {
						*obj = driver;
						ExReleasePushLockExclusiveEx(&driverRootDir->Lock, 0);

						KeLeaveCriticalRegion(); ////

						ObDereferenceObject(driverRootDir);
						ZwClose(directoryHandle);
						return;
					}

					entry = entry->ChainLink;
				}

			}

			ExReleasePushLockExclusiveEx(&driverRootDir->Lock, 0);
		}
		KeLeaveCriticalRegion();
		ObDereferenceObject(driverRootDir);
		ZwClose(directoryHandle);

	}

	void klib::Modules::GetDriverObjects(IN OUT klib::std::array<PDRIVER_OBJECT>& objs)
	{
		//TODO: requires array to be implemented.
		return;

		OBJECT_ATTRIBUTES attributes;
		HANDLE directoryHandle = 0;
		POBJECT_DIRECTORY driverRootDir;
		NTSTATUS status = STATUS_SUCCESS;

		klib::ukString baseDirName(L"\\driver");
		InitializeObjectAttributes(&attributes, baseDirName.str(), OBJ_CASE_INSENSITIVE, NULL, NULL);

		status = ZwOpenDirectoryObject(&directoryHandle, DIRECTORY_ALL_ACCESS, &attributes);
		if (!NT_SUCCESS(status)) {
			KdPrintEx((0, 0, "[kLib] ZwOpenDirectoryObject failed with status %\n", status));
			return;
		}
		status = ObReferenceObjectByHandle(directoryHandle, DIRECTORY_ALL_ACCESS, 0, KernelMode, (void**)&driverRootDir, 0);
		if (!NT_SUCCESS(status)) {
			KdPrintEx((0, 0, "[kLib] ObReferenceObjectByHandle failed with status %x\n", status));
			ZwClose(directoryHandle);
			return;
		}
		if (driverRootDir == nullptr) {

			KdPrintEx((0, 0, "[kLib] driverRootDir is null\n"));
			ZwClose(directoryHandle);
			return;
		}
		KeEnterCriticalRegion();
		{
			ExAcquirePushLockExclusiveEx(&driverRootDir->Lock, NULL);

			constexpr int size = sizeof(driverRootDir->HashBuckets) / sizeof(_OBJECT_DIRECTORY_ENTRY*);
			objs = klib::std::array<PDRIVER_OBJECT>(size);


			UINT64 totalCount = 0;
			for (int i = 0; i < size - 1; i++) {

				auto entry = driverRootDir->HashBuckets[i];
				if (!isValidKernelAddress((UINT64)entry)) {
					KdPrintEx((0, 0, "[kLib] Invalid driver object hash entry 0x%I64x\n", (UINT64)entry));
					continue;
				}
				while (isValidKernelAddress((UINT64)entry)) {

					totalCount++;
					entry = entry->ChainLink;
				}

			}
			for (int i = 0; i < size - 1; i++) {

				auto entry = driverRootDir->HashBuckets[i];
				if (!isValidKernelAddress((UINT64)entry)) {
					KdPrintEx((0, 0, "[kLib] Invalid driver object hash entry 0x%I64x\n", (UINT64)entry));
					continue;
				}
				while (isValidKernelAddress((UINT64)entry)) {

					totalCount++;
					entry = entry->ChainLink;
				}

			}
			ExReleasePushLockExclusiveEx(&driverRootDir->Lock, 0);
		}
		KeLeaveCriticalRegion();
		ObDereferenceObject(driverRootDir);
		ZwClose(directoryHandle);
	}

	UINT64 klib::_SysModule::FindPattern(const char* bMask, const char* szMask)
	{
		return klib::Modules::FindPattern(this, bMask, szMask);
	}
#pragma code_seg(pop)
