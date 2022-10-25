// nxrminstca.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <msi.h>
#include <msiquery.h>
#include <stdio.h>
#include <Winreg.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <SetupAPI.h>
//#pragma comment(lib, "cmcfg32.lib")
#pragma comment(lib, "advapi32.lib")
#include <accctrl.h>
#include <aclapi.h>
#include <string>
#include <tchar.h>
#include <shellapi.h>
#include <Shlobj.h>
#include <experimental/filesystem>

#include "../../../../SDWRmcLib/Winutil/keym.h"
#include "../../../../SDWRmcLib/SDLAPI_forInstaller.h"


#define PRODUCT_NAME L"NextLabs Rights Management"
#define FILENAME_REGISTER L"register.xml"
#define FILENAME_AUDIT_LOG L"audit.db"

#define MAX_DRIVERFILES 2
const wchar_t *wstrSourceDriverFiles[MAX_DRIVERFILES] = {L"drv2\\nxrmdrv.sys", L"drv1\\nxrmflt.sys"};
const wchar_t *wstrDistDriverFiles[MAX_DRIVERFILES] = {L"nxrmdrv.sys", L"nxrmflt.sys"};

#define MAX_UNSTOPPABLE_DRIVERS 2
const wchar_t * const wstrUnstoppableDriverSrcDirs[MAX_UNSTOPPABLE_DRIVERS] = {L"drv2", L"drv3"};
const wchar_t * const wstrUnstoppableDriverNames[MAX_UNSTOPPABLE_DRIVERS] = {L"nxrmdrv", L"nxrmvhd"};

#define MAX_STOPPABLE_DRIVERS 1
const wchar_t * const wstrStoppableDriverNames[MAX_STOPPABLE_DRIVERS] = {L"nxrmflt"};



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
//fix Bug 64520 - [SanctuaryDir]Files can be share via right-click menu "Share" 
//fix Bug 65116 - Right-click menu button "Share" still hiddened after uninstall SkyDRM build 
UINT __stdcall DisableRrightClickMenuShare() {
	OutputDebugStringW(L"Enter DisableRrightClickMenuShare");
	UINT res = ERROR_SUCCESS;
	HKEY hKey = NULL;
	std::wstring oriValue = L"{e2bf9676-5f8f-435c-97eb-11607a5bedf7}";
	std::wstring expectValue = L"--{e2bf9676-5f8f-435c-97eb-11607a5bedf7}";
	WCHAR wstrTemp[MAX_PATH] = { 0 };
	DWORD dwBufsize = 0;

	do {

		res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\Classes\\*\\shellex\\ContextMenuHandlers\\ModernSharing"),
			0,
			KEY_WRITE | KEY_READ,
			&hKey);

		if (ERROR_SUCCESS != res) {
			OutputDebugStringW(L"Error!!! RegOpenKeyEx");
			break;
		}

		dwBufsize = sizeof(wstrTemp) * sizeof(WCHAR);
		res = RegQueryValueEx(hKey,
			TEXT(""),
			NULL,
			NULL,
			(LPBYTE)wstrTemp,
			&dwBufsize);
		if (ERROR_SUCCESS != res) {
			OutputDebugStringW(L"Error!!! RegQueryValueEx");
			break;
		}

		if (ERROR_SUCCESS != _wcsicmp(wstrTemp, oriValue.c_str())) {
			res = ERROR_SUCCESS;
			OutputDebugStringW(L"Unexpect value not {e2bf9676-5f8f-435c-97eb-11607a5bedf7}");
			break;
		}

		DWORD dwData = (DWORD)expectValue.length() * sizeof(WCHAR);
		res = RegSetValueEx(hKey,
			NULL,
			0,
			REG_SZ,
			(const BYTE*)expectValue.c_str(),
			dwData);
		if (ERROR_SUCCESS != res) {
			OutputDebugStringW(L"Error!!! RegSetValueEx");
			break;
		}

	} while (FALSE);

	if (hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}
	OutputDebugStringW(L"Leave DisableRrightClickMenuShare");
	return res;
}

//fix Bug 64520 - [SanctuaryDir]Files can be share via right-click menu "Share" 
//fix Bug 65116 - Right-click menu button "Share" still hiddened after uninstall SkyDRM build 
UINT __stdcall EnabeleRrightClickMenuShare() {
	OutputDebugStringW(L"Enter EnabeleRrightClickMenuShare");
	UINT res = ERROR_SUCCESS;
	HKEY hKey = NULL;
	std::wstring oriValue = L"--{e2bf9676-5f8f-435c-97eb-11607a5bedf7}";
	std::wstring expectValue = L"{e2bf9676-5f8f-435c-97eb-11607a5bedf7}";
	WCHAR wstrTemp[MAX_PATH] = { 0 };
	DWORD dwBufsize = 0;

	do {

		res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\Classes\\*\\shellex\\ContextMenuHandlers\\ModernSharing"),
			0,
			KEY_WRITE | KEY_READ,
			&hKey);

		if (ERROR_SUCCESS != res) {
			OutputDebugStringW(L"Error!!! RegOpenKeyEx");
			break;
		}

		dwBufsize = sizeof(wstrTemp) * sizeof(WCHAR);
		res = RegQueryValueEx(hKey,
			TEXT(""),
			NULL,
			NULL,
			(LPBYTE)wstrTemp,
			&dwBufsize);
		if (ERROR_SUCCESS != res) {
			OutputDebugStringW(L"Error!!! RegQueryValueEx");
			break;
		}

		if (ERROR_SUCCESS != _wcsicmp(wstrTemp, oriValue.c_str())) {
			res = ERROR_SUCCESS;
			OutputDebugStringW(L"Unexpect value not --{e2bf9676-5f8f-435c-97eb-11607a5bedf7}");
			break;
		}

		DWORD dwData = (DWORD)expectValue.length() * sizeof(WCHAR);
		res = RegSetValueEx(hKey,
			NULL,
			0,
			REG_SZ,
			(const BYTE*)expectValue.c_str(),
			dwData);
		if (ERROR_SUCCESS != res) {
			OutputDebugStringW(L"Error!!! RegSetValueEx");
			break;
		}

	} while (FALSE);

	if (hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}
	OutputDebugStringW(L"Leave EnabeleRrightClickMenuShare");
	return res;
}

DWORD AddAceToObjectsSecurityDescriptor(
	LPTSTR pszObjName,          // name of object
	SE_OBJECT_TYPE ObjectType,  // type of object
	LPTSTR pszTrustee,          // trustee for new ACE
	TRUSTEE_FORM TrusteeForm,   // format of trustee structure
	DWORD dwAccessRights,       // access mask for new ACE
	ACCESS_MODE AccessMode,     // type of ACE
	DWORD dwInheritance         // inheritance flags for new ACE
)
{
	OutputDebugStringW(L"Enter AddAceToObjectsSecurityDescriptor");
	DWORD dwRes = 0;
	PACL pOldDACL = NULL, pNewDACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea;

	if (NULL == pszObjName)
		return ERROR_INVALID_PARAMETER;

	// Get a pointer to the existing DACL.

	dwRes = GetNamedSecurityInfo(pszObjName, ObjectType,
		DACL_SECURITY_INFORMATION,
		NULL, NULL, &pOldDACL, NULL, &pSD);
	if (ERROR_SUCCESS != dwRes) {
		OutputDebugStringW(L"GetNamedSecurityInfo Error");
		goto Cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for the new ACE. 

	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = dwAccessRights;
	ea.grfAccessMode = AccessMode;
	ea.grfInheritance = dwInheritance;
	ea.Trustee.TrusteeForm = TrusteeForm;
	ea.Trustee.ptstrName = pszTrustee;

	// Create a new ACL that merges the new ACE
	// into the existing DACL.

	dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
	if (ERROR_SUCCESS != dwRes) {
		OutputDebugStringW(L"SetEntriesInAcl Error");
		goto Cleanup;
	}

	// Attach the new ACL as the object's DACL.

	dwRes = SetNamedSecurityInfo(pszObjName, ObjectType,
		DACL_SECURITY_INFORMATION,
		NULL, NULL, pNewDACL, NULL);
	if (ERROR_SUCCESS != dwRes) {
		OutputDebugStringW(L"SetNamedSecurityInfo Error");
		goto Cleanup;
	}

Cleanup:

	if (pSD != NULL)
		LocalFree((HLOCAL)pSD);
	if (pNewDACL != NULL)
		LocalFree((HLOCAL)pNewDACL);

	OutputDebugStringW(L"Leave AddAceToObjectsSecurityDescriptor");
	return dwRes;
}


BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
)
{
	OutputDebugStringW(L"Enter SetPrivilege");
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		OutputDebugStringW(L"LookupPrivilegeValue error");
		OutputDebugStringW(L"Leave SetPrivilege");
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		OutputDebugStringW(L"AdjustTokenPrivileges error");
		OutputDebugStringW(L"Leave SetPrivilege");
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		OutputDebugStringW(L"The token does not have the specified privilege.");
		OutputDebugStringW(L"Leave SetPrivilege");
		return FALSE;
	}
	OutputDebugStringW(L"Leave SetPrivilege");
	return TRUE;
}

BOOL TakeOwnership(LPTSTR lpszOwnFile, SE_OBJECT_TYPE type)
{
	OutputDebugStringW(L"Enter TakeOwnership");
	BOOL bRetval = FALSE;
	HANDLE hToken = NULL;
	PSID pSIDAdmin = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	DWORD dwRes;

	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pSIDAdmin))
	{
		OutputDebugStringW(L"AllocateAndInitializeSid (Admin) error");
		goto Cleanup;
	}

	// enable the SE_TAKE_OWNERSHIP_NAME privilege, create a SID for 
	// the Administrators group, take ownership of the object, and 
	// disable the privilege. Then try again to set the object's DACL.

	// Open a handle to the access token for the calling process.
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES,
		&hToken))
	{
		OutputDebugStringW(L"OpenProcessToken failed");
		goto Cleanup;
	}

	// Enable the SE_TAKE_OWNERSHIP_NAME privilege.
	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE))
	{
		OutputDebugStringW(L"You must be logged on as Administrator");
		goto Cleanup;
	}

	// Set the owner in the object's security descriptor.
	dwRes = SetNamedSecurityInfo(
		lpszOwnFile,                 // name of the object
		type,						 // type of object
		OWNER_SECURITY_INFORMATION,  // change only the object's owner
		pSIDAdmin,                   // SID of Administrator group
		NULL,
		NULL,
		NULL);

	if (dwRes != ERROR_SUCCESS)
	{
		OutputDebugStringW(L"Could not set owner. Error");
		goto Cleanup;
	}

	// Disable the SE_TAKE_OWNERSHIP_NAME privilege.
	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, FALSE))
	{
		OutputDebugStringW(L"Failed SetPrivilege call unexpectedly.");
		goto Cleanup;
	}

Cleanup:

	if (pSIDAdmin)
		FreeSid(pSIDAdmin);

	if (hToken)
		CloseHandle(hToken);

	OutputDebugStringW(L"Leave TakeOwnership");
	return bRetval;
}


