

#pragma once
#ifndef __NUDF_CONVERSION_HPP__
#define __NUDF_CONVERSION_HPP__


#include <string>
#include <vector>
#include <algorithm>

namespace NX {
namespace conversion {

std::wstring utf8_to_utf16(const std::string& s);
std::string utf16_to_utf8(const std::wstring& ws);
std::wstring ansi_to_utf16(const std::string& s);
std::string ansi_to_utf8(const std::string& s);

std::wstring to_utf16(const std::wstring& s);
std::wstring to_utf16(const std::string& s);

std::wstring to_wstring(unsigned char v);
std::wstring to_wstring(unsigned short v);
std::wstring to_wstring(unsigned long v);
std::wstring to_wstring(unsigned int v);
std::wstring to_wstring(unsigned __int64 v);
std::wstring to_wstring(short v);
std::wstring to_wstring(long v);
std::wstring to_wstring(int v);
std::wstring to_wstring(__int64 v);
std::wstring to_wstring(float v, int precision = 3);
std::wstring to_wstring(double v, int precision = 7);
std::wstring to_wstring(unsigned char* v, size_t size);
std::wstring to_wstring(const std::vector<unsigned char>& v);

std::string to_string(unsigned char v);
std::string to_string(unsigned short v);
std::string to_string(unsigned long v);
std::string to_string(unsigned int v);
std::string to_string(unsigned __int64 v);
std::string to_string(short v);
std::string to_string(long v);
std::string to_string(int v);
std::string to_string(__int64 v);
std::string to_string(float v, int precision = 3);
std::string to_string(double v, int precision = 7);
std::string to_string(unsigned char* v, size_t size);
std::string to_string(const std::vector<unsigned char>& v);

std::wstring x_www_form_urlencode(const std::wstring& raw_content);
std::wstring x_www_form_urldecode(const std::wstring& encoded_content);

std::wstring to_base64(const std::vector<unsigned char>& data);
std::wstring to_base64(const unsigned char* data, unsigned long size);
std::vector<unsigned char> from_base64(const std::string& base64);
std::vector<unsigned char> from_base64(const std::wstring& base64);
std::vector<unsigned char> hex_to_bytes(const std::wstring& hex);

template <typename T>
std::basic_string<T> lower_str(const std::basic_string<T>& s)
{
    std::basic_string<T> s2(s);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
    return std::move(s2);
}

template <typename T>
std::basic_string<T> upper_str(const std::basic_string<T>& s)
{
    std::basic_string<T> s2(s);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);
    return std::move(s2);
}

#define strlower lower_str<char>
#define wcslower lower_str<wchar_t>
#define strupper upper_str<char>
#define wcsupper upper_str<wchar_t>


typedef enum pad_position {
    pad_front = 0,
    pad_end
} pad_position;

template<typename CharType>
void to_fixed_width(std::basic_string<CharType>& s, size_t width, pad_position pos = pad_front, CharType ch = CharType(' '))
{
    while (width > s.length()) {
        (pad_front == pos) ? s.insert(0, ch) : s.push_back(ch);
    }
}

template<typename CharType>
std::basic_string<CharType> to_fixed_width_copy(const std::basic_string<CharType>& s, size_t width, pad_position pos = pad_front, CharType ch = CharType(' '))
{
    std::basic_string<CharType> scopy;
    if (pad_front == pos) {
        if (width > s.length()) {
            scopy.resize(width - s.length(), ch);
        }
        scopy += s;
    }
    else {
        scopy += s;
        while (width > scopy.length()) {
            scopy.push_back(ch);
        }
    }
    return std::move(scopy);
}

template <typename CharType>
std::vector<std::basic_string<CharType>> buffer_to_strings(const std::vector<CharType>& buf)
{
    std::vector<std::basic_string<CharType>> v;
    if (buf.empty()) {
        return v;
    }
    const CharType* p = buf.data();
    while (0 != (*p)) {
        std::basic_string<CharType> s(p);
        v.push_back(s);
        p += s.length() + 1;
    }
    return std::move(v);
}

template <typename CharType>
std::vector<CharType> strings_to_buffer(const std::vector<std::basic_string<CharType>>& strings)
{
    std::vector<CharType> buf;
    if (strings.empty()) {
        return std::move(buf);
    }
    std::for_each(strings.begin(), strings.end(), [&](const std::basic_string<CharType>& s) {
        std::for_each(s.begin(), s.end(), [&](const CharType& c) {
            buf.push_back(c);
        });
        buf.push_back(CharType(0));
    });
    buf.push_back(CharType(0));
    return std::move(buf);
}

template <typename CharType>
std::vector<CharType> pair_to_buffer(_In_ const std::vector<std::pair<std::basic_string<CharType>,std::basic_string<CharType>>> &pairs)
{
	std::vector<CharType> buf;
	if (pairs.empty()) {
		return std::move(buf);
	}
	std::for_each(pairs.begin(), pairs.end(), [&](const std::pair < std::basic_string<CharType>,std::basic_string<CharType>>& p) {
		if (!p.first.empty()) {
			std::for_each(p.first.begin(), p.first.end(), [&](const CharType& c) {
				buf.push_back(c);
			});
			buf.push_back(CharType('='));
			std::for_each(p.second.begin(), p.second.end(), [&](const CharType& c) {
				buf.push_back(c);
			});
			buf.push_back(CharType(0));
		}
	});
	buf.push_back(CharType(0));
	return std::move(buf);
}

template <typename CharType>
std::vector<std::pair<std::basic_string<CharType>, std::basic_string<CharType>>> buffer_to_pair(_In_ const std::vector<CharType> buf)
{
	std::vector<std::pair<std::basic_string<CharType>, std::basic_string<CharType>>> pairs;
	if (buf.empty()) {
		return std::move(pairs);
	}
	bool bfstring = true;
	std::basic_string<CharType> first, second;
	std::for_each(buf.begin(), buf.end(), [&](const CharType& c) {
		if (c == CharType(0)) {
			if (!first.empty()){
				pairs.push_back(std::pair<std::basic_string<CharType>, std::basic_string<CharType>>(first, second));
				first.clear();
				second.clear();
				bfstring = true;
			}
		}
		else if (c == CharType('=')) {
			bfstring = false;
		}
		else {
			if (bfstring) {
				first.push_back(c);
			}
			else {
				second.push_back(c);
			}
		}
	});
	return std::move(pairs);
}

template <typename CharType>
std::vector<std::pair<std::basic_string<CharType>, std::basic_string<CharType>>> buffer_to_pair(_In_ const CharType * pbuf)
{
	std::vector<std::pair<std::basic_string<CharType>, std::basic_string<CharType>>> pairs;
	if (NULL == pbuf) {
		return std::move(pairs);
	}
	bool bfstring = true;
	std::basic_string<CharType> first, second;
	while (pbuf[0] != CharType(0)) {
		std::basic_string<CharType> wsPair(pbuf);
		pbuf += (wsPair.length() + 1);
		std::basic_string<CharType> name;
		std::basic_string<CharType> value;
		typename std::basic_string<CharType>::size_type pos = wsPair.find_first_of(L'=');
		if (pos == std::wstring::npos) {
			continue;
		}
		name = wsPair.substr(0, pos);
		value = wsPair.substr(pos + 1);
		pairs.push_back(std::pair<std::basic_string<CharType>, std::basic_string<CharType>>(name, value));
	}
	return std::move(pairs);
}

__forceinline unsigned short convert_endian16(unsigned short u)
{
    return (((u >> 8) & 0x00FF) | ((u << 8) & 0xFF00));
}

__forceinline unsigned long convert_endian32(unsigned long u)
{
    return (  ((u >> 24) & 0xFF)
            | ((u >> 8) & 0xFF00)
            | ((u << 24) & 0xFF000000)
            | ((u << 8) & 0xFF0000)
            );
}

__forceinline unsigned __int64 convert_endian64(unsigned __int64 u)
{
    return (  ((u >> 56) & 0xFF)
            | ((u >> 40) & 0xFF00)
            | ((u >> 24) & 0xFF0000)
            | ((u >> 8) & 0xFF000000)
            | ((u << 56) & 0xFF00000000000000)
            | ((u << 40) & 0xFF000000000000)
            | ((u << 24) & 0xFF0000000000)
            | ((u << 8) & 0xFF00000000)
            );
}

}
}


#endif