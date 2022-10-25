#include "stdafx.h"
#include <set>
#include <string>
#include <algorithm>
#include "common/celog2/celog.h"
#include "nudf/ntapi.hpp"
#include "SDRmcInstance.h"
#include "Shlwapi.h"

#include "Network/http_client.h"
#include "Winutil/securefile.h"
#include "Winutil/host.h"

#include "rmccore/network/httpConst.h"
#include "Crypt/sha.h"
#include <experimental/filesystem>
#include "Winutil/inifile.h"
#include "Winutil/tstdlibs.h"
#include "./Watermark/watermark.h"
#include <Shlobj.h>
#include "RegistryServiceEntry.h"
#include "nudf\winutil.hpp"



#define CELOG_CUR_MODULE "rmdsdk"
#define CELOG_CUR_FILE CELOG_FILEPATH_SOURCES_SDWL_SDWRMCLIB_SDRMCINSTANCE_CPP

using namespace SkyDRM;
using namespace RMCCORE;
using namespace NX::REST::http;

#define NXRMC_TENANT_HISTORY_FILENAME	L"Tenant.history"
#define NXRMC_INSTANCE_HISTORY_FILENAME	L"Instance.history"
#define NXRMC_USER_INFO_FILENAME		L"User.history"
#define NXRMC_USER_LOGIN_FILENAME		L"User.login"
 
#define FILE_VERSION_ELEMENT_NAME			"Version"
#define FILE_TENTANTS_KEY_NAME				"Tenants"
#define FILE_USER_KEY_NAME					"User"
#define FILE_INSTANCE_KEY_NAME				"Instance"
#define FILE_USEREMAIL_ELEMENT_NAME			"UserEmail"
#define FILE_USERID_ELEMENT_NAME			"UserID"
#define FILE_TENTANTID_ELEMENT_NAME			"Tenant"
#define FILE_ROUTER_ELEMENT_NAME			"Router"
#define FILE_RMSURL_ELEMENT_NAME			"ServerURL"
#define FILE_CLIENTID_ELEMENT_NAME			"ClientID"
#define FILE_SYSTEM_KEY_NAME				"System"

// This is the SHA-256 checksum of the shared secret for the security string.
static const std::vector<unsigned char> secret_sha256 = {
	0x70, 0x32, 0x13, 0x06, 0xFC, 0xE0, 0x09, 0x16,
	0x0E, 0x20, 0xA6, 0x6D, 0x33, 0x1F, 0x1C, 0x75,
	0xDF, 0xF1, 0xD3, 0xA0, 0xCF, 0x49, 0x41, 0xF3,
	0x97, 0xEA, 0x86, 0x7B, 0xAE, 0x0D, 0xF6, 0x12
};

bool IsSecurityStringCorrect(const std::string &security)
{
	std::vector<unsigned char> sha256_result;
	sha256_result.resize(256 / CHAR_BIT, 0);
	if (!NX::crypt::CreateSha256((const unsigned char *)security.data(), (ULONG)security.size(), sha256_result.data()))
	{
		return false;
	}

	return sha256_result == secret_sha256;
}

static std::wstring PrepareDeviceId()
{
	const std::wstring& id = NX::win::GetLocalHostInfo().GetFQDN().empty() ? NX::win::GetLocalHostInfo().GetHostName() : NX::win::GetLocalHostInfo().GetFQDN();
	std::wstring encodedId;
	std::for_each(id.begin(), id.end(), [&](const wchar_t c) {
		if (c > 31 && c < 127) {
			encodedId.append(&c, 1);
		}
		else {
			wchar_t wz[16] = { 0 };
			swprintf_s(wz, L"\\u%04X", c);
			encodedId.append(wz);
		}
	});
	return encodedId;
}

static std::map<std::wstring, std::wstring> get_environment_variables()
{
	std::map<std::wstring, std::wstring> variables;

	const wchar_t* sys_envs = ::GetEnvironmentStringsW();
	if (NULL == sys_envs) {
		return variables;
	}

	while (sys_envs[0] != L'\0') {
		std::wstring line(sys_envs);
		sys_envs += (line.length() + 1);
		auto pos = line.find(L'=');
		std::wstring var_name;
		std::wstring var_value;
		if (pos != std::wstring::npos) {
			var_name = line.substr(0, pos);
			var_value = line.substr(pos + 1);
		}
		else {
			var_name = line;
		}
		if (0 == _wcsicmp(var_name.c_str(), L"APPDATA")
			|| 0 == _wcsicmp(var_name.c_str(), L"LOCALAPPDATA")
			|| 0 == _wcsicmp(var_name.c_str(), L"USERPROFILE")
			|| 0 == _wcsicmp(var_name.c_str(), L"USERNAME")
			) {
			std::transform(var_name.begin(), var_name.end(), var_name.begin(), toupper);
		}
		std::transform(var_value.begin(), var_value.end(), var_value.begin(), tolower);
		variables[var_name] = var_value;
	}

	return std::move(variables);
}

CSDRmcInstance::CSDRmcInstance()
{
	::InitializeCriticalSection(&_lock);
}

CSDRmcInstance::CSDRmcInstance(const std::string &productName, uint32_t productMajorVer, uint32_t productMinorVer, uint32_t productBuild, const std::wstring &sdklibfolder, const std::wstring &tempfolder, const std::string &clientid, RMPlatformID id) : ISDRmcInstance(), m_strTempFolder(tempfolder)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"sdklibfolder=%s, tempfolder=%s, clientid=%hs, id=%d\n", sdklibfolder.c_str(), tempfolder.c_str(), clientid.c_str(), id);
	SDWLResult ret = RESULT(0);
	::InitializeCriticalSection(&_lock);
	PrepareClientKey();
	if (PathIsDirectoryW(tempfolder.data())) {
		m_strTempFolder = tempfolder;
		ret = LoadTenantInfo(m_strTempFolder);
		if (ret) {
			ret = LoadInstanceInfo();
		}
	}

	if (!ret) {
		m_strDeviceID = PrepareDeviceId();
		m_sysparam.SetDeviceID(NX::conv::to_string(m_strDeviceID));
		m_sysparam.SetPlatformID(id);

		if (clientid.length() != 0) {
			m_clientid = clientid;
			m_sysparam.SetClientID(clientid);
		}
		m_sysparam.SetProduct(RMProduct(productName, productMajorVer, productMinorVer, productBuild));
		m_CurUser.SetSystemParameters(m_sysparam);
		m_CurUser.SetSDKLibFolder(sdklibfolder);
		m_strSDKLibFolder = sdklibfolder;
	}

	CELOG_RETURN;
}

void CSDRmcInstance::Init(const std::string &productName, uint32_t productMajorVer, uint32_t productMinorVer, uint32_t productBuild, const std::wstring &sdklibfolder, const std::wstring &tempfolder, const std::string &clientid, RMPlatformID id)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"sdklibfolder=%s, tempfolder=%s, clientid=%hs, id=%d\n", sdklibfolder.c_str(), tempfolder.c_str(), clientid.c_str(), id);
	SDWLResult ret = RESULT(0);
	PrepareClientKey();
	if (PathIsDirectoryW(tempfolder.data())) {
		m_strTempFolder = tempfolder;
		ret = LoadTenantInfo(m_strTempFolder);
		if (ret) {
			ret = LoadInstanceInfo();
		}
	}

	if (!ret) {
		m_strDeviceID = PrepareDeviceId();
		m_sysparam.SetDeviceID(NX::conv::to_string(m_strDeviceID));
		m_sysparam.SetPlatformID(id);

		if (clientid.length() != 0) {
			m_clientid = clientid;
			m_sysparam.SetClientID(clientid);
		}
		m_sysparam.SetProduct(RMProduct(productName, productMajorVer, productMinorVer, productBuild));
		m_CurUser.SetSystemParameters(m_sysparam);
		m_CurUser.SetSDKLibFolder(sdklibfolder);
		m_strSDKLibFolder = sdklibfolder;
	}

	CELOG_RETURN;
}

CSDRmcInstance::~CSDRmcInstance()
{
	CELOG_ENTER;
	::DeleteCriticalSection(&_lock);
	//SDWLResult res = Save();
	CELOG_RETURN;
}

bool CSDRmcInstance::IsRPMDriverExist()
{
	return m_drvcoreMgr.isDriverExist();
}

bool CSDRmcInstance::IsFileTagValid(
    const std::string& tag,
    std::string& outTag)
{
    if (tag.empty())
        return true;

    bool bRet = false;
    try
    {
        nlohmann::json root = nlohmann::json::parse(tag);
        if (root.is_object())
        {
            bRet = true;
            outTag = root.dump();
        }
    }
    catch (...)
    {
    }

    return bRet;
}



