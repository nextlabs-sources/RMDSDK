

#pragma once
#ifndef __NUDF_STRING_HPP__
#define __NUDF_STRING_HPP__



#include <exception>
#include <string>
#include <vector>

namespace NX {

template <typename T>
class string_buffer
{
public:
    string_buffer(std::basic_string<T>& str, size_t len) : _s(str)
    {
        // ctor
        _buf.resize(len + 1, 0);
    }


    ~string_buffer()
    {
        _s = std::basic_string<T>(_buf.data());      // copy to string passed by ref at construction
    }

    // auto conversion to serve as windows function parameter
    inline operator T* () throw() { return _buf.data(); }

private:
    // No copy allowed
    string_buffer(const string_buffer<T>& c) {}
    // No assignment allowed
    string_buffer& operator= (const string_buffer<T>& c) { return *this; }

private:
    std::basic_string<T>&   _s;
    std::vector<T>          _buf;
};

std::string string_formater(const char* format, ...);
std::wstring string_formater(const wchar_t* format, ...);
std::string safe_buffer_to_string(const unsigned char* buf, size_t size);

namespace utility {
    

template<typename CharType>
bool is_digit(CharType ch)
{
    return (ch >= CharType('0') && ch <= CharType('9'));
}

template<typename CharType>
bool is_hex(CharType ch)
{
    ch = (CharType)tolower((int)ch);
    return (is_digit<CharType>(ch) || (ch >= CharType('a') && ch <= CharType('f')));
}

template<typename CharType>
unsigned char hex_to_uchar(CharType c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    }
    else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    }
    else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    }
    else {
        ; // assert(false);
    }
    return 0;
}

template<typename CharType>
int hex_to_int(CharType c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    }
    else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    }
    else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    }
    else {
        ; // assert(false);
    }
    return 0;
}

__inline bool is_space(int c)
{
    return (c == ' '        // White space 0x20
         || c == '\f'       // form feed
         || c == '\n'       // line feed
         || c == '\r'       // carriage return
         || c == '\t'       // horizontal tab
         || c == '\v'       // vertical tab
         || c == 0x0085     // NEXT LINE
         || c == 0x00A0     // NO-BREAK SPACE
         || c == 0x1608     // OGHAM SPACE MARK
         || (c >= 0x2000 && c <= 0x200A)
                            // 0x2000 - EN QUAD
                            // 0x2001 - EM QUAD
                            // 0x2002 - EN SPACE
                            // 0x2003 - EM SPACE
                            // 0x2004 - THREE-PER-EM SPACE
                            // 0x2005 - FOUR-PER-EM SPACE
                            // 0x2006 - SIX-PER-EM SPACE
                            // 0x2007 - FIGURE SPACE
                            // 0x2008 - PUNCTUATION SPACE
                            // 0x2009 - THIN SPACE
                            // 0x200A - HAIR SPACE
         || c == 0x2028     // LINE SEPARATOR
         || c == 0x2029     // PARAGRAPH SEPARATOR
         || c == 0x202F     // NARROW NO-BREAK SPACE
         || c == 0x205F     // MEDIUM MATHEMATICAL SPACE
         || c == 0x3000     // IDEOGRAPHIC SPACE
         );
}

template<typename CharType>
bool is_alphabet(CharType ch)
{
    ch = (CharType)tolower((int)ch);
    return (ch >= CharType('a') && ch <= CharType('z'));
}

template<typename CharType>
bool iequal(CharType c1, CharType c2)
{
    return (tolower(c1) == tolower(c2));
}

template<typename CharType>
bool iequal(const CharType* s1, const CharType* s2)
{
    do {
        if (!iequal<CharType>(*s1, *s2)) {
            return false;
        }
    } while ((*(s1++)) != 0 && (*(s2++)) != 0);
    return true;
}

template<typename CharType>
bool iequal(const std::basic_string<CharType>& s1, const std::basic_string<CharType>& s2)
{
    return iequal<CharType>(s1.c_str(), s2.c_str());
}

template<typename CharType>
int icompare(CharType c1, CharType c2)
{
    c1 = tolower(c1);
    c2 = tolower(c2);
    return ((c1 == c2) ? 0 : (c1 > c2 ?  1 : (-1)));
}

template<typename CharType>
int icompare(const CharType* s1, const CharType* s2)
{
    int result = 0;
    do {
        result = icompare<CharType>(*s1, *s2);
    } while (0 == result && (*(s1++)) != 0 && (*(s2++)) != 0);
    return result;
}

