#include "stdafx.h"
#include "Winutil/securefile.h"
#include "SDRSecureFile.h"

using namespace SkyDRM;

SDRSecureFile::SDRSecureFile(std::wstring tempfolder)
{
	m_strWorkFolder = tempfolder;
}

SDWLResult SDRSecureFile::FileRead(const std::wstring& file, std::string& s)
{
	try {
		std::wstring path = m_strWorkFolder + L"\\" + file;
		NX::RmSecureFile sf(path, _key);
		return sf.Read(s);
	}
	catch (...) {
		return RESULT2(ERROR_INVALID_DATA, "fail to read data to string");
	}
}

SDWLResult SDRSecureFile::FileWrite(const std::wstring& file, const std::string& s)
{
	try {
		std::wstring path = m_strWorkFolder + L"\\" + file;
		NX::RmSecureFile sf(path, _key);
		return sf.Write((const UCHAR*)s.c_str(), (ULONG)s.length());
	}
	catch (...) {
		return RESULT2(ERROR_INVALID_DATA, "fail to write data to file");
	}
}

SDWLResult SDRSecureFile::FileDelete(const std::wstring& file)
{
	SDWLResult res;

	try {
		std::wstring path = m_strWorkFolder + L"\\" + file;
		if (!DeleteFile(path.c_str()))
		{
			return RESULT2(ERROR_INVALID_DATA, "fail to delete file");
		}
	}
	catch (...) {
		return RESULT2(ERROR_INVALID_DATA, "fail to delete file");
	}

	return res;
}
