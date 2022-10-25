
#include "pch.h"
#include "Shlwapi.h"
#include "nxlfilehandler.h"
#include "nudf\eh.hpp"
#include "RegistryServiceEntry.h"
#include <array>
#include <algorithm>
#include <regex>


const char		NX_SECURITY_CODE[] = "{6829b159-b9bb-42fc-af19-4a6af3c9fcf6}";
const wstring	NX_REGKEY_NXRMHANDLER = L"SOFTWARE\\NextLabs\\SkyDRM\\nxrmhandler";
const wstring	NX_REGKEY_LOCALAPP = L"SOFTWARE\\NextLabs\\SkyDRM\\LocalApp";
const wstring	NX_REGITEM_NAME = L"Executable";
const wstring	NX_NXRMVIEWER = L"nxrmviewer.exe";
const wstring	NX_REGVERB_OPEN = L"open";

const wchar_t	NX_FILE_BACKSLASH = '\\';
const wchar_t	NX_FILE_DOT = '.';

std::wstring trim_and_tolower(const std::wstring& str)
{
    std::wstring strData(str);
    strData.erase(0, strData.find_first_not_of(L" "));
    strData.erase(strData.find_last_not_of(L" ") + 1);

    if (strData.empty())
    {
        return strData;
    }

    std::transform(strData.begin(), strData.end(), strData.begin(), ::towlower);
    return strData;
}

CNXLFileHandler::CNXLFileHandler(const wstring& strExeFile)
	: m_pInstance(nullptr)
	, m_pTenant(nullptr)
	, m_pSDRmUser(nullptr)
	, m_strNxlFileHandlerApp(strExeFile)
{
	//CleanUpStringObject();
}

CNXLFileHandler::~CNXLFileHandler()
{
	CleanUp();
}

void CNXLFileHandler::OpenNxlFile(const wstring& strNxlFile, const wstring& arguments)
{
	::OutputDebugStringA("CNXLFileHandler::OpenNxlFile Enter \n");

	if (!IsUserLogin()) {
		NotifyUserLogin();
		::OutputDebugStringA("CNXLFileHandler::OpenNxlFile Leave \n");
		return;
	}

	wstring strFile = NX::fs::dos_fullfilepath(strNxlFile).path();
	PrepareOriginalFile(strFile);
	m_arguments = arguments;
	//if (m_strOriginalFileExt.empty()) {
	//	::OutputDebugStringA("CNXLFileHandler::OpenNxlFile error file path \n");
	//	return;
	//}

	LaunchAppToViewFile();

	//if (IsUserLogin())
	//{
	//	LaunchAppToViewFile();
	//}
	//else
	//{
	//	NotifyUserLogin();
	//}

	::OutputDebugStringA("CNXLFileHandler::OpenNxlFile Leave \n");
}

