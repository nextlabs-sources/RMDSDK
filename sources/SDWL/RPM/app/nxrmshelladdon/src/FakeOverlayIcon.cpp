// FakeOverlayIcon.cpp : Implementation of CFakeOverlayIcon.

#include "stdafx.h"
#include "FakeOverlayIcon.h"
#include "dllmain.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <array>

#include "SDLAPI.h"
#include "nudf/filesys.hpp"

const int kPathLen = 2048;

extern HINSTANCE g_hInstance;
extern ISDRmcInstance* pInstance;
static std::string  g_security = "{6829b159-b9bb-42fc-af19-4a6af3c9fcf6}";

static void parse_dirs(std::vector<std::wstring>& markDir, const WCHAR* str)
{
	std::wstringstream f(str);
	std::wstring s;
	while (std::getline(f, s, L'<'))
	{
		s.pop_back();   // Remove Ext option
		s.pop_back();   // Remove Overwrite option
		s.pop_back();   // Remove AutoProtect option

		if (boost::iends_with(s, L"\\")) {
			markDir.push_back(s);
		}
		else {
			markDir.push_back(s + L"\\");
		}

		// Skip the file tags string.
		std::getline(f, s, L'<');
        // Skip the wsid string.
        std::getline(f, s, L'<');
        // Skip the rms-user-id string.
        std::getline(f, s, L'<');
        // Skip the RPM optioin string.
        std::getline(f, s, L'<');
    }
}

// CFakeOverlayIcon
STDMETHODIMP CFakeOverlayIcon::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	HRESULT nRet = S_FALSE;
	NX::fs::dos_fullfilepath input_path(pwszPath);

	if (!PathIsDirectoryW(input_path.global_dos_path().c_str()))
	{
		std::wstring fileWithNxlPath = input_path.global_dos_path();
		fileWithNxlPath += L".nxl";

		WIN32_FIND_DATAW findFileData = { 0 };
		HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);

			if (!boost::algorithm::iends_with(findFileData.cFileName, L".nxl"))
			{
				nRet = S_OK;
			}
		}
	}
	else
	{
		HKEY hKey = NULL;
		if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\NextLabs\\SkyDRM", 0, KEY_READ | KEY_WOW64_64KEY, &hKey))
		{
			DWORD cbData = 0;

			if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, NULL, &cbData) && 0 != cbData)
			{
				LPBYTE lpData = new BYTE[cbData];
				if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, lpData, &cbData))
				{
					std::wstring wstrPath = input_path.path();
					wstrPath += L"\\";

					std::vector<std::wstring> markDir;
					parse_dirs(markDir, (const WCHAR*)lpData);

					for (std::wstring& dir : markDir)
					{
						if (dir.size() == wstrPath.size())
						{
							if (boost::algorithm::iequals(wstrPath, dir))
							{
								nRet = S_OK;
								break;
							}
						}
						else if (dir.size() < wstrPath.size())
						{
							if (boost::algorithm::istarts_with(wstrPath, dir))
							{
								nRet = S_OK;
								break;
							}
						}
					}
				}

				delete[]lpData;
			}

			RegCloseKey(hKey);
		}
	}

	return nRet;
}

STDMETHODIMP CFakeOverlayIcon::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int * pIndex, DWORD * pdwFlags)
{
	HRESULT nRet = S_OK;

	if (!GetModuleFileNameW(g_hInstance,
		pwszIconFile,
		cchMax))
	{
		nRet = S_FALSE;
	}
	else
	{
		*pIndex = 0;
		*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
	}

	return nRet;
}

STDMETHODIMP CFakeOverlayIcon::GetPriority(int * pIPriority)
{
	*pIPriority = 0;
	return S_OK;
}

