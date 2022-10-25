
#include "stdafx.h"
#include "registry_whitelist.h"
#include <nudf\crypto.hpp>
#include <nudf\conversion.hpp>
#include <psapi.h>
#include <Shlwapi.h>
#include <regex>
#include <codecvt>
#include <locale>

#pragma comment(lib, "Shlwapi.lib")

//HKEY_LOCAL_MACHINE\SOFTWARE\NextLabs\SkyDRM\OSRmx\whitelists
const wchar_t CONST_SZ_WHITELIST_KEY[] = L"SOFTWARE\\NextLabs\\SkyDRM\\OSRmx\\whitelists";

//ACTIONS
//VIEW | PRINT | SAVE | SAVEAS | COPYCONTENT | PRINTSCREEN
const wchar_t CONST_NX_APP_ACTION_VIEW[] = L"view";
const wchar_t CONST_NX_APP_ACTION_PRINT[] = L"print";
const wchar_t CONST_NX_APP_ACTION_SAVE[] = L"save";
const wchar_t CONST_NX_APP_ACTION_SAVEAS[] = L"saveas";
const wchar_t CONST_NX_APP_ACTION_COPYCONTENT[] = L"copycontent";
const wchar_t CONST_NX_APP_ACTION_PRINTSCREEN[] = L"printscreen";

namespace nx {
	cregistry_whitelist::cregistry_whitelist()
	{
		m_bAppInWhiteLists = false;
		load_whitelist_from_registry();
	}

	cregistry_whitelist::~cregistry_whitelist()
	{

	}

	cregistry_whitelist* cregistry_whitelist::getInstance()
	{
		static cregistry_whitelist instance;
		return &instance;
	}

	bool cregistry_whitelist::is_app_in_whitelist()
	{
		return m_bAppInWhiteLists;
	}

	bool cregistry_whitelist::is_app_can_view()
	{
		return has_action(CONST_NX_APP_ACTION_VIEW);
	}

	bool cregistry_whitelist::is_app_can_print()
	{
		return has_action(CONST_NX_APP_ACTION_PRINT);
	}

	bool cregistry_whitelist::is_app_can_save()
	{
		return has_action(CONST_NX_APP_ACTION_SAVE);
	}

	bool cregistry_whitelist::is_app_can_saveas()
	{
		return has_action(CONST_NX_APP_ACTION_SAVEAS);
	}

	bool cregistry_whitelist::is_app_can_copycontent()
	{
		return has_action(CONST_NX_APP_ACTION_COPYCONTENT);
	}

	bool cregistry_whitelist::is_app_can_printscreen()
	{
		return has_action(CONST_NX_APP_ACTION_PRINTSCREEN);
	}

	bool str_istarts_with(const std::wstring& s, const std::wstring& subStr)
	{
		if (s.length() < subStr.length())
			return false;

		std::wstring s1(s);
		std::wstring s2(subStr);
		std::transform(s1.begin(), s1.end(), s1.begin(), tolower);
		std::transform(s2.begin(), s2.end(), s2.begin(), tolower);

		return s1.find(s2) == 0 ? true : false;
	}

	// Fix bug 70888 that Cannot trust the application from UNC path
	// Acquired the process path by call GetModuleFileNameExW maybe a bit inconsistent with the path reading from registry whitelist,
	// So should do special handle for this case.
	// Like:
	//      registryPath is "\\hz-ts02.nextlabs.com\LABDATA\Public\Test tools\NextlabsTool\Tool_ReadFile-s.exe"; but
	//      curProcessPath is "\\hz-ts02\LABDATA\Public\Test tools\NextlabsTool\Tool_ReadFile-s.exe"
	bool IsUNCPathMatch(const std::wstring& curProcessPath, const std::wstring& registryPath) {
		const std::wstring prefix(L"\\\\");

		auto curPos = curProcessPath.find(prefix);
		auto regPos = registryPath.find(prefix);
		if (curPos == 0 && regPos == 0) {
			auto removePrefixCur = curProcessPath.substr(prefix.size());
			auto removePrefixReg = registryPath.substr(prefix.size());

			auto curPos1 = removePrefixCur.find(L"\\");
			auto regPos1 = removePrefixReg.find(L"\\");
			if (curPos1 != std::wstring::npos && regPos1 != std::wstring::npos) {
				auto hostCur = removePrefixCur.substr(0, curPos1);
				auto dirCur = removePrefixCur.substr(curPos1 + 1);
				std::transform(dirCur.begin(), dirCur.end(), dirCur.begin(), ::tolower);

				auto hostReg = removePrefixReg.substr(0, regPos1);
				auto dirReg = removePrefixReg.substr(regPos1 + 1);
				std::transform(dirReg.begin(), dirReg.end(), dirReg.begin(), ::tolower);

				if (str_istarts_with(hostReg, hostCur) && dirCur == dirReg) {
					::OutputDebugStringW(L"The UNC path is matched!");
					return true;
				}
			}
		}

		return false;
	}

