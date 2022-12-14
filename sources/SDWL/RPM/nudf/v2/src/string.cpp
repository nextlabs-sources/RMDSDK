

#include <Windows.h>
#include <stdio.h>

#include <vector>

#include <nudf\string.hpp>



std::string NX::string_formater(const char* format, ...)
{
    va_list args;
    int     len = 0;
    std::string s;

    va_start(args, format);
    len = _vscprintf_l(format, 0, args) + 1;
    vsprintf_s(NX::string_buffer<char>(s, len), len, format, args);
    va_end(args);

    return std::move(s);
}

std::wstring NX::string_formater(const wchar_t* format, ...)
{
    va_list args;
    int     len = 0;
    std::wstring s;

    va_start(args, format);
    len = _vscwprintf_l(format, 0, args) + 1;
    vswprintf_s(NX::string_buffer<wchar_t>(s, len), len, format, args);
    va_end(args);

    return std::move(s);
}

std::string NX::safe_buffer_to_string(const unsigned char* buf, size_t size)
{
    if (0 == size || nullptr == buf) {
        return std::string();
    }
    const char* p = (const char*)buf;
    const char* end_pos = (const char*)(buf + size);
    while (p != end_pos && 0 != *p) {
        ++p;
    }
    return std::string((const char*)buf, p);
}
