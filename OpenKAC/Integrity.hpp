#pragma once
#include "runtime/helpers.hpp"
#include "Defs.h"
#include <bcrypt.h>
namespace OpenKAC {
    typedef struct _SysModuleSectionSignatures {
        char rdata[65];
        char PAGE[65];
        char txt[65];
    }SysModuleSectionSignatures,*PSysModuleSectionSignatures;
    class Integrity {
       inline static BCRYPT_ALG_HANDLE  algoHandle = 0; 
       inline static BCRYPT_HASH_HANDLE hashHandle = 0;
       inline static DWORD cbHash = 0;
       inline static DWORD cbHashObject = 0;
       inline static PBYTE pbHashObject = NULL;
    public:
        static void InitHashGen();
        static void UnInitHashGen();
        static int CreateSha256(UCHAR** hashOut, char* ptr, size_t size);
        static void CheckHashesOfDriver(PSysModule driver, PSysModuleSectionSignatures sigs);
    };
}