std::vector<std::wstring> query_selected_file(IDataObject *pdtobj)
{
	std::vector<std::wstring> files;
	FORMATETC   FmtEtc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM   Stg = { 0 };
	HDROP       hDrop = NULL;

	memset(&Stg, 0, sizeof(Stg));
	Stg.tymed = CF_HDROP;

	// Find CF_HDROP data in pDataObj
	if (FAILED(pdtobj->GetData(&FmtEtc, &Stg))) {
		return files;
	}

	// Get the pointer pointing to real data
	hDrop = (HDROP)GlobalLock(Stg.hGlobal);
	if (NULL == hDrop) {
		ReleaseStgMedium(&Stg);
		return files;
	}

	// How many files are selected?
	const int nFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
	if (0 == nFiles) {
		return files;
	}

	for (int i = 0; i < nFiles; i++) {
		wchar_t s[MAX_PATH];
		if (0 != DragQueryFileW(hDrop, i, s, MAX_PATH)) {
			//push all files in MenuFilter will be checked later
			files.push_back(s);
		}
	}

	GlobalUnlock(Stg.hGlobal);
	ReleaseStgMedium(&Stg);

	return files;
}

bool isDir(const std::wstring& path)
{
	DWORD dwAttrs = GetFileAttributesW(NX::fs::dos_fullfilepath(path).global_dos_path().c_str());
	if (dwAttrs != INVALID_FILE_ATTRIBUTES) {
		if (FILE_ATTRIBUTE_DIRECTORY & dwAttrs) {
			return true;
		}
	}

	return false;
}

