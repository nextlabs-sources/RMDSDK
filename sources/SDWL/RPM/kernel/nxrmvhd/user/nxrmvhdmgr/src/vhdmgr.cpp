
#include <Windows.h>
#include <assert.h>
#include <Shlobj.h>
#include <Dbt.h>

#include <boost\algorithm\string.hpp>

#include <nudf\eh.hpp>
#include <nudf\crypto.hpp>
#include <nudf\string.hpp>
#include <nudf\winutil.hpp>

#ifndef ASSERT
#define ASSERT assert
#endif

#include "nxrmvhddef.h"
#include "vhdmgr.hpp"


using namespace VHD;



static BOOL PasswordToKey(_In_opt_ const WCHAR* wzPassword, _Out_writes_bytes_(32) UCHAR* pbKey)
{
    static const UCHAR Salt[32] = {
        0xB0, 0x00, 0x17, 0xCD, 0xFF, 0xC5, 0xA7, 0xB1,
        0xB0, 0x00, 0x17, 0xCD, 0xFF, 0xC5, 0xA7, 0xB1,
        0xB0, 0x00, 0x17, 0xCD, 0xFF, 0xC5, 0xA7, 0xB1,
        0xB0, 0x00, 0x17, 0xCD, 0xFF, 0xC5, 0xA7, 0xB1
    };

    const ULONG msg_size = (NULL != wzPassword && L'\0' != wzPassword[0]) ? ((ULONG)(2 * wcslen(wzPassword))) : 0;

    std::vector<unsigned char> buf;
    buf.resize(sizeof(ULONG) + msg_size, 0);
    memcpy(buf.data(), &msg_size, sizeof(ULONG));
    if (0 != msg_size) {
        memcpy(buf.data() + sizeof(ULONG), wzPassword, msg_size);
    }

    return NX::crypto::hmac_sha256(buf.data(), (ULONG)buf.size(), Salt, 32, pbKey) ? TRUE : FALSE;
}

BOOL VHD::CreateVhd(_In_ LPCWSTR wzFile, _In_ ULONG dwSize, _In_ BOOL bEncrypt, _In_opt_bytecount_(16) const UCHAR* pbDuid, _In_opt_ const WCHAR* wzPassword)
{
    BOOL bRet = FALSE;

    HANDLE h = INVALID_HANDLE_VALUE;


    h = ::CreateFileW(wzFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return FALSE;
    }

    do {

        VHDFILEHEADER   header = { 0 };
        UCHAR           kek[32] = { 0 };
        DWORD           bytes_written = 0;
        std::vector<unsigned char> cekey;

        // set header
        memset(&header, 0, sizeof(header));
        header.Magic = VHDFMT_FILE_MAGIC;
        header.Version = VHDFMT_FILE_VERSION;
        if (NULL == pbDuid) {
        }
        else {
            memcpy(header.UniqueId, pbDuid, 16);
        }
        header.DiskSpace = ((LONGLONG)dwSize) * 0x100000;   // Convert MB to Bytes
        if (NULL != wzPassword && L'\0' != wzPassword[0]) {
            header.Flags |= VHD_FLAG_PASSWORD_ENABLED;
        }

        memset(&kek, 0, sizeof(kek));
        if (!PasswordToKey(wzPassword, kek)) {
            break;
        }

        // Generate CEK
        if (bEncrypt) {
            header.Flags |= VHD_FLAG_ENCRYPTION_ENABLED;
            if (!NX::crypto::aes_key::generate_random_aes_key(header.DiskKey)) {
                break;
            }
        }

        // Copy Unique Id
        memcpy(header.DiskKey + 32, header.UniqueId, 16);

        // Encrypt
        NX::crypto::aes_key kekey;
        kekey.import_key(kek, 32);
        if (!NX::crypto::aes_encrypt(kekey, header.DiskKey, 64, NULL, 0, VHDFMT_SECTOR_SIZE)) {
            break;
        }

        // Allocate
        ::SetFilePointer(h, VHDFMT_BLOCK_SIZE, NULL, FILE_BEGIN);
        if (!::SetEndOfFile(h)) {
            break;
        }
        ::SetFilePointer(h, 0, NULL, FILE_BEGIN);

        // Write File Header
        if (!::WriteFile(h, &header, sizeof(header), &bytes_written, NULL)) {
            break;
        }

        bRet = TRUE;

    } while (FALSE);
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
    if (!bRet) {
        ::DeleteFileW(wzFile);
    }

    return bRet;
}