//Example strFile: NXApplication-2019-09-09-16-55-42.swift.nxl
void CNXLFileHandler::PrepareOriginalFile(const wstring& strFile)
{
    m_strOriginalNxlFile = strFile;

    size_t nxlExtPos = strFile.rfind(NX_FILE_DOT); //here . position (.nxl)
    std::wstring fileExt;
    if (nxlExtPos != wstring::npos)
    {
        fileExt = strFile.substr(nxlExtPos);
        fileExt = trim_and_tolower(fileExt);
    }

    if (0 == fileExt.compare(L".nxl"))
    {
		if (!m_pSDRmUser)
		{
			m_strOriginalFileExt = L"";
			return;
		}

		wstring strOriginalFile = strFile.substr(0, nxlExtPos); // here like: NXApplication-2019-09-09-16-55-42.swift
		size_t pos = strOriginalFile.rfind(NX_FILE_DOT); //here . position (.swift)
		if (pos != wstring::npos)
		{
			//Protect txt.1 (*.1, *.2...)
			std::wstring REMOVE_SYSTEM_ATUO_RENAME_POSTFIX = L"\\.\\d{1,}$";

			//Protect docx(001)
			std::wstring REMOVE_SYSTEM_ATUO_RENAME_POSTFIX2 = L"\\(\\d{0,}\\)$";

			std::wregex regex1(REMOVE_SYSTEM_ATUO_RENAME_POSTFIX, std::regex_constants::ECMAScript | std::regex_constants::icase);
			if (std::regex_search(strOriginalFile, regex1)) {
				strOriginalFile = std::regex_replace(strOriginalFile, regex1, L"");
				pos = strOriginalFile.rfind(NX_FILE_DOT);
			}

			std::wregex regex2(REMOVE_SYSTEM_ATUO_RENAME_POSTFIX2, std::regex_constants::ECMAScript | std::regex_constants::icase);
			if (std::regex_search(strOriginalFile, regex2)) {
				strOriginalFile = std::regex_replace(strOriginalFile, regex2, L"");
				pos = strOriginalFile.rfind(NX_FILE_DOT);
			}

			if (pos == wstring::npos) {
				std::wstring errMsg = L"CNXLFileHandler::PrepareOriginalFile, error file path: ";
				errMsg += strFile;
				::OutputDebugStringW(errMsg.c_str());
				return;
			}

			//m_strOriginalFileExt = strOriginalFile.substr(pos); //here like .swift
			 std::wstring plainExtention = strOriginalFile.substr(pos); //here like .swift
			 m_strOriginalFileExt = plainExtention;

			if (!QueryFileAssciation()) {

				std::wstring fileOriginalExtention = L"";
				SDWLResult ret = m_pSDRmUser->GetNxlFileOriginalExtention(m_strOriginalNxlFile, fileOriginalExtention);
				if (0 != ret.GetCode()) {
					::OutputDebugStringW(L"GetNxlFileOriginalExtention failed");
					m_strOriginalFileExt = plainExtention;
					return;
				}
				else {
					::OutputDebugStringW(L"GetNxlFileOriginalExtention succeeded");
					m_strOriginalFileExt = fileOriginalExtention;

					if (!QueryFileAssciation()) {
						m_strOriginalFileExt = plainExtention;
					}
				}
			}
		}
		else
		{
			/*	std::wstring errMsg = L"CNXLFileHandler::PrepareOriginalFile, error file path: ";
				errMsg += strFile;
				::OutputDebugStringW(errMsg.c_str());*/

			std::wstring fileOriginalExtention = L"";
			SDWLResult ret = m_pSDRmUser->GetNxlFileOriginalExtention(m_strOriginalNxlFile, fileOriginalExtention);
			if (0 != ret.GetCode()) {
				::OutputDebugStringW(L"GetNxlFileOriginalExtention failed");
				m_strOriginalFileExt = L"";
				return;
			}
			else {
				::OutputDebugStringW(L"GetNxlFileOriginalExtention succeeded");
				m_strOriginalFileExt = fileOriginalExtention;
			}
		}
    }
    else
    {
        ::OutputDebugStringA("CNXLFileHandler::PrepareOriginalFile normal file (not a nxl file) \n");

        m_strRPMNxlFile = strFile;
        m_strOriginalFileExt = L"";
        if (nxlExtPos != wstring::npos)
        {
            m_strOriginalFileExt = strFile.substr(nxlExtPos);
        }
    }
}

bool CNXLFileHandler::IsUserLogin()
{
	if (m_pSDRmUser)
	{
		return true;
	}

	string	strPasscode(NX_SECURITY_CODE);
	SDWLResult result = RPMGetCurrentLoggedInUser(strPasscode, m_pInstance, m_pTenant, m_pSDRmUser);

	if ((0 == result.GetCode()) && m_pSDRmUser)
	{
		return true;
	}

	char szBuf[1024] = { 0 };
	sprintf_s(szBuf, sizeof(szBuf) / sizeof(char), "CNXLFileHandler::IsUserLogin code:%d message:%s \n",
		result.GetCode(), result.GetMsg().c_str());

	::OutputDebugStringA(szBuf);
	return false;
}

void CNXLFileHandler::NotifyUserLogin()
{
	if (!m_pInstance)
	{
		::OutputDebugStringA("CNXLFileHandler::NotifyUserLogin m_pInstance is NULL");
		return;
	}

    std::wstring nxlfile = L"\"" + m_strOriginalNxlFile + L"\"";
	SDWLResult result = m_pInstance->RPMRequestLogin(m_strNxlFileHandlerApp, nxlfile);
}

