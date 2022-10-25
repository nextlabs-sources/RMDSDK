
#include "..\stdafx.h"
#include <Wincrypt.h>
#include <cctype>
#include "..\Common\stringex.h"
#include <experimental/filesystem>
#include <sddl.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "nlohmann/json.hpp"
#include "nudf/filesys.hpp"

using namespace NX::conv;

std::string NX::conv::utf16toutf8(const std::wstring& ws)
{
    std::string s;
    if (!ws.empty()) {
        const int reserved_size = (int)(ws.length() * 3 + 1);
        if (0 == WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.length(), NX::string_buffer(s, reserved_size), (int)reserved_size, nullptr, nullptr)) {
            s.clear();
        }
    }
    return s;
}

std::wstring NX::conv::utf8toutf16(const std::string& s)
{
    std::wstring ws;
    if (!s.empty()) {
        const int reserved_size = (int)s.length();
        if (0 == MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), NX::wstring_buffer(ws, reserved_size), (int)reserved_size)) {
            ws.clear();
        }
    }
    return ws;
}

std::wstring NX::conv::ansitoutf16(const std::string& s)
{
    std::wstring ws;
    if (!s.empty()) {
        const int reserved_size = (int)s.length();
        if (0 == MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.length(), NX::wstring_buffer(ws, reserved_size), (int)reserved_size)) {
            ws.clear();
        }
    }
    return ws;
}

std::wstring NX::conv::to_wstring(const std::wstring& s)
{
    return s;
}

std::wstring NX::conv::to_wstring(const std::string& s)
{
    return utf8toutf16(s);
}

std::string NX::conv::to_string(const std::wstring& s)
{
    return utf16toutf8(s);
}

std::string NX::conv::to_string(const std::string& s)
{
    return s;
}

std::string NX::conv::remove_extension(const std::string& filename) {
	size_t lastdot = filename.find_last_of(".");
	if (lastdot == std::string::npos) return filename;
	return filename.substr(0, lastdot);
}

std::wstring NX::conv::remove_extension(const std::wstring& filename) {
	size_t lastdot = filename.find_last_of(L".");
	if (lastdot == std::wstring::npos) return filename;
	return filename.substr(0, lastdot);
}

std::string NX::conv::string_replace(std::string src, std::string const& target, std::string const& repl)
{
	// handle error situations/trivial cases

	if (target.length() == 0) {
		// searching for a match to the empty string will result in 
		//  an infinite loop
		//  it might make sense to throw an exception for this case
		return src;
	}

	if (src.length() == 0) {
		return src;  // nothing to match against
	}

	size_t idx = 0;

	for (;;) {
		idx = src.find(target, idx);
		if (idx == std::string::npos)  break;

		src.replace(idx, target.length(), repl);
		idx += repl.length();
	}

	return src;
}

std::wstring NX::conv::wstring_replace(std::wstring src, std::wstring const& target, std::wstring const& repl)
{
	// handle error situations/trivial cases

	if (target.length() == 0) {
		// searching for a match to the empty string will result in 
		//  an infinite loop
		//  it might make sense to throw an exception for this case
		return src;
	}

	if (src.length() == 0) {
		return src;  // nothing to match against
	}

	size_t idx = 0;

	for (;;) {
		idx = src.find(target, idx);
		if (idx == std::wstring::npos)  break;

		src.replace(idx, target.length(), repl);
		idx += repl.length();
	}

	return src;
}