BOOL VHD::CreateVhdEx(_In_ LPCWSTR wzFile, _In_ ULONG dwSize, _In_ BOOL bEncrypt, _In_opt_bytecount_(16) const UCHAR* pbDuid, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive, _Out_ PULONG pdwDiskId)
{
    if (!CreateVhd(wzFile, dwSize, bEncrypt, pbDuid, wzPassword)) {
        return FALSE;
    }

    return MountVhdEx(wzFile, wzPassword, TRUE, bVisible, wzPreferredDrive, pdwDiskId);
}

BOOL VHD::VerifyPassword(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword)
{
    BOOL bRet = FALSE;
    HANDLE h = INVALID_HANDLE_VALUE;

    do {

        VHDFILEHEADER   header = { 0 };
        DWORD dwBytesRead = 0;

        h = ::CreateFileW(wzFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            break;
        }

        if (!::ReadFile(h, &header, sizeof(header), &dwBytesRead, NULL)) {
            break;
        }
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;

        if (dwBytesRead != (ULONG)sizeof(header)) {
            break;
        }

        if (header.Magic != VHDFMT_FILE_MAGIC || header.Version != VHDFMT_FILE_VERSION) {
            break;
        }

        if (VHD_FLAG_PASSWORD_ENABLED == (header.Flags & VHD_FLAG_PASSWORD_ENABLED)) {
            if (NULL == wzPassword) {
                break;
            }
            if (L'\0' == wzPassword[0]) {
                break;
            }
        }

        try {

            UCHAR kek[32] = { 0 };
            memset(kek, 0, sizeof(kek));
            if (!PasswordToKey(wzPassword, kek)) {
                throw std::exception("fail to generate key");
            }

            NX::crypto::aes_key kekey;
            kekey.import_key(kek, 32);
            if (!NX::crypto::aes_decrypt(kekey, header.DiskKey, 64, NULL, 0, VHDFMT_SECTOR_SIZE)) {
                throw std::exception("fail to decrypt key");
            }

            // Make sure the password is correct
            bRet = (0 == memcmp(header.DiskKey + 32, header.UniqueId, 16));
            if (!bRet) {
                SetLastError(ERROR_WRONG_PASSWORD);
                throw std::exception("wrong key");
            }
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }

    } while (FALSE);
    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return bRet;
}

