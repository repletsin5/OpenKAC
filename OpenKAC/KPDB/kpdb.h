#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <ntimage.h>
#include <minwindef.h>
#include <intrin.h>
#include <ntddndis.h>
#include <fltkernel.h>

#ifndef STRSAFE_MAX_CCH
#define STRSAFE_MAX_CCH     2147483647  // max buffer size, in characters, that we support (same as INT_MAX)
#endif
C_ASSERT(STRSAFE_MAX_CCH <= 2147483647);
C_ASSERT(STRSAFE_MAX_CCH > 1);

#define STRSAFE_MAX_LENGTH  (STRSAFE_MAX_CCH - 1)   // max buffer length, in characters, that we support
#include "tpi.h"

static const char kMagic[32] = {
	0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x43, 0x2F,
	0x43, 0x2B, 0x2B, 0x20, 0x4D, 0x53, 0x46, 0x20, 0x37, 0x2E, 0x30, 0x30,
	0x0D, 0x0A, 0x1A, 0x44, 0x53, 0x00, 0x00, 0x00
};


#pragma pack(push, 1)
typedef struct _SuperBlock
{
	CHAR  FileMagic[sizeof(kMagic)];
	DWORD BlockSize;
	DWORD FreeBlockMapBlock;
	DWORD NumBlocks;
	DWORD NumDirectoryBytes;
	DWORD Unknown;
	DWORD BlockMapAddr;
}SuperBlock;

typedef struct _StreamData {
	PCHAR StreamPointer;
	SIZE_T StreamSize;
}StreamData;

typedef struct _DBIHeader
{
	LONG	VersionSignature;
	DWORD	VersionHeader;
	DWORD	Age;
	WORD	GlobalStreamIndex;
	WORD	BuildNumber;
	WORD	PublicStreamIndex;
	WORD	PdbDllVersion;
	WORD	SymRecordStream;
	WORD	PdbDllRbld;
	LONG	ModInfoSize;
	LONG	SectionContributionSize;
	LONG	SectionMapSize;
	LONG	SourceInfoSize;
	LONG	TypeServerSize;
	DWORD	MFCTypeServerIndex;
	LONG	OptionalDbgHeaderSize;
	LONG	ECSubstreamSize;
	WORD	Flags;
	WORD	Machine;
	DWORD	Padding;
}DBIHeader;

typedef struct _PUBSYM32
{
	WORD reclen;     // Record length
	WORD rectyp;     // S_PUB32
	DWORD pubsymflags;
	DWORD off;
	WORD seg;
	char name[1];    // Length-prefixed name
}PUBSYM32;

enum SYM_ENUM_e {
	// [�]
	S_CONSTANT = 0x1107,  // constant symbol
	S_UDT = 0x1108,  // User defined type
	S_LDATA32 = 0x110c,  // Module-local symbol
	S_GDATA32 = 0x110d,  // Global data symbol
	S_PUB32 = 0x110e, // a public symbol (CV internal reserved)
	S_PROCREF = 0x1125, // Reference to a procedure
	S_LPROCREF = 0x1127, // Local Reference to a procedure
	// [�]
};

typedef struct _SYMBOL_DATA {
	PCHAR SymbolName;
	//UINT SymbolNameHash;
	DWORD SectionOffset;
	DWORD SymbolRVA;
	WORD SectionNumber;
} SYMBOL_DATA, * PSYMBOL_DATA;
#pragma pack(pop)

BOOL KpdbIsPDBMagicValid(SuperBlock* super);
PVOID KpdbGetPDBStreamDirectory(PVOID base);
StreamData* KpdbGetPDBStreams(PVOID base, DWORD* streams_count);
BOOL KpdbGetPDBSymbolOffset(PVOID pdbfile, PSYMBOL_DATA SymbolDataList);
void KpdbConvertSecOffsetToRVA(DWORD64 ModuleBase, PSYMBOL_DATA SymbolDataList);