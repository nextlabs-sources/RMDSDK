#include "pch.h"
#include "madCHook.h"
#include "util.hpp"
#include "exported_interface.h"
#include "file_api_signature.h"
#include "Global.h"





CreateFileW_Signature             Hooked_CreateFileW_Next = NULL;
DeleteFileW_Signature             Hooked_DeleteFileW_Next = NULL;
MoveFileExW_Signature             Hooked_MoveFileExW_Next = NULL;
CopyFileExW_Signature             Hooked_CopyFileExW_Next = NULL;
ReplaceFileW_Signature            Hooked_ReplaceFileW_Next = NULL;
CreateDirectoryW_Signature        Hooked_CreateDirectoryW_Next = NULL;
SetFileAttributesW_Signature      Hooked_SetFileAttributesW_Next = NULL;
GetFileAttributesW_Signature      Hooked_GetFileAttributesW_Next = NULL;
StgCreateStorageEx_Signature      Hooked_StgCreateStorageEx_Next = NULL;
// lower level
NtCreateFile_Signature            Hooked_NtCreateFile_Next = NULL;
NtOpenFile_Signature              Hooked_NtOpenFile_Next = NULL;

// logic and business driven layer
namespace {


    inline void fv_breakpoint() {
        if (global.is_allow_debug_breakpoint) {
            ::DebugBreak();
        }
    }


    // true -> p has been detoured
    inline bool DetourPath(std::wstring& p) {
        CRwSharedLocker l(&global.fv_sw_lock);
        for (auto i : global.fvs) {
            if (((fv*)i)->on_event_detour(p)) {
                return true;
            }
        }
        return false;
    }
}