BOOL ReencryptBlock(HANDLE h, _In_reads_bytes_opt_(32) const UCHAR* old_key, _In_reads_bytes_opt_(32) const UCHAR* new_key, __int64 block_offset)
{
    LARGE_INTEGER to_move = { 0, 0 };
    LARGE_INTEGER new_pos = { 0, 0 };
    std::vector<unsigned char> buf;
    ULONG bytes_read_written = 0;

    if (NULL == old_key && NULL == new_key) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    to_move.QuadPart = block_offset;
    if (!SetFilePointerEx(h, to_move, &new_pos, FILE_BEGIN)) {
        return FALSE;
    }

    buf.resize(VHDFMT_BLOCK_SIZE, 0);
    if (!ReadFile(h, buf.data(), VHDFMT_BLOCK_SIZE, &bytes_read_written, NULL)) {
        return FALSE;
    }
    if (VHDFMT_BLOCK_SIZE != bytes_read_written) {
        return FALSE;
    }

    if (NULL != old_key) {
        NX::crypto::aes_key decrypt_key;
        if (!decrypt_key.import_key(old_key, 32)) {
            return FALSE;
        }
        if (!NX::crypto::aes_decrypt(decrypt_key, buf.data(), VHDFMT_BLOCK_SIZE, NULL, block_offset - VHDFMT_BLOCK_START, VHDFMT_SECTOR_SIZE)) {
            return FALSE;
        }
    }

    if (NULL != new_key) {
        NX::crypto::aes_key encrypt_key;
        if (!encrypt_key.import_key(new_key, 32)) {
            return FALSE;
        }
        if (!NX::crypto::aes_encrypt(encrypt_key, buf.data(), VHDFMT_BLOCK_SIZE, NULL, block_offset - VHDFMT_BLOCK_START, VHDFMT_SECTOR_SIZE)) {
            return FALSE;
        }
    }

    to_move.QuadPart = block_offset;
    if (!SetFilePointerEx(h, to_move, &new_pos, FILE_BEGIN)) {
        return FALSE;
    }
    if (!::WriteFile(h, buf.data(), VHDFMT_BLOCK_SIZE, &bytes_read_written, NULL)) {
        return FALSE;
    }
    if (VHDFMT_BLOCK_SIZE != bytes_read_written) {
        return FALSE;
    }

    return TRUE;
}

BOOL VHD::ReinitializeTemplateVhd(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bEncrypt)
{
    BOOL bRet = FALSE;
    HANDLE h = INVALID_HANDLE_VALUE;

    static const ULONG valid_block_state = BLOCK_STATE_ALLOCED | BLOCK_STATE_INITED;

    do {

        VHDFILEHEADER   header = { 0 };
        DWORD dwBytesRead = 0;

        h = ::CreateFileW(wzFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            break;
        }

        if (!::ReadFile(h, &header, sizeof(header), &dwBytesRead, NULL)) {
            break;
        }

        if (dwBytesRead != (ULONG)sizeof(header)) {
            break;
        }

        if (header.Magic != VHDFMT_FILE_MAGIC || header.Version != VHDFMT_FILE_VERSION) {
            break;
        }

        if (VHD_FLAG_PASSWORD_ENABLED == (header.Flags & VHD_FLAG_PASSWORD_ENABLED)) {
            // template disk never use password
            break;
        }
        if (VHD_FLAG_ENCRYPTION_ENABLED == (header.Flags & VHD_FLAG_ENCRYPTION_ENABLED)) {
            // template disk never encrypt disk
            break;
        }

        UCHAR kek[32] = { 0 };
        UCHAR cek[32] = { 0 };

        memset(kek, 0, sizeof(kek));
        memset(cek, 0, sizeof(cek));
        if (!PasswordToKey(wzPassword, kek)) {
            break;
        }
        if (NULL != wzPassword && 0 != wzPassword[0]) {
            header.Flags |= VHD_FLAG_PASSWORD_ENABLED;
        }

        const NX::win::guid disk_id = NX::win::guid::generate();
        if (disk_id.empty()) {
            break;
        }
        memcpy(header.UniqueId, &disk_id, sizeof(GUID));

        if (bEncrypt) {
            if (!NX::crypto::aes_key::generate_random_aes_key(cek)) {
                break;
            }
            memcpy(header.DiskKey, cek, 32);
            header.Flags |= VHD_FLAG_ENCRYPTION_ENABLED;
        }
        memcpy(header.DiskKey + 32, header.UniqueId, 16);

        NX::crypto::aes_key kekey;
        kekey.import_key(kek, 32);
        if (!NX::crypto::aes_encrypt(kekey, header.DiskKey, 64, NULL, 0, VHDFMT_SECTOR_SIZE)) {
            break;
        }

        // Write header
        if (INVALID_SET_FILE_POINTER == SetFilePointer(h, 0, NULL, FILE_BEGIN)) {
            break;
        }

        DWORD bytes_read_written = 0;
        if (!::WriteFile(h, &header, sizeof(header), &bytes_read_written, NULL)) {
            break;
        }

        // Need to use new CEK to encrypt existing sectors
        if (bEncrypt) {

            // encrypt all sectors
            VHDBAT  vhdbat = { 0 };
            memset(&vhdbat, 0, sizeof(vhdbat));
            if (INVALID_SET_FILE_POINTER == SetFilePointer(h, VHDFMT_BLOCKTABLE_START, NULL, FILE_BEGIN)) {
                goto _error_exit;
            }
            
            if (!::ReadFile(h, &vhdbat, (ULONG)sizeof(VHDBAT), &bytes_read_written, NULL)) {
                goto _error_exit;
            }

            const int total_blocks = (int)(header.DiskSpace / VHDFMT_BLOCK_SIZE);
            for (int i = 0; i < total_blocks; i++) {
                const ULONG block_state = VhdGetBlockState(&vhdbat, i);
                if (valid_block_state == (block_state & valid_block_state)) {
                    const __int64 offset = SeqIdToBlockOffset(VhdGetBlockSeqId(&vhdbat, i));
                    if (!ReencryptBlock(h, NULL, cek, offset)) {
                        goto _error_exit;
                    }
                }
            }
        }

        bRet = TRUE;

    _error_exit:
        ;   // NOTHING
    } while (FALSE);
    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return bRet;
}

