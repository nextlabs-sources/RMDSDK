#include "stdafx.h"
#include "nxversion.h"
#include "common/celog2/celog.h"
#include "rmccore/base/coreinit.h"
#include "SDRmcInstance.h"
#include "SDLError.h"
#include "SDLResult.h"
#include "SDLAPI.h"

#include<Shlwapi.h>


#define CELOG_CUR_MODULE "rmdsdk"
#define CELOG_CUR_FILE CELOG_FILEPATH_SOURCES_SDWL_SDWRMCLIB_SDLAPI_CPP


using namespace SkyDRM;

#define CLIENT_ID		""


void SDWLibInit(void)
{
	CELog_Init();
	CoreInit("", "");

	CELOG_LOG(CELOG_CRITICAL, L"\n");
	CELOG_LOG(CELOG_CRITICAL, L"===============================================================\n");
	CELOG_LOG(CELOG_CRITICAL, L"*                  SkyDRM Client SDK Library                  *\n");
	CELOG_LOG(CELOG_CRITICAL, L"*                     Version %02d.%02d.%05d                     *\n", VERSION_MAJOR, VERSION_MINOR, BUILD_NUMBER);
	CELOG_LOG(CELOG_CRITICAL, L"===============================================================\n");
	CELOG_LOG(CELOG_CRITICAL, L"\n");
}

void SDWLibCleanup(void)
{
	CELOG_LOG(CELOG_CRITICAL, L"SkyDRM Client SDK Library cleaning up\n");

	CoreCleanup();
	CELog_Destroy();
}

DWORD SDWLibGetVersion(void)
{
	return SDWIN_LIB_VERSION_NUMBER;
}

SDWLResult SDWLibCreateInstance(ISDRmcInstance ** pInstance)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);
	CSDRmcInstance *p = new CSDRmcInstance();
	if (NULL == p) {
		res = RESULT2(SDWL_NOT_ENOUGH_MEMORY, "allocate momory for SDRmcInstance failed!");
	}

	*pInstance = p;
	CELOG_LOG(CELOG_INFO, L"*pInstance=%p\n", *pInstance);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult SDWLibCreateSDRmcInstance(const WCHAR * sdklibfolder, const WCHAR * tempfolder, ISDRmcInstance ** pInstance, const char * clientid, uint32_t id)
{
	return SDWLibCreateSDRmcInstance("", 0, 0, 0, sdklibfolder, tempfolder, pInstance, clientid, id);
}

SDWLResult SDWLibCreateSDRmcInstance(const CHAR * productName, uint32_t productMajorVer, uint32_t productMinorVer, uint32_t productBuild, const WCHAR * sdklibfolder, const WCHAR * tempfolder, ISDRmcInstance ** pInstance, const char * clientid, uint32_t id)
{
	CELOG_ENTER;

	CELOG_LOG(CELOG_INFO, L"Client product name=\"%hs\", version=%lu.%lu.%lu\n", productName, productMajorVer, productMinorVer, productBuild);
	CELOG_LOG(CELOG_INFO, L"sdklibfolder=%s, tempfolder=%s, clientid=%hs, id=%u\n", sdklibfolder, tempfolder, clientid, id);
	SDWLResult res = RESULT(0);

	// must have tempfolder, do not care sdk lib folder. The sdklibfolder folder should be there if we use policy envalution
	if (!PathIsDirectoryW(tempfolder)) {
		std::string msg = "tempfolder path not found" + std::string((CHAR *)tempfolder);
		res = RESULT2(ERROR_PATH_NOT_FOUND, msg.c_str());
		CELOG_RETURN_VAL_T(res);
	}

	CSDRmcInstance *p;
	if (NULL == clientid) {
		p = new CSDRmcInstance(productName, productMajorVer, productMinorVer, productBuild, sdklibfolder, tempfolder, CLIENT_ID, (RMCCORE::RMPlatformID)id);
	}
	else {
		p = new CSDRmcInstance(productName, productMajorVer, productMinorVer, productBuild, sdklibfolder, tempfolder, clientid, (RMCCORE::RMPlatformID)id);
	}
	

	if (NULL == p) {
		res = RESULT2(SDWL_NOT_ENOUGH_MEMORY, "allocate momory for SDRmcInstance failed!");
	}

	*pInstance = p;
	CELOG_LOG(CELOG_INFO, L"*pInstance=%p\n", *pInstance);
	CELOG_RETURN_VAL_T(res);
}

void SDWLibDeleteRmcInstance(ISDRmcInstance	*pInstance) 
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"pInstance=%p\n", pInstance);
	CSDRmcInstance * p = (CSDRmcInstance *)pInstance;
	if (NULL != p) {
		//p->Save();
		delete p;
	}

	CELOG_RETURN;
}

LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
{
	strValue = strDefaultValue;
	WCHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		strValue = szBuffer;
	}
	return nError;
}


SDWLResult RPMGetCurrentLoggedInUser(std::string &passcode, ISDRmcInstance *&pInstance, ISDRmTenant *&pTenant, ISDRmUser *&puser)
{
	CELOG_ENTER;

	std::wstring sdklibfolder;
	std::wstring workingfolder;
	std::wstring tempfolder;
	std::wstring router;
	std::wstring tenant;

	pInstance = new CSDRmcInstance();
	bool blogin = false;
	SDWLResult res = pInstance->RPMGetCurrentUserInfo(router, tenant, workingfolder, tempfolder, sdklibfolder, blogin);
	if (!res)
		CELOG_RETURN_VAL_T(res);

	if (blogin == false)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

	delete pInstance;
	pInstance = NULL;

	if (workingfolder.size() == 0 && tempfolder.size() == 0 && sdklibfolder.size() == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

	res = SDWLibCreateSDRmcInstance("", 0, 0, 0, sdklibfolder.c_str(), tempfolder.c_str(), &pInstance);
	if (!res)
		CELOG_RETURN_VAL_T(res);

	res = pInstance->Initialize(workingfolder, router, tenant);
	if (!res)
		CELOG_RETURN_VAL_T(res);

	res = pInstance->RPMGetLoginUser(passcode, &puser);
	if (!res)
		CELOG_RETURN_VAL_T(res);

	res = pInstance->GetCurrentTenant(&pTenant);
	if (!res)
		CELOG_RETURN_VAL_T(res);

	// we will not initialize RPM related login as we suppose external/3rd party application already did this after login

	CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
}