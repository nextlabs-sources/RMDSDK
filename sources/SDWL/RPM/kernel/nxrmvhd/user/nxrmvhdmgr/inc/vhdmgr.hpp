

#ifndef __VHD_MGR_HPP__
#define __VHD_MGR_HPP__


#include <string>
#include <vector>

#include "nxrmvhddef.h"


namespace VHD {



BOOL CreateVhd(_In_ LPCWSTR wzFile, _In_ ULONG dwSize, _In_ BOOL bEncrypt, _In_opt_bytecount_(16) const UCHAR* pbDuid, _In_opt_ const WCHAR* wzPassword);
BOOL CreateVhdEx(_In_ LPCWSTR wzFile, _In_ ULONG dwSize, _In_ BOOL bEncrypt, _In_opt_bytecount_(16) const UCHAR* pbDuid, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive, _Out_ PULONG pdwDiskId);
BOOL VerifyPassword(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword);
BOOL ReinitializeTemplateVhd(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bEncrypt);
BOOL MountVhd(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive, _Out_ PULONG pdwDiskId);
BOOL MountVhdEx(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive, _In_ BOOL bFormat, _Out_ PULONG pdwDiskId);
BOOL UnmountVhd(_In_ ULONG dwDiskId);
BOOL Format(LPCWSTR VolumeName);
BOOL ResetEncryptionKey(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword);

class CFmIfs
{
public:
    CFmIfs();
    ~CFmIfs();

    BOOL Format(LPCWSTR DriveRoot, BOOL QuickFormat, ULONG ClusterSize = 4096);

private:
    HMODULE _h;
};

class CVhdInfo
{
public:
    CVhdInfo() : _diskid(-1), _drive(0), _visible(FALSE), _removable(FALSE)
    {
    }
    CVhdInfo(const NXRMVHDINFO& inf);
    virtual ~CVhdInfo()
    {
    }

    inline const std::wstring& GetFile() const throw() {return _file;}
    inline const std::wstring& GetNtName() const throw() {return _nt_name;}
    inline const std::wstring& GetDosName() const throw() {return _dos_name;}
    inline ULONG GetDiskId() const throw() {return _diskid;}
    inline WCHAR GetDriveLetter() const throw() {return _drive;}
    inline BOOL  IsVisible() const throw() {return _visible;}
    inline BOOL  IsRemovable() const throw() {return _removable;}
    inline BOOL  IsValid() const throw() {return ((-1!=_diskid) && !_nt_name.empty());}

    inline void SetFile(const std::wstring& file) throw() {_file = file;}
    inline void SetNtName(const std::wstring& nt_name) throw() {_nt_name = nt_name;}
    inline void SetDosName(const std::wstring& dos_name) throw() {_dos_name = dos_name;}
    inline void SetDiskId(ULONG disk_id) throw() {_diskid = disk_id;}
    inline void SetDriveLetter(WCHAR drive) throw() {_drive = drive;}
    inline void SetVisible(BOOL visible) throw() {_visible = visible;}
    inline void SetRemovable(BOOL removable) throw() {_removable = removable;}

    CVhdInfo& operator = (const CVhdInfo& info) throw()
    {
        if(this != (&info)) {
            _file       = info.GetFile();;
            _nt_name    = info.GetNtName();
            _dos_name   = info.GetDosName();
            _diskid     = info.GetDiskId();
            _drive      = info.GetDriveLetter();
            _visible    = info.IsVisible();
            _removable  = info.IsRemovable();
        }

        return *this;
    }


private:
    std::wstring    _file;
    std::wstring    _nt_name;
    std::wstring    _dos_name;
    ULONG           _diskid;
    WCHAR           _drive;
    BOOL            _visible;
    BOOL            _removable;
};

class CVhdMgr
{
public:
    CVhdMgr();
    virtual ~CVhdMgr();

    inline BOOL IsConnected() const { return (INVALID_HANDLE_VALUE != _h); }

    BOOL Mount(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive,  _Out_ CVhdInfo& vhdi);
    BOOL Unmount(_In_ ULONG id);
    BOOL UnmountAll();
    BOOL Query(_In_ ULONG id, _Out_ CVhdInfo& vhdi);
    BOOL QueryAll(_Out_ std::vector<CVhdInfo>& vhdis);

protected:
    BOOL Connect(_In_ BOOL bReadOnly);
    void Disconnect();
    BOOL InterUnmount(_In_ const CVhdInfo& vhdi);
        
private:
    HANDLE  _h;
};


}

#endif