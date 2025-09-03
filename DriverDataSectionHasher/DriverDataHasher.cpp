// DriverDataSectionHasher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <argh.h>
#include <Windows.h>
extern "C" {
#include <winternl.h>
}
#include <memoryapi.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/rsa.h>
#include <cryptopp/authenc.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/files.h>
#include <string>
#include <filesystem>
#include <iostream>
#define LONESHA256_STATIC
#include "lonesha256.h"
std::string GetHashOfMemory(void* data, size_t size)
{
	unsigned char digest[CryptoPP::SHA256::DIGESTSIZE];
	//CryptoPP::SHA256().CalculateDigest(digest, (unsigned char*)data, size);
	//if (CryptoPP::SHA256().VerifyDigest(digest, (unsigned char*)data, size)) {
	if(lonesha256(digest, (unsigned char*)data,size) == 0){
		CryptoPP::HexEncoder encoder;
		std::string output;
		encoder.Attach(new CryptoPP::StringSink(output));
		encoder.Put(digest, sizeof(digest));
		encoder.MessageEnd();
		return output;
	}
	return "";
}
int main(int argc, char** argv)
{
	argh::parser cmdl(argc, argv);

	if (!cmdl[1].empty()) {
		
		std::string fileName;
		cmdl(1) >> fileName;
		if (!std::filesystem::exists(fileName)) {
			std::cout << "Can't find '" << fileName << "'" << std::endl;
			return 0;
		}
		std::ifstream inputFile(fileName, std::ios_base::binary);

		inputFile.seekg(0, std::ios_base::end);
		auto length = inputFile.tellg();
		inputFile.seekg(0, std::ios_base::beg);

		// Make a buffer of the exact size of the file and read the data into it.Dont need to free as it will exit when needing to free, so the os will do it.
		char* fileBuffer = (char*)VirtualAlloc(0, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		inputFile.read(fileBuffer, length);

		inputFile.close();


		PIMAGE_DOS_HEADER diskDOSheader = (PIMAGE_DOS_HEADER)fileBuffer;
		PIMAGE_NT_HEADERS diskModuleimageNTHeaders = (PIMAGE_NT_HEADERS)((UINT64)fileBuffer + diskDOSheader->e_lfanew);
		PIMAGE_SECTION_HEADER sectionLocation = IMAGE_FIRST_SECTION(diskModuleimageNTHeaders);

		for (int i = 0; i < diskModuleimageNTHeaders->FileHeader.NumberOfSections; i++) {
			PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)sectionLocation;
			std::string name = std::string((char*)sectionHeader->Name);
			if (name == ".text") {
				char* toHash = (char*)(((UINT64)fileBuffer) + sectionHeader->PointerToRawData);

				std::string hash = GetHashOfMemory(toHash, sectionHeader->SizeOfRawData);
				if(hash == "")
					return 1;
				std::cout << ".text: " << hash << std::endl;
			}
			if (name == "PAGE") {
				char* toHash = (char*)(((UINT64)fileBuffer) + sectionHeader->PointerToRawData);

				std::string hash = GetHashOfMemory(toHash, sectionHeader->SizeOfRawData);
				if (hash == "")
					return 1;
				std::cout << "PAGE: " << hash << std::endl;
			}
			if (name == ".rdata") {
				char* toHash = (char*)(((UINT64)fileBuffer) + sectionHeader->PointerToRawData);

				std::string hash = GetHashOfMemory(toHash, sectionHeader->SizeOfRawData);
				if (hash == "")
					return 1;
				std::cout << ".rdata: " << hash << std::endl;
			}
			if (name == "INIT") {
				char* toHash = (char*)(((UINT64)fileBuffer) + sectionHeader->PointerToRawData);

				std::string hash = GetHashOfMemory(toHash, sectionHeader->SizeOfRawData);
				if (hash == "")
					return 1;
				std::cout << "INIT: " << hash << std::endl;
			}
			sectionLocation++;
		}
	}
}
