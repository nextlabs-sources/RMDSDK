
#include "stdafx.h"
#include "sdkwrapper.h"
#include "SDLAPI.h"
#include "helper.h"
#include "RegistryServiceEntry.h"
//
// Global
//
ISDRmcInstance* g_RmsIns = NULL;
// for caller user hTenant to reference, we have to cache it here
std::list<ISDRmTenant*> g_listTenant = std::list<ISDRmTenant*>();
// for current login user
ISDRmUser* g_User = NULL;

// RMP security 
static const std::string gRPM_Security = "{6829b159-b9bb-42fc-af19-4a6af3c9fcf6}";
static const std::string NEW_RECIPIENTS = "newRecipients";
static const std::string REMOVED_RECIPIENTS = "removedRecipients";

#pragma region SDK_Level

NXSDK_API void SdkLibInit()
{
	OutputDebugStringA("Call SdkLibInit\n");
	SDWLibInit();
}

NXSDK_API void SdkLibCleanup()
{
	OutputDebugStringA("Call SdkLibCleanup\n");
	SDWLibCleanup();
}

NXSDK_API DWORD GetSDKVersion()
{
	return SDWLibGetVersion();
}

NXSDK_API DWORD GetCurrentLoggedInUser(HANDLE* phSession, HANDLE* hUser)
{
	ISDRmcInstance *pInstance = NULL;
	ISDRmTenant *pTenant = NULL;
	ISDRmUser *puser = NULL;
	std::string strPasscode = gRPM_Security;
	SDWLResult res = RPMGetCurrentLoggedInUser(strPasscode, pInstance, pTenant, puser);

	if (!res) {

		// For this case, will still return the 'Instance', and we'll call RPMRequestLogin by it.
		if (pInstance != NULL) 
		{
			g_RmsIns = pInstance;
			*phSession = (HANDLE*)g_RmsIns;
		}
		return res.GetCode();
	}

	if (res.GetCode() == 0) {
		g_RmsIns = pInstance;
		*phSession = (HANDLE*)g_RmsIns;
		g_listTenant.push_back(pTenant);
		g_User = puser;
		*hUser = g_User;
	}
	return SDWL_SUCCESS;
}