HANDLE WINAPI Hooked_CreateFileW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
) {
    DEVLOG_FUN;
    HANDLE rt = INVALID_HANDLE_VALUE;
    if (!lpFileName) {
        return Hooked_CreateFileW_Next(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }
    

    if (global.rc.is_disabled()) {
        return Hooked_CreateFileW_Next(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }
    recursion_control_auto auto_disable(global.rc);
    
    std::wstring p = lpFileName;
    DetourPath(p);

#ifdef _DEBUG
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG

    rt = Hooked_CreateFileW_Next(p.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    //rt = Hooked_CreateFileW_Next(p.c_str(), dwDesiredAccess, dwAllShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

#ifdef _DEBUG
    if (rt == INVALID_HANDLE_VALUE) {
        fv_breakpoint();
    }
#endif // _DEBUG

    return rt;
}

BOOL WINAPI Hooked_DeleteFileW(_In_ LPCWSTR lpFileName) {
    DEVLOG_FUN;
    if (!lpFileName) {
        return Hooked_DeleteFileW_Next(lpFileName);
    }

    if (global.rc.is_disabled()) {
        return Hooked_DeleteFileW_Next(lpFileName);
    }
    recursion_control_auto auto_disable(global.rc);
    std::wstring p = lpFileName;
    DetourPath(p);
#ifdef _DEBUG
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG

    return Hooked_DeleteFileW_Next(p.c_str());
}

BOOL WINAPI Hooked_MoveFileExW(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_     DWORD    dwFlags
) {
    DEVLOG_FUN;
    BOOL rt = false;
    if (!lpExistingFileName) {
        return Hooked_MoveFileExW_Next(lpExistingFileName, lpNewFileName, dwFlags);
    }
    if (!lpNewFileName) {
        return Hooked_MoveFileExW_Next(lpExistingFileName, lpNewFileName, dwFlags);
    }


    if (global.rc.is_disabled()) {
        return Hooked_MoveFileExW_Next(lpExistingFileName, lpNewFileName, dwFlags);
    }
    recursion_control_auto auto_disable(global.rc);

    std::wstring pE = lpExistingFileName;
    DetourPath(pE);
    std::wstring pN = lpNewFileName;
    DetourPath(pN);

#ifdef _DEBUG
    if (iend_with(pE, L"asd") || iend_with(pN, L"asd") ) {
        fv_breakpoint();
    }
#endif // _DEBUG

    rt= Hooked_MoveFileExW_Next(pE.c_str(), pN.c_str(), dwFlags);
    return rt;
}

BOOL WINAPI Hooked_CopyFileExW(
    _In_        LPCWSTR lpExistingFileName,
    _In_        LPCWSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
) {
    DEVLOG_FUN;
    BOOL rt = false;
    if (!lpExistingFileName) {
        return Hooked_CopyFileExW_Next(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    }
    if (!lpNewFileName) {
        return Hooked_CopyFileExW_Next(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    }

    if (global.rc.is_disabled()) {
        return Hooked_CopyFileExW_Next(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    }
    recursion_control_auto auto_disable(global.rc);

    std::wstring pE = lpExistingFileName;
    DetourPath(pE);
    std::wstring pN = lpNewFileName;
    DetourPath(pN);

#ifdef _DEBUG
    if (iend_with(pE, L"asd") || iend_with(pN, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG

    rt= Hooked_CopyFileExW_Next(pE.c_str(), pN.c_str(), lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    return rt;
}

BOOL WINAPI Hooked_ReplaceFileW(
    _In_       LPCWSTR lpReplacedFileName,
    _In_       LPCWSTR lpReplacementFileName,
    _In_opt_   LPCWSTR lpBackupFileName,
    _In_       DWORD    dwReplaceFlags,
    _Reserved_ LPVOID   lpExclude,
    _Reserved_ LPVOID  lpReserved
) {
    DEVLOG_FUN;
    BOOL rt = false;
    if (!lpReplacedFileName) {
        return Hooked_ReplaceFileW_Next(lpReplacedFileName, lpReplacementFileName, lpBackupFileName, dwReplaceFlags, lpExclude, lpReserved);
    }
    if (!lpReplacementFileName) {
        return Hooked_ReplaceFileW_Next(lpReplacedFileName, lpReplacementFileName, lpBackupFileName, dwReplaceFlags, lpExclude, lpReserved);
    }
    if (global.rc.is_disabled()) {
        return Hooked_ReplaceFileW_Next(lpReplacedFileName, lpReplacementFileName, lpBackupFileName, dwReplaceFlags, lpExclude, lpReserved);
    }
    recursion_control_auto auto_disable(global.rc);

    std::wstring p1 = lpReplacedFileName;
    DetourPath(p1);
    std::wstring p2 = lpReplacementFileName;
    DetourPath(p2);
    std::wstring p3;
    if (lpBackupFileName) {
        p3 = lpBackupFileName;
    }
    DetourPath(p3);

#ifdef _DEBUG
    if (iend_with(p1, L"asd") || iend_with(p2, L"asd") || iend_with(p3, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG


    rt = Hooked_ReplaceFileW_Next(p1.c_str(), p2.c_str(), p3.c_str(), dwReplaceFlags, lpExclude, lpReserved);
    return rt;

}

BOOL WINAPI Hooked_CreateDirectoryW(
    _In_ LPCWSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
) {
    DEVLOG_FUN;
    BOOL rt = false;
    if (!lpPathName) {
        return Hooked_CreateDirectoryW_Next(lpPathName, lpSecurityAttributes);
    }

    if (global.rc.is_disabled()) {
        return Hooked_CreateDirectoryW_Next(lpPathName, lpSecurityAttributes);
    }
    recursion_control_auto auto_disable(global.rc);
    std::wstring p = lpPathName;
    DetourPath(p);
#ifdef _DEBUG
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG
    rt= Hooked_CreateDirectoryW_Next(p.c_str(), lpSecurityAttributes);
    return rt;
}

BOOL WINAPI  Hooked_SetFileAttributesW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwFileAttributes
) {
    DEVLOG_FUN;
    BOOL rt = false;
    if (!lpFileName) {
        return Hooked_SetFileAttributesW_Next(lpFileName, dwFileAttributes);
    }
    if (global.rc.is_disabled()) {
        return Hooked_SetFileAttributesW_Next(lpFileName, dwFileAttributes);
    }

    recursion_control_auto auto_disable(global.rc);
    std::wstring p = lpFileName;
    DetourPath(p);
#ifdef _DEBUG
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG
    rt= Hooked_SetFileAttributesW_Next(p.c_str(), dwFileAttributes);
    return rt;
}

DWORD WINAPI  Hooked_GetFileAttributesW(
    _In_ LPCWSTR lpFileName)
{
    DEVLOG_FUN;
    DWORD rt = 0;
    if (!lpFileName) {
        return Hooked_GetFileAttributesW_Next(lpFileName);
    }
    if (global.rc.is_disabled()) {
        return Hooked_GetFileAttributesW_Next(lpFileName);
    }

    recursion_control_auto auto_disable(global.rc);
    std::wstring p = lpFileName;
    DetourPath(p);
#ifdef _DEBUG
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG
    rt= Hooked_GetFileAttributesW_Next(p.c_str());
    return rt;
}

HRESULT __stdcall Hooked_StgCreateStorageEx(
    const WCHAR* pwcsName,
    DWORD                grfMode,
    DWORD                stgfmt,
    DWORD                grfAttrs,
    STGOPTIONS* pStgOptions,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    REFIID               riid,
    void** ppObjectOpen
)
{
    DEVLOG_FUN;
    HRESULT rt = ERROR_SUCCESS;
    if (!pwcsName) {
        return Hooked_StgCreateStorageEx_Next(pwcsName, grfMode, stgfmt, grfAttrs, pStgOptions, pSecurityDescriptor, riid, ppObjectOpen);
    }

    if (global.rc.is_disabled()) {
        return Hooked_StgCreateStorageEx_Next(pwcsName, grfMode, stgfmt, grfAttrs, pStgOptions, pSecurityDescriptor, riid, ppObjectOpen);
    }
    recursion_control_auto auto_disable(global.rc);
    std::wstring p = pwcsName;
    DetourPath(p);
#ifdef _DEBUG
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG
    rt = Hooked_StgCreateStorageEx_Next(p.c_str(), grfMode, stgfmt, grfAttrs, pStgOptions, pSecurityDescriptor, riid, ppObjectOpen);
    return rt;
}


NTSTATUS NTAPI Hooked_NtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
) 
{
    if (global.rc.is_disabled()) {
        return Hooked_NtOpenFile_Next(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    }
    recursion_control_auto auto_disable(global.rc);
    DEVLOG_FUN;
    std::wstring p(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length);
#ifdef _DEBUG
    DEVLOG(p.c_str()); DEVLOG(L"\n");
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG
    return Hooked_NtOpenFile_Next(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}



NTSTATUS NTAPI Hooked_NtCreateFile(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength
) 
{
    if (global.rc.is_disabled()) {
        return Hooked_NtCreateFile_Next(FileHandle, DesiredAccess,
            ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    }
    recursion_control_auto auto_disable(global.rc);
    DEVLOG_FUN;
    std::wstring p(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length);
#ifdef _DEBUG
    DEVLOG(p.c_str()); DEVLOG(L"\n");
    if (iend_with(p, L"asd")) {
        fv_breakpoint();
    }
#endif // _DEBUG
    return Hooked_NtCreateFile_Next(FileHandle, DesiredAccess, 
        ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}


void hook_api_for_fv() {
    HookAPI("KERNELBASE", "CreateFileW", (PVOID)Hooked_CreateFileW, (PVOID*)&Hooked_CreateFileW_Next);
    HookAPI("KERNELBASE", "DeleteFileW", (PVOID)Hooked_DeleteFileW, (PVOID*)&Hooked_DeleteFileW_Next);
    HookAPI("KERNELBASE", "MoveFileExW", (PVOID)Hooked_MoveFileExW, (PVOID*)&Hooked_MoveFileExW_Next);
    HookAPI("KERNELBASE", "CopyFileExW", (PVOID)Hooked_CopyFileExW, (PVOID*)&Hooked_CopyFileExW_Next);
    HookAPI("KERNELBASE", "ReplaceFileW", (PVOID)Hooked_ReplaceFileW, (PVOID*)&Hooked_ReplaceFileW_Next);
    HookAPI("KERNELBASE", "CreateDirectoryW", (PVOID)Hooked_CreateDirectoryW, (PVOID*)&Hooked_CreateDirectoryW_Next);
    HookAPI("KERNELBASE", "SetFileAttributesW", (PVOID)Hooked_SetFileAttributesW, (PVOID*)&Hooked_SetFileAttributesW_Next);
    HookAPI("KERNELBASE", "GetFileAttributesW", (PVOID)Hooked_GetFileAttributesW, (PVOID*)&Hooked_GetFileAttributesW_Next);
    HookAPI("coml2", "StgCreateStorageEx", (PVOID)Hooked_StgCreateStorageEx, (PVOID*)&Hooked_StgCreateStorageEx_Next);

    // very low level, only for log and analize purporse
    HookAPI("ntdll", "NtCreateFile", Hooked_NtCreateFile, (void**)&Hooked_NtCreateFile_Next);
    HookAPI("ntdll", "NtOpenFile", Hooked_NtOpenFile, (void**)&Hooked_NtOpenFile_Next);
}