// fix Bug 64684 - [SanctuaryDir]Data leak: file can be send via "Share", "Email" button on Explorer ribbon bar 
UINT __stdcall DisableShareTable_64_32() {
	OutputDebugStringW(L"Enter DisableShareTable_64_32");
	UINT res = ERROR_SUCCESS;
	HKEY hKey = NULL;
	HKEY runKey = NULL;
	int size = 2;
	REGSAM samDesired = KEY_ALL_ACCESS | KEY_WOW64_64KEY;

	for (int index = 0; index < size; index++) {

		do {
			res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				TEXT("SOFTWARE\\Classes\\CLSID"),
				0,
				samDesired,
				&hKey);

			if (ERROR_SUCCESS != res) {
				OutputDebugStringW(L"Error!!! RegOpenKeyEx: SOFTWARE\\Classes\\CLSID");
				break;
			}

			res = RegRenameKey(hKey, TEXT("{e2bf9676-5f8f-435c-97eb-11607a5bedf7}"), TEXT("--{e2bf9676-5f8f-435c-97eb-11607a5bedf7}"));
			if (ERROR_SUCCESS != res) {
				OutputDebugStringW(L"Error!!! RegRenameKey");
				break;
			}

		} while (FALSE);

		if (hKey)
		{
			RegCloseKey(hKey);
			hKey = NULL;
		}

		if (runKey)
		{
			RegCloseKey(runKey);
			runKey = NULL;
		}

		samDesired = KEY_ALL_ACCESS | KEY_WOW64_32KEY;
	}

	OutputDebugStringW(L"Leave DisableShareTable_64_32");
	return res;
}

// fix Bug 64684 - [SanctuaryDir]Data leak: file can be send via "Share", "Email" button on Explorer ribbon bar 
UINT __stdcall EnableShareTable_64_32() {
	OutputDebugStringW(L"Enter EnableShareTable_64_32");
	UINT res = ERROR_SUCCESS;
	HKEY hKey = NULL;
	int size = 2;
	REGSAM samDesired = KEY_ALL_ACCESS | KEY_WOW64_64KEY;

	for (int index = 0; index < size; index++) {

		do {
			res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				TEXT("SOFTWARE\\Classes\\CLSID"),
				0,
				samDesired,
				&hKey);

			if (ERROR_SUCCESS != res) {
				OutputDebugStringW(L"Error!!! RegOpenKeyEx");
				break;
			}

			res = RegRenameKey(hKey, TEXT("--{e2bf9676-5f8f-435c-97eb-11607a5bedf7}"), TEXT("{e2bf9676-5f8f-435c-97eb-11607a5bedf7}"));
			if (ERROR_SUCCESS != res) {
				OutputDebugStringW(L"Error!!! RegRenameKey");
				break;
			}
		} while (FALSE);

		if (hKey)
		{
			RegCloseKey(hKey);
			hKey = NULL;
		}

		samDesired = KEY_ALL_ACCESS | KEY_WOW64_32KEY;
	}
	OutputDebugStringW(L"Leave EnableShareTable_64_32");
	return res;
}


// fix Bug 64684 - [SanctuaryDir]Data leak: file can be send via "Share", "Email" button on Explorer ribbon bar 
UINT __stdcall DisableShareTable() {
	OutputDebugStringW(L"Enter DisableShareTable");
	UINT res = ERROR_SUCCESS;
	BOOL bRetval = FALSE;
	DWORD dwRes = 0;
	PSID pSIDAdmin = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	std::wstring regKeyPath = L"MACHINE\\SOFTWARE\\Classes\\CLSID\\{e2bf9676-5f8f-435c-97eb-11607a5bedf7}";
	std::wstring subRegKeyPath = L"MACHINE\\SOFTWARE\\Classes\\CLSID\\{e2bf9676-5f8f-435c-97eb-11607a5bedf7}\\InProcServer32";
	bRetval=TakeOwnership(&regKeyPath[0], SE_REGISTRY_WOW64_64KEY);
	bRetval=TakeOwnership(&subRegKeyPath[0], SE_REGISTRY_WOW64_64KEY);
	bRetval=TakeOwnership(&regKeyPath[0], SE_REGISTRY_WOW64_32KEY);
	bRetval=TakeOwnership(&subRegKeyPath[0], SE_REGISTRY_WOW64_32KEY);

	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pSIDAdmin))
	{
		OutputDebugStringW(L"AllocateAndInitializeSid (Admin) error");
	}
	dwRes =AddAceToObjectsSecurityDescriptor(&regKeyPath[0], SE_REGISTRY_WOW64_64KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	dwRes =AddAceToObjectsSecurityDescriptor(&subRegKeyPath[0], SE_REGISTRY_WOW64_64KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	dwRes =AddAceToObjectsSecurityDescriptor(&regKeyPath[0], SE_REGISTRY_WOW64_32KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	dwRes =AddAceToObjectsSecurityDescriptor(&subRegKeyPath[0], SE_REGISTRY_WOW64_32KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	res =DisableShareTable_64_32();
	OutputDebugStringW(L"Leave DisableShareTable");
	return res;
}

// fix Bug 64684 - [SanctuaryDir]Data leak: file can be send via "Share", "Email" button on Explorer ribbon bar 
UINT __stdcall EnableShareTable() {
	OutputDebugStringW(L"Enter EnableShareTable");
	UINT res = ERROR_SUCCESS;
	BOOL bRetval = FALSE;
	DWORD dwRes = 0;
	PSID pSIDAdmin = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	std::wstring regKeyPath = L"MACHINE\\SOFTWARE\\Classes\\CLSID\\{e2bf9676-5f8f-435c-97eb-11607a5bedf7}";
	std::wstring subRegKeyPath = L"MACHINE\\SOFTWARE\\Classes\\CLSID\\{e2bf9676-5f8f-435c-97eb-11607a5bedf7}\\InProcServer32";
	bRetval=TakeOwnership(&regKeyPath[0], SE_REGISTRY_WOW64_64KEY);
	bRetval=TakeOwnership(&subRegKeyPath[0], SE_REGISTRY_WOW64_64KEY);
	bRetval=TakeOwnership(&regKeyPath[0], SE_REGISTRY_WOW64_32KEY);
	bRetval=TakeOwnership(&subRegKeyPath[0], SE_REGISTRY_WOW64_32KEY);

	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pSIDAdmin))
	{
		OutputDebugStringW(L"AllocateAndInitializeSid (Admin) error");
	}
	dwRes=AddAceToObjectsSecurityDescriptor(&regKeyPath[0], SE_REGISTRY_WOW64_64KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	dwRes=AddAceToObjectsSecurityDescriptor(&subRegKeyPath[0], SE_REGISTRY_WOW64_64KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	dwRes=AddAceToObjectsSecurityDescriptor(&regKeyPath[0], SE_REGISTRY_WOW64_32KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	dwRes=AddAceToObjectsSecurityDescriptor(&subRegKeyPath[0], SE_REGISTRY_WOW64_32KEY, (LPTSTR)pSIDAdmin, TRUSTEE_IS_SID, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
	res =EnableShareTable_64_32();
	OutputDebugStringW(L"Leave EnableShareTable");
	return res;
}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR


//Note:  MessageBox can not use in deferred execution since not be able to get UILevel property
UINT _stdcall MessageAndLogging(MSIHANDLE hInstall, BOOL bLogOnly, const WCHAR* wstrMsg )
{
	if(bLogOnly == FALSE && hInstall!= NULL)
	{
		INT nUILevel =0;
		WCHAR wstrTemp[2] = {0};
		DWORD dwBufsize = 0;
		
		dwBufsize = sizeof(dwBufsize)/sizeof(WCHAR);	
		if(ERROR_SUCCESS == MsiGetProperty(hInstall, TEXT("UILevel"), wstrTemp, &dwBufsize))
		{
			nUILevel = _wtoi(wstrTemp);
		}

		if(nUILevel > 2)
		{
			MessageBox(GetForegroundWindow(),(LPCWSTR) wstrMsg, (LPCWSTR)PRODUCT_NAME, MB_OK|MB_ICONWARNING);	
		}
	}

	//add log here
	PMSIHANDLE hRecord = MsiCreateRecord(1);
	if(hRecord !=NULL)
	{
		MsiRecordSetString(hRecord, 0, wstrMsg);
		// send message to running installer
		MsiProcessMessage(hInstall, INSTALLMESSAGE_INFO, hRecord);
		MsiCloseHandle(hRecord);
	}

	
	return ERROR_SUCCESS;
}//return service current status, or return 0 for service not existed

DWORD GetServiceStatus(const wchar_t *wstrServiceName)
{
	SC_HANDLE hSCManager,hService;
	WCHAR wstrTemp[128] = {0};
	DWORD dwErrorCode = 0;

	if ( wstrServiceName==NULL || wstrServiceName[0]==L'\0')	
		return 0;
	
	hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (hSCManager==NULL)
	{
		dwErrorCode = GetLastError();
		swprintf_s(wstrTemp,128, L"Open SC Menager Failed. Error Code: %d", dwErrorCode);
		//MessageBox(GetForegroundWindow(), (LPCWSTR)wstrTemp,(LPCWSTR)PRODUCT_NAME , MB_OK|MB_ICONWARNING);
		return 0;
	}

	hService = OpenService(hSCManager, wstrServiceName, GENERIC_READ);	
	if (hService== NULL)
	{		
		CloseServiceHandle(hSCManager);
		return 0;
	}

	SERVICE_STATUS_PROCESS ServiceStatus;
	ZeroMemory(&ServiceStatus, sizeof(ServiceStatus));
	DWORD dwBytesNeeded = 0;

	if (!QueryServiceStatusEx(hService,
							SC_STATUS_PROCESS_INFO,
							(LPBYTE)&ServiceStatus,
							sizeof(ServiceStatus),
							&dwBytesNeeded))
	{
		dwErrorCode = GetLastError();
		swprintf_s(wstrTemp,128,L"SC query Service status failed. Error Code: %d", dwErrorCode);
		//MessageBox(GetForegroundWindow(), (LPCWSTR)wstrTemp, (LPCWSTR)PRODUCT_NAME, MB_OK|MB_ICONWARNING);
		CloseServiceHandle(hSCManager);
		CloseServiceHandle(hService);
		return 0;
	}

	DWORD dwStatus = 0;
	dwStatus = ServiceStatus.dwCurrentState;
	
	CloseServiceHandle(hSCManager);
	CloseServiceHandle(hService);

	return dwStatus;
}


BOOL SHCopy(LPCWSTR from, LPCWSTR to, BOOL bDeleteFrom)
{
	SHFILEOPSTRUCT fileOp = {0};
	WCHAR newFrom[MAX_PATH + 1];
	WCHAR newTo[MAX_PATH + 1];

	if(bDeleteFrom)
		fileOp.wFunc = FO_MOVE;
	else
		fileOp.wFunc = FO_COPY;

	fileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

	wcscpy_s(newFrom, MAX_PATH, from);
	newFrom[wcslen(from) + 1] = NULL;
	fileOp.pFrom = newFrom;

	wcscpy_s(newTo, MAX_PATH, to);
	newTo[wcslen(to) + 1] = NULL;
	fileOp.pTo = newTo;

	int result = SHFileOperation(&fileOp);

	return result == 0;
}

//*****************************************************************************************************
//				START MSI CUSTOM ACTION FUNCTION HERE
//*****************************************************************************************************

UINT __stdcall MyTest(MSIHANDLE hInstall )
{
	//MessageBox(GetForegroundWindow(),(LPCWSTR) L"Hello world, I am here # 1", (LPCWSTR)PRODUCT_NAME, MB_OK);	
	MessageAndLogging(hInstall, FALSE, L"Hello world, I am here # 1 " );
	
	return ERROR_SUCCESS;
}

//CACleanUp, call in deferred execution in system context
UINT __stdcall UninstallCleanUp(MSIHANDLE hInstall )
{
	HKEY hKey = NULL;
		
	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									TEXT("SYSTEM\\CurrentControlSet\\services\\"),
									0, 
									KEY_ALL_ACCESS, 
									&hKey))
	{			
		SHDeleteKey(hKey,TEXT("nxrmdrv"));	
		SHDeleteKey(hKey,TEXT("nxrmserv"));	
		SHDeleteKey(hKey,TEXT("nxrmflt"));	
		RegCloseKey(hKey);
	}

	LSTATUS status;

	// Delete HKLM\SOFTWARE\NextLabs\RPM and all its subkeys.  Ignore error.
	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
									TEXT("SOFTWARE\\NextLabs"),
									0,
									KEY_ALL_ACCESS | KEY_WOW64_64KEY,
									&hKey))
	{
		status = SHDeleteKey(hKey, TEXT("RPM"));
		if (status != ERROR_SUCCESS)
		{
			WCHAR wstrMsg[1024];
			swprintf_s(wstrMsg, L"******** NXRMLOG: UninstallCleanUp: SHDeleteKey(HKLM\\SOFTWARE\\NextLabs\\RPM) failed. ERROR CODE: %lu", status);
			MessageAndLogging(hInstall, TRUE, wstrMsg);
		}

		// Delete HKLM\SOFTWARE\NextLabs if it has no more values or subkeys.
		// Ignore error.
		DWORD numSubKeys, numValues;
		if((ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL,
											 &numSubKeys, NULL, NULL,
											 &numValues, NULL, NULL, NULL,
											 NULL)) &&
		   (numSubKeys == 0 && numValues == 0))
		{
			status = RegDeleteKeyEx(HKEY_LOCAL_MACHINE,
									TEXT("SOFTWARE\\NextLabs"),
									KEY_WOW64_64KEY, 0);
			if (status != ERROR_SUCCESS)
			{
				WCHAR wstrMsg[1024];
				swprintf_s(wstrMsg, L"******** NXRMLOG: UninstallCleanUp: RegDeleteKeyEx(HKLM\\SOFTWARE\\NextLabs) failed. ERROR CODE: %lu", status);
				MessageAndLogging(hInstall, TRUE, wstrMsg);
			}
		}

		RegCloseKey(hKey);
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Uninstall clean up done."));

   return ERROR_SUCCESS;
}

//CAResetDrvReg, call in deferred execution in system context
UINT __stdcall ResetNxrmdrvRegdata(MSIHANDLE hInstall )
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start update registry for drivers."));
	//remove pending delete flag for nxrmdrv duing major upgrade
	HKEY hKey = NULL;
	WCHAR wstrTemp[MAX_PATH] = {0};
	DWORD dwBufsize = 0;

	
	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									TEXT("SYSTEM\\CurrentControlSet\\services\\nxrmdrv\\"),
									0, 
									KEY_WRITE|KEY_READ, 
									&hKey))

	{	
		 dwBufsize = sizeof(wstrTemp)/sizeof(WCHAR);
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, 			
											TEXT("DeleteFlag"),
											NULL, 
											NULL, 
											(LPBYTE)wstrTemp, 
											& dwBufsize))
		{
			RegDeleteValue(hKey,TEXT("DeleteFlag"));
			MessageAndLogging(hInstall, TRUE, TEXT("Remove delete mode."));
		}

		DWORD dwStart = 1; //reset to 1 will auto start driver after reboot
		if (ERROR_SUCCESS == RegSetValueEx(hKey, 			
											TEXT("Start"),
											NULL, 
											REG_DWORD, 
											(const BYTE*)&dwStart,
											sizeof(dwStart)))
		{
			MessageAndLogging(hInstall, TRUE, TEXT("Reset to auto start."));
		}

		RegCloseKey(hKey);
	}
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG:  Update registry for drivers done."));
   return ERROR_SUCCESS;
}

