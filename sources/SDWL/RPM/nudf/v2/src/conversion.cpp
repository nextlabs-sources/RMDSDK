


#include <Windows.h>
#include <assert.h>
#include <Wincrypt.h>

#include <algorithm>
#include <codecvt>

#include <nudf\eh.hpp>
#include <nudf\string.hpp>
#include <nudf\handyutil.hpp>
#include <nudf\conversion.hpp>



std::wstring NX::conversion::utf8_to_utf16(const std::string& s)
{
    std::wstring ws;
    if (s.empty()) {
        return ws;
    }
    const unsigned long reserved_size = (unsigned long)(s.length() + 1);
    ws.reserve(reserved_size);
    if (!s.empty()) {
        if (0 == MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), NX::string_buffer<wchar_t>(ws, reserved_size), reserved_size)) {
            ws.clear();
        }
    }
    return std::move(ws);
}

std::string NX::conversion::utf16_to_utf8(const std::wstring& ws)
{
    std::string s;
    if (ws.empty()) {
        return s;
    }
    const unsigned long reserved_size = (unsigned long)((ws.length() / 2) * 3 + 1);
    s.reserve(reserved_size);
    if (!ws.empty()) {
        if (0 == WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.length(), NX::string_buffer<char>(s, reserved_size), (int)reserved_size, nullptr, nullptr)) {
            s.clear();
        }
    }
	return s;// std::move(s);
}

std::wstring NX::conversion::ansi_to_utf16(const std::string& s)
{
    std::wstring ws;
    if (s.empty()) {
        return ws;
    }
    const unsigned long reserved_size = (unsigned long)(s.length() + 1);
    ws.reserve(reserved_size);
    if (!s.empty()) {
        if (0 == MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.length(), NX::string_buffer<wchar_t>(ws, reserved_size), (int)reserved_size)) {
            ws.clear();
        }
    }
    return std::move(ws);
}

std::string NX::conversion::ansi_to_utf8(const std::string& s)
{
    if (s.empty()) {
        return std::string();
    }
    std::wstring ws = ansi_to_utf16(s);
    return std::move(utf16_to_utf8(ws));
}

std::wstring NX::conversion::to_utf16(const std::wstring& s)
{
    return s;
}

std::wstring NX::conversion::to_utf16(const std::string& s)
{
    return std::move(NX::conversion::utf8_to_utf16(s));
}