bool isContainDir(const std::vector<std::wstring>& selectedFiles)
{
	bool bRet = false;
	for (auto path : selectedFiles)
	{
		if (isDir(path)) {
			bRet = true;
			break;
		}
	}

	return bRet;
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool isContainSanctuaryDir(const std::vector<std::wstring>& selectedFiles)
{
	bool bRet = false;
	for (auto path : selectedFiles)
	{
		if (isDir(path))
		{
			uint32_t dirstatus = 0;
			SDWLResult res;
			std::wstring tempTags;
			res = pInstance->IsSanctuaryFolder(path, &dirstatus, tempTags);
			if (res.GetCode() == 0)
			{
				if ((dirstatus & RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR)
					|| (dirstatus & RPM_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR)
					|| (dirstatus & RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR))
				{
					bRet = true;
					break;
				}
			}
		}
	}

	return bRet;
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

bool isOfficeFormatFile(const std::wstring& ext)
{
	bool bRet = false;
	static std::array<std::wstring, 21> arrOffice =
	{
		L".doc", L".docx", L".dot", L".dotx", L"docm", L"dotm", L"rtf"
		L".xls", L".xlsx", L".xlt", L".xltx", L"xlsm", L"xltm", L"xlsb",
		L".ppt", L".pptx", L".ppsx", L".potx", L"pot", L"potm", L"pptm"
	};

	for (auto item : arrOffice)
	{
		if (boost::algorithm::iends_with(ext, item)) {
			bRet = true;
			break;
		}
	}

	return bRet;
}

std::wstring regGetValue(const std::wstring &strSubKey, const std::wstring &strValueName, DWORD dwType, HKEY hRoot)
{
	HKEY hKey = NULL;
	std::wstring strValue;
	LSTATUS lStatus = ::RegOpenKeyExW(hRoot, strSubKey.c_str(), 0, KEY_QUERY_VALUE, &hKey); //| KEY_WOW64_64KEY
	if (lStatus == ERROR_SUCCESS)
	{
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

bool HideNewItemForOffice(const std::wstring& doctype, bool bhide)
{
	std::wstring strDoc = regGetValue(L"SOFTWARE\\Classes\\" + doctype, L"", REG_SZ, HKEY_LOCAL_MACHINE);

	std::wstring strSubkey = L"SOFTWARE\\Classes\\" + strDoc + L"\\shell\\New";

	SDWLResult rt;
	if (bhide)
	{
		rt = pInstance->RPMSetAppRegistry(strSubkey, L"LegacyDisable", L"", 0, g_security); // 0 - set value

	}
	else {
		rt = pInstance->RPMSetAppRegistry(strSubkey, L"LegacyDisable", L"", 1, g_security); // 1 - delete value
	}

	return rt;
}

void DisableSendToItem(HMENU hmenu, bool isDisable)
{
	// Disable "Send to" item if file is under the sanctuary dir.
	int count = GetMenuItemCount(hmenu);
	for (int i = 0; i < count; i++)
	{
		MENUITEMINFO mii;
		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_FTYPE | MIIM_STRING;
		mii.dwTypeData = NULL;
		BOOL ok = GetMenuItemInfo(hmenu, i, TRUE, &mii); // get the string length
		if (mii.fType != MFT_STRING)
			continue;
		UINT size = (mii.cch + 1) * 2;
		LPWSTR menuTitle = (LPWSTR)malloc(size);
		mii.cch = size;
		mii.fMask = MIIM_TYPE;
		mii.dwTypeData = menuTitle;
		ok = GetMenuItemInfo(hmenu, i, TRUE, &mii); // Get the actual string data.

		if (wcscmp(menuTitle, L"Se&nd to") == 0)
		{
			if (isDisable)
				EnableMenuItem(hmenu, i, MF_BYPOSITION | MF_GRAYED | MF_DISABLED);
			else
				EnableMenuItem(hmenu, i, MF_BYPOSITION | MF_ENABLED);
		}

		free(menuTitle);
	}
}

bool NeedDisableSendTo(const std::vector<std::wstring>& selectedFiles)
{
	bool bRet = false;

	if (selectedFiles.empty()) {
		return bRet;
	}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	if (isContainDir(selectedFiles))
	{
		if (isContainSanctuaryDir(selectedFiles)) {
			bRet = true;
		}
		else {
			bRet = false;
		}
	}
	else // All documents
	{
		auto oneFile = selectedFiles.front();
		auto parentDir = oneFile.erase(oneFile.rfind(L'\\'));
		RPMFolderRelation desFolderRpmRelation = GetFolderRelation(parentDir);
		if (desFolderRpmRelation.bSanctuaryFolder
			|| desFolderRpmRelation.bSanctuaryInheritedFolder)
		{
			bRet = true;
		}
	}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR


	return bRet;
}

STDMETHODIMP CFakeOverlayIcon::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
	if (!g_otherProcess)
	{
		vecSelectedFiles = query_selected_file(pdtobj);
		if (isContainDir(vecSelectedFiles))
		{
			return S_OK;
		}

		// Below is handle Office "New" item
		std::wstring ext(L"");
		auto strSelectedFile = vecSelectedFiles.front();
		std::wstring wstrTmp(strSelectedFile);
		auto index = strSelectedFile.rfind(L'.');
		if (index != std::wstring::npos)
		{
			ext = strSelectedFile.substr(index);
		}

		// Record folder path to handle "Send to" menu item.
		strSelectedFile.erase(strSelectedFile.rfind(L'\\'));

		if (!isOfficeFormatFile(ext)) {
			return S_OK;
		}

		// If the office file is under RPM folder, also need to hide "New" item
		unsigned int dirStatus = 0;
		bool fileStatus = false;
		auto res = pInstance->RPMGetFileStatus(wstrTmp, &dirStatus, &fileStatus);
		bool bHideNewItemForRPMFolder = false;
		if (res.GetCode() == 0)
		{
			if (dirStatus & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR))
			{
				// If is nxl file
				if (fileStatus) {
					// Hide "New" item for office file.
					bHideNewItemForRPMFolder = HideNewItemForOffice(ext, true);
				}
				else {
					HideNewItemForOffice(ext, false);
				}
			}
			else
			{
				// Restore
				HideNewItemForOffice(ext, false);
			}
		}

		if (bHideNewItemForRPMFolder)
		{
			return S_OK;
		}


#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		// If is sancDir
		RPMFolderRelation desFolderRpmRelation = GetFolderRelation(strSelectedFile);
		if (desFolderRpmRelation.bSanctuaryFolder || desFolderRpmRelation.bSanctuaryInheritedFolder)
		{
			// Hide "New" item for office file.
			HideNewItemForOffice(ext, true);
		}
		else
		{
			// Restore
			HideNewItemForOffice(ext, false);
		}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

		return S_OK;
	}

	// Fix bug 60176
	if ((IDataObject*)10 >= pdtobj)
	{
		return S_OK;
	}

	FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stg = { TYMED_HGLOBAL };

	if (FAILED(pdtobj->GetData(&etc, &stg)))
	{
		return S_OK;
	}

	HDROP hdrop = (HDROP)GlobalLock(stg.hGlobal);
	if (NULL == hdrop)
	{
		ReleaseStgMedium(&stg);
		return S_OK;
	}

	UINT uNumFiles = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);

	for (UINT uFile = 0; uFile < uNumFiles; uFile++)
	{
		WCHAR szFile[kPathLen] = { 0 };

		// temporary amend solution
		// 
		// Amend the Common dialog right click menu hiden issue, and it does not work sometimes when nxosdlp.dll is introduced,
		// Because "DragQueryFileW" is hooked in nxosdlp.dll and blocked nxl file and sanctuary file.
		// So here when the function call failed(return value is 0), we probably can think this judge has been processed by nxosdlp module,
		// and directly set the variable "bNeedBlock" as TRUE.
		if (0 == DragQueryFileW(hdrop, uFile, szFile, kPathLen))
		{
			bNeedBlock = TRUE;
			break;
		}

		if (!PathIsDirectoryW(szFile))
		{
			std::wstring fileWithNxlPath = szFile;
			fileWithNxlPath += L".nxl";

			WIN32_FIND_DATAW findFileData = { 0 };
			HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				FindClose(hFind);

				if (!boost::algorithm::iends_with(findFileData.cFileName, L".nxl"))
				{
					bNeedBlock = TRUE;
					break;
				}
			}
		}
		else
		{
			// When user right click one RPM folder or its Ancestor\Descendant folder in common dialog, also block the menu.
			HKEY hKey = NULL;
			if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\NextLabs\\SkyDRM", 0, KEY_READ | KEY_WOW64_64KEY, &hKey))
			{
				DWORD cbData = 0;

				if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, NULL, &cbData) && 0 != cbData)
				{
					LPBYTE lpData = new BYTE[cbData];
					if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, lpData, &cbData))
					{
						std::wstring wstrPath = szFile;
						wstrPath += L"\\";
						std::transform(wstrPath.begin(), wstrPath.end(), wstrPath.begin(), tolower);

						std::vector<std::wstring> markDir;
						parse_dirs(markDir, (const WCHAR*)lpData);

						for (std::wstring& dir : markDir)
						{
							if (dir.size() == wstrPath.size())
							{
								if (boost::algorithm::iequals(wstrPath, dir))
								{
									bNeedBlock = TRUE;
									break;
								}
							}
							else if (dir.size() < wstrPath.size())
							{
								if (boost::algorithm::istarts_with(wstrPath, dir))
								{
									bNeedBlock = TRUE;
									break;
								}
							}
							else
							{
								if (boost::algorithm::istarts_with(dir, wstrPath))
								{
									bNeedBlock = TRUE;
									break;
								}
							}
						}
					}

					delete[]lpData;
				}

				RegCloseKey(hKey);
			}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			// If is sancDir
			RPMFolderRelation desFolderRpmRelation = GetFolderRelation(szFile);
			if (desFolderRpmRelation.bSanctuaryAncestralFolder ||
				desFolderRpmRelation.bSanctuaryFolder ||
				desFolderRpmRelation.bSanctuaryInheritedFolder)
			{
				bNeedBlock = TRUE;
				break;
			}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		}
	}

	GlobalUnlock(stg.hGlobal);
	ReleaseStgMedium(&stg);

	return S_OK;
}

STDMETHODIMP CFakeOverlayIcon::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	if (!bNeedBlock)
	{
#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		if (NeedDisableSendTo(vecSelectedFiles))
		{
			DisableSendToItem(hmenu, true);
		}
		else
		{
			DisableSendToItem(hmenu, false);
		}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

		return S_OK;
	}

	if (hmenu != NULL)
	{
		DestroyMenu(hmenu);
	}

	return S_FALSE;
}

HRESULT CFakeOverlayIcon::GetCommandString(_In_  UINT_PTR idCmd, _In_  UINT uType, UINT *pReserved, _Out_cap_(cchMax)  LPSTR pszName, _In_  UINT cchMax)
{
	return S_OK;
}

STDMETHODIMP CFakeOverlayIcon::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
	return S_OK;
}