BOOL VHD::MountVhd(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive, _Out_ PULONG pdwDiskId)
{
    CVhdMgr manager;
    CVhdInfo vhdi;

    if (!VerifyPassword(wzFile, wzPassword)) {
        return FALSE;
    }

    if (manager.Mount(wzFile, wzPassword, bVisible, wzPreferredDrive, vhdi) && vhdi.IsValid()) {
        *pdwDiskId = vhdi.GetDiskId();
        return TRUE;
    }

    return FALSE;
}

BOOL VHD::MountVhdEx(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive, _In_ BOOL bFormat, _Out_ PULONG pdwDiskId)
{
    CVhdMgr manager;
    CVhdInfo vhdi;

    if (!VerifyPassword(wzFile, wzPassword)) {
        return FALSE;
    }

    if (manager.Mount(wzFile, wzPassword, bVisible, wzPreferredDrive, vhdi) && vhdi.IsValid()) {

        if (bFormat) {
            //if (!manager.Format(vhdi.GetDiskId())) {
            //    manager.Unmount(vhdi.GetDiskId());
            //    return FALSE;
            //}
        }
        *pdwDiskId = vhdi.GetDiskId();
        return TRUE;
    }

    return FALSE;
}

BOOL VHD::UnmountVhd(_In_ ULONG dwDiskId)
{
    CVhdMgr manager;
    return manager.Unmount(dwDiskId);
}

BOOL VHD::Format(LPCWSTR VolumeName)
{
    CFmIfs fmifs;
    return fmifs.Format(VolumeName, TRUE, 4096);
}

