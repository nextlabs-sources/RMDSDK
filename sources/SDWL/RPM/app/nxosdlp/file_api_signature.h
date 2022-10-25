#pragma once

#include <Windows.h>
#include <winternl.h>
//
//  File API
//
typedef HANDLE(WINAPI* CreateFileW_Signature)(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
    );

typedef BOOL(WINAPI* DeleteFileW_Signature)(_In_ LPCWSTR lpFileName);

typedef BOOL(WINAPI* MoveFileExW_Signature)(_In_ LPCWSTR lpExistingFileName, _In_opt_ LPCWSTR lpNewFileName, _In_     DWORD    dwFlags);

typedef BOOL(WINAPI* CopyFileExW_Signature)(
    _In_        LPCWSTR lpExistingFileName,
    _In_        LPCWSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
    );

typedef BOOL (WINAPI* ReplaceFileW_Signature)(
    _In_       LPCWSTR lpReplacedFileName,
    _In_       LPCWSTR lpReplacementFileName,
    _In_opt_   LPCWSTR lpBackupFileName,
    _In_       DWORD    dwReplaceFlags,
    _Reserved_ LPVOID   lpExclude,
    _Reserved_ LPVOID  lpReserved
);

typedef BOOL(WINAPI* CreateDirectoryW_Signature)(_In_ LPCWSTR lpPathName, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes);

typedef BOOL (WINAPI*  RemoveDirectoryW_Signature)(_In_ LPCWSTR lpPathName);

typedef BOOL(WINAPI* SetFileAttributesW_Signature)(_In_ LPCWSTR lpFileName, _In_ DWORD dwFileAttributes);

typedef DWORD(WINAPI* GetFileAttributesW_Signature)(_In_ LPCWSTR lpFileName);

//
//  Security
//
typedef BOOL (WINAPI* SetFileSecurityW_Signature)( _In_ LPCWSTR lpFileName,_In_ SECURITY_INFORMATION SecurityInformation,_In_ PSECURITY_DESCRIPTOR pSecurityDescriptor);

typedef BOOL (WINAPI* GetFileSecurityW_Signature)(_In_ LPCWSTR lpFileName,_In_ SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor,_In_ DWORD nLength,_Out_ LPDWORD lpnLengthNeeded);





//
//  COM
//

// other more higher interface that will indirectly call File_Interface in Win32
typedef HRESULT(__stdcall* StgCreateStorageEx_Signature)(
    const WCHAR* pwcsName,
    DWORD                grfMode,
    DWORD                stgfmt,
    DWORD                grfAttrs,
    STGOPTIONS* pStgOptions,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    REFIID               riid,
    void** ppObjectOpen
    );



//
//  NT DLL,  very low level
//

typedef NTSTATUS(NTAPI* NtCreateFile_Signature)(
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
    );

typedef NTSTATUS(NTAPI* NtOpenFile_Signature)(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );