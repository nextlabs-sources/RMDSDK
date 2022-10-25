
#include <Windows.h>
#include <assert.h>
#include <winternl.h>

#include <nudf\shared\fltdef.h>

#include "nxlfmthlp.hpp"

using namespace NX;
using namespace NX::NXL;


#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef NTSTATUS(WINAPI *ZWQUERYEAFILE) (
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_reads_bytes_opt_(EaListLength) PVOID EaList,
    _In_ ULONG EaListLength,
    _In_opt_ PULONG EaIndex,
    _In_ BOOLEAN RestartScan
    );

typedef NTSTATUS(WINAPI *ZWSETEAFILE) (
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

typedef NTSTATUS(WINAPI *ZWCREATEFILE)(
    _Out_    PHANDLE            FileHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_     POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_    PIO_STATUS_BLOCK   IoStatusBlock,
    _In_opt_ PLARGE_INTEGER     AllocationSize,
    _In_     ULONG              FileAttributes,
    _In_     ULONG              ShareAccess,
    _In_     ULONG              CreateDisposition,
    _In_     ULONG              CreateOptions,
    _In_opt_ PVOID              EaBuffer,
    _In_     ULONG              EaLength
    );

#pragma pack(push, 4)

typedef struct _FILE_GET_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR EaNameLength;
    CHAR EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

#pragma pack(pop)


HRESULT __stdcall RAW::EncryptFile(const WCHAR *FileName)
{
    return RAW::EncryptFileEx(FileName, NULL, NULL, 0);
}
HRESULT __stdcall RAW::EncryptFileEx(const WCHAR *FileName, const char *tag, UCHAR *data, USHORT datalen)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    do
    {
        hFile = CreateFileW(FileName,
            GENERIC_READ | FILE_WRITE_EA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        hr = RAW::EncryptFileEx2(hFile, tag, data, datalen);

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT __stdcall RAW::EncryptFileEx2(HANDLE hFile, const char *tag, UCHAR *data, USHORT datalen)
{
    HRESULT hr = S_OK;

    UCHAR EaBuf[64] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;
    ZWSETEAFILE fn_ZwSetEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");
    fn_ZwSetEaFile = (ZWSETEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwSetEaFile");

    do
    {
        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
								  &IoStatus,
								  EaBuf,
								  EaOutputLength,
								  TRUE,
								  EaBuf,
								  EaInputLength,
								  NULL,
								  TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue == NXRM_CONTENT_IS_ENCRYPTED)
        {
            break;
        }

        memset(EaBuf, 0, sizeof(EaBuf));

        pFullEaInfo->NextEntryOffset = 0;
        pFullEaInfo->Flags = 0;
        pFullEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_ENCRYPT_CONTENT);
        pFullEaInfo->EaValueLength = sizeof(UCHAR);

        memcpy(pFullEaInfo->EaName, NXRM_EA_ENCRYPT_CONTENT, sizeof(NXRM_EA_ENCRYPT_CONTENT));

        *(UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1) = 1;

        EaInputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_ENCRYPT_CONTENT) + sizeof(UCHAR);

        Status = fn_ZwSetEaFile(hFile,
								&IoStatus,
								EaBuf,
								EaInputLength);

        if (Status != STATUS_SUCCESS)
        {
            hr = E_UNEXPECTED;
            break;
        }

        if (IoStatus.Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(IoStatus.Status);
            break;
        }

        //
        // verify the file is encrypted in case driver was stopped between the first QueryEa call and SetEa call
        //
        memset(EaBuf, 0, sizeof(EaBuf));

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
								  &IoStatus,
								  EaBuf,
								  EaOutputLength,
								  TRUE,
								  EaBuf,
								  EaInputLength,
								  NULL,
								  TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

        if (data && datalen && strlen(tag))
        {
            hr = RAW::SyncNXLHeader(hFile, tag, data, datalen);
        }

    } while (FALSE);

    return hr;
}

HRESULT __stdcall RAW::SyncNXLHeader(HANDLE hFile, const char *tag, UCHAR *data, USHORT datalen)
{
    HRESULT hr = S_OK;

    UCHAR EaBuf[64] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;
    ZWSETEAFILE fn_ZwSetEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    UCHAR *pEaData = NULL;
    UCHAR *pEaValue = NULL;

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");
    fn_ZwSetEaFile = (ZWSETEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwSetEaFile");

    do
    {
        //
        // only accept one of three sections
        //
        if (strcmp(tag, NXL_SECTION_NAME_FILEINFO) &&
            strcmp(tag, NXL_SECTION_NAME_FILEPOLICY) &&
            strcmp(tag, NXL_SECTION_NAME_FILETAG))
        {
            hr = E_UNEXPECTED;
            break;
        }

        //
        // step 1: making sure 1) driver is running 2) file is a encrypted NXL file
        //
        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

        //
        // step 2:  Sending command to kernel
        //

        //
        // calc input length
        //
        EaInputLength = (ULONG)(sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_TAG) + strlen(tag) + sizeof('\0') + \
            sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_SYNC_HEADER) + datalen);

        pEaData = (UCHAR*)malloc(EaInputLength);

        if (!pEaData)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        memset(pEaData, 0, EaInputLength);

        //
        // EA 1: TAG
        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)pEaData;

        pFullEaInfo->NextEntryOffset = (ULONG)(sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_TAG) + strlen(tag) + sizeof('\0'));
        pFullEaInfo->Flags = 0;
        pFullEaInfo->EaNameLength = (UCHAR)(strlen(NXRM_EA_TAG));
        pFullEaInfo->EaValueLength = (USHORT)(strlen(tag) + sizeof('\0'));

        memcpy(pFullEaInfo->EaName, NXRM_EA_TAG, sizeof(NXRM_EA_TAG));

        pEaValue = ((UCHAR*)pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

        memcpy(pEaValue, tag, strlen(tag) + sizeof('\0'));

        //
        // EA 2: SYN header
        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)((UCHAR*)pFullEaInfo + pFullEaInfo->NextEntryOffset);

        pFullEaInfo->NextEntryOffset = 0;
        pFullEaInfo->Flags = 0;
        pFullEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_SYNC_HEADER);
        pFullEaInfo->EaValueLength = datalen;

        memcpy(pFullEaInfo->EaName, NXRM_EA_SYNC_HEADER, sizeof(NXRM_EA_SYNC_HEADER));

        pEaValue = ((UCHAR*)pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

        memcpy(pEaValue, data, datalen);

        Status = fn_ZwSetEaFile(hFile,
            &IoStatus,
            pEaData,
            EaInputLength);

        if (Status != STATUS_SUCCESS)
        {
            hr = E_UNEXPECTED;
            break;
        }

        if (IoStatus.Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(IoStatus.Status);
            break;
        }

    } while (FALSE);


    if (pEaData)
    {
        free(pEaData);
        pEaData = NULL;
    }

    return hr;
}

