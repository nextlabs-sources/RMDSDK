

#ifndef __NX_COMMON_STRINGEX_H__
#define __NX_COMMON_STRINGEX_H__

#include <Windows.h>
#include "string.h"

#include <string>

namespace NX {

    namespace conv {

        std::string utf16toutf8(const std::wstring& ws);
        std::wstring utf8toutf16(const std::string& s);
        std::wstring ansitoutf16(const std::string& s);
        std::wstring to_wstring(const std::wstring& s);
        std::wstring to_wstring(const std::string& s);
        std::string to_string(const std::wstring& s);
        std::string to_string(const std::string& s);
		std::string string_replace(std::string src, std::string const& target, std::string const& repl);
		std::wstring wstring_replace(std::wstring src, std::wstring const& target, std::wstring const& repl);
		bool string_icompare(const std::string &str1, const std::string &str2);
        bool wstring_icompare(const std::wstring &str1, const std::wstring &str2);
        std::string string_ireplace(const std::string &sourceString, const std::string &searchString, const std::string &replaceString);
		void GetUserInfo(LPWSTR wzSid, int nSize, LPWSTR UserName, int UserNameLen);

        std::vector<std::pair<std::wstring, std::wstring>> ImportJsonTags(const std::string& s);

		std::vector<unsigned char> Base64Decode(const std::string& base64);
		std::vector<unsigned char> Base64Decode(const std::wstring& base64);
		std::string Base64Encode(const std::vector<unsigned char>& data);
		std::string Base64Encode(const unsigned char* data, unsigned long size);

		std::string UrlEncode(const std::string& url, bool is_x_www_form = true);
		std::string UrlDecode(const std::string& url);
		std::string remove_extension(const std::string& filename);
		std::wstring remove_extension(const std::wstring& filename);

		void split_string(const std::string& s, std::vector<std::string>& v, const std::string& c);

        template <typename T>
        std::basic_string<T> to_string(int n)
        {
            std::string s;
            sprintf_s(string_buffer(s, 128), "%d", n);
            return std::basic_string<T>(s.begin(), s.end());
        }

        template <typename T>
        std::basic_string<T> to_string(__int64 n)
        {
            std::string s;
            sprintf_s(string_buffer(s, 128), "%I64d", n);
            return std::basic_string<T>(s.begin(), s.end());
        }

        template <typename T>
        std::basic_string<T> to_string(double d)
        {
            std::string s;
            sprintf_s(string_buffer(s, 128), "%f", d);
            return std::basic_string<T>(s.begin(), s.end());
        }
    }	// NX::conv


	std::string FormatString(const char* format, ...);
	std::wstring FormatString(const wchar_t* format, ...);
	

    template <typename T>
    std::basic_string<T> escape(const std::basic_string<T>& s)
    {
        std::basic_string<T> es;
        const T* p = s.c_str();
        while (*p) {
            switch (*p)
            {
            case T('\''):
                es.push_back((T)'\\');
                es.push_back((T)'\'');
                break;
            case T('"'):
                es.push_back((T)'\\');
                es.push_back((T)'"');
                break;
            case T('\\'):
                es.push_back((T)'\\');
                es.push_back((T)'\\');
                break;
            case T('\n'):
                es.push_back((T)'\\');
                es.push_back((T)'n');
                break;
            case T('\r'):
                es.push_back((T)'\\');
                es.push_back((T)'r');
                break;
            case T('\t'):
                es.push_back((T)'\\');
                es.push_back((T)'t');
                break;
            case T('\b'):
                es.push_back((T)'\\');
                es.push_back((T)'b');
                break;
            case T('\v'):
            case T('\0'):
                // Not allowed
                break;
            default:
                es.push_back(*p);
                break;
            }
            ++p;
        }
        return es;
    }

    template <typename T>
    bool unescape(const std::basic_string<T>& es, std::basic_string<T>& s, int* errpos)
    {
        std::basic_string<T> s;
        const T* p = s.begin();

        if (errpos) *errpos = 0;

        while (*p)
        {
            if ((T)'\\' != *p) {
                s.push_back(*p);
            }
            else {
                ++p;
                if (0 == *p)
                    goto _end;

                if (*p == (T)'\'') {
                    s.push_back('\'');
                }
                else if (*p == (T)'\"') {
                    s.push_back('"');
                }
                else if (*p == (T)'\\') {
                    s.push_back('\\');
                }
                else if (*p == (T)'n') {
                    s.push_back('\n');
                }
                else if (*p == (T)'r') {
                    s.push_back('\r');
                }
                else if (*p == (T)'t') {
                    s.push_back('\t');
                }
                else if (*p == (T)'b') {
                    s.push_back('\b');
                }
                else if (*p == (T)'v') {
                    s.push_back('\v');
                }
                else if (*p == (T)'x') {
                    // '\xHH'
                    char c = 0;
                    if (0 == *(++p)) goto _end;
                    c = (char)ctoi<T>(*p);
                    if (0 == *(++p)) goto _end;
                    c <<= 4;
                    c |= (char)ctoi<T>(*p);
                    s.push_back(T(c));
                }
                else if (*p == (T)'u') {
                    // '\uHHHH'
                    wchar_t c = 0;
                    if (0 == *(++p)) goto _end;
                    c = (char)ctoi<T>(*p);
                    if (0 == *(++p)) goto _end;
                    c <<= 4;
                    c |= (char)ctoi<T>(*p);
                    if (0 == *(++p)) goto _end;
                    c <<= 4;
                    c |= (char)ctoi<T>(*p);
                    if (0 == *(++p)) goto _end;
                    c <<= 4;
                    c |= (char)ctoi<T>(*p);
                    wchar_t ws[2] = { c, 0 };
                    if (sizeof(T) == sizeof(wchar_t)) {
                        s.push_back(c);
                    }
                    else {
                        s.append(conv::utf16toutf8(ws));
                    }
                }
                else {
                    s.append(*p);
                }
            }
            ++p;
        }

    _end:
        return s;
    }
}


#endif