SDWLResult CSDRmcInstance::AddRPMDir(const std::wstring &path, uint32_t option, const std::string &fileTags)
{
	CELOG_ENTER;
	if (!m_CurUser.IsUserLogin())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));
	CELOG_LOG(CELOG_INFO, L"AddRPMDir: path: %s\n", path.c_str());

    NX::fs::dos_fullfilepath fpath(path);
	if (!PathIsDirectory(fpath.global_dos_path().c_str()))
		CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "path is not directory"));

	SDWLResult res;

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	// Don't allow adding any Sanctuary dir, its ancestor, or its descendant
	// as RPM dir.
	uint32_t sancDirStatus;
	std::wstring sancFileTags;
	res = IsSanctuaryFolder(path, &sancDirStatus, sancFileTags);
	if (!res)
		CELOG_RETURN_VAL_T(res);
	if (sancDirStatus & RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "Adding Sanctuary directory as RPM directory is not allowed."));
	if (sancDirStatus & RPM_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "Adding ancestor of Sanctuary directory as RPM directory is not allowed."));
	if (sancDirStatus & RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "Adding descendant of Sanctuary directory as RPM directory is not allowed."));
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	std::size_t found = -1;

	std::map<std::wstring, std::wstring> envvars = get_environment_variables();
	found = path.find(L":");
	if (found == std::string::npos)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "path is not valid"));

	if (_wcsnicmp(fpath.path().c_str(), L"c:\\", fpath.path().size()) == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\ is not allowed"));

    if (_wcsnicmp(fpath.path().c_str(), L"c:", fpath.path().size()) == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c: is not allowed"));


    if (_wcsnicmp(fpath.path().c_str(), envvars[L"USERPROFILE"].c_str(), fpath.path().size()) == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "UserProfile folder is not allowed"));


    if (_wcsnicmp(fpath.path().c_str(), L"c:\\users", fpath.path().size()) == 0)
            CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\users is not allowed"));

    if (_wcsnicmp(fpath.path().c_str(), L"c:\\program files", fpath.path().size()) == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\program is not allowed"));

    if (_wcsnicmp(fpath.path().c_str(), L"c:\\program files (x86)", fpath.path().size()) == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\program (x86) files is not allowed"));

    if (_wcsnicmp(fpath.path().c_str(), L"c:\\programdata", fpath.path().size()) == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\programdata is not allowed"));

    if (_wcsnicmp(fpath.path().c_str(), L"c:\\windows\\system32", fpath.path().size()) == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\windows\\system32 is not allowed"));

    if (_wcsnicmp(fpath.path().c_str(), L"c:\\windows", fpath.path().size()) == 0)
        CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\windows is not allowed"));

    std::string ttags;
    if (fileTags.size() > 0 && !IsFileTagValid(fileTags, ttags))
    {
        std::string message = "Invalid Json format : " + fileTags;
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, message));
    }

	RPMResetNXLinFolder(fpath.path());

	option = SDRmRPMFolderOption::RPMFOLDER_NORMAL | 
		(option & (SDRmRPMFolderOption::RPMFOLDER_API | SDRmRPMFolderOption::RPMFOLDER_EXT | SDRmRPMFolderOption::RPMFOLDER_NORMAL | 
			SDRmRPMFolderOption::RPMFOLDER_OVERWRITE | SDRmRPMFolderOption::RPMFOLDER_AUTOPROTECT | SDRmRPMFolderOption::RPMFOLDER_MYFOLDER));
	res = m_drvcoreMgr.insert_dir(fpath.path(), (uint32_t)option, NX::conv::utf8toutf16(fileTags));

	CELOG_RETURN_VAL_T(res);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool IsHaveHardlinksForFilesInDir(const std::wstring dirPath) {
	bool ret = false;

	NX::fs::dos_fullfilepath _fullpath(dirPath);
	std::wstring pattern = _fullpath.global_dos_path() + L"\\*";
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {

		do {
			if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) {
				continue;
			}

			std::wstring fnodePath = _fullpath.global_dos_path() + L"\\" + data.cFileName;
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				ret = IsHaveHardlinksForFilesInDir(fnodePath);
				if (ret) {
					break;
				}
			}
			// normal file
			else {
				HANDLE hFile = ::CreateFileW(fnodePath.c_str(), 0, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					BY_HANDLE_FILE_INFORMATION info;
					if (GetFileInformationByHandle(hFile, &info) && info.nNumberOfLinks > 1) { // Note: more than 1 if exist.
						ret = true;

						CloseHandle(hFile);
						break;
					}
					CloseHandle(hFile);
				}
			}

		} while (FindNextFile(hFind, &data) != 0);

		FindClose(hFind);
	}

	return ret;
}

SDWLResult CSDRmcInstance::AddSanctuaryDir(const std::wstring &path, const std::string &fileTags)
{
	CELOG_ENTER;
	if (!m_CurUser.IsUserLogin())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

	CELOG_LOG(CELOG_INFO, L"AddSanctuaryDir: path: %s\n", path.c_str());

	NX::fs::dos_fullfilepath fullPath(path);
	if (!PathIsDirectory(fullPath.global_dos_path().c_str()))
		CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "path is not directory"));

	// Don't allow adding any RPM dir, its ancestor, or its descendant as
	// Sanctuary dir.
	uint32_t rpmDirStatus;
	SDRmRPMFolderOption rpmOption;
	std::wstring rpmFileTags;
	SDWLResult res;
	res = IsRPMFolder(fullPath.path(), &rpmDirStatus, &rpmOption, rpmFileTags);
	if (!res)
		CELOG_RETURN_VAL_T(res);
	if (rpmDirStatus & RPM_SAFEDIRRELATION_SAFE_DIR)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "Adding RPM directory as Sanctuary directory is not allowed."));
	if (rpmDirStatus & RPM_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "Adding ancestor of RPM directory as Sanctuary directory is not allowed."));
	if (rpmDirStatus & RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "Adding descendant of RPM directory as Sanctuary directory is not allowed."));

	std::size_t found = -1;

	found = path.find(L":");
	if (found == std::string::npos)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "path is not valid"));

	std::wstring fpath = fullPath.path();
	std::transform(fpath.begin(), fpath.end(), fpath.begin(), ::tolower);
	found = fpath.find(L"/");
	if (found != std::string::npos)
		std::replace(fpath.begin(), fpath.end(), '/', '\\');

	if (fpath.compare(L"c:\\") == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c:\\ is not allowed"));

	found = fpath.rfind(L"\\");
	if (found == fpath.size() - 1)
		fpath.replace(found, 2, L"");

	if (fpath.compare(L"c:") == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "c: is not allowed"));

	// If exist some file with hard links in this directory, deny it. fix bug 66198
	if (IsHaveHardlinksForFilesInDir(fpath)) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "Adding the directory as Sanctuary directory is not allowed, since there are files with hard links in it."));
	}

	res = m_drvcoreMgr.insert_sanctuary_dir(fpath, NX::conv::utf8toutf16(fileTags));

	CELOG_RETURN_VAL_T(res);
}

#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

SDWLResult CSDRmcInstance::RemoveRPMDir(const std::wstring &path, bool bForce)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"RemoveRPMDir: path: %s\n", path.c_str());
	if (!m_CurUser.IsUserLogin())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

	NX::fs::dos_fullfilepath fpath(path);
	if (!PathIsDirectory(fpath.global_dos_path().c_str()))
		CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "The input path is not valid directory"));

	SDWLResult res = m_drvcoreMgr.remove_dir(fpath.path(), bForce);
	if (res)
	{
		/************ Save to registry *********************/

		HKEY hKey;
		if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\NextLabs\\SkyDRM", 0, KEY_ALL_ACCESS, &hKey))
		{
			CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegOpenKeyEx failed."));
		}
		DWORD dwValueType;
		BYTE* valueBuf;
		DWORD dwCbData;
		LSTATUS regresult;
		// Get value length first
		regresult = RegQueryValueEx(hKey, L"APIRPMFolder", NULL, &dwValueType, NULL, &dwCbData);
		if (ERROR_SUCCESS != regresult && ERROR_FILE_NOT_FOUND != regresult)
		{
			RegCloseKey(hKey);
			CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegQueryValueExW failed to get value length"));
		}
		if (ERROR_FILE_NOT_FOUND == regresult)
		{
			RegCloseKey(hKey);
			CELOG_RETURN_VAL_T(res);
		}
		// Get value.
		valueBuf = new BYTE[dwCbData + 2];
		if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"APIRPMFolder", NULL, &dwValueType, valueBuf, &dwCbData))
		{
			RegCloseKey(hKey);
			delete[] valueBuf;
			CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegQueryValueExW failed to get value"));
		}
		std::wstring rpmPath = L"";
		rpmPath.assign((wchar_t*)valueBuf);
		std::transform(rpmPath.begin(), rpmPath.end(), rpmPath.begin(), ::tolower);
		delete[] valueBuf;
		std::vector<std::string> existing_rpm;
		std::string fpath_utf8 = toUtf8(fpath.path());
		std::transform(fpath_utf8.begin(), fpath_utf8.end(), fpath_utf8.begin(), ::tolower);
		NX::conv::split_string(toUtf8(rpmPath), existing_rpm, ";");
		std::string new_rpms;
		for each (std::string _path in existing_rpm)
		{
			if (_path == fpath_utf8)
				continue;

			new_rpms += _path + ";";
		}
		std::wstring _szrpms = toUtf16(new_rpms);
		const wchar_t* cwPath = _szrpms.c_str();
		if (ERROR_SUCCESS != RegSetValueEx(hKey, L"APIRPMFolder", 0, REG_SZ, (byte*)cwPath, (DWORD)(wcslen(cwPath) + 1) * sizeof(wchar_t)))
		{
			RegCloseKey(hKey);
			CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegSetKeyValueW set APIRPM folder failed."));
		}
		if (ERROR_SUCCESS != RegCloseKey(hKey))
		{
			CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegCloseKey failed."));
		}
	}

	CELOG_RETURN_VAL_T(res);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
SDWLResult CSDRmcInstance::RemoveSanctuaryDir(const std::wstring &path)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"RemoveSanctuaryDir: path: %s\n", path.c_str());
	if (!m_CurUser.IsUserLogin())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

	NX::fs::dos_fullfilepath _fullpath(path);
	if (!PathIsDirectory(_fullpath.global_dos_path().c_str()))
		CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "The input path is not valid directory"));

	SDWLResult res = m_drvcoreMgr.remove_sanctuary_dir(_fullpath.path());

	CELOG_RETURN_VAL_T(res);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

SDWLResult CSDRmcInstance::GetRPMDir(std::vector<std::wstring> &paths, SDRmRPMFolderQuery option)
{
	CELOG_ENTER;
	if (!m_CurUser.IsUserLogin())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

	if (option == SDRmRPMFolderQuery::RPMFOLDER_MYAPIFOLDERS)
	{
        m_drvcoreMgr.get_rpm_folder(paths, SDRmRPMFolderQuery::RPMFOLDER_MYAPIFOLDERS);
    }
	else if (option == SDRmRPMFolderQuery::RPMFOLDER_ALLFOLDERS)
	{
        m_drvcoreMgr.get_rpm_folder(paths, SDRmRPMFolderQuery::RPMFOLDER_ALLFOLDERS);
	}
    else if (option == SDRmRPMFolderQuery::RPMFOLDER_MYFOLDERS)
    {
        m_drvcoreMgr.get_rpm_folder(paths, SDRmRPMFolderQuery::RPMFOLDER_MYFOLDERS);
    }
    else if (option == SDRmRPMFolderQuery::RPMFOLDER_MYRPMFOLDERS)
    {
        m_drvcoreMgr.get_rpm_folder(paths, SDRmRPMFolderQuery::RPMFOLDER_MYRPMFOLDERS);
    }

	CELOG_RETURN_VAL_T(RESULT2(SDWL_SUCCESS, ""));
}

SDWLResult CSDRmcInstance::SetRPMClientId(const std::string &clientid)
{
	if (clientid.size() == 0)
		return RESULT2(SDWL_INVALID_DATA, "Invalid clientid");

	std::wstring wclientid = NX::conv::utf8toutf16(clientid);
	return m_drvcoreMgr.set_clientid(wclientid);
}

SDWLResult CSDRmcInstance::SetRPMLoginResult(const std::string &jsonreturn, const std::string &security)
{
    // we are sure here we already do SDK Login
    // set client id here
    SetRPMClientId(m_sysparam.GetClientID());

    // We are sure SDK/Client login successfully before this call.
    // Then we need to sync the router/tenant to RPM service here also.
    SaveLoginInfoForSSO();

    return m_drvcoreMgr.set_login_result(jsonreturn, security);
}

SDWLResult CSDRmcInstance::RPMAddActivityLog(const std::string& strJson)
{
    return m_drvcoreMgr.add_activity_log(strJson);
}

SDWLResult CSDRmcInstance::RPMAddNXLMetadata(const std::string& strJson)
{
    return m_drvcoreMgr.add_nxl_metadata(strJson);
}

SDWLResult CSDRmcInstance::RPMLockFileForSync(const std::string& strJson)
{
    return m_drvcoreMgr.lock_file_forsync(strJson);
}

SDWLResult CSDRmcInstance::RPMRequestRunRegCmd(const std::string& reg_cmd, const std::string &security)
{
	::EnterCriticalSection(&_lock);
	SDWLResult res = m_drvcoreMgr.request_run_registry_command(reg_cmd, security);
	::LeaveCriticalSection(&_lock);
	return res;
}

SDWLResult CSDRmcInstance::RPMRegisterFileAssociation(const std::wstring& fileextension, const std::wstring& apppath, const std::string &security)
{
	std::wstring strExecutable(apppath);
	if (strExecutable.empty())
	{
		NX::win::file_association fileassociation(fileextension);
		strExecutable = fileassociation.get_executable();
		if (strExecutable.empty())
		{
			return RESULT2(SDWL_NOT_FOUND, "Can't find the file associatoin.");
		}
	}

	CRegistryServiceEntry entry(this, security);
	std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM\\nxrmhandler\\" + fileextension;
	return entry.set_value(HKEY_LOCAL_MACHINE, strKey, L"", strExecutable);
}

SDWLResult CSDRmcInstance::RPMUnRegisterFileAssociation(const std::wstring& fileextension, const std::string &security)
{
	CRegistryServiceEntry entry(this, security);
	std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM\\nxrmhandler\\" + fileextension;
	return entry.delete_key(HKEY_LOCAL_MACHINE, strKey);
}

