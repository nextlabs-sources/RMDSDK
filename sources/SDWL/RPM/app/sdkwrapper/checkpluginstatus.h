#pragma once
#include "plugingtypes.h"

#define NXSDK_API extern "C" __declspec(dllexport)

NXSDK_API bool IsWindows64Bit();

NXSDK_API PLUGIN_STATUS CheckPluginStatus(HKEY hKeyAddins);

NXSDK_API PLUGIN_STATUS CheckPluginStatus2(HKEY hRootKey, const wchar_t* wszAppType, const wchar_t* wszPlatform);

NXSDK_API bool IsPluginWell(const wchar_t* wszAppType, const wchar_t* wszPlatform);