	//HKEY_LOCAL_MACHINE\SOFTWARE\NextLabs\SkyDRM\OSRmx\whitelists
	void cregistry_whitelist::load_whitelist_from_registry()
	{
		m_strAppProcessPath = get_current_process_path();
		m_strAppProcessPath.erase(0, m_strAppProcessPath.find_first_not_of(L" "));
		m_strAppProcessPath.erase(m_strAppProcessPath.find_last_not_of(L" ") + 1);
		std::transform(m_strAppProcessPath.begin(), m_strAppProcessPath.end(), m_strAppProcessPath.begin(), ::towlower);
		m_strAppWhiteListName = get_whitelist_itemname(m_strAppProcessPath);

		std::wstring strRegKey = CONST_SZ_WHITELIST_KEY + std::wstring(L"\\") + m_strAppWhiteListName;

		::OutputDebugStringW(L"#########begin load_whitelist_from_registry###########\n");
		::OutputDebugStringW(m_strAppProcessPath.c_str());
		::OutputDebugStringW(strRegKey.c_str());

		std::wstring strAppPath = reg_get_value(strRegKey, L"path", HKEY_LOCAL_MACHINE);
		strAppPath.erase(0, strAppPath.find_first_not_of(L" "));
		strAppPath.erase(strAppPath.find_last_not_of(L" ") + 1);
		if (strAppPath.empty())
		{
			::OutputDebugStringW(L"#########end load_whitelist_from_registry strAppPath is empty###########\n");
			return;
		}

		::OutputDebugStringW(strAppPath.c_str());
		::OutputDebugStringW(L"#########end load_whitelist_from_registry###########\n");
		std::transform(strAppPath.begin(), strAppPath.end(), strAppPath.begin(), ::towlower);

		if (0 == m_strAppProcessPath.compare(strAppPath) ||
			(::PathIsUNC(m_strAppProcessPath.c_str()) && IsUNCPathMatch(m_strAppProcessPath, strAppPath)))
		{
			m_bAppInWhiteLists = true;
		}

		std::wstring strActions = reg_get_value(strRegKey, L"actions", HKEY_LOCAL_MACHINE);
		parse_actions(strActions);
	}

	void cregistry_whitelist::parse_actions(const std::wstring& strActions)
	{
		std::wstring appActions = strActions;
		appActions.erase(0, appActions.find_first_not_of(L" "));
		appActions.erase(appActions.find_last_not_of(L" ") + 1);

		if (appActions.empty())
			return;

		std::transform(appActions.begin(), appActions.end(), appActions.begin(), ::towlower);
		auto vecActions = regex_split(appActions, L"[|]+");
		for (auto item : vecActions)
		{
			std::wstring strItem = item;
			strItem.erase(0, strItem.find_first_not_of(L" "));
			strItem.erase(strItem.find_last_not_of(L" ") + 1);

			m_setActions.insert(strItem);
		}
	}

	bool cregistry_whitelist::has_action(const std::wstring& strAction)
	{
		std::wstring appAction = strAction;
		appAction.erase(0, appAction.find_first_not_of(L" "));
		appAction.erase(appAction.find_last_not_of(L" ") + 1);
		std::transform(appAction.begin(), appAction.end(), appAction.begin(), ::towlower);

		auto itFind = m_setActions.find(appAction);
		if (m_setActions.end() != itFind)
		{
			return true;
		}
		return false;
	}

	//appname:hash(apppath), for example: notepad.exe:12434231435
	std::wstring cregistry_whitelist::get_whitelist_itemname(const std::wstring& strAppPath)
	{
		std::wstring appPath(strAppPath);
		auto pos = appPath.rfind(L"\\");
		if (std::wstring::npos == pos)
			return L"";

		//std::wstring appName = appPath.substr(pos + 1);
		//appName.erase(0, appName.find_first_not_of(L" "));
		//appName.erase(appName.find_last_not_of(L" ") + 1);
		//std::transform(appName.begin(), appName.end(), appName.begin(), ::towlower);

		//std::hash<std::wstring> hash_fn;
		//size_t str_hash = hash_fn(appPath);
		//std::wstring strAppPathHash = std::to_wstring(str_hash);

		//std::wstring strItemName = appName;// +L":" + strAppPathHash;

        std::wstring strItemName = gen_md5_hex_string(appPath);

		return strItemName;
	}

