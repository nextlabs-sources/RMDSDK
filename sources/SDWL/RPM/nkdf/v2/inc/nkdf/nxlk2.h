


#ifndef __NEXTLABS_FILE_FORMAT_H__
#define __NEXTLABS_FILE_FORMAT_H__
#include <ntifs.h>
#include <fltkernel.h>
#include <nxlfmt.h>


//
//  Struct Alignment: 4
//
#pragma pack(push)
#pragma pack(4)



typedef struct _NXL_OWNER_INFO {
    CHAR    OwnerId[256];
    UCHAR   PublicKey1[256];
    UCHAR   PublicKey2[256];
} NXL_OWNER_INFO, *PNXL_OWNER_INFO;
typedef const NXL_OWNER_INFO* PCNXL_OWNER_INFO;

typedef struct _NXL_OPEN_INFO {
    ULONG   ModeAndFlags;
    ULONG   TokenLevel;
    UCHAR   Udid[16];
    UCHAR   IvSeed[16];
    CHAR    OwnerId[256];
    UCHAR   PublicKey1[256];
    UCHAR   PublicKey2[256];
    ULONG   BlockSize;
    ULONG   PageSize;
    ULONG   ContentOffset;
    ULONGLONG   ContentLength;
} NXL_OPEN_INFO, *PNXL_OPEN_INFO;
typedef const NXL_OPEN_INFO* PCNXL_OPEN_INFO;

typedef struct _NXL_CREATE_INFO {
    ULONG   FileFlags;
    ULONG   SecureMode;
    ULONG   KeyFlags;
    ULONG   TokenLevel;
    UCHAR   Token[32];
    UCHAR   Udid[16];
    UCHAR   PublicKey1[256];
    UCHAR   PublicKey2[256];
    STRING  OwnerId;
    STRING  FileMessage;
} NXL_CREATE_INFO, *PNXL_CREATE_INFO;
typedef const NXL_CREATE_INFO* PCNXL_CREATE_INFO;

typedef struct _NXL_CREATE_SECURITY {
    ULONG   SecureMode;
    ULONG   TokenLevel;
    UCHAR   Token[32];
    UCHAR   PasswordHash[32];
} NXL_CREATE_SECURITY, *PNXL_CREATE_SECURITY;
typedef const NXL_CREATE_SECURITY* PCNXL_CREATE_SECURITY;

typedef struct _NXL_CREATE_SECTION {
    CHAR    Name[16];
    ULONG   Size;
} NXL_CREATE_SECTION, *PNXL_CREATE_SECTION;
typedef const NXL_CREATE_SECTION* PCNXL_CREATE_SECTION;

typedef struct _NXL_CREATE_SECTION_BLOB {
    ULONG               Count;
    NXL_CREATE_SECTION  Sections[1];
} NXL_CREATE_SECTION_BLOB, *PNXL_CREATE_SECTION_BLOB;
typedef const NXL_CREATE_SECTION_BLOB* PCNXL_CREATE_SECTION_BLOB;

typedef struct _NXL_CONTEXT {
    UCHAR		Udid[16];
    UCHAR		ContentKey[32];
    UCHAR		Token[32];
    UCHAR       IvSeed[16];
    ULONG		KeySize;
    ULONG		BlockSize;
    ULONG		PageSize;
    ULONG		ContentOffset;
	ULONGLONG	ContentLength;
} NXL_CONTEXT, *PNXL_CONTEXT;
typedef const NXL_CONTEXT* PCNXL_CONTEXT;


#pragma pack(pop)   // Struct Alignment: 4

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLCheck(
         _In_ PFLT_INSTANCE Instance,
         _In_ PFILE_OBJECT FileObject
         );

_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NXLCheck2(
          _In_ PFLT_FILTER Filter,
          _In_ PFLT_INSTANCE Instance,
          _In_ PCUNICODE_STRING FileName
          );

// Return STATUS_OBJECT_TYPE_MISMATCH if target file is NOT an NXL file
_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLQueryOpen(
             _In_ PFLT_INSTANCE Instance,
             _In_ PFILE_OBJECT FileObject,
             _Out_ NXL_SIGNATURE_LITE* Signature,
             _Out_ PNXL_OPEN_INFO OpenInfo
             );

