

#include <Windows.h>

#include <vector>

#include <nudf\ntapi.hpp>




#define STATUS_NOT_IMPLEMENTED      ((NTSTATUS)0xC0000002L)


namespace {

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG         NumberOfLinks;
    BOOLEAN       DeletePending;
    BOOLEAN       Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG         FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION {
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;


typedef NTSTATUS(__stdcall* NTCREATEFILE)(
    _Out_    PHANDLE            FileHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_     POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_    PIO_STATUS_BLOCK   IoStatusBlock,
    _In_opt_ PLARGE_INTEGER     AllocationSize,
    _In_     ULONG              FileAttributes,
    _In_     ULONG              ShareAccess,
    _In_     ULONG              CreateDisposition,
    _In_     ULONG              CreateOptions,
    _In_     PVOID              EaBuffer,
    _In_     ULONG              EaLength
    );

typedef NTSTATUS(__stdcall* NTREADFILE)(
    _In_     HANDLE           FileHandle,
    _In_opt_ HANDLE           Event,
    _In_opt_ PIO_APC_ROUTINE  ApcRoutine,
    _In_opt_ PVOID            ApcContext,
    _Out_    PIO_STATUS_BLOCK IoStatusBlock,
    _Out_    PVOID            Buffer,
    _In_     ULONG            Length,
    _In_opt_ PLARGE_INTEGER   ByteOffset,
    _In_opt_ PULONG           Key
    );

typedef NTSTATUS(__stdcall* NTWRITEFILE)(
    _In_     HANDLE           FileHandle,
    _In_opt_ HANDLE           Event,
    _In_opt_ PIO_APC_ROUTINE  ApcRoutine,
    _In_opt_ PVOID            ApcContext,
    _Out_    PIO_STATUS_BLOCK IoStatusBlock,
    _In_     PVOID            Buffer,
    _In_     ULONG            Length,
    _In_opt_ PLARGE_INTEGER   ByteOffset,
    _In_opt_ PULONG           Key
    );

typedef NTSTATUS(__stdcall* NTCLOSE)(
    _In_ HANDLE Handle
    );

typedef NTSTATUS(__stdcall* NTQUERYATTRIBUTESFILE)(
    _In_  POBJECT_ATTRIBUTES      ObjectAttributes,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    );

typedef NTSTATUS(__stdcall* NTQUERYINFORMATIONFILE)(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       IoStatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG                  Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass
    );

typedef NTSTATUS(__stdcall* NTSETINFORMATIONFILE)(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       IoStatusBlock,
    _In_  PVOID                  FileInformation,
    _In_  ULONG                  Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass
    );


class ntdll
{
public:
    ntdll() : NtCreateFile(NULL),
              NtClose(NULL),
              NtReadFile(NULL),
              NtWriteFile(NULL),
              NtQueryAttributesFile(NULL),
              NtQueryInformationFile(NULL),
              NtSetInformationFile(NULL)
    {
        initialize();
    }

    ~ntdll()
    {
    }

    void initialize()
    {
        static HMODULE h = NULL;
        if (NULL == h) {
            h = ::GetModuleHandleW(L"ntdll.dll");
            if (NULL == h) {
                return;
            }
            NtCreateFile = (NTCREATEFILE)::GetProcAddress(h, "NtCreateFile");
            NtClose = (NTCLOSE)::GetProcAddress(h, "NtClose");
            NtReadFile = (NTREADFILE)::GetProcAddress(h, "NtReadFile");
            NtWriteFile = (NTWRITEFILE)::GetProcAddress(h, "NtWriteFile");
            NtQueryAttributesFile = (NTQUERYATTRIBUTESFILE)::GetProcAddress(h, "NtQueryAttributesFile");
            NtQueryInformationFile = (NTQUERYINFORMATIONFILE)::GetProcAddress(h, "NtQueryInformationFile");
            NtSetInformationFile = (NTSETINFORMATIONFILE)::GetProcAddress(h, "NtSetInformationFile");
        }
    }

    NTCREATEFILE            NtCreateFile;
    NTCLOSE                 NtClose;
    NTREADFILE              NtReadFile;
    NTWRITEFILE             NtWriteFile;
    NTQUERYATTRIBUTESFILE   NtQueryAttributesFile;
    NTQUERYINFORMATIONFILE  NtQueryInformationFile;
    NTSETINFORMATIONFILE    NtSetInformationFile;
};

}

static ntdll    _ntdll;

DWORD NT::StatusToWin32Error(NTSTATUS status)
{
    DWORD oldError;
    DWORD result;
    DWORD br;
    OVERLAPPED o;

    o.Internal = status;
    o.InternalHigh = 0;
    o.Offset = 0;
    o.OffsetHigh = 0;
    o.hEvent = 0;
    oldError = GetLastError();
    GetOverlappedResult(NULL, &o, &br, FALSE);
    result = GetLastError();
    SetLastError(oldError);
    return result;
}

HANDLE NT::CreateFile(_In_ LPCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ LPSECURITY_ATTRIBUTES SecurityAttributes,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ ULONG* Information
    )
{
    NTSTATUS status = 0;
    HANDLE FileHandle = NULL;

    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
    UNICODE_STRING UnicodeFileName = { 0 };
    IO_STATUS_BLOCK IoStatusBlock = { 0 };

    if (NULL == _ntdll.NtCreateFile || NULL == _ntdll.NtClose) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return NULL;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    UnicodeFileName.MaximumLength = (unsigned short)(wcslen(FileName) * 2);
    UnicodeFileName.Length = UnicodeFileName.MaximumLength;
    UnicodeFileName.Buffer = (PWSTR)FileName;
    InitializeObjectAttributes(&ObjectAttributes, &UnicodeFileName, OBJ_CASE_INSENSITIVE, NULL, SecurityAttributes);

    status = _ntdll.NtCreateFile(&FileHandle,
        DesiredAccess | SYNCHRONIZE,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions | FILE_SYNCHRONOUS_IO_ALERT,
        NULL,
        0);
    if (0 != status) {
        FileHandle = NULL;
        SetLastError(StatusToWin32Error(status));
    }

    if (NULL != Information) {
        *Information = (ULONG)IoStatusBlock.Information;
    }

    return FileHandle;
}

BOOL NT::CreateDirectory(_In_ LPCWSTR DirectoryName,
    _In_opt_ LPSECURITY_ATTRIBUTES SecurityAttributes,
    _In_ BOOL Hidden,
    _Out_opt_ PBOOL Existing)
{
    ULONG Information = 0;
    ULONG FileAttributes = FILE_ATTRIBUTE_DIRECTORY;

    if (Hidden) {
        FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
    }

    HANDLE FileHandle = NT::CreateFile(DirectoryName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_LIST_DIRECTORY | FILE_TRAVERSE | DELETE,
        SecurityAttributes,
        FileAttributes,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_DIRECTORY_FILE,
        &Information
        );

    if (NULL == FileHandle) {
        return FALSE;
    }

    NT::CloseHandle(FileHandle);
    FileHandle = NULL;

    if (NULL != Existing) {
        *Existing = (FILE_OPENED == Information) ? TRUE : FALSE;
    }
    return TRUE;
}

BOOL NT::CloseHandle(HANDLE Filehandle)
{
    if (NULL == _ntdll.NtCreateFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return NULL;
    }

    NTSTATUS status = _ntdll.NtClose(Filehandle);
    if (0 != status) {
        SetLastError(StatusToWin32Error(status));
        return FALSE;
    }

    return TRUE;
}

BOOL NT::DeleteFile(_In_ LPCWSTR FileName)
{
    HANDLE FileHandle = NULL;

    FileHandle = NT::CreateFile(FileName, FILE_GENERIC_WRITE | DELETE, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_DELETE_ON_CLOSE, NULL);
    if (NULL == FileHandle) {
        return FALSE;
    }

    NT::CloseHandle(FileHandle);
    FileHandle = NULL;
    return TRUE;
}

BOOL NT::ReadFile(_In_ HANDLE Filehandle, _Out_ PVOID Buffer, _In_ ULONG Length, _Out_ PULONG BytesRead, _In_ PLARGE_INTEGER ByteOffset)
{
    IO_STATUS_BLOCK IoStatusBlock = { 0 };


    if (NULL == _ntdll.NtReadFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    NTSTATUS status = _ntdll.NtReadFile(Filehandle, NULL, NULL, NULL, &IoStatusBlock, Buffer, Length, ByteOffset, NULL);
    if (0 != status) {
        *BytesRead = 0;
        SetLastError(StatusToWin32Error(status));
        return FALSE;
    }

    *BytesRead = (ULONG)IoStatusBlock.Information;
    return TRUE;
}

BOOL NT::WriteFile(_In_ HANDLE Filehandle, _In_ PVOID Buffer, _In_ ULONG Length, _Out_ PULONG BytesWritten, _In_ PLARGE_INTEGER ByteOffset)
{
    IO_STATUS_BLOCK IoStatusBlock = { 0 };

    if (NULL == _ntdll.NtWriteFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    NTSTATUS status = _ntdll.NtWriteFile(Filehandle, NULL, NULL, NULL, &IoStatusBlock, Buffer, Length, ByteOffset, NULL);
    if (0 != status) {
        *BytesWritten = 0;
        SetLastError(StatusToWin32Error(status));
        return FALSE;
    }

    *BytesWritten = (ULONG)IoStatusBlock.Information;
    return TRUE;
}

BOOL NT::MoveFile(_In_ LPCWSTR Source, _In_ LPCWSTR Target, _In_ BOOL ReplaceIfExists)
{
    HANDLE FileHandle = NULL;
    FILE_RENAME_INFORMATION* RenameInfo = NULL;
    ULONG RenameInfoLength = 0;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    std::vector<unsigned char> buf;

    if (0 == _wcsicmp(Source, Target)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (NULL == _ntdll.NtSetInformationFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    FileHandle = NT::CreateFile(Source, DELETE, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, 0, NULL);
    if (NULL == FileHandle) {
        return FALSE;
    }

    RenameInfoLength = (ULONG)(sizeof(FILE_RENAME_INFORMATION) + wcslen(Target) * sizeof(WCHAR));
    buf.resize(RenameInfoLength, 0);
    RenameInfo = (FILE_RENAME_INFORMATION*)buf.data();
    RenameInfo->ReplaceIfExists = ReplaceIfExists ? TRUE : FALSE;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = (ULONG)(wcslen(Target) * sizeof(WCHAR));
    memcpy(RenameInfo->FileName, Target, RenameInfo->FileNameLength);

    NTSTATUS status = _ntdll.NtSetInformationFile(FileHandle,
        &IoStatusBlock,
        RenameInfo,
        RenameInfoLength,
        (FILE_INFORMATION_CLASS)10 /*FileRenameInformation*/);


    if (0 != status) {
        SetLastError(StatusToWin32Error(status));
    }
    return (0 == status) ? TRUE : FALSE;
}

BOOL NT::GetCurrentOffset(_In_ HANDLE FileHandle, _Out_ PLARGE_INTEGER CurrentOffset)
{
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    FILE_POSITION_INFORMATION PositionInfo = { 0 };

    if (NULL == _ntdll.NtQueryInformationFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    memset(&PositionInfo, 0, sizeof(PositionInfo));
    NTSTATUS status = _ntdll.NtQueryInformationFile(FileHandle,
        &IoStatusBlock,
        &PositionInfo,
        (ULONG)sizeof(PositionInfo),
        (FILE_INFORMATION_CLASS)14 /*FilePositionInformation*/);

    if (0 == status) {
        CurrentOffset->QuadPart = PositionInfo.CurrentByteOffset.QuadPart;
    }
    else {
        SetLastError(StatusToWin32Error(status));
    }

    return (0 == status) ? TRUE : FALSE;
}

BOOL NT::SetCurrentOffset(_In_ HANDLE FileHandle, _In_ PLARGE_INTEGER CurrentOffset)
{
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    FILE_POSITION_INFORMATION PositionInfo = { 0 };

    if (NULL == _ntdll.NtSetInformationFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    PositionInfo.CurrentByteOffset.QuadPart = CurrentOffset->QuadPart;

    NTSTATUS status = _ntdll.NtSetInformationFile(FileHandle,
        &IoStatusBlock,
        &PositionInfo,
        (ULONG)sizeof(PositionInfo),
        (FILE_INFORMATION_CLASS)14 /*FilePositionInformation*/);

    if (0 != status) {
        SetLastError(StatusToWin32Error(status));
    }
    return (0 == status) ? TRUE : FALSE;
}

BOOL NT::GetFileSize(_In_ HANDLE FileHandle, _Out_ PLARGE_INTEGER FileSize)
{
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    FILE_STANDARD_INFORMATION StandardInfo = { 0 };

    if (NULL == _ntdll.NtQueryInformationFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    memset(&StandardInfo, 0, sizeof(StandardInfo));

    NTSTATUS status = _ntdll.NtQueryInformationFile(FileHandle,
        &IoStatusBlock,
        &StandardInfo,
        (ULONG)sizeof(StandardInfo),
        (FILE_INFORMATION_CLASS)5 /*FileStandardInformation*/);

    if (0 == status) {
        FileSize->QuadPart = StandardInfo.EndOfFile.QuadPart;
    }
    else {
        SetLastError(StatusToWin32Error(status));
    }

    return (0 == status) ? TRUE : FALSE;
}

BOOL NT::GetFileSizeEx(_In_ LPCWSTR FileName, _Out_ PLARGE_INTEGER FileSize)
{
    HANDLE FileHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    FILE_BASIC_INFORMATION BasicInformation = { 0 };


    if (NULL == _ntdll.NtSetInformationFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    FileHandle = NT::CreateFile(FileName, FILE_READ_ATTRIBUTES, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL);
    if (NULL == FileHandle) {
        return FALSE;
    }

    BOOL ret = NT::GetFileSize(FileHandle, FileSize);
    _ntdll.NtClose(FileHandle);
    FileHandle = NULL;

    return ret;
}

BOOL NT::SetFileSize(_In_ HANDLE FileHandle, _In_ PLARGE_INTEGER FileSize)
{
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    FILE_END_OF_FILE_INFORMATION EofInformation = { 0 };


    if (NULL == _ntdll.NtSetInformationFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    memset(&EofInformation, 0, sizeof(EofInformation));
    EofInformation.EndOfFile.QuadPart = FileSize->QuadPart;
    NTSTATUS status = _ntdll.NtSetInformationFile(FileHandle,
        &IoStatusBlock,
        &EofInformation,
        (ULONG)sizeof(EofInformation),
        (FILE_INFORMATION_CLASS)20 /*FileEndOfFileInformation*/);

    _ntdll.NtClose(FileHandle);
    FileHandle = NULL;

    if (0 != status) {
        SetLastError(StatusToWin32Error(status));
    }
    return (0 == status) ? TRUE : FALSE;
}

DWORD NT::GetFileAttributes(_In_ LPCWSTR FileName)
{
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
    UNICODE_STRING UnicodeFileName = { 0 };
    FILE_BASIC_INFORMATION BasicInformation = { 0 };

    if (NULL == _ntdll.NtQueryAttributesFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return INVALID_FILE_ATTRIBUTES;
    }

    UnicodeFileName.MaximumLength = (unsigned short)(wcslen(FileName) * 2);
    UnicodeFileName.Length = UnicodeFileName.MaximumLength;
    UnicodeFileName.Buffer = (PWSTR)FileName;
    InitializeObjectAttributes(&ObjectAttributes, &UnicodeFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    NTSTATUS status = _ntdll.NtQueryAttributesFile(&ObjectAttributes, &BasicInformation);
    if (0 != status) {
        SetLastError(StatusToWin32Error(status));
        return INVALID_FILE_ATTRIBUTES;
    }

    return BasicInformation.FileAttributes;
}

BOOL NT::SetFileAttributes(_In_ LPCWSTR FileName, _In_ DWORD FileAttributes)
{
    HANDLE FileHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    FILE_BASIC_INFORMATION BasicInformation = { 0 };
    

    if (NULL == _ntdll.NtSetInformationFile) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    FileHandle = NT::CreateFile(FileName, FILE_WRITE_ATTRIBUTES, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL);
    if (NULL == FileHandle) {
        return FALSE;
    }

    memset(&IoStatusBlock, 0, sizeof(IoStatusBlock));
    memset(&BasicInformation, 0, sizeof(BasicInformation));
    BasicInformation.FileAttributes = (0 == FileAttributes) ? FILE_ATTRIBUTE_NORMAL : FileAttributes;
    NTSTATUS status = _ntdll.NtSetInformationFile(FileHandle,
        &IoStatusBlock,
        &BasicInformation,
        (ULONG)sizeof(BasicInformation),
        (FILE_INFORMATION_CLASS)4 /*FileBasicInformation*/);

    _ntdll.NtClose(FileHandle);
    FileHandle = NULL;

    if (0 != status) {
        SetLastError(StatusToWin32Error(status));
    }
    return (0 == status) ? TRUE : FALSE;
}