bool CNXLFileHandler::PreOpenNormalFile()
{
    bool ret = false;

    do
    {
        if (0 != m_strOriginalNxlFile.compare(m_strRPMNxlFile))
        {
            break;
        }

        NX::win::file_association fileassociation(m_strOriginalFileExt);
        m_strOriginalExtFileHandlerCmd = fileassociation.get_executable();
        if (m_strOriginalExtFileHandlerCmd.empty())
        {
            break;
        }

        size_t nxlExtPos = m_strOriginalExtFileHandlerCmd.rfind(NX_FILE_DOT);
        std::wstring fileExt;
        if (nxlExtPos != wstring::npos)
        {
            fileExt = m_strOriginalExtFileHandlerCmd.substr(nxlExtPos);
            fileExt = trim_and_tolower(fileExt);
        }

        if (0 == fileExt.compare(L".exe"))
        {
            ret = true;
            break;
        }

        std::wstring dbgMsg = L"CNXLFileHandler::PreOpenNormalFile, ";
        dbgMsg += m_strOriginalExtFileHandlerCmd;

        ::OutputDebugStringW(dbgMsg.c_str());
        m_strOriginalExtFileHandlerCmd.clear();

    } while (false);

    return ret;
}

bool CNXLFileHandler::PreOpenNxlFile()
{
    return QueryFileAssciation()
        && IsOfficeRmxAddInEnabled()
        && CopyFileToRPMFolder();
}

bool CNXLFileHandler::QueryFileAssciation()
{
	if (!m_pInstance)
	{
		::OutputDebugStringA("CNXLFileHandler::QueryFileAssciation m_pInstance is NULL");
		return false;
	}

	std::wstring strKey = NX_REGKEY_NXRMHANDLER + NX_FILE_BACKSLASH + m_strOriginalFileExt;
	SkyDRM::CRegistryServiceEntry registryService(m_pInstance, NX_SECURITY_CODE);
	SDWLResult ret = registryService.get_value(HKEY_LOCAL_MACHINE, strKey, L"", m_strOriginalExtFileHandlerCmd);

	if (0 != ret.GetCode())
	{
		std::string strMsg = "CNXLFileHandler::QueryFileAssciation return false, message : " + ret.GetMsg() + "\n";
		::OutputDebugStringA(strMsg.c_str());
		return false;
	}

	return true;
}

bool CNXLFileHandler::IsOfficeRmxAddInEnabled()
{
	std::wstring strCustomerApp = m_strOriginalExtFileHandlerCmd;
	strCustomerApp.erase(0, strCustomerApp.find_first_not_of(L" "));
	strCustomerApp.erase(strCustomerApp.find_last_not_of(L" ") + 1);

	if (strCustomerApp.empty())
		return true;

	std::wstring strAppName;
	auto pos = strCustomerApp.rfind(L"\\");
	if (std::wstring::npos != pos)
	{
		strAppName = strCustomerApp.substr(pos + 1);
	}

	std::transform(strAppName.begin(), strAppName.end(), strAppName.begin(), ::towlower);
	static std::map<std::wstring, std::wstring> s_mapOfficeApp = { {L"excel.exe", L"Excel"}, {L"winword.exe", L"Word"}, {L"powerpnt.exe", L"PowerPoint"} };
	auto itFind = s_mapOfficeApp.find(strAppName);
	if (s_mapOfficeApp.end() == itFind)
	{
		return true;
	}

	if (!IsOfficeAppLoadBehaviorEnabled(itFind->second))
	{
		::OutputDebugStringA("CNXLFileHandler::IsOfficeRmxAddInEnabled LoadBehavior != 3");
		return false;
	}

	return true;
}