//CACheckPendingReboot, call in immediate excution
UINT __stdcall CheckRebootCondition(MSIHANDLE hInstall)
{
	DWORD dwRetStatus = 0;
	UINT uiRet =0;

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start check reboot status."));

	dwRetStatus = GetServiceStatus(L"nxrmdrv"); //running
	if(dwRetStatus == SERVICE_RUNNING)
	{
		MessageAndLogging(hInstall, TRUE, TEXT("NXRMLOG: nxrmdrv is running."));
		if( GetServiceStatus(L"nxrmflt") != SERVICE_RUNNING ) //not running
		{
			MessageAndLogging(hInstall, TRUE, TEXT("NXRMLOG: nxrmflt is stopped. Needs to reboot this computer."));
			uiRet =  MsiSetProperty(hInstall, L"REBOOT", L"Force");
			if (uiRet != ERROR_SUCCESS )
			{
				MessageAndLogging(hInstall, FALSE, TEXT("Set Force REBOOT property failed."));
				return ERROR_INSTALL_FAILURE; 
			}
		}
		else
		{
			MessageAndLogging(hInstall, TRUE, TEXT("NXRMLOG: nxrmflt is running. No reboot needed."));
		}
			
	}
	
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Check reboot status done."));

	return ERROR_SUCCESS;
}

//CAFindFile, call in immediate excution
UINT __stdcall FindConfigFile(MSIHANDLE hInstall )
{
	//0. Skip checking for file if we are to use the built-in one
	WCHAR wstrPropVal[1024];
	DWORD dwPropVal = _countof(wstrPropVal);
	UINT uiRet = 0;
	uiRet = MsiGetProperty(hInstall, TEXT("NXRMUSEBUILTINCONFIGFILE"), wstrPropVal, &dwPropVal);
	if (uiRet == ERROR_SUCCESS && _wcsicmp(wstrPropVal, L"Yes") == 0)
	{
		MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Skip checking file Register.xml"));
		return ERROR_SUCCESS;
	}

	WCHAR wstrSourceDir[MAX_PATH] = {0};
	WCHAR wstrTemp[MAX_PATH] = {0};
	DWORD dwPathBuffer = 0;
	WCHAR wstrMsg[128] = {0};
	DWORD dwErrorCode = 0;
	BOOL bFindFileError =FALSE;

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start checking file Register.xml"));

	//1. Get source path from MSI
	dwPathBuffer = sizeof(wstrSourceDir)/sizeof(WCHAR);
	uiRet = MsiGetProperty(hInstall, TEXT("SourceDir"), wstrSourceDir, &dwPathBuffer );
	if( uiRet != ERROR_SUCCESS)
	{
		dwErrorCode = GetLastError();
		swprintf_s(wstrMsg, 128, L"Get Souce dirctory from Msi failed. Error Code: %d", dwErrorCode);
		MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg );
	
		return ERROR_INSTALL_FAILURE; 
	}

	//2. Try getting parent dir of OriginalDatabase if SourceDir is not defined.
	if (dwPathBuffer == 0)
	{
		dwPathBuffer = _countof(wstrSourceDir);
		uiRet = MsiGetProperty(hInstall, TEXT("OriginalDatabase"), wstrSourceDir, &dwPathBuffer);
		if(uiRet != ERROR_SUCCESS)
		{
			swprintf_s(wstrMsg, L"Get Original Database from Msi failed. Error Code: %u", uiRet);
			MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg );
			return ERROR_INSTALL_FAILURE;
		}

		//Find parent dir of OriginalDatabase
		WCHAR *p = wcsrchr(wstrSourceDir, L'\\');
		if (p == NULL)
		{
			swprintf_s(wstrMsg, L"Can't find parent directory of OriginaDatabase \"%s\".", wstrSourceDir);
			MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg );
			bFindFileError = TRUE;
		}
		else
		{
			*p = L'\0';
		}
	}

	if (!bFindFileError)
	{
		//Check if file exist
		if(wstrSourceDir[wcslen(wstrSourceDir)-1]!= L'\\')
		{
			wcscat_s(wstrSourceDir, MAX_PATH,  L"\\");
		}
		wcscat_s(wstrSourceDir, MAX_PATH, FILENAME_REGISTER );

		if (GetFileAttributes(wstrSourceDir)==INVALID_FILE_ATTRIBUTES && GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			MessageAndLogging(hInstall, TRUE, TEXT("File not found, try current directory...."));
			bFindFileError = TRUE;
		}
	}
	
	//3. Try CURRENTDIRECTORY property in reinstall mode
	if(bFindFileError)
	{
		ZeroMemory(wstrSourceDir, sizeof(wstrSourceDir));
		uiRet = 0;
		dwPathBuffer = sizeof(wstrSourceDir)/sizeof(WCHAR);

		uiRet = MsiGetProperty(hInstall, TEXT("CURRENTDIRECTORY"), wstrSourceDir, &dwPathBuffer );
		if( uiRet != ERROR_SUCCESS)
		{
			dwErrorCode = GetLastError();
			swprintf_s(wstrMsg, 128, L"Get Current dirctory from Msi failed. Error Code: %d", dwErrorCode);
			MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg );
	
			return ERROR_INSTALL_FAILURE; 
		}
	
		//Check if file exist
		if(wstrSourceDir[wcslen(wstrSourceDir)-1]!= L'\\')
		{
			wcscat_s(wstrSourceDir, MAX_PATH,  L"\\");
		}
		wcscat_s(wstrSourceDir, MAX_PATH, FILENAME_REGISTER );

		if (GetFileAttributes(wstrSourceDir)==INVALID_FILE_ATTRIBUTES && GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			// 4. Try installdir in REINSTALL mode
			WCHAR wstrReinstall[MAX_PATH] = {0};
			DWORD dwBuffer = sizeof(wstrReinstall)/sizeof(WCHAR);
			uiRet =0;
			uiRet = MsiGetProperty(hInstall, TEXT("REINSTALL"), wstrReinstall, &dwBuffer );
			
			if( uiRet != ERROR_SUCCESS || wcslen(wstrReinstall) == 0) //not in reinstall
			{
				MessageAndLogging(hInstall, TRUE, TEXT("The optional configuration file is not found."));
				return ERROR_SUCCESS;
			}
			
			ZeroMemory(wstrSourceDir, sizeof(wstrSourceDir));
			dwPathBuffer = sizeof(wstrSourceDir)/sizeof(WCHAR);
			uiRet =0;
			uiRet = MsiGetProperty(hInstall, TEXT("INSTALLDIR"), wstrSourceDir, &dwPathBuffer );
			if( uiRet != ERROR_SUCCESS)
			{
				dwErrorCode = GetLastError();
				swprintf_s(wstrMsg, 128, L"Get install dirctory from Msi failed. Error Code: %d", dwErrorCode);
				MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg );
				return ERROR_INSTALL_FAILURE; 
			}

			if(wstrSourceDir[wcslen(wstrSourceDir)-1]!= L'\\')
			{
				wcscat_s(wstrSourceDir, MAX_PATH,  L"\\");
			}
			wcscat_s(wstrSourceDir, MAX_PATH, L"SkyDRM\\RPM\\conf\\");
			wcscat_s(wstrSourceDir, MAX_PATH, FILENAME_REGISTER );
			

			if (GetFileAttributes(wstrSourceDir)==INVALID_FILE_ATTRIBUTES )
			{		
				MessageAndLogging(hInstall, TRUE, TEXT("The optional configuration file is not found."));
				return ERROR_SUCCESS;
			}
		}	

	}

	//get temp path
	DWORD dwRetVal = 0;
	dwRetVal = GetTempPath(MAX_PATH, wstrTemp);                                 
    if ((dwRetVal > MAX_PATH) || (dwRetVal == 0))
    {
		MessageAndLogging(hInstall, FALSE, TEXT("Failed to get temp path in this computer."));
        return ERROR_INSTALL_FAILURE;
    }
	
	// verify temp path exists
	HANDLE hTempFile = INVALID_HANDLE_VALUE;
	hTempFile = CreateFile(	wstrTemp,
							GENERIC_READ,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING|CREATE_NEW,
							FILE_FLAG_BACKUP_SEMANTICS,
							NULL);
		
	if ( hTempFile == INVALID_HANDLE_VALUE ) 
	{
		if (!CreateDirectory(wstrTemp, NULL))
		{
			dwErrorCode = GetLastError();
			if ( dwErrorCode != ERROR_ALREADY_EXISTS )
			{
				swprintf_s(wstrMsg, 128, L"Failed to create temp path. Error Code: %d", dwErrorCode);
				MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg);
				return ERROR_INSTALL_FAILURE;
			}
		}		
	}
	CloseHandle(hTempFile);
	
	//Move file from source to temp
	if(wstrTemp[wcslen(wstrTemp)-1] != L'\\')
	{
		wcscat_s(wstrTemp, MAX_PATH,  L"\\");
	}	
	wcscat_s(wstrTemp, MAX_PATH, FILENAME_REGISTER);

	SetFileAttributes(wstrTemp, FILE_ATTRIBUTE_NORMAL);
	
	if( CopyFile(wstrSourceDir, wstrTemp, FALSE)== FALSE) //Failed
	{
		dwErrorCode = GetLastError();
		swprintf_s(wstrMsg, 128, L"Failed to copy file to temp path. Error Code: %d", dwErrorCode);
		MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg);
		return ERROR_INSTALL_FAILURE; 
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Checking file Register.xml done.  Status: Good."));

    return ERROR_SUCCESS;
}

