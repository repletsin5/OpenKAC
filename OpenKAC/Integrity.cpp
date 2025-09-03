#include "Integrity.hpp"
#include "memory.hpp"
#include "Defs.h"
#include <bcrypt.h>
#include <ntstrsafe.h>
#include <runtime/Helpers.hpp>
#pragma code_seg(push)
#pragma code_seg("PAGE")

char txtString[8] = ".text";
char pageString[8] = "PAGE";
char rdataString[8] = ".rdata";
char initString[8] = "INIT";

int OpenKAC::Integrity::CreateSha256(UCHAR** hashOut, char* ptr, size_t size)
{
	PAGED_CODE();

	if (hashHandle == 0) {
		*hashOut = 0;
		return -1;
	}
	if (ptr == 0) {
		*hashOut = 0;
		return -1;
	}

	if (!NT_SUCCESS(BCryptHashData(
		hashHandle,
		(PBYTE)ptr,
		size,
		0))) {

		*hashOut = 0;
		return -1;
	}
	*hashOut = new BYTE(cbHash);
	if (!NT_SUCCESS(BCryptFinishHash(hashHandle,*hashOut, cbHash, 0))) {

		*hashOut = 0;
		return -1;
	}

	return cbHash;
}



void OpenKAC::Integrity::InitHashGen()
{
	PAGED_CODE();

	DWORD cbData = 0;

	BCryptOpenAlgorithmProvider(&algoHandle, BCRYPT_SHA256_ALGORITHM, 0, BCRYPT_HASH_REUSABLE_FLAG);
	BCryptGetProperty(algoHandle, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0);
	pbHashObject = new BYTE(cbHashObject);

	BCryptGetProperty(&algoHandle, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0);

	BCryptCreateHash(algoHandle, &hashHandle, pbHashObject,
		cbHashObject,
		NULL,
		0,
		BCRYPT_HASH_REUSABLE_FLAG);

}
void OpenKAC::Integrity::UnInitHashGen()
{
	PAGED_CODE();

	if (algoHandle)
	{
		BCryptCloseAlgorithmProvider(algoHandle, 0);
		algoHandle = 0;
	}

	if (hashHandle)
	{
		BCryptDestroyHash(hashHandle);
		hashHandle = 0;
	}

	if (pbHashObject)
	{
		delete pbHashObject;
		pbHashObject = 0;
	}

}
void OpenKAC::Integrity::CheckHashesOfDriver(PSysModule driver, PSysModuleSectionSignatures sigs)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(sigs);

	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)driver->addr;
	PIMAGE_NT_HEADERS loadedModuleimageNTHeaders = (PIMAGE_NT_HEADERS)((UINT64)driver->addr + dosHeader->e_lfanew);
	PIMAGE_SECTION_HEADER loadedModulesectionLocation = IMAGE_FIRST_SECTION(loadedModuleimageNTHeaders);
	UINT64 loadedModulesectionSize = sizeof(IMAGE_SECTION_HEADER);
	UCHAR* hashOut = 0;

	//TODO: remove copied code.
	for (int i = 0; i < loadedModuleimageNTHeaders->FileHeader.NumberOfSections; i++) {
		PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)loadedModulesectionLocation;
		if (strcmp((char*)sectionHeader->Name, txtString) == 0) {
			UINT64 sectionbase = driver->addr + sectionHeader->VirtualAddress;
			kdprint("%llx\n", sectionbase);

			//BCryptHashData(hashHandle, (PUCHAR)sectionbase, sectionHeader->SizeOfRawData, 0);

			//BCryptFinishHash(
			//	hashHandle,
			//	hashOut,
			//	cbHash,
			//	0);
			//lonesha256(hashOut, (PUCHAR)sectionbase, sectionHeader->SizeOfRawData);

			char* outHash = 0;
			char* stringifiedHash = 0;

			int hashSize = CreateSha256(&hashOut, (CHAR*)sectionbase, sectionHeader->SizeOfRawData);
			if (hashSize >= 0) {
				UINT32 reqSize = (hashSize * 2) + 1;
				stringifiedHash = new char((hashSize * 2) + 1);

				for (int i = 0; i < hashSize; i++) {
					RtlStringCchPrintfA(&stringifiedHash[0], reqSize, "%s%X", stringifiedHash, hashOut[i]);
				}
				stringifiedHash[reqSize - 1] = '\0';
				kdprint("Hashing .text\n");
				kdprint("Hash %s\n", stringifiedHash);
				delete stringifiedHash;

			}
		}
		else if (strcmp((char*)sectionHeader->Name, pageString) == 0) {
			UINT64 sectionbase = driver->addr + sectionHeader->VirtualAddress;
			//lonesha256(hashOut, (PUCHAR)sectionbase, sectionHeader->SizeOfRawData);
			char* outHash = 0;
			char* stringifiedHash = 0;

			int hashSize = CreateSha256(&hashOut, (CHAR*)sectionbase, sectionHeader->SizeOfRawData);
			if (hashSize >= 0) {
				UINT32 reqSize = (hashSize * 2) + 1;
				stringifiedHash = new char((hashSize * 2) + 1);

				for (int i = 0; i < hashSize; i++) {
					RtlStringCchPrintfA(&stringifiedHash[0], reqSize, "%s%X", stringifiedHash, hashOut[i]);
				}
				stringifiedHash[reqSize - 1] = '\0';
				kdprint("Hashing .text\n");
				kdprint("Hash %s\n", stringifiedHash);
				delete stringifiedHash;

			}

		}
		else if (strcmp((char*)sectionHeader->Name, rdataString) == 0) {
			UINT64 sectionbase = driver->addr + sectionHeader->VirtualAddress;
			//lonesha256(hashOut, (PUCHAR)sectionbase, sectionHeader->SizeOfRawData);
			char* outHash = 0;
			char* stringifiedHash = 0;

			int hashSize = CreateSha256(&hashOut, (CHAR*)sectionbase, sectionHeader->SizeOfRawData);
			if (hashSize >= 0) {
				UINT32 reqSize = (hashSize * 2) + 1;
				stringifiedHash = new char((hashSize * 2) + 1);

				for (int i = 0; i < hashSize; i++) {
					RtlStringCchPrintfA(&stringifiedHash[0], reqSize, "%s%X", stringifiedHash, hashOut[i]);
				}
				stringifiedHash[reqSize - 1] = '\0';
				kdprint("Hashing .text\n");
				kdprint("Hash %s\n", stringifiedHash);
				delete stringifiedHash;
			}

		}
		else if (strcmp((char*)sectionHeader->Name, initString) == 0) {
			UINT64 sectionbase = driver->addr + sectionHeader->VirtualAddress;
			//lonesha256(hashOut, (PUCHAR)sectionbase, sectionHeader->SizeOfRawData);
			char* outHash = 0;
			char* stringifiedHash = 0;

			int hashSize = CreateSha256(&hashOut, (CHAR*)sectionbase, sectionHeader->SizeOfRawData);
			if (hashSize >= 0) {
				UINT32 reqSize = (hashSize * 2) + 1;
				stringifiedHash = new char((hashSize * 2) + 1);

				for (int i = 0; i < hashSize; i++) {
					RtlStringCchPrintfA(&stringifiedHash[0], reqSize, "%s%X", stringifiedHash, hashOut[i]);
				}
				stringifiedHash[reqSize - 1] = '\0';
				kdprint("Hashing .text\n");
				kdprint("Hash %s\n", stringifiedHash);
				delete stringifiedHash;

			}

		}
		if (hashOut != 0)
			delete hashOut;
		loadedModulesectionLocation++;
	}

}
#pragma code_seg(pop)