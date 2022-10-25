
#pragma once
#ifndef __NXRM_VHD_HELP_HPP__
#define __NXRM_VHD_HELP_HPP__

#include <Windows.h>

#include <string>



#

class vhd_manager_dll
{
private:
    typedef bool (WINAPI* FnNCreateVhd)(_In_ LPCWSTR wzFile, _In_ ULONG dwSize, _In_ bool bEncrypt, _In_opt_ const WCHAR* wzPassword);
    typedef bool (WINAPI* FnNCreateVhdFromTemplate)(_In_ ULONG dwTemplateId, _In_ LPCWSTR wzFile, _In_ LPCWSTR wzPassword, _In_ bool bEncrypt);
    typedef bool (WINAPI* FnNFormatVhd)(_In_ WCHAR cDrive);
    typedef bool (WINAPI* FnNMountVhd)(_In_ LPCWSTR wzFile, _In_opt_ LPCWSTR wzPassword, bool bVisible, WCHAR cPreferredDrive, _Out_ PULONG pdwDiskId);
    typedef bool (WINAPI* FnNUnmountVhd)(_In_ ULONG dwDiskId);
    typedef bool (WINAPI* FnNGetVhdProperty)(_In_ ULONG dwDiskId, _In_ ULONG dwPropertyId, _Out_ LPVOID Buffer, _Inout_ PULONG pdwSize);
    typedef bool (WINAPI* FnNSetVhdProperty)(_In_ ULONG dwDiskId, _In_ ULONG dwPropertyId, _In_ LPVOID Buffer, _In_ ULONG pdwSize);
    typedef bool (WINAPI* FnNVerifyVhdPassword)(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword);

public:
    vhd_manager_dll() : _h(NULL),
        NCreateVhd(NULL),
        NCreateVhdFromTemplate(NULL),
        NFormatVhd(NULL),
        NMountVhd(NULL),
        NUnmountVhd(NULL),
        NGetVhdProperty(NULL),
        NSetVhdProperty(NULL),
        NVerifyVhdPassword(NULL)
    {
    }

    ~vhd_manager_dll()
    {
        clear();
    }

    void initialize(const std::wstring& dll_path)
    {
        if (_h != NULL) {
            return;
        }

        _h = ::LoadLibraryW(dll_path.c_str());
        if (NULL == _h) {
            throw std::exception("DLL not found");
        }

        try {

            NCreateVhd = (FnNCreateVhd)GetProcAddress(_h, MAKEINTRESOURCEA(1));
            if (NULL == NCreateVhd) {
                throw std::exception("Function \"NCreateVhd\" not found");
            }

            NCreateVhdFromTemplate = (FnNCreateVhdFromTemplate)GetProcAddress(_h, MAKEINTRESOURCEA(2));
            if (NULL == NCreateVhdFromTemplate) {
                throw std::exception("Function \"NCreateVhdFromTemplate\" not found");
            }

            NFormatVhd = (FnNFormatVhd)GetProcAddress(_h, MAKEINTRESOURCEA(3));
            if (NULL == NFormatVhd) {
                throw std::exception("Function \"NFormatVhd\" not found");
            }

            NMountVhd = (FnNMountVhd)GetProcAddress(_h, MAKEINTRESOURCEA(4));
            if (NULL == NMountVhd) {
                throw std::exception("Function \"NMountVhd\" not found");
            }

            NUnmountVhd = (FnNUnmountVhd)GetProcAddress(_h, MAKEINTRESOURCEA(5));
            if (NULL == NUnmountVhd) {
                throw std::exception("Function \"NUnmountVhd\" not found");
            }

            NGetVhdProperty = (FnNGetVhdProperty)GetProcAddress(_h, MAKEINTRESOURCEA(6));
            if (NULL == NGetVhdProperty) {
                throw std::exception("Function \"NGetVhdProperty\" not found");
            }

            NSetVhdProperty = (FnNSetVhdProperty)GetProcAddress(_h, MAKEINTRESOURCEA(7));
            if (NULL == NSetVhdProperty) {
                throw std::exception("Function \"NSetVhdProperty\" not found");
            }

            NVerifyVhdPassword = (FnNVerifyVhdPassword)GetProcAddress(_h, MAKEINTRESOURCEA(8));
            if (NULL == NVerifyVhdPassword) {
                throw std::exception("Function \"NVerifyVhdPassword\" not found");
            }

        }
        catch (const std::wstring& e) {
            clear();
            throw e;
        }
    }

    void clear()
    {
        NCreateVhd = NULL;
        NCreateVhdFromTemplate = NULL;
        NFormatVhd = NULL;
        NMountVhd = NULL;
        NUnmountVhd = NULL;
        NGetVhdProperty = NULL;
        NSetVhdProperty = NULL;
        NVerifyVhdPassword = NULL;
        if (_h != NULL) {
            ::FreeLibrary(_h);
            _h = NULL;
        }
    }

    typedef enum VHD_TEMPLATE_ID {
        SMALL_VHD = 0,
        MEDIUM_VHD,
        LARGE_VHD
    } VHD_TEMPLATE_ID;

    inline bool empty() const { return (NULL == _h); }
    inline bool vhd_create(const std::wstring& file, unsigned long size_in_mb, bool encrypt_required, const std::wstring& password) {
        return empty() ? false : NCreateVhd(file.c_str(), size_in_mb, encrypt_required, password.empty() ? NULL : password.c_str());
    }
    inline bool vhd_create_from_template(VHD_TEMPLATE_ID id, const std::wstring& file, const std::wstring& password, bool encrypt_required) {
        return empty() ? false : NCreateVhdFromTemplate(id, file.c_str(), password.empty() ? NULL : password.c_str(), encrypt_required);
    }
    inline bool vhd_mount(const std::wstring& file, const std::wstring& password, bool visible, wchar_t preffered_drive_letter, unsigned long* pdisk_id) {
        return empty() ? false : NMountVhd(file.c_str(), password.empty() ? NULL : password.c_str(), visible, preffered_drive_letter, pdisk_id);
    }
    inline bool vhd_unmount(unsigned long disk_id) {
        return empty() ? false : NUnmountVhd(disk_id);
    }
    inline bool vhd_get_property(unsigned long disk_id, unsigned long property_id, _Out_writes_bytes_opt_(*buf_size) void* buf, _Inout_ unsigned long* buf_size) {
        return empty() ? false : NGetVhdProperty(disk_id, property_id, buf, buf_size);
    }
    inline bool vhd_set_property(unsigned long disk_id, unsigned long property_id, _Out_writes_bytes_opt_(buf_size) const void* buf, _Inout_ unsigned long buf_size) {
        return empty() ? false : NSetVhdProperty(disk_id, property_id, (LPVOID*)buf, buf_size);
    }
    inline bool vhd_verify_password(const std::wstring& file, const std::wstring& password) {
        return empty() ? false : NVerifyVhdPassword(file.c_str(), password.empty() ? NULL : password.c_str());
    }

private:
    HMODULE _h;
    FnNCreateVhd                NCreateVhd;
    FnNCreateVhdFromTemplate    NCreateVhdFromTemplate;
    FnNFormatVhd                NFormatVhd;
    FnNMountVhd                 NMountVhd;
    FnNUnmountVhd               NUnmountVhd;
    FnNGetVhdProperty           NGetVhdProperty;
    FnNSetVhdProperty           NSetVhdProperty;
    FnNVerifyVhdPassword        NVerifyVhdPassword;
};



#endif