//CACopyFile, call in deferred execution in system context
UINT __stdcall CopyConfigFile(MSIHANDLE hInstall ) //run in deferred execution
{
	WCHAR wstrSourceDir[MAX_PATH] = {0};
	WCHAR wstrInstallDir[MAX_PATH] = {0};
	DWORD dwPathBuffer = 0;
	WCHAR wstrMsg[128] = {0};
	DWORD dwErrorCode = 0;

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start copy config file."));
	//get current Installdir from MSI
	dwPathBuffer = sizeof(wstrInstallDir)/sizeof(WCHAR);
	if(ERROR_SUCCESS !=  MsiGetProperty(hInstall, TEXT("CustomActionData"), wstrInstallDir, & dwPathBuffer))
	{
		dwErrorCode = GetLastError();
		swprintf_s(wstrMsg, 128, L"Failed to get install directory from MSI. Error Code: %d", dwErrorCode);
		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);//log only
	
		return ERROR_INSTALL_FAILURE;
	}


	if(wstrInstallDir[wcslen(wstrInstallDir)-1]!= L'\\')
	{
		wcscat_s(wstrInstallDir, L"\\");
	}	
	wcscat_s(wstrInstallDir, L"SkyDRM\\RPM\\conf\\");
	wcscat_s(wstrInstallDir, FILENAME_REGISTER);

	//get file from temp
	DWORD dwRetVal = 0;
	dwRetVal = GetTempPath(MAX_PATH, wstrSourceDir);                                 
    if ((dwRetVal > MAX_PATH) || (dwRetVal == 0))
    {
		MessageAndLogging(hInstall, TRUE, TEXT("Failed to get temp path in this computer."));
        return ERROR_INSTALL_FAILURE;
    }

	if(wstrSourceDir[wcslen(wstrSourceDir)-1]!= L'\\')
	{
		wcscat_s(wstrSourceDir, L"\\");
	}
	wcscat_s(wstrSourceDir, FILENAME_REGISTER);

	//If file does not exist in temp, it must be because we're using a built-in file.  So no need to copy.
	if (GetFileAttributes(wstrSourceDir)==INVALID_FILE_ATTRIBUTES && GetLastError()==ERROR_FILE_NOT_FOUND)
	{
		MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Skip copying file Register.xml"));
		return ERROR_SUCCESS;
	}

	//prevent read only file already existed
	SetFileAttributes(wstrInstallDir, FILE_ATTRIBUTE_NORMAL); 
	
	//Move file from Temp to Install Directory
	if(CopyFile(wstrSourceDir, wstrInstallDir, FALSE)== FALSE)
	{
		dwErrorCode = GetLastError();
		swprintf_s(wstrMsg, 128, L"Copy Register.xml file failed. Error Code: %d", dwErrorCode);

		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
		return ERROR_INSTALL_FAILURE; 
	}

	//Clean up file
	DeleteFile(wstrSourceDir);

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Copy config file success."));

    return ERROR_SUCCESS;

}

