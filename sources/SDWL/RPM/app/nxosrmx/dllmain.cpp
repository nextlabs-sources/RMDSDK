#include "stdafx.h"
#include "global_data_model.h"
#include "osrmx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		DEVLOG(L"OS_RMX DLL_PROCESS_ATTACH\n");
		global.this_dll_module = hModule;
		::DisableThreadLibraryCalls(hModule);
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH:
		nx::Uninitialize();
		DEVLOG(L"OS_RMX DLL_PROCESS_DETACH\n");
        break;
    }
    return TRUE;
}



// nxrmcore will load this lib and call this func
extern "C" __declspec(dllexport) 
bool Initialize() {
	if (nx::Initialize()) {
		DEVLOG(L"Os_rmx Initailizing OK\n");
		return true;
	}
	else {
		DEVLOG(L"Os_rmx Initailizing Failed\n");
		// Notice 
		// any functions in this dll can not call freelibrary to free it self
		// since once done, code_segment disappear
		//::FreeLibrary(nx::global.this_dll_module);
		return false;
	}
}
