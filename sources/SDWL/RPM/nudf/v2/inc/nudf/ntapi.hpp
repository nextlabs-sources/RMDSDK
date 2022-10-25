#pragma once
#ifndef __NTFILE_ACCESS_HPP__
#define __NTFILE_ACCESS_HPP__

#include <Winternl.h>
#include <string>


typedef LONG NTSTATUS;

namespace NT {


DWORD StatusToWin32Error(NTSTATUS status);

HANDLE CreateFile(_In_ LPCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ LPSECURITY_ATTRIBUTES SecurityAttributes,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ ULONG* Information
    );

BOOL CreateDirectory(_In_ LPCWSTR DirectoryName,
    _In_opt_ LPSECURITY_ATTRIBUTES SecurityAttributes,
    _In_ BOOL Hidden,
    _Out_opt_ PBOOL Existing);

BOOL CloseHandle(HANDLE Filehandle);

BOOL DeleteFile(_In_ LPCWSTR FileName);

BOOL ReadFile(_In_ HANDLE Filehandle, _Out_ PVOID Buffer, _In_ ULONG Length, _Out_ PULONG BytesRead, _In_ PLARGE_INTEGER ByteOffset);
BOOL WriteFile(_In_ HANDLE Filehandle, _In_ PVOID Buffer, _In_ ULONG Length, _Out_ PULONG BytesWritten, _In_ PLARGE_INTEGER ByteOffset);
BOOL MoveFile(_In_ LPCWSTR Source, _In_ LPCWSTR Target, _In_ BOOL ReplaceIfExists);

BOOL GetCurrentOffset(_In_ HANDLE FileHandle, _Out_ PLARGE_INTEGER CurrentOffset);
BOOL SetCurrentOffset(_In_ HANDLE FileHandle, _In_ PLARGE_INTEGER CurrentOffset);

BOOL GetFileSize(_In_ HANDLE FileHandle, _Out_ PLARGE_INTEGER FileSize);
BOOL GetFileSizeEx(_In_ LPCWSTR FileName, _Out_ PLARGE_INTEGER FileSize);
BOOL SetFileSize(_In_ HANDLE FileHandle, _In_ PLARGE_INTEGER FileSize);

DWORD GetFileAttributes(_In_ LPCWSTR FileName);
BOOL SetFileAttributes(_In_ LPCWSTR FileName, _In_ DWORD FileAttributes);


}



#endif