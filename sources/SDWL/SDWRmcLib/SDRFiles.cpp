#include "SDRFiles.h"
#include "Network/http_client.h"
#include "Winutil/securefile.h"

#include "rmccore\network\httpClient.h"


using namespace SkyDRM;
using namespace RMCCORE;
using namespace NX::REST::http;


SDRFiles::~SDRFiles()
{
	for (std::list<CSDRmNXLFile>::iterator it = m_nxlFleList.begin(); it != m_nxlFleList.end(); ++it)
	{
		CSDRmNXLFile nxlfile = (CSDRmNXLFile)(*it);
		nxlfile.Close();
	}
}

bool SDRFiles::RemoveFile(ISDRmNXLFile* file)
{
	DWORD err = 0;
	if (!file)
		return false;
	auto it = std::find_if(m_nxlFleList.begin(), m_nxlFleList.end(),
		[file](CSDRmNXLFile& f)->bool { return f.GetFileName().compare(file->GetFileName()) == 0; });

	if (it == m_nxlFleList.end()) {
		return false;
	}
	// remove it
	((CSDRmNXLFile*)file)->Close(); // avoid file sharing
	it->Close();
	std::wstring filename = it->GetFileName();
	m_nxlFleList.erase(it);

	if (m_workFolder.compare(L"") != 0)
	{
		if (!DeleteFile(std::wstring(m_workFolder + L"\\" + filename).c_str())) {
			return false;
		}
	}
	return true;
}


ISDRmNXLFile* SDRFiles::GetFile(size_t index) const
{
	if (index >= m_nxlFleList.size()) {
		return NULL;
	}
	int i = 0;
	try {
		auto iter = std::find_if(m_nxlFleList.cbegin(), m_nxlFleList.cend(),
			[index, &i](const CSDRmNXLFile& f)-> bool { return index == i++; });
		return iter != m_nxlFleList.end() ? (ISDRmNXLFile*)(&(*iter)) : NULL;
	}
	catch (const std::exception& e) {
		std::string err = e.what();
		return NULL;
	}
}

ISDRmNXLFile* SDRFiles::GetFile(const std::wstring& filepath) const
{
	if (m_nxlFleList.empty()) {
		return NULL;
	}
	try {
		std::wstring filename = NX::conv::to_wstring(RMLocalFile(filepath).GetFileName());
		auto iter = std::find_if(m_nxlFleList.cbegin(), m_nxlFleList.cend(),
			[&filename](CSDRmNXLFile f) -> bool { return f.GetFileName().compare(filename) == 0; });
		return iter != m_nxlFleList.end() ? (ISDRmNXLFile*)(&(*iter)) : NULL;
	}
	catch (const std::exception& e) {
		std::string err = e.what();
		return NULL;
	}
}

std::vector<std::wstring> SDRFiles::GetList() const
{
	std::vector<std::wstring> rt;
	for each (CSDRmNXLFile o in m_nxlFleList)
	{
		rt.push_back(NX::conv::to_wstring(o.GetName()));
	}
	return rt;
}

std::vector<std::wstring> SDRFiles::InitFileList()
{
	std::vector<std::wstring> rt;

	// Enumerate working folder to find all *.nxl files
	std::wstring sDir = m_workFolder + L"\\*.nxl";
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile((LPCWSTR)sDir.c_str(), &ffd);
	if (INVALID_HANDLE_VALUE == hFind)
		return rt;

	do
	{
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			rt.push_back((wchar_t*)ffd.cFileName);
			m_nxlFleList.push_back(CSDRmNXLFile(m_workFolder + L"\\" + (wchar_t*)ffd.cFileName));
		}
	} while (FindNextFile(hFind, &ffd) != 0);


	return rt;
}