SDWLResult CSDRmcInstance::RPMGetOpenedFileRights(std::map<std::wstring, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>>& mapOpenedFileRights, const std::string &security, std::wstring &ex_useremail, unsigned long ulProcessId)
{
	SDWLResult res = m_drvcoreMgr.request_opened_file_rights(mapOpenedFileRights, security, ex_useremail, ulProcessId);
	return res;
}

SDWLResult CSDRmcInstance::SetRPMUserAttr(const std::string& jsonreturn)
{
    return m_drvcoreMgr.set_user_attr(jsonreturn);
}

BOOL CSDRmcInstance::RPMIsAPIUser()
{
	if (m_drvcoreMgr.request_is_apiuser())
		return TRUE;

	return FALSE;
}

SDWLResult CSDRmcInstance::RPMLogout()
{
	return m_drvcoreMgr.logout();
}

SDWLResult CSDRmcInstance::SetRPMServiceStop(bool enable, const std::string &security)
{
	return m_drvcoreMgr.set_service_stop(enable, security);
}

SDWLResult CSDRmcInstance::StopPDP(const std::string &security)
{
	CELOG_ENTER;

	if (!IsSecurityStringCorrect(security))
	{
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid security string"));
	}

	CELOG_RETURN_VAL_T(m_PDP.StopPDPMan());
}

SDWLResult CSDRmcInstance::SetRPMDeleteCacheToken()
{
	return m_drvcoreMgr.delete_cheched_token();
}

SDWLResult CSDRmcInstance::RPMGetCurrentUserInfo(std::wstring& router, std::wstring& tenant, std::wstring& workingfolder, std::wstring& tempfolder, std::wstring& sdklibfolder, bool &blogin)
{
	return m_drvcoreMgr.get_user_info(router,  tenant, workingfolder, tempfolder, sdklibfolder, blogin);
}

SDWLResult CSDRmcInstance::RPMDeleteFile(const std::wstring &filepath)
{
	// for read-only file, we shall reset the read-only attribute first
	NX::fs::dos_fullfilepath _dosfilepath(filepath);

	std::wstring _filepath = _dosfilepath.global_dos_path();
	CELOG_LOG(CELOG_INFO, L"RPMDeleteFile filepath = %s\n", _filepath.c_str());
	DWORD attr = GetFileAttributesW(_filepath.c_str());
	DWORD dwErr = ::GetLastError();
	if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_READONLY))
	{
		attr &= ~FILE_ATTRIBUTE_READONLY;
		SetFileAttributes(_filepath.c_str(), attr);
	}
	
	//Fix http://bugs.cn.nextlabs.com/show_bug.cgi?id=57198 
	//Even input invalid file path, RPMDeleteFile() still return success 
	if ((INVALID_FILE_ATTRIBUTES == attr) && (ERROR_FILE_NOT_FOUND == dwErr))
	{
		return RESULT2(SDWL_PATH_NOT_FOUND, "The system cannot find the path specified.");
	}

	if ((INVALID_FILE_ATTRIBUTES == attr) && (ERROR_PATH_NOT_FOUND == dwErr))
	{
		return RESULT2(SDWL_PATH_NOT_FOUND, "The system cannot find the path specified.");
	}

	if ((INVALID_FILE_ATTRIBUTES == attr) && (ERROR_INVALID_NAME == dwErr))
	{
		return RESULT2(SDWL_PATH_NOT_FOUND, "The system cannot find the path specified.");
	}

	return m_drvcoreMgr.delete_file(_dosfilepath.path());
}

SDWLResult CSDRmcInstance::RPMDeleteFolder(const std::wstring &folderpath)
{
	// for read-only file, we shall reset the read-only attribute first
	NX::fs::dos_fullfilepath _folderpath(folderpath);
	CELOG_LOG(CELOG_INFO, L"RPMDeleteFolder folder path = %s\n", _folderpath.path().c_str());
	DWORD attr = GetFileAttributesW(_folderpath.global_dos_path().c_str());
	DWORD dwErr = ::GetLastError();
	if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_READONLY))
	{
		attr &= ~FILE_ATTRIBUTE_READONLY;
		SetFileAttributes(_folderpath.global_dos_path().c_str(), attr);
	}

	if ((INVALID_FILE_ATTRIBUTES == attr) && (ERROR_PATH_NOT_FOUND == dwErr))
	{
		return RESULT(SDWL_PATH_NOT_FOUND);
	}

	return m_drvcoreMgr.delete_folder(_folderpath.path());
}

SDWLResult CSDRmcInstance::RPMCopyFile(const std::wstring &srcpath, const std::wstring &destpath, bool deletesource)
{
	// for read-only file, we shall reset the read-only attribute first
	NX::fs::dos_fullfilepath _srcpath(srcpath);
	NX::fs::dos_fullfilepath _destpath(destpath);

	CELOG_LOG(CELOG_INFO, L"RPMCopyFile src-filepath = %s\n", _srcpath.path().c_str());
	CELOG_LOG(CELOG_INFO, L"RPMCopyFile dest-filepath = %s\n", _destpath.path().c_str());

	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(_srcpath.global_dos_path().c_str()))
	{
		// source file is invalid
		return RESULT2(SDWL_PATH_NOT_FOUND, "source file is invalid");
	}

	DWORD attr = GetFileAttributes(_destpath.global_dos_path().c_str());
    if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        // Dest file is a directory
        return RESULT2(SDWL_INVALID_DATA, "dest path is invalid");
    }

	if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_READONLY))
	{
		attr &= ~FILE_ATTRIBUTE_READONLY;
		SetFileAttributes(_destpath.global_dos_path().c_str(), attr);
	}

	return m_drvcoreMgr.copy_file(_srcpath.path(), _destpath.path(), deletesource);
}

SDWLResult CSDRmcInstance::RPMGetFileRights(const std::wstring &filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, int option)
{
	SDWLResult res;

	CELOG_LOG(CELOG_INFO, L"RPMGetFileRights filepath = %s\n", filepath.c_str());

	// will handle long path in the rpm service
	std::wstring fpath = NX::fs::dos_fullfilepath(filepath).path();

	res = m_drvcoreMgr.get_rights(fpath, rightsAndWatermarks, option); // option set to 1, get file rights set in NXL header for ad-hoc file
	if (res.GetCode() > 0)
		return res;

	return res;
}

SDWLResult CSDRmcInstance::RPMGetFileStatus(const std::wstring &filename, uint32_t* dirstatus, bool* filestatus)
{
	CELOG_LOG(CELOG_INFO, L"filename %s\n", filename.c_str());
	SDWLResult res;

	// will handle long path in the rpm service
	std::wstring fpath = NX::fs::dos_fullfilepath(filename).path();
	res = m_drvcoreMgr.get_file_status(fpath, dirstatus, filestatus);

	CELOG_LOG(CELOG_INFO, L"finish: dirstatus %u\n", *dirstatus);
	return res;
}

SDWLResult CSDRmcInstance::RPMGetFileInfo(const std::wstring &filepath, std::string &duid, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &userRightsAndWatermarks,
	std::vector<SDRmFileRight> &rights, SDR_WATERMARK_INFO &waterMark, SDR_Expiration &expiration, std::string &tags,
	std::string &tokengroup, std::string &creatorid, std::string &infoext, DWORD &attributes, DWORD &isRPMFolder, DWORD &isNXLFile, bool checkOwner)
{
	CELOG_LOG(CELOG_INFO, L"RPMGetFileInfo %s\n", filepath.c_str());
	SDWLResult res;

	// will handle long path in the rpm service
	std::wstring _fpath = NX::fs::dos_fullfilepath(filepath).path();
	res = m_drvcoreMgr.get_file_info(_fpath, duid, userRightsAndWatermarks, rights, waterMark, expiration, tags, tokengroup, creatorid, infoext,
		attributes, isRPMFolder, isNXLFile, checkOwner);

	CELOG_LOG(CELOG_INFO, L"finish: RPMGetFileInfo attributes %u\n", attributes);
	return res;
}

SDWLResult CSDRmcInstance::RPMSetFileAttributes(const std::wstring &nxlFilePath, DWORD dwFileAttributes)
{
	CELOG_LOG(CELOG_INFO, L"RPMSetFileAttributes %s\n", nxlFilePath.c_str());
	SDWLResult res;
	// will handle long path in the rpm service
	std::wstring _fpath = NX::fs::dos_fullfilepath(nxlFilePath).path();
	res = m_drvcoreMgr.set_file_attributes(_fpath, dwFileAttributes);
	CELOG_LOG(CELOG_INFO, L"finish: RPMSetFileAttributes\n");
	return res;
}

SDWLResult CSDRmcInstance::RPMGetFileAttributes(const std::wstring &nxlFilePath, DWORD &dwFileAttributes) {
	CELOG_LOG(CELOG_INFO, L"RPMGetFileAttributes %s\n", nxlFilePath.c_str());
	SDWLResult res;
	// will handle long path in the rpm service
	std::wstring _fpath = NX::fs::dos_fullfilepath(nxlFilePath).path();
	res = m_drvcoreMgr.get_file_attributes(_fpath, dwFileAttributes);
	CELOG_LOG(CELOG_INFO, L"finish: RPMGetFileAttributes\n");
	return res;
}

SDWLResult CSDRmcInstance::RPMWindowsEncryptFile(const std::wstring &FilePath) {
	CELOG_LOG(CELOG_INFO, L"RPMWindowsEncryptFile %s\n", FilePath.c_str());
	SDWLResult res;
	// will handle long path in the rpm service
	std::wstring _fpath = NX::fs::dos_fullfilepath(FilePath).path();
	res = m_drvcoreMgr.windows_encrypt_file(_fpath);
	CELOG_LOG(CELOG_INFO, L"finish: RPMWindowsEncryptFile\n");
	return res;
}

SDWLResult CSDRmcInstance::Initialize(const std::wstring &router, const std::wstring &tenant)
{
	SDWLResult res;
	res = CheckKeyManager();
	if (!res)
		return res;

	CSDRmTenant info(router, tenant);

	res = QueryTenantInfo(info);
	if (!res) {
		//No RMS information, query online
		res = QueryTenantServerInfo(info);
		if (!res) {
			return res;
		}
		m_arrTenant.push_back(info);
	}

	m_CurTenant = info;
	m_CurUser.SetTenant(m_CurTenant);

	//
	// Fix bug, clientID in m_sysparam will changed, need to tell m_user 
	// By osmond,
	//
	m_CurUser.SetSystemParameters(m_sysparam);
	//res = SetRPMRegKey(router, info.GetTenant());
	//res = m_drvcoreMgr.set_router_key(m_CurTenant.GetRouterURL(), m_CurTenant.GetTenant(), m_strWorkFolder, m_strTempFolder, m_strSDKLibFolder);

	return RESULT(0);
}

SDWLResult CSDRmcInstance::PrepareClientKey()
{
	// First, try to load client key from current machine's store
	SDWLResult res = m_keyManager.Load(true);
	if (!res) {
		// If there is no global client key, try to load it from current user's cert store
		res = m_keyManager.Load(false, true);
		if (!res) {
			return res;
		}
	}
	return RESULT(0);
}

SDWLResult CSDRmcInstance::Initialize(const std::wstring &workingfolder, const std::wstring &router, const std::wstring &tenant)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"workingfolder=%s, router=%s, tenant=%s\n", workingfolder.c_str(), router.c_str(), tenant.c_str());
	SDWLResult res;
	res = CheckKeyManager();
	if (!res)
		CELOG_RETURN_VAL_T(res);

	res = SetWorkingFolder(workingfolder);
	if (!res)
		CELOG_RETURN_VAL_T(res);

	LoadTenantInfo(m_strWorkFolder);
	LoadInstanceInfo();

	CELOG_RETURN_VAL_T(Initialize(router, tenant));
}

