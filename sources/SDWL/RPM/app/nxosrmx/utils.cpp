#include "stdafx.h"
#include "utils.h"

#include <psapi.h>
#include <strsafe.h>

//
NXUTILS_NAMESPACE
//

BOOL GetFileNameFromHandle(HANDLE hFile, std::wstring& strFilePath)
{
	const static int BUFSIZE = 512;
	strFilePath.clear();

	BOOL bSuccess = FALSE;
	wchar_t pszFilename[MAX_PATH + 1] = { 0 };
	HANDLE hFileMap;

	// Get the file size.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

	if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
	{
		return FALSE;
	}

	// Create a file mapping object.
	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);
	if (!hFileMap)
	{
		return FALSE;
	}

	// Create a file mapping to get the file name.
	void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
	if (pMem)
	{
		if (GetMappedFileName(GetCurrentProcess(), pMem, pszFilename, MAX_PATH))
		{
			// Translate path with device name to drive letters.
			TCHAR szTemp[BUFSIZE] = { 0 };
			if (GetLogicalDriveStrings(BUFSIZE - 1, szTemp))
			{
				wchar_t szName[MAX_PATH] = { 0 };
				wchar_t szDrive[3] = TEXT(" :");
				BOOL bFound = FALSE;
				wchar_t* p = szTemp;

				do
				{
					// Copy the drive letter to the template string
					*szDrive = *p;

					// Look up each device name
					if (QueryDosDevice(szDrive, szName, MAX_PATH))
					{
						size_t uNameLen = wcslen(szName);
						if (uNameLen < MAX_PATH)
						{
							bFound = _wcsnicmp(pszFilename, szName, uNameLen) == 0 && *(pszFilename + uNameLen) == L'\\';

							if (bFound)
							{
								// Reconstruct pszFilename using szTempFile
								// Replace device path with DOS path
								wchar_t szTempFile[MAX_PATH] = { 0 };
								StringCchPrintf(szTempFile, MAX_PATH, TEXT("%s%s"), szDrive, pszFilename + uNameLen);
								StringCchCopyN(pszFilename, MAX_PATH + 1, szTempFile, wcslen(szTempFile));
							}
						}
					}

					// Go to the next NULL character.
					while (*p++);
				} while (!bFound && *p); // end of string
			}
		}
		bSuccess = TRUE;
		UnmapViewOfFile(pMem);
	}
	CloseHandle(hFileMap);

	strFilePath = pszFilename;
	return bSuccess;
}

std::wstring utf82utf16(const std::string& str) 
{
	if (str.empty())
	{
		return std::wstring();
	}
	int num_chars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
	std::wstring wstrTo;
	if (num_chars)
	{
		wstrTo.resize(num_chars + 1);
		if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstrTo[0], num_chars))
		{
			wstrTo = std::wstring(wstrTo.c_str());
			return wstrTo;
		}
	}
	return std::wstring();
}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool is_sanctuary_file(const std::wstring &wstrPath) {
	bool result = false;

	const std::wstring path = wstrPath + L"\\";
	HKEY hKey = NULL;
	if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\NextLabs\\SkyDRM", 0, KEY_READ | KEY_WOW64_64KEY, &hKey))
	{
		DWORD cbData = 0;
		LPCWSTR name = L"sanctuaryfolder";
		if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, NULL, NULL, &cbData) && 0 != cbData)
		{
			LPBYTE lpData = new BYTE[cbData];
			if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, NULL, lpData, &cbData))
			{
				std::vector<std::wstring> markDir;
				parse_dirs(markDir, (const WCHAR*)lpData);

				for (std::wstring& dir : markDir)
				{
					if (dir.size() == path.size())
					{
						if (path == dir)
						{
							result = TRUE;
							break;
						}
					}
					else if (dir.size() < path.size())
					{
						if (path.substr(0, dir.size()) == dir)
						{
							result = TRUE;
							break;
						}
					}
				}
			}

			delete[]lpData;

		}

		RegCloseKey(hKey);

	}

	return result;
}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR



//
END_NXUTILS_NAMESPACE
//