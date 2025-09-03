#pragma once
#include "std_includes.h"
extern "C++" {
	namespace klib {
		//typedef void (*_str_init_funcs)(void*, void*);
		template<typename STR, typename SSTR, void(__stdcall* INITFN)(STR*, SSTR*)>
		class kString {
		public:
			kString();
			kString(SSTR*);
			~kString();
			SSTR* c_str();
			STR* str();
		private:
			SSTR* sourceStr;
			STR outStr;
		};

		using akString = kString<ANSI_STRING, const char, RtlInitAnsiString>;
		using ukString = kString<UNICODE_STRING, const wchar_t, RtlInitUnicodeString>;



	};
}

template<typename STR, typename SSTR, void(__stdcall* INITFN)(STR*, SSTR*)>
klib::kString<STR, SSTR, INITFN>::kString()
{
	this->sourceStr = "";
	INITFN(&this->outStr, this->sourceStr);
}

template<typename STR, typename SSTR, void(__stdcall* INITFN)(STR*, SSTR*)>
klib::kString<STR, SSTR, INITFN>::kString(SSTR* str)
{

	if (str == nullptr) {
		//TODO:Error
	}
	sourceStr = str;
	INITFN(&this->outStr, str);
}

template<typename STR, typename SSTR, void(__stdcall* INITFN)(STR*, SSTR*)>
inline klib::kString<STR, SSTR, INITFN>::~kString()
{
}


template<typename STR, typename SSTR, void(__stdcall* INITFN)(STR*, SSTR*)>
SSTR* klib::kString<STR, SSTR, INITFN>::c_str()
{
	return this->sourceStr;
}

template<typename STR, typename SSTR, void(__stdcall* INITFN)(STR*, SSTR*)>
STR* klib::kString<STR, SSTR, INITFN>::str()
{
	return &this->outStr;
}
