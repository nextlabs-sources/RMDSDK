#pragma once

#include "SDLInstance.h"
#include "SDLResult.h"
#include "Winutil/keym.h"

#include <string>

namespace SkyDRM 
{
	class SDRSecureFile
	{
	public:
		SDRSecureFile() {};
		SDRSecureFile(std::wstring tempfolder);
		std::wstring GetWorkFolder() { return m_strWorkFolder; };
		void SetKey(const std::vector<UCHAR>& key) { _key = key; };
		SDWLResult FileRead(const std::wstring& file, std::string& s);
		SDWLResult FileWrite(const std::wstring& file, const std::string& s);
		SDWLResult FileDelete(const std::wstring& file);

	private:
		std::wstring m_strWorkFolder;
		std::vector<UCHAR> _key;
	};
}