SDWLResult CSDRmcInstance::IsInitFinished(bool& finished)
{
	CELOG_ENTER;

	CELOG_RETURN_VAL_T(m_CurUser.IsInitFinished(finished));
}

SDWLResult CSDRmcInstance::CheckSoftwareUpdate(std::string &newVersionStr, std::string &downloadURL, std::string &checksum)
{
    CELOG_ENTER;

    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(m_CurTenant.GetRouterURL()));

    StringBodyRequest request(m_CurTenant.GetProductUpdateQuery(m_sysparam));
    StringResponse response;

    SDWLResult res = spConn->SendRequest(request, response);

    if (!res) {
        CELOG_RETURN_VAL_T(res);
    }

    unsigned short status = response.GetStatus();
    if (status != http::status_codes::OK.id) {
        CELOG_RETURN_VAL_T(RESULT2(SDWL_RMS_ERRORCODE_BASE + status, NX::conv::to_string(response.GetPhrase())));
    }

    const std::string& jsondata = response.GetBody();

    try {
        RMProduct prod = m_sysparam.GetProduct();
        RetValue ret = prod.ImportFromRMSResponse(jsondata);
        if (!ret && ret.GetCode() != RMCCORE_RMS_ERRORCODE_BASE + http::status_codes::NotModified.id)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

        m_sysparam.SetProduct(prod);
        newVersionStr = m_sysparam.GetProduct().GetNewVersionString();
        downloadURL = m_sysparam.GetProduct().GetDownloadURL();
        checksum = m_sysparam.GetProduct().GetDownloadChecksum();
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::SetRPMRegKey(std::wstring router, std::wstring tenant)
{
	if (router.size() == 0)
		router = L"https://skydrm.com";

	return m_drvcoreMgr.set_router_key(router, tenant, m_strWorkFolder, m_strTempFolder, m_strSDKLibFolder);
}


SDWLResult CSDRmcInstance::CheckKeyManager()
{
	if (m_keyManager.GetClientKey().empty())
		return RESULT2(SDWL_CERT_NOT_INSTALLED, "RMC client certificate is not installed");
	return RESULT(0);
}

SDWLResult CSDRmcInstance::SecureFileRead(const std::wstring& file, std::string& s)
{
	SDWLResult res;
	res = CheckKeyManager();
	if (!res)
		return res;

	NX::RmSecureFile sf(file, m_keyManager.GetClientKey());
	return sf.Read(s);
}

SDWLResult CSDRmcInstance::SecureFileWrite(const std::wstring& file, const std::string& s)
{
	SDWLResult res;
	res = CheckKeyManager();
	if (!res)
		return res;

	NX::RmSecureFile sf(file, m_keyManager.GetClientKey());
	return sf.Write((const UCHAR*)s.c_str(), (ULONG)s.length());
}

SDWLResult CSDRmcInstance::GetCurrentTenant(ISDRmTenant ** ptenant)
{
	*ptenant = NULL;
	if (m_CurTenant.GetRMSURL().length() == 0) {
		return RESULT(SDWL_NOT_FOUND);
	}

	*ptenant = &m_CurTenant;

	return RESULT(0);
}

SDWLResult CSDRmcInstance::AddTenant(CSDRmTenant info)
{
	if (info.GetRMSURL().length() == 0)
		return RESULT(SDWL_NOT_FOUND);

	SDWLResult res = QueryTenantInfo(info);

	if (res) {
		UpdateTenantInfo(info);
		return RESULT(0);
	}

	m_arrTenant.push_back(info);
	return RESULT(0);
}



SDWLResult CSDRmcInstance::LoadTenantInfo(const std::wstring& folder)
{
    //std::wstring file = folder +L"\\" + NXRMC_TENANT_HISTORY_FILENAME;
    std::string s;
    SDRSecureFile sFile(folder);
    sFile.SetKey(m_keyManager.GetClientKey());
    SDWLResult res = sFile.FileRead(NXRMC_TENANT_HISTORY_FILENAME, s);

    if (!res)
        return res;

    try {
        nlohmann::json root = nlohmann::json::parse(s);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON error, data from tenant file!");
        }

        nlohmann::json& tenants = root.at(FILE_TENTANTS_KEY_NAME);
        for (auto it = tenants.begin(); tenants.end() != it; it++)
        {
            CSDRmTenant tenant;
            tenant.ImportFromJson(*it);
            res = AddTenant(tenant);
            m_CurTenant = tenant;
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmcInstance::SaveTenantInfo()
{
    SDWLResult res = RESULT(0);

    try {
        nlohmann::json& root = nlohmann::json::object();
        root[FILE_VERSION_ELEMENT_NAME] = SDWIN_LIB_VERSION_NUMBER;

        root[FILE_TENTANTS_KEY_NAME] = nlohmann::json::array();
        nlohmann::json& tenants = root[FILE_TENTANTS_KEY_NAME];
        for each (CSDRmTenant tenantinfo in m_arrTenant)
        {
            nlohmann::json tenant = tenantinfo.ExportToJson();
            root[FILE_TENTANTS_KEY_NAME].push_back(tenant);
        }

        std::string s = root.dump();
        if (s.empty())
            return RESULT2(SDWL_INVALID_DATA, "fail to export tenant info to Json");

        res = m_File.FileWrite(NXRMC_TENANT_HISTORY_FILENAME, s);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        return RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmcInstance::LoadInstanceInfo()
{
    std::string s;
    //std::wstring file = m_strWorkFolder +L"\\" + NXRMC_INSTANCE_HISTORY_FILENAME;
    SDWLResult res = m_File.FileRead(NXRMC_INSTANCE_HISTORY_FILENAME, s);

    if (!res)
        return res;

    if (s.empty())
    {
        return res;
    }

    try {
        nlohmann::json root = nlohmann::json::parse(s);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON error, data from Instance.history file!");
        }

        std::string tenant = root.at(FILE_TENTANTID_ELEMENT_NAME).get<std::string>();
        std::string router = root.at(FILE_ROUTER_ELEMENT_NAME).get<std::string>();
        std::string rms = root.at(FILE_RMSURL_ELEMENT_NAME).get<std::string>();

        CSDRmTenant info(router, tenant);
        res = QueryTenantInfo(info);
        if (!res) {
            info.SetRMS(rms);
        }

        std::string clientid = root.at(FILE_CLIENTID_ELEMENT_NAME).get<std::string>();
        nlohmann::json& system = root.at(FILE_SYSTEM_KEY_NAME);
        m_sysparam.ImportFromJson(system);
        if (m_clientid.length() != 0) // restore clientid to the current input
            m_sysparam.SetClientID(m_clientid);

        m_CurTenant = info;
        nlohmann::json& user = root.at(FILE_USER_KEY_NAME);
        std::string useremail = user.at(FILE_USEREMAIL_ELEMENT_NAME).get<std::string>();
        unsigned int userid = user.at(FILE_USERID_ELEMENT_NAME).get<uint32_t>();
        if (useremail.length() != 0)
        {
            res = LoadUserInfo(NX::itos<wchar_t>(userid));
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmcInstance::SaveInstanceInfo()
{
    SDWLResult res = RESULT(0);

    try {
        nlohmann::json root = nlohmann::json::object();
        root[FILE_VERSION_ELEMENT_NAME] = SDWIN_LIB_VERSION_NUMBER;
        root[FILE_TENTANTID_ELEMENT_NAME] = NX::conv::to_string(m_CurTenant.GetTenant());
        root[FILE_ROUTER_ELEMENT_NAME] = NX::conv::to_string(m_CurTenant.GetRouterURL());
        root[FILE_RMSURL_ELEMENT_NAME] = NX::conv::to_string(m_CurTenant.GetRMSURL());
        root[FILE_CLIENTID_ELEMENT_NAME] = NX::conv::to_string(m_keyManager.GetClientId());

        root[FILE_SYSTEM_KEY_NAME] = m_sysparam.ExportToJson();

        root[FILE_USER_KEY_NAME] = nlohmann::json::object();
        nlohmann::json& user = root[FILE_USER_KEY_NAME];
        user[FILE_USEREMAIL_ELEMENT_NAME] = NX::conv::to_string(m_CurUser.GetEmail());
        user[FILE_USERID_ELEMENT_NAME] = m_CurUser.GetUserID();

        std::string s = root.dump();
        if (s.empty())
            return RESULT2(SDWL_INVALID_DATA, "fail to export tenant info to Json");

        res = m_File.FileWrite(NXRMC_INSTANCE_HISTORY_FILENAME, s);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        return RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmcInstance::LoadUserInfo(const std::wstring& userid)
{
    CELOG_ENTER;
    std::wstring work_dir = m_strWorkFolder + L"\\" + m_CurTenant.GetTenant();
    SDWLResult res;

    if (!PathIsDirectoryW(work_dir.c_str())) {
        if (!CreateDirectoryW(work_dir.c_str(), NULL)) {
            CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));

        }
    }
    work_dir += L"\\" + userid;
    if (!PathIsDirectoryW(work_dir.c_str())) {
        if (!CreateDirectoryW(work_dir.c_str(), NULL)) {
            CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));

        }
    }

	// For GE Power: not use protected dir, use work dir to read user info

    /*std::wstring config_dir;
    res = m_drvcoreMgr.get_protected_profiles_dir(config_dir);
    if (!res) {
        CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to get protected profiles directory!"));
    }

	if (NX::fs::is_dos_path(config_dir) == false)
    {
        if (!NT::CreateDirectory(config_dir.c_str(), NULL, FALSE, NULL)) {
		{
            CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
        }
	} else if (!PathIsDirectoryW(config_dir.c_str()))
	{
		if (!CreateDirectoryW(config_dir.c_str(), NULL))
		{
			CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
    }
	}

    config_dir += L"\\" + m_CurTenant.GetTenant();
	if (NX::fs::is_dos_path(config_dir) == false)
    {
        if (!NT::CreateDirectory(config_dir.c_str(), NULL, FALSE, NULL)) {
		{
            CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
        }
	} else if (!PathIsDirectoryW(config_dir.c_str()))
	{
		if (!CreateDirectoryW(config_dir.c_str(), NULL))
		{
			CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
    }
	}

    config_dir += L"\\" + userid;
	if (NX::fs::is_dos_path(config_dir) == false)
    {
        if (!NT::CreateDirectory(config_dir.c_str(), NULL, FALSE, NULL)) {
		{
            CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
        }
    }
	} else if (!PathIsDirectoryW(config_dir.c_str()))
	{
		if (!CreateDirectoryW(config_dir.c_str(), NULL))
		{
			CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
        }
	}*/
	
	std::string s;

    //SDRSecureFile sFile(config_dir);
	SDRSecureFile sFile(work_dir);

    sFile.SetKey(m_keyManager.GetClientKey());
    m_CurUser.SetRWFile(sFile, work_dir);

    //get login data 
    res = sFile.FileRead(NXRMC_USER_LOGIN_FILENAME, m_loginData);

    res = sFile.FileRead(NXRMC_USER_INFO_FILENAME, s);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    try {
        nlohmann::json root = nlohmann::json::parse(s);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON error, data from User.history file!"));
        }

        const nlohmann::json& user = root.at(FILE_USER_KEY_NAME);
        m_CurUser.ImportDataFromJson(user);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::SaveCurrentUser()
{
    SDWLResult res = RESULT(0);
    if (m_CurUser.GetName().length() == 0)
        return res;

    std::wstring dir = m_strWorkFolder + L"\\" + m_CurTenant.GetTenant();
    if (!PathIsDirectoryW(dir.c_str())) {
        if (!CreateDirectoryW(dir.c_str(), NULL)) {
            return RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!");
        }
    }

    dir += L"\\" + NX::itos<wchar_t>(m_CurUser.GetUserID());
    if (!PathIsDirectoryW(dir.c_str())) {
        if (!CreateDirectoryW(dir.c_str(), NULL)) {
            return RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!");
        }
    }

    try {
        nlohmann::json root = nlohmann::json::object();
        root[FILE_VERSION_ELEMENT_NAME] = SDWIN_LIB_VERSION_NUMBER;
        root[FILE_USER_KEY_NAME] = m_CurUser.ExportDataToJson();

        std::string s = root.dump();
        if (s.empty())
            return RESULT2(SDWL_INVALID_DATA, "fail to export tenant info to Json");

        //const std::wstring outfile = dir +L"\\" + NXRMC_USER_INFO_FILENAME;
        res = m_CurUser.FileWrite(NXRMC_USER_INFO_FILENAME, s);

        // save login data
        res = m_CurUser.FileWrite(NXRMC_USER_LOGIN_FILENAME, m_loginData);

        m_CurUser.Save();

    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmcInstance::Save()
{
	SDWLResult res = RESULT(0);

	res = SaveTenantInfo();
	res = SaveInstanceInfo();
	res = SaveCurrentUser();

	return res;
}

SDWLResult CSDRmcInstance::SetWorkingFolder(std::wstring workingfolder)
{
	SDRSecureFile sFile(workingfolder);
	sFile.SetKey(m_keyManager.GetClientKey());
	m_File = sFile;

	if (0 == workingfolder.compare(m_strWorkFolder))
		return RESULT(0);



	if (PathIsDirectoryW(workingfolder.data())) {
		m_strWorkFolder = workingfolder;
		//LoadTenantInfo(m_strWorkFolder);
		//LoadInstanceInfo();
	}
	else {
		return RESULT(SDWL_PATH_NOT_FOUND);
	}

	return RESULT(0);
}

SDWLResult CSDRmcInstance::UpdateTenantInfo(CSDRmTenant & info)
{
	for each (CSDRmTenant tinfo in m_arrTenant)
	{
		if (tinfo.GetRouterURL().compare(info.GetRouterURL()) == 0 && tinfo.GetTenant().compare(info.GetTenant()) == 0) {
			tinfo = info;//update the tenant info
			return RESULT(0);
		}
	}

	return RESULT(SDWL_NOT_FOUND);
}
SDWLResult CSDRmcInstance::QueryTenantInfo(CSDRmTenant & info)
{
	for each (CSDRmTenant tinfo in m_arrTenant)
	{
		if (tinfo.GetRouterURL().compare(info.GetRouterURL()) == 0 && 
			(tinfo.GetTenant().compare(info.GetTenant()) == 0 || info.GetTenant().compare(L"") == 0)) {
			info = tinfo;//retrieve the tenant info value
			return RESULT(0);
		}
	}

	return RESULT(SDWL_NOT_FOUND);
}

SDWLResult CSDRmcInstance::QueryTenantServerInfo(CSDRmTenant& info)
{
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();

    SDWLResult res;
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(info.GetRouterURL(), &res));
    if (0 != res.GetCode())
    {
        return res;
    }

    HTTPRequest query = info.GetTenantQuery();
    StringBodyRequest request(query);
    StringResponse response;

    res = spConn->SendRequest(request, response);
    if (!res) {
        return res;
    }

    const std::string& jsondata = response.GetBody();
    try {
        RetValue ret = info.ImportFromRMSResponse(jsondata);
        if (!ret)
            return RESULT2(ret.GetCode(), ret.GetMessage());
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmcInstance::GetLoginRequest(ISDRmHttpRequest **prequest)
{
	CELOG_ENTER;
	if (NULL == prequest) {
		CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "Invalid Parameter"));
	}
	SDWLResult res;
	res = m_LoginRequest.Initialize(m_CurTenant, m_sysparam);

	if (!res)
		CELOG_RETURN_VAL_T(res);
	*prequest = &m_LoginRequest;

	CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult CSDRmcInstance::SetLoginResult(const std::string &jsonreturn, ISDRmUser **puser, const std::string &security)
{
    CELOG_ENTER;
    SDWLResult res;

    if (NULL == puser) {
        CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "Invalid Parameter"));
    }

    res = m_drvcoreMgr.verify_security(security);
    if (!res) {
        CELOG_RETURN_VAL_T(res);
    }

    try {
        // Following codes is to fix bug #50872, however this won't work
        // application start flow:
        //		1. load instance
        //		2. load last user info
        //		3. do exact login
        //		
        //		actually, we need to think about step 2, whether to load last user info
        //		SetLoginResult is called after step 2, so now we have m_CurUser 
        //		initialized with last user info, and m_login is true.
        //		
        //if (m_CurUser.IsLogin())
        //	CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "User already login"));

        res = m_CurUser.SetLoginResult(jsonreturn);
        if (!res)
            CELOG_RETURN_VAL_T(res);

        // If tenant name is still not known yet, it must be cause user logged
        // in with default tenant.  So we set the actual tenant name here.
        if (m_CurTenant.GetTenant() != NX::conv::to_wstring(m_CurUser.GetTenant().GetTenant()))
        {
            m_CurTenant.SetTenant(m_CurUser.GetTenant().GetTenant());
            // And we need to replace the Tenant Array with the new tenant
            for (size_t i = 0; i < m_arrTenant.size(); i++)
            {
                CSDRmTenant tinfo = m_arrTenant.at(i);
                if (tinfo.GetRouterURL().compare(m_CurTenant.GetRouterURL()) == 0) {
                    m_arrTenant.at(i).SetTenant(m_CurUser.GetTenant().GetTenant());
                }
            }

        }

        std::string id = itos<char>(m_CurUser.GetUserID());
        std::wstring tenantpath = m_strWorkFolder + L"\\" + m_CurTenant.GetTenant();
        std::wstring work_path = m_strWorkFolder + L"\\" + m_CurTenant.GetTenant() + L"\\" + toUtf16(id);
        std::wstring config_path;
        DWORD err = 0;
        res = m_drvcoreMgr.get_protected_profiles_dir(config_path);
        if (!res) {
            CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to get protected profiles directory!"));
        }

		if (NX::fs::is_dos_path(config_path) == false)
		{
			if (!NT::CreateDirectory(config_path.c_str(), NULL, FALSE, NULL))
			{
				err = GetLastError();
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
			}
		}
		else if (!PathIsDirectoryW(config_path.c_str()))
		{
			if (!CreateDirectoryW(config_path.c_str(), NULL))
			{
				err = GetLastError();
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
			}
		}

        config_path += L"\\" + m_CurTenant.GetTenant();
		if (NX::fs::is_dos_path(config_path) == false)
		{
			if (!NT::CreateDirectory(config_path.c_str(), NULL, FALSE, NULL))
			{
				err = GetLastError();
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
			}
		}
		else if (!PathIsDirectoryW(config_path.c_str()))
		{
			if (!CreateDirectoryW(config_path.c_str(), NULL))
			{
				err = GetLastError();
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
			}
		}

		config_path += L"\\" + toUtf16(id);
		if (NX::fs::is_dos_path(config_path) == false)
		{
			if (!NT::CreateDirectory(config_path.c_str(), NULL, FALSE, NULL))
			{
				err = GetLastError();
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
			}
		}
		else if (!PathIsDirectoryW(config_path.c_str()))
		{
			if (!CreateDirectoryW(config_path.c_str(), NULL))
			{
				err = GetLastError();
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create user directory!"));
			}
		}

		// For GE Power: not use protected dir, use work dir to write user info
        //SDRSecureFile sFile(config_path);
		SDRSecureFile sFile(work_path);

        sFile.SetKey(m_keyManager.GetClientKey());
        m_CurUser.SetRWFile(sFile, work_path);
        if (!PathIsDirectoryW(tenantpath.c_str()))
        {
            if (!CreateDirectoryW(tenantpath.c_str(), NULL))
            {
                err = GetLastError();
                CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "Fail to create tenant directory!"));
            }
        }


        // Login ok, reset user's client id
        m_CurUser.SetSystemParameters(m_sysparam);

        // Load User local nxl files
        m_CurUser.GetSavedFiles();
        m_CurUser.m_SDRmcInstance = this;

        res = m_CurUser.InitializeLogin(m_CurTenant);
        if (!res)
            CELOG_RETURN_VAL_T(res);
    }
    catch (...) {
        // The JSON data is NOT correct
        CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "JSON response is not correct"));
    }
    m_loginData = jsonreturn;
    *puser = &m_CurUser;
    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::SaveLoginInfoForSSO(void)
{
	CELOG_ENTER;

	// after login success, we shall tell RPM service to save the user info.
	// then the other plugins can directly get current logined user without re-login
	// now, we do a workaround to save the info to registry
	std::wstring router;
	std::wstring tenant;

	router = m_CurTenant.GetRouterURL();
	tenant = m_CurTenant.GetTenant();
	std::wstring msg = L"SaveLoginInfoForSSO::set_router_key: router - [" + router + L"], tenant [" + tenant + L"]";
	OutputDebugString(msg.c_str());

	CELOG_RETURN_VAL_T(m_drvcoreMgr.set_router_key(router, tenant, m_strWorkFolder, m_strTempFolder, m_strSDKLibFolder));
}

SDWLResult CSDRmcInstance::SyncUserAttributes() {
	CELOG_ENTER;
	if (!m_CurUser.IsUserLogin()) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));
	}
	std::string jsonreturn;
	SDWLResult res = m_CurUser.SyncUserAttributes(jsonreturn);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}
	if (!IsRPMDriverExist()) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "RPM driver is not ready. RPM User attributes will not be synchronized."));
	}
	CELOG_RETURN_VAL_T(SetRPMUserAttr(jsonreturn));
}