void CNXLFileHandler::GetRegisterNxlFile(std::map<std::wstring, std::wstring>& mapFile)
{
	mapFile.clear();

	const int nMaxKeyLength = 1024;
	const int nMaxValueLength = 1024;
	DWORD dwType = REG_SZ;

	HKEY hKey = NULL;
	const std::wstring strPath = L"Software\\NextLabs\\SkyDRM\\Session";
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return;
	}

	TCHAR    achKey[nMaxKeyLength] = { 0 };
	DWORD    cbName = 0;
	TCHAR    achClass[MAX_PATH] = L"";
	DWORD    cchClassName = MAX_PATH;
	DWORD    cSubKeys = 0;
	DWORD    cbMaxSubKey = 0;
	DWORD    cchMaxClass = 0;
	DWORD    dwValue = 0;
	DWORD    cchMaxValue = 0;
	DWORD    cbMaxValueData = 0;
	DWORD    cbSecurityDescriptor = 0;
	FILETIME ftLastWriteTime;

	LSTATUS ret = RegQueryInfoKey(hKey, achClass, &cchClassName, NULL, &cSubKeys,
		&cbMaxSubKey, &cchMaxClass, &dwValue, &cchMaxValue, &cbMaxValueData,
		&cbSecurityDescriptor, &ftLastWriteTime);

	if (ERROR_SUCCESS != ret)
	{
		RegCloseKey(hKey);
		return;
	}

	if (0 == dwValue)
	{
		RegCloseKey(hKey);
		return;
	}

	TCHAR  achValue[nMaxValueLength] = { 0 };
	DWORD cchValue = nMaxValueLength;

	for (DWORD i = 0; i < dwValue; i++)
	{
		cchValue = nMaxValueLength;
		achValue[0] = '\0';

		if (ERROR_SUCCESS == RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL))
		{
			TCHAR szBuffer[nMaxValueLength] = { 0 };
			DWORD dwNameLen = nMaxValueLength;
			DWORD rQ = 0;

			rQ = RegQueryValueEx(hKey, achValue, 0, &dwType, (LPBYTE)szBuffer, &dwNameLen);
			if (rQ == ERROR_SUCCESS)
			{
				mapFile[achValue] = szBuffer;
			}
		}
	}

	RegCloseKey(hKey);
}

bool CNXLFileHandler::GetLastFileTime(const std::wstring& strFile, __time64_t& ftime)
{
	ftime = 0;
	struct _stat64i32 buf;
	if (0 != _wstat(strFile.c_str(), &buf))
	{
		return false;
	}

	ftime = std::max<__time64_t>({ buf.st_atime, buf.st_ctime, buf.st_mtime });
	return true;
}

bool CNXLFileHandler::IsFileOpened()
{
	wstring excat_nxl_file_path = NX::fs::dos_fullfilepath(m_strOriginalNxlFile).path();
	SkyDRM::CSDRmNXLFile nxlFile(excat_nxl_file_path);
	std::string strSrcDuid = nxlFile.GetDuid();
	uint64_t u64SrcFileSize = nxlFile.GetFileSize();
	uint64_t u64SrcMTime = nxlFile.GetDateModified();

	std::map<std::wstring, std::wstring> mapFile;
	GetRegisterNxlFile(mapFile);

	__time64_t latestFtime = 0;

	bool bOpened = false;
	for (auto item : mapFile)
	{
		if (0 == NX::icompare(excat_nxl_file_path, item.second))
		{
			SkyDRM::CSDRmNXLFile nxlDstFile(item.first + L".nxl");
			std::string strDstDuid = nxlDstFile.GetDuid();
			uint64_t u64DstFileSize = nxlDstFile.GetFileSize();
			if (0 != strSrcDuid.compare(strDstDuid))
			{
				::OutputDebugStringA("CNXLFileHandler::IsFileOpened original file , RPM file DUID not equal \n");
				std::wstring strSrcLog = L"CNXLFileHandler::IsFileOpened original file path = " + m_strOriginalNxlFile + L"\n";
				std::wstring strDstLog = L"CNXLFileHandler::IsFileOpened RPM file path = " + item.first + L".nxl \n";
				::OutputDebugStringW(strSrcLog.c_str());
				::OutputDebugStringW(strDstLog.c_str());
				continue;
			}

			uint64_t u64DstMTime = nxlDstFile.GetDateModified();
			if (u64SrcMTime > u64DstMTime)
			{
				::OutputDebugStringA("CNXLFileHandler::IsFileOpened original file modify time > RPM file");
				std::wstring strSrcLog = L"CNXLFileHandler::IsFileOpened original file path = " + m_strOriginalNxlFile + L"\n";
				std::wstring strDstLog = L"CNXLFileHandler::IsFileOpened RPM file path = " + item.first + L".nxl \n";
				::OutputDebugStringW(strSrcLog.c_str());
				::OutputDebugStringW(strDstLog.c_str());
				continue;
			}

			__time64_t ftime = 0;
			if(GetLastFileTime(item.first, ftime))
			{
				if (ftime > latestFtime)
				{
					latestFtime = ftime;
					m_strRPMNxlFile = item.first;
					bOpened = true;
				}
			}

			if (bOpened) {
				//sync file attributes between original nxl file and have copyed into RPM dir file(RPM file)
				//1 get ori nxl file attributes
				DWORD ori_nxl_file_attributes;
				ori_nxl_file_attributes = ::GetFileAttributesW(excat_nxl_file_path.c_str());

				//2 sync attributes to RPM file
				WIN32_FIND_DATA pNextInfo;
				HANDLE h = FindFirstFile((m_strRPMNxlFile + L".nxl").c_str(), &pNextInfo);// need call FindFirstFile first then can operate to nxl file
				FindClose(h);

				// sync RPM nxl file attributes
				BOOL reb = ::SetFileAttributesW((m_strRPMNxlFile + L".nxl").c_str(), ori_nxl_file_attributes);

				// sync RPM hidden file attributes
				SDWLResult ret = m_pInstance->RPMSetFileAttributes((m_strRPMNxlFile).c_str(), ori_nxl_file_attributes);
				if (0 != ret.GetCode()) {
					::OutputDebugStringW(L"sync file attributes between original nxl file and RPM file have failed");
				}
				else {
					::OutputDebugStringW(L"sync file attributes between original nxl file and RPM file have succeeded");
				}
			}
		}
	}

	return bOpened;
}

