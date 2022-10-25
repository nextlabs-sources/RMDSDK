// nxrmhandler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "nxlfilehandler.h"
#include <experimental/filesystem>

using namespace std;


bool RegistrySetValue(HKEY hRoot,
	const std::wstring& strKey,
	const std::wstring& strItemName,
	const std::wstring& strItemValue)
{
	bool bRet = false;
	try
	{
		NX::win::reg_key key;
		if (!key.exist(hRoot, strKey))
		{
			key.create(hRoot, strKey, NX::win::reg_key::reg_position::reg_default);
			if (!strItemName.empty() || !strItemValue.empty())
			{
				key.set_value(strItemName, strItemValue, true);
			}
		}
		else
		{
			key.open(hRoot, strKey, NX::win::reg_key::reg_position::reg_default, false);
			if (!strItemName.empty() || !strItemValue.empty())
			{
				key.set_value(strItemName, strItemValue, true);
			}
		}
	}
	catch (const std::exception& e)
	{
		UNREFERENCED_PARAMETER(e);
		DWORD dwErr = ::GetLastError();
		return false;
	}
	return true;
}

void RegistryDeleteKey(HKEY hRoot,
	const std::wstring& strKey)
{
	try
	{
		NX::win::reg_key key;
		if (key.exist(hRoot, strKey))
		{
			key.remove(hRoot, strKey);
		}
	}
	catch (const std::exception& e)
	{
		UNREFERENCED_PARAMETER(e);
		DWORD dwErr = ::GetLastError();
		return;
	}
}

std::wstring trim(const std::wstring& str)
{
	std::wstring::size_type pos = str.find_first_not_of(L' ');
	if (pos == string::npos)
	{
		return str;
	}

	std::wstring::size_type pos2 = str.find_last_not_of(L' ');
	if (pos2 != string::npos)
	{
		return str.substr(pos, pos2 - pos + 1);
	}
	return str.substr(pos);
}

const std::wstring NX_REG_SOFTWARE_CLASS = L"SOFTWARE\\Classes\\";
const std::wstring NX_REG_FILE_FILTER = L".nxl";
const std::wstring NX_REG_FILE_HANDLER = L"NextLabs.Handler.1";

void RegisterFileAssociation(const std::wstring& strModulePath)
{
	std::wstring strKeyExt = NX_REG_SOFTWARE_CLASS + NX_REG_FILE_FILTER;
	RegistrySetValue(HKEY_LOCAL_MACHINE, strKeyExt, L"", NX_REG_FILE_HANDLER);

	std::wstring strKeyHandler = NX_REG_SOFTWARE_CLASS + NX_REG_FILE_HANDLER;

	std::wstring strKeyDefaultIcon = strKeyHandler + L"\\DefaultIcon";
	std::wstring strIcon = L"\"" + strModulePath + L"\"" + L", 0";
	RegistrySetValue(HKEY_LOCAL_MACHINE, strKeyDefaultIcon, L"", strIcon);

	std::wstring strKeyOpen = strKeyHandler + L"\\shell\\open\\command";
	std::wstring strOpenValue = L"\"" + strModulePath + L"\"" + L" \"%1\"";
	RegistrySetValue(HKEY_LOCAL_MACHINE, strKeyOpen, L"", strOpenValue);
}

void UnRegisterFileAssociation()
{
	std::wstring strKeyExt = NX_REG_SOFTWARE_CLASS + NX_REG_FILE_FILTER;
	RegistryDeleteKey(HKEY_LOCAL_MACHINE, strKeyExt);

	std::wstring strKeyHandler = NX_REG_SOFTWARE_CLASS + NX_REG_FILE_HANDLER;

	std::wstring strKeyCommand = strKeyHandler + L"\\shell\\open\\command";
	RegistryDeleteKey(HKEY_LOCAL_MACHINE, strKeyCommand);

	std::wstring strKeyOpen = strKeyHandler + L"\\shell\\open";
	RegistryDeleteKey(HKEY_LOCAL_MACHINE, strKeyOpen);

	std::wstring strKeyShell = strKeyHandler + L"\\shell";
	RegistryDeleteKey(HKEY_LOCAL_MACHINE, strKeyShell);

	std::wstring strKeyDefaultIcon = strKeyHandler + L"\\DefaultIcon";
	RegistryDeleteKey(HKEY_LOCAL_MACHINE, strKeyDefaultIcon);

	RegistryDeleteKey(HKEY_LOCAL_MACHINE, strKeyHandler);
}