NXSDK_API DWORD CreateSDKSession(const wchar_t * wcTempPath, HANDLE * phSession)
{
	OutputDebugStringA("call CreateSDKSession\n");
	// sanity check
	if (wcTempPath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	// Read NextLabs install folder from registry
	std::wstring strInstallDir;
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NextLabs", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		helper::GetStringRegKey(hKey, L"InstallDir", strInstallDir, L"C:\\Program Files\\NextLabs");
	else if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\NextLabs", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		helper::GetStringRegKey(hKey, L"InstallDir", strInstallDir, L"C:\\Program Files\\NextLabs");
	else
		strInstallDir = L"C:\\Program Files\\NextLabs";
	RegCloseKey(hKey);
	wchar_t* wsInstallDir = helper::allocStrInComMem(strInstallDir);

	// since 8/28/2018, we have to provide an cient string, now is :  SkyDRM LocalApp For Windows
	static const char* ProductName = "SkyDRM Desktop for Windows";
	SDWLResult rt = SDWLibCreateSDRmcInstance(ProductName, 10, 0, 0, wsInstallDir, wcTempPath, &g_RmsIns);
	if (!rt) {
		return rt.GetCode();
	}

	// Initialize PDP connection
	bool isPDPFinished = false;
	int i = 0;
	do {
		g_RmsIns->IsInitFinished(isPDPFinished);
		// PDP in theory needs almost 10 - 20 seconds to start, we have to sleep and connect again and again
		if (i == 20 || isPDPFinished)
			break;

		i++;
		Sleep(1000);
	} while (!isPDPFinished);
	// Give Caller an named handle
	*phSession = (HANDLE*)g_RmsIns;
	return SDWL_SUCCESS;
}

NXSDK_API DWORD DeleteSDKSession(HANDLE  hSession)
{
	OutputDebugStringA("call DeleteSDKSession\n");
	if ((void*)hSession != (void*)g_RmsIns) {
		return SDWL_INTERNAL_ERROR;
	}

	SDWLibDeleteRmcInstance(g_RmsIns);
	g_RmsIns = NULL;


	return SDWL_SUCCESS;
}

NXSDK_API DWORD WaitInstanceInitFinish()
{
	if (g_RmsIns == NULL)
	{
		return false;
	}

	const DWORD dwTryTime = 3;
	bool bFinished = false;
	DWORD dwWaitTime = 0;
	SDWLResult rt;
	do
	{
		dwWaitTime++;
		bFinished = false;
		rt = g_RmsIns->IsInitFinished(bFinished);

		if (!bFinished)
		{
			Sleep(1000);
		}
	} while ((!bFinished) && (dwWaitTime < dwTryTime));

	if (bFinished)
	{
		return SDWL_SUCCESS;
	}
	else 
	{
		return rt.GetCode();
	}
}

#pragma endregion

#pragma region SDWL_Session
NXSDK_API DWORD SDWL_Session_Initialize(HANDLE hSession, 
	const wchar_t * router, const wchar_t * tenant)
{
	OutputDebugStringA("call SDWL_Session_Initialize\n");
	// sanity check
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (router == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (tenant == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_RmsIns->Initialize(router, tenant);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Session_Initialize2(HANDLE hSession, 
	const wchar_t * workingfolder, 
	const wchar_t * router, 
	const wchar_t * tenant)
{
	OutputDebugStringA("call SDWL_Session_Initialize2\n");
	// sanity check
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (workingfolder == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (router == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (tenant == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_RmsIns->Initialize(workingfolder, router, tenant);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Session_SaveSession(HANDLE hSession, const wchar_t * folder)
{
	OutputDebugStringA("call SDWL_Session_SaveSession\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_RmsIns->Save();
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Session_GetCurrentTenant(HANDLE hSession, HANDLE * phTenant)
{
	OutputDebugStringA("call SDWL_Session_GetCurrentTenant\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	ISDRmTenant* pTenant = NULL;
	auto rt = g_RmsIns->GetCurrentTenant(&pTenant);
	if (!rt || pTenant == NULL) {
		return rt.GetCode();
	}
	// set value to caller
	*phTenant = (HANDLE)pTenant;
	// cache this value
	g_listTenant.push_back(pTenant);
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Session_GetLoginParams(HANDLE hSession, 
	wchar_t** ppURL, NXL_LOGIN_COOKIES** ppCookies, size_t* pSize)
{
	OutputDebugStringA("Call SDWL_Session_GetLoginParams\n");
	//sanity check
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (ppURL == NULL || ppCookies == NULL || pSize ==  NULL) {
		return SDWL_INTERNAL_ERROR;

	}

	ISDRmHttpRequest* pRequest = NULL;
	auto rt = g_RmsIns->GetLoginRequest(&pRequest);
	if (!rt || pRequest == NULL) {
		return rt.GetCode();
	}

	auto url = pRequest->GetPath();
	*ppURL = helper::allocStrInComMem(url);	

	// prepare the values to caller.
	auto cookies = pRequest->GetCookies();
	// set size
	int size = (int)cookies.size();
	*pSize = size;
	if (size == 0) {
		return SDWL_SUCCESS;
	}
	// alloc buf
	NXL_LOGIN_COOKIES* p= (NXL_LOGIN_COOKIES*)::CoTaskMemAlloc(size * sizeof(NXL_LOGIN_COOKIES));
	for (int i = 0; i < size; i++) {
		HttpCookie q = cookies[i];
		// key
		p[i].key = helper::allocStrInComMem(q.first);
		p[i].values = helper::allocStrInComMem(q.second);
	}
	*ppCookies = p;
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Session_SetLoginRequest(HANDLE hSession,
	const wchar_t* JsonReturn, const wchar_t* security, HANDLE* hUser)
{
	OutputDebugStringA("call SDWL_Session_SetLoginRequest\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (JsonReturn == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmUser* puser = NULL;
	std::string utf8JsonReturn = helper::utf162utf8(JsonReturn);
	std::string utf8Security = helper::utf162utf8(security);
	auto rt = g_RmsIns->SetLoginResult(utf8JsonReturn, &puser, utf8Security);
	if (!rt || puser == NULL) {
		return rt.GetCode();
	}

	// set value to caller
	*hUser = (HANDLE)puser;
	// catch value
	g_User = puser;
	// Call once to get heartbeat (user attributes, policies)
	g_User->GetHeartBeatInfo();
	// by osmond, this should to blame SDK-devs, it has to call save() here, I either do not know why
	g_RmsIns->Save();

	// Initialize RPM
	if (g_RmsIns->IsRPMDriverExist())
	{
		auto ret = g_RmsIns->SetRPMLoginResult(utf8JsonReturn, gRPM_Security);
		if (!ret)
		{
			return ret.GetCode();
		}
		// Read NextLabs install folder from registry
		//std::wstring strInstallDir;
		//HKEY hKey;
		//if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NextLabs", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		//    helper::GetStringRegKey(hKey, L"InstallDir", strInstallDir, L"C:\\Program Files\\NextLabs");
		//else if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\NextLabs", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		//    helper::GetStringRegKey(hKey, L"InstallDir", strInstallDir, L"C:\\Program Files\\NextLabs");
		//else
		//    strInstallDir = L"C:\\Program Files\\NextLabs";
		//RegCloseKey(hKey);

		// g_RmsIns->SetRPMPDPDir(strInstallDir);
		//g_RmsIns->SetRPMPolicyBundle();
		g_RmsIns->SyncUserAttributes();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Session_GetLoginUser(HANDLE hSession, 
	const wchar_t * UserEmail, 
	const wchar_t * PassCode,
	HANDLE * hUser)
{
	OutputDebugStringA("call SDWL_Session_GetLoginUser\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (UserEmail == NULL || PassCode == NULL) {
		return SDWL_INTERNAL_ERROR;
	}


	ISDRmUser* puser = NULL;
	auto rt = g_RmsIns->GetLoginUser(
		helper::utf162utf8( UserEmail), 
		helper::utf162utf8(PassCode), 
		&puser);
	if (!rt || puser == NULL ) {
		return rt.GetCode();
	}
	//set value to caller
	*hUser = (HANDLE)puser;
	g_User = puser;

	// Call once to get heartbeat (user attributes, policies)
	g_User->GetHeartBeatInfo();
	// by osmond, this should to blame SDK-devs, it has to call save() here, I either do not know why
	g_RmsIns->Save();

	// Initialize RPM
	if (g_RmsIns->IsRPMDriverExist())
	{
		std::string JsonReturn;
		auto ret = g_RmsIns->GetLoginData(helper::utf162utf8(UserEmail), helper::utf162utf8(PassCode), JsonReturn);
		if (!ret)
		{
			return ret.GetCode();
		}

		ret = g_RmsIns->SetRPMLoginResult(JsonReturn, gRPM_Security);
		if (!ret)
		{
			return ret.GetCode();
		}

		// Read NextLabs install folder from registry
		//std::wstring strInstallDir;
		//HKEY hKey;
		//if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NextLabs", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		//	helper::GetStringRegKey(hKey, L"InstallDir", strInstallDir, L"C:\\Program Files\\NextLabs");
		//else if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\NextLabs", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		//	helper::GetStringRegKey(hKey, L"InstallDir", strInstallDir, L"C:\\Program Files\\NextLabs");
		//else
		//	strInstallDir = L"C:\\Program Files\\NextLabs";
		//RegCloseKey(hKey);

		//g_RmsIns->SetRPMPDPDir(strInstallDir);
		//g_RmsIns->SetRPMPolicyBundle();
		g_RmsIns->SyncUserAttributes();
	}

	return SDWL_SUCCESS;
}




#pragma endregion

#pragma region Tenant_Level
NXSDK_API DWORD SDWL_Tenant_GetTenant(HANDLE hTenant, wchar_t ** pptenant)
{
	OutputDebugStringA("call SDWL_Tenant_GetTenant\n");
	// find if hTenant lies in g_listTenant( std::list<ISDRmTenant*> )
	auto result = std::find(g_listTenant.begin(), g_listTenant.end(), hTenant);
	if (result == g_listTenant.end() || *result== NULL) {
		// not found or value is invalid
		return SDWL_INTERNAL_ERROR;
	}

	*pptenant = helper::allocStrInComMem((*result)->GetTenant());

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Tenant_GetRouterURL(HANDLE hTenant, wchar_t ** pprouterurl)
{
	OutputDebugStringA("call SDWL_Tenant_GetRouterURL\n");
	// find if hTenant lies in g_listTenant
	auto result = std::find(g_listTenant.begin(), g_listTenant.end(), hTenant);
	if (result == g_listTenant.end() || *result == NULL) {
		// not found or value is invalid
		return SDWL_INTERNAL_ERROR;
	}

	*pprouterurl = helper::allocStrInComMem((*result)->GetRouterURL());
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_Tenant_GetRMSURL(HANDLE hTenant, wchar_t ** pprmsurl)
{
	OutputDebugStringA("call SDWL_Tenant_GetRMSURL\n");
	// find if hTenant lies in g_listTenant
	auto result = std::find(g_listTenant.begin(), g_listTenant.end(), hTenant);
	if (result == g_listTenant.end() || *result == NULL) {
		// not found or value is invalid
		return SDWL_INTERNAL_ERROR;
	}
	*pprmsurl = helper::allocStrInComMem((*result)->GetRMSURL());
	return SDWL_SUCCESS;
}



NXSDK_API DWORD SDWL_Tenant_ReleaseTenant(HANDLE hTenant)
{
	OutputDebugStringA("call SDWL_Tenant_ReleaseTenant\n");
	// find if hTenant lies in g_listTenant
	auto result = std::find(g_listTenant.begin(), g_listTenant.end(), hTenant);
	if (result == g_listTenant.end()) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	else {
		g_listTenant.remove(*result);
	}
	return SDWL_SUCCESS;
}
#pragma endregion

#pragma region User_Level

#pragma region User_Base
NXSDK_API DWORD SDWL_User_GetUserName(HANDLE hUser, wchar_t ** ppname)
{
	OutputDebugStringA("call SDWL_User_GetUserName\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	*ppname = helper::allocStrInComMem(g_User->GetName());
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetUserEmail(HANDLE hUser, wchar_t ** ppemail)
{
	OutputDebugStringA("call SDWL_User_GetUserEmail\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	*ppemail = helper::allocStrInComMem(g_User->GetEmail());
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetPasscode(HANDLE hUser, wchar_t ** pppasscode)
{
	OutputDebugStringA("call SDWL_User_GetPasscode\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	*pppasscode = helper::allocStrInComMem(helper::utf82utf16(g_User->GetPasscode()));
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetUserId(HANDLE hUser, unsigned int* userId) 
{
	OutputDebugStringA("call SDWL_User_GetUserId\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	*userId = g_User->GetUserID();

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectMembershipId(HANDLE hUser, DWORD32 projectId, wchar_t ** projectMembershipId)
{
	OutputDebugStringA("call SDWL_User_ProjectMembershipId\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	// do
	*projectMembershipId = helper::allocStrInComMem(helper::utf82utf16(g_User->GetMembershipID(projectId)));


	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_TenantMembershipId(HANDLE hUser, wchar_t * tenantId, wchar_t ** tenantMembershipId)
{
	OutputDebugStringA("call SDWL_User_TenantMembershipId\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if (tenantId == NULL || wcslen(tenantId) < 1) {
		return SDWL_INTERNAL_ERROR;
	}

	// do
	*tenantMembershipId = helper::allocStrInComMem(
		helper::utf82utf16(
		g_User->GetMembershipID(
			helper::utf162utf8(tenantId))
	));


	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_LogoutUser(HANDLE hUser)
{
	OutputDebugStringA("call SDWL_User_LogoutUser\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	// Firstly, call RPM logout
	if (g_RmsIns != NULL)
	{
		auto ret = g_RmsIns->RPMLogout();
		if (!ret) {
			return ret.GetCode();
		}
	}

	// Then call this, will failed when network offline, so call RPMLogout firstly.
	auto rt = g_User->LogoutUser();
	if (!rt) {
		return rt.GetCode();
	}

	// set glocal to NULL
	g_User = NULL;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetUserType(HANDLE hUser, int * type)
{
	OutputDebugStringA("call SDWL_User_GetUserType\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (type == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	*type = g_User->GetIdpType();
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateUserInfo(HANDLE hUser)
{
	OutputDebugStringA("call SDWL_User_UpdateUserInfo\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}


	auto rt = g_User->UpdateUserInfo();
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateMyDriveInfo(HANDLE hUser)
{
	OutputDebugStringA("call SDWL_User_UpdateMyDriveInfo\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_User->UpdateMyDriveInfo();
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetMyDriveInfo(HANDLE hUser, DWORD64 * usage, DWORD64 * total, DWORD64* vaultUsage, DWORD64* vaultQuota)
{
	OutputDebugStringA("call SDWL_User_GetMyDriveInfo\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (usage == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (total == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	DWORD64 u = 0, t = 0, vu = 0,vt=0;
	auto rt = g_User->GetMyDriveInfo(u, t,vu,vt);
	if (!rt) {
		return rt.GetCode();
	}
	//set vaule
	*usage = u;
	*total = t;
	*vaultUsage = vu;
	*vaultQuota = vt;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetLocalFile(HANDLE hUser, HANDLE * hLocalFiles)
{
	OutputDebugStringA("call SDWL_User_GetLocalFile\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRFiles *pF = NULL;
	//auto rt = g_User->GetLocalFileManager(&pF);
	//if (!rt || pF == NULL) {
	//	return SDWL_INTERNAL_ERROR;
	//}	

	// set valueto called
	*hLocalFiles = (HANDLE)pF;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_RemoveLocalFile(HANDLE hUser, const wchar_t * nxlFilePath, bool* pResult)
{
	OutputDebugStringA("call SDWL_User_RemoveLocalFile\n");
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	ISDRFiles *pF = NULL;
	//auto rt = g_User->GetLocalFileManager(&pF);
	//if (!rt || pF == NULL) {
	//	return SDWL_INTERNAL_ERROR;
	//}
	//
	//*pResult=pF->RemoveFile(pF->GetFile(nxlFilePath));

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProtectFile(HANDLE hUser, 
	const wchar_t* path, 
	int* pRights,int len, 
	WaterMark watermark, Expiration expiration, 
	const wchar_t* tags,
	const wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_ProtectFile for default myvault\n");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}	
	if (path == NULL || pRights == NULL || len ==0 || outPath== NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
		

	std::wstring p(path);
	std::vector<SDRmFileRight> r;
	for (int i = 0; i < len; i++) {
		SDRmFileRight l = (SDRmFileRight)pRights[i];
		r.push_back(l);
	}
	SDR_WATERMARK_INFO w;
	{
		w.text = helper::utf162utf8(watermark.text);
		w.fontName = helper::utf162utf8(watermark.fontName);
		w.fontColor = helper::utf162utf8(watermark.fontColor);
		w.fontSize = watermark.fontSize;
		w.transparency = watermark.transparency;
		w.rotation = (WATERMARK_ROTATION)watermark.rotation;
		w.repeat = watermark.repeat;
	}
	SDR_Expiration e;
	{
		e.type = (IExpiryType)expiration.type;
		e.start = expiration.start;
		e.end = expiration.end;
	}
	std::string t(helper::utf162utf8(tags));
	std::wstring o;

	auto rt = g_User->ProtectFile(p,o, r,w,e,t);
	if (!rt) {
		return rt.GetCode();
	}

	*outPath = helper::allocStrInComMem(o);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProtectFileToProject(int projectId,
	HANDLE hUser,
	const wchar_t* path,
	int* pRights, int len,
	WaterMark watermark, Expiration expiration,
	const wchar_t* tags,
	const wchar_t** outPath) 
{
	OutputDebugStringA("call SDWL_User_ProtectFileToProject\n");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (path == NULL || outPath== NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	// find membership id by projectId,
	auto mId = g_User->GetMembershipID(projectId);
	if (mId.empty() || mId.length() < 5) {
		// invalid project membership id
		OutputDebugStringA("Error in porject_protect_file can not find proper membershipId by projectID user provided\n");
		return SDWL_INTERNAL_ERROR;
	}


	std::wstring p(path);
	std::vector<SDRmFileRight> r;
	for (int i = 0; i < len; i++) {
		SDRmFileRight l = (SDRmFileRight)pRights[i];
		r.push_back(l);
	}
	SDR_WATERMARK_INFO w;
	{
		w.text = helper::utf162utf8(watermark.text);
		w.fontName = helper::utf162utf8(watermark.fontName);
		w.fontColor = helper::utf162utf8(watermark.fontColor);
		w.fontSize = watermark.fontSize;
		w.transparency = watermark.transparency;
		w.rotation = (WATERMARK_ROTATION)watermark.rotation;
		w.repeat = watermark.repeat;
	}
	SDR_Expiration e;
	{
		e.type = (IExpiryType)expiration.type;
		e.start = expiration.start;
		e.end = expiration.end;
	}
	std::string t(helper::utf162utf8(tags));
	std::wstring o;
	auto rt = g_User->ProtectFile(p,o,r, w, e, t,mId);
	if (!rt) {
		return rt.GetCode();
	}

	*outPath = helper::allocStrInComMem(o);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateRecipients(HANDLE hUser,
	HANDLE hNxlFile,
	const wchar_t* addmails[],int lenaddmails,
	const wchar_t* delmails[],int lendelmails,
	const wchar_t* comments)
{
	OutputDebugStringA("call SDWL_User_UpdateRecipients\n");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	
	ISDRmNXLFile *pf = (ISDRmNXLFile *)hNxlFile;
	

	// prepare param
	std::vector<std::string> adds;
	std::vector<std::string> dels;

	for (int i = 0; i < lenaddmails; i++) {
		adds.push_back(helper::utf162utf8(addmails[i]));
	}

	for (int i = 0; i < lendelmails; i++) {
		dels.push_back(helper::utf162utf8(delmails[i]));
	}

	// call
	if (NULL == comments) {
		comments = L"";
	}
	auto rt = g_User->UpdateRecipients(pf, adds, comments);
	if (!rt) {
		return rt.GetCode();
		
	}	

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateRecipients2(
			HANDLE hUser, 
			const wchar_t * nxlFilePath, 
			const wchar_t * addmails[], int lenaddmails,
			const wchar_t * delmails[], int lendelmails,
	const wchar_t* comments)
{
	OutputDebugStringA("call SDWL_User_UpdateRecipients2\n");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile * pfile = NULL;
	auto rt=g_User->OpenFile(nxlFilePath, &pfile);
	if (!rt) {
		if (pfile != NULL) {
			g_User->CloseFile(pfile);
		}
		return rt.GetCode();
	}

	DWORD rt2 = SDWL_User_UpdateRecipients(hUser, (HANDLE)pfile, addmails, lenaddmails, delmails, lendelmails, comments);
	if (pfile != NULL) {
		g_User->CloseFile(pfile);
	}
	return rt2;
}

NXSDK_API DWORD SDWL_User_GetRecipients(
			HANDLE hUser, HANDLE hNxlFile, 
			wchar_t ** emails, int * peSize,
			wchar_t ** addEmials, int * paeSize,
			wchar_t ** removEmails, int * preSize)
{
	OutputDebugStringA("call SDWL_User_GetRecipients\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile *pf = (ISDRmNXLFile *)hNxlFile;
	
	if (!emails || !peSize ||
		!addEmials || !paeSize ||
		!removEmails || !preSize) {
		return SDWL_INTERNAL_ERROR;
	}

	// prepare params
	std::vector<std::string> totalE;
	std::vector<std::string> addE;
	std::vector<std::string> reE;

	*emails = NULL, *peSize = 0;
	*addEmials = NULL, *paeSize = 0;
	*removEmails = NULL, *preSize = 0;
	
	auto rt = g_User->GetRecipients(pf, totalE, addE, reE);
	if (!rt) {
		return rt.GetCode();
	}
	
	// prepare out params
	if (totalE.size() > 0) {
		*peSize = (int)totalE.size();
		// prepare Mem
		wchar_t** pbuf = (wchar_t**)::CoTaskMemAlloc(*peSize * sizeof(wchar_t*));
		for (size_t i = 0; i < totalE.size(); i++) {
			std::wstring s = helper::utf82utf16(totalE.at(i));
			pbuf[i] = (wchar_t*)::CoTaskMemAlloc((s.size()+1) * sizeof(wchar_t));
			wcscpy_s(pbuf[i], s.size() + 1, s.c_str());
		}
		*emails = (wchar_t*)pbuf;
	}
	if (addE.size() > 0) {
		*paeSize = (int)addE.size();
		// prepare Mem
		wchar_t** pbuf = (wchar_t**)::CoTaskMemAlloc(*peSize * sizeof(wchar_t*));
		for (size_t i = 0; i < addE.size(); i++) {
			std::wstring s = helper::utf82utf16(addE.at(i));
			pbuf[i] = (wchar_t*)::CoTaskMemAlloc((s.size() + 1) * sizeof(wchar_t));
			wcscpy_s(pbuf[i], s.size() + 1, s.c_str());
		}
		*addEmials = (wchar_t*)pbuf;
	}
	if (reE.size() > 0 ) {
		*preSize = (int)reE.size();
		// prepare Mem
		wchar_t** pbuf = (wchar_t**)::CoTaskMemAlloc(*peSize * sizeof(wchar_t*));
		for (size_t i = 0; i < reE.size(); i++) {
			std::wstring s = helper::utf82utf16(reE.at(i));
			pbuf[i] = (wchar_t*)::CoTaskMemAlloc((s.size() + 1) * sizeof(wchar_t));
			wcscpy_s(pbuf[i], s.size() + 1, s.c_str());
		}
		*removEmails = (wchar_t*)pbuf;
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetRecipients2(
		HANDLE hUser, const wchar_t * nxlFilePath, 
		wchar_t ** emails, int * peSize,
		wchar_t ** addEmials, int * paeSize,
		wchar_t ** removEmails, int * preSize)
{
	OutputDebugStringA("call SDWL_User_GetRecipients2\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile * pfile = NULL;
	auto rt = g_User->OpenFile(nxlFilePath, &pfile);
	if (!rt) {
		if (pfile != NULL) {
			g_User->CloseFile(pfile);
		}
		return rt.GetCode();
	}

	DWORD rt2 = SDWL_User_GetRecipients(hUser, (HANDLE)pfile, emails, peSize, addEmials, paeSize, removEmails, preSize);
	if (pfile != NULL) {
		g_User->CloseFile(pfile);
	}

	return rt2;
}

NXSDK_API DWORD SDWL_User_GetRecipients3(
	HANDLE hUser, const wchar_t* nxlFilePath,
	wchar_t** emails, wchar_t** addEmails, wchar_t** removeEmails) {
	OutputDebugStringA("call SDWL_User_GetRecipients3\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	ISDRmNXLFile * pfile = NULL;
	auto rt = g_User->OpenFile(nxlFilePath, &pfile);
	if (!rt) {
		if (pfile) {
			g_User->CloseFile(pfile);
			pfile = NULL;
		}
		return rt.GetCode();
	}

	std::vector<std::string> totalE;
	std::vector<std::string> addE;
	std::vector<std::string> removeE;
	auto ret = g_User->GetRecipients(pfile, totalE, addE, removeE);
	if (!ret) {
		if (pfile) {
			g_User->CloseFile(pfile);
			pfile = NULL;
		}
		return rt.GetCode();
	}

	//Release nxl file handle.
	if (pfile) {
		g_User->CloseFile(pfile);
		pfile = NULL;
	}

	std::string recipents;
	std::string recipentsAdd;
	std::string recipentsRemove;

	//Parse recipents email.
	std::for_each(totalE.begin(), totalE.end(), [&recipents](std::string e) {
		recipents.append(e).append(";");
	});
	//Parse recipents added email.
	std::for_each(addE.begin(), addE.end(), [&recipentsAdd](std::string e) {
		recipentsAdd.append(e).append(";");
	});
	//Parse recipents removed email.
	std::for_each(removeE.begin(), removeE.end(), [&recipentsRemove](std::string e) {
		recipentsRemove.append(e).append(";");
	});

	//Prepare data.
	{
		*emails = helper::allocStrInComMem(helper::utf82utf16(recipents));
		*addEmails = helper::allocStrInComMem(helper::utf82utf16(recipentsAdd));
		*removeEmails = helper::allocStrInComMem(helper::utf82utf16(recipentsRemove));
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UploadFile(HANDLE hUser, const wchar_t* nxlFilePath, 
	const wchar_t* sourcePath, const wchar_t* recipients, const wchar_t* comments, bool bOverwrite)
{
	OutputDebugStringA("call SDWL_User_UploadFile\n");
	// sanity check
	if (g_User == NULL || nxlFilePath == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	// call
	auto rt = g_User->UploadFile(nxlFilePath, sourcePath, recipients, comments, bOverwrite);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_OpenFile(HANDLE hUser,
	const wchar_t* nxl_path,
	HANDLE* hNxlFile )
{
	OutputDebugStringA("call SDWL_User_OpenFile\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxl_path == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	ISDRmNXLFile* p = NULL;
	auto rt = g_User->OpenFile(std::wstring(nxl_path), &p);
	if (!rt) {
		return rt.GetCode();
	}

	//set value
	*hNxlFile = (HANDLE)p;	

	//// Open file successfully, set the token to RPM also
	//if (g_RmsIns->IsRPMDriverExist())
	//{
	//	g_RmsIns->CacheRPMFileToken(nxl_path);
	//}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_OpenFileForMetaData(HANDLE hUser,
  const wchar_t* nxl_path,
  HANDLE* hNxlFile)
{
  OutputDebugStringA("call SDWL_User_OpenFileMetaData\n");
  // sanity check
  if (g_User == NULL) {
    // not found
    return SDWL_INTERNAL_ERROR;
  }
  if ((ISDRmUser*)hUser != g_User) {
    return SDWL_INTERNAL_ERROR;
  }
  if (nxl_path == NULL) {
    return SDWL_INTERNAL_ERROR;
  }

  ISDRmNXLFile* p = NULL;
  auto rt = g_User->OpenFileForMetaData(std::wstring(nxl_path), &p);
  if (!rt) {
    return rt.GetCode();
  }

  //set value
  *hNxlFile = (HANDLE)p;

  return SDWL_SUCCESS;
}


NXSDK_API DWORD SDWL_User_CacheRPMFileToken(HANDLE hUser,
	const wchar_t* nxl_path)
{
	OutputDebugStringA("call SDWL_User_CacheRPMFileToken\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (nxl_path == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	//// Open file successfully, set the token to RPM also
	//if (g_RmsIns->IsRPMDriverExist())
	//{
	//	g_RmsIns->CacheRPMFileToken(nxl_path);
	//}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_CloseFile(HANDLE hUser, HANDLE hNxlFile)
{
	OutputDebugStringA("call SDWL_User_CloseFile\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;
	auto rt = g_User->CloseFile(pF);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ForceCloseFile(HANDLE hUser, const wchar_t * nxl_path)
{
	OutputDebugStringA("call SDWL_User_ForceCloseFile\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxl_path == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	ISDRmNXLFile *pFile = NULL;
	auto rt = g_User->OpenFile(nxl_path, &pFile);
	if (pFile != NULL) {
		g_User->CloseFile(pFile);
	}
	return rt;
}

NXSDK_API DWORD SDWL_User_DecryptNXLFile(HANDLE hUser, 
	HANDLE hNxlFile, 
	const wchar_t* path)
{
	OutputDebugStringA("call SDWL_User_DecryptNXLFile\n");
	//// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (path == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;

	auto rt = g_User->DecryptNXLFile(pF, std::wstring(path));
	if (!rt) {
		return rt.GetCode();
	}
		
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UploadActivityLogs(HANDLE hUser)
{
	OutputDebugStringA("call SDWL_User_UploadActivityLogs\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	// call
	//auto rt = g_User->UploadActivityLogs();
	//if (!rt) {
	//	return rt.GetCode();
	//}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetHeartBeatInfo(HANDLE hUser)
{
	OutputDebugStringA("call SDWL_User_GetHeartBeatInfo\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_User->GetHeartBeatInfo();
	if (!rt) {
		return rt.GetCode();
	}
	


	// Do heartbeat to RPM
	if (g_RmsIns->IsRPMDriverExist())
	{
		//g_RmsIns->SetRPMPolicyBundle();
		g_RmsIns->SyncUserAttributes();
	}
	return SDWL_SUCCESS;
}

//NXSDK_API DWORD SDWL_User_GetPolicyBundle(HANDLE hUser, char * tenantName, char ** ppPolicyBundle)
//{
//	OutputDebugStringA("call SDWL_User_GetPolicyBundle\n");
//	// sanity check
//	if (g_User == NULL) {
//		// not found
//		return SDWL_INTERNAL_ERROR;
//	}
//	if ((ISDRmUser*)hUser != g_User) {
//		return SDWL_INTERNAL_ERROR;
//	}
//
//	if (tenantName == NULL || strlen(tenantName) < 5) {
//		return SDWL_INTERNAL_ERROR;
//	}
//
//	// do
//	std::string bundle;
//	auto rt = g_User->GetPolicyBundle(helper::ansi2utf16(tenantName), bundle);
//	if (!rt) {
//		return SDWL_INTERNAL_ERROR;
//	}
//	*ppPolicyBundle = helper::allocStrInComMem(bundle);
//	return SDWL_SUCCESS;
//}

NXSDK_API DWORD SDWL_User_GetWaterMarkInfo(HANDLE hUser, WaterMark * pWaterMark)
{
	OutputDebugStringA("call SDWL_User_GetWaterMarkInfo\n");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	SDR_WATERMARK_INFO wm = g_User->GetWaterMarkInfo();
	WaterMark* buf = pWaterMark;
	{
		buf->text = helper::allocStrInComMem(helper::utf82utf16( wm.text));
		buf->fontName = helper::allocStrInComMem(helper::utf82utf16(wm.fontName));
		buf->fontColor = helper::allocStrInComMem(helper::utf82utf16(wm.fontColor));
		buf->repeat = wm.repeat;
		buf->fontSize = wm.fontSize;
		buf->rotation = wm.rotation;
		buf->transparency = wm.transparency;
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetHeartBeatFrequency(HANDLE hUser, uint32_t * nSeconds)
{
	OutputDebugStringA("call SDWL_User_GetHeartBeatFrequency\n");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	int minutes=g_User->GetHeartBeatFrequency();
	// server will return minutes as the unity, change it to seconds.
	*nSeconds = minutes * 60;
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetProjectsInfo(HANDLE hUser, 
	ProjtectInfo ** pProjects, int * pSize)
{
	OutputDebugStringA("call SDWL_User_GetProjectsInfo\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (pProjects == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (pSize == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	auto vec = g_User->GetProjectsInfo();
	int size = (int)vec.size();
	*pSize = size;

	if (!size) {
		*pProjects = NULL;
		return SDWL_SUCCESS;
	}

	ProjtectInfo* p = (ProjtectInfo*)::CoTaskMemAlloc(size * sizeof(ProjtectInfo));

	for (int i = 0; i < size; i++) {
		SDR_PROJECT_INFO pif = vec[i];
		p[i].id = pif.projid;
		p[i].name = helper::allocStrInComMem(helper::utf82utf16(pif.name));
		p[i].displayname = helper::allocStrInComMem(helper::utf82utf16(pif.displayname));
		p[i].description = helper::allocStrInComMem(helper::utf82utf16(pif.description));
		p[i].owner = pif.bowner;
		p[i].totalfiles = pif.totalfiles;
		p[i].tenantid = helper::allocStrInComMem(helper::utf82utf16(pif.tokengroupname));
		// get isEnabledAdhoc
		bool isEnabledAhodc = true;
		SDWL_User_CheckProjectEnableAdhoc(hUser, NULL, &isEnabledAhodc);
		p[i].isEnableAdhoc = isEnabledAhodc == true ? 1 : 0;
	}
	*pProjects = p;
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_CheckProjectEnableAdhoc(HANDLE hUser, const wchar_t * projectTenandId, bool * isEnable)
{
	OutputDebugStringA("call SDWL_User_CheckProjectEnableAdhoc\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (isEnable == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	bool workspace = true;
	int heartbeatNoUse=0;

	std::string tenantId = projectTenandId==NULL?std::string():helper::utf162utf8(projectTenandId);
	std::string _systemProjectTenantId = "";
	int _systemProjectId = 0;

	auto rt=g_User->GetTenantPreference(isEnable, &workspace, &heartbeatNoUse, &_systemProjectId, _systemProjectTenantId, tenantId);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_CheckWorkSpaceEnable(HANDLE hUser, bool * isEnable)
{
	OutputDebugStringA("call SDWL_User_CheckSystemBucketEnableAdhoc\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (isEnable == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	bool adhoc = true;
	int heartbeatNoUse = 0;
	std::string tenantId = std::string();
	std::string _systemProjectTenantId = "";
	int _systemProjectId = 0;


	auto rt = g_User->GetTenantPreference(&adhoc, isEnable, &heartbeatNoUse, &_systemProjectId, _systemProjectTenantId, tenantId);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

// by osmond, this is almost same as SDWL_User_CheckProjectEnableAdhoc, special designed for 
NXSDK_API DWORD SDWL_User_CheckSystemBucketEnableAdhoc(HANDLE hUser, bool * isEnable)
{
	OutputDebugStringA("call SDWL_User_CheckSystemBucketEnableAdhoc\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (isEnable == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	bool workspace = true;
	int heartbeatNoUse = 0;
	std::string tenantId = std::string();
	std::string _systemProjectTenantId = "";
	int _systemProjectId = 0;


	auto rt = g_User->GetTenantPreference(isEnable, &workspace, &heartbeatNoUse, &_systemProjectId, _systemProjectTenantId, tenantId);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;

}

NXSDK_API DWORD SDWL_User_CheckSystemProject(HANDLE hUser, const wchar_t * projectTenandId, int * systemProjectId, const wchar_t** systemProjectTenandId)
{
	//OutputDebugStringA("call SDWL_User_CheckSystemProject\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (systemProjectId == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	bool workspace = true;
	int heartbeatNoUse = 0;

	std::string tenantId = projectTenandId == NULL ? std::string() : helper::utf162utf8(projectTenandId);
	std::string _systemProjectTenantId = "";
	int _systemProjectId = 0;

	bool isEnable;
	auto rt = g_User->GetTenantPreference(&isEnable, &workspace, &heartbeatNoUse, &_systemProjectId, _systemProjectTenantId, tenantId);
	if (!rt) {
		return rt.GetCode();
	}

	// if SDK return 0 for system project id, let's try to read from registry
	if (_systemProjectId <= 0 && _systemProjectTenantId.size() == 0)
	{
		DWORD dwSysProjectID = 0;
		std::wstring strSysProjectTenantID;
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NextLabs\\SkyDRM", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			helper::GetDWORDRegKey(hKey, L"SysProjectID", dwSysProjectID, 0);
			helper::GetStringRegKey(hKey, L"SysProjectTenantID", strSysProjectTenantID, L"");

			_systemProjectId = dwSysProjectID;
			_systemProjectTenantId = helper::utf162utf8(strSysProjectTenantID);
			RegCloseKey(hKey);
		}
	}

	*systemProjectId = _systemProjectId;
	*systemProjectTenandId = helper::allocStrInComMem(helper::utf82utf16(_systemProjectTenantId));

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_CheckInPlaceProtection(HANDLE hUser, const wchar_t * projectTenandId, bool * DeleteSource)
{
	//OutputDebugStringA("call SDWL_User_CheckInPlaceProtection\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (DeleteSource == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	bool isDeleteSource = false;

	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NextLabs\\SkyDRM", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		helper::GetBoolRegKey(hKey, L"DeleteSource", isDeleteSource, false);
		RegCloseKey(hKey);
	}

	*DeleteSource = isDeleteSource;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectClassifacation(
	HANDLE hUser,
	const wchar_t * tenantid,
	ProjectClassifacationLables ** ppProjectClassifacationLables,
	uint32_t* pSize)
{
	OutputDebugStringA("call SDWL_User_ProjectClassifacation\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (tenantid == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring wstrTenantID = tenantid;
	if (helper::utf162utf8(wstrTenantID) == g_User->GetSystemProjectTenantId())
	{
		wstrTenantID = helper::utf82utf16(g_User->GetDefaultTokenGroupName());
	}
	// do
	std::vector<SDR_CLASSIFICATION_CAT> cats;
	auto rt = g_User->GetClassification(
		helper::utf162utf8(wstrTenantID),
		cats);
	if (!rt) {
		return rt.GetCode();
	}

	int size = (int)cats.size();
	*pSize = (uint32_t)size;
	if (size == 0) {
		*pSize = NULL;
		return SDWL_SUCCESS;
	}


	// convert cats into ProjectClassifacationLables;
	ProjectClassifacationLables *p = (ProjectClassifacationLables*)::CoTaskMemAlloc(size * sizeof(ProjectClassifacationLables));
	for (auto i = 0; i < size; i++) {
		auto pif = cats[i];
		p[i].name = helper::allocStrInComMem(helper::utf82utf16(pif.name));
		p[i].multiseclect = pif.multiSelect;
		p[i].mandatory = pif.mandatory;
		//
		std::string tmp_lables, tmp_defautls;
		for (auto j = 0; j < pif.labels.size(); j++) {
			tmp_lables += pif.labels[j].name + ";";
			if (pif.labels[j].allow) {
				tmp_defautls += "1;";
			}
			else {
				tmp_defautls += "0;";
			}
		}
		p[i].labels = helper::allocStrInComMem(helper::utf82utf16(tmp_lables));
		p[i].isdefaults = helper::allocStrInComMem(helper::utf82utf16(tmp_defautls));
	}
	*ppProjectClassifacationLables = p;
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_EvaulateNxlFileRights(HANDLE hUser,
	const wchar_t * filePath,
	CENTRAL_RIGHTS** pArray,
	uint32_t* pArrSize,
	bool doOwnerCheck)
{
	OutputDebugStringA("call SDWL_User_EvaulateNxlFileRights\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (filePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsAndWatermarks;
	auto rt = g_User->GetRights(filePath, rightsAndWatermarks, doOwnerCheck);
	if (!rt) {
		return rt.GetCode();
	}

	//Prepare output data.
	{
		const auto pSize = rightsAndWatermarks.size();
		if (pSize > 0) {
			CENTRAL_RIGHTS* p = (CENTRAL_RIGHTS*)::CoTaskMemAlloc(sizeof(CENTRAL_RIGHTS)*pSize);
			for (size_t i = 0; i < pSize; i++) {
				//Padding single CENTRAL_RIGHTS.
				p[i].rights = rightsAndWatermarks[i].first;

				std::vector<SDR_WATERMARK_INFO> wms = rightsAndWatermarks[i].second;
				const auto wmsSize = wms.size();
				if (wmsSize > 0) {
					WaterMark* wm = (WaterMark*)::CoTaskMemAlloc(sizeof(WaterMark)*wmsSize);
					for (size_t j = 0; j < wmsSize; j++) {
						wm[j].text = helper::allocStrInComMem(helper::utf82utf16(wms[j].text));
						wm[j].fontName = helper::allocStrInComMem(helper::utf82utf16(wms[j].fontName));
						wm[j].fontColor = helper::allocStrInComMem(helper::utf82utf16(wms[j].fontColor));
						wm[j].repeat = wms[j].repeat;
						wm[j].fontSize = wms[j].fontSize;
						wm[j].rotation = wms[j].rotation;
						wm[j].transparency = wms[j].transparency;
					}
					p[i].watermarks = wm;
					p[i].watermarkLenth = wmsSize;
				}
				else {
					p[i].watermarks = NULL;
					p[i].watermarkLenth = 0;
				}
			}
			*pArray = p;
			*pArrSize = (uint32_t)pSize;
		}
		else {
			*pArray = NULL;
			*pArrSize = 0;
		}
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_AddNxlFileLog(HANDLE hUser, const wchar_t * filePath, int Oper, bool isAllow)
{
	OutputDebugStringA("call SDWL_User_AddNxlFileLog\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (filePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	// by osmond, in 12/27/2018 sdk exported new fun for add log with only path,
	// so we dont need to open the nxl first before adding tag
	/*ISDRmNXLFile * file = NULL;
	auto rt = g_User->OpenFile(filePath, &file);
	if (!rt) {
		return rt.GetCode();
	}
	if (file == NULL) {
		return SDWL_INTERNAL_ERROR;
	}*/

	auto rt = g_User->AddActivityLog(filePath,
		(RM_ActivityLogOperation)Oper, isAllow ? RL_RAllowed : RL_RDenied);
	// close file immediately
	//g_User->CloseFile(file);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetPreference(HANDLE hUser,
	Expiration* expiration,
	wchar_t**  watermarkStr)
{
	OutputDebugStringA("call SDWL_User_GetPreference\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	std::wstring watermark;
	uint32_t option; uint64_t start; uint64_t end;
	auto rt = g_User->GetUserPreference(option,
		start, end, watermark);
	if (!rt) {
		return rt.GetCode();
	}
	// prepare out params
	expiration->type = (ExpiryType)option;
	expiration->start = start;
	expiration->end = end;
	*watermarkStr = helper::allocStrInComMem(watermark);
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdatePreference(HANDLE hUser, Expiration expiration,
	const wchar_t*  watermarkStr)
{
	OutputDebugStringA("call SDWL_User_UpdatePreference\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_User->UpdateUserPreference(expiration.type,
		expiration.start, expiration.end, watermarkStr);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetNxlFileFingerPrint(HANDLE hUser,
	const wchar_t * nxlFilePath,
	NXL_FILE_FINGER_PRINT * pFingerPrint,
	bool doOwnerCheck)
{
	OutputDebugStringA("call SDWL_User_GetNxlFileFingerPrint\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	SDR_NXL_FILE_FINGER_PRINT fp;
	auto rt = g_User->GetFingerPrint(nxlFilePath, fp, doOwnerCheck);
	if (!rt) {
		return rt.GetCode();
	}

	ISDRmNXLFile * file = NULL;
	rt = g_User->OpenFile(nxlFilePath, &file);
	if (!rt) {
		return rt.GetCode();
	}
	if (file == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	DWORD64 datecreated = file->Get_DateCreated();
	DWORD64 datemodified = file->Get_DateModified();
	if (datemodified == 0) datemodified = datecreated;

	g_User->CloseFile(file);
	// fill 
	{
		pFingerPrint->name = helper::allocStrInComMem(fp.name);
		pFingerPrint->localPath = helper::allocStrInComMem(fp.localPath);
		pFingerPrint->fileSize = fp.fileSize;
		pFingerPrint->isOwner = fp.isOwner;
		pFingerPrint->isFromMyVault = fp.isFromMyVault;
		pFingerPrint->isFromProject = fp.isFromProject;
		pFingerPrint->isFromSystemBucket = fp.isFromSystemBucket;
		pFingerPrint->projectId = fp.projectId;
		pFingerPrint->isByAdHocPolicy = fp.isByAdHocPolicy;
		pFingerPrint->IsByCentrolPolicy = fp.IsByCentrolPolicy;
		pFingerPrint->tags = helper::allocStrInComMem(fp.tags);
		pFingerPrint->expiration.type = (ExpiryType)fp.expiration.type;
		pFingerPrint->expiration.start = fp.expiration.start;
		pFingerPrint->expiration.end = fp.expiration.end;
		pFingerPrint->adHocWatermar = helper::allocStrInComMem(fp.adHocWatermar);
		pFingerPrint->fileCreated = datecreated;
		pFingerPrint->fileModified = datemodified;
		//New added for Admin Rights feature.
		pFingerPrint->hasAdminRights = fp.hasAdminRights;
		pFingerPrint->duid = helper::allocStrInComMem(fp.duid);
		// regard rights as bit-enabled one
		DWORD64 rs = 0;
		std::for_each(fp.rights.begin(), fp.rights.end(), [&rs] (SDRmFileRight i) {
			rs |= (DWORD64)i;
		});

		pFingerPrint->rights = rs;
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetNxlFileTagsWithoutToken(
	HANDLE hUser,
	const wchar_t* nxlFilePath,
	wchar_t** pTags) {
	OutputDebugStringA("call SDWL_User_GetNxlFileTagsWithoutToken\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	SDR_NXL_FILE_FINGER_PRINT fp;
	auto rt = g_User->GetFingerPrintWithoutToken(nxlFilePath, fp);
	if (!rt) {
		return rt.GetCode();
	}
	*pTags = helper::allocStrInComMem(fp.tags);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateNxlFileRights(
	HANDLE hUser,
	const wchar_t* nxlFilePath,
	int* pRights, int rightsArrLength,
	WaterMark watermark, Expiration expiration,
	const wchar_t* tags)
{
	OutputDebugStringA("call SDWL_User_UpdateNxlFileRights\n");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	//Get Nxl file path first.
	std::wstring p(nxlFilePath);

	//Padding rights.
	int len = rightsArrLength;
	std::vector<SDRmFileRight> r;
	for (int i = 0; i < len; i++) {
		SDRmFileRight l = (SDRmFileRight)pRights[i];
		r.push_back(l);
	}
	//Padding watermark.
	SDR_WATERMARK_INFO w;
	{
		w.text = helper::utf162utf8(watermark.text);
		w.fontName = helper::utf162utf8(watermark.fontName);
		w.fontColor = helper::utf162utf8(watermark.fontColor);
		w.fontSize = watermark.fontSize;
		w.transparency = watermark.transparency;
		w.rotation = (WATERMARK_ROTATION)watermark.rotation;
		w.repeat = watermark.repeat;
	}
	//Padding expiration.
	SDR_Expiration e;
	{
		e.type = (IExpiryType)expiration.type;
		e.start = expiration.start;
		e.end = expiration.end;
	}
	//Padding tags.
	std::string t(helper::utf162utf8(tags));
	auto rt = g_User->ChangeRightsOfFile(p, r, w, e, t);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateProjectNxlFileRights(
	HANDLE hUser,
	const wchar_t* nxlFilePath,
	const uint32_t projectId,
	const wchar_t* fileName,
	const wchar_t* parentPathId,
	int* rights, int rightsArrLength,
	WaterMark watermark, Expiration expiration,
	const wchar_t* tags) {
	OutputDebugStringA("call SDWL_User_UpdateProjectNxlFileRights");

	// sanity check.
	if (hUser == NULL || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL || fileName == NULL) {
		// missing required params.
		return SDWL_INTERNAL_ERROR;
	}
	std::wstring nxlfp(nxlFilePath);
	UINT32 pId = projectId;
	std::string fName(helper::utf162utf8(fileName));
	std::string parentPId(helper::utf162utf8(parentPathId));

	// padding rights section.
	std::vector<SDRmFileRight> r;
	{
		int len = rightsArrLength;
		for (int i = 0; i < len; i++) {
			r.push_back((SDRmFileRight)rights[i]);
		}
	}

	// padding watermark.
	SDR_WATERMARK_INFO w;
	{
		w.text = helper::utf162utf8(watermark.text);
		w.fontName = helper::utf162utf8(watermark.fontName);
		w.fontColor = helper::utf162utf8(watermark.fontColor);
		w.fontSize = watermark.fontSize;
		w.transparency = watermark.transparency;
		w.rotation = (WATERMARK_ROTATION)watermark.rotation;
		w.repeat = watermark.repeat;
	}

	// padding expiration.
	SDR_Expiration e;
	{
		e.type = (IExpiryType)expiration.type;
		e.start = expiration.start;
		e.end = expiration.end;
	}

	// padding tags.
	std::string t(helper::utf162utf8(tags));

	auto rt = g_User->ChangeRightsOfProjectFile(nxlfp, pId, fName, parentPId, r, w, e, t);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetMyVaultFileMetaData(
	HANDLE hUser,
	const wchar_t* pNxlFilePath,
	const wchar_t* pPathId,
	MYVAULT_FILE_META_DATA* pMetadata) {
	OutputDebugStringA("call SDWL_User_GetMyVaultFileMetaData");
	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (pNxlFilePath == NULL || pPathId == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (pMetadata == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	std::wstring p(pNxlFilePath);
	std::string pid(helper::utf162utf8(pPathId));

	SDR_FILE_META_DATA md;
	auto rt = g_User->GetNXLFileMetadata(p, pid, md);
	if (!rt) {
		return rt.GetCode();
	}

	{
		//Padding metadata.
		pMetadata->name = helper::allocStrInComMem(helper::utf82utf16(md.name));
		pMetadata->fileLink = helper::allocStrInComMem(helper::utf82utf16(md.fileLink));

		pMetadata->lastModify = md.lastmodify;
		pMetadata->protectOn = md.protectedon;
		pMetadata->sharedOn = md.sharedon;
		pMetadata->isShared = md.shared;
		pMetadata->isDeleted = md.deleted;
		pMetadata->isRevoked = md.revoked;
		pMetadata->protectionType = md.protectionType;
		pMetadata->isOwner = md.owner;
		pMetadata->isNxl = md.nxl;

		std::string recipents;
		std::for_each(md.recipients.begin(), md.recipients.end(), [&recipents](std::string i) {
			recipents.append(i).append(";");
		});
		pMetadata->recipents = helper::allocStrInComMem(helper::utf82utf16(recipents));
		pMetadata->pathDisplay = helper::allocStrInComMem(helper::utf82utf16(md.pathDisplay));
		pMetadata->pathId = helper::allocStrInComMem(helper::utf82utf16(md.pathid));
		pMetadata->tags = helper::allocStrInComMem(helper::utf82utf16(md.tags));

		pMetadata->expiration.type = (ExpiryType)md.expiration.type;
		pMetadata->expiration.start = md.expiration.start;
		pMetadata->expiration.end = md.expiration.end;
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyVaultShareFile(
	HANDLE hUser,
	const wchar_t* nxlLocalPath,
	const wchar_t* recipients,
	const wchar_t* repositoryId,
	const wchar_t* fileName,
	const wchar_t* filePathId,
	const wchar_t* filePath,
	const wchar_t* comments) {
	OutputDebugStringA("call SDWL_User_MyVaultShareFile");

	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	if (nxlLocalPath == NULL) {
		// nxl local file not found.
		return SDWL_INTERNAL_ERROR;
	}
	if (recipients == NULL) {
		// recipients must need first.
		return SDWL_INTERNAL_ERROR;
	}
	if (fileName == NULL || filePathId == NULL || filePath == NULL) {
		// params reqired missing.
		return SDWL_INTERNAL_ERROR;
	}

	// prepare params send them to sdk.
	std::wstring nlp(nxlLocalPath);

	std::vector<std::string> rv;
	{
		std::string recipentsStr(helper::utf162utf8(recipients));
		helper::SplitStr(recipentsStr, rv, ",");
	}

	std::string rId(helper::utf162utf8(repositoryId));
	std::string fn(helper::utf162utf8(fileName));
	std::string fpid(helper::utf162utf8(filePathId));
	std::string fp(helper::utf162utf8(filePath));

	if (NULL == comments) {
		comments = L"";
	}
	std::string cmts(helper::utf162utf8(comments));

	auto rt = g_User->ShareFileFromMyVault(nlp, rv, rId, fn, fpid, fp, cmts);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_SharedWithMeReshareFile(
	HANDLE hUser,
	const wchar_t* transactionId,
	const wchar_t* transactionCode,
	const wchar_t* emaillist) {
	OutputDebugStringA("call SDWL_User_SharedWithMeReshareFile");

	// sanity check
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (transactionId == NULL || transactionCode == NULL || emaillist == NULL) {
		// params are required.
		return SDWL_INTERNAL_ERROR;
	}

	std::string tranId(helper::utf162utf8(transactionId));
	std::string tranCode(helper::utf162utf8(transactionCode));
	std::string emails(helper::utf162utf8(emaillist));

	auto rt = g_User->SharedWithMeReShareFile(tranId, tranCode, emails);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ResetSourcePath(
	HANDLE hUser,
	const wchar_t* nxlFilePath,
	const wchar_t* sourcePath) {
	OutputDebugStringA("Call SDWL_User_ResetSourcePath.\n");

	//Sanity check.
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL || sourcePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring nxlpth(nxlFilePath);
	std::wstring srcpth(sourcePath);

	auto rt = g_User->ResetSourcePath(nxlpth, srcpth);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetFileRightsFromCentralPoliciesByTenant(
	HANDLE hUser,
	const wchar_t* tenantName,
	const wchar_t* tags,
	CENTRAL_RIGHTS** pArray,
	uint32_t* pArrSize) {
	OutputDebugStringA("Call SDWL_User_GetFileRightsFromCentralPoliciesByTenant.\n");

	//Sanity check.
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if (tenantName == NULL || tags == NULL) {
		//Params needed.
		return SDWL_INTERNAL_ERROR;
	}

	std::string tn(helper::utf162utf8(tenantName));
	std::string t(helper::utf162utf8(tags));

	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> tmp;
	auto rt = g_User->GetFileRightsFromCentralPolicies(tn, t, tmp);
	if (!rt) {
		return rt.GetCode();
	}

	//Prepare output data.
	{
		const auto pSize = tmp.size();
		if (pSize > 0) {
			CENTRAL_RIGHTS* p = (CENTRAL_RIGHTS*)::CoTaskMemAlloc(sizeof(CENTRAL_RIGHTS)*pSize);
			for (size_t i = 0; i < pSize; i++) {
				//Padding single CENTRAL_RIGHTS.
				p[i].rights = tmp[i].first;

				std::vector<SDR_WATERMARK_INFO> wms = tmp[i].second;
				const auto wmsSize = wms.size();
				if (wmsSize > 0) {
					WaterMark* wm = (WaterMark*)::CoTaskMemAlloc(sizeof(WaterMark)*wmsSize);
					for (size_t j = 0; j < wmsSize; j++) {
						wm[j].text = helper::allocStrInComMem(helper::utf82utf16(wms[j].text));
						wm[j].fontName = helper::allocStrInComMem(helper::utf82utf16(wms[j].fontName));
						wm[j].fontColor = helper::allocStrInComMem(helper::utf82utf16(wms[j].fontColor));
						wm[j].repeat = wms[j].repeat;
						wm[j].fontSize = wms[j].fontSize;
						wm[j].rotation = wms[j].rotation;
						wm[j].transparency = wms[j].transparency;
					}
					p[i].watermarks = wm;
					p[i].watermarkLenth = wmsSize;
				}
				else {
					p[i].watermarks = NULL;
					p[i].watermarkLenth = 0;
				}
			}
			*pArray = p;
			*pArrSize = (uint32_t)pSize;
		}
		else {
			*pArray = NULL;
			*pArrSize = 0;
		}
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetFileRightsFromCentralPolicyByProjectId(
	HANDLE hUser,
	const uint32_t projectId,
	const wchar_t* tags,
	CENTRAL_RIGHTS** pArray,
	uint32_t* pArrSize,
	bool doOwnerCheck) {

	OutputDebugStringA("Call SDWL_User_GetFileRightsFromCentralPolicyByProjectId.\n");

	//Sanity check.
	if (g_User == NULL || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if (tags == NULL) {
		//Params needed.
		return SDWL_INTERNAL_ERROR;
	}

	uint32_t pid = projectId;
	std::string t(helper::utf162utf8(tags));

	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> tmp;
	auto rt = g_User->GetFileRightsFromCentralPolicies(pid, t, tmp, doOwnerCheck);
	if (!rt) {
		return rt.GetCode();
	}

	//Prepare output data.
	{
		const auto pSize = tmp.size();
		if (pSize > 0) {
			CENTRAL_RIGHTS* p = (CENTRAL_RIGHTS*)::CoTaskMemAlloc(sizeof(CENTRAL_RIGHTS)*pSize);
			for (size_t i = 0; i < pSize; i++) {
				//Padding single CENTRAL_RIGHTS.
				p[i].rights = tmp[i].first;

				std::vector<SDR_WATERMARK_INFO> wms = tmp[i].second;
				const auto wmsSize = wms.size();
				if (wmsSize > 0) {
					WaterMark* wm = (WaterMark*)::CoTaskMemAlloc(sizeof(WaterMark)*wmsSize);
					for (size_t j = 0; j < wmsSize; j++) {
						wm[j].text = helper::allocStrInComMem(helper::utf82utf16(wms[j].text));
						wm[j].fontName = helper::allocStrInComMem(helper::utf82utf16(wms[j].fontName));
						wm[j].fontColor = helper::allocStrInComMem(helper::utf82utf16(wms[j].fontColor));
						wm[j].repeat = wms[j].repeat;
						wm[j].fontSize = wms[j].fontSize;
						wm[j].rotation = wms[j].rotation;
						wm[j].transparency = wms[j].transparency;
					}
					p[i].watermarks = wm;
					p[i].watermarkLenth = wmsSize;
				}
				else {
					p[i].watermarks = NULL;
					p[i].watermarkLenth = 0;
				}
			}
			*pArray = p;
			*pArrSize = (uint32_t)pSize;
		}
		else {
			*pArray = NULL;
			*pArrSize = 0;
		}
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProtectFileFrom(HANDLE hUser, const wchar_t* srcplainfile, const wchar_t* origianlnxlfile, wchar_t** output)
{
	OutputDebugStringA("call SDWL_User_ProtectFileFrom\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == srcplainfile || nullptr == origianlnxlfile) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring outputPath;
	auto rt = g_User->ProtectFileFrom(srcplainfile, origianlnxlfile, outputPath);
	if (!rt) {
		return rt.GetCode();
	}
	*output = helper::allocStrInComMem(outputPath);

	return SDWL_SUCCESS;
}

#pragma endregion // User_Base

#pragma region User_MyVault

NXSDK_API DWORD SDWL_User_MyVaultFileIsExist(HANDLE hUser, const wchar_t* pathId, bool* bExist)
{
	OutputDebugStringA("call SDWL_User_MyVaultFileIsExist\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId) {
		return SDWL_INTERNAL_ERROR;
	}

	bool isExist = false;
	auto rt = g_User->MyVaultFileIsExist(helper::utf162utf8(pathId), isExist);
	if (!rt) {
		return rt.GetCode();
	}
	*bExist = isExist;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyVaultGetNxlFileHeader(HANDLE hUser, const wchar_t* pathId, const wchar_t* targetFolder, 
	wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_MyVaultGetNxlFileHeader\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId || nullptr == targetFolder) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string target = helper::utf162utf8(targetFolder);
	auto rt = g_User->MyVaultGetNxlFileHeader(helper::utf162utf8(pathId), target);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(helper::utf82utf16(target));

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UploadMyVaultFile(HANDLE hUser, const wchar_t * nxlFilePath, 
	const wchar_t* sourcePath, const wchar_t* recipients, const wchar_t* comments, bool bOverwrite)
{
	OutputDebugStringA("call SDWL_User_UploadMyVaultFile\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nxlFilePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	DWORD rt2 = SDWL_User_UploadFile(hUser, nxlFilePath, sourcePath, recipients, comments, bOverwrite);

	return rt2;
}

NXSDK_API DWORD SDWL_User_ListMyVaultAllFiles(HANDLE hUser,
	const wchar_t * orderBy, const wchar_t * searchString,
	MyVaultFileInfo ** ppFiles, uint32_t * psize)
{
	OutputDebugStringA("call SDWL_User_ListMyVaultAllFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<SDR_MYVAULT_FILE_INFO> vec;
	std::vector<SDR_MYVAULT_FILE_INFO> pageVec;
	uint32_t pageId = 1;
	uint32_t pageSize = 1000;
	do
	{
		pageVec.clear();

		auto rt = g_User->GetMyVaultFiles(pageId, pageSize,
			helper::utf162utf8(orderBy),
			helper::utf162utf8(searchString),
			pageVec);
		if (!rt) {
			return rt.GetCode();
		}

		for (auto i : pageVec)
		{
			vec.push_back(i);
		}

	} while (pageVec.size() == pageSize && pageId++);

	auto size = vec.size();
	*psize = (uint32_t)size;
	if (*psize == 0) {
		*ppFiles = NULL;
		return SDWL_SUCCESS;
	}

	//
	MyVaultFileInfo* p = (MyVaultFileInfo*)::CoTaskMemAlloc(sizeof(MyVaultFileInfo)*size);

	for (size_t i = 0; i < size; i++)
	{
		SDR_MYVAULT_FILE_INFO pif = vec[i];

		p[i].pathId = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathid));
		p[i].displayPath = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathdisplay));
		p[i].repoId = helper::allocStrInComMem(helper::utf82utf16(pif.m_repoid));
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(pif.m_duid));
		p[i].nxlName = helper::allocStrInComMem(helper::utf82utf16(pif.m_nxlname));
		//
		p[i].lastModifiedTime = pif.m_lastmodified;
		p[i].creationTime = pif.m_creationtime;
		p[i].sharedTime = pif.m_sharedon;
		p[i].sharedWithList = helper::allocStrInComMem(helper::utf82utf16(pif.m_sharedwith));
		p[i].size = pif.m_size;
		//
		p[i].is_deleted = pif.m_bdeleted;
		p[i].is_revoked = pif.m_brevoked;
		p[i].is_shared = pif.m_bshared;
		//
		p[i].source_repo_type = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcerepotype));
		p[i].source_file_displayPath = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcefilepathdisplay));
		p[i].source_file_pathId = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcefilepathid));
		p[i].source_repo_name = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcereponame));
		p[i].source_repo_id = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcerepoid));

	}
	*ppFiles = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ListMyVaultFiles(HANDLE hUser,
	uint32_t pageId, uint32_t pageSize,
	const wchar_t * orderBy, const wchar_t * searchString,
	MyVaultFileInfo ** ppFiles, uint32_t * psize)
{
	OutputDebugStringA("call SDWL_User_ListMyVaultFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	std::vector<SDR_MYVAULT_FILE_INFO> vec;
	auto rt = g_User->GetMyVaultFiles(pageId, pageSize,
		helper::utf162utf8(orderBy),
		helper::utf162utf8(searchString),
		vec);
	if (!rt) {
		return rt.GetCode();
	}
	auto size = vec.size();
	*psize = (uint32_t)size;
	if (size == 0) {
		*ppFiles = NULL;
		return SDWL_SUCCESS;
	}
	//
	MyVaultFileInfo* p = (MyVaultFileInfo*)::CoTaskMemAlloc(sizeof(MyVaultFileInfo)*size);

	for (size_t i = 0; i < size; i++)
	{
		SDR_MYVAULT_FILE_INFO pif = vec[i];

		p[i].pathId = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathid));
		p[i].displayPath = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathdisplay));
		p[i].repoId = helper::allocStrInComMem(helper::utf82utf16(pif.m_repoid));
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(pif.m_duid));
		p[i].nxlName = helper::allocStrInComMem(helper::utf82utf16(pif.m_nxlname));
		//
		p[i].lastModifiedTime = pif.m_lastmodified;
		p[i].creationTime = pif.m_creationtime;
		p[i].sharedTime = pif.m_sharedon;
		p[i].sharedWithList = helper::allocStrInComMem(helper::utf82utf16(pif.m_sharedwith));
		p[i].size = pif.m_size;
		//
		p[i].is_deleted = pif.m_bdeleted;
		p[i].is_revoked = pif.m_brevoked;
		p[i].is_shared = pif.m_bshared;
		//
		p[i].source_repo_type = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcerepotype));
		p[i].source_file_displayPath = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcefilepathdisplay));
		p[i].source_file_pathId = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcefilepathid));
		p[i].source_repo_name = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcereponame));
		p[i].source_repo_id = helper::allocStrInComMem(helper::utf82utf16(pif.m_sourcerepoid));

	}
	*ppFiles = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_DownloadMyVaultFiles(HANDLE hUser,
	const wchar_t * rmsFilePathId,
	const wchar_t * downloadLocalFolder,
	Type_DownlaodFile type,
	const wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_DownloadMyVaultFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (rmsFilePathId == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (downloadLocalFolder == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	auto utf8Pathid = helper::utf162utf8(rmsFilePathId);
	std::wstring destfolder = downloadLocalFolder;
	auto rt = g_User->MyVaultDownloadFile(utf8Pathid, destfolder, type);
	if (!rt) {
		return rt.GetCode();
	}

	*outPath = helper::allocStrInComMem(destfolder);

	return SDWL_SUCCESS;

}


#pragma endregion // User_MyVault

#pragma region User_myDrive
NXSDK_API DWORD SDWL_User_MyDriveListFiles(
	HANDLE hUser,
	const wchar_t* pathId,
	MyDriveFileInfo** pFileList,
	uint32_t* pLen)
{
	OutputDebugStringA("SDWL_User_MyDriveListFiles \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId) {
		return SDWL_INTERNAL_ERROR;
	}

	//std::string encodedPathId = helper::UrlEncode(helper::utf162utf8(pathId));

	std::vector<SDR_MYDRIVE_FILE_INFO> vec;
	auto rt = g_User->MyDriveListFiles(pathId, vec); // helper::utf82utf16(encodedPathId)
	if (!rt) {
		return rt.GetCode();
	}

	int size = (size_t)vec.size();
	*pLen = size;
	if (size == 0) {
		*pFileList = nullptr;
		return SDWL_SUCCESS;
	}

	MyDriveFileInfo* pf = (MyDriveFileInfo*)::CoTaskMemAlloc(sizeof(MyDriveFileInfo) * size);
	for (int i = 0; i < size; i++) {
		pf[i].pathId = helper::allocStrInComMem(helper::utf82utf16(vec[i].m_pathid));
		pf[i].pathDisplay = helper::allocStrInComMem(helper::utf82utf16(vec[i].m_pathdisplay));
		pf[i].name = helper::allocStrInComMem(helper::utf82utf16(vec[i].m_name));
		pf[i].lastModified = vec[i].m_lastmodified;
		pf[i].size = vec[i].m_size;
		pf[i].isFolder = vec[i].m_bfolder ? 1 : 0;
	}

	*pFileList = pf;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyDriveDownloadFile(
	HANDLE hUser,
	const wchar_t* pathId,
	const wchar_t* targetFolder,
    wchar_t** outPath) 
{
	OutputDebugStringA("SDWL_User_MyDriveDownloadFile \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId || nullptr == targetFolder) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring target(targetFolder);
	auto rt = g_User->MyDriveDownloadFile(pathId, target);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(target);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyDriveUploadFile(
	HANDLE hUser,
	const wchar_t* pathId,
	const wchar_t* destFolder,
	bool overwrite) 
{
	OutputDebugStringA("SDWL_User_MyDriveUploadFile \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId || nullptr == destFolder) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_User->MyDriveUploadFile(pathId, destFolder, overwrite);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyDriveCreateFolder(
	HANDLE hUser,
	const wchar_t* name,
	const wchar_t* parentFolder)
{
	OutputDebugStringA("SDWL_User_MyDriveCreateFolder \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == name || nullptr == parentFolder) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_User->MyDriveCreateFolder(name, parentFolder);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API WORD SDWL_User_MyDriveDeleteItem(
	HANDLE hUser,
	const wchar_t* pathId) 
{
	OutputDebugStringA("SDWL_User_MyDriveDeleteItem \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_User->MyDriveDeleteItem(pathId);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyDriveCopyItem(
	HANDLE hUser,
	const wchar_t* srcPathId,
	const wchar_t* destPathId) 
{
	OutputDebugStringA("SDWL_User_MyDriveCopyItem \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == srcPathId || nullptr == destPathId) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_User->MyDriveCopyItem(srcPathId, destPathId);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyDriveMoveItem(
	HANDLE hUser,
	const wchar_t* srcPathId,
	const wchar_t* destPathId)
{
	OutputDebugStringA("SDWL_User_MyDriveMoveItem \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == srcPathId || nullptr == destPathId) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_User->MyDriveMoveItem(srcPathId, destPathId);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_MyDriveCreateShareURL(
	HANDLE hUser,
	const wchar_t* pathId,
	wchar_t** outSharedURL) 
{
	OutputDebugStringA("SDWL_User_MyDriveCreateShareURL \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId) {
		return SDWL_INTERNAL_ERROR;
	}
	std::wstring outUrl;
	auto rt = g_User->MyDriveCreateShareURL(pathId, outUrl);
	if (!rt) {
		return rt.GetCode();
	}
	*outSharedURL = helper::allocStrInComMem(outUrl);

	return SDWL_SUCCESS;
}

#pragma endregion // User_myDrive

#pragma region User_WorkSpace

NXSDK_API DWORD SDWL_User_WorkSpaceFileIsExist(HANDLE hUser, const wchar_t* pathId, bool* bExist)
{
	OutputDebugStringA("call SDWL_User_WorkSpaceFileIsExist\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId) {
		return SDWL_INTERNAL_ERROR;
	}

	bool isExist = false;
	auto rt = g_User->WorkspaceFileIsExist(helper::utf162utf8(pathId), isExist);
	if (!rt) {
		return rt.GetCode();
	}
	*bExist = isExist;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_WorkSpaceGetNxlFileHeader(HANDLE hUser, const wchar_t* pathId,
	const wchar_t* targetFolder, wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_WorkSpaceGetNxlFileHeader\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId || nullptr == targetFolder) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string target = helper::utf162utf8(targetFolder);
	auto rt = g_User->WorkspaceGetNxlFileHeader(helper::utf162utf8(pathId), target);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(helper::utf82utf16(target));

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_WorkSpaceFileOverwrite(HANDLE hUser, const wchar_t* parentPathid,
	const wchar_t* nxlFilePath, bool bOverwrite)
{
	OutputDebugStringA("call SDWL_User_WorkSpaceFileOverwrite\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == parentPathid || nullptr == nxlFilePath) {
		return SDWL_INTERNAL_ERROR;
	}

	// open the file first
	ISDRmNXLFile* pFile = nullptr;
	auto rt = g_User->OpenFile(nxlFilePath, &pFile);
	if (!rt) {
		if (pFile != nullptr) {
			g_User->CloseFile(pFile);
		}
		return rt.GetCode();
	}

	auto rt2 = g_User->WorkspaceFileOverwrite(parentPathid, pFile, bOverwrite);
	// close file
	if (pFile != nullptr) {
		g_User->CloseFile(pFile);
	}

	if (!rt2) {
		return rt2.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_DownloadWorkSpaceFile(HANDLE hUser,
	const wchar_t* rmsPathId,
	const wchar_t* downloadLocalFolder,
	Type_DownlaodFile type,
	const wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_DownloadWorkSpaceFile \n");
	// sanity check
	if (NULL == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (NULL == rmsPathId || NULL == downloadLocalFolder) {
		return SDWL_INTERNAL_ERROR;
	}

	auto utf8Pathid = helper::utf162utf8(rmsPathId);
	std::wstring destfolder = downloadLocalFolder;
	auto rt = g_User->WorkspaceDownloadFile(utf8Pathid, destfolder, type);
	if (!rt)
	{
		return rt.GetCode();
	}

	*outPath = helper::allocStrInComMem(destfolder);
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ListWorkSpaceAllFiles(HANDLE hUser,
	const wchar_t* pathId,
	const wchar_t* orderBy,
	const wchar_t* searchString,
	WorkSpaceFileInfo** ppFiles,
	uint32_t* psize)
{
	OutputDebugStringA("call SDWL_User_ListWorkSpaceAllFiles \n");
	// sanity check
	if (NULL == g_User || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (NULL == pathId) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string encodedPathId = helper::UrlEncode(helper::utf162utf8(pathId));

	std::vector<SDR_WORKSPACE_FILE_INFO> vec;
	std::vector<SDR_WORKSPACE_FILE_INFO> pageVec;
	uint32_t pageId = 1;
	uint32_t pageSize = 1000;

	do {
		pageVec.clear();
		auto rt = g_User->GetWorkspaceFiles(pageId, pageSize, encodedPathId, helper::utf162utf8(orderBy),
			helper::utf162utf8(searchString), pageVec);
		if (!rt) {
			return rt.GetCode();
		}

		for (auto i : pageVec) {
			vec.push_back(i);
		}

	} while (pageVec.size() == pageSize && pageId++);

	*psize = (uint32_t)vec.size();
	if (*psize == 0) {
		*ppFiles = NULL;
		return SDWL_SUCCESS;
	}

	WorkSpaceFileInfo* p = (WorkSpaceFileInfo*)::CoTaskMemAlloc((*psize) * sizeof(WorkSpaceFileInfo));
	for (uint32_t i = 0; i < (*psize); i++) {
		auto pif = vec[i];
		p[i].fileId = helper::allocStrInComMem(helper::utf82utf16(pif.m_fileid));
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(pif.m_duid));
		p[i].pathDisplay = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathdisplay));
		p[i].pathId = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathid));
		p[i].nxlName = helper::allocStrInComMem(helper::utf82utf16(pif.m_nxlname));
		p[i].fileType = helper::allocStrInComMem(helper::utf82utf16(pif.m_filetype));
		p[i].lastModified = pif.m_lastmodified;
		p[i].created = pif.m_created;
		p[i].size = pif.m_size;
		p[i].isFolder = pif.m_bfolder;
		p[i].ownerId = pif.m_ownerid;
		p[i].ownerDisplayName = helper::allocStrInComMem(helper::utf82utf16(pif.m_ownerdisplayname));
		p[i].ownerEmail = helper::allocStrInComMem(helper::utf82utf16(pif.m_owneremail));
		p[i].modifiedBy = pif.m_modifiedby;
		p[i].modifedByName = helper::allocStrInComMem(helper::utf82utf16(pif.m_modifiedbyname));
		p[i].modifedByEmail = helper::allocStrInComMem(helper::utf82utf16(pif.m_modifiedbyemail));
	}

	*ppFiles = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateWorkSpaceNxlFileRights(HANDLE hUser,
	const wchar_t* oldNxlFilePath,
	const wchar_t* name,
	const wchar_t* parentPathId,
	int* rights,
	int rightsArrLength,
	WaterMark watermark,
	Expiration expiration,
	const wchar_t* tags)
{
	OutputDebugStringA("call SDWL_User_UpdateWorkSpaceNxlFileRights\n");
	// sanity check
	if (NULL == g_User || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (NULL == oldNxlFilePath || NULL == name || NULL == parentPathId) {
		return SDWL_INTERNAL_ERROR;
	}

	// padding rights
	int len = rightsArrLength;
	std::vector<SDRmFileRight> r;
	for (int i = 0; i < len; i++) {
		SDRmFileRight l = (SDRmFileRight)rights[i];
		r.push_back(l);
	}

	// padding watermark
	SDR_WATERMARK_INFO w;
	{
		w.text = helper::utf162utf8(watermark.text);
		w.fontName = helper::utf162utf8(watermark.fontName);
		w.fontColor = helper::utf162utf8(watermark.fontColor);
		w.fontSize = watermark.fontSize;
		w.transparency = watermark.transparency;
		w.rotation = (WATERMARK_ROTATION)watermark.rotation;
		w.repeat = watermark.repeat;
	}

	//padding expiration
	SDR_Expiration e;
	{
		e.type = (IExpiryType)expiration.type;
		e.start = expiration.start;
		e.end = expiration.end;
	}

	// padding tags
	std::string t(helper::utf162utf8(tags));

	std::wstring filepath(oldNxlFilePath);
	auto rt = g_User->ChangeRightsOfWorkspaceFile(filepath,
		helper::utf162utf8(name),
		helper::utf162utf8(parentPathId),
		r, w, e, t);

	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UploadWorkSpaceFile(HANDLE hUser,
	const wchar_t* destFolder,
	const wchar_t * nxlFilePath,
	bool overWrite)
{
	OutputDebugStringA("call SDWL_User_UploadWorkSpaceFile\n");
	// sanity check
	if (NULL == g_User || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (NULL == destFolder || NULL == nxlFilePath) {
		return SDWL_INTERNAL_ERROR;
	}

	// open the nxl file first
	ISDRmNXLFile* pFile = NULL;
	auto rt = g_User->OpenFile(nxlFilePath, &pFile);
	if (!rt) {
		if (pFile != NULL) {
			g_User->CloseFile(pFile);
		}
		return rt.GetCode();
	}

	auto rt2 = g_User->UploadWorkspaceFile(destFolder, pFile, overWrite);
	// Close file
	if (pFile != NULL) {
		g_User->CloseFile(pFile);
	}

	if (!rt2) {
		return rt2.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetWorkSpaceFileMetadata(HANDLE hUser,
	const wchar_t* pathId,
	WORKSPACE_FILE_META_DATA* pmetadata)
{
	OutputDebugStringA("call SDWL_User_GetWorkSpaceFileMetadata\n");
	// sanity check
	if (NULL == g_User || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (NULL == pathId || NULL == pmetadata) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string p(helper::utf162utf8(pathId));
	SDR_FILE_META_DATA md;
	auto rt = g_User->GetWorkspaceFileMetadata(p, md);
	if (!rt) {
		return rt.GetCode();
	}

	// padding metadata
	{
		pmetadata->name = helper::allocStrInComMem(helper::utf82utf16(md.name));
		pmetadata->fileLink = helper::allocStrInComMem(helper::utf82utf16(md.fileLink));
		//
		pmetadata->lastModify = md.lastmodify;
		pmetadata->protectOn = md.protectedon;
		pmetadata->sharedOn = md.sharedon;
		pmetadata->isShared = md.shared;
		pmetadata->isDeleted = md.deleted;
		pmetadata->isRevoked = md.revoked;
		pmetadata->protectionType = md.protectionType;
		pmetadata->isOwner = md.owner;
		pmetadata->isNxl = md.nxl;
		// recipients
		std::string recipients;
		std::for_each(md.recipients.begin(), md.recipients.end(), [&recipients](std::string i) {
			recipients.append(i).append(";");
		});
		pmetadata->recipents = helper::allocStrInComMem(helper::utf82utf16(recipients));
		pmetadata->pathDisplay = helper::allocStrInComMem(helper::utf82utf16(md.pathDisplay));
		pmetadata->pathId = helper::allocStrInComMem(helper::utf82utf16(md.pathid));
		pmetadata->tags = helper::allocStrInComMem(helper::utf82utf16(md.tags));

		pmetadata->expiration.type = (ExpiryType)md.expiration.type;
		pmetadata->expiration.start = md.expiration.start;
		pmetadata->expiration.end = md.expiration.end;
	}

	return SDWL_SUCCESS;
}
#pragma endregion // User_WorkSpace

#pragma region User_Project

NXSDK_API DWORD SDWL_User_ProjectFileIsExist(HANDLE hUser, uint32_t projectId, const wchar_t* pathId, bool* bExist)
{
	OutputDebugStringA("call SDWL_User_ProjectFileIsExist\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId) {
		return SDWL_INTERNAL_ERROR;
	}

	bool isExist = false;
	auto rt = g_User->ProjectFileIsExist(projectId, helper::utf162utf8(pathId), isExist);
	if (!rt) {
		return rt.GetCode();
	}
	*bExist = isExist;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectGetNxlFileHeader(HANDLE hUser, uint32_t projectId, const wchar_t* pathId,
	const wchar_t* targetFolder, wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_ProjectGetNxlFileHeader\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == pathId || nullptr == targetFolder) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string target = helper::utf162utf8(targetFolder);
	auto rt = g_User->ProjectGetNxlFileHeader(projectId, helper::utf162utf8(pathId), target);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(helper::utf82utf16(target));

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectFileOverwrite(HANDLE hUser, uint32_t projectId, const wchar_t* parentPathid,
	const wchar_t* nxlFilePath, bool bOverwrite)
{
	OutputDebugStringA("call SDWL_User_ProjectFileOverwrite\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == parentPathid || nullptr == nxlFilePath) {
		return SDWL_INTERNAL_ERROR;
	}

	// open the file first
	ISDRmNXLFile* pFile = nullptr;
	auto rt = g_User->OpenFile(nxlFilePath, &pFile);
	if (!rt) {
		if (pFile != nullptr) {
			g_User->CloseFile(pFile);
		}
		return rt.GetCode();
	}

	auto rt2 = g_User->ProjectFileOverwrite(projectId, parentPathid, pFile, bOverwrite);
	// close file
	if (pFile != nullptr) {
		g_User->CloseFile(pFile);
	}

	if (!rt2) {
		return rt2.GetCode();
	}

	return SDWL_SUCCESS;
}


NXSDK_API DWORD SDWL_User_GetListProjtects(HANDLE hUser, 
	uint32_t pagedId, uint32_t pageSize, 
	const wchar_t * orderBy, ProjtectFilter filter)
{
	OutputDebugStringA("call SDWL_User_GetListProjtects\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_User->GetListProjects(pagedId, pageSize, 
		 helper::utf162utf8(orderBy), 
		(RM_ProjectFilter)filter);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectDownloadFile(HANDLE hUser, 
	uint32_t projectId, 
	const wchar_t * pathId, 
	const wchar_t * downloadPath, 
	Type_DownlaodFile type, const wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_ProjectDownloadFile\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (pathId == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (downloadPath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (outPath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	//
	auto rmsPath = helper::utf162utf8(pathId);

	std::wstring destfolder = downloadPath;
	auto rt = g_User->ProjectDownloadFile(projectId, rmsPath, destfolder, (RM_ProjectFileDownloadType)type);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(destfolder);

	return SDWL_SUCCESS;

}

NXSDK_API DWORD SDWL_User_ProjectListAllFiles(HANDLE hUser,
	uint32_t projectId,
	const wchar_t * orderby,
	const wchar_t * pathId,
	const wchar_t * searchStr,
	ProjectFileInfo ** pplistFiles,
	uint32_t* plistSize)
{
	OutputDebugStringA("call SDWL_User_ProjectListAllFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	// For fix bug 52592 that the full path contains Chinese character folder.
	std::string encodedPathId = helper::UrlEncode(helper::utf162utf8(pathId));

	std::vector<SDR_PROJECT_FILE_INFO> vec;
	std::vector<SDR_PROJECT_FILE_INFO> pageVec;
	uint32_t pagedId = 1;
	uint32_t pageSize = 1000;
	do
	{
		pageVec.clear();

		auto rt = g_User->GetProjectListFiles(projectId, pagedId, pageSize,
			helper::utf162utf8(orderby),
			encodedPathId,
			helper::utf162utf8(searchStr), pageVec);
		if (!rt) {
			return rt.GetCode();
		}

		for (auto i : pageVec)
		{
			vec.push_back(i);
		}
	} while (pageVec.size() == pageSize && pagedId++);

	auto size = vec.size();
	*plistSize = (uint32_t)size;
	if (size == 0) {
		*pplistFiles = NULL;
		return SDWL_SUCCESS;
	}

	ProjectFileInfo* p = (ProjectFileInfo*)::CoTaskMemAlloc(size * sizeof(ProjectFileInfo));
	for (int i = 0; i < size; i++) {
		auto pif = vec[i];
		p[i].id = helper::allocStrInComMem(helper::utf82utf16(pif.m_fileid));
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(pif.m_duid));
		p[i].displayPath = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathdisplay));
		p[i].pathId = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathid));
		p[i].nxlName = helper::allocStrInComMem(helper::utf82utf16(pif.m_nxlname));
		// new added , as RMS defined,each timestamp will use millseconds,but we will use second
		p[i].lastModifiedTime = pif.m_lastmodified;
		p[i].creationTime = pif.m_created;
		p[i].filesize = pif.m_size;
		p[i].isFolder = pif.m_bfolder;
		p[i].ownerId = pif.m_ownerid;
		p[i].ownerDisplayName = helper::allocStrInComMem(helper::utf82utf16(pif.m_ownerdisplayname));
		p[i].ownerEmail = helper::allocStrInComMem(helper::utf82utf16(pif.m_owneremail));
	}
	*pplistFiles = p;
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectListFiles(HANDLE hUser,
	uint32_t projectId, 
	uint32_t pagedId, 
	uint32_t pageSize, 
	const wchar_t * orderby, 
	const wchar_t * pathId,
	const wchar_t * searchStr,
	ProjectFileInfo ** pplistFiles, 
	uint32_t* plistSize)
{
	OutputDebugStringA("call SDWL_User_ProjectListFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}

	// For fix bug 52592 that the full path contains Chinese character folder.
	std::string encodedPathId = helper::UrlEncode(helper::utf162utf8(pathId));

	std::vector<SDR_PROJECT_FILE_INFO> vec;

	auto rt = g_User->GetProjectListFiles(projectId, pagedId, pageSize, 
		helper::utf162utf8(orderby),
		encodedPathId, 
		helper::utf162utf8(searchStr), vec);
	if (!rt) {
		return rt.GetCode();
	}

	int size = (int)vec.size();
	*plistSize = (uint32_t)size;
	if (size == 0) {
		*pplistFiles = NULL;
		return SDWL_SUCCESS;
	}

	ProjectFileInfo* p = (ProjectFileInfo*)::CoTaskMemAlloc(size * sizeof(ProjectFileInfo));
	for (int i = 0; i < size; i++) {
		auto pif = vec[i];
		p[i].id = helper::allocStrInComMem(helper::utf82utf16(pif.m_fileid));
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(pif.m_duid));
		p[i].displayPath = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathdisplay));
		p[i].pathId = helper::allocStrInComMem(helper::utf82utf16(pif.m_pathid));
		p[i].nxlName = helper::allocStrInComMem(helper::utf82utf16(pif.m_nxlname));
		// new added , as RMS defined,each timestamp will use millseconds,but we will use second
		p[i].lastModifiedTime = pif.m_lastmodified;
		p[i].creationTime = pif.m_created;
		p[i].filesize = pif.m_size;
		p[i].isFolder = pif.m_bfolder;
		p[i].ownerId = pif.m_ownerid;
		p[i].ownerDisplayName= helper::allocStrInComMem(helper::utf82utf16(pif.m_ownerdisplayname));
		p[i].ownerEmail= helper::allocStrInComMem(helper::utf82utf16(pif.m_owneremail));
	}
	*pplistFiles = p;
	return SDWL_SUCCESS;
}

// New interface
NXSDK_API DWORD SDWL_User_ProjectUploadFileEx(HANDLE hUser,
	uint32_t projectid,
	const wchar_t* rmsDestFolder,
	const wchar_t* nxlFilePath,
	int uploadType,
	bool userConfirmedFileOverwrite)
{
	OutputDebugStringA("call SDWL_User_ProjectUploadFileEx\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (rmsDestFolder == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	// open the file first
	ISDRmNXLFile *pFile = NULL;
	auto rt = g_User->OpenFile(nxlFilePath, &pFile);
	if (!rt) {
		if (pFile != NULL) {
			g_User->CloseFile(pFile);
		}
		return rt.GetCode();
	}

	auto rt2 = g_User->UploadProjectFile(projectid, rmsDestFolder, pFile, uploadType, userConfirmedFileOverwrite);

	if (pFile != NULL) {
		g_User->CloseFile(pFile);
	}

	if (!rt2) {
		return rt2.GetCode();
	}
	return SDWL_SUCCESS;
}

#pragma endregion // User_Project

#pragma region Sharing transaction for Project

NXSDK_API DWORD SDWL_User_ProjectListFile(HANDLE hUser,
	uint32_t projectId,
	uint32_t pageId,
	uint32_t pageSize,
	const wchar_t* orderBy,
	const wchar_t* pathId,
	const wchar_t* searchStr,
	FilterType type,
	ProjectSharedFile** ppListFiles,
	uint32_t* pListSize)
{
	OutputDebugStringA("SDWL_User_ProjectListFile \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string encodedPathId = helper::UrlEncode(helper::utf162utf8(pathId));

	std::vector<SDR_PROJECT_SHARED_FILE> vecFiles;
	auto rt = g_User->ProjectListFile(projectId, pageId, pageSize,
		helper::utf162utf8(orderBy),
		encodedPathId,
		helper::utf162utf8(searchStr),
		(RM_FILTER_TYPE)type,
		vecFiles);
	if (!rt) {
		return rt.GetCode();
	}

	int size = (size_t)vecFiles.size();
	*pListSize = size;
	if (size == 0) {
		*ppListFiles = NULL;
		return SDWL_SUCCESS;
	}

	ProjectSharedFile* p = (ProjectSharedFile*)::CoTaskMemAlloc(size * sizeof(ProjectSharedFile));
	for (int i = 0; i < size; i++) {
		auto f = vecFiles[i];
		p[i].id = helper::allocStrInComMem(helper::utf82utf16(f.m_id));
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(f.m_duid));
		p[i].pathDisplay = helper::allocStrInComMem(helper::utf82utf16(f.m_pathdisplay));
		p[i].pathId = helper::allocStrInComMem(helper::utf82utf16(f.m_pathid));
		p[i].name = helper::allocStrInComMem(helper::utf82utf16(f.m_name));
		p[i].fileType = helper::allocStrInComMem(helper::utf82utf16(f.m_filetype));
		p[i].lastModified = f.m_lastmodified;
		p[i].creationTime = f.m_creationtime;
		p[i].size = f.m_size;
		//
		p[i].isFolder = f.m_folder;
		p[i].isShared = f.m_isshared;
		p[i].isRevoked = f.m_revoked;
		//
		p[i].owner.userId = f.m_owner.m_userid;
		p[i].owner.displayName = helper::allocStrInComMem(helper::utf82utf16(f.m_owner.m_displayname));
		p[i].owner.email = helper::allocStrInComMem(helper::utf82utf16(f.m_owner.m_email));
		//
		p[i].lastModifiedUser.userId = f.m_lastmodifieduser.m_userid;
		p[i].lastModifiedUser.displayName = helper::allocStrInComMem(helper::utf82utf16(f.m_lastmodifieduser.m_displayname));
		p[i].lastModifiedUser.email = helper::allocStrInComMem(helper::utf82utf16(f.m_lastmodifieduser.m_email));
		// 
		std::string sharedWithProjects;
		for (int i = 0; i < f.m_sharewithproject.size(); i++) {
			if (i == 0) {
				sharedWithProjects += f.m_sharewithproject[i];
			}
			else {
				sharedWithProjects += ";";
				sharedWithProjects += f.m_sharewithproject[i];
			}
		}
		p[i].sharedWithProject = helper::allocStrInComMem(helper::utf82utf16(sharedWithProjects));
	}
	*ppListFiles = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectListTotalFile(HANDLE hUser,
	uint32_t projectId,
	const wchar_t* orderBy,
	const wchar_t* pathId,
	const wchar_t* searchStr,
	FilterType type,
	ProjectSharedFile** ppListFiles,
	uint32_t* pListSize)
{
	OutputDebugStringA("SDWL_User_ProjectListTotalFile \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string encodedPathId = helper::UrlEncode(helper::utf162utf8(pathId));

	std::vector<SDR_PROJECT_SHARED_FILE> totalVec;
	std::vector<SDR_PROJECT_SHARED_FILE> pageVec;
	uint32_t pagedId = 1;
	uint32_t pageSize = 1000;
	
	do 
	{
		pageVec.clear();

		auto rt = g_User->ProjectListFile(projectId, pagedId, pageSize,
			helper::utf162utf8(orderBy),
			encodedPathId,
			helper::utf162utf8(searchStr),
			(RM_FILTER_TYPE)type,
			pageVec);
		if (!rt) {
			return rt.GetCode();
		}

		for (auto one : pageVec) {
			totalVec.push_back(one);
		}

	} while (pageVec.size() == pageSize && pagedId++);

	int size = (size_t)totalVec.size();
	*pListSize = size;
	if (size == 0) {
		*ppListFiles = NULL;
		return SDWL_SUCCESS;
	}

	ProjectSharedFile* p = (ProjectSharedFile*)::CoTaskMemAlloc(size * sizeof(ProjectSharedFile));
	for (int i = 0; i < size; i++) {
		auto f = totalVec[i];
		p[i].id = helper::allocStrInComMem(helper::utf82utf16(f.m_id));
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(f.m_duid));
		p[i].pathDisplay = helper::allocStrInComMem(helper::utf82utf16(f.m_pathdisplay));
		p[i].pathId = helper::allocStrInComMem(helper::utf82utf16(f.m_pathid));
		p[i].name = helper::allocStrInComMem(helper::utf82utf16(f.m_name));
		p[i].fileType = helper::allocStrInComMem(helper::utf82utf16(f.m_filetype));
		p[i].lastModified = f.m_lastmodified;
		p[i].creationTime = f.m_creationtime;
		p[i].size = f.m_size;
		//
		p[i].isFolder = f.m_folder;
		p[i].isShared = f.m_isshared;
		p[i].isRevoked = f.m_revoked;
		//
		p[i].owner.userId = f.m_owner.m_userid;
		p[i].owner.displayName = helper::allocStrInComMem(helper::utf82utf16(f.m_owner.m_displayname));
		p[i].owner.email = helper::allocStrInComMem(helper::utf82utf16(f.m_owner.m_email));
		//
		p[i].lastModifiedUser.userId = f.m_lastmodifieduser.m_userid;
		p[i].lastModifiedUser.displayName = helper::allocStrInComMem(helper::utf82utf16(f.m_lastmodifieduser.m_displayname));
		p[i].lastModifiedUser.email = helper::allocStrInComMem(helper::utf82utf16(f.m_lastmodifieduser.m_email));
		// 
		std::string sharedWithProjects;
		for (int i = 0; i < f.m_sharewithproject.size(); i++) {
			if (i == 0) {
				sharedWithProjects += std::to_string(f.m_sharewithproject[i]);
			}
			else {
				sharedWithProjects += ";";
				sharedWithProjects += std::to_string(f.m_sharewithproject[i]);
			}
		}
		p[i].sharedWithProject = helper::allocStrInComMem(helper::utf82utf16(sharedWithProjects));
	}
	*ppListFiles = p;

	return SDWL_SUCCESS;

}

NXSDK_API DWORD SDWL_User_ProjectListSharedWithMeFiles(HANDLE hUser,
	uint32_t projectId,
	uint32_t pageId,
	uint32_t pageSize,
	const wchar_t* orderBy,
	const wchar_t* searchStr,
	ProjectSharedWithMeFile** ppListFiles,
	uint32_t* pListSize)
{
	OutputDebugStringA("SDWL_User_ProjectListSharedWithMeFiles \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<SDR_PROJECT_SHAREDWITHME_FILE> vecFiles;
	auto rt = g_User->ProjectListSharedWithMeFiles(projectId, pageId, pageSize,
		helper::utf162utf8(orderBy), 
		helper::utf162utf8(searchStr),
		vecFiles);
	if (!rt) {
		return rt.GetCode();
	}

	int size = vecFiles.size();
	*pListSize = size;
	if (size == 0) {
		*ppListFiles = NULL;
		return SDWL_SUCCESS;
	}

	ProjectSharedWithMeFile* p = (ProjectSharedWithMeFile*)::CoTaskMemAlloc(size * sizeof(ProjectSharedWithMeFile));
	for (int i = 0; i < size; i++) {
		auto f = vecFiles[i];
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(f.m_duid));
		p[i].name = helper::allocStrInComMem(helper::utf82utf16(f.m_name));
		p[i].fileType = helper::allocStrInComMem(helper::utf82utf16(f.m_filetype));
		p[i].size = f.m_size;
		p[i].sharedDate = f.m_shareddate;
		p[i].sharedBy = helper::allocStrInComMem(helper::utf82utf16(f.m_sharedby));
		p[i].transactionId = helper::allocStrInComMem(helper::utf82utf16(f.m_transactionid));
		p[i].transactionCode = helper::allocStrInComMem(helper::utf82utf16(f.m_transactioncode));
		p[i].comment = helper::allocStrInComMem(helper::utf82utf16(f.m_comment));
		p[i].isOwner = f.m_isowner;
		p[i].protectType = f.m_protectedtype;
		p[i].sharedByProject = helper::allocStrInComMem(helper::utf82utf16(f.m_sharedbyproject));
		// rights
		DWORD64 rights = 0;
		std::for_each(f.m_rights.begin(), f.m_rights.end(), [&rights](SDRmFileRight i){
			rights |= (DWORD64)i;
		});
		p[i].rights = rights;
	}
	*ppListFiles = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectListTotalSharedWithMeFiles(HANDLE hUser,
	uint32_t projectId,
	const wchar_t* orderBy,
	const wchar_t* searchStr,
	ProjectSharedWithMeFile** ppListFiles,
	uint32_t* pListSize)
{
	OutputDebugStringA("SDWL_User_ProjectListTotalSharedWithMeFiles \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<SDR_PROJECT_SHAREDWITHME_FILE> totalVec;
	std::vector<SDR_PROJECT_SHAREDWITHME_FILE> pageVec;
	uint32_t pageId = 1;
	uint32_t pageSize = 1000;

	do
	{
		pageVec.clear();

		auto rt = g_User->ProjectListSharedWithMeFiles(projectId, pageId, pageSize,
			helper::utf162utf8(orderBy),
			helper::utf162utf8(searchStr),
			pageVec);

		if (!rt) {
			return rt.GetCode();
		}

		for (auto one : pageVec) {
			totalVec.push_back(one);
		}

	} while (pageVec.size() == pageSize && pageId++);

	int size = totalVec.size();
	*pListSize = size;
	if (size == 0) {
		*ppListFiles = NULL;
		return SDWL_SUCCESS;
	}

	ProjectSharedWithMeFile* p = (ProjectSharedWithMeFile*)::CoTaskMemAlloc(size * sizeof(ProjectSharedWithMeFile));
	for (int i = 0; i < size; i++) {
		auto f = totalVec[i];
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(f.m_duid));
		p[i].name = helper::allocStrInComMem(helper::utf82utf16(f.m_name));
		p[i].fileType = helper::allocStrInComMem(helper::utf82utf16(f.m_filetype));
		p[i].size = f.m_size;
		p[i].sharedDate = f.m_shareddate;
		p[i].sharedBy = helper::allocStrInComMem(helper::utf82utf16(f.m_sharedby));
		p[i].transactionId = helper::allocStrInComMem(helper::utf82utf16(f.m_transactionid));
		p[i].transactionCode = helper::allocStrInComMem(helper::utf82utf16(f.m_transactioncode));
		p[i].comment = helper::allocStrInComMem(helper::utf82utf16(f.m_comment));
		p[i].isOwner = f.m_isowner;
		p[i].protectType = f.m_protectedtype;
		p[i].sharedByProject = helper::allocStrInComMem(helper::utf82utf16(f.m_sharedbyproject));
		// rights
		DWORD64 rights = 0;
		std::for_each(f.m_rights.begin(), f.m_rights.end(), [&rights](SDRmFileRight i) {
			rights |= (DWORD64)i;
		});
		p[i].rights = rights;
	}
	*ppListFiles = p;

	return SDWL_SUCCESS;

}

// Download a project file from the specify project (projectId) which is shared by other project
NXSDK_API DWORD SDWL_User_ProjectDownloadSharedWithMeFile(HANDLE hUser,
	uint32_t projectId,
	const wchar_t* transactionCode,
	const wchar_t* transactionId,
	wchar_t* localDestFolder,
	bool forViewer,
	wchar_t** outPath) 
{
	OutputDebugStringA("SDWL_User_ProjectDownloadSharedWithMeFile \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring targetFolder = localDestFolder;
	auto rt = g_User->ProjectDownloadSharedWithMeFile(projectId, transactionCode, transactionId, targetFolder, forViewer);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(targetFolder);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectPartialDownloadSharedWithMeFile(HANDLE hUser,
	uint32_t projectId,
	const wchar_t* transactionCode,
	const wchar_t* transactionId,
	wchar_t* localDestFolder,
	bool forViewer,
	wchar_t** outPath)
{
	OutputDebugStringA("SDWL_User_ProjectPartialDownloadSharedWithMeFile \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring targetFolder = localDestFolder;
	auto rt = g_User->ProjectPartialDownloadSharedWithMeFile(projectId, transactionCode, transactionId, targetFolder, forViewer);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(targetFolder);

	return SDWL_SUCCESS;
}

// Share a project file, which is shared by other project, to another project.
NXSDK_API DWORD SDWL_User_ProjectReshareSharedWithMeFile(HANDLE hUser,
	uint32_t proId,
	const wchar_t* transactionId,
	const wchar_t* transactionCode,
	const wchar_t* emailList, // mandatory for myVault only, optional otherwise
	uint32_t* recipients, uint32_t arrLen,
	ProjectReshareFileResult* pResult)
{
	OutputDebugStringA("SDWL_User_ProjectReshareSharedWithMeFile \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<uint32_t> vec;
	for (int i = 0; i < arrLen; i++) {
		vec.push_back(recipients[i]);
	}

	SDR_PROJECT_RESHARE_FILE_RESULT result;
	auto rt = g_User->ProjectReshareSharedWithMeFile(proId,
		helper::utf162utf8(transactionId),
		helper::utf162utf8(transactionCode),
		helper::utf162utf8(emailList),
		vec,
		result);
	if (!rt) {
		return rt.GetCode();
	}

	ProjectReshareFileResult* p = (ProjectReshareFileResult*)::CoTaskMemAlloc(sizeof(ProjectReshareFileResult));
	p->protectType = result.m_protectiontype;
	p->newTransactionId = helper::allocStrInComMem(helper::utf82utf16(result.m_newtransactionid));
	//
	p->newSharedList = helper::allocStrInComMem(helper::utf82utf16(helper::vectorUint32ToString(result.m_newsharedlist)));
	p->alreadySharedList = helper::allocStrInComMem(helper::utf82utf16(helper::vectorUint32ToString(result.m_alreadysharedlist)));
	pResult = p;

	return SDWL_SUCCESS;
}

// Update a shared file's recipients, like add a new projectId, or remove an exist projectId 
NXSDK_API DWORD SDWL_User_ProjectUpdateSharedFileRecipients(HANDLE hUser,
	const wchar_t* duid,
	uint32_t* addRecipients, uint32_t addLen, // If add recipients
	uint32_t* removedRecipients, uint32_t removedLen, // If removed recipients
	const wchar_t* comment,
	uint32_t** ppAddResult, uint32_t* pAddLen, // add results
	uint32_t** ppRemovedResult, uint32_t* pRemovedLen) // removed results
{
	OutputDebugStringA("SDWL_User_ProjectUpdateSharedFileRecipients \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<uint32_t> addVec;
	for (int i = 0; i < addLen; i++) {
		addVec.push_back(addRecipients[i]);
	}
	std::vector<uint32_t> removedVec;
	for (int i = 0; i < removedLen; i++) {
		removedVec.push_back(removedRecipients[i]);
	}
	std::map<std::string, std::vector<uint32_t>> recipients;
	recipients.insert(std::pair<std::string, std::vector<uint32_t>>(NEW_RECIPIENTS, addVec));
	recipients.insert(std::pair<std::string, std::vector<uint32_t>>(REMOVED_RECIPIENTS, removedVec));

	std::map<std::string, std::vector<uint32_t>> result;
	auto rt = g_User->ProjectUpdateSharedFileRecipients(helper::utf162utf8(duid),
		recipients,
		helper::utf162utf8(comment),
		result);
	if (!rt) {
		return rt.GetCode();
	}

	//default
	*ppAddResult = NULL;
	*ppRemovedResult = NULL;

	auto it = result.find(NEW_RECIPIENTS);
	if (it != result.end()) {
		auto retAdd = it->second;
		*pAddLen = retAdd.size();
		if (*pAddLen > 0) {
			uint32_t* p = (uint32_t*)::CoTaskMemAlloc(sizeof(uint32_t)*retAdd.size());
			for (int i = 0; i < retAdd.size(); i++) {
				p[i] = retAdd[i];
			}
			*ppAddResult = p;
		}
	}

	auto ite = result.find(REMOVED_RECIPIENTS);
	if (ite != result.end()) {
		auto retRemoved = ite->second;
		*pRemovedLen = retRemoved.size();
		if (*pRemovedLen > 0) {
			uint32_t* p = (uint32_t*)::CoTaskMemAlloc(sizeof(uint32_t)*retRemoved.size());
			for (int i = 0; i < retRemoved.size(); i++) {
				p[i] = retRemoved[i];
			}
			*ppRemovedResult = p;
		}
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectShareFile(HANDLE hUser,
	uint32_t proId,
	uint32_t* recipients, uint32_t len,
	const wchar_t* name,
	const wchar_t* filePathId,
	const wchar_t* filePath,
	const wchar_t* comment,
	ProjectShareFileResult* ret)
{
	OutputDebugStringA("SDWL_User_ProjectShareFile \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<uint32_t> vec;
	for (int i = 0; i < len; i++) {
		vec.push_back(recipients[i]);
	}

	// Will failed and prompt invalid pathId if do url encode.
	//std::string encodedPathId = helper::UrlEncode(helper::utf162utf8(filePathId));

	SDR_PROJECT_SHARE_FILE_RESULT result;
	auto rt = g_User->ProjectShareFile(proId,
		vec,
		helper::utf162utf8(name),
		helper::utf162utf8(filePathId),
		helper::utf162utf8(filePath),
		helper::utf162utf8(comment),
		result);
	if (!rt) {
		return rt.GetCode();
	}

	ProjectShareFileResult* p = (ProjectShareFileResult*)::CoTaskMemAlloc(sizeof(ProjectShareFileResult));
	p->name = helper::allocStrInComMem(helper::utf82utf16(result.m_filename));
	p->duid = helper::allocStrInComMem(helper::utf82utf16(result.m_duid));
	p->filePathId = helper::allocStrInComMem(helper::utf82utf16(result.m_filepathid));
	p->transactionId = helper::allocStrInComMem(helper::utf82utf16(result.m_transactionid));
	//
	p->newSharedList = helper::allocStrInComMem(helper::utf82utf16(helper::vectorUint32ToString(result.m_newsharedlist)));
	p->alreadySharedList = helper::allocStrInComMem(helper::utf82utf16(helper::vectorUint32ToString(result.m_alreadysharedlist)));
	
	ret = p;
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ProjectRevokeShareFile(HANDLE hUser, const wchar_t* duid)
{
	OutputDebugStringA("SDWL_User_ProjectRevokeShareFile \n");
	if (g_User == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_User->ProjectRevokeSharedFile(helper::utf162utf8(duid));
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

#pragma endregion // Sharing transaction for Project

#pragma region User_SharedWithMe
NXSDK_API DWORD SDWL_User_ListSharedWithMeAllFiles(HANDLE hUser,
	const wchar_t * orderBy,
	const wchar_t * searchString,
	SharedWithMeFileInfo ** ppFiles,
	uint32_t* pSize)
{
	OutputDebugStringA("call SDWL_User_ListSharedWithMeAllFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<SDR_SHAREDWITHME_FILE_INFO> vec;
	std::vector<SDR_SHAREDWITHME_FILE_INFO> pageVec;
	uint32_t pageId = 1;
	uint32_t pageSize = 1000;
	do
	{
		pageVec.clear();

		auto rt = g_User->GetSharedWithMeFiles(pageId, pageSize,
			helper::utf162utf8(orderBy),
			helper::utf162utf8(searchString),
			pageVec);

		if (!rt) {
			return rt.GetCode();
		}

		for (auto i : pageVec)
		{
			vec.push_back(i);
		}

	} while (pageVec.size() == pageSize && pageId++);

	auto size = vec.size();
	*pSize = (uint32_t)size;
	if (size == 0) {
		*ppFiles = NULL;
		return SDWL_SUCCESS;
	}

	// convert it to c-like struct and encapsule into COM-mem for c# used
	SharedWithMeFileInfo* p = (SharedWithMeFileInfo*)::CoTaskMemAlloc(size * sizeof(SharedWithMeFileInfo));

	for (size_t i = 0; i < size; i++)
	{
		auto j = vec[i];
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(j.m_duid));
		p[i].nxlName = helper::allocStrInComMem(helper::utf82utf16(j.m_nxlname));
		p[i].fileType = helper::allocStrInComMem(helper::utf82utf16(j.m_filetype));
		p[i].sharedbyWhoEmail = helper::allocStrInComMem(helper::utf82utf16(j.m_sharedby));
		p[i].transactionId = helper::allocStrInComMem(helper::utf82utf16(j.m_transactionid));
		p[i].transactionCode = helper::allocStrInComMem(helper::utf82utf16(j.m_transactioncode));
		p[i].sharedlinkUrl = helper::allocStrInComMem(helper::utf82utf16(j.m_sharedlink));
		p[i].rights = helper::allocStrInComMem(helper::utf82utf16(j.m_rights));
		p[i].comments = helper::allocStrInComMem(helper::utf82utf16((j.m_comments)));

		p[i].size = j.m_size;
		p[i].sharedDateMillis = j.m_shareddate;
		p[i].isOwner = j.m_isowner;
	}
	*ppFiles = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_ListSharedWithMeFiles(HANDLE hUser,
	uint32_t pageId, uint32_t pageSize,
	const wchar_t * orderBy,
	const wchar_t * searchString,
	SharedWithMeFileInfo ** ppFiles,
	uint32_t* pSize)
{
	OutputDebugStringA("call SDWL_User_ListSharedWithMeFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	std::vector<SDR_SHAREDWITHME_FILE_INFO> vec;
	auto rt = g_User->GetSharedWithMeFiles(pageId, pageSize,
		helper::utf162utf8(orderBy),
		helper::utf162utf8(searchString),
		vec);

	auto size = vec.size();
	*pSize = (uint32_t)size;
	if (!rt) {
		return rt.GetCode();
	}

	if (size == 0) {
		*ppFiles = NULL;
		return SDWL_SUCCESS;
	}

	// convert it to c-like struct and encapsule into COM-mem for c# used
	SharedWithMeFileInfo* p = (SharedWithMeFileInfo*)::CoTaskMemAlloc(size * sizeof(SharedWithMeFileInfo));

	for (size_t i = 0; i < size; i++)
	{
		auto j = vec[i];
		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(j.m_duid));
		p[i].nxlName = helper::allocStrInComMem(helper::utf82utf16(j.m_nxlname));
		p[i].fileType = helper::allocStrInComMem(helper::utf82utf16(j.m_filetype));
		p[i].sharedbyWhoEmail = helper::allocStrInComMem(helper::utf82utf16(j.m_sharedby));
		p[i].transactionId = helper::allocStrInComMem(helper::utf82utf16(j.m_transactionid));
		p[i].transactionCode = helper::allocStrInComMem(helper::utf82utf16(j.m_transactioncode));
		p[i].sharedlinkUrl = helper::allocStrInComMem(helper::utf82utf16(j.m_sharedlink));
		p[i].rights = helper::allocStrInComMem(helper::utf82utf16(j.m_rights));
		p[i].comments = helper::allocStrInComMem(helper::utf82utf16((j.m_comments)));

		p[i].size = j.m_size;
		p[i].sharedDateMillis = j.m_shareddate;
		p[i].isOwner = j.m_isowner;
	}
	*ppFiles = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_DownloadSharedWithMeFiles(HANDLE hUser,
	const wchar_t * transactionId,
	const wchar_t * transactionCode,
	const wchar_t * downlaodDestLocalFolder,
	bool forViewer,
	const wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_DownloadSharedWithMeFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (transactionId == NULL || transactionCode == NULL || downlaodDestLocalFolder == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring destfolder = downlaodDestLocalFolder;
	auto rt = g_User->SharedWithMeDownloadFile(transactionCode, transactionId, destfolder, forViewer);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(destfolder);

	return SDWL_SUCCESS;

}

NXSDK_API DWORD SDWL_User_DownloadSharedWithMePartialFiles(HANDLE hUser,
	const wchar_t * transactionId,
	const wchar_t * transactionCode,
	const wchar_t * downlaodDestLocalFolder,
	bool forViewer,
	const wchar_t** outPath)
{
	OutputDebugStringA("call SDWL_User_DownloadSharedWithMePartialFiles\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (transactionId == NULL || transactionCode == NULL || downlaodDestLocalFolder == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring destfolder = downlaodDestLocalFolder;
	auto rt = g_User->SharedWithMeDownloadPartialFile(transactionCode, transactionId, destfolder, forViewer);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(destfolder);

	return SDWL_SUCCESS;

}
#pragma endregion // User_SharedWithMe

#pragma region User_SharedWorkspace

NXSDK_API DWORD SDWL_User_ListSharedWorkspaceAllFiles(
	HANDLE hUser,
	const wchar_t* repoId,
	const wchar_t* orderBy,
	const wchar_t* searchString,
	const wchar_t* path,
	SharedWorkspaceFileInfo** ppFiles,
	uint32_t* pSize
)
{
	OutputDebugStringA("call SDWL_User_ListSharedWorkspaceAllFiles \n");
	// sanity check
	if (nullptr == g_User || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == repoId || nullptr == path) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<SDR_SHARED_WORKSPACE_FILE_INFO> vec;
	std::vector<SDR_SHARED_WORKSPACE_FILE_INFO> pageVec;
	uint32_t pageId = 1;
	uint32_t pageSize = 1000;

	std::string encodedPath = helper::UrlEncode(helper::utf162utf8(path));

	do {
		pageVec.clear();
		auto rt = g_User->GetSharedWorkspaceListFiles(helper::utf162utf8(repoId) ,pageId, pageSize, helper::utf162utf8(orderBy),
			encodedPath, helper::utf162utf8(searchString), pageVec);
		if (!rt) {
			return rt.GetCode();
		}

		for (auto i : pageVec) {
			vec.push_back(i);
		}

	} while (pageVec.size() == pageSize && pageId++);

	*pSize = (uint32_t)vec.size();
	if (*pSize == 0) {
		*ppFiles = nullptr;
		return SDWL_SUCCESS;
	}

	SharedWorkspaceFileInfo* pF = (SharedWorkspaceFileInfo*)CoTaskMemAlloc(sizeof(SharedWorkspaceFileInfo) * (*pSize));
	for (uint32_t i = 0; i < *pSize; i++) {
		auto f = vec[i];
		pF[i].fileId = helper::allocStrInComMem(helper::utf82utf16(f.fileId));
		pF[i].path = helper::allocStrInComMem(helper::utf82utf16(f.path));
		pF[i].pathId = helper::allocStrInComMem(helper::utf82utf16(f.pathId));
		pF[i].fileName = helper::allocStrInComMem(helper::utf82utf16(f.fileName));
		pF[i].fileType = helper::allocStrInComMem(helper::utf82utf16(f.fileType));
		pF[i].isProtectedFile = f.isProtectedFile ? 1 : 0;
		pF[i].lastModified = f.lastModified;
		pF[i].creationTime = f.creationTime;
		pF[i].size = f.fileSize;
		pF[i].isFolder = f.isFolder ? 1 : 0;
	}
	*ppFiles = pF;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UploadSharedWorkspaceFile(
	HANDLE hUser,
	const wchar_t* repoId,
	const wchar_t* destFolder,
	const wchar_t* filePath,
	int uploadType,
	bool userConfirmedFileOverwrite
)
{
	OutputDebugStringA("call SDWL_User_UploadSharedWorkspaceFile\n");
	// sanity check
	if (nullptr == g_User || (ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == repoId || nullptr == destFolder || nullptr == filePath) {
		return SDWL_INTERNAL_ERROR;
	}

	// Open nxl file
	ISDRmNXLFile* pFile = nullptr;
	auto rt = g_User->OpenFile(filePath, &pFile);
	if (!rt) {
		if (pFile != nullptr) {
			g_User->CloseFile(pFile);
		}
		return rt.GetCode();
	}

	auto rt2 = g_User->UploadSharedWorkspaceFile(helper::utf162utf8(repoId), destFolder, pFile, uploadType, userConfirmedFileOverwrite);

	if (pFile != nullptr) {
		g_User->CloseFile(pFile);
	}

	if (!rt2) {
		return rt2.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_DownloadSharedWorkspaceFile(
	HANDLE hUser,
	const wchar_t* repoId,
	const wchar_t* path,
	const wchar_t* targetFolder,
	uint32_t downloadType,
	bool isNxl,
	wchar_t** outPath
)
{
	OutputDebugStringA("call SDWL_User_DownloadSharedWorkspaceFile \n");

	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == repoId || nullptr == path || nullptr == targetFolder) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring destFolder(targetFolder);
	auto rt = g_User->DownloadSharedWorkspaceFile(helper::utf162utf8(repoId), helper::utf162utf8(path),
		destFolder, downloadType, isNxl);
	if (!rt) {
		*outPath = nullptr;
		return rt.GetCode();
	}

	*outPath = helper::allocStrInComMem(destFolder);
	
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_IsSharedWorkspaceFileExist(
	HANDLE hUser,
	const wchar_t* repoId,
	const wchar_t* path,
	bool* bExist
)
{
	OutputDebugStringA("call SDWL_User_IsSharedWorkspaceFileExist\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == repoId || nullptr == path) {
		return SDWL_INTERNAL_ERROR;
	}

	bool isExist = false;
	auto rt = g_User->IsSharedWorkspaceFileExist(helper::utf162utf8(repoId), helper::utf162utf8(path), isExist);
	if (!rt) {
		return rt.GetCode();
	}
	*bExist = isExist;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetSharedWorkspaceNxlFileHeader(
	HANDLE hUser,
	const wchar_t* repoId,
	const wchar_t* path,
	const wchar_t* targetFolder,
	wchar_t** outPath
)
{
	OutputDebugStringA("call SDWL_User_GetSharedWorkspaceNxlFileHeader\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == repoId || nullptr == path || nullptr == targetFolder) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string target = helper::utf162utf8(targetFolder);
	auto rt = g_User->GetWorkspaceNxlFileHeader(helper::utf162utf8(repoId), helper::utf162utf8(path), target);
	if (!rt) {
		return rt.GetCode();
	}
	*outPath = helper::allocStrInComMem(helper::utf82utf16(target));

	return SDWL_SUCCESS;
}

#pragma endregion // User_SharedWorkspace

NXSDK_API DWORD SDWL_User_CopyNxlFile(
	HANDLE hUser,
	const wchar_t* fileName,
	const wchar_t* filePath,
	NxlFileSpaceType fileSpaceType,
	const wchar_t* spaceId,
	const wchar_t* destFileName,
	const wchar_t* destFolderPath,
	NxlFileSpaceType destFileSpaceType,
	const wchar_t* destSpaceId,
	bool bOverwrite,
	const wchar_t* transactionCode,
	const wchar_t* transactionId
)
{
	OutputDebugStringA("call SDWL_User_CopyNxlFile\n");
	if (nullptr == g_User) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == fileName || nullptr == filePath || nullptr == spaceId
		|| nullptr == destFileName || nullptr == destFolderPath || nullptr == destSpaceId
		|| nullptr == transactionCode || nullptr == transactionId) {
		return SDWL_INTERNAL_ERROR;
	}

	std::string s_fileName = helper::utf162utf8(fileName);
	std::string s_filePath = helper::utf162utf8(filePath);
	std::string s_spaceId = helper::utf162utf8(spaceId);
	std::string s_destFileName = helper::utf162utf8(destFileName);
	std::string s_destFolderPath = helper::utf162utf8(destFolderPath);
	std::string s_destSpaceId = helper::utf162utf8(destSpaceId);
	std::string s_transactionCode = helper::utf162utf8(transactionCode);
	std::string s_transactionId = helper::utf162utf8(transactionId);

	auto rt = g_User->CopyNxlFile(s_fileName, s_filePath, (RM_NxlFileSpaceType)fileSpaceType, s_spaceId,
		s_destFileName, s_destFolderPath, (RM_NxlFileSpaceType)destFileSpaceType, s_destSpaceId,
		bOverwrite, s_transactionCode, s_transactionId);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

#pragma endregion // User_Level

#pragma region Repository
NXSDK_API DWORD SDWL_User_GetRepositories(HANDLE hUser,
	RepositoryInfo** pList, uint32_t* pLen)
{
	OutputDebugStringA("SDWL_User_GetRepositories \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<SDR_REPOSITORY> vecRepo;
	auto rt = g_User->GetRepositories(vecRepo);
	if (!rt) {
		return rt.GetCode();
	}

	int size = (size_t)vecRepo.size();
	*pLen = size;
	if (size == 0) {
		*pList = nullptr;
		return SDWL_SUCCESS;
	}

	RepositoryInfo* pRI = (RepositoryInfo*)::CoTaskMemAlloc(sizeof(RepositoryInfo) * size);
	for (int i = 0; i < size; i++) {
		pRI[i].isShard = vecRepo[i].m_isShared ? 1 : 0;
		pRI[i].isDefault = vecRepo[i].m_isDefault ? 1 : 0;
		pRI[i].createTime = vecRepo[i].m_createTime;
		pRI[i].updateTime = vecRepo[i].m_updatedTime;
		pRI[i].repoId = helper::allocStrInComMem(vecRepo[i].m_repoId);
		pRI[i].name = helper::allocStrInComMem(vecRepo[i].m_name);
		pRI[i].type = helper::allocStrInComMem(vecRepo[i].m_type);
		pRI[i].providerClass = helper::allocStrInComMem(vecRepo[i].m_providerClass);
		pRI[i].accountName = helper::allocStrInComMem(vecRepo[i].m_accountName);
		pRI[i].accountId = helper::allocStrInComMem(vecRepo[i].m_accountId);
		pRI[i].token = helper::allocStrInComMem(vecRepo[i].m_token);
		pRI[i].preference = helper::allocStrInComMem(vecRepo[i].m_preference);
	}
	*pList = pRI;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetRepositoryAccessToken(
	HANDLE hUser,
	const wchar_t* repoId,
	wchar_t** accessToken)
{
	OutputDebugStringA("SDWL_User_GetRepositoryAccessToken \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if (nullptr == repoId) {
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring result;
	auto rt = g_User->GetRepositoryAccessToken(repoId, result);
	if (!rt) {
		return rt.GetCode();
	}
	*accessToken = helper::allocStrInComMem(result);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_GetRepositoryAuthorizationUrl(
	HANDLE hUser,
	const wchar_t* repoType,
	const wchar_t* name,
	wchar_t** autoURL)
{
	OutputDebugStringA("SDWL_User_GetRepositoryAuthorizationUrl \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if (nullptr == name) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring result;
	auto rt = g_User->GetRepositoryAuthorizationUrl(repoType, name, result);
	if (!rt) {
		return rt.GetCode();
	}
	*autoURL = helper::allocStrInComMem(result);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_UpdateRepository(
	HANDLE hUser,
	const wchar_t* repoId,
	const wchar_t* token,
	const wchar_t* name)
{
	OutputDebugStringA("SDWL_User_UpdateRepository \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if (nullptr == repoId) {
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_User->UpdateRepository(repoId, token, name);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_User_RemoveRepository(
	HANDLE hUser,
	const wchar_t* repoId)
{
	OutputDebugStringA("SDWL_User_RemoveRepository \n");
	if (nullptr == g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	if (nullptr == repoId) {
		return SDWL_INTERNAL_ERROR;
	}

	if ((ISDRmUser*)hUser != g_User) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_User->RemoveRepository(repoId);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

#pragma endregion // Repository

#pragma region LocalFiles

NXSDK_API DWORD SDWL_File_GetListNumber(HANDLE hFile, int* pSize)
{
	OutputDebugStringA("call SDWL_File_GetListNumber\n");
	// sanity check
	ISDRFiles* pF = (ISDRFiles*)hFile;

	*pSize  = (int)pF->GetListNumber();

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_File_GetList(HANDLE hFile, 
	wchar_t** strArray, 
	int* pSize)
{
	OutputDebugStringA("call SDWL_File_GetList\n");
	// sanity check
	if (strArray == NULL || pSize == NULL) {
		return SDWL_INTERNAL_ERROR;

	}
	
	ISDRFiles* pF = (ISDRFiles*)hFile;
	auto rt = pF->GetList();
	const auto size = rt.size();

	if (size < 0) {
		*strArray = NULL;
		*pSize = (int)size;
		return SDWL_INTERNAL_ERROR;
	}

	if (size == 0) {
		*strArray = NULL;
		*pSize = 0;
		return SDWL_SUCCESS;
	}

	// alloc mem
	wchar_t** pBuf = (wchar_t**)::CoTaskMemAlloc(size*sizeof(wchar_t*));
	// alloc each item
	for (size_t i = 0; i < size; i++) {
		pBuf[i] = helper::allocStrInComMem(rt[i]);
	}
	// set pSize
	*pSize = (int)size;
	*strArray = (wchar_t*)pBuf;


	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_File_GetFile(HANDLE hFile, int index, HANDLE* hNxlFile)
{
	OutputDebugStringA("call SDWL_File_GetFile\n");
	// sanity check
	ISDRFiles* pF = (ISDRFiles*)hFile;
	
	// begin
	auto rt = pF->GetFile(index);
	if (rt == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	
	*hNxlFile = rt;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_File_GetFile2(HANDLE hFile, 
	const wchar_t * FileName,HANDLE* hNxlFile)
{
	OutputDebugStringA("call SDWL_File_GetFile2\n");
	// sanity check
	ISDRFiles* pF = (ISDRFiles*)hFile;
	
	// begin
	std::wstring w = std::wstring(FileName);
	auto rt = pF->GetFile(w);
	if (rt != NULL) {
		*hNxlFile = rt;
		return SDWL_SUCCESS;		
	}

	// SDK new added function, by design SDK's nxl file list only save that local created.

	// for rms downloaded file, we must use g_User's OpenFile to get it object
	ISDRmNXLFile* pNxl = NULL;
	g_User->OpenFile(w, &pNxl);

	if (pNxl != NULL) {
		*hNxlFile = pNxl;
		return SDWL_SUCCESS;
	}

	return SDWL_INTERNAL_ERROR;

	
}

NXSDK_API DWORD SDWL_File_RemoveFile(HANDLE hFile, HANDLE hNxlFile,bool *pResult)
{
	OutputDebugStringA("call SDWL_File_RemoveFile\n");
	// sanity check
	ISDRFiles* pF = (ISDRFiles*)hFile;
	
	ISDRmNXLFile* p = (ISDRmNXLFile*)hNxlFile;
	
	auto rt = pF->RemoveFile(p);

	*pResult = rt;

	return SDWL_SUCCESS;
}
#pragma endregion // LocalFiles

#pragma region NxlFile

NXSDK_API DWORD SDWL_NXL_File_GetFileName(HANDLE hNxlFile, wchar_t** ppname)
{
	OutputDebugStringA("call SDWL_NXL_File_GetFileName\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;

	*ppname = helper::allocStrInComMem(pF->GetFileName());
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_GetFileSize(HANDLE hNxlFile, DWORD64* pSize)
{
	OutputDebugStringA("call SDWL_NXL_File_GetFileSize\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;

	*pSize = pF->GetFileSize();

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_IsValidNxl(HANDLE hNxlFile, bool* pResult)
{
	OutputDebugStringA("call SDWL_NXL_File_IsValidNxl\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;

	*pResult = pF->IsValidNXL();

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_GetRights(HANDLE hNxlFile, int** pprights, int* pLen)
{
	OutputDebugStringA("call SDWL_NXL_File_GetRights\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;
	// begin
	auto rt = pF->GetRights();
	size_t size = rt.size();
	if (size == 0) {
		*pprights = NULL;
		*pLen = 0;
		return SDWL_SUCCESS;
	}

	int* buf = (int*)::CoTaskMemAlloc(sizeof(int*)*size);

	for (size_t i=0; i < size; i++) {
		buf[i] = rt[i];
	}
	*pLen = (int)size;
	*pprights = buf;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_GetWaterMark(HANDLE hNxlFile, WaterMark * pWaterMark)
{
	OutputDebugStringA("call SDWL_NXL_File_GetWaterMark\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;
	// begin
	SDR_WATERMARK_INFO wm = pF->GetWaterMark();

	WaterMark* buf = pWaterMark;
	
	{
		buf->text = helper::allocStrInComMem(helper::utf82utf16(wm.text));
		buf->fontName = helper::allocStrInComMem(helper::utf82utf16(wm.fontName));
		buf->fontColor = helper::allocStrInComMem(helper::utf82utf16(wm.fontColor));
		buf->repeat = wm.repeat;
		buf->fontSize = wm.fontSize;
		buf->rotation = wm.rotation;
		buf->transparency = wm.transparency;		
	}

	return SDWL_SUCCESS;

}

NXSDK_API DWORD SDWL_NXL_File_GetExpiration(HANDLE hNxlFile, Expiration * pExpiration)
{
	OutputDebugStringA("call SDWL_NXL_File_GetExpiration\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;
	// begin

	auto ex = pF->GetExpiration();

	{
		pExpiration->start = ex.start;
		pExpiration->end = ex.end;
		pExpiration->type = (ExpiryType)ex.type;
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_GetTags(HANDLE hNxlFile, wchar_t ** ppTags)
{
	OutputDebugStringA("call SDWL_NXL_File_GetTags\n");
	
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;
	
	// begin
	*ppTags = helper::allocStrInComMem(helper::utf82utf16(pF->GetTags()));

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_CheckRights(HANDLE hNxlFile, int right, bool* pResult)
{
	OutputDebugStringA("call SDWL_NXL_File_CheckRights\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;

	auto rt = pF->CheckRights((SDRmFileRight)right);

	*pResult = rt;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_IsUploadToRMS(HANDLE hNxlFile, bool* pResult)
{
	OutputDebugStringA("call SDWL_NXL_File_IsUploadToRMS\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	//ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;

	//*pResult = pF->IsUploadToRMS();

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_GetAdhocWatermarkString(HANDLE hNxlFile, wchar_t** ppWmStr)
{
	OutputDebugStringA("call SDWL_NXL_File_GetAdhocWatermarkString\n");
	// sanity check
	if (hNxlFile == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	ISDRmNXLFile* pF = (ISDRmNXLFile*)hNxlFile;

	std::string utf8Str = pF->GetAdhocWaterMarkString();
	*ppWmStr = helper::allocStrInComMem(helper::utf82utf16(utf8Str));
	
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_GetActivityInfo(const wchar_t* fileName,
	NXL_FILE_ACTIVITY_INFO** pInfo,
	DWORD* pSize)
{
	OutputDebugStringA("call SDWL_NXL_File_GetActivityInfo\n");
	// sanity check
	if (g_User == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (fileName == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<SDR_FILE_ACTIVITY_INFO>v;
	auto rt = g_User->GetActivityInfo(fileName, v);
	if (!rt) {
		return rt.GetCode();
	}

	// fill info
	auto size = v.size();
	*pSize = (DWORD)size;
	if (size == 0) {
		*pInfo = NULL;
		return SDWL_SUCCESS;

	}

	NXL_FILE_ACTIVITY_INFO* p = (NXL_FILE_ACTIVITY_INFO*)::CoTaskMemAlloc(size * sizeof(NXL_FILE_ACTIVITY_INFO));

	for (size_t i = 0; i < size; i++) {
		SDR_FILE_ACTIVITY_INFO item = v[i];

		p[i].duid = helper::allocStrInComMem(helper::utf82utf16(item.duid));
		p[i].email = helper::allocStrInComMem(helper::utf82utf16(item.email));
		p[i].operation = helper::allocStrInComMem(helper::utf82utf16(item.operation));
		p[i].deviceType = helper::allocStrInComMem(helper::utf82utf16(item.deviceType));
		p[i].deviceId = helper::allocStrInComMem(helper::utf82utf16(item.deviceId));
		p[i].accessResult = helper::allocStrInComMem(helper::utf82utf16(item.accessResult));
		p[i].accessTime = item.accessTime;
	}

	*pInfo = p;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_NXL_File_GetNxlFileActivityLog(
	HANDLE hNxlFile,
	DWORD64 startPos, DWORD64 count,
	BYTE searchField, 
	const wchar_t* searchText,
	BYTE orderByField, 
	bool orderByReverse)
{
	OutputDebugStringA("call SDWL_NXL_File_GetNxlFileActivityLog\n");
	// sanity check
	//if (g_User == NULL) {
	//	// not found
	//	return SDWL_INTERNAL_ERROR;
	//}	

	//ISDRmNXLFile *pf = (ISDRmNXLFile *)hNxlFile;
	//
	//auto rt = g_User->GetNXLFileActivitylog(pf,
	//	startPos,
	//	count,
	//	searchField,
	//	helper::utf162utf8(searchText),
	//	orderByField, 
	//	orderByReverse);

	//if (!rt) {
	//	return rt.GetCode();
	//}
	return SDWL_SUCCESS;
}
#pragma endregion

#pragma region RPM_DRIVER

NXSDK_API DWORD SDWL_RPM_GetRPMDir(HANDLE hSession, RPMDir** ppArr, int * pLen, int option)
{
	OutputDebugStringA("call SDWL_RPM_GetRPMDir\n");
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	std::vector<std::wstring> vecPaths;
	auto rt = g_RmsIns->GetRPMDir(vecPaths, (SDRmRPMFolderQuery) option);
	if (!rt) {
		return rt.GetCode();
	}
	if (vecPaths.size() == 0) {
		*ppArr = nullptr;
		*pLen = 0;
		return SDWL_SUCCESS;
	}
	*pLen = vecPaths.size();

	RPMDir* pA = (RPMDir*)::CoTaskMemAlloc(sizeof(RPMDir)*(vecPaths.size()));

	for (int i = 0; i < vecPaths.size(); i++) {
		pA[i].pArr = helper::allocStrInComMem(vecPaths[i]);
		pA[i].len = (UINT)vecPaths[i].size();
	}
	*ppArr = pA;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_GetFileInfo(HANDLE hSession, const wchar_t* filePath, wchar_t** duid, CENTRAL_RIGHTS** pArray, uint32_t* pArrSize/* userRightsAndWatermarks */,
	int** pArrRights, uint32_t* pArrRightsSize/* rights */, WaterMark* watermark, Expiration* expiration, wchar_t** tags,
	wchar_t** tokenGroup, wchar_t** creatorId, wchar_t** infoExt, DWORD* attributes, DWORD* isRPMFolder, DWORD* isNxlFile)
{
	OutputDebugStringA("SDWL_RPM_GetFileInfo\n");
	if (nullptr == g_RmsIns) {
		return SDWL_INTERNAL_ERROR;
	}
	if (nullptr == filePath) {
		return SDWL_INTERNAL_ERROR;
	}
	
	std::string _duid;
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> _userRightsAndWatermarks;
	std::vector<SDRmFileRight> _rights;
	SDR_WATERMARK_INFO _watermark;
	SDR_Expiration _expiration;
	std::string _tags;
	std::string _tokengroup;
	std::string _creatorid;
	std::string _infoext;
	DWORD _attributes;
	DWORD _isRPMFolder;
	DWORD _isNxlFile;
	auto rt = g_RmsIns->RPMGetFileInfo(filePath,
		_duid,_userRightsAndWatermarks, _rights, _watermark, _expiration, _tags, _tokengroup, _creatorid, _infoext,
		_attributes, _isRPMFolder, _isNxlFile);

	if (!rt) {
		return rt.GetCode();
	}

	// prepare out parameters
	//
	*duid = helper::allocStrInComMem(helper::utf82utf16(_duid));

	// userRightsAndWatermark
	std::size_t size = _userRightsAndWatermarks.size();
	if (size > 0) {
		CENTRAL_RIGHTS* pA = (CENTRAL_RIGHTS*)::CoTaskMemAlloc(sizeof(CENTRAL_RIGHTS) * size);
		for (int i = 0; i < size; i++) 
		{
			pA[i].rights = _userRightsAndWatermarks[i].first;

			//
			std::vector<SDR_WATERMARK_INFO> watermarks = _userRightsAndWatermarks[i].second;
			size_t watermarksSize = watermarks.size();
			if (watermarksSize > 0) {
				WaterMark* pWM = (WaterMark*)::CoTaskMemAlloc(sizeof(WaterMark) * watermarksSize);
				for (int j = 0; j < watermarksSize; j++) 
				{
					auto info = watermarks[j];
					pWM[j].text = helper::allocStrInComMem(helper::utf82utf16(info.text));
					pWM[j].fontName = helper::allocStrInComMem(helper::utf82utf16(info.fontName));
					pWM[j].fontColor = helper::allocStrInComMem(helper::utf82utf16(info.fontColor));
					pWM[j].fontSize = info.fontSize;
					pWM[j].transparency = info.transparency;
					pWM[j].rotation = info.rotation;
					pWM[j].repeat = info.repeat;
				}
				pA[i].watermarks = pWM;
				pA[i].watermarkLenth = watermarksSize;
			}
			else {
				pA[i].watermarks = nullptr;
				pA[i].watermarkLenth = 0;
			}
		}

		*pArray = pA;
		*pArrSize = size;
	}
	else {
		*pArray = nullptr;
		*pArrSize = 0;
	}

	// rights
	std::size_t rightsSize = _rights.size();
	if (rightsSize > 0) {
		int* pRights = (int*)::CoTaskMemAlloc(sizeof(int) * rightsSize);
		for (int i = 0; i < rightsSize; i++) {
			pRights[i] = (int)_rights[i];
		}

		*pArrRights = pRights;
		*pArrRightsSize = rightsSize;
	}
	else {
		*pArrRights = nullptr;
		*pArrRightsSize = 0;
	}

	// watermark info
	watermark->text = helper::allocStrInComMem(helper::utf82utf16(_watermark.text));
	watermark->fontName = helper::allocStrInComMem(helper::utf82utf16(_watermark.fontName));
	watermark->fontColor = helper::allocStrInComMem(helper::utf82utf16(_watermark.fontColor));
	watermark->fontSize = _watermark.fontSize;
	watermark->transparency = _watermark.transparency;
	watermark->rotation = _watermark.rotation;
	watermark->repeat = _watermark.repeat;

	// Expiration
	expiration->type = (ExpiryType)_expiration.type;
	expiration->start = _expiration.start;
	expiration->end = _expiration.end;

	*tags = helper::allocStrInComMem(helper::utf82utf16(_tags));
	*tokenGroup = helper::allocStrInComMem(helper::utf82utf16(_tokengroup));
	*creatorId = helper::allocStrInComMem(helper::utf82utf16(_creatorid));
	*infoExt = helper::allocStrInComMem(helper::utf82utf16(_infoext));
	*attributes = _attributes;
	*isRPMFolder = _isRPMFolder;
	*isNxlFile = _isNxlFile;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_GetFileRights(HANDLE hSession, const wchar_t* filePath, int ** pprights, int * pLen,
	WaterMark * pWaterMark, int option)
{
	OutputDebugStringA("call SDWL_RPM_GetFileRights\n");
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (filePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsAndWatermarks;
	auto rt = g_RmsIns->RPMGetFileRights(filePath, rightsAndWatermarks, option);
	if (!rt) {
		return rt.GetCode();
	}
	if (rightsAndWatermarks.size() == 0) {
		*pprights = NULL;
		*pLen = 0;
		return SDWL_SUCCESS;
	}
	int size = (int)rightsAndWatermarks.size();
	*pLen = size;
	int* buf = (int*)::CoTaskMemAlloc(sizeof(int*)*size);
	bool bFilledWaterMark = false;
	// fill data, with ugly code, we only care abour the first watermark valuse and ignore others
	for (int i = 0; i < size; i++) {
		auto& cur = rightsAndWatermarks[i];
		buf[i] = cur.first;
		if (!bFilledWaterMark && cur.second.size() > 0) {
			WaterMark* buf = pWaterMark;
			{
				auto& wm = cur.second[0];
				buf->text = helper::allocStrInComMem(helper::utf82utf16(wm.text));
				buf->fontName = helper::allocStrInComMem(helper::utf82utf16(wm.fontName));
				buf->fontColor = helper::allocStrInComMem(helper::utf82utf16(wm.fontColor));
				buf->repeat = wm.repeat;
				buf->fontSize = wm.fontSize;
				buf->rotation = wm.rotation;
				buf->transparency = wm.transparency;
			}
		}
	}

	*pprights = buf;
	return SDWL_SUCCESS;

}

NXSDK_API DWORD SDWL_RPM_ReadFileTags(HANDLE hSession, const wchar_t* filePath, wchar_t** tags)
{
	OutputDebugStringA("call SDWL_RPM_ReadFileTags\n");
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (filePath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	std::wstring tag = L"";
	auto rt = g_RmsIns->RPMReadFileTags(filePath, tag);
	if (!rt) {
		return rt.GetCode();
	}

	*tags = helper::allocStrInComMem(tag);

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_IsRPMDriverExist(HANDLE hSession, bool* pResult)
{
	OutputDebugStringA("call SDWL_RPM_IsRPMDriverExist to check RPM driver\n");
	

	// sanity check
	if (g_RmsIns == NULL ) {
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}

	*pResult=g_RmsIns->IsRPMDriverExist();

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_AddRPMDir(HANDLE hSession, const wchar_t* path, uint32_t option, const wchar_t* filetags)
{
	OutputDebugStringA("call SDWL_RPM_AddRPMDir to set RPM folder\n");
	// sanity check
	if (g_RmsIns == NULL ) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}

	if (!g_RmsIns->IsRPMDriverExist()) {
		return SDWL_INTERNAL_ERROR;
	}
	
	// padding tags
	std::string t(helper::utf162utf8(filetags));
	auto rt=g_RmsIns->AddRPMDir(path,option, t);
    if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_RemoveRPMDir(HANDLE hSession, const wchar_t* path, bool bForce, wchar_t** msg)
{
	OutputDebugStringA("call SDWL_RPM_RemoveRPMDir to remove RPM folder\n");
	// sanity check
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}
	if (!g_RmsIns->IsRPMDriverExist()) {
		return SDWL_INTERNAL_ERROR;
	}
	
	// do
	auto rt= g_RmsIns->RemoveRPMDir(path, bForce);
	if (!rt) {
		*msg = helper::allocStrInComMem(helper::utf82utf16(rt.GetMsg()));
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_RegisterApp(HANDLE hSession, const wchar_t * appPath)
{
	OutputDebugStringA("call SDWL_RPM_RegisterApp\n");
	// sanity check
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}
	if (appPath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt=g_RmsIns->RPMRegisterApp(appPath, gRPM_Security);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_NotifyRMXStatus(HANDLE hSession, bool running)
{
	OutputDebugStringA("call SDWL_RPM_NotifyRMXStatus\n");
	// sanity check
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_RmsIns->RPMNotifyRMXStatus(running, gRPM_Security);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_UnregisterApp(HANDLE hSession, const wchar_t * appPath)
{
	OutputDebugStringA("call SDWL_RPM_UnregisterApp");
	// sanity check
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}
	if (appPath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_RmsIns->RPMUnregisterApp(appPath, gRPM_Security);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_AddTrustedApp(HANDLE hSession, const wchar_t* appPath)
{
	OutputDebugStringA("call SDWL_RPM_AddTrustedApp");
	// sanity check
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_RmsIns->RPMAddTrustedApp(appPath, gRPM_Security);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_AddTrustedProcess(HANDLE hSession, int pid)
{
	OutputDebugStringA("call SDWL_RPM_AddTrustedProcess");
	// sanity check
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_RmsIns->RPMAddTrustedProcess(pid, gRPM_Security);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_RemoveTrustedProcess(HANDLE hSession, int pid)
{
	OutputDebugStringA("call SDWL_RPM_RemoveTrustedProcess");
	// sanity check
	if (g_RmsIns == NULL) {
		// not found
		return SDWL_INTERNAL_ERROR;
	}
	if (g_RmsIns != hSession) {
		return SDWL_INTERNAL_ERROR;
	}
	auto rt = g_RmsIns->RPMRemoveTrustedProcess(pid, gRPM_Security);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_EditFile(HANDLE hSession, const wchar_t * srcNxlPath, wchar_t ** outSrcPath)
{
	OutputDebugStringA("call SDWL_RPM_EditFile\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	std::wstring outPath = L"";

	auto rt = g_RmsIns->RPMEditCopyFile(srcNxlPath, outPath);
	if (!rt) {
		return rt.GetCode();
	}

	*outSrcPath = helper::allocStrInComMem(outPath);
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_EditSaveFile(HANDLE hSession, const wchar_t* filePath, const wchar_t* originalNxlFilePath, bool deleteSource, UINT32 exitmode)
{
	OutputDebugStringA("call SDWL_RPM_EditSaveFile\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_RmsIns->RPMEditSaveFile(filePath, originalNxlFilePath, deleteSource, exitmode);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_DeleteFile(HANDLE hSession, const wchar_t * srcNxlPath)
{
	OutputDebugStringA("call SDWL_RPM_DeleteFile\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_RmsIns->RPMDeleteFile(srcNxlPath);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPMDeleteFolder(HANDLE hSession, const wchar_t * srcFolderPath)
{
	OutputDebugStringA("call SDWL_RPMDeleteFolder\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_RmsIns->RPMDeleteFolder(srcFolderPath);
	if (!rt) {
		return rt.GetCode();
	}
	return SDWL_SUCCESS;
}


NXSDK_API DWORD SDWL_RPM_IsSafeFolder(HANDLE hSession, const wchar_t* srcNxlPath, bool* outIsSafeFolder, int* option, wchar_t** tags)
{
	OutputDebugStringA("call SDWL_RPM_IsSafeFolder\n");
	if (g_RmsIns == NULL) {
		return SDWL_INTERNAL_ERROR;
	}
	if (srcNxlPath == NULL) {
		return SDWL_INTERNAL_ERROR;
	}

	uint32_t dirStat = 0;
	SDRmRPMFolderOption RPMFolderOption;
	std::wstring fileTags;
	auto rt = g_RmsIns->IsRPMFolder(srcNxlPath, &dirStat, &RPMFolderOption, fileTags);
	if (!rt) {
		return rt.GetCode();
	}
	*option = RPMFolderOption;
	*tags = helper::allocStrInComMem(fileTags);
	*outIsSafeFolder = (dirStat & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)) ? true:false;

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_GetFileStatus(HANDLE hSession, const wchar_t* wstrFilePath, uint32_t*  outDirstatus, bool * outFilestatus)
{
	if (g_RmsIns == NULL)
	{
		return false;
	}

	uint32_t dirstatus = 0;
	bool filestatus = false;

	SDWLResult rt = g_RmsIns->RPMGetFileStatus(wstrFilePath, &dirstatus, &filestatus);
	if (!rt) {
		return rt.GetCode();
	}

	*outDirstatus = dirstatus;
	*outFilestatus = filestatus;

	return SDWL_SUCCESS;
}


NXSDK_API DWORD SDWL_RPM_RequestLogin(HANDLE hSession, const wchar_t* callbackCmd, const wchar_t* callbackCmdPara)
{
	OutputDebugStringA("call SDWL_RPM_RequestLogin \n");
	if (g_RmsIns == NULL || callbackCmd == NULL)
	{
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_RmsIns->RPMRequestLogin(callbackCmd, callbackCmdPara);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_RequestLogout(HANDLE hSession, bool* isAllow, UINT32 option)
{
	OutputDebugStringA("call SDWL_RPM_RequestLogout \n");
	if (g_RmsIns == NULL)
	{
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_RmsIns->RPMRequestLogout(isAllow, option);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_NotifyMessage(HANDLE hSession, const wchar_t* app, const wchar_t* target, const wchar_t* message,
	UINT32 msgtype, const wchar_t* operation, UINT32 result, UINT32 fileStatus)
{
	OutputDebugStringA("call SDWL_RPM_NotifyMessage \n");
	if (g_RmsIns == NULL)
	{
		return SDWL_INTERNAL_ERROR;
	}

	if (app == NULL || target == NULL || operation == NULL || message == NULL)
	{
		return SDWL_INTERNAL_ERROR;
	}

	auto rt = g_RmsIns->RPMNotifyMessage(app, target, message, msgtype, operation, result, fileStatus);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}


NXSDK_API DWORD SDWL_RPM_RegisterFileAssociation(HANDLE hSession, const wchar_t* fileextension, const wchar_t* apppath) {
	OutputDebugStringA("call SDWL_RPM_RegisterFileAssociation\n");
	if (NULL == g_RmsIns)
	{
		return false;
	}
	auto rt = g_RmsIns->RPMRegisterFileAssociation(fileextension, apppath, gRPM_Security);
	if (!rt) {
		return rt.GetCode();
	}

	return SDWL_SUCCESS;
}

NXSDK_API DWORD SDWL_RPM_CopyFile(HANDLE hSession, const wchar_t* srcPath, const wchar_t* destPath, bool isDeleteSource)
{
  OutputDebugStringA("call SDWL_RPM_CopyFile\n");
  if (g_RmsIns == NULL) {
    return SDWL_INTERNAL_ERROR;
  }

  auto rt = g_RmsIns->RPMCopyFile(srcPath, destPath, isDeleteSource);
  if (!rt) {
    return rt.GetCode();
  }

  return SDWL_SUCCESS;
}


NXSDK_API bool SDWL_Register_SetValue(HANDLE hSession, HKEY hRoot, const wchar_t* strKey, const wchar_t* strItemName, UINT32 u32ItemValue)
{
	OutputDebugStringA("call SDWL_Register_SetValue\n");

	if (NULL == g_RmsIns)
	{
		return false;
	}

	SkyDRM::CRegistryServiceEntry serviceEntry(g_RmsIns, gRPM_Security);
	SDWLResult res = serviceEntry.set_value(hRoot, strKey, strItemName, u32ItemValue);
	
	bool result = false;
	if (0 == res.GetCode()) 
	{
		result = true;
	}
	return result;
}

#pragma endregion // RPM_DRIVER


#pragma region SysHelper

// call win32
			    
NXSDK_API DWORD SDWL_SYSHELPER_MonitorRegValueDeleted(const wchar_t* regValuename,RegChangeCallback callback)
{
	DWORD rt = SDWL_SUCCESS;
	//Notify the caller of changes to a value of the key. 
	//This can include adding or deleting a value, or changing an existing value.
	DWORD  dwFilter = REG_NOTIFY_CHANGE_LAST_SET;
	const wchar_t* SubKey = L"Software\\NextLabs\\SkyDRM\\Session";
	HKEY hkSession = 0;
	LONG   lErrorCode;

	// sanity check
	if (NULL == callback) {
		return SDWL_INTERNAL_ERROR;
	}
	//
	//  moniter
	//
	bool isContinue = true;
	while (isContinue)
	{
		
		// open key as Change Notify used
		lErrorCode = ::RegOpenKeyExW(HKEY_CURRENT_USER,
			SubKey, 0, KEY_NOTIFY | KEY_QUERY_VALUE , &hkSession);

		if (lErrorCode != ERROR_SUCCESS) {
			rt= SDWL_INTERNAL_ERROR;
			break;
		}

		// monitor changed
		lErrorCode = ::RegNotifyChangeKeyValue(hkSession, false, dwFilter, NULL, FALSE);
		if (lErrorCode != ERROR_SUCCESS) {
			// release res
			rt = SDWL_INTERNAL_ERROR;
			break;
		}

		// whether regKeyname has been deleted
		if (helper::IsRegValueExist(hkSession, regValuename)) {
			continue;
		}
		
		// notify
		callback(helper::allocStrInComMem(regValuename));
		isContinue = false;
		rt = SDWL_SUCCESS;

		// release res
		RegCloseKey(hkSession);
	}
	   	 
	return rt;
}
#pragma endregion