bool CNXLFileHandler::CopyFileToRPMFolder()
{
	if (!m_pInstance)
	{
		::OutputDebugStringA("CNXLFileHandler::CopyFileToRPMFolder m_pInstance is NULL");
		return false;
	}

	//if (IsFileOpened())
		//return true;

	m_strRPMNxlFile = L"";
	SDWLResult result = m_pInstance->RPMEditCopyFile(NX::fs::dos_fullfilepath(m_strOriginalNxlFile).path(), m_strRPMNxlFile);

	if (!m_strRPMNxlFile.empty())
	{
		return true;
	}

	char szBuf[1024] = { 0 };
	sprintf_s(szBuf, sizeof(szBuf) / sizeof(char), "CNXLFileHandler::CopyFileToRPMFolder code:%d message:%s \n",
		result.GetCode(), result.GetMsg().c_str());

	::OutputDebugStringA(szBuf);
	return false;
}

void CNXLFileHandler::LaunchAppToViewFile()
{
    if (PreOpenNormalFile() || PreOpenNxlFile())
    {
        LaunchCustomerAppToView();
    }
    else
    {
        LaunchNXAppToView();
    }
}

void CNXLFileHandler::LaunchCustomerAppToView()
{
	std::wstring strRMPNxlFile = L"\"" + m_strRPMNxlFile + L"\"";

	//For .xls, .xlsx file if use ShellExecuteExW to open powerpoint exe, the view will be messy
	if (IsExcelFileExtension(m_strOriginalFileExt))
	{
		LaunchProcess(NX_REGVERB_OPEN, strRMPNxlFile);
	}
	else
	{
		wstring strCustomerApp = m_strOriginalExtFileHandlerCmd;
		strCustomerApp.erase(0, strCustomerApp.find_first_not_of(L" "));
		strCustomerApp.erase(strCustomerApp.find_last_not_of(L" ") + 1);
		if (strCustomerApp.empty())
		{
			LaunchProcess(NX_REGVERB_OPEN, strRMPNxlFile);
		}
		else
		{
			std::wstring strExeParam(strRMPNxlFile);
			std::wstring strExePath(strCustomerApp);
			std::transform(strExePath.begin(), strExePath.end(), strExePath.begin(), ::towlower);
			std::wstring strExt(L".exe");
			auto pos = strExePath.rfind(strExt);
			if ((std::wstring::npos != pos) && (pos + strExt.length() < strCustomerApp.length()))
			{
				std::wstring strCommand = strCustomerApp.substr(pos+1 + strExt.length());
				strCustomerApp = strCustomerApp.substr(0, pos + strExt.length());
				if (strCustomerApp.at(0) == L'\"')
				{
					strCustomerApp += L"\"";
				}

				strExeParam = strCommand + L" " + strRMPNxlFile;
			}
			LaunchProcessEx(NX_REGVERB_OPEN, strCustomerApp, strExeParam);
		}
	}
}