HRESULT __stdcall RAW::ReadTags(const WCHAR *FileName, UCHAR *data, USHORT *datalen)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    do
    {
        hFile = CreateFileW(FileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        hr = RAW::ReadTagsEx(hFile, data, datalen);

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT __stdcall RAW::ReadTagsEx(HANDLE hFile, UCHAR *data, USHORT *datalen)
{
    HRESULT hr = S_OK;

    UCHAR EaBuf[64] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;
    FILE_GET_EA_INFORMATION	 *pQueryTagEa = NULL;

    UCHAR *pEaData = NULL;
    UCHAR *pTagData = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");

    do
    {

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

        pEaData = (UCHAR*)malloc(sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_TAG) + sizeof(NXL_SECTION_NAME_FILETAG) + \
            sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_SYNC_HEADER) + 4096);

        if (!pEaData)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        pQueryTagEa = (FILE_GET_EA_INFORMATION *)pEaData;

        pQueryTagEa->NextEntryOffset = 0;
        pQueryTagEa->EaNameLength = sizeof(NXRM_EA_TAG) - sizeof('\0');

        memcpy(pQueryTagEa->EaName,
            NXRM_EA_TAG,
            sizeof(NXRM_EA_TAG));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_TAG);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_TAG) + sizeof(NXL_SECTION_NAME_FILETAG) + \
            sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_SYNC_HEADER) + 4096;

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            pEaData,
            EaOutputLength,
            TRUE,
            pEaData,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        if (IoStatus.Information < sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_TAG) + sizeof(NXL_SECTION_NAME_FILETAG) + \
            sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_SYNC_HEADER))
        {
            hr = E_UNEXPECTED;
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)pEaData;

        if (pFullEaInfo->NextEntryOffset != (ULONG)(sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_TAG) + sizeof(NXL_SECTION_NAME_FILETAG)) ||
            pFullEaInfo->Flags != 0x80 ||
            pFullEaInfo->EaNameLength != sizeof(NXRM_EA_TAG) - sizeof(char) ||
            pFullEaInfo->EaValueLength != sizeof(NXL_SECTION_NAME_FILETAG))
        {
            hr = E_UNEXPECTED;
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)((UCHAR*)pFullEaInfo + pFullEaInfo->NextEntryOffset);

        if (pFullEaInfo->NextEntryOffset != 0 ||
            pFullEaInfo->Flags != 0x80 ||
            pFullEaInfo->EaNameLength != sizeof(NXRM_EA_SYNC_HEADER) - sizeof(char))
        {
            hr = E_UNEXPECTED;
            break;
        }

        if (*datalen < pFullEaInfo->EaValueLength)
        {
            hr = E_UNEXPECTED;
            break;
        }

        pTagData = ((UCHAR*)pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

        memcpy(data,
            pTagData,
            pFullEaInfo->EaValueLength);

        *datalen = pFullEaInfo->EaValueLength;

    } while (FALSE);

    if (pEaData)
    {
        free(pEaData);
        pEaData = NULL;
    }

    return hr;
}