//CACopyDrivers, call in deferred execution in system context
UINT __stdcall CopyDriverFiles(MSIHANDLE hInstall ) //run in deferred execution
{	
	WCHAR wstrInstallDir[MAX_PATH] = {0};
	WCHAR wstrBinDir[MAX_PATH] = {0};
	WCHAR wstrWindowsDir[MAX_PATH] ={0};
	WCHAR wstrSystemDirDrivers[MAX_PATH] ={0};
	DWORD dwPathBuffer = 0;
	UINT uiRetCode = 0;

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start copy driver files."));

	//get current Installdir from MSI
	dwPathBuffer = sizeof(wstrInstallDir)/sizeof(WCHAR);
	uiRetCode =  MsiGetProperty(hInstall, TEXT("CustomActionData"), wstrInstallDir, &dwPathBuffer);
	if(ERROR_SUCCESS != uiRetCode)
	{
		WCHAR wstrMsg[128] = {0};		
		swprintf_s(wstrMsg, 128, L"NXRMLOG: Failed to get install directory from MSI. Error Code: %d", uiRetCode);
		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);//log only	
		return ERROR_INSTALL_FAILURE;
	}
	
	if(wstrInstallDir[wcslen(wstrInstallDir)-1]!= L'\\')
	{
		wcscat_s(wstrInstallDir, _countof(wstrInstallDir),  L"\\");
	}
	wcscpy_s(wstrBinDir, MAX_PATH, wstrInstallDir );
	wcscat_s(wstrBinDir, _countof(wstrBinDir),  L"SkyDRM\\RPM\\bin\\");

	if(!GetSystemWindowsDirectory(wstrWindowsDir, MAX_PATH))
	{
		MessageAndLogging(hInstall, TRUE, TEXT("NXRMLOG: Failed to get windows directory in this computer."));
        return ERROR_INSTALL_FAILURE;
	}

	if(wstrWindowsDir[wcslen(wstrWindowsDir)-1]!= L'\\')	
	{
		wcscat_s(wstrWindowsDir, _countof(wstrWindowsDir),  L"\\");
	}
	wcscpy_s(wstrSystemDirDrivers, MAX_PATH, wstrWindowsDir);
	wcscat_s(wstrSystemDirDrivers, _countof(wstrSystemDirDrivers),  L"System32\\drivers\\");
	

	//start copy *.sys files from install directory to system32\\drivers
	PVOID OldValue = NULL;
	if( Wow64DisableWow64FsRedirection(&OldValue) ) 
	{
		for(int i=0; i<MAX_DRIVERFILES; i++)
		{
			WCHAR wstrFile[MAX_PATH] ={0};
			WCHAR wstrDistFile[MAX_PATH] ={0};
			ZeroMemory(wstrFile, sizeof(wstrFile));
			wcscpy_s(wstrFile, MAX_PATH, wstrBinDir);
			wcscat_s(wstrFile, _countof(wstrFile), wstrSourceDriverFiles[i]);

			ZeroMemory(wstrDistFile, sizeof(wstrDistFile));
			wcscpy_s(wstrDistFile, MAX_PATH, wstrSystemDirDrivers);
			wcscat_s(wstrDistFile, _countof(wstrDistFile), wstrDistDriverFiles[i]);
		
			SetFileAttributes(wstrDistFile, FILE_ATTRIBUTE_NORMAL);

			if(CopyFile(wstrFile, wstrDistFile, FALSE)== FALSE) //Failed
			{
				DWORD lastErr = GetLastError();
				WCHAR wstrMsg[1024] = {0};
				swprintf_s(wstrMsg, 1024, L"NXRMLOG: Copy driver file from %s to %s failed. Error Code: %d", wstrFile, wstrDistFile, lastErr);
				MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
				if(lastErr != ERROR_SHARING_VIOLATION)
				{
					Wow64RevertWow64FsRedirection(OldValue);
					return ERROR_INSTALL_FAILURE;
				}
			}
			else
			{
				WCHAR wstrMsg[1024] = {0};
				swprintf_s(wstrMsg, 1024, L"NXRMLOG: Copy driver file from %s to %s success.", wstrFile , wstrDistFile);
				MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
			}
		}
		Wow64RevertWow64FsRedirection(OldValue) ;
	}

	
	//Get nxrmdrv driverstore path info from register**********************************
	HKEY hKey;
	LONG lResult;

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DIFx\\Services\\nxrmdrv\\"),
									0, 
									KEY_READ|KEY_WOW64_64KEY, 
									&hKey);
	
	if(ERROR_SUCCESS == lResult )
	{	
		WCHAR wstrTemp[MAX_PATH];
		DWORD dwBufsize = 0;
		dwBufsize = sizeof(wstrTemp)*sizeof(WCHAR);
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, 			
											TEXT("RefCount"),
											NULL, 
											NULL, 
											(LPBYTE)wstrTemp, 
											&dwBufsize))
		{
			WCHAR wstrSys[MAX_PATH] = {0};
			WCHAR wstrInf[MAX_PATH] = {0};
			WCHAR wstrSourceSys[MAX_PATH] = {0};
			WCHAR wstrSourceInf[MAX_PATH] = {0};
			swprintf_s(wstrSys, MAX_PATH, L"%sSystem32\\DRVSTORE\\%s\\nxrmdrv.sys", wstrWindowsDir ,wstrTemp);
			swprintf_s(wstrInf, MAX_PATH, L"%sSystem32\\DRVSTORE\\%s\\nxrmdrv.inf", wstrWindowsDir ,wstrTemp);
			swprintf_s(wstrSourceSys, MAX_PATH, L"%sdrv2\\nxrmdrv.sys", wstrBinDir);
			swprintf_s(wstrSourceInf, MAX_PATH, L"%sdrv2\\nxrmdrv.inf", wstrBinDir);
			
			OldValue = NULL;
			if( Wow64DisableWow64FsRedirection(&OldValue) )
			{
				SetFileAttributes(wstrSys, FILE_ATTRIBUTE_NORMAL);
				if(CopyFile(wstrSourceSys, wstrSys, FALSE)==FALSE) //failed
				{
					WCHAR wstrMsg[1024] = {0};
					swprintf_s(wstrMsg, 1024, L"NXRMLOG: ERROR CODE: %d , copy driver file from %s to %s failed. ", GetLastError(), wstrSourceSys, wstrSys);
					MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);

				}
				SetFileAttributes(wstrInf, FILE_ATTRIBUTE_NORMAL);
				if(CopyFile(wstrSourceInf, wstrInf, FALSE)== FALSE)
				{
					WCHAR wstrMsg[1024] = {0};
					swprintf_s(wstrMsg, 1024, L"NXRMLOG: ERROR CODE: %d , copy driver file from %s to %s failed. ", GetLastError(), wstrSourceInf, wstrInf);
					MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
				}
				Wow64RevertWow64FsRedirection(OldValue) ;
			}
		}
		else
		{
			WCHAR wstrMsg[1024] = {0};
			swprintf_s(wstrMsg, 1024, L"NXRMLOG: query key for nxrmdrv error. ERROR CODE: %d",GetLastError());
			MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
		}
		RegCloseKey(hKey);
	}
	else
	{
		WCHAR wstrMsg[1024] = {0};
		swprintf_s(wstrMsg, 1024, L"NXRMLOG: open key nxrmdrv error. ERROR CODE: %d", lResult);
		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
	}
		

	//Get nxrmflt driverstore path info from register	
	hKey = NULL;
	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DIFx\\Services\\nxrmflt\\"),
									0, 
									KEY_READ|KEY_WOW64_64KEY, 
									&hKey))

	{	
		WCHAR wstrTemp[MAX_PATH] = {0};
		DWORD dwBufsize = 0;
	
		dwBufsize = sizeof(wstrTemp)/sizeof(WCHAR);
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, 			
											TEXT("RefCount"),
											NULL, 
											NULL, 
											(LPBYTE)wstrTemp, 
											& dwBufsize))
		{
			WCHAR wstrSys[MAX_PATH] = {0};
			WCHAR wstrInf[MAX_PATH] = {0};
			WCHAR wstrSourceSys[MAX_PATH] = {0};
			WCHAR wstrSourceInf[MAX_PATH] = {0};
			swprintf_s(wstrSys, MAX_PATH, L"%sSystem32\\DRVSTORE\\%s\\nxrmflt.sys", wstrWindowsDir ,wstrTemp);
			swprintf_s(wstrInf, MAX_PATH, L"%sSystem32\\DRVSTORE\\%s\\nxrmflt.inf", wstrWindowsDir ,wstrTemp);
			swprintf_s(wstrSourceSys, MAX_PATH, L"%sdrv1\\nxrmflt.sys", wstrBinDir);
			swprintf_s(wstrSourceInf, MAX_PATH, L"%sdrv1\\nxrmflt.inf", wstrBinDir);

			OldValue = NULL;
			if( Wow64DisableWow64FsRedirection(&OldValue) )
			{
				SetFileAttributes(wstrSys, FILE_ATTRIBUTE_NORMAL);
				if(CopyFile(wstrSourceSys, wstrSys, FALSE)==FALSE) //failed
				{
					WCHAR wstrMsg[1024] = {0};
					swprintf_s(wstrMsg, 1024, L"NXRMLOG: ERROR CODE: %d , copy driver file from %s to %s failed. ", GetLastError(), wstrSourceSys, wstrSys);
					MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);

				}
				SetFileAttributes(wstrInf, FILE_ATTRIBUTE_NORMAL);
				if(CopyFile(wstrSourceInf, wstrInf, FALSE)== FALSE)
				{
					WCHAR wstrMsg[1024] = {0};
					swprintf_s(wstrMsg, 1024, L"NXRMLOG: ERROR CODE: %d , copy driver file from %s to %s failed. ", GetLastError(), wstrSourceInf, wstrInf);
					MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
				}
				Wow64RevertWow64FsRedirection(OldValue) ;
			}
		}
		else
		{
			WCHAR wstrMsg[1024] = {0};
			swprintf_s(wstrMsg, 1024, L"NXRMLOG: query key for nxrmflt error. ERROR CODE: %d",GetLastError());
			MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
		}

		RegCloseKey(hKey);
	}
	else
	{
		WCHAR wstrMsg[1024] = {0};
		swprintf_s(wstrMsg, 1024, L"NXRMLOG: open key nxrmflt error. ERROR CODE: %d", lResult);
		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
	}

	
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Copy driver file done."));
    return ERROR_SUCCESS;

}

//CAStopPCService, call in immediate execution
UINT __stdcall StopPCService(MSIHANDLE hInstall) //run in immediate execution
{
    MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start stopping PC service."));

    if (GetServiceStatus(L"ComplianceEnforcerService") != SERVICE_RUNNING)
    {
        MessageAndLogging(hInstall, TRUE, L"NXRLOG: ComplianceEnforcerService is not running.  No need to stop it.");
        return ERROR_SUCCESS;
    }
    
    SDWLResult res;
	res = SDWLibStopPDP();
	if (!res)
	{
		WCHAR wstrMsg[1024];
		swprintf_s(wstrMsg, L"NXRMLOG: SDWLibStopPDP failed. Error Code: %lu, Msg: %hs", res.GetCode(), res.GetMsg().c_str());
		MessageAndLogging(hInstall, TRUE, wstrMsg);
		return ERROR_INSTALL_FAILURE;
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Stopping PC service done."));
	return ERROR_SUCCESS;
}

//CAStopRMService, call in immediate execution
UINT __stdcall StopRMService(MSIHANDLE hInstall) //run in immediate execution
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start stopping RM service."));

	if (GetServiceStatus(L"nxrmserv") != SERVICE_RUNNING)
	{
		MessageAndLogging(hInstall, TRUE, L"NXRLOG: nxrmserv is not running.  No need to stop it.");
		return ERROR_SUCCESS;
	}

	// Enable the stopping of nxrmserv.
	SDWLResult res;
	res = SDWLibSetRPMServiceStop(true);
	if (!res)
	{
		WCHAR wstrMsg[1024];
		swprintf_s(wstrMsg, L"NXRMLOG: SDWLibSetRPMServiceStop failed. Error Code: %lu, Msg: %hs", res.GetCode(), res.GetMsg().c_str());
		MessageAndLogging(hInstall, TRUE, wstrMsg);
		return ERROR_INSTALL_FAILURE;
	}

	// Stop nxrmserv.
	res = SDWLibStopRPMService();
	if (!res)
	{
		WCHAR wstrMsg[1024];
		swprintf_s(wstrMsg, L"NXRMLOG: SDWLibStopRPMService failed. Error Code: %d, Msg: %hs", res.GetCode(), res.GetMsg().c_str());
		MessageAndLogging(hInstall, TRUE, wstrMsg);
		return ERROR_INSTALL_FAILURE;
	}

	// Check the status until it is stopped or until 30 seconds has passed.
	const int numTries = 30;

	for (int i = 0; i < numTries; i++)
	{
		if (GetServiceStatus(L"nxrmserv") == SERVICE_STOPPED)
		{
			break;
		}
		MessageAndLogging(hInstall, TRUE, L"NXRMLOG: waiting for nxrmserv to finish stopping");
		Sleep(1000);
	}

	// Wait one more second, just in case the service process hasn't exited
	// yet after reporting its status as SERVICE_STOPPED.
	Sleep(1000);

	// Whether the service has stopped or not, we return success in order to
	// let the installer continue.
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Stopping RM service done."));
	return ERROR_SUCCESS;
}

//CAStopStoppableDrivers, call in deferred execution in system context
UINT __stdcall StopStoppableDrivers(MSIHANDLE hInstall) //run in deferred execution
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start stopping stoppable drivers."));

	SC_HANDLE hSCManager;

	hSCManager = OpenSCManager(NULL, NULL, 0);
	if (hSCManager == NULL)
	{
		WCHAR wstrMsg[1024];
		swprintf_s(wstrMsg, L"NXRMLOG: OpenSCManager failed. Error Code: %lu", GetLastError());
		MessageAndLogging(hInstall, TRUE, wstrMsg);
		return ERROR_INSTALL_FAILURE;
	}

	for (int i = 0; i < MAX_STOPPABLE_DRIVERS; i++)
	{
		if (GetServiceStatus(wstrStoppableDriverNames[i]) == SERVICE_RUNNING)
		{
			SC_HANDLE hService;
			hService = OpenService(hSCManager, wstrStoppableDriverNames[i], SERVICE_STOP);
			if (hService == NULL)
			{
				WCHAR wstrMsg[1024];
				swprintf_s(wstrMsg, L"NXRMLOG: OpenService(%s) failed. Error Code: %lu", wstrStoppableDriverNames[i], GetLastError());
				MessageAndLogging(hInstall, TRUE, wstrMsg);
				CloseServiceHandle(hSCManager);
				return ERROR_INSTALL_FAILURE;
			}

			SERVICE_STATUS status;
			if (!ControlService(hService, SERVICE_CONTROL_STOP, &status))
			{
				WCHAR wstrMsg[1024];
				swprintf_s(wstrMsg, L"NXRMLOG: ControlService(%s) failed. Error Code: %lu", wstrStoppableDriverNames[i], GetLastError());
				MessageAndLogging(hInstall, TRUE, wstrMsg);
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCManager);
				return ERROR_INSTALL_FAILURE;
			}

			CloseServiceHandle(hService);
		}
	}

	CloseServiceHandle(hSCManager);

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Stopping stoppable drivers done."));
	return ERROR_SUCCESS;
}

