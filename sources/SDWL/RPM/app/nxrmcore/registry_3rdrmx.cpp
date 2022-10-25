
#include "stdafx.h"
#include "registry_3rdrmx.h"
#include <psapi.h>
#include <regex>

#include <string>
#include <algorithm>

#pragma comment(lib, "crypt32.lib")

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

const wchar_t CONST_SZ_3RDRMX_KEY[] = L"SOFTWARE\\NextLabs\\SkyDRM\\3rdRMX";

namespace nx {
	cregistry_3rdrmx::cregistry_3rdrmx()
	{
		m_bAppHasRMX = false;
		load_rmx_from_registry();
	}

	cregistry_3rdrmx::~cregistry_3rdrmx()
	{

	}

	cregistry_3rdrmx* cregistry_3rdrmx::getInstance()
	{
		static cregistry_3rdrmx instance;
		return &instance;
	}

	bool cregistry_3rdrmx::is_app_has_rmx()
	{
		return m_bAppHasRMX;
	}

	//appname:hash(apppath), for example: notepad.exe:12434231435
	std::wstring cregistry_3rdrmx::get_app_name(const std::wstring& strAppPath)
	{
		std::wstring appPath(strAppPath);
		auto pos = appPath.rfind(L"\\");
		if (std::wstring::npos == pos)
			return L"";

		std::wstring appName = appPath.substr(pos + 1);
		appName.erase(0, appName.find_first_not_of(L" "));
		appName.erase(appName.find_last_not_of(L" ") + 1);
		std::transform(appName.begin(), appName.end(), appName.begin(), ::towlower);

		std::hash<std::wstring> hash_fn;
		size_t str_hash = hash_fn(appPath);
		std::wstring strAppPathHash = std::to_wstring(str_hash);

		std::wstring strItemName = appName;
		return strItemName;
	}

	void cregistry_3rdrmx::load_rmx_from_registry()
	{
		m_strAppProcessPath = get_current_process_path();
		m_strAppProcessPath.erase(0, m_strAppProcessPath.find_first_not_of(L" "));
		m_strAppProcessPath.erase(m_strAppProcessPath.find_last_not_of(L" ") + 1);
		std::transform(m_strAppProcessPath.begin(), m_strAppProcessPath.end(), m_strAppProcessPath.begin(), ::towlower);
		m_strAppName = get_app_name(m_strAppProcessPath);

		std::wstring strRegKey = CONST_SZ_3RDRMX_KEY + std::wstring(L"\\") + m_strAppName;

		::OutputDebugStringW(L"#########begin load_3rdrmx_from_registry###########\n");
		::OutputDebugStringW(m_strAppProcessPath.c_str());
		::OutputDebugStringW(strRegKey.c_str());

		std::wstring strAppPath = reg_get_value(strRegKey, L"path", HKEY_LOCAL_MACHINE);
		strAppPath.erase(0, strAppPath.find_first_not_of(L" "));
		strAppPath.erase(strAppPath.find_last_not_of(L" ") + 1);
		if (strAppPath.empty())
		{
			::OutputDebugStringW(L"#########end load_3rdrmx_from_registry strAppPath is empty###########\n");
			return;
		}

		::OutputDebugStringW(strAppPath.c_str());
		::OutputDebugStringW(L"#########end load_3rdrmx_from_registry###########\n");
		std::transform(strAppPath.begin(), strAppPath.end(), strAppPath.begin(), ::towlower);

		std::wstring strRMXPath = reg_get_value(strRegKey, L"rmx", HKEY_LOCAL_MACHINE);
		strRMXPath.erase(0, strRMXPath.find_first_not_of(L" "));
		strRMXPath.erase(strRMXPath.find_last_not_of(L" ") + 1);
		if (strRMXPath.empty())
		{
			::OutputDebugStringW(L"#########end load_3rdrmx_from_registry strRMXPath is empty###########\n");
			return;
		}

		::OutputDebugStringW(strRMXPath.c_str());
		::OutputDebugStringW(L"#########end load_whitelist_from_registry###########\n");
		std::transform(strRMXPath.begin(), strRMXPath.end(), strRMXPath.begin(), ::towlower);

		if (0 == m_strAppProcessPath.compare(strAppPath) && strRMXPath.size() > 0)
		{
			m_bAppHasRMX = true;
		}
		m_strAppRMXPath = strRMXPath;
	}

	std::vector<std::wstring> cregistry_3rdrmx::regex_split(const std::wstring& in, const std::wstring& delim)
	{
		std::wregex re{ delim };
		return std::vector<std::wstring> {
			std::wsregex_token_iterator(in.begin(), in.end(), re, -1), std::wsregex_token_iterator()
		};
	}

	std::wstring cregistry_3rdrmx::get_current_process_path()
	{
		DWORD dwProcessId = ::GetCurrentProcessId();
		std::wstring strAppPath = get_process_path(dwProcessId);

		strAppPath.erase(0, strAppPath.find_first_not_of(L" "));
		strAppPath.erase(strAppPath.find_last_not_of(L" ") + 1);
		return strAppPath;
	}

	extern std::wstring convert_long_path(const std::wstring& s);
	
	std::wstring cregistry_3rdrmx::get_process_path(DWORD ProcessId)
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