std::wstring NX::conversion::to_wstring(unsigned char v)
{
    std::wstring ws = NX::string_formater(L"%02X", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(unsigned short v)
{
    std::wstring ws = NX::string_formater(L"0x%04X", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(unsigned long v)
{
    std::wstring ws = NX::string_formater(L"0x%08X", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(unsigned int v)
{
    std::wstring ws = NX::string_formater(L"0x%08X", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(unsigned __int64 v)
{
    std::wstring ws = NX::string_formater(L"0x%016llX", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(short v)
{
    std::wstring ws = NX::string_formater(L"%d", (int)v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(long v)
{
    std::wstring ws = NX::string_formater(L"%d", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(int v)
{
    std::wstring ws = NX::string_formater(L"%d", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(__int64 v)
{
    std::wstring ws = NX::string_formater(L"%I64d", v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(float v, int precision)
{
    std::wstring fmt(L"%%.");
    fmt += to_wstring(precision) + L"f";
    std::wstring ws = NX::string_formater(fmt.c_str(), v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(double v, int precision)
{
    std::wstring fmt(L"%%.");
    fmt += to_wstring(precision) + L"f";
    std::wstring ws = NX::string_formater(fmt.c_str(), v);
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(unsigned char* v, size_t size)
{
    std::wstring ws;
    std::for_each(v, v + size, [&](unsigned char c) {
        ws += to_wstring(c);
    });
    return std::move(ws);
}

std::wstring NX::conversion::to_wstring(const std::vector<unsigned char>& v)
{
    std::wstring ws;
    std::for_each(v.begin(), v.end(), [&](unsigned char c) {
        ws += to_wstring(c);
    });
    return std::move(ws);
}

std::string NX::conversion::to_string(unsigned char v)
{
    std::string s = NX::string_formater("%02X", v);
    return std::move(s);
}

std::string NX::conversion::to_string(unsigned short v)
{
    std::string s = NX::string_formater("0x%04X", v);
    return std::move(s);
}

std::string NX::conversion::to_string(unsigned long v)
{
    std::string s = NX::string_formater("0x%08X", v);
    return std::move(s);
}

std::string NX::conversion::to_string(unsigned int v)
{
    std::string s = NX::string_formater("0x%08X", v);
    return std::move(s);
}

std::string NX::conversion::to_string(unsigned __int64 v)
{
    std::string s = NX::string_formater("0x%016llX", v);
    return std::move(s);
}

std::string NX::conversion::to_string(short v)
{
    std::string s = NX::string_formater("%d", (int)v);
    return std::move(s);
}

std::string NX::conversion::to_string(long v)
{
    std::string s = NX::string_formater("%d", v);
    return std::move(s);
}

std::string NX::conversion::to_string(int v)
{
    std::string s = NX::string_formater("%d", v);
    return std::move(s);
}

std::string NX::conversion::to_string(__int64 v)
{
    std::string s = NX::string_formater("%I64d", v);
    return std::move(s);
}

std::string NX::conversion::to_string(float v, int precision)
{
    std::string fmt("%%.");
    fmt += to_string(precision) + "f";
    std::string s = NX::string_formater(fmt.c_str(), v);
    return std::move(s);
}

std::string NX::conversion::to_string(double v, int precision)
{
    std::string fmt("%%.");
    fmt += to_string(precision) + "f";
    std::string s = NX::string_formater(fmt.c_str(), v);
    return std::move(s);
}

std::string NX::conversion::to_string(unsigned char* v, size_t size)
{
    std::string s;
    std::for_each(v, v + size, [&](unsigned char c) {
        s += to_string(c);
    });
    return std::move(s);
}

std::string NX::conversion::to_string(const std::vector<unsigned char>& v)
{
    std::string s;
    std::for_each(v.begin(), v.end(), [&](unsigned char c) {
        s += to_string(c);
    });
    return std::move(s);
}

std::wstring NX::conversion::x_www_form_urlencode(const std::wstring& raw_content)
{
    std::wstring s;

    for (int i = 0; i < (int)raw_content.length(); i++) {
        const wchar_t ch = raw_content[i];
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9') || ch == L'.' || ch == L'-' || ch == L'*' || ch == L'_' || ch == L'=' || ch == L'&') {
            s += ch;
        }
        else if (ch == L' ') {
            s += L'+';
        }
        else {
            const wchar_t utf16_ch[2] = { ch, 0 };
            std::string utf8_ch = NX::conversion::utf16_to_utf8(utf16_ch);
            for (unsigned int j = 0; j < utf8_ch.length(); j++) {
                s += '%';
                s += to_wstring((unsigned char)utf8_ch[j]);
            }
        }
    }

    return std::move(s);
}

std::wstring NX::conversion::x_www_form_urldecode(const std::wstring& encoded_content)
{
    std::wstring s;
    const wchar_t* p = encoded_content.c_str();
    int left_ch = (int)encoded_content.length();

    while (0 != *p) {

        if (*p != L'%') {
            s += (*p == L'+') ? L' ' : ((wchar_t)(*p));
            --left_ch;
        }
        else {
            assert(*p == L'%');
            std::string utf8_str;
            while (*p == L'%') {
                assert(left_ch >= 3);
                if (left_ch < 3) {
                    return std::move(s);   // error
                }
                assert(NX::utility::is_hex<wchar_t>(*(p + 1)) && NX::utility::is_hex<wchar_t>(*(p + 2)));
                if(!NX::utility::is_hex<wchar_t>(*(p + 1)) || !NX::utility::is_hex<wchar_t>(*(p + 2))) {
                    return std::move(s);   // error
                }
                char byte_ch = (char)NX::utility::hex_to_uchar<wchar_t>(*(p + 1));
                byte_ch <<= 4;
                byte_ch |= (char)NX::utility::hex_to_uchar<wchar_t>(*(p + 2));
                utf8_str += byte_ch;
                // move to next
                p += 3;
                left_ch -= 3;
            }
            // convert utf8 to utf-16
            s += utf8_to_utf16(utf8_str);
        }
    }

    return std::move(s);
}

std::wstring NX::conversion::to_base64(const std::vector<unsigned char>& data)
{
    return to_base64(data.data(), (unsigned long)data.size());
}

std::wstring NX::conversion::to_base64(const unsigned char* data, unsigned long size)
{
    unsigned long out_size = 0;
    CryptBinaryToStringA(data, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &out_size);
    std::vector<char> buf;
    buf.resize(out_size + 1, 0);
    if (!CryptBinaryToStringA(data, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf.data(), &out_size)) {
        buf.clear();
        return std::wstring();
    }
    std::string s = buf.data();
    return std::wstring(s.begin(), s.end());
}

std::vector<unsigned char> NX::conversion::from_base64(const std::string& base64)
{
    std::vector<unsigned char> buf;
    unsigned long out_size = 0;
    CryptStringToBinaryA(base64.data(), (unsigned long)base64.length(), CRYPT_STRING_ANY, NULL, &out_size, NULL, NULL);
    buf.resize(out_size, 0);
    if (!CryptStringToBinaryA(base64.data(), (unsigned long)base64.length(), CRYPT_STRING_ANY, buf.data(), &out_size, NULL, NULL)) {
        buf.clear();
    }
    return std::move(buf);
}

std::vector<unsigned char> NX::conversion::from_base64(const std::wstring& base64)
{
    std::string s = NX::conversion::utf16_to_utf8(base64);
    return from_base64(s);
}

std::vector<unsigned char> NX::conversion::hex_to_bytes(const std::wstring& wstr) {

	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
	std::string hex = conv1.to_bytes(wstr);
	std::vector<unsigned char> bytes;
	for (unsigned int i = 0; i < hex.length(); i += 2) {
		std::string byteString = hex.substr(i, 2);
		unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
	}
	return bytes;
}