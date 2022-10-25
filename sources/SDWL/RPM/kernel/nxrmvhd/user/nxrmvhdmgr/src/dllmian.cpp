

#include <Windows.h>

#include <nudf\eh.hpp>
#include <nudf\winutil.hpp>
#include <nudf\filesys.hpp>
#include <nudf\resutil.hpp>
#include <nudf\zip.hpp>

#include "resource.h"
#include "nxrmvhddef.h"
#include "vhdmgr.hpp"


HINSTANCE _hInstance = NULL;

extern bool extract_template_disk(unsigned long id, const std::wstring& file);

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        _hInstance = hInstance;
        ::DisableThreadLibraryCalls((HMODULE)hInstance);
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return TRUE;
}


bool WINAPI NCreateVhd(_In_ LPCWSTR wzFile, _In_ ULONG dwSize, _In_ bool bEncrypt, _In_opt_ const WCHAR* wzPassword)
{
    NX::win::guid disk_guid = NX::win::guid::generate();

    if (!VHD::CreateVhd(wzFile, dwSize, bEncrypt ? TRUE : FALSE, (const unsigned char*)&disk_guid, wzPassword)) {
        NX::set_last_error(ERROR_UNKNOWN_ERROR, false);
        return false;
    }

    return true;
}

bool WINAPI NCreateVhdFromTemplate(_In_ ULONG dwTemplateId, _In_ LPCWSTR wzFile, _In_ LPCWSTR wzPassword, _In_ bool bEncrypt)
{
    const UINT uResId = (3 == dwTemplateId) ? IDR_LARGE_VHD : ((2 == dwTemplateId) ? IDR_MEDIUM_VHD : IDR_SMALL_VHD);
    const ULONG decompress_buf_size = (3 == dwTemplateId) ? 27262976UL : ((2 == dwTemplateId) ? 20971520UL : 18874368UL);
    const unsigned char* pdata = nullptr;
    unsigned long size = 0;

    if (!NX::RES::extract_resource(_hInstance, uResId, L"ZIP", (unsigned char**)&pdata, &size)) {
        return false;
    }

    assert(NULL != pdata);
    assert(0 != size);

    if (!NX::ZIP::decompress_to_file(pdata, size, decompress_buf_size, wzFile, true)) {
        return false;
    }

    if (!VHD::ReinitializeTemplateVhd(wzFile, wzPassword, bEncrypt ? TRUE : FALSE)) {
        ::DeleteFile(wzFile);
        return false;
    }

    return true;
}

bool WINAPI NFormatVhd(_In_ WCHAR cDrive)
{
    NX::set_last_error(ERROR_INVALID_FUNCTION, false);
    return false;
}

bool WINAPI NMountVhd(_In_ LPCWSTR wzFile, _In_opt_ LPCWSTR wzPassword, bool bVisible, WCHAR cPreferredDrive, _Out_ PULONG pdwDiskId)
{
    if (!VHD::MountVhd(wzFile, wzPassword, bVisible ? TRUE : FALSE, cPreferredDrive, pdwDiskId)) {
        NX::set_last_error(ERROR_UNKNOWN_ERROR, false);
        return false;
    }

    return true;
}

bool WINAPI NUnmountVhd(_In_ ULONG dwDiskId)
{
    if (!VHD::UnmountVhd(dwDiskId)) {
        NX::set_last_error(ERROR_UNKNOWN_ERROR, false);
        return false;
    }

    return true;
}

bool WINAPI NGetVhdProperty(_In_ ULONG dwDiskId, _In_ ULONG dwPropertyId, _Out_ LPVOID Buffer, _Inout_ PULONG pdwSize)
{
    VHD::CVhdMgr manager;
    VHD::CVhdInfo vhdi;
    bool ret = false;
    const unsigned long input_size = *pdwSize;
    
    if (!manager.Query(dwDiskId, vhdi)) {
        NX::set_last_error(ERROR_NOT_FOUND, false);
        return false;
    }

    unsigned long output_size = 0;

    switch (dwPropertyId)
    {
    case VHD_PROPERTY_NT_NAME:
        if (vhdi.GetNtName().empty()) {
            NX::set_last_error(ERROR_NOT_FOUND, false);
        }
        else {
            output_size = (unsigned long)(sizeof(wchar_t) * (vhdi.GetNtName().length() + 1));
            if (input_size < output_size) {
                NX::set_last_error(ERROR_BUFFER_OVERFLOW, false);
            }
            else {
                memcpy(Buffer, vhdi.GetNtName().c_str(), output_size);
                ret = true;
            }
        }
        break;

    case VHD_PROPERTY_DOS_NAME:
        if (vhdi.GetDosName().empty()) {
            NX::set_last_error(ERROR_NOT_FOUND, false);
        }
        else {
            output_size = (unsigned long)(sizeof(wchar_t) * (vhdi.GetDosName().length() + 1));
            if (input_size < output_size) {
                NX::set_last_error(ERROR_BUFFER_OVERFLOW, false);
            }
            else {
                memcpy(Buffer, vhdi.GetDosName().c_str(), output_size);
                ret = true;
            }
        }
        break;

    default:
        NX::set_last_error(ERROR_NOT_FOUND, false);
        break;
    }

    *pdwSize = output_size;
    return ret;
}

bool WINAPI NSetVhdProperty(_In_ ULONG dwDiskId, _In_ ULONG dwPropertyId, _In_ LPVOID Buffer, _In_ ULONG pdwSize)
{
    NX::set_last_error(ERROR_INVALID_FUNCTION, false);
    return false;
}

bool WINAPI NVerifyVhdPassword(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword)
{
    return VHD::VerifyPassword(wzFile, wzPassword) ? true : false;
}