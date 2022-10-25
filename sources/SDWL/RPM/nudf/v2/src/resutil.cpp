
#include <Windows.h>
#include <stdio.h>
#include <assert.h>

#include <nudf\resutil.hpp>
#include <nudf\string.hpp>

using namespace NX::RES;



std::wstring NX::RES::LoadMessage(_In_ HMODULE module, _In_ UINT id, _In_ ULONG max_length, _In_ DWORD langid, _In_opt_ LPCWSTR default_msg)
{
    std::wstring wstr;
    if(NULL == module || 0 == FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, module, id, langid, NX::string_buffer<wchar_t>(wstr, max_length),  max_length,  NULL)) {
        wstr.clear();
    }
    if(wstr.empty() && NULL!=default_msg) {
        wstr = default_msg;
    }
    return wstr;
}

std::wstring NX::RES::LoadMessageEx(_In_ HMODULE module, _In_ UINT id, _In_ ULONG max_length, _In_ DWORD langid, _In_opt_ LPCWSTR default_msg, ...)
{
    std::wstring wstr;
    std::wstring fmt;
    std::vector<wchar_t> buf;
    int len = 0;

    fmt = NX::RES::LoadMessage(module, id, max_length, langid, default_msg);
    if(fmt.empty()) {
        return wstr;
    }

    va_list args;
    va_start(args, default_msg);
    len = _vscwprintf_l(fmt.c_str(), 0, args) + 1;
    buf.resize(len, 0);
    if(!buf.empty()) {
        vswprintf_s(&buf[0], len, fmt.c_str(), args); // C4996
        wstr = &buf[0];
    }
    va_end(args);

    return wstr;
}

bool NX::RES::extract_resource(_In_opt_ HINSTANCE hInstance, _In_ UINT id, _In_ LPCWSTR type, _Out_ unsigned char** ppdata, _Out_ unsigned long* psize)
{
    HRSRC hrsrc = FindResourceW(hInstance, MAKEINTRESOURCE(LOWORD((UINT)(UINT_PTR)id)), type);
    if (NULL == hrsrc) {
        SetLastError(ERROR_NOT_FOUND);
        return false;
    }

    DWORD dwSize = SizeofResource(hInstance, hrsrc);
    if (0 == dwSize) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }

    HGLOBAL hglob = LoadResource(hInstance, hrsrc);
    if (NULL == hglob) {
        return false;
    }

    LPVOID lplock = LockResource(hglob);
    if (NULL == lplock) {
        return false;
    }

    *ppdata = (unsigned char*)lplock;
    *psize = dwSize;
    return true;
}

bool NX::RES::extract_resource(_In_opt_ HINSTANCE hInstance, _In_ UINT id, _In_ LPCWSTR type, _In_ std::vector<unsigned char>& data)
{
    const unsigned char* p = NULL;
    unsigned long size = 0;

    if (!NX::RES::extract_resource(hInstance, id, type, (unsigned char**)&p, &size)) {
        return false;
    }

    assert(NULL != p);
    assert(0 != size);

    data.resize(size, 0);
    memcpy(data.data(), p, size);
    return true;
}

bool NX::RES::extract_resource(_In_opt_ HINSTANCE hInstance, _In_ UINT id, _In_ LPCWSTR type, _In_ const std::wstring& file, _In_ DWORD file_attributes, _In_ bool replace_existing)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    bool result = false;
    bool file_created = false;

    const unsigned char* p = NULL;
    unsigned long size = 0;

    if (!NX::RES::extract_resource(hInstance, id, type, (unsigned char**)&p, &size)) {
        return false;
    }

    assert(NULL != p);
    assert(0 != size);

    do 
    {
        DWORD bytes_written = 0;

        const bool read_only = (FILE_ATTRIBUTE_READONLY == (file_attributes & FILE_ATTRIBUTE_READONLY));
        const bool hidden_file = (FILE_ATTRIBUTE_HIDDEN == (file_attributes & FILE_ATTRIBUTE_HIDDEN));
        if (read_only) {
            file_attributes &= (~FILE_ATTRIBUTE_READONLY);
        }

        h = ::CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, replace_existing ? CREATE_ALWAYS : CREATE_NEW, file_attributes, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            break;
        }

        file_created = true;

        if (!::WriteFile(h, p, size, &bytes_written, NULL)) {
            break;
        }
        // Succeeded, close file
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;

        // set read_only attribute if necessary
        if (read_only) {
            file_attributes |= FILE_ATTRIBUTE_READONLY;
            ::SetFileAttributesW(file.c_str(), file_attributes);
        }

        result = true;

    } while (FALSE);

    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
    if (!result && file_created) {
        ::DeleteFileW(file.c_str());
    }

    return result;
}