void CNXLFileHandler::LaunchNXAppToView()
{
	if (!m_pInstance)
	{
		::OutputDebugStringA("CNXLFileHandler::LuanchNXAppToView m_pInstance is NULL");
		return;
	}

	wstring strItemValue;
	SkyDRM::CRegistryServiceEntry registryService(m_pInstance, NX_SECURITY_CODE);
	SDWLResult ret = registryService.get_value(HKEY_LOCAL_MACHINE, NX_REGKEY_LOCALAPP, NX_REGITEM_NAME, strItemValue);
	if (0 != ret.GetCode())
	{//Not find NX_REGKEY_LOCALAPP in register
		::OutputDebugStringA("CNXLFileHandler::LuanchNXAppToView not find SOFTWARE\\NextLabs\\SkyDRM\\LocalApp in registry");

		//wstring strTarget = NX_REGITEM_NAME;
		//wstring strMessage = L" not found in registry:" + NX_REGKEY_LOCALAPP;
		wstring strMessage = L"To view this file, ensure that Rights Management Desktop (RMD) for Windows is installed.";
		m_pInstance->RPMNotifyMessage(L"SkyDRM", L"", strMessage, 1); //strTarget
		return;
	}

	size_t pos = strItemValue.rfind(NX_FILE_BACKSLASH); //'\\'
	wstring strFolder = strItemValue.substr(0, pos + 1);
	wstring strNxrmviewerPath = strFolder + NX_NXRMVIEWER;

	if (!::PathFileExistsW(strNxrmviewerPath.c_str()))
	{// Not find nxrmviewer.exe
		::OutputDebugStringA("CNXLFileHandler::LuanchNXAppToView not find nxrmviewer.exe");
		wstring strMessage = L"nxrmviewer.exe not found in : " + strNxrmviewerPath;
		m_pInstance->RPMNotifyMessage(L"SkyDRM", L"", strMessage, 1);

		return;
	}
	
	wstring strParameters = std::wstring() + L"-view " + L"\"" + NX::fs::dos_fullfilepath(m_strOriginalNxlFile).path() + L"\"" + L" " + m_arguments;
	LaunchProcessEx(NX_REGVERB_OPEN, strNxrmviewerPath, strParameters);
}