SDWLResult CSDRmcInstance::RPMAddCachedToken(const RMToken & token)
{
	CELOG_ENTER;
	if (!m_CurUser.IsUserLogin()) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));
	}
	if (!IsRPMDriverExist()) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "RPM driver is not ready. RPM User attributes will not be synchronized."));
	}

	SDWLResult res;
	res = m_drvcoreMgr.add_cached_token(token.GetDUID(), token.GetOtp(), token.GetKey());
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMDeleteFileToken(const std::wstring &filename) 
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" filename= %s\n", filename.c_str());

	if (filename.size() == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "File name is empty"));

	std::wstring fpath = NX::fs::dos_fullfilepath(filename).path();
	CELOG_LOG(CELOG_INFO, L"RPMDeleteFileToken: %s\n", fpath.c_str());
	
	SDWLResult res = m_drvcoreMgr.delete_file_token(fpath);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMRemoveCachedToken(const std::string &duid)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" duid=%hs\n", duid.c_str());

	if (duid.size() == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "duid is empty"));

	SDWLResult res = m_drvcoreMgr.remove_cached_token(duid);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMRegisterApp(const std::wstring &appPath, const std::string &security)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" appPath=%s\n", appPath.c_str());
	::EnterCriticalSection(&_lock);
    std::wstring strPath = NX::fs::dos_fullfilepath(appPath).path();
    SDWLResult res = m_drvcoreMgr.register_app(strPath, security);
	::LeaveCriticalSection(&_lock);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMUnregisterApp(const std::wstring &appPath, const std::string &security)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" appPath=%s\n", appPath.c_str());
	::EnterCriticalSection(&_lock);
    std::wstring strPath = NX::fs::dos_fullfilepath(appPath).path();
    SDWLResult res = m_drvcoreMgr.unregister_app(strPath, security);
	::LeaveCriticalSection(&_lock);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMNotifyRMXStatus(bool running, const std::string &security)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" running=%s\n", running ? L"true" : L"false");
	::EnterCriticalSection(&_lock);
	SDWLResult res = m_drvcoreMgr.notify_rmx_status(running, security);
	::LeaveCriticalSection(&_lock);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMAddTrustedProcess(unsigned long processId, const std::string &security)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" processId=%lu\n", processId);
	::EnterCriticalSection(&_lock);
	SDWLResult res = m_drvcoreMgr.add_trusted_process(processId, security);
	::LeaveCriticalSection(&_lock);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMRemoveTrustedProcess(unsigned long processId, const std::string &security)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" processId=%lu\n", processId);
	::EnterCriticalSection(&_lock);
	SDWLResult res = m_drvcoreMgr.remove_trusted_process(processId, security);
	::LeaveCriticalSection(&_lock);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMAddTrustedApp(const std::wstring &appPath, const std::string &security)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" appPath=%s\n", appPath.c_str());
	std::wstring fpath = NX::fs::dos_fullfilepath(appPath).path();
	::EnterCriticalSection(&_lock);
	SDWLResult res = m_drvcoreMgr.add_trusted_app(fpath, security);
	::LeaveCriticalSection(&_lock);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMRemoveTrustedApp(const std::wstring &appPath, const std::string &security)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L" appPath=%s\n", appPath.c_str());
	std::wstring fpath = NX::fs::dos_fullfilepath(appPath).path();
	::EnterCriticalSection(&_lock);
	SDWLResult res = m_drvcoreMgr.remove_trusted_app(fpath, security);
	::LeaveCriticalSection(&_lock);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMQueryAPIUser(std::wstring &apiuser_logindata)
{
	CELOG_ENTER;
	::EnterCriticalSection(&_lock);
	SDWLResult res = m_drvcoreMgr.request_apiuser_logindata(apiuser_logindata);
	::LeaveCriticalSection(&_lock);
	CELOG_LOG(CELOG_INFO, L" apiuser login info=%s\n", apiuser_logindata.c_str());
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmcInstance::RPMGetSecretDir(std::wstring &path)
{
	SDWLResult res = m_drvcoreMgr.get_secret_dir(m_rpmSecretDir);
	if (res.GetCode() == 0)
		path = m_rpmSecretDir;
	return res;
}

SDWLResult CSDRmcInstance::RPMStartPDP(const std::string &security)
{
    CELOG_ENTER;

    if (!IsSecurityStringCorrect(security))
    {
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid security string"));
    }

    CELOG_RETURN_VAL_T(m_drvcoreMgr.launch_pdp_process());
}

SDWLResult CSDRmcInstance::RPMIsAppRegistered(const std::wstring &appPath, bool& registered)
{
	std::wstring fpath = NX::fs::dos_fullfilepath(appPath).path();
	return m_drvcoreMgr.is_app_registered(fpath, registered);
}

SDWLResult CSDRmcInstance::RPMGetLoginUser(const std::string &passcode, ISDRmUser **puser)
{
	CELOG_ENTER;

	if (!IsSecurityStringCorrect(passcode))
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "Invalid passcode to auto login"));

	if (!m_CurUser.IsUserLogin())
	{
		// try to load from API User
		std::wstring apiuser_logindata;
		SDWLResult res = RPMQueryAPIUser(apiuser_logindata);
		if (!res)
			CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

		res = m_CurUser.SetLoginResult(NX::conv::utf16toutf8(apiuser_logindata));
		if (!res)
			CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

		if (!m_CurUser.IsUserLogin())
			CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

		std::string ret;
		m_CurUser.SyncUserAttributes(ret);
	}

	// Load user cached nxl file
	m_CurUser.GetSavedFiles();
	m_CurUser.m_SDRmcInstance = this;

	*puser = &m_CurUser;

	CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
}