	std::wstring cregistry_3rdrmx::reg_get_value(const std::wstring &strSubKey, const std::wstring &strValueName, HKEY hRoot)
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

	BOOL CheckCertificateInfo(PCCERT_CONTEXT pCertContext);
	bool verify_nextlabs_signature(const std::wstring& module)
	{
		bool bRet = false;
		HCERTSTORE hStore = NULL;
		HCRYPTMSG hMsg = NULL;
		PCCERT_CONTEXT pCertContext = NULL;
		BOOL fResult;
		DWORD dwEncoding, dwContentType, dwFormatType;
		PCMSG_SIGNER_INFO pSignerInfo = NULL;
		DWORD dwSignerInfo;
		CERT_INFO CertInfo;

		do
		{
			// Get message handle and store handle from the signed file.
			fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
				module.c_str(),
				CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
				CERT_QUERY_FORMAT_FLAG_BINARY,
				0,
				&dwEncoding,
				&dwContentType,
				&dwFormatType,
				&hStore,
				&hMsg,
				NULL);
			if (!fResult)
				break;

			// Get signer information size.
			fResult = CryptMsgGetParam(hMsg,
				CMSG_SIGNER_INFO_PARAM,
				0,
				NULL,
				&dwSignerInfo);
			if (!fResult)
				break;

			// Allocate memory for signer information.
			pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
			if (!pSignerInfo)
				break;

			// Get Signer Information.
			fResult = CryptMsgGetParam(hMsg,
				CMSG_SIGNER_INFO_PARAM,
				0,
				(PVOID)pSignerInfo,
				&dwSignerInfo);
			if (!fResult)
				break;

			// Search for the signer certificate in the temporary 
			// certificate store.
			CertInfo.Issuer = pSignerInfo->Issuer;
			CertInfo.SerialNumber = pSignerInfo->SerialNumber;

			pCertContext = CertFindCertificateInStore(hStore,
				ENCODING,
				0,
				CERT_FIND_SUBJECT_CERT,
				(PVOID)& CertInfo,
				NULL);
			if (!pCertContext)
				break;

			if (CheckCertificateInfo(pCertContext) == false)
				break;

			bRet = true;
		} while (false);

		// Clean up.
		if (pSignerInfo != NULL) LocalFree(pSignerInfo);
		if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
		if (hStore != NULL) CertCloseStore(hStore, 0);
		if (hMsg != NULL) CryptMsgClose(hMsg);

		return bRet;
	}

	BYTE NextLabs_SerialNum[] = { 0x0c, 0x48, 0x42, 0xfc, 0x99, 0xfa, 0x72, 0x0b, 0xff, 0x7b, 0x0a, 0x47, 0x7c, 0x33, 0x06, 0xba };
	BYTE NextLabs_SerialNum2[] = { 0x0c, 0xe3, 0x70, 0x3c, 0xb4, 0x46, 0x23, 0xb6, 0x24, 0x1d, 0x04, 0xb0, 0xf1, 0xa9, 0x11, 0x66 };
	std::wstring NextLabs_Info = L"nextlabs inc.";

	BOOL CheckCertificateInfo(PCCERT_CONTEXT pCertContext)
	{
		BOOL fReturn = FALSE;
		LPTSTR szName = NULL;
		DWORD dwData;

		do
		{
			//// Print Serial Number.
			//dwData = pCertContext->pCertInfo->SerialNumber.cbData;

			//bool bValidSerial = true;
			//for (DWORD n = 0; n < dwData; n++)
			//{
			//	if (n < 16)
			//	{
			//		if (pCertContext->pCertInfo->SerialNumber.pbData[dwData - (n + 1)] != NextLabs_SerialNum[n])
			//		{
			//			bValidSerial = false;
			//			break;
			//		}
			//	}
			//}

			//// check certificate 2
			//if (bValidSerial == false)
			//{
			//	bValidSerial = true;
			//	for (DWORD n = 0; n < dwData; n++)
			//	{
			//		if (n < 16)
			//		{
			//			if (pCertContext->pCertInfo->SerialNumber.pbData[dwData - (n + 1)] != NextLabs_SerialNum2[n])
			//			{
			//				bValidSerial = false;
			//				break;
			//			}
			//		}
			//	}
			//}
			//if (bValidSerial == false) break;

			// Get Subject name size.
			if (!(dwData = CertGetNameString(pCertContext,
				CERT_NAME_SIMPLE_DISPLAY_TYPE,
				0,
				NULL,
				NULL,
				0)))
			{
				break;
			}

			// Allocate memory for subject name.
			szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
			if (!szName)
			{
				break;
			}

			// Get subject name.
			if (!(CertGetNameString(pCertContext,
				CERT_NAME_SIMPLE_DISPLAY_TYPE,
				0,
				NULL,
				szName,
				dwData)))
			{
				break;
			}

			// Print Subject Name.
			std::wstring subject(szName);
			std::transform(subject.begin(), subject.end(), subject.begin(), tolower);
			if (NextLabs_Info != subject)
				break;

			fReturn = TRUE;
		} while (false);

		if (szName != NULL) LocalFree(szName);

		return fReturn;
	}


}