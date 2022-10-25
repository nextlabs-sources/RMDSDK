#pragma once

#include "SDRmcInstance.h"
#include "SDLAPI.h"
#include "SDLInstance.h"
#include "nudf\winutil.hpp"

using namespace std;

class CNXLFileHandler
{
public:
	CNXLFileHandler(const wstring& strExeFile);
	~CNXLFileHandler();

public:
	void OpenNxlFile(const wstring& strNxlFile, const std::wstring& arguments);

protected:

	void PrepareOriginalFile(const wstring& strFile);

	bool IsUserLogin();

	void NotifyUserLogin();

    bool PreOpenNormalFile();

    bool PreOpenNxlFile();

	bool QueryFileAssciation();

	bool IsOfficeRmxAddInEnabled();

	void GetRegisterNxlFile(std::map<std::wstring, std::wstring>& mapFile);

	bool GetLastFileTime(const std::wstring& strFile, __time64_t& ftime);

	bool IsFileOpened();

	bool CopyFileToRPMFolder();

	void LaunchAppToViewFile();

	void LaunchCustomerAppToView();

	void LaunchNXAppToView();

	bool LaunchProcess(const wstring& strVerb, 
		const wstring& strParameter);

	bool LaunchProcessEx(const wstring& strVerb, 
		const wstring& strExeFile, 
		const wstring& strParameter);

	bool IsExcelFileExtension(const wstring& strFileExtension);

	bool IsOfficeAppLoadBehaviorEnabled(const std::wstring& strAppName);

	void CleanUp();

	void CleanUpStringObject();

protected:
	ISDRmcInstance*		m_pInstance;
	ISDRmTenant*		m_pTenant;
	ISDRmUser*			m_pSDRmUser;

	wstring				m_strOriginalNxlFile;
	wstring				m_strNxlFileHandlerApp;

	wstring				m_strOriginalFileExt;
	wstring				m_strRPMNxlFile;
	wstring				m_strOriginalExtFileHandlerCmd;
	wstring				m_arguments;
};