//CAInstallUnstoppableDrivers, call in deferred execution in system context
UINT __stdcall InstallUnstoppableDrivers(MSIHANDLE hInstall ) //run in deferred execution
{
#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    DisableRrightClickMenuShare();
	DisableShareTable();
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start installing unstoppable drivers."));

	WCHAR wstrInstallDir[MAX_PATH];
	WCHAR wstrBinDir[MAX_PATH];
	DWORD dwPathBuffer;
	UINT uiRetCode;

	//get current InstallDir from MSI
	dwPathBuffer = sizeof(wstrInstallDir)/sizeof(WCHAR);
	uiRetCode =  MsiGetProperty(hInstall, TEXT("CustomActionData"), wstrInstallDir, &dwPathBuffer);
	if(ERROR_SUCCESS != uiRetCode)
	{
		WCHAR wstrMsg[128];
		swprintf_s(wstrMsg, L"NXRMLOG: Failed to get install directory from MSI. Error Code: %u", uiRetCode);
		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);//log only
		return ERROR_INSTALL_FAILURE;
	}

	if(wstrInstallDir[wcslen(wstrInstallDir)-1]!= L'\\')
	{
		wcscat_s(wstrInstallDir, L"\\");
	}
	wcscpy_s(wstrBinDir, wstrInstallDir );
	wcscat_s(wstrBinDir, L"SkyDRM\\RPM\\bin");

	PVOID OldValue;
	BOOL disabledFsRedirection = Wow64DisableWow64FsRedirection(&OldValue);
	if (!disabledFsRedirection)
	{
		DWORD lastErr = GetLastError();
		if (lastErr != ERROR_INVALID_FUNCTION)
		{
			WCHAR wstrMsg[1024];
			swprintf_s(wstrMsg, L"NXRMLOG: Disable FS redirection failed, Error Code: %lu", lastErr);
			MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
			return ERROR_INSTALL_FAILURE;
		}
	}

	SC_HANDLE hSCManager;

	hSCManager = OpenSCManager(NULL, NULL, 0);
	if (hSCManager == NULL)
	{
		WCHAR wstrMsg[1024];
		swprintf_s(wstrMsg, L"NXRMLOG: OpenSCManager failed. Error Code: %lu", GetLastError());
		MessageAndLogging(hInstall, TRUE, wstrMsg);

		if (disabledFsRedirection) Wow64RevertWow64FsRedirection(OldValue);
		return ERROR_INSTALL_FAILURE;

	}

	for (int i = 0; i < MAX_UNSTOPPABLE_DRIVERS; i++)
	{
		WCHAR cmdLine[MAX_PATH + 100];
		swprintf_s(cmdLine, L"DefaultInstall 128 %s\\%s\\%s.inf", wstrBinDir, wstrUnstoppableDriverSrcDirs[i], wstrUnstoppableDriverNames[i]);
		InstallHinfSection(NULL, NULL, cmdLine, 0);

		//Usually the driver is already running if we are upgrading from another build to this build.  So don't try to start it again in this case.
		if (GetServiceStatus(wstrUnstoppableDriverNames[i]) != SERVICE_RUNNING)
		{
			SC_HANDLE hService;
			hService = OpenService(hSCManager, wstrUnstoppableDriverNames[i], SERVICE_START);
			if (hService == NULL)
			{
				WCHAR wstrMsg[1024];
				swprintf_s(wstrMsg, L"NXRMLOG: OpenService(%s) failed. Error Code: %lu", wstrUnstoppableDriverNames[i], GetLastError());
				MessageAndLogging(hInstall, TRUE, wstrMsg);

				CloseServiceHandle(hSCManager);
				if (disabledFsRedirection) Wow64RevertWow64FsRedirection(OldValue);
				return ERROR_INSTALL_FAILURE;
			}

			if (!StartService(hService, 0, NULL))
			{
				WCHAR wstrMsg[1024];
				swprintf_s(wstrMsg, L"NXRMLOG: StartService(%s) failed. Error Code: %lu", wstrUnstoppableDriverNames[i], GetLastError());
				MessageAndLogging(hInstall, TRUE, wstrMsg);

				CloseServiceHandle(hService);
				CloseServiceHandle(hSCManager);
				if (disabledFsRedirection) Wow64RevertWow64FsRedirection(OldValue);
				return ERROR_INSTALL_FAILURE;
			}

			CloseServiceHandle(hService);
		}
	}

	CloseServiceHandle(hSCManager);
	if (disabledFsRedirection) Wow64RevertWow64FsRedirection(OldValue);

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Installing unstoppable drivers done."));
	return ERROR_SUCCESS;
}

//CAScheduleRebootForUnstoppableDrivers, call in immediate execution
UINT __stdcall ScheduleRebootForUnstoppableDrivers(MSIHANDLE hInstall)
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start Scheduling reboot for unstoppable drivers."));

	for (int i = 0; i < MAX_UNSTOPPABLE_DRIVERS; i++)
	{
		if (GetServiceStatus(wstrUnstoppableDriverNames[i]) == SERVICE_RUNNING)
		{
			// One of the unstoppable drivers is running.  Need to reboot after uninstallation.
			UINT uiRetCode;

			uiRetCode =  MsiSetProperty(hInstall, L"ISSCHEDULEREBOOT", L"1");
			if (uiRetCode != ERROR_SUCCESS)
			{
				WCHAR wstrMsg[1024];
				swprintf_s(wstrMsg, L"Set ISSCHEDULEREBOOT property failed, uiRetCode=%u.", uiRetCode);
				MessageAndLogging(hInstall, FALSE, wstrMsg);
				// Don't return error.  Continue uninstallation anyway.
			}

			break;
		}
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Scheduling reboot for unstoppable drivers done."));
	return ERROR_SUCCESS;
}

//CAUninstallUnstoppableDrivers, call in deferred execution in system context
UINT __stdcall UninstallUnstoppableDrivers(MSIHANDLE hInstall ) //run in deferred execution
{
#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	EnabeleRrightClickMenuShare();
	EnableShareTable();
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start uninstalling unstoppable drivers."));

	WCHAR wstrInstallDir[MAX_PATH];
	WCHAR wstrBinDir[MAX_PATH];
	WCHAR wstrWindowsDir[MAX_PATH];
	WCHAR wstrSystemDirDrivers[MAX_PATH];
	DWORD dwPathBuffer;
	UINT uiRetCode;

	//get current InstallDir from MSI
	dwPathBuffer = sizeof(wstrInstallDir)/sizeof(WCHAR);
	uiRetCode =  MsiGetProperty(hInstall, TEXT("CustomActionData"), wstrInstallDir, &dwPathBuffer);
	if(ERROR_SUCCESS != uiRetCode)
	{
		WCHAR wstrMsg[128];
		swprintf_s(wstrMsg, L"NXRMLOG: Failed to get install directory from MSI. Error Code: %u", uiRetCode);
		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);//log only
		return ERROR_INSTALL_FAILURE;
	}

	if(wstrInstallDir[wcslen(wstrInstallDir)-1]!= L'\\')
	{
		wcscat_s(wstrInstallDir, L"\\");
	}
	wcscpy_s(wstrBinDir, wstrInstallDir );
	wcscat_s(wstrBinDir, L"SkyDRM\\RPM\\bin");

	if(!GetSystemWindowsDirectory(wstrWindowsDir, MAX_PATH))
	{
		MessageAndLogging(hInstall, TRUE, TEXT("NXRMLOG: Failed to get windows directory in this computer."));
        return ERROR_INSTALL_FAILURE;
	}
	if(wstrWindowsDir[wcslen(wstrWindowsDir)-1]!= L'\\')	
	{
		wcscat_s(wstrWindowsDir,  L"\\");
	}
	wcscpy_s(wstrSystemDirDrivers, wstrWindowsDir);
	wcscat_s(wstrSystemDirDrivers, L"System32\\drivers\\");


	PVOID OldValue;
	BOOL disabledFsRedirection = Wow64DisableWow64FsRedirection(&OldValue);
	if (!disabledFsRedirection)
	{
		DWORD lastErr = GetLastError();
		if (lastErr != ERROR_INVALID_FUNCTION)
		{
			WCHAR wstrMsg[1024];
			swprintf_s(wstrMsg, L"NXRMLOG: Disable FS redirection failed, Error Code: %lu", lastErr);
			MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);
			return ERROR_INSTALL_FAILURE;
		}
	}

	for (int i = 0; i < MAX_UNSTOPPABLE_DRIVERS; i++)
	{
		WCHAR cmdLine[MAX_PATH + 100];
		swprintf_s(cmdLine, L"DefaultUninstall 128 %s\\%s\\%s.inf", wstrBinDir, wstrUnstoppableDriverSrcDirs[i], wstrUnstoppableDriverNames[i]);
		InstallHinfSection(NULL, NULL, cmdLine, 0);

		WCHAR wstrDrvFile[MAX_PATH];
		swprintf_s(wstrDrvFile, L"%s%s.sys", wstrSystemDirDrivers, wstrUnstoppableDriverNames[i]);

		if (!DeleteFile(wstrDrvFile))
		{
			WCHAR wstrMsg[1024];
			swprintf_s(wstrMsg, L"******** NXRMLOG: DeleteFile(\"%s\") failed. ERROR CODE: %lu.  Delaying delete until reboot.", wstrDrvFile, GetLastError());
			MessageAndLogging(hInstall, TRUE, wstrMsg);

			if (!MoveFileEx(wstrDrvFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
			{
				swprintf_s(wstrMsg, L"******** NXRMLOG: MoveFileEx(\"%s\") failed. ERROR CODE: %lu.", wstrDrvFile, GetLastError());
				MessageAndLogging(hInstall, TRUE, wstrMsg);
			}
		}
	}

	if (disabledFsRedirection) Wow64RevertWow64FsRedirection(OldValue);

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Uninstalling unstoppable drivers done."));
	return ERROR_SUCCESS;
}



//CABackupLogs
UINT __stdcall BackupLogFiles(MSIHANDLE hInstall )
{
	BOOL bBackupConfFiles = TRUE;

	//Get ProductCode of previous product.
	UINT uiRet;
	WCHAR wstrPropVal[1024];
	DWORD dwPropVal;
	dwPropVal = _countof(wstrPropVal);
	uiRet = MsiGetProperty(hInstall, TEXT("OLDPRODUCTS"), wstrPropVal, &dwPropVal);
	if (uiRet != ERROR_SUCCESS)
	{
		WCHAR wstrMsg[128];
		swprintf_s(wstrMsg, L"NXRMLOG: Failed to get product code of previous product from MSI. Error Code: %d", uiRet);
		MessageAndLogging(hInstall, TRUE, wstrMsg);//log only
		return ERROR_INSTALL_FAILURE;
	}

	//Get version number of previous product from Registry.
	HKEY hKey = NULL;
	WCHAR regKey[128] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
	LONG lRet;

	wcscat_s(regKey, wstrPropVal);
	lRet =  RegOpenKeyEx(HKEY_LOCAL_MACHINE, regKey, 0,
                         KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey);
	if (lRet == ERROR_SUCCESS)
	{
		WCHAR wstrTemp[128];
		DWORD dwBufsize = sizeof(wstrTemp);

		if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"DisplayVersion", NULL,
											 NULL, (LPBYTE) wstrTemp,
											 &dwBufsize))
		{
			//If the previous product is 8.x.x or 9.0.x, don't back up config
			//files
			if (wcsncmp(wstrTemp, L"8.", 2) == 0 ||
				wcsncmp(wstrTemp, L"9.0.", 4) == 0)
			{
				bBackupConfFiles = FALSE;
			}
		}

		RegCloseKey(hKey);
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start back up log files. "));

	WCHAR wstrInstallPath[MAX_PATH] = {0};
	WCHAR wstrInstallLogDir[MAX_PATH] = {0};
	WCHAR wstrInstallConfDir[MAX_PATH] = {0};
	WCHAR wstrInstallDebugDumpFile[MAX_PATH] = {0};
	WCHAR wstrTempDir[MAX_PATH] = {0};
	WCHAR wstrTempLogDir[MAX_PATH] = {0};
	WCHAR wstrTempConfDir[MAX_PATH] = {0};
	WCHAR wstrTempConfRegisterFile[MAX_PATH] = {0};
	WCHAR wstrTempDebugDumpFile[MAX_PATH] = {0};
	DWORD dwBufsize = 0;
	BOOL bFoundInstDir =FALSE;

	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									TEXT("SYSTEM\\CurrentControlSet\\services\\nxrmserv\\"),
									0, 
									KEY_READ, 
									&hKey))

	{	
		WCHAR wstrTemp[MAX_PATH] = {0};
		DWORD dwBufsize = sizeof(wstrTemp);

		if (ERROR_SUCCESS == RegQueryValueEx(hKey, 			
											TEXT("ImagePath"),
											NULL, 
											NULL, 
											(LPBYTE)wstrTemp, 
											& dwBufsize))
		{
			WCHAR* pStr = NULL;
			WCHAR* pStr1 = NULL;
			WCHAR* pStrNext = NULL;

			pStr1 = wcstok_s(wstrTemp, TEXT("\""), &pStrNext);
			if(pStr1!= NULL)
				pStr = wcsstr(pStr1, TEXT("bin\\"));

			if( pStr != NULL)
			{	
				wcsncpy_s(wstrInstallPath, MAX_PATH, pStr1, pStr-pStr1);

				wcscpy_s(wstrInstallLogDir, MAX_PATH, wstrInstallPath);	
				wcscat_s(wstrInstallLogDir, _countof(wstrInstallLogDir),  L"log\\*");

				wcscpy_s(wstrInstallConfDir, MAX_PATH, wstrInstallPath);					
				wcscat_s(wstrInstallConfDir, _countof(wstrInstallConfDir),  L"conf\\*");

				wcscpy_s(wstrInstallDebugDumpFile, wstrInstallPath);
				wcscat_s(wstrInstallDebugDumpFile, L"DebugDump.txt");

				bFoundInstDir = TRUE;
			}
		}

		RegCloseKey(hKey);
	}
	
	if(!bFoundInstDir)
	{
		MessageAndLogging(hInstall, TRUE, TEXT("The previous install path does not found. "));
		return ERROR_SUCCESS;
	}

	//get file from temp
	DWORD dwRetVal = 0;
	dwRetVal = GetTempPath(MAX_PATH,  wstrTempDir);                                 
    if ((dwRetVal > MAX_PATH) || (dwRetVal == 0))
    {
		MessageAndLogging(hInstall, TRUE, TEXT("Failed to get temp path in this computer."));
		return ERROR_SUCCESS;
    }
	
	if(wstrTempDir[wcslen(wstrTempDir)-1]!= L'\\')
	{
		wcscat_s(wstrTempDir, _countof(wstrTempDir),  L"\\");
	}

	wcscpy_s(wstrTempLogDir, MAX_PATH, wstrTempDir);	
	wcscat_s(wstrTempLogDir, _countof(wstrTempLogDir),  L"NxrmLog\\");

	BOOL result1 = SHCopy(wstrInstallLogDir, wstrTempLogDir, FALSE);
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Back up log file done."));


	if (bBackupConfFiles)
	{
		wcscpy_s(wstrTempConfDir, MAX_PATH, wstrTempDir);
		wcscat_s(wstrTempConfDir, _countof(wstrTempConfDir),  L"NxrmConf\\");

		BOOL result2 = SHCopy(wstrInstallConfDir, wstrTempConfDir, FALSE);	

		// Don't back-up (hence restore) register.xml, because we want to use the
		// new register.xml in this build instead of keeping the register.xml from
		// the previous build.
		wcscpy_s(wstrTempConfRegisterFile, wstrTempConfDir);
		//wcscat_s(wstrTempConfRegisterFile, FILENAME_REGISTER);
		//DeleteFile(wstrTempConfRegisterFile);

		MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Back up Conf files done."));
	}
	else
	{
		MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Skip Backing up Conf files."));
	}

	wcscpy_s(wstrTempDebugDumpFile, wstrTempDir);
	wcscat_s(wstrTempDebugDumpFile, L"NxrmDebugDump.txt");

	BOOL result3 = SHCopy(wstrInstallDebugDumpFile, wstrTempDebugDumpFile, FALSE);	
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Back up debug dump file done."));

    return ERROR_SUCCESS;
}