void OpenNxlFile(const std::wstring& strNxlFile,const std::wstring& arguments , const std::wstring strModuleFile)
{
	wstring strFile = NX::fs::dos_fullfilepath(strNxlFile).path();
	size_t pos = strFile.rfind('.');
	if (std::wstring::npos == pos)
		return;

	wstring strExtension = strFile.substr(pos);
	std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), ::towlower);
	if (0 != strExtension.compare(L".exe"))
	{
		CNXLFileHandler nxlFileHandler(strModuleFile);
		nxlFileHandler.OpenNxlFile(strNxlFile, arguments);
	}
}


std::wstring to_unlimitedpath2(const std::wstring& filepath)
{
	std::experimental::filesystem::path tfilepath(filepath);
	std::wstring unlimitedpath = tfilepath;

	long     length = 0;
	TCHAR*   buffer = NULL;
	// First obtain the size needed by passing NULL and 0.
	length = GetLongPathName(tfilepath.c_str(), NULL, 0);
	if (length > (long)(unlimitedpath.size() + 1))
	{
		buffer = new TCHAR[length];
		length = GetLongPathName(tfilepath.c_str(), buffer, length);
		if (length > 0)
			unlimitedpath = buffer;
		delete[] buffer;
	}
	else if (length == 0)
	{
		DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND)
		{
			// we need to seperate the file name and folder
			std::wstring _parentfolder = tfilepath.parent_path();
			length = GetLongPathName(_parentfolder.c_str(), NULL, 0);
			if (length > (long)(_parentfolder.size() + 1))
			{
				buffer = new TCHAR[length];
				length = GetLongPathName(_parentfolder.c_str(), buffer, length);
				if (length > 0)
					unlimitedpath = (std::wstring)buffer + L"\\" + (std::wstring)(tfilepath.filename());
				delete[] buffer;
			}
		}
	}

	if (unlimitedpath.length() < (MAX_PATH - 10)) // temp 
		return unlimitedpath;

	if (unlimitedpath.find(L"\\\\?") == 0)
		return unlimitedpath;

	if (unlimitedpath.find(L"\\\\") == 0)
	{
		unlimitedpath.erase(0, 1);
		unlimitedpath = L"\\\\?\\UNC" + unlimitedpath;
		return unlimitedpath;
	}

	unlimitedpath = L"\\\\?\\" + unlimitedpath;

	return unlimitedpath;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//::MessageBox(NULL, L"Text", L"Caption", MB_OK);

	::OutputDebugStringW(L"Enter nxrmhandler \n");
	::OutputDebugStringW(L"============================================================\n");
	::OutputDebugStringW(L"=======================Command Line=========================\n");

	int nArgc = 0;
	LPWSTR *szArgList = nullptr;
	LPWSTR commandLine = ::GetCommandLineW();
	szArgList = ::CommandLineToArgvW(commandLine, &nArgc);
	for (int i = 0; i < nArgc; i++)
	{
		::OutputDebugStringW(szArgList[i]);
	}
	::OutputDebugStringW(L"============================================================\n");

	WCHAR szFile[MAX_PATH] = { 0 };
	GetModuleFileNameW(0, szFile, MAX_PATH);
	wstring strModuleFile(szFile);

	if (1 == nArgc)
	{
		wstring strFilePath = NX::fs::dos_fullfilepath(std::wstring(szArgList[0])).path();
		OpenNxlFile(strFilePath, L"", strModuleFile);
	}
	else if (2 == nArgc)
	{
		std::wstring strCmd = trim(szArgList[1]);
		std::transform(strCmd.begin(), strCmd.end(), strCmd.begin(), ::towlower);
		if (0 == strCmd.compare(L"-i"))
		{
			RegisterFileAssociation(strModuleFile);
		}
		else if (0 == strCmd.compare(L"-u"))
		{
			UnRegisterFileAssociation();
		}
		else
		{
			wstring strFilePath = NX::fs::dos_fullfilepath(std::wstring(szArgList[1])).path();
			OpenNxlFile(strFilePath, L"", strModuleFile);
		}
	}
	else if (3 == nArgc)
	{
		wstring strFilePath = NX::fs::dos_fullfilepath(std::wstring(szArgList[1])).path();
		OpenNxlFile(strFilePath, szArgList[2], strModuleFile);
	}

	::OutputDebugStringW(L"Leave nxrmhandler \n");
	return 0;
}