SDWLResult CSDRmcInstance::GetLoginData(const std::string &useremail, const std::string &passcode, std::string &data)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"useremail=%hs\n", useremail.c_str());

	std::string curPasscode = m_CurUser.GetPasscode();
	if (passcode.compare(curPasscode) != 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Passcode incorrect."));
	std::string email = NX::conv::to_string(m_CurUser.GetEmail());

	if (toLower(useremail).compare(toLower(email)) != 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Email incorrect."));

    data = m_loginData;

	CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
}

SDWLResult CSDRmcInstance::GetLoginUser(const std::string &useremail, const std::string &passcode, ISDRmUser **puser)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"useremail=%hs\n", useremail.c_str());

	std::string curPasscode = m_CurUser.GetPasscode();
	if (passcode.compare(curPasscode) != 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Passcode incorrect."));
	std::string email = NX::conv::to_string(m_CurUser.GetEmail());
	
	if (toLower(useremail).compare(toLower(email)) != 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Email incorrect."));

	if (!m_CurUser.IsUserLogin())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_LOGIN_REQUIRED, "User not logged in."));

	// Load user cached nxl file
	m_CurUser.GetSavedFiles();
	m_CurUser.m_SDRmcInstance = this;

	*puser = &m_CurUser;

	CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
}

SDWLResult CSDRmcInstance::Logout()
{
	SDWLResult res;
	res = m_CurUser.LogoutUser();
	if (res.GetCode() == 0)
		SaveCurrentUser();

	return res;
}

SDWLResult CSDRmcInstance::RPMEditCopyFile(const std::wstring &filepath, std::wstring& destpath)
{
	// get RPM folder
	std::wstring rpmfolder = L"";
	SDWLResult res = RPMGetSecretDir(rpmfolder);
	if (!res && destpath.size() == 0)
		return res;

	return m_CurUser.RPMEditCopyFile(filepath, destpath, rpmfolder);
}

SDWLResult CSDRmcInstance::RPMEditSaveFile(const std::wstring& filepath, const std::wstring& originalNXLfilePath, bool deletesource, uint32_t exitedit, const std::wstring& tags)
{
	return m_CurUser.RPMEditSaveFile(filepath, originalNXLfilePath, deletesource, exitedit,tags);
}

SDWLResult CSDRmcInstance::RPMGetRights(const std::wstring& filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks)
{
	std::wstring fpath = NX::fs::dos_fullfilepath(filepath).path();
	return m_CurUser.RPMGetRights(fpath, rightsAndWatermarks);
}

SDWLResult CSDRmcInstance::RPMReadFileTags(const std::wstring& filepath, std::wstring &tags)
{
	if (m_CurUser.IsLogin() == false)
		return RESULT2(SDWL_LOGIN_REQUIRED, "user is not logined");
	std::wstring fpath = NX::fs::dos_fullfilepath(filepath).path();
	return m_drvcoreMgr.read_file_tags(filepath, tags);
}


SDWLResult CSDRmcInstance::IsRPMFolder(const std::wstring &filepath, unsigned int* dirstatus, SDRmRPMFolderOption* option, std::wstring& filetags)
{
	std::wstring _filepath = NX::fs::dos_fullfilepath(filepath).path();

	if (!PathIsDirectory(NX::fs::dos_fullfilepath(filepath).global_dos_path().c_str()))
		return RESULT2(SDWL_PATH_NOT_FOUND, "filepath is not directory");

	auto found = _filepath.find(L":");
	if (found == std::string::npos)
		return RESULT2(SDWL_INVALID_DATA, "path is not valid");

	SDWLResult res = m_drvcoreMgr.is_rpm_folder(_filepath, dirstatus, option, filetags);

	return res;
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
SDWLResult CSDRmcInstance::IsSanctuaryFolder(const std::wstring &path, unsigned int* dirstatus, std::wstring& filetags)
{
	std::wstring _filepath = NX::fs::dos_fullfilepath(path).path();

	if (!PathIsDirectory(NX::fs::dos_fullfilepath(path).global_dos_path().c_str()))
		return RESULT2(SDWL_PATH_NOT_FOUND, "path is not directory");

	auto found = _filepath.find(L":");
	if (found == std::string::npos)
		return RESULT2(SDWL_INVALID_DATA, "path is not valid");

	SDWLResult res = m_drvcoreMgr.is_sanctuary_folder(_filepath, dirstatus, filetags);

	return res;
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

SDWLResult CSDRmcInstance::RPMSetAppRegistry(const std::wstring& subkey, const std::wstring& name, const std::wstring& data, uint32_t op, const std::string &security)
{
	SDWLResult res = m_drvcoreMgr.set_app_key(subkey, name, data, op, security);
	return res;
}

// rename a normal file
// example: abc.dox ==> abc-(1).docx, abc ==> abc-(1)
bool rename_normal_file(const std::wstring fname)
{
	std::experimental::filesystem::path _file(fname);
	std::wstring _prename = _file;
	std::wstring _ext = _file.extension();
	if (_ext.size() > 0)
	{
		// get file path string without extension
		_prename = _prename.substr(0, _prename.size() - _ext.size());
	}

	int i = 0;
	bool bfinished = false;
	// if prename is "abc-(3)", we need to follow "3", and change prename to "abc"
	if (_prename.rfind(L')') == (_prename.size() - 1))
	{
		size_t nlpos = _prename.rfind(L"-(");
		size_t nrpos = _prename.size() - 1;
		if (nlpos != std::wstring::npos && ((nlpos + 2) <= nrpos))
		{
			std::wstring ws_num = _prename.substr(nlpos + 2, nrpos - nlpos - 2);
			int _i = -1;
			bool bmatch = false;
			if (ws_num == L"0")
			{ 
				bmatch = true;
			}
			else
			{
				_i = _wtoi(ws_num.c_str());
				if (_i == 0)
				{
					// not integer
				}
				else
				{
					// integer, match the pattern "abc-(3)"
					i = _i;
					bmatch = true;
				}
			}

			// match the pattern "abc-(3)"
			_prename = _prename.substr(0, nlpos);
		}
	}
	while (bfinished == false)
	{
		i++;
		std::wstring _newname = _prename + L"-(" + std::to_wstring(i)  + L")" + _ext;
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(_newname.c_str()))
		{
			std::wstring _newname_nxl = _newname + L".nxl";
			if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(_newname_nxl.c_str()))
			{
				// the new file does not exists and new file.nxl also not exts, OK
				if (MoveFile(_file.c_str(), _newname.c_str()))
				{
					bfinished = true;
				}
				else
				{
					// MoveFile failed
					// maybe the source file is locked
					// then we have to rename the NXL file
					return false;
				}
			}
		}
	}

	return true;
}