//CARestoreLogs, call in deferred execution in system context
UINT __stdcall RestoreLogFiles(MSIHANDLE hInstall )
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start Restore log and conf files. "));

	HKEY hKey = NULL;
	WCHAR wstrInstallDir[MAX_PATH] = {0};	
	WCHAR wstrLogDir[MAX_PATH] = {0};
	WCHAR wstrConfDir[MAX_PATH] = {0};
	WCHAR wstrDebugDumpFile[MAX_PATH] = {0};
	WCHAR wstrTempDir[MAX_PATH] = {0};
	WCHAR wstrTempLogDir[MAX_PATH] = {0};
	WCHAR wstrTempConfDir[MAX_PATH] = {0};
	WCHAR wstrTempDebugDumpFile[MAX_PATH] = {0};

	DWORD dwBufsize = 0;
	BOOL bFoundLogDir = FALSE;
	UINT uiRetCode = 0;

	
	//get current Installdir from MSI
	dwBufsize = sizeof(wstrInstallDir)/sizeof(WCHAR);
	uiRetCode =  MsiGetProperty(hInstall, TEXT("CustomActionData"), wstrInstallDir, &dwBufsize);
	if(ERROR_SUCCESS != uiRetCode)
	{
		WCHAR wstrMsg[128] = {0};		
		swprintf_s(wstrMsg, 128, L"NXRMLOG: Failed to get install directory from MSI. Error Code: %d", uiRetCode);
		MessageAndLogging(hInstall, TRUE, (LPCWSTR)wstrMsg);//log only	
		return ERROR_SUCCESS;
	}
	
	if(wstrInstallDir[wcslen(wstrInstallDir)-1]!= L'\\')
	{
		wcscat_s(wstrInstallDir, _countof(wstrInstallDir),  L"\\");
	}

	wcscpy_s(wstrLogDir, MAX_PATH, wstrInstallDir);
	wcscat_s(wstrLogDir, _countof(wstrLogDir),  L"SkyDRM\\RPM\\log\\");

	wcscpy_s(wstrConfDir, MAX_PATH, wstrInstallDir);
	wcscat_s(wstrConfDir, _countof(wstrConfDir),  L"SkyDRM\\RPM\\conf\\");

	wcscpy_s(wstrDebugDumpFile, wstrInstallDir);
	wcscat_s(wstrDebugDumpFile, L"SkyDRM\\RPM\\DebugDump.txt");

	
	//get file from temp
	DWORD dwRetVal = 0;
	dwRetVal = GetTempPath(MAX_PATH,  wstrTempDir);                                 
    if ((dwRetVal > MAX_PATH) || (dwRetVal == 0))
    {
		MessageAndLogging(hInstall, TRUE, TEXT("Failed to get temp path in this computer."));
		return ERROR_SUCCESS;
    }
	
	if(wstrTempDir[wcslen(wstrTempDir)-1]!= L'\\')
	{
		wcscat_s(wstrTempDir, _countof(wstrTempDir),  L"\\");
	}

	//Restore log	
	//for move files
	wcscpy_s(wstrTempLogDir, MAX_PATH, wstrTempDir);
	wcscat_s(wstrTempLogDir, _countof(wstrTempLogDir),  L"NxrmLog\\*"); 
	BOOL result1 = SHCopy(wstrTempLogDir, wstrLogDir, TRUE);
	// for clean up 
	WCHAR wstrRemoveTempLogDir[MAX_PATH]= {0};
	wcscpy_s(wstrRemoveTempLogDir, MAX_PATH, wstrTempDir);
	wcscat_s(wstrRemoveTempLogDir, _countof(wstrRemoveTempLogDir),  L"NxrmLog"); 
	RemoveDirectory(wstrRemoveTempLogDir);
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Restore log files done."));

	//Restore conf	
	//for move files
	wcscpy_s(wstrTempConfDir, MAX_PATH, wstrTempDir);
	wcscat_s(wstrTempConfDir, _countof(wstrTempConfDir),  L"NxrmConf\\*"); 
	BOOL result2 = SHCopy(wstrTempConfDir, wstrConfDir, TRUE);
	// for clean up later
	WCHAR wstrRemoveTempConfDir[MAX_PATH]= {0};
	wcscpy_s(wstrRemoveTempConfDir, MAX_PATH, wstrTempDir);
	wcscat_s(wstrRemoveTempConfDir, _countof(wstrRemoveTempConfDir),  L"NxrmConf"); 
	RemoveDirectory(wstrRemoveTempConfDir);
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Restore Conf files done."));

	//Restore debug dump
	//for move file
	wcscpy_s(wstrTempDebugDumpFile, wstrTempDir);
	wcscat_s(wstrTempDebugDumpFile, L"NxrmDebugDump.txt");
	BOOL result3 = SHCopy(wstrTempDebugDumpFile, wstrDebugDumpFile, TRUE);
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Restore debug dump file done."));

    return ERROR_SUCCESS;

}

