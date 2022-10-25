#pragma once
#include "stdafx.h"
#include"checkpluginstatus.h"


bool IsWindows64Bit()
{
#if _WIN64
	return true;
#elif _WIN32

	BOOL isWow64 = FALSE;
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)
		GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &isWow64))
			return false;

		if (isWow64)
			return true;
		else
			return false;
	}
	else
		return false;

#endif 
}

PLUGIN_STATUS CheckPluginStatus(HKEY hKeyAddins)
{
	PLUGIN_STATUS dwStatus = PLUGIN_STATUS_NOT_EXIST;
	const wchar_t* wszPluginKey = L"nxlrmaddin";

	HKEY hPluginKey = NULL;
	LSTATUS lStatus = RegOpenKeyExW(hKeyAddins, wszPluginKey, 0, KEY_QUERY_VALUE, &hPluginKey);
	if (lStatus == ERROR_SUCCESS)
	{
		DWORD dwLoadBehavior = 0;
		DWORD dwSizeValue = sizeof(dwLoadBehavior);
		DWORD dwType = REG_DWORD;
		lStatus = RegQueryValueExW(hPluginKey, L"LoadBehavior", NULL, &dwType, (LPBYTE)&dwLoadBehavior, &dwSizeValue);
		if (lStatus == ERROR_SUCCESS && dwLoadBehavior == 3)
		{
			dwStatus = PLUGIN_STATUS_EXIST_AND_LOAD;
		}
		else
		{
			dwStatus = PLUGIN_STATUS_EXIST_NOT_LOAD;
		}
	}
	else// if (lStatus == ERROR_FILE_NOT_FOUND)
	{
		dwStatus = PLUGIN_STATUS_NOT_EXIST;
	}

	//free
	if (hPluginKey != NULL)
	{
		RegCloseKey(hPluginKey);
		hPluginKey = NULL;
	}

	return dwStatus;
}

PLUGIN_STATUS CheckPluginStatus2(HKEY hRootKey, const wchar_t* wszAppType, const wchar_t* wszPlatform)
{
	PLUGIN_STATUS dwStatus = PLUGIN_STATUS_NOT_EXIST;
	HKEY hKeyAddins = NULL;
	if (IsWindows64Bit())
	{
		if (_wcsicmp(wszPlatform, L"x86") == 0) {
			std::wstring wstrAddinsPath = L"SOFTWARE\\WOW6432Node\\Microsoft\\Office\\";
			wstrAddinsPath += wszAppType;
			wstrAddinsPath += L"\\Addins";

			LSTATUS lStatus = RegOpenKeyExW(hRootKey, wstrAddinsPath.c_str(), 0, KEY_QUERY_VALUE, &hKeyAddins);
			if (lStatus == ERROR_SUCCESS) {
				dwStatus = CheckPluginStatus(hKeyAddins);
			}
			else {
				dwStatus = PLUGIN_STATUS_NOT_EXIST;
			}
		}
		else if (_wcsicmp(wszPlatform, L"x64") == 0) {
			std::wstring wstrAddinsPath = L"SOFTWARE\\Microsoft\\Office\\";
			wstrAddinsPath += wszAppType;
			wstrAddinsPath += L"\\Addins";

			LSTATUS lStatus = RegOpenKeyExW(hRootKey, wstrAddinsPath.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKeyAddins);
			if (lStatus == ERROR_SUCCESS) {

				//disable key redirect
				long lDisable = RegDisableReflectionKey(hKeyAddins);

				//check status
				dwStatus = CheckPluginStatus(hKeyAddins);
			}
			else {
				dwStatus = PLUGIN_STATUS_NOT_EXIST;
			}
		}
	}
	else {
		std::wstring wstrAddinsPath = L"SOFTWARE\\Microsoft\\Office\\";
		wstrAddinsPath += wszAppType;
		wstrAddinsPath += L"\\Addins";

		LSTATUS lStatus = RegOpenKeyExW(hRootKey, wstrAddinsPath.c_str(), 0, KEY_QUERY_VALUE, &hKeyAddins);
		if (lStatus == ERROR_SUCCESS) {
			//check status
			dwStatus = CheckPluginStatus(hKeyAddins);
		}
		else {
			dwStatus = PLUGIN_STATUS_NOT_EXIST;
		}
	}

	//free
	if (hKeyAddins != NULL)
	{
		RegCloseKey(hKeyAddins);
		hKeyAddins = NULL;
	}

	return dwStatus;
}

/*
*Input:
*wszAppType: Word, Excel, PowerPoint.
*wszPlatform: x86,x64
*/
bool IsPluginWell(const wchar_t* wszAppType, const wchar_t* wszPlatform)
{
	bool bWell = true;
	DWORD dwPluginStatusOnMachine = CheckPluginStatus2(HKEY_LOCAL_MACHINE, wszAppType, wszPlatform);
	if (dwPluginStatusOnMachine != PLUGIN_STATUS_EXIST_AND_LOAD)
	{
		bWell = false;
	}
	else
	{
		//the plugin may be disable on HKEY_CURRENT_USER, we need to check it.
		DWORD dwPluginStatusOnCurUser = CheckPluginStatus2(HKEY_CURRENT_USER, wszAppType, wszPlatform);
		if (dwPluginStatusOnCurUser == PLUGIN_STATUS_EXIST_NOT_LOAD)
		{
			bWell = false;
		}
		else
		{
			//if the plugin information is not exist in HKEY_CURRENT_USER, or exit and the load behavior is 
			//correct, the whole status is well.
		}
	}
	return bWell;
}