bool CNXLFileHandler::LaunchProcess(const wstring& strVerb,
	const wstring& strParameter)
{
    HINSTANCE hRet = ShellExecuteW(NULL, strVerb.c_str(), strParameter.c_str(), NULL, NULL, SW_SHOWNORMAL);

    if (42 == (int)hRet)
    {
        ::OutputDebugStringA("CNXLFileHandler::LaunchProcess, Try Again Operation is NULL.");
        hRet = ShellExecuteW(NULL, NULL, strParameter.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    if ((int)hRet > SE_ERR_DLLNOTFOUND)
    {
        return true;
    }

    DWORD dwErr = ::GetLastError();
    char szBuf[1024] = { 0 };
    sprintf_s(szBuf, sizeof(szBuf) / sizeof(char), "CNXLFileHandler::LaunchProcess errorcode:%lu hRet:%d \n", dwErr, (int)hRet);
    ::OutputDebugStringA(szBuf);

    return false;
}

bool CNXLFileHandler::LaunchProcessEx(const wstring& strVerb,
	const wstring& strExeFile,
	const wstring& strParameter)
{
	SHELLEXECUTEINFO ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = strVerb.c_str();
	ShExecInfo.lpFile = strExeFile.c_str();
	ShExecInfo.lpParameters = strParameter.c_str();
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOWNORMAL;
	ShExecInfo.hInstApp = NULL;

	if (::ShellExecuteExW(&ShExecInfo))
	{
		return true;
	}

	DWORD dwErr = ::GetLastError();

	char szBuf[1024] = { 0 };
	sprintf_s(szBuf, sizeof(szBuf) / sizeof(char), "CNXLFileHandler::LaunchProcessEx errorcode:%lu \n", dwErr);
	::OutputDebugStringA(szBuf);

	return false;
}

bool CNXLFileHandler::IsExcelFileExtension(const wstring& strFileExtension)
{
	std::array<std::wstring, 10> arrFileExt = { L".xls", L".xlsx", L".xlt",
		L".xltx", L".xlsb", L".csv", L".ods", L".xlm", L".xltm", L".xlsm" };

	std::wstring strExt = strFileExtension;
	strExt.erase(0, strExt.find_first_not_of(L" "));
	strExt.erase(strExt.find_last_not_of(L" ") + 1);
	std::transform(strExt.begin(), strExt.end(), strExt.begin(), ::towlower);

	for (auto item : arrFileExt)
	{
		if (0 == strExt.compare(item))
			return true;
	}

	return false;
}

bool CNXLFileHandler::IsOfficeAppLoadBehaviorEnabled(const std::wstring& strAppName)
{
	uint32_t u32Value = 0;
	std::wstring strItemName(L"LoadBehavior");
	//HKEY_CURRENT_USER, like : Software\Microsoft\Office\Excel\Addins\NxlRmAddin
	{
		std::wstring strKey = L"Software\\Microsoft\\Office\\" + strAppName + L"\\Addins\\NxlRmAddin";
		SkyDRM::CRegistryServiceEntry registryService(m_pInstance, NX_SECURITY_CODE);
		SDWLResult ret = registryService.get_value(HKEY_CURRENT_USER, strKey, strItemName, u32Value);
		if ((0 == ret.GetCode()) && (3 != u32Value))
			return false;
	}

	//HKEY_LOCAL_MACHINE, like : SOFTWARE\Microsoft\Office\Excel\Addins\NxlRMAddin
	{
		std::wstring strKey = L"Software\\Microsoft\\Office\\" + strAppName + L"\\Addins\\NxlRmAddin";
		SkyDRM::CRegistryServiceEntry registryService(m_pInstance, NX_SECURITY_CODE);
		SDWLResult ret = registryService.get_value(HKEY_LOCAL_MACHINE, strKey, strItemName, u32Value);
		if ((0 == ret.GetCode()) && (3 != u32Value))
			return false;
	}

	//HKEY_LOCAL_MACHINE, like : SOFTWARE\Wow6432Node\Microsoft\Office\Excel\Addins\NxlRMAddin
	{
		std::wstring strKey = L"Software\\Wow6432Node\\Microsoft\\Office\\" + strAppName + L"\\Addins\\NxlRmAddin";
		SkyDRM::CRegistryServiceEntry registryService(m_pInstance, NX_SECURITY_CODE);
		SDWLResult ret = registryService.get_value(HKEY_LOCAL_MACHINE, strKey, strItemName, u32Value);
		if ((0 == ret.GetCode()) && (3 != u32Value))
			return false;
	}

	//HKEY_LOCAL_MACHINE, like : SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Microsoft\Office\Excel\Addins\NxlRmAddin
	{
		std::wstring strKey = L"Software\\Microsoft\\Office\\ClickToRun\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Office" + strAppName + L"\\Addins\\NxlRmAddin";
		SkyDRM::CRegistryServiceEntry registryService(m_pInstance, NX_SECURITY_CODE);
		SDWLResult ret = registryService.get_value(HKEY_LOCAL_MACHINE, strKey, strItemName, u32Value);
		if ((0 == ret.GetCode()) && (3 != u32Value))
			return false;
	}

	//HKEY_LOCAL_MACHINE, like : SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Wow6432Node\Microsoft\Office\Excel\Addins\NxlRmAddin
	{
		std::wstring strKey = L"Software\\Microsoft\\Office\\ClickToRun\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Office\\" + strAppName + L"\\Addins\\NxlRmAddin";
		SkyDRM::CRegistryServiceEntry registryService(m_pInstance, NX_SECURITY_CODE);
		SDWLResult ret = registryService.get_value(HKEY_LOCAL_MACHINE, strKey, strItemName, u32Value);
		if ((0 == ret.GetCode()) && (3 != u32Value))
			return false;
	}

	return true;
}

void CNXLFileHandler::CleanUp()
{
	if (m_pInstance)
	{
		SDWLibDeleteRmcInstance(m_pInstance);
		m_pInstance = nullptr;
	}

	CleanUpStringObject();
}

void CNXLFileHandler::CleanUpStringObject()
{
	m_strOriginalNxlFile = L"";
	m_strNxlFileHandlerApp = L"";

	m_strOriginalFileExt = L"";
	m_strRPMNxlFile = L"";
	m_strOriginalExtFileHandlerCmd = L"";
}