// If skipAuditLog is TRUE:
// - Return TRUE if:
//   - Directory and all of its content are deleted.  (*auditLogFound will be
//     set to FALSE.)
//   - Directory is not deleted because audit log files are present, but all
//     non-audit-log files and empty sub-directories are deleted.
//     (*auditLogFound will be set to TRUE.)
// - Return FALSE if:
//   - Directory is not deleted because some non-audit-log files or empty
//     sub-directories cannot be deleted.  (*auditLogFound will be undefined.)
//
// If skipAuditLog is FALSE:
// - Return TRUE if :
//   - Directory and all of its content are deleted.
// - Return FALSE if:
//   - Directory is not deleted because some files or empty sub-directories
//     cannot be deleted.
// (*auditLogFound will be undefined.)
BOOL DeleteDirMaybeSkipAuditLogs(MSIHANDLE hInstall, const wchar_t *dir,
								 BOOL tryAfterReboot, BOOL skipAuditLog,
								 BOOL *auditLogFound)
{
	BOOL ret = TRUE;
	HANDLE h;
	WIN32_FIND_DATA data;

	*auditLogFound = FALSE;
	wchar_t wildcardPathBuf[MAX_PATH];
	wcscpy_s(wildcardPathBuf, dir);
	wcscat_s(wildcardPathBuf, L"\\*");

	h = FindFirstFile(wildcardPathBuf, &data);
	if (h == INVALID_HANDLE_VALUE)
	{
		DWORD lastErr = GetLastError();
		// Even if directory is empty, FindFirstFile() should still have found
		// "."  or ".." (except for empty root directories, in which case we
		// can't delete anyway).  Hence this error is a real error.
		WCHAR wstrMsg[1024];
		swprintf_s(wstrMsg, L"******** NXRMLOG: DeleteDirMaybeSkipAuditLogs: FindFirstFile(\"%s\") failed. ERROR CODE: %lu", wildcardPathBuf, lastErr);
		MessageAndLogging(hInstall, TRUE, wstrMsg);

		// If the error is access-denied, try to delete the directory itself
		// anyway.  If deletion is successful, we'll know that the directory
		// is empty.
		//
		// This handles some Content.IE5 directories where we get
		// ERROR_ACCESS_DENIED when trying to traverse it but we can
		// successfully delete it.
		if (lastErr != ERROR_ACCESS_DENIED)
		{
			return FALSE;
		}
	}
	else
	{
		do
		{
			if (wcscmp(data.cFileName, L".") == 0 ||
				wcscmp(data.cFileName, L"..") == 0)
			{
				continue;
			}

			wchar_t subPathBuf[MAX_PATH];
			wcscpy_s(subPathBuf, dir);
			wcscat_s(subPathBuf, L"\\");
			wcscat_s(subPathBuf, data.cFileName);

			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				// Recurse.
				BOOL auditLogFoundInSubDir;

				if (!DeleteDirMaybeSkipAuditLogs(hInstall, subPathBuf,
												 tryAfterReboot, skipAuditLog,
												 &auditLogFoundInSubDir))
				{
					ret = FALSE;
				}

				if (auditLogFoundInSubDir)
				{
					*auditLogFound = TRUE;
				}
			}
			else
			{
				if (skipAuditLog && wcscmp(data.cFileName, FILENAME_AUDIT_LOG) ==0)
				{
					*auditLogFound = TRUE;
					continue;
				}

				if (!DeleteFile(subPathBuf))
				{
					WCHAR wstrMsg[1024];
					swprintf_s(wstrMsg, L"******** NXRMLOG: DeleteDirMaybeSkipAuditLogs: DeleteFile(\"%s\") failed. ERROR CODE: %lu", subPathBuf, GetLastError());
					MessageAndLogging(hInstall, TRUE, wstrMsg);

					if (tryAfterReboot)
					{
						if (!MoveFileEx(subPathBuf, NULL,
										MOVEFILE_DELAY_UNTIL_REBOOT))
						{
							swprintf_s(wstrMsg, L"******** NXRMLOG: DeleteDirMaybeSkipAuditLogs: MoveFileEx(\"%s\") failed. ERROR CODE: %lu", subPathBuf, GetLastError());
							MessageAndLogging(hInstall, TRUE, wstrMsg);
							ret = FALSE;
						}
					} else {
						ret = FALSE;
					}
				}
			}
		} while (FindNextFile(h, &data));

		FindClose(h);
	}

	// Remove this directory if 1) no error occurred in this directory or any
	// sub-directories, and 2) no audit log files were found in this directory
	// or any sub-directories.
	if (ret && !*auditLogFound)
	{
		if (!RemoveDirectory(dir))
		{
			WCHAR wstrMsg[1024];
			swprintf_s(wstrMsg, L"******** NXRMLOG: DeleteDirMaybeSkipAuditLogs: RemoveDirectory(\"%s\") failed. ERROR CODE: %lu", dir, GetLastError());
			MessageAndLogging(hInstall, TRUE, wstrMsg);

			if (tryAfterReboot)
			{
				if (!MoveFileEx(dir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
				{
					swprintf_s(wstrMsg, L"******** NXRMLOG: DeleteDirMaybeSkipAuditLogs: MoveFileEx(\"%s\") failed. ERROR CODE: %lu", dir, GetLastError());
					MessageAndLogging(hInstall, TRUE, wstrMsg);
					ret = FALSE;
				}
			} else {
				ret = FALSE;
			}
		}
	}

	return ret;
}

BOOL DeleteProfilesCommon(MSIHANDLE hInstall, BOOL tryAfterReboot,
						  BOOL skipAuditLog)
{
	WCHAR wstrSourceDir[MAX_PATH];
	DWORD dwPathBuffer;
	UINT uiRet;

	dwPathBuffer = _countof(wstrSourceDir);
	uiRet = MsiGetProperty(hInstall, TEXT("CustomActionData"), wstrSourceDir, &dwPathBuffer );

	if(wstrSourceDir[wcslen(wstrSourceDir)-1]!= L'\\')
	{
		wcscat_s(wstrSourceDir, _countof(wstrSourceDir),  L"\\");
	}

	wcscat_s(wstrSourceDir, L"SkyDRM\\RPM\\profiles");

	// It's okay if profiles directory is not found.
	if (GetFileAttributes(wstrSourceDir) == INVALID_FILE_ATTRIBUTES &&
		GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: profiles directory not found."));
		return TRUE;
	}

	BOOL auditLogFound;
	if (!DeleteDirMaybeSkipAuditLogs(hInstall, wstrSourceDir, tryAfterReboot,
									 skipAuditLog, &auditLogFound))
	{
		MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Delete profiles error."));
		return FALSE;
	}

	return TRUE;
}

//CADeleteProfiles, call in deferred execution in system context
UINT __stdcall DeleteProfiles(MSIHANDLE hInstall)
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start deleting profiles."));

	if (!DeleteProfilesCommon(hInstall, TRUE, FALSE))
	{
		return ERROR_INSTALL_FAILURE;
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Deleting profiles done."));
	return ERROR_SUCCESS;
}

//CADeleteProfilesExceptAuditLogs, call in deferred execution in system context
UINT __stdcall DeleteProfilesExceptAuditLogs(MSIHANDLE hInstall)
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start deleting profiles except audit logs."));

	if (!DeleteProfilesCommon(hInstall, TRUE, TRUE))
	{
		return ERROR_INSTALL_FAILURE;
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Deleting profiles except audit logs done."));
	return ERROR_SUCCESS;
}

//CAClientReg, call in deferred execution in system context
UINT __stdcall ClientReg(MSIHANDLE hInstall)
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start registering client ID."));

	WCHAR wstrMsg[1024];

	WCHAR wstrAllUsers[256];
	DWORD dwAllUsers;
	UINT uiRet;

	dwAllUsers = _countof(wstrAllUsers);
	uiRet = MsiGetProperty(hInstall, TEXT("CustomActionData"), wstrAllUsers, &dwAllUsers);
	if(ERROR_SUCCESS != uiRet)
	{
		swprintf_s(wstrMsg, L"******** NXRMLOG: ClientReg: Failed to get ALLUSERS from MSI.  ERROR CODE: %u", uiRet);
		MessageAndLogging(hInstall, FALSE, (LPCWSTR)wstrMsg);
		return ERROR_INSTALL_FAILURE;
	}

	NX::RmcKeyManager clientId;
	SDWLResult res;

	// First, try to load client key from local machine's store.
	res = clientId.Load(true);
	if (!res) {
		if (res.GetCode() != ERROR_NOT_FOUND) {
			swprintf_s(wstrMsg, L"******** NXRMLOG: ClientReg: Failed to load registered client ID from local machine's store.  ERROR CODE: %d, msg: %hs",
				res.GetCode(), res.GetMsg().c_str());
			MessageAndLogging(hInstall, FALSE, wstrMsg);
			return ERROR_INSTALL_FAILURE;
		}

		// There is no client key in local machine's store.

		if (wcscmp(wstrAllUsers, L"1") == 0) {
			// We are installing for local machine.

			// Create client key in local machine's store.
			res = clientId.Create(true);
			if (!res) {
				swprintf_s(wstrMsg, L"******** NXRMLOG: ClientReg: Failed to register client ID in local machine's store.  ERROR CODE: %d, msg: %hs",
					res.GetCode(), res.GetMsg().c_str());
				MessageAndLogging(hInstall, FALSE, wstrMsg);
				return ERROR_INSTALL_FAILURE;
			}

			MessageAndLogging(hInstall, TRUE, L"******** NXRMLOG: ClientReg: Registered Client ID in local machine's store.");
		} else {
			// We are installing for current user.

			// Try to load client key from current user's store.
			res = clientId.Load(false);
			if (!res) {
				if (res.GetCode() != ERROR_NOT_FOUND) {
					swprintf_s(wstrMsg, L"******** NXRMLOG: ClientReg: Failed to load registered client ID from current user's store.  ERROR CODE: %d, msg: %hs",
						res.GetCode(), res.GetMsg().c_str());
					MessageAndLogging(hInstall, FALSE, wstrMsg);
					return ERROR_INSTALL_FAILURE;
				}

				// There is no client key in current user's store.

				// Create client key in current user's store.
				res = clientId.Create(false);
				if (!res) {
					swprintf_s(wstrMsg, L"******** NXRMLOG: ClientReg: Failed to register client ID in current user's store.  ERROR CODE: %d, msg: %hs",
						res.GetCode(), res.GetMsg().c_str());
					MessageAndLogging(hInstall, FALSE, wstrMsg);
					return ERROR_INSTALL_FAILURE;
				}

				MessageAndLogging(hInstall, TRUE, L"******** NXRMLOG: ClientReg: Registered Client ID in current user's store.");
			} else {
				// There is existing cliekt key in current user's store.
				MessageAndLogging(hInstall, TRUE, L"******** NXRMLOG: ClientReg: Found existing Client ID in current user's store.");
			}
		}
	} else {
		// There is existing cliekt key in local machine's store.
		MessageAndLogging(hInstall, TRUE, L"******** NXRMLOG: ClientReg: Found existing Client ID in local machine's store.");
	}

	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Registering client ID done."));
	return ERROR_SUCCESS;
}

UINT __stdcall DeleteUserInfoFile(MSIHANDLE hInstall)
{
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: Start delete user info file."));
	TCHAR szPath[MAX_PATH];
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath) == S_OK)
	{
		std::wstring user_dir(szPath);
		user_dir = user_dir.substr(0, user_dir.find_first_of(L"\\/") + 6);
		for (auto& p : std::experimental::filesystem::directory_iterator(user_dir)) {
			if (std::experimental::filesystem::is_directory(p.path())) {
				std::experimental::filesystem::path user_data_dir(p.path().wstring() + L"\\AppData\\Local\\NextLabs\\SkyDRM\\rmsdk");
				if (std::experimental::filesystem::exists(user_data_dir)) {
					if (std::experimental::filesystem::remove_all(user_data_dir)) {
						MessageAndLogging(hInstall, TRUE, user_data_dir.wstring().c_str());
					}
				}
			}
		}
	}
	
	MessageAndLogging(hInstall, TRUE, TEXT("******** NXRMLOG: End delete user info file."));
	return ERROR_SUCCESS;
}
