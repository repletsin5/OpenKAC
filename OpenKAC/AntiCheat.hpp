#pragma once
#define WDF_DEVICE_NO_WDMSEC_H
extern "C" {
#include <ntifs.h>
#include <ntdef.h>
#include <wdf.h>
#include <ntimage.h>
}
//#include "Driver.hpp"
//#include "Trace.h"
//#include "driver.tmh"

namespace OpenKAC {
	class AntiCheat {

	public:
		AntiCheat();
		~AntiCheat();
		inline static OpenKAC::AntiCheat* ac = nullptr; //this could be an issue giving other places control over this ptr.
		inline static WDFDRIVER driver = nullptr;
		PEPROCESS GetProcess();
		void SetProcess(PEPROCESS);
		void CreateThread();
		void StripHandles();
		static OB_PREOP_CALLBACK_STATUS ObPreOperation(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION PreInfo);
		static void ObPostOperation(PVOID RegistrationContext, POB_POST_OPERATION_INFORMATION PostInfo);
	private:
		PETHREAD acThread = {};
		HANDLE acThreadHandle = {};

		PEPROCESS proc = 0;



	};
	

}