_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NXLCreate(
          _In_ PFLT_FILTER Filter,
          _In_ PFLT_INSTANCE Instance,
          _In_ PCUNICODE_STRING FileName,
		  _In_ PHANDLE OutFileHandle,
	      _Out_ PFILE_OBJECT* OutFileObject,
          _In_ BOOLEAN Overwrite,
          _In_ PCNXL_CREATE_INFO CreateInfo,
          _In_reads_bytes_opt_(32) const UCHAR* RecoveryKey,
          _Out_ PNXL_CONTEXT Context
          );

_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NXLCreateEx(
            _In_ PFLT_FILTER Filter,
            _In_ PFLT_INSTANCE Instance,
            _In_ PCUNICODE_STRING FileName,
			_In_ PHANDLE OutFileHandle,
            _Out_ PFILE_OBJECT* OutFileObject,
            _In_ BOOLEAN Overwrite,
            _In_ PCNXL_CREATE_INFO CreateInfo,
            _In_reads_bytes_opt_(32) const UCHAR* RecoveryKey,
            _In_opt_ PCNXL_CREATE_SECTION_BLOB SectionBlob,
            _Out_ PNXL_CONTEXT Context
            );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLOpen(
        _In_ PFLT_INSTANCE Instance,
        _In_ PFILE_OBJECT FileObject,
        _In_reads_bytes_(32) const UCHAR* Token,
        _Out_ PNXL_CONTEXT Context
        );

// ByteOffset & Length must be aligned with Encryption Block Size
_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLSafeRead(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PNXL_CONTEXT Context,
    _In_ PLARGE_INTEGER ByteOffset,
    _Out_ PUCHAR Buffer,
    _In_ ULONG Length,
    _Out_ PULONG BytesRead
    );

// ByteOffset & Length must be aligned with Encryption Block Size
_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLSafeWrite(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PNXL_CONTEXT Context,
    _In_ PLARGE_INTEGER ByteOffset,
    _In_ PUCHAR Buffer,
    _In_ ULONG Length,
    _Out_ PULONG BytesWritten
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLQuerySection(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _Out_ NXL_SECTION_2* SectionInfo
    );


/*
This function try to get raw section data (maybe compressed and encrypted)
a. Section data is compressed
   Valid raw data length = compressed-data-length
   Checksum = HMAC_SHA256(Token, original-data-length (ULONG, 4 bytes) + valid-raw-data)
b. Section data is encrypted
   Valid raw data length = ROUND_TO_SIZE(original-data-length, 32)
c. Section data is compressed and then encrypted
   Valid raw data length = ROUND_TO_SIZE(compressed-data-length, 32)

Checksum

    Checksum = HMAC_SHA256(Token, original-data-length (ULONG, 4 bytes) + valid-raw-data)

*/
_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLGetRawSectionData(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _Out_ NXL_SECTION_2* SectionInfo,
    _Out_writes_bytes_opt_(*Length) PVOID Buffer,
    _Inout_ PULONG Length
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLGetSectionData(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _In_reads_bytes_(32) const VOID* Token,
    _In_reads_bytes_(16) const VOID* IvSeed,
    _Out_writes_bytes_opt_(*Length) PVOID Buffer,
    _Inout_ PULONG Length
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLSetSectionData(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(32) const UCHAR* Token,
    _In_reads_bytes_(16) const UCHAR* IvecSeed,
    _In_ const CHAR* SectionName,
    _In_reads_bytes_opt_(Length) const VOID* Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN EncryptRequired
    );

VOID
NXLFreeContext(
    _In_ PNXL_CONTEXT Context
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLUpdateContentLengthInDynamicHeader(
	_In_ PFLT_INSTANCE Instance,
	_In_ PFILE_OBJECT FileObject,
	_In_ ULONGLONG	ContentLength,
	_Inout_opt_ PNXL_CONTEXT Context
	);

#endif      // __NEXTLABS_FILE_FORMAT_H__