static BOOL ReencryptBlocks(HANDLE h, const ULONG max_block_count, const unsigned char* old_key, const unsigned char* new_key)
{
    BOOL    bRet = FALSE;
    VHDBAT  vhd_bat = { 0 };
    ULONG   bytes_read = 0;
    ULONG   bytes_written = 0;

    if (INVALID_SET_FILE_POINTER == ::SetFilePointer(h, VHDFMT_BLOCK_SIZE, NULL, FILE_BEGIN)) {
        return FALSE;
    }

    if (!::ReadFile(h, &vhd_bat, sizeof(VHDBAT), &bytes_read, NULL)) {
        return FALSE;
    }

    try {

        std::vector<unsigned char>  block_data;
        NX::crypto::aes_key old_aes_key;
        NX::crypto::aes_key new_aes_key;
        old_aes_key.import_key(old_key, 32);
        new_aes_key.import_key(new_key, 32);

        block_data.resize(VHDFMT_BLOCK_SIZE, 0);

        for (unsigned long i = 0; i < max_block_count; i++) {

            ULONG state = VhdGetBlockState(&vhd_bat, i);
            const ULONG valid_state = BLOCK_STATE_ALLOCED | BLOCK_STATE_INITED;
            if (valid_state == (state & valid_state)) {

                LARGE_INTEGER offset = { 0 };
                offset.QuadPart = SeqIdToBlockOffset(VhdGetBlockSeqId(&vhd_bat, i));

                if (!::SetFilePointerEx(h, offset, NULL, FILE_BEGIN)) {
                    throw std::exception("fail to move");
                }

                if (!::ReadFile(h, block_data.data(), VHDFMT_BLOCK_SIZE, &bytes_read, NULL)) {
                    throw std::exception("fail to read");
                }
                if(VHDFMT_BLOCK_SIZE != bytes_read) {
                    throw std::exception("fail to read block");
                }

                const ULONGLONG Ivec = offset.QuadPart - VHDFMT_BLOCK_START;

                if(!NX::crypto::aes_decrypt(old_aes_key, block_data.data(), (ULONG)block_data.size(), NULL, Ivec, VHDFMT_SECTOR_SIZE)) {
                    throw std::exception("fail to decrypt");
                }
                if(!NX::crypto::aes_encrypt(new_aes_key, block_data.data(), (ULONG)block_data.size(), NULL, Ivec, VHDFMT_SECTOR_SIZE)) {
                    throw std::exception("fail to encrypt");
                }

                if (!::SetFilePointerEx(h, offset, NULL, FILE_BEGIN)) {
                    throw std::exception("fail to move (write)");
                }

                if (!::WriteFile(h, block_data.data(), VHDFMT_BLOCK_SIZE, &bytes_written, NULL)) {
                    throw std::exception("fail to write");
                }
            }
        }

        bRet = TRUE;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        if (0 == GetLastError()) {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    return bRet;
}

BOOL VHD::ResetEncryptionKey(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword)
{
    BOOL bRet = FALSE;

    HANDLE h = INVALID_HANDLE_VALUE;


    h = ::CreateFileW(wzFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return FALSE;
    }

    do {

        VHDFILEHEADER   header = { 0 };
        UCHAR           kek[32] = { 0 };
        UCHAR           old_cekey[32] = { 0 };
        UCHAR           new_cekey[32] = { 0 };
        DWORD           bytes_read = 0;
        DWORD           bytes_written = 0;

        if (!::ReadFile(h, &header, sizeof(header), &bytes_read, NULL)) {
            break;
        }

        const ULONG max_block_count = (ULONG)((header.DiskSpace + (VHDFMT_BLOCK_SIZE - 1)) / VHDFMT_BLOCK_SIZE);

        // set header
        memset(&header, 0, sizeof(header));
        header.Magic = VHDFMT_FILE_MAGIC;
        header.Version = VHDFMT_FILE_VERSION;

        memset(&kek, 0, sizeof(kek));
        if (!PasswordToKey(wzPassword, kek)) {
            break;
        }

        memset(&old_cekey, 0, sizeof(old_cekey));
        memset(&new_cekey, 0, sizeof(new_cekey));

        // Decrypt CEK
        NX::crypto::aes_key kekey;
        kekey.import_key(kek, 32);
        if (!NX::crypto::aes_decrypt(kekey, header.DiskKey, 64, 0, VHDFMT_SECTOR_SIZE)) {
            break;
        }
        memcpy(old_cekey, header.DiskKey, 32);

        if (!NX::crypto::aes_key::generate_random_aes_key(new_cekey)) {
            break;
        }
        
        // Encrypt
        if(!NX::crypto::aes_encrypt(kekey, header.DiskKey, 64, NULL, 0, VHDFMT_SECTOR_SIZE)) {
            break;
        }

        if (!ReencryptBlocks(h, max_block_count, old_cekey, new_cekey)) {
            break;
        }

        // Write File Header
        ::SetFilePointer(h, 0, NULL, FILE_BEGIN);
        if (!::WriteFile(h, &header, sizeof(header), &bytes_written, NULL)) {
            break;
        }

        bRet = TRUE;

    } while (FALSE);
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
    if (!bRet) {
        ::DeleteFileW(wzFile);
    }

    return bRet;
}


CFmIfs::CFmIfs() : _h(NULL)
{
    _h = ::LoadLibraryW(L"fmifs.dll");
}

CFmIfs::~CFmIfs()
{
    if (NULL != _h) {
        ::FreeLibrary(_h);
        _h = NULL;
    }
}

typedef enum {
    PROGRESS,
    DONEWITHSTRUCTURE,
    UNKNOWN2,
    UNKNOWN3,
    UNKNOWN4,
    UNKNOWN5,
    INSUFFICIENTRIGHTS,
    UNKNOWN7,
    UNKNOWN8,
    UNKNOWN9,
    UNKNOWNA,
    DONE,
    UNKNOWNC,
    UNKNOWND,
    OUTPUT,
    STRUCTUREPROGRESS
} CALLBACKCOMMAND;

typedef struct _TEXTOUTPUT {
    DWORD Lines;
    PCHAR Output;
} TEXTOUTPUT, *PTEXTOUTPUT;

BOOL FormatResult = FALSE;
static BOOLEAN __stdcall FormatExCallback(CALLBACKCOMMAND Command, DWORD Modifier, PVOID Argument)
{
    // 
    // We get other types of commands, but we don't have to pay attention to them
    //
    switch (Command)
    {
    case DONE:
        FormatResult = (FALSE == *((PBOOLEAN)Argument)) ? FALSE : TRUE;
        break;

    case PROGRESS:  // (PDWORD)Argument;
    case OUTPUT:    // (PTEXTOUTPUT)Argument;
    default:
        break;
    }
    return TRUE;
}

BOOL CFmIfs::Format(LPCWSTR DriveRoot, BOOL QuickFormat, ULONG ClusterSize)
{
#define FMIFS_HARDDISK 0xC
#define FMIFS_FLOPPY   0x8


    typedef BOOLEAN(__stdcall *PFMIFSCALLBACK)(CALLBACKCOMMAND Command, DWORD SubAction, PVOID ActionInfo);
    typedef VOID(__stdcall *PFORMATEX)(PWCHAR DriveRoot,
        DWORD MediaFlag,
        PWCHAR Format,
        PWCHAR Label,
        BOOL QuickFormat,
        DWORD ClusterSize,
        PFMIFSCALLBACK Callback);

    if (NULL == _h) {
        return FALSE;
    }
    PFORMATEX FormatEx = (PFORMATEX)GetProcAddress(_h, "FormatEx");
    if (NULL == FormatEx) {
        return FALSE;
    }

    FormatResult = FALSE;
    FormatEx((PWCHAR)DriveRoot, FMIFS_HARDDISK, L"NTFS", L"", QuickFormat, ClusterSize, FormatExCallback);
    return FormatResult;
}

CVhdInfo::CVhdInfo(const NXRMVHDINFO& inf) : _diskid(inf.DiskId), _drive(inf.DriveLetter), _visible(inf.Visible), _removable(inf.Removable), _nt_name(inf.VolumeName)
{
    if (inf.Visible) {
        swprintf_s(NX::string_buffer<wchar_t>(_dos_name, 128), 128, L"%s%C:", DOS_MOUNT_PREFIX, inf.DriveLetter);
    }
}


CVhdMgr::CVhdMgr() : _h(INVALID_HANDLE_VALUE)
{
    Connect(FALSE);
}

CVhdMgr::~CVhdMgr()
{
    if (IsConnected()) {
        Disconnect();
    }
}

BOOL CVhdMgr::Connect(_In_ BOOL bReadOnly)
{
    if(_h != INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    _h = ::CreateFileW(NXRMVHD_WIN32_DEVICE_NAME_W,
                       bReadOnly ? GENERIC_READ : (GENERIC_READ|GENERIC_WRITE),
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_DEVICE,
                       NULL);
    return (INVALID_HANDLE_VALUE == _h) ? FALSE : TRUE;
}

void CVhdMgr::Disconnect()
{
    if(_h != INVALID_HANDLE_VALUE) {
        CloseHandle(_h);
        _h = INVALID_HANDLE_VALUE;
    }
}

BOOL CVhdMgr::Mount(_In_ LPCWSTR wzFile, _In_opt_ const WCHAR* wzPassword, _In_ BOOL bVisible, _In_ WCHAR wzPreferredDrive, _Out_ CVhdInfo& vhdi)
{
    BOOL bRet = FALSE;

    if (!IsConnected()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    do {

        NXRMVHDMOUNTDRIVE   mount = { 0 };
        NXRMVHDINFO         vhdinf = { 0 };
        ULONG               outsize = 0;

        memset(&vhdinf, 0, sizeof(vhdinf));
        memset(&mount, 0, sizeof(mount));
        mount.BytesPerSector = VHDFMT_SECTOR_SIZE;
        mount.Removable = FALSE;
        mount.Visible = bVisible;
        mount.PreferredDriveLetter = wzPreferredDrive;

        if (!PasswordToKey(wzPassword, mount.Key)) {
            break;
        }

        if (!boost::algorithm::istarts_with(wzFile, L"\\??\\")) {
            wcsncpy_s(mount.HostFileName, NXRMVHD_MAX_PATH, L"\\??\\", _TRUNCATE);
            wcsncat_s(mount.HostFileName, NXRMVHD_MAX_PATH, wzFile, _TRUNCATE);
        }
        else {
            wcsncpy_s(mount.HostFileName, NXRMVHD_MAX_PATH, wzFile, _TRUNCATE);
        }

        if (!DeviceIoControl(_h, IOCTL_NXRMVHD_MOUNT_DISK, &mount, sizeof(mount), &vhdinf, sizeof(vhdinf), &outsize, NULL)) {
            break;
        }

        vhdi = CVhdInfo(vhdinf);

        if (mount.Visible && 0 != vhdinf.DriveLetter) {
            ULONG_PTR dwResult = 0;
            DEV_BROADCAST_VOLUME dbv = { 0 };
            WCHAR wzDrivePath[3] = {0, L':', 0};
            wzDrivePath[0] = vhdinf.DriveLetter;
            dbv.dbcv_size = sizeof(DEV_BROADCAST_VOLUME);
            dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
            dbv.dbcv_reserved = 0;
            dbv.dbcv_unitmask = (1 << (vhdinf.DriveLetter - L'A'));
            dbv.dbcv_flags = 0;
            SendMessageTimeoutW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)(&dbv), SMTO_ABORTIFHUNG, 200, (PDWORD_PTR)&dwResult);
            SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, wzDrivePath, NULL);
        }

        bRet = TRUE;

    } while (FALSE);

    return bRet;
}

BOOL CVhdMgr::InterUnmount(_In_ const CVhdInfo& vhdi)
{
    ULONG id = vhdi.GetDiskId();
    ULONG outbuf = 0;
    ULONG outsize = 0;

    if (!DeviceIoControl(_h, IOCTL_NXRMVHD_UNMOUNT_DISK, &id, sizeof(ULONG), &outbuf, sizeof(ULONG), &outsize, NULL)) {
        return FALSE;
    }

    if (vhdi.IsVisible() && 0 != vhdi.GetDriveLetter()) {
        ULONG_PTR dwResult = 0;
        DEV_BROADCAST_VOLUME dbv = { 0 };
        const WCHAR cchDrive = vhdi.GetDriveLetter();
        dbv.dbcv_size = sizeof(DEV_BROADCAST_VOLUME);
        dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
        dbv.dbcv_reserved = 0;
        dbv.dbcv_unitmask = (1 << (cchDrive - L'A'));
        dbv.dbcv_flags = 0;
        SendMessageTimeoutW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVICEREMOVECOMPLETE, (LPARAM)(&dbv), SMTO_ABORTIFHUNG, 200, (PDWORD_PTR)&dwResult);
        WCHAR wzDrive[3] = { 0, L':', 0 };
        wzDrive[0] = cchDrive;
        SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATH, wzDrive, NULL);
    }

    return TRUE;
}

