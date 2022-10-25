/*
    Nextlabs OS Data Loss Prevention(nxosdlp)

-hook:
    # filter out drag-drop feature when source file is nxl format
    # control ole-feature when source file is nxl format, such as
        - ole insert 
        - ole create link
        - ole create stream
09/16/2020
- file virtualization
    # to hook FILE APIs that related with FilePath -> to redirect it to a predefined safe place
*/
#include "pch.h"
#include <madCHook.h>
#include "util.hpp"
#include "Global.h"


HMODULE this_dll_module = NULL;

Global global;

bool Initialize() {
    // init global
    global.this_dll_module = this_dll_module;
    global.is_inited_madchook = false;
    // Hook API   

    InitializeMadCHook();
    global.is_inited_madchook = true;

    // check if allow 
    Registry::param rp(LR"_(SOFTWARE\NextLabs\SkyDRM\OsDLP)_");
    Registry r;
    std::uint32_t bDebug = -1;
    r.get(rp, L"allow_debugpoint", bDebug);
    global.is_allow_debug_breakpoint = (bDebug == 1);

    // by default, setup anit ole link and embed;
    /*if (is_outlook_process()) {
        DEVLOG(L"igore outlook process");
        return true;
    }*/

    if (is_solidwords_related_process()) {
        DEVLOG(L"igore solidword related processes");
        return true;
    }

    if (is_browser_related_process()) {
      DEVLOG(L"igore browser related processes");
      return true;
    }

    setup_anti_ole_link_embed();
    return true;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DEVLOG(L"OS_DLP DLL_PROCESS_ATTACH\n");
        ::DisableThreadLibraryCalls(hModule);
        this_dll_module = hModule;
        Initialize();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        DEVLOG(L"OS_DLP DLL_PROCESS_DETACH\n");
        break;
    }
    return TRUE;
}