// rename a normal file
// example: abc.dox.nxl ==> abc-(1).docx.nxl
bool rename_nxl_file(const std::wstring fname)
{
	std::experimental::filesystem::path _file(fname);
	std::wstring _prename = _file;
	std::wstring _ext = _file.extension();
	std::transform(_ext.begin(), _ext.end(), _ext.begin(), ::tolower);
	if (wcscmp(_ext.c_str(), L".nxl") != 0)
		return false; // not a NXL file
	// get file path string without ".nxl"
	_prename = _prename.substr(0, _prename.size() - _ext.size());

	std::experimental::filesystem::path _file_nonnxl(_prename);
	_prename = _file_nonnxl;
	_ext = _file_nonnxl.extension();
	if (_ext.size() > 0)
	{
		// get file path string without extension
		_prename = _prename.substr(0, _prename.size() - _ext.size());
	}
	int i = 0;
	bool bfinished = false;
	// if prename is "abc-(3)", we need to follow "3", and change prename to "abc"
	if (_prename.rfind(L')') == (_prename.size() - 1))
	{
		size_t nlpos = _prename.rfind(L"-(");
		size_t nrpos = _prename.size() - 1;
		if (nlpos != std::wstring::npos && ((nlpos + 2) <= nrpos))
		{
			std::wstring ws_num = _prename.substr(nlpos + 2, nrpos - nlpos - 2);
			int _i = -1;
			bool bmatch = false;
			if (ws_num == L"0")
			{
				bmatch = true;
			}
			else
			{
				_i = _wtoi(ws_num.c_str());
				if (_i == 0)
				{
					// not integer
				}
				else
				{
					// integer, match the pattern "abc-(3)"
					i = _i;
					bmatch = true;
				}
			}

			// match the pattern "abc-(3)"
			_prename = _prename.substr(0, nlpos);
		}
	}

	while (bfinished == false)
	{
		i++;
		std::wstring _newname_nonnxl = _prename + L"-(" + std::to_wstring(i) + L")" + _ext;
		std::wstring _newname = _prename + L"-(" + std::to_wstring(i) + L")" + _ext + L".nxl";
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(_newname.c_str()) && 
			INVALID_FILE_ATTRIBUTES == GetFileAttributesW(_newname_nonnxl.c_str()))
		{
			// the new file (both .nxl and normal) does not exists, OK
			if (MoveFile(_file.c_str(), _newname.c_str()))
			{
				bfinished = true;
			}
			else
			{
				// MoveFile failed
				// maybe the source NXL file is locked
				return false;
			}
		}
	}

	return true;
}

void CSDRmcInstance::reset_nxl_files(std::vector<std::wstring> &v)
{
	//
	// Rules
	//	pre-condition: same folder has following files
	//	1. abc.docx.nxl, abc.docx, abc - (1).docx
	//	==> abc.docx.nxl, abc - (2).docx, abc - (1).docx
	//	2. abc.nxl.docx.nxl       
	//	==> abc.docx.nxl 

	//Soure file name              After marked
	//(1) 1234.nxl txt.txt.nxl  -> 1234 txt.txt.nxl
	//(2) 1234.nxl txt.nxl       -> 1234 txt.nxl
	//(3) 1234.nxltxt.txt.nxl   -> 1234txt.txt.nxl
	//(4) 1234.nxltxt.nxl        -> 1234.nxltxt.nxl(only this case keep the.nxl)
	//(5) 1234.nxl.txt.nxl       -> 1234.txt.nxl
	//(6) 1234.nxl.nxltxt.nxl	 -> 1234.nxltxt.nxl
	//(7) 1234.nxlabc.nxltxt.nxl	 -> 1234abc.nxltxt.nxl
	for (size_t i = 0; i < v.size(); i++)
	{
		std::wstring nxl_filepath = NX::fs::dos_fullfilepath(v[i]).global_dos_path();
		std::experimental::filesystem::path _file_nxl(nxl_filepath);
		std::wstring _ext = _file_nxl.extension();
		std::transform(_ext.begin(), _ext.end(), _ext.begin(), ::tolower);
		if (wcscmp(_ext.c_str(), L".nxl") == 0)
		{
			// the file should be NXL
			// get file path string without ".nxl"
			std::wstring file_withoutnxl = nxl_filepath.substr(0, nxl_filepath.size() - _ext.size());
			std::experimental::filesystem::path _file_withoutnxl(file_withoutnxl);
			std::wstring _ext2 = _file_withoutnxl.extension();
			std::wstring lower_ext2 = _ext2;
			std::transform(_ext2.begin(), _ext2.end(), lower_ext2.begin(), ::tolower);
			bool b_name_changed = false;
			if ((wcscmp(lower_ext2.c_str(), L".nxl") != 0) && lower_ext2.find(L".nxl") != std::wstring::npos && lower_ext2.find(L" ") == std::wstring::npos)
			{
				// match case #(4) 1234.nxltxt.nxl
				std::wstring file_withoutnxl2 = file_withoutnxl.substr(0, file_withoutnxl.size() - _ext2.size());

				std::wstring lower_file_withoutnxl2 = file_withoutnxl2;
				std::transform(file_withoutnxl2.begin(), file_withoutnxl2.end(), lower_file_withoutnxl2.begin(), ::tolower);
				if (lower_file_withoutnxl2.find(L".nxl") != std::wstring::npos)
				{
					// match case #(6), (7)
					b_name_changed = true;

					// we remove ".nxl" from the string
					std::string source = NX::conv::utf16toutf8(file_withoutnxl2);
					std::string searchString = ".nxl";
					std::string replaceString = "";
					std::string result = NX::conv::string_ireplace(source, searchString, replaceString);
					file_withoutnxl = NX::conv::utf8toutf16(result) + _ext2;
				}
			}
			else
			{
				// for case (1), (2), (3), (5)
				// we remove ".nxl" from the string
				std::wstring lower_file_withoutnxl = file_withoutnxl;
				std::transform(file_withoutnxl.begin(), file_withoutnxl.end(), lower_file_withoutnxl.begin(), ::tolower);
				if (lower_file_withoutnxl.find(L".nxl") != std::wstring::npos)
				{
					b_name_changed = true;

					// remove "nxl" in string.

					std::string source = NX::conv::utf16toutf8(file_withoutnxl);
					std::string searchString = ".nxl";
					std::string replaceString = "";
					std::string result = NX::conv::string_ireplace(source, searchString, replaceString);
					file_withoutnxl = NX::conv::utf8toutf16(result);
				}
			}

			// if the file without the last .nxl exists, we need to rename the non-nxl file
			// now we need to rename the file
			if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(file_withoutnxl.c_str()))
			{
				// there is normal file exists
				if (rename_normal_file(file_withoutnxl) == false)
				{
					// can't rename the normal file
					// we have to rename the original NXL file
				}
			}

			{
				// rename the NXL file if changed
				if (b_name_changed)
				{
					std::wstring _new_nxl = file_withoutnxl + L".nxl";
					if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(_new_nxl.c_str()))
					{
						// rename the NXL file
						rename_nxl_file(_new_nxl);
					}

					MoveFile(nxl_filepath.c_str(), _new_nxl.c_str());
				}
			}
		}
	}
}
void CSDRmcInstance::load_nxl_files(const std::wstring& folder, std::vector<std::wstring> &v)
{
	NX::fs::dos_fullfilepath _fullpath(folder);
	std::wstring pattern = _fullpath.global_dos_path() + L"\\*";
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
				continue;

			std::wstring fnode_name = _fullpath.path() + L"\\" + data.cFileName;
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// a subdirectory
				load_nxl_files(fnode_name, v);
			}
			else
			{
				// a normal file
				std::experimental::filesystem::path fnxl_name(fnode_name);
				std::wstring ext = fnxl_name.extension();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
				if (wcscmp(ext.c_str(), L".nxl") == 0)
					v.push_back(fnode_name);
			}
		} while (FindNextFile(hFind, &data) != 0);

		FindClose(hFind);
	}
}

void CSDRmcInstance::RPMResetNXLinFolder(const std::wstring &filepath)
{
	std::vector<std::wstring> nxlfiles;
	load_nxl_files(filepath, nxlfiles);
	reset_nxl_files(nxlfiles);
}



/*
OB_OVERLAY("Text", "$(User) \\n$(Date)\\n$(Host)\\n$(Time)\\n$(Classifications)",
"Transparency", "40",
"FontName", "Sitka Text",
"FontSize", "30",
"TextColor", "#888888",
"Rotation", "Anticlockwise"/"Clockwise"
*/

SDWLResult CSDRmcInstance::RPMSetViewOverlay(void * target_window, const std::wstring &nxlfilepath, const std::tuple<int, int, int, int>& display_offset)
{
	if (!m_CurUser.IsLogin())
		return RESULT2(SDWL_LOGIN_REQUIRED, "User is not login.");

	//std::wstring filepath = inputpath;
	CSDRmNXLFile *file = NULL;
	SDWLResult res = m_CurUser.OpenFile(nxlfilepath, (ISDRmNXLFile**)&file);
	if (!res)
		return res;

	// file tags
	const std::string tags = file->GetTags();
	const std::vector<std::pair<std::wstring, std::wstring>> attrs = NX::conv::ImportJsonTags(tags);
	m_CurUser.CloseFile(file);

	// file watermark
	SDR_WATERMARK_INFO watermark;
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rights;
	res = m_CurUser.GetRights(nxlfilepath, rights, true);
	if (rights.size() <= 0)
		return RESULT2(SDWL_ACCESS_DENIED, "You are not authorized.");

	for (const auto& right : rights)
	{
		if (right.first == SDRmFileRight::RIGHT_VIEW)
		{
			if (right.second.size() <= 0)
				return RESULT2(SDWL_INVALID_DATA, "No WaterMark (view) defined.");

			watermark = right.second[0];
			break;
		}
	}

	std::string wmtext = watermark.text;
	if (wmtext.size() <= 0)
		return RESULT2(SDWL_INVALID_DATA, "No WaterMark Text (view) defined.");

	// convert the watermark text with pre-defined macro
	int begin = -1;
	int end = -1;
	std::string converted_wmtext;
	for (size_t i = 0; i < wmtext.size(); i++)
	{
		if (wmtext.data()[i] == '$')
		{
			if (begin >= 0)
			{
				std::string s2 = wmtext.substr(begin, (i - begin));
				converted_wmtext.append(s2);
			}
			begin = (int)i;
		}
		else if (wmtext.data()[i] == ')')
		{
			end = (int)i;
			if (begin >= 0)
			{
				std::string s1 = wmtext.substr(begin, (end - begin + 1));
				std::string s2 = wmtext.substr(begin, (end - begin + 1));
				std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
				bool has_attr = false;
				for (size_t i = 0; i < attrs.size(); i++) {
					std::string a_s1 = "$(" + toUtf8(attrs[i].first) + ")";
					std::transform(a_s1.begin(), a_s1.end(), a_s1.begin(), ::tolower);
					if (a_s1 == s2)
					{
						std::string v_s1 = toUtf8(attrs[i].second);
						converted_wmtext.append(v_s1);
						converted_wmtext.append(" ");
						has_attr = true;
					}
				}

				if (has_attr == false)
				{
					// non-supported classification, append back
					converted_wmtext.append(s1);
				}
			}
			else
			{
				// wrong format
				converted_wmtext.push_back(wmtext.data()[i]);
			}
			begin = -1;
			end = -1;
		}
		else
		{
			if (begin >= 0)
			{
				// in processing $xxxx
			}
			else
				converted_wmtext.push_back(wmtext.data()[i]);
		}
	}

	if (begin >= 0 && end == -1)
	{
		// never match $xxxx
		std::string s2 = wmtext.substr(begin);
		converted_wmtext.append(s2);
	}

	watermark.text = converted_wmtext;
	return RPMSetViewOverlay(target_window, watermark, display_offset);
}