void NX::conv::split_string(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	if (s.empty())
	{
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

	if (pos1 != s.length())
	{
		v.push_back(s.substr(pos1));
	}
}

std::vector<unsigned char> NX::conv::Base64Decode(const std::string& base64)
{
	std::vector<unsigned char> buf;
	unsigned long out_size = 0;
	CryptStringToBinaryA(base64.data(), (unsigned long)base64.length(), CRYPT_STRING_ANY, NULL, &out_size, NULL, NULL);
	buf.resize(out_size, 0);
	if (!CryptStringToBinaryA(base64.data(), (unsigned long)base64.length(), CRYPT_STRING_ANY, buf.data(), &out_size, NULL, NULL)) {
		buf.clear();
	}
	return buf;
}

std::vector<unsigned char> NX::conv::Base64Decode(const std::wstring& base64)
{
	std::string s = NX::conv::utf16toutf8(base64);
	return Base64Decode(s);
}

std::string NX::conv::Base64Encode(const std::vector<unsigned char>& data)
{
	return Base64Encode(data.data(), (unsigned long)data.size());
}

std::string NX::conv::Base64Encode(const unsigned char* data, unsigned long size)
{
	unsigned long out_size = 0;
	CryptBinaryToStringA(data, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &out_size);
	std::vector<char> buf;
	buf.resize(out_size + 1, 0);
	if (!CryptBinaryToStringA(data, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf.data(), &out_size)) {
		buf.clear();
		return std::string();
	}
	return std::string(buf.data());
}

std::string NX::conv::UrlEncode(const std::string& url, bool is_x_www_form)
{
	std::string s;
	std::for_each(url.begin(), url.end(), [&](const char& c) {
		if (isalnum(c) || c == '-' || c == '_' || c == '~' || (is_x_www_form && (c == '=' || c == '&' || c == '.'))) {
			s.push_back(c);
		}
		else {
			const std::string& hexs = utohs<char>((unsigned char)c);
			s.push_back('%');
			s.append(hexs);
		}
	});
	return s;
}

std::string NX::conv::UrlDecode(const std::string& url)
{
	std::string s;
	const char* p = url.c_str();

	while (*p) {
		if ('%' == *p) {
			++p;
			if (*p && *(p + 1)) {
				if (!ishex(*p) || !ishex(*(p + 1)))
					break;
				char c = ctoi<char>(*(p++));
				c <<= 4;
				c |= (char)ctoi<char>(*(p++));
				s.push_back(c);
			}
			else {
				break;
			}
		}
		else {
			s.push_back(*(p++));
		}
	}

	return s;
}

std::string NX::FormatString(const char* format, ...)
{
	va_list args;
	int     len = 0;
	std::string s;

	va_start(args, format);
	len = _vscprintf_l(format, 0, args) + 1;
	vsprintf_s(string_buffer(s, len), len, format, args);
	va_end(args);

	return s;
}

std::wstring NX::FormatString(const wchar_t* format, ...)
{
	va_list args;
	int     len = 0;
	std::wstring s;

	va_start(args, format);
	len = _vscwprintf_l(format, 0, args) + 1;
	vswprintf_s(wstring_buffer(s, len), len, format, args);
	va_end(args);

	return s;
}

bool NX::conv::string_icompare(const std::string &str1, const std::string &str2)
{
	std::string _str1 = str1;
	std::string _str2 = str2;

	return ((_str1.size() == _str2.size()) && std::equal(_str1.begin(), _str1.end(), _str2.begin(), [](char & c1, char & c2) {
		return (c1 == c2 || std::toupper(c1) == std::toupper(c2));
	}));
}

bool NX::conv::wstring_icompare(const std::wstring &str1, const std::wstring &str2)
{
    std::string _str1 = NX::conv::utf16toutf8(str1);
    std::string _str2 = NX::conv::utf16toutf8(str2);;

    return ((_str1.size() == _str2.size()) && std::equal(_str1.begin(), _str1.end(), _str2.begin(), [](char & c1, char & c2) {
        return (c1 == c2 || std::toupper(c1) == std::toupper(c2));
        }));
}

std::string NX::conv::string_ireplace(const std::string &sourceString, const std::string &searchString, const std::string &replaceString) {
	std::string source = sourceString;
	std::string lower = source;
	transform(source.begin(), source.end(), lower.begin(), ::tolower);
	auto pos = lower.find(searchString);
	while (pos != std::string::npos) {
		source.replace(pos, searchString.size(), replaceString);
		lower = source;
		transform(source.begin(), source.end(), lower.begin(), ::tolower);
		pos = lower.find(searchString);
	}

	return source;
}

void NX::conv::GetUserInfo(LPWSTR wzSid, int nSize, LPWSTR UserName, int UserNameLen)
{
	HANDLE hTokenHandle = 0;

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hTokenHandle))
	{
		if (GetLastError() == ERROR_NO_TOKEN)
		{
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTokenHandle))
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// Get SID
	UCHAR InfoBuffer[512];
	DWORD cbInfoBuffer = 512;
	LPTSTR StringSid = 0;
	WCHAR   uname[64] = { 0 }; DWORD unamelen = 63;
	WCHAR   dname[64] = { 0 }; DWORD dnamelen = 63;
	WCHAR   fqdnname[MAX_PATH + 1]; memset(fqdnname, 0, sizeof(fqdnname));
	SID_NAME_USE snu;
	if (!GetTokenInformation(hTokenHandle, TokenUser, InfoBuffer, cbInfoBuffer, &cbInfoBuffer))
		return;
	if (ConvertSidToStringSid(((PTOKEN_USER)InfoBuffer)->User.Sid, &StringSid))
	{
		_tcsncpy_s(wzSid, nSize, StringSid, _TRUNCATE);
		if (StringSid) LocalFree(StringSid);
	}
	if (LookupAccountSid(NULL, ((PTOKEN_USER)InfoBuffer)->User.Sid, uname, &unamelen, dname, &dnamelen, &snu))
	{
		wcsncat_s(UserName, UserNameLen, uname, _TRUNCATE);
	}
}

std::vector<std::pair<std::wstring, std::wstring>> NX::conv::ImportJsonTags(const std::string &s)
{
    std::vector<std::pair<std::wstring, std::wstring>> attrs;
    try {
        nlohmann::json root = nlohmann::json::parse(s);
        if (!root.is_object())
        {
            return attrs;
        }

        for (auto& element : root.items())
        {
            std::wstring key = conv::utf8toutf16(element.key());
            std::vector<std::string> vecValues = element.value().get<std::vector<std::string>>();
            for (auto it = vecValues.begin(); vecValues.end() != it; it++)
            {
                std::wstring strValue = conv::utf8toutf16(*it);
                attrs.push_back(std::pair<std::wstring, std::wstring>(key, strValue));
            }
        }
    }
    catch (...)
    {
        attrs.clear();
    }

    return attrs;
}