    std::wstring cregistry_whitelist::gen_md5_hex_string(const std::wstring& strAppPath)
    {
        std::wstring strMd5;
        std::vector<unsigned char> md5_result;
        md5_result.resize(16, 0);

        std::string strData = to_string(strAppPath);

        if (NX::crypto::md5((const unsigned char*)strData.data(), (ULONG)strData.size(), md5_result.data()))
        {
            strMd5 = NX::conversion::to_wstring(md5_result);
        }

        return strMd5;
    }

    std::string cregistry_whitelist::to_string(const std::wstring& wstr)
    {
        using convert_t = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_t, wchar_t> strconverter;
        return strconverter.to_bytes(wstr);
    }

	std::vector<std::wstring> cregistry_whitelist::regex_split(const std::wstring& in, const std::wstring& delim)
	{
		std::wregex re{ delim };
		return std::vector<std::wstring> {
			std::wsregex_token_iterator(in.begin(), in.end(), re, -1), std::wsregex_token_iterator()
		};
	}

	std::wstring cregistry_whitelist::get_current_process_path()
	{
		DWORD dwProcessId = ::GetCurrentProcessId();
		std::wstring strAppPath = get_process_path(dwProcessId);

		strAppPath.erase(0, strAppPath.find_first_not_of(L" "));
		strAppPath.erase(strAppPath.find_last_not_of(L" ") + 1);
		return strAppPath;
	}

	std::wstring convert_long_path(const std::wstring& s)
	{
		std::wstring final_path(s);

		// convert to long path
		long     length = 0;
		TCHAR*   buffer = NULL;
		// First obtain the size needed by passing NULL and 0.
		length = GetLongPathName(final_path.c_str(), NULL, 0);
		if (length > (long)(final_path.size() + 1))
		{
			buffer = new TCHAR[length];
			length = GetLongPathName(final_path.c_str(), buffer, length);
			if (length > 0)
				final_path = buffer;
			delete[] buffer;
		}

		return final_path;
	}

	std::wstring cregistry_whitelist::get_process_path(DWORD ProcessId)
	{
		DWORD		dwErr = 0;
		HANDLE		hProcess = NULL;
		std::wstring	strAppImagePath;

		do
		{
			//::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ProcessId); //PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
			hProcess = ::GetCurrentProcess(); // Fix bug 60070 - Pops "No permission to view" even file has rights in Adobe Reader DC
			if (!hProcess)
			{
				dwErr = ::GetLastError();
				std::wstring strErr = L"get_process_path GetCurrentProcess error: " + std::to_wstring(dwErr) + L"\n";
				OutputDebugStringW(strErr.c_str());
				break;
			}

			wchar_t szBuf[MAX_PATH] = { 0 };
			DWORD dwRet = ::GetModuleFileNameExW(hProcess, NULL, szBuf, sizeof(szBuf)/sizeof(szBuf[0]));
			if (dwRet > 0)
			{
				strAppImagePath = szBuf;

				// One special case is that for jt2go 32 bit, it will acquire short path instead of full path
				// and this will result in mismatch with whitelists registry value of "path".(Bug 66252)
				if (strAppImagePath.find(L"~") != std::wstring::npos) {
					strAppImagePath = convert_long_path(strAppImagePath);
					::OutputDebugString(L"------ the strAppImagePath is ------>");
					::OutputDebugString(strAppImagePath.c_str());
				}
			}
			else
			{
				dwErr = ::GetLastError();
				std::wstring strErr = L"get_process_path GetModuleFileNameExW error: " + std::to_wstring(dwErr) + L"\n";
				OutputDebugStringW(strErr.c_str());
			}

		} while (FALSE);

		::CloseHandle(hProcess);
		return strAppImagePath;
	}

	std::wstring cregistry_whitelist::reg_get_value(const std::wstring &strSubKey, const std::wstring &strValueName, HKEY hRoot)
	{
		HKEY hKey = NULL;
		std::wstring strValue;
		LSTATUS lStatus = ::RegOpenKeyExW(hRoot, strSubKey.c_str(), 0, KEY_QUERY_VALUE| KEY_WOW64_64KEY, &hKey);
		if (lStatus == ERROR_SUCCESS)
		{
			DWORD dwType = REG_SZ;
			std::vector<unsigned char> buf;
			unsigned long value_size = 1;

			lStatus = ::RegQueryValueExW(hKey, strValueName.c_str(), NULL, &dwType, (LPBYTE)buf.data(), &value_size);
			if (ERROR_SUCCESS == lStatus)
			{
				buf.resize(value_size, 0);
				lStatus = ::RegQueryValueExW(hKey, strValueName.c_str(), NULL, &dwType, (LPBYTE)buf.data(), &value_size);
				if (ERROR_SUCCESS == lStatus)
				{
					strValue = (const wchar_t*)buf.data();
				}
			}
		}

		if (hKey != NULL)
		{
			RegCloseKey(hKey);
			hKey = NULL;
		}

		return std::move(strValue);
	}
}