/*
OB_OVERLAY("Text", "$(User) \\n$(Date)\\n$(Host)\\n$(Time)", 
"Transparency", "40", 
"FontName", "Sitka Text", 
"FontSize", "30", 
"TextColor", "#888888", 
"Rotation", "Anticlockwise"/"Clockwise"
*/
SDWLResult CSDRmcInstance::RPMSetViewOverlay(void * target_window, const SDR_WATERMARK_INFO &watermark, const std::tuple<int, int, int, int>& display_offset)
{

	// date and time
	std::string strdate;
	std::string strtime;
	time_t rawtime;
	struct tm timeinfo;
	char date_buffer[256] = { 0 };
	char time_buffer[256] = { 0 };
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", &timeinfo);
	strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &timeinfo);
	strdate = std::string(date_buffer);
	strtime = std::string(time_buffer);

	// host
	const std::string& host = toUtf8(NX::win::GetLocalHostInfo().GetFQDN().empty() ? NX::win::GetLocalHostInfo().GetHostName() : NX::win::GetLocalHostInfo().GetFQDN());

	// user id
	std::string wmtext = watermark.text;
	std::string userid;
	if (m_CurUser.IsLogin())
		userid = toUtf8(m_CurUser.GetEmail());
	else
	{
		// Get Windows SID
		WCHAR wzName[MAX_PATH + 1] = { 0 };
		WCHAR wzSid[MAX_PATH + 1] = { 0 };
		NX::conv::GetUserInfo(wzSid, MAX_PATH, wzName, MAX_PATH);
		userid = toUtf8(std::wstring(wzName)) + " " + toUtf8(std::wstring(wzSid));
	}
	
	// convert the watermark text with pre-defined macro
	int begin = -1;
	int end = -1;
	std::string converted_wmtext;
	for (size_t i = 0; i < wmtext.size(); i++)
	{
		if (wmtext.data()[i] == '$')
		{
			if (begin >= 0)
			{
				std::string s2 = wmtext.substr(begin, (i - begin));
				converted_wmtext.append(s2);
			}
			begin = (int)i;
		}
		else if (wmtext.data()[i] == ')')
		{
			end = (int)i;
			if (begin >= 0)
			{
				std::string s1 = wmtext.substr(begin, (end - begin + 1));
				std::string s2 = wmtext.substr(begin, (end - begin + 1));
				std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
				if (s2 == "$(user)")
				{
					converted_wmtext.append(userid);
					converted_wmtext.append(" ");
				}
				else if (s2 == "$(break)")
				{
					converted_wmtext.append("\n");
				}
				else if (s2 == "$(date)")
				{
					converted_wmtext.append(strdate);
					converted_wmtext.append(" ");
				}
				else if (s2 == "$(time)")
				{
					converted_wmtext.append(strtime);
					converted_wmtext.append(" ");
				}
				else if (s2 == "$(host)")
				{
					converted_wmtext.append(host);
					converted_wmtext.append(" ");
				}
				else
				{
					// non-supported macro, append back
					converted_wmtext.append(s1);
				}
			}
			else
			{
				// wrong format
				converted_wmtext.push_back(wmtext.data()[i]);
			}
			begin = -1;
			end = -1;
		}
		else
		{
			if (begin >= 0)
			{
				// in processing $xxxx
			}
			else
				converted_wmtext.push_back(wmtext.data()[i]);
		}
	}

	if (begin >= 0 && end == -1)
	{
		// never match $xxxx
		std::string s2 = wmtext.substr(begin);
		converted_wmtext.append(s2);
	}

	std::wstring overlay_text = toUtf16(converted_wmtext);

	// check other params of watermark
	BOOL bdefault = false;
	if (watermark.transparency == 0 && watermark.fontColor.size() == 0 && watermark.fontName.size() == 0 && watermark.fontSize == 0 &&
		watermark.rotation == 0 && watermark.rotation == 0)
		bdefault = true;

	unsigned char transparency = 70;
	if (!bdefault)
		transparency = watermark.transparency;

	std::tuple<unsigned char, unsigned char, unsigned char, unsigned char> font_color = { transparency, 0, 128, 21 };
	if (!bdefault)
	{
		std::string wzcolor = watermark.fontColor;
		if (wzcolor.size() == 7 && wzcolor.data()[0] == '#')
		{
			// color is #888888
			const char *pc = wzcolor.data();
			std::string sz_r = wzcolor.substr(1, 2);
			std::string sz_g = wzcolor.substr(3, 2);
			std::string sz_b = wzcolor.substr(5, 2);
			long int _r = strtol(sz_r.data(), NULL, 16);
			long int _g = strtol(sz_g.data(), NULL, 16);
			long int _b = strtol(sz_b.data(), NULL, 16);

			font_color = { transparency, (unsigned char)_r, (unsigned char)_g, (unsigned char)_b };
		}
		else if (wzcolor.size() == 6)
		{
			// color is #888888
			const char *pc = wzcolor.data();
			std::string sz_r = wzcolor.substr(0, 2);
			std::string sz_g = wzcolor.substr(2, 2);
			std::string sz_b = wzcolor.substr(4, 2);
			long int _r = strtol(sz_r.data(), NULL, 16);
			long int _g = strtol(sz_g.data(), NULL, 16);
			long int _b = strtol(sz_b.data(), NULL, 16);

			font_color = { transparency, (unsigned char)_r, (unsigned char)_g, (unsigned char)_b };
		}
	}

	std::wstring font_name = L"Arial";
	if (!bdefault)
	{
		font_name = toUtf16(watermark.fontName);
	}

	int font_size = 22;
	if (!bdefault)
		font_size = watermark.fontSize;

	int font_rotation = 45;
	if (!bdefault)
	{
		if (watermark.rotation == WATERMARK_ROTATION::CLOCKWISE)
			font_rotation = 45;
		if (watermark.rotation == WATERMARK_ROTATION::ANTICLOCKWISE)
			font_rotation = -45;
		else
			font_rotation = 0;
	}

	int font_sytle = 0;
	int text_alignment = 0;

	return RPMSetViewOverlay(target_window, overlay_text, font_color, font_name, font_size, font_rotation, font_sytle, text_alignment, display_offset);
}

SDWLResult CSDRmcInstance::RPMSetViewOverlay(void * target_window,
	const std::wstring & overlay_text,
	const std::tuple<unsigned char, unsigned char, unsigned char, unsigned char>& font_color,
	const std::wstring & font_name , 
	int font_size , int font_rotation , int font_sytle, int text_alignment,
	const std::tuple<int, int, int, int>& display_offset)
{
	if (overlay_text.empty()) {
		return RESULT2(ERROR_INVALID_PARAMETER, "Invalid overlay_text value");
	}
	// force get GDI+ init here for first use, in case of user env not do this job
	auto& ins = ViewOverlyController::getInstance();

	// prepare params
	OverlayConfig::FontStyle fs;
	switch (font_sytle)
	{
	case 0:
		fs = OverlayConfig::FontStyle::FS_Regular;
		break;
	case 1:
		fs = OverlayConfig::FontStyle::FS_Bold;
		break;
	case 2:
		fs = OverlayConfig::FontStyle::FS_Italic;
		break;
	case 3:
		fs = OverlayConfig::FontStyle::FS_BoldItalic;
		break;
	default:
		fs = OverlayConfig::FontStyle::FS_Regular;
		break;
	}
	OverlayConfig::TextAlignment	alignment;
	switch (text_alignment)
	{
	case 0:
		alignment = OverlayConfig::TextAlignment::TA_Left;
		break;
	case 1:
		alignment = OverlayConfig::TextAlignment::TA_Centre;
		break;
	case 2:
		alignment = OverlayConfig::TextAlignment::TA_Right;
		break;
	default:
		alignment = OverlayConfig::TextAlignment::TA_Centre;
		break;
	}
	
	OverlayConfigBuilder builder;
	builder
		.SetString(overlay_text)
		.SetFontColor(std::get<0>(font_color), std::get<1>(font_color), std::get<2>(font_color), std::get<3>(font_color))
		.SetFontName(font_name)
		.SetFontSize(font_size)
		.SetFontRotation(font_rotation)
		.SetFontStyle(fs)
		.SetLineAlignment(alignment)
		.SetTextAlignment(alignment)
		.SetDisplayOffset({std::get<0>(display_offset), std::get<1>(display_offset), std::get<2>(display_offset), std::get<3>(display_offset)});
	;
	
	ins.Attach((HWND)target_window, builder.Build());

	return RESULT(ERROR_SUCCESS);
}

SDWLResult CSDRmcInstance::RPMClearViewOverlay(void * target_window)
{
	ViewOverlyController::getInstance().Detach((HWND)target_window);
	return RESULT(ERROR_SUCCESS);
}

SDWLResult CSDRmcInstance::RPMClearViewOverlayAll() {
	ViewOverlyController::getInstance().Clear();
	return RESULT(ERROR_SUCCESS);
}

SDWLResult CSDRmcInstance::RPMPopupNewToken(const std::wstring &membershipid, RMToken &token)
{
	std::string token_id;
	std::string token_value;
	std::string token_otp;

	SDWLResult res = m_drvcoreMgr.popup_new_token(membershipid, token_id, token_otp, token_value);
	if (res)
	{
		RMToken _token(token_id, token_value, 0);
		_token.SetOtp(token_otp);

		token = _token;
	}
	return res;
}

SDWLResult CSDRmcInstance::RPMFindCachedToken(const std::wstring &duid, RMToken &token, time_t &ttl)
{
	std::string token_id;
	std::string token_value;
	std::string token_otp;
	time_t token_ttl = 0;

	SDWLResult res = m_drvcoreMgr.find_cached_token(duid, token_id, token_otp, token_value, ttl);
	if (res)
	{
		RMToken _token(token_id, token_value, 0);
		_token.SetOtp(token_otp);

		token = _token;
	}
	return res;
}

SDWLResult CSDRmcInstance::RPMRequestLogin(const std::wstring &callback_cmd, const std::wstring &callback_cmd_param)
{
	SDWLResult res = m_drvcoreMgr.request_login(callback_cmd, callback_cmd_param);
	return res;
}


SDWLResult CSDRmcInstance::RPMRequestLogout(bool* isAllow, uint32_t option)
{
	SDWLResult res = RESULT(0);

	// 1. Try to communicate to service to check if now exist some nxl file is opened.
	res = m_drvcoreMgr.request_logout(1);
	if (!res)
	{
		*isAllow = false;
	}
	else 
	{
		// 2.Broadcast to check if recipients is executing some special operation, e.g. protecting/uploading in rmd etc.
		DWORD dwRecipients = BSM_APPLICATIONS;
		long ret = ::BroadcastSystemMessage(BSF_QUERY | BSF_FORCEIFHUNG | BSF_NOTIMEOUTIFNOTHUNG | BSF_ALLOWSFW,  
			&dwRecipients,
			50005, // check if allow logout
			0,
			0);

		if (ret == -1)
		{
			*isAllow = false;
			return RESULT2(SDWL_INTERNAL_ERROR, "Unable to broadcast the message.");
		}
		else if (ret == 0)
		{
			*isAllow = false;
			return RESULT2(170, "Request logout is denied, since some operation is executing.");
		}
		else
		{
			*isAllow = true;
			if (option == 0)
			{
				// Broadcast to all recipients(rmd,viewer,service manager e.g.) begin to logout.
				::BroadcastSystemMessage(BSF_QUERY | BSF_FORCEIFHUNG | BSF_NOTIMEOUTIFNOTHUNG | BSF_ALLOWSFW,
					&dwRecipients,
					50006, // execute logout
					0,
					0);
			}
		}

	}
	
	return res;
}

SDWLResult CSDRmcInstance::RPMNotifyMessage(const std::wstring &app, const std::wstring &target, const std::wstring &message, 
	uint32_t msgtype, const std::wstring &operation, uint32_t result, uint32_t fileStatus)
{
	SDWLResult res = m_drvcoreMgr.notify_message(app, target, message, msgtype, operation, result, fileStatus);
	return res;
}