HRESULT __stdcall RAW::CheckRights(const WCHAR *FileName, ULONGLONG *RightMask, ULONGLONG *CustomRightsMask, ULONGLONG *EvaluationId)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    UCHAR EaBuf[128] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    ULONGLONG *p = NULL;

    do
    {
        //Open file first to make sure it is a file instead of directory.
		hFile = CreateFileW(FileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

		fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");
		
		Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

        //
        //
        //
        memset(EaBuf, 0, sizeof(EaBuf));

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_CHECK_RIGHTS);
        memcpy(pEaInfo->EaName, NXRM_EA_CHECK_RIGHTS, sizeof(NXRM_EA_CHECK_RIGHTS));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_CHECK_RIGHTS);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_CHECK_RIGHTS) + sizeof(ULONGLONG) + sizeof(ULONGLONG) + sizeof(ULONGLONG);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != (sizeof(ULONGLONG) + sizeof(ULONGLONG) + sizeof(ULONGLONG)))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        p = (ULONGLONG*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

        *RightMask = *(p + 0);
        *CustomRightsMask = *(p + 1);
        *EvaluationId = *(p + 2);

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT __stdcall RAW::DecryptFile(const WCHAR *FileName)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    do
    {
		//try open the file exclusively to make sure no other application is opening this file.
		hFile = CreateFileW(FileName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if(INVALID_HANDLE_VALUE == hFile) {
			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;

        hFile = CreateFileW(FileName,
            GENERIC_READ | FILE_WRITE_EA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        hr = RAW::DecryptFile2(hFile);

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;

}

HRESULT __stdcall RAW::DecryptFile2(HANDLE hFile)
{
    HRESULT hr = S_OK;

    UCHAR EaBuf[64] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;
    ZWSETEAFILE fn_ZwSetEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");
    fn_ZwSetEaFile = (ZWSETEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwSetEaFile");

    do
    {
        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            break;
        }

        memset(EaBuf, 0, sizeof(EaBuf));

        pFullEaInfo->NextEntryOffset = 0;
        pFullEaInfo->Flags = 0;
        pFullEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_ENCRYPT_CONTENT);
        pFullEaInfo->EaValueLength = sizeof(UCHAR);

        memcpy(pFullEaInfo->EaName, NXRM_EA_ENCRYPT_CONTENT, sizeof(NXRM_EA_ENCRYPT_CONTENT));

        *(UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1) = 0;

        EaInputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_ENCRYPT_CONTENT) + sizeof(UCHAR);

        Status = fn_ZwSetEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaInputLength);

        if (Status != STATUS_SUCCESS)
        {
            hr = E_UNEXPECTED;
            break;
        }

        if (IoStatus.Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(IoStatus.Status);
            break;
        }

    } while (FALSE);

    return hr;
}

HRESULT __stdcall RAW::IsDecryptedFile(const WCHAR *FileName)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    UCHAR EaBuf[128] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    ULONGLONG *p = NULL;

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");

    do
    {
        hFile = CreateFileW(FileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT __stdcall RAW::IsDecryptedFile2(const WCHAR *FileName)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    UCHAR EaBuf[128] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;
    ZWCREATEFILE fn_ZwCreateFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    ULONGLONG *p = NULL;

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");
    fn_ZwCreateFile = (ZWCREATEFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwCreateFile");

    do
    {
        if (fn_ZwCreateFile)
        {
            OBJECT_ATTRIBUTES ObjectAttributes = { 0 };

            UNICODE_STRING	FileNameString = { 0 };

            std::wstring	tmpFileName;

            tmpFileName += L"\\??\\";
            tmpFileName += FileName;

            FileNameString.Buffer = (WCHAR*)tmpFileName.c_str();
            FileNameString.Length = (USHORT)(tmpFileName.length() * sizeof(WCHAR));
            FileNameString.MaximumLength = (USHORT)(tmpFileName.length() * sizeof(WCHAR));

            InitializeObjectAttributes(&ObjectAttributes,
                &FileNameString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

            Status = fn_ZwCreateFile(&hFile,
                FILE_GENERIC_READ,
                &ObjectAttributes,
                &IoStatus,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0);

            if (Status != STATUS_SUCCESS)
            {
                hFile = INVALID_HANDLE_VALUE;
                hr = HRESULT_FROM_NT(Status);
                break;
            }
        }
        else
        {
            hFile = CreateFileW(FileName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT __stdcall RAW::SetSourceFileName(const WCHAR *FileName, const WCHAR *SrcNTFileName)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    UCHAR EaBuf[4096] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;
    ZWSETEAFILE fn_ZwSetEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    ULONGLONG *p = NULL;

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");
    fn_ZwSetEaFile = (ZWSETEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwSetEaFile");

    do
    {
        if (wcslen(SrcNTFileName)*sizeof(WCHAR) > 2048)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            break;
        }

        hFile = CreateFileW(FileName,
            GENERIC_READ | FILE_WRITE_EA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

        //
        //
        //
        memset(EaBuf, 0, sizeof(EaBuf));

        pFullEaInfo->NextEntryOffset = 0;
        pFullEaInfo->Flags = 0;
        pFullEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_SET_SOURCE);
        pFullEaInfo->EaValueLength = (USHORT)(min(wcslen(SrcNTFileName) * sizeof(WCHAR), 2048));

        memcpy(pFullEaInfo->EaName, NXRM_EA_SET_SOURCE, sizeof(NXRM_EA_SET_SOURCE));

        memcpy((pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1),
            SrcNTFileName,
            min(wcslen(SrcNTFileName) * sizeof(WCHAR), 2048));

        EaInputLength = (ULONG)(sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_SET_SOURCE) + min(wcslen(SrcNTFileName) * sizeof(WCHAR), 2048));

        Status = fn_ZwSetEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaInputLength);

        if (Status != STATUS_SUCCESS)
        {
            hr = E_UNEXPECTED;
            break;
        }

        if (IoStatus.Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(IoStatus.Status);
            break;
        }

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}


HRESULT __stdcall NX::NXL::RAW::CheckRightsNoneCache(const WCHAR *FileName, ULONGLONG *RightMask, ULONGLONG *CustomRightsMask, ULONGLONG *EvaluationId)
{
    HRESULT hr = S_OK;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    UCHAR EaBuf[128] = { 0 };
    ULONG EaInputLength = 0;
    ULONG EaOutputLength = 0;

    UCHAR EaValue = 0;

    FILE_GET_EA_INFORMATION *pEaInfo = NULL;
    FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;

    ZWQUERYEAFILE fn_ZwQueryEaFile = NULL;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK	IoStatus = { 0 };

    ULONGLONG *p = NULL;

    fn_ZwQueryEaFile = (ZWQUERYEAFILE)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryEaFile");

    do
    {
        hFile = CreateFileW(FileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_IS_CONTENT_ENCRYPTED);
        memcpy(pEaInfo->EaName, NXRM_EA_IS_CONTENT_ENCRYPTED, sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != sizeof(UCHAR))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        EaValue = *((UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1));

        if (EaValue != NXRM_CONTENT_IS_ENCRYPTED)
        {
            hr = E_UNEXPECTED;
            break;
        }

        //
        //
        //
        memset(EaBuf, 0, sizeof(EaBuf));

        pEaInfo = (FILE_GET_EA_INFORMATION *)EaBuf;
        pEaInfo->NextEntryOffset = 0;
        pEaInfo->EaNameLength = (UCHAR)strlen(NXRM_EA_CHECK_RIGHTS_NONECACHE);
        memcpy(pEaInfo->EaName, NXRM_EA_CHECK_RIGHTS_NONECACHE, sizeof(NXRM_EA_CHECK_RIGHTS_NONECACHE));

        EaInputLength = sizeof(FILE_GET_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_CHECK_RIGHTS_NONECACHE);
        EaOutputLength = sizeof(FILE_FULL_EA_INFORMATION) - sizeof(CHAR) + sizeof(NXRM_EA_CHECK_RIGHTS_NONECACHE) + sizeof(ULONGLONG) + sizeof(ULONGLONG) + sizeof(ULONGLONG);

        Status = fn_ZwQueryEaFile(hFile,
            &IoStatus,
            EaBuf,
            EaOutputLength,
            TRUE,
            EaBuf,
            EaInputLength,
            NULL,
            TRUE);

        if (Status != STATUS_SUCCESS)
        {
            hr = HRESULT_FROM_NT(Status);
            break;
        }

        pFullEaInfo = (FILE_FULL_EA_INFORMATION *)EaBuf;

        if (pFullEaInfo->EaValueLength != (sizeof(ULONGLONG) + sizeof(ULONGLONG) + sizeof(ULONGLONG)))
        {
            //
            // nxrmflt.sys is not running or rights management service is not running
            //
            hr = E_UNEXPECTED;
            break;
        }

        p = (ULONGLONG*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

        *RightMask = *(p + 0);
        *CustomRightsMask = *(p + 1);
        *EvaluationId = *(p + 2);

    } while (FALSE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}