template<typename CharType>
int icompare(const std::basic_string<CharType>& s1, const std::basic_string<CharType>& s2)
{
    return icompare<CharType>(s1.c_str(), s2.c_str());
}

template <typename CharType>
std::vector<unsigned char> hex_string_to_buffer(const std::basic_string<CharType>& s)
{
    std::vector<unsigned char> buf;
    const size_t count = s.length() / 2;
    for (int i = 0; i < (int)count; i++) {
        unsigned char ch = hex_to_uchar(s[2 * i]);
        ch <<= 4;
        ch |= hex_to_uchar(s[2 * i + 1]);
        buf.push_back(ch);
    }
    return std::move(buf);
}

template<class T>
void Split(const std::basic_string<T>& s, T c, std::vector<std::basic_string<T>>& list)
{
	std::basic_string<T> ls(s);
	std::basic_string<T>::size_type pos;
	do {
		pos = ls.find_first_of(c);
		if (std::basic_string<T>::npos == pos) {
			if (!ls.empty()) {
				boost::algorithm::trim(ls);
				if (!ls.empty()) {
					list.push_back(ls);
				}
			}
		}
		else {
			std::basic_string<T> component = ls.substr(0, pos);
			ls = ls.substr(pos + 1);
			boost::algorithm::trim(component);
			boost::algorithm::trim(ls);
			if (!component.empty()) {
				list.push_back(component);
			}
		}
	} while (!ls.empty() && std::basic_string<T>::npos != pos);
}

template<class T>
void Split(const std::basic_string<T>& s, const std::basic_string<T>& c, std::vector<std::basic_string<T>>& list)
{
	std::basic_string<T> ls(s);
	std::basic_string<T>::size_type pos;
	do {
		pos = ls.find(c);
		if (std::basic_string<T>::npos == pos) {
			if (!ls.empty()) {
				list.push_back(ls);
			}
		}
		else {
			std::basic_string<T> component = ls.substr(0, pos);
			ls = ls.substr(pos + c.length());
			if (!component.empty()) {
				list.push_back(component);
			}
		}
	} while (!ls.empty() && std::basic_string<T>::npos != pos);
}

template<class T>
void Split(const std::basic_string<T>& s, T c, std::vector<int>& list)
{
	std::basic_string<T> ls = Trim<T>(s);
	std::vector<std::basic_string<T>> slist;
	Split<T>(ls, c, slist);
	for (std::vector<std::basic_string<T>>::iterator it = slist.begin(); it != slist.end(); ++it) {
		int val = 0;
		if (ToInt(*it, &val)) {
			list.push_back(val);
		}
	}
}

template<class T>
void Split(const std::basic_string<T>& s, T c, std::vector<__int64>& list)
{
	std::basic_string<T> ls = Trim<T>(s);
	std::vector<std::basic_string<T>> slist;
	Split<T>(ls, c, slist);
	for (std::vector<std::basic_string<T>>::iterator it = slist.begin(); it != slist.end(); ++it) {
		__int64 val = 0;
		if (ToInt64(*it, &val)) {
			list.push_back(val);
		}
	}
}

template<class T>
void Split(const std::basic_string<T>& s, T c, std::vector<unsigned int>& list)
{
	std::basic_string<T> ls = Trim<T>(s);
	std::vector<std::basic_string<T>> slist;
	Split<T>(ls, c, slist);
	for (std::vector<std::basic_string<T>>::iterator it = slist.begin(); it != slist.end(); ++it) {
		unsigned int val = 0;
		if (ToUint(*it, &val)) {
			list.push_back(val);
		}
	}
}

template<class T>
void Split(const std::basic_string<T>& s, T c, std::vector<unsigned __int64>& list)
{
	std::basic_string<T> ls = Trim<T>(s);
	std::vector<std::basic_string<T>> slist;
	Split<T>(ls, c, slist);
	for (std::vector<std::basic_string<T>>::iterator it = slist.begin(); it != slist.end(); ++it) {
		unsigned __int64 val = 0;
		if (ToUint64(*it, &val)) {
			list.push_back(val);
		}
	}
}

template <typename CharType>
int Replaceall_string(_Inout_ std::basic_string<CharType> & sourcestr, _In_ const std::basic_string<CharType> replacestr, _In_ const std::basic_string<CharType> newstr)
{
	int iret = 0;
	size_t pos = sourcestr.find(replacestr);
	while (std::basic_string<CharType>::npos != pos) {
		sourcestr.replace(pos, replacestr.length(), newstr);
		pos = sourcestr.find(replacestr, pos + newstr.length());
		iret++;
	}

	return iret;
}
}

}

#endif