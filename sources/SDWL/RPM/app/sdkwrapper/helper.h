#pragma once
#include "stdafx.h"

/*
Notice:
	- Nxl accepts UTF8 as its default coding page, when interfaces get any char-str from client, convert it to utf8
	- Client accpets ANSI as its default coding page, when return any char-str to client, convert it to ansi
*/
namespace helper {

	std::wstring ansi2utf16(const std::string &str)
	{
		if (str.empty())
		{
			return std::wstring();
		}
		int num_chars = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.length(), NULL, 0);
		std::wstring wstrTo;
		if (num_chars)
		{
			wstrTo.resize(num_chars + 1);
			if (MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.length(), &wstrTo[0], num_chars))
			{
				wstrTo = std::wstring(wstrTo.c_str());
				return wstrTo;
			}
		}
		return std::wstring();
	}

	std::wstring utf82utf16(const std::string& str) {
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

	std::string utf162utf8(const std::wstring &wstr) {
		if (wstr.empty()) {
			return std::string();
		}
		int num_chars = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
		std::string to;
		if (num_chars) {
			to.resize(num_chars + 1);
			if (WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &to[0], (int)to.size(), NULL, NULL)) {
				to = std::string(to.c_str());
				return to;
			}
		}
		return std::string();
	}

	

	std::string utf162Ansi(const std::wstring &wstr) {
		if (wstr.empty()) {
			return std::string();
		}
		int num_chars = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
		std::string to;
		if (num_chars) {
			to.resize(num_chars + 1);
			if (WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &to[0], (int)to.size(), NULL, NULL)) {
				to = std::string(to.c_str());
				return to;
			}
		}
		return std::string();

	}

	inline std::string utf82ansi(const std::string& str) {
		return utf162Ansi(utf82utf16(str));
	}

	inline std::string ansi2utf8(const std::string& str) {
		return utf162utf8(ansi2utf16(str));
	}

	inline char* allocStrInComMem(const std::string str) {
		char* buf = NULL;
		const char* p = str.c_str();
		// allco buf and copy p into buf
		buf = (char*)::CoTaskMemAlloc((str.size() + 1) * sizeof(char));
		strcpy_s(buf, str.size() + 1, p);

		return buf;
	}

	inline wchar_t* allocStrInComMem(const std::wstring wstr) {
		wchar_t* buf = NULL;
		const wchar_t* p = wstr.c_str();
		// allco buf and copy p into buf
		buf = (wchar_t*)::CoTaskMemAlloc((wstr.size() + 1) * sizeof(wchar_t));
		wcscpy_s(buf, wstr.size() + 1, p);

		return buf;
	}

	inline unsigned char ToHex(unsigned char x)
	{
		return  x > 9 ? x + 55 : x + 48;
	}

	std::string vectorUint32ToString(const std::vector<uint32_t>& vec) {
		std::string ret = "";
		for (int i = 0; i < vec.size(); i++) {
			if (i == 0) {
				ret += vec[i];
			}
			else {
				ret += ";";
				ret += vec[i];
			}
		}

		return ret;
	}

	std::string UrlEncode(const std::string& str)
	{
		std::string strTemp = "";
		size_t length = str.length();
		for (size_t i = 0; i < length; i++)
		{
			if (isalnum((unsigned char)str[i]) ||
				(str[i] == '-') ||
				(str[i] == '_') ||
				(str[i] == '.') ||
				(str[i] == '~'))
			{
				strTemp += str[i];
			}
			else if (str[i] == ' ')
			{
				strTemp += "+";
			}
			else if (str[i] == '\0')
			{
			}
			else
			{
				strTemp += '%';
				strTemp += ToHex((unsigned char)str[i] >> 4);
				strTemp += ToHex((unsigned char)str[i] % 16);
			}
		}
		return strTemp;
	}

	LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
	{
		strValue = strDefaultValue;
		WCHAR szBuffer[512];
		DWORD dwBufferSize = sizeof(szBuffer);
		ULONG nError;
		nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
		if (ERROR_SUCCESS == nError)
		{
			strValue = szBuffer;
		}
		return nError;
	}

	LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue)
	{
		nValue = nDefaultValue;
		DWORD dwBufferSize(sizeof(DWORD));
		DWORD nResult(0);
		LONG nError = RegQueryValueExW(hKey,
			strValueName.c_str(),
			0,
			NULL,
			reinterpret_cast<LPBYTE>(&nResult),
			&dwBufferSize);
		if (ERROR_SUCCESS == nError)
		{
			nValue = nResult;
		}
		return nError;
	}

	LONG GetBoolRegKey(HKEY hKey, const std::wstring &strValueName, bool &bValue, bool bDefaultValue)
	{
		DWORD nDefValue((bDefaultValue) ? 1 : 0);
		DWORD nResult(nDefValue);
		LONG nError = GetDWORDRegKey(hKey, strValueName.c_str(), nResult, nDefValue);
		if (ERROR_SUCCESS == nError)
		{
			bValue = (nResult != 0) ? true : false;
		}
		return nError;
	}

	inline bool IsRegValueExist(HKEY hKey, const std::wstring &strValue) {

		DWORD dwLoadBehavior = 0;
		DWORD dwSizeValue = sizeof(dwLoadBehavior);
		DWORD dwType = REG_SZ;

		LSTATUS status = ::RegQueryValueExW(hKey, strValue.c_str(),NULL, &dwType, (LPBYTE)&dwLoadBehavior, &dwSizeValue);
		if (ERROR_FILE_NOT_FOUND == status) {
			return false;
		}
		else {
			return true;
		}

	}

	std::string& string_replace(std::string& s, const std::string& from, const std::string& to)
	{
		if (!from.empty())
			for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
				s.replace(pos, from.size(), to);
		return s;
	}

	void SplitStr(const std::string &s, std::vector<std::string> &v, const std::string& c) {
		if (s.empty()) {
			return;
		}
		std::string::size_type pos1, pos2;
		pos1 = 0;
		pos2 = s.find(c);

		while (std::string::npos != pos2)
		{
			v.push_back(s.substr(pos1, pos2 - pos1));

			pos1 = pos2 + c.size();
			pos2 = s.find(c, pos1);
		}

		if (pos1 != s.length()) {
			v.push_back(s.substr(pos1));
		}
	}
}