BOOL CVhdMgr::Unmount(_In_ ULONG id)
{
    BOOL bRet = FALSE;

    if (!IsConnected()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (-1 == id) {
        return UnmountAll();
    }

    do {

        CVhdInfo    vhdi;
        ULONG       outbuf = 0;
        ULONG       outsize = 0;

        if (!Query(id, vhdi)) {
            if (0 == GetLastError()) {
                SetLastError(ERROR_NOT_FOUND);
            }
            break;
        }

        bRet = InterUnmount(vhdi);

    } while (FALSE);

    return bRet;
}

BOOL CVhdMgr::UnmountAll()
{
    if (!IsConnected()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    std::vector<CVhdInfo> vhdis;

    if (!QueryAll(vhdis)) {
        return FALSE;
    }

    if (vhdis.empty()) {
        return TRUE;
    }

    BOOL bRet = TRUE;

    std::for_each(vhdis.begin(), vhdis.end(), [&](const CVhdInfo& inf) {
        if (!InterUnmount(inf)) {
            bRet = FALSE;
        }
    });

    return bRet;
}

BOOL CVhdMgr::Query(_In_ ULONG id, _Out_ CVhdInfo& vhdi)
{
    BOOL bRet = FALSE;

    if (!IsConnected()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    do {

        NXRMVHDINFO vhdinf = { 0 };
        ULONG       outsize = 0;

        memset(&vhdinf, 0, sizeof(vhdinf));

        if (!DeviceIoControl(_h, IOCTL_NXRMVHD_QUERY_DISK, &id, sizeof(ULONG), &vhdinf, sizeof(vhdinf), &outsize, NULL)) {
            break;
        }
        if (outsize == 0) {
            if (0 == GetLastError()) {
                SetLastError(ERROR_NOT_FOUND);
            }
            break;
        }

        vhdi = CVhdInfo(vhdinf);
        bRet = TRUE;

    } while (FALSE);

    return bRet;
}

BOOL CVhdMgr::QueryAll(_Out_ std::vector<CVhdInfo>& vhdis)
{
    BOOL bRet = FALSE;

    if (!IsConnected()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    vhdis.clear();

    do {

        ULONG   id = -1;
        ULONG   retsize = 0;
        ULONG   count = 0;
        std::vector<UCHAR>  buf;
        ULONG               outsize = 0;
        NXRMVHDINFOS*       vhdinfs = NULL;

        if (!DeviceIoControl(_h, IOCTL_NXRMVHD_QUERY_DISK, &id, sizeof(ULONG), &count, sizeof(ULONG), &retsize, NULL)) {
            break;
        }

        if (0 == count) {
            bRet = TRUE;
            break;
        }

        outsize = sizeof(NXRMVHDINFOS) + sizeof(NXRMVHDINFO)*(count - 1);
        buf.resize(outsize, 0);
        vhdinfs = (NXRMVHDINFOS*)(&buf[0]);
        if (!DeviceIoControl(_h, IOCTL_NXRMVHD_QUERY_DISK, &id, sizeof(ULONG), vhdinfs, outsize, &retsize, NULL)) {
            break;
        }

        for (int i = 0; i < (int)vhdinfs->Count; i++) {
            vhdis.push_back(CVhdInfo(vhdinfs->Infs[i]));
        }

        bRet = TRUE;

    } while (FALSE);

    return bRet;
}