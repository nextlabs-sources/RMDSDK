
#include <ntifs.h>

#include <nkdf/error.h>
#include <nkdf/basic.h>
#include <nkdf/crypto.h>
#include <nkdf/fs.h>

#include <nkdf/basic/compress.h>

#include <nkdf/nxlk2.h>



_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLReadHeaderField(
                   _In_ PFLT_INSTANCE Instance,
                   _In_ PFILE_OBJECT FileObject,
                   _In_ ULONG FieldOffset,
                   _In_ ULONG FieldSize,
                   _Out_writes_bytes_(FieldSize) PVOID Data
                   );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLWriteHeaderField(
					_In_ PFLT_INSTANCE Instance,
					_In_ PFILE_OBJECT FileObject,
					_In_ ULONG FieldOffset,
					_In_ ULONG FieldSize,
					_In_reads_bytes_(FieldSize) const VOID* Data
					);

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLCalculateHeaderChecksum(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(32) const UCHAR* Token,
    _Out_writes_bytes_(32) UCHAR* Checksum
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLUpdateHeaderChecksum(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(32) const UCHAR* Token
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLValidateHeaderChecksum(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(32) const UCHAR* Token
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLValidateHeaderChecksum2(
    _In_ const NXL_HEADER_2* Header,
    _In_reads_bytes_(32) const UCHAR* Token
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLInitSectionHeader(
    _Inout_ NXL_HEADER_2* Header,
    _In_reads_bytes_(32) const UCHAR* Token,
    _In_opt_ PCNXL_CREATE_SECTION_BLOB SectionBlob
    );

ULONG
SetSectionBit(
    _In_ ULONG Map,
    _In_ ULONG Id
    );

ULONG
ClearSectionBit(
    _In_ ULONG Map,
    _In_ ULONG Id
    );

BOOLEAN
CheckSectionBit(
    _In_ ULONG Map,
    _In_ ULONG Id
    );

ULONG
FindFirstAvaiableSectionBitAndSet(
    _In_ PULONG Map
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLFindSection(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _Out_ PULONG SectionId
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLFindSectionEx(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _Out_ PULONG SectionId,
    _Out_opt_ NXL_SECTION_2* SectionInfo
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLReadSectionRecord(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG SectionId,
    _Out_ NXL_SECTION_2* SectionInfo
    );

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLWriteSectionRecord(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG SectionId,
    _In_ const NXL_SECTION_2* SectionInfo
    );

_Check_return_
NTSTATUS
NXLVerifySectionChecksum(
    _In_reads_bytes_(16) const UCHAR* Token,
    _In_ const VOID* Buffer,
    _In_ ULONG Length,
    _In_reads_bytes_(32) const UCHAR* Checksum,
    _Out_ PBOOLEAN Match
    );

//  Assign text sections for each routine.
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NXLCheck)
#pragma alloc_text(PAGE, NXLCheck2)
#pragma alloc_text(PAGE, NXLCreate)
#pragma alloc_text(PAGE, NXLCreateEx)
#pragma alloc_text(PAGE, NXLQueryOpen)
#pragma alloc_text(PAGE, NXLOpen)
#pragma alloc_text(PAGE, NXLReadHeaderField)
#pragma alloc_text(PAGE, NXLValidateHeaderChecksum)
#pragma alloc_text(PAGE, NXLValidateHeaderChecksum2)
#pragma alloc_text(PAGE, NXLInitSectionHeader)
#pragma alloc_text(PAGE, NXLFindSection)
#pragma alloc_text(PAGE, NXLReadSectionRecord)
#pragma alloc_text(PAGE, NXLWriteSectionRecord)
#endif

static const ULONG MagicMapBits[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};

static const CHAR SECTION_FILEINFO_NAME_BUFFER[16] = {
    '.', 'F', 'i', 'l', 'e', 'I', 'n', 'f', 'o', 0, 0, 0, 0, 0, 0, 0
};
static const CHAR SECTION_POLICY_NAME_BUFFER[16] = {
    '.', 'F', 'i', 'l', 'e', 'P', 'o', 'l', 'i', 'c', 'y', 0, 0, 0, 0, 0
};
static const CHAR SECTION_FILETAG_NAME_BUFFER[16] = {
    '.', 'F', 'i', 'l', 'e', 'T', 'a', 'g', 0, 0, 0, 0, 0, 0, 0, 0
};

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLCheck(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    try {

        NXL_SIGNATURE_LITE Signature = { 0 };

        // Get signature
        RtlZeroMemory(&Signature, sizeof(NXL_SIGNATURE_LITE));
        Status = NXLReadHeaderField(Instance, FileObject, 0, (ULONG)sizeof(NXL_SIGNATURE_LITE), &Signature);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        // Check magic code
        if (NXL_MAGIC_CODE_1 == Signature.magic.code) {
            Status = STATUS_REVISION_MISMATCH;
        }
        else if (NXL_MAGIC_CODE_2 == Signature.magic.code) {
            // We only handle version 2
            Status = STATUS_SUCCESS;
        }
        else {
            Status = STATUS_UNKNOWN_REVISION;
        }

    try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}

_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NXLCheck2(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_INSTANCE Instance,
    _In_ PCUNICODE_STRING FileName
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE FileHandle = NULL;
    PFILE_OBJECT FileObject = NULL;

    PAGED_CODE();

    try {

        ULONG Result = 0;

        Status = NkFltOpenFile(Filter, Instance, &FileHandle, GENERIC_READ, FileName, &Result, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE, IO_IGNORE_SHARE_ACCESS_CHECK);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = ObReferenceObjectByHandle(FileHandle, GENERIC_READ, *IoFileObjectType, KernelMode, &FileObject, NULL);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = NXLCheck(Instance, FileObject);

    try_exit: NOTHING;
    }
    finally {

        if (NULL != FileObject) {
            ObDereferenceObject(FileObject);
            FileObject = NULL;
        }

        if (NULL != FileHandle) {
            FltClose(FileHandle);
            FileHandle = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLQueryOpen(
             _In_ PFLT_INSTANCE Instance,
             _In_ PFILE_OBJECT FileObject,
             _Out_ NXL_SIGNATURE_LITE* Signature,
             _Out_ PNXL_OPEN_INFO OpenInfo
             )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    try {

        LARGE_INTEGER  ByteOffset = { 0, 0 };
        ULONG  BytesRead = 0;

        // Get signature
        RtlZeroMemory(Signature, sizeof(NXL_SIGNATURE_LITE));
        Status = NXLReadHeaderField(Instance, FileObject, 0, (ULONG)sizeof(NXL_SIGNATURE_LITE), Signature);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        
        // Check magic code
        if (NXL_MAGIC_CODE_1 == Signature->magic.code) {
            try_return(Status = STATUS_REVISION_MISMATCH);
        }
        else if (NXL_MAGIC_CODE_2 == Signature->magic.code) {
            // We only handle version 2
            NOTHING;
        }
        else {
            try_return(Status = STATUS_UNKNOWN_REVISION);
        }

        Status = NXLCheck(Instance, FileObject);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        // Read UDID
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.duid), 16, OpenInfo->Udid);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read OwnerId
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.owner_id), 256, OpenInfo->OwnerId);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read Secure Mode
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.keys), (ULONG)sizeof(ULONG), &OpenInfo->ModeAndFlags);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read IV Seed
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), 16, OpenInfo->IvSeed);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read Token Level
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.token_level), (ULONG)sizeof(ULONG), &OpenInfo->TokenLevel);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read Public Key #1
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.public_key1), (ULONG)FIELD_SIZE(NXL_HEADER_2, fixed.keys.public_key1), OpenInfo->PublicKey1);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read Public Key #2
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.public_key2), (ULONG)FIELD_SIZE(NXL_HEADER_2, fixed.keys.public_key2), OpenInfo->PublicKey2);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read Block Size
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.block_size), (ULONG)FIELD_SIZE(NXL_HEADER_2, fixed.file_info.block_size), &OpenInfo->BlockSize);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Page Size is fixed
        OpenInfo->PageSize = NXL_PAGE_SIZE;
        // Read Content Offset
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.content_offset), (ULONG)FIELD_SIZE(NXL_HEADER_2, fixed.file_info.content_offset), &OpenInfo->ContentOffset);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read Content Length
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, dynamic.content_length), (ULONG)FIELD_SIZE(NXL_HEADER_2, dynamic.content_length), &OpenInfo->ContentLength);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = STATUS_SUCCESS;


    try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}


_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NXLCreate(
          _In_ PFLT_FILTER Filter,
          _In_ PFLT_INSTANCE Instance,
          _In_ PCUNICODE_STRING FileName,
          _Out_ PHANDLE OutFileHandle,
		  _Out_opt_ PFILE_OBJECT *OutFileObject,
          _In_ BOOLEAN Overwrite,
          _In_ PCNXL_CREATE_INFO CreateInfo,
          _In_reads_bytes_opt_(32) const UCHAR* RecoveryKey,
          _Out_ PNXL_CONTEXT Context
          )
{
    PAGED_CODE();
    return NXLCreateEx(Filter, Instance, FileName, OutFileHandle, OutFileObject, Overwrite, CreateInfo, RecoveryKey, NULL, Context);
}

_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NXLCreateEx(
            _In_ PFLT_FILTER Filter,
            _In_ PFLT_INSTANCE Instance,
            _In_ PCUNICODE_STRING FileName,
			_Out_ PHANDLE OutFileHandle,
			_Out_opt_ PFILE_OBJECT *OutFileObject,
            _In_ BOOLEAN Overwrite,
            _In_ PCNXL_CREATE_INFO CreateInfo,
            _In_reads_bytes_opt_(32) const UCHAR* RecoveryKey,
            _In_opt_ PCNXL_CREATE_SECTION_BLOB SectionBlob,
            _Out_ PNXL_CONTEXT Context
            )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NXL_HEADER_2* Header = NULL;

    PAGED_CODE();

    try {

		UCHAR ContentKeyHashBuffer[32+4] = { 0 };
		UCHAR ContentKeyCheckSum[32] = { 0 };
        ULONG SectionDataOffset = 0;
        INT i = 0;
        ULONG Result = 0;
        ULONG BytesWritten = 0;
        LARGE_INTEGER FileSize = { 0, 0 };
        LARGE_INTEGER ByteOffset = { 0, 0 };

        // Init NXL Context
        RtlZeroMemory(Context, sizeof(NXL_CONTEXT));
        Status = NkAesGenerateKey(Context->ContentKey, 32);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        RtlCopyMemory(Context->Udid, CreateInfo->Udid, 16);
        Status = NkAesGenerateKey(Context->IvSeed, 16);
		if (!NT_SUCCESS(Status)) {
			try_return(Status);
		}
        Context->KeySize = 32;
        Context->BlockSize = NXL_BLOCK_SIZE;
        Context->PageSize = NXL_PAGE_SIZE;
        RtlCopyMemory(Context->Token, CreateInfo->Token, 32);

        Header = (NXL_HEADER_2*)ExAllocatePoolWithTag(PagedPool, sizeof(NXL_HEADER_2), TAG_TEMP);
        if (NULL == Header) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(Header, sizeof(NXL_HEADER_2));

        // Signature Header
        Header->fixed.signature.magic.code = NXL_MAGIC_CODE_2;
        Header->fixed.signature.version = NXL_VERSION_30;
        if (CreateInfo->FileMessage.Length != 0) {
            RtlCopyMemory(Header->fixed.signature.message, CreateInfo->FileMessage.Buffer, CreateInfo->FileMessage.Length);
        }

        // File Info Header
        RtlCopyMemory(Header->fixed.file_info.duid, CreateInfo->Udid, 16);
        RtlCopyMemory(Header->fixed.file_info.owner_id, CreateInfo->OwnerId.Buffer, CreateInfo->OwnerId.Length);
        Header->fixed.file_info.flags = CreateInfo->FileFlags;
        Header->fixed.file_info.alignment = NXL_PAGE_SIZE;
        Header->fixed.file_info.algorithm = NXL_ALGORITHM_AES256;
        Header->fixed.file_info.block_size = NXL_BLOCK_SIZE;
        Header->fixed.file_info.extended_data_offset = 0;
        // content_offset will be set at last step

        // Keys Header
        Header->fixed.keys.mode_and_flags = CreateInfo->KeyFlags & 0x00FFFFFF;
        Header->fixed.keys.mode_and_flags |= (CreateInfo->SecureMode << 24);
        Header->fixed.keys.token_level = CreateInfo->TokenLevel;
        RtlCopyMemory(Header->fixed.keys.iv_seed, Context->IvSeed, 16);
        RtlCopyMemory(Header->fixed.keys.public_key1, CreateInfo->PublicKey1, 256);
        RtlCopyMemory(Header->fixed.keys.public_key2, CreateInfo->PublicKey2, 256);
        // Set CEK protected by Token
        RtlCopyMemory(Header->fixed.keys.token_cek, Context->ContentKey, 32);
		Status = NkAesEncrypt2(Context->Token, 32, Header->fixed.keys.iv_seed, 0, NXL_BLOCK_SIZE, Header->fixed.keys.token_cek, 32);
		if (!NT_SUCCESS(Status)) {
			try_return(Status);
		}

        // Calculate HMAC(Token, Cek)
		RtlCopyMemory(ContentKeyHashBuffer + sizeof(ULONG), Context->ContentKey, 32);
		*(ULONG*)ContentKeyHashBuffer = sizeof(Context->ContentKey);
		Status = NkHmacSha256Hash(ContentKeyHashBuffer, sizeof(ULONG) + sizeof(Context->ContentKey), Context->Token, 32, ContentKeyCheckSum);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}
		
		RtlCopyMemory(Header->fixed.keys.token_cek_checksum, ContentKeyCheckSum, 32);
        

        if (NULL != RecoveryKey) {
            RtlCopyMemory(Header->fixed.keys.recovery_cek, RecoveryKey, 32);
			RtlCopyMemory(ContentKeyHashBuffer + sizeof(ULONG), RecoveryKey, 32);
			*(ULONG*)ContentKeyHashBuffer = 32;
			Status = NkHmacSha256Hash(ContentKeyHashBuffer, sizeof(ULONG) + 32, Context->Token, 32, ContentKeyCheckSum);
			if (!NT_SUCCESS(Status)) {
				return Status;
			}
            RtlCopyMemory(Header->fixed.keys.recovery_cek_checksum, CreateInfo->Udid, 16);
			Status = NkAesEncrypt2(Context->Token, 32, Header->fixed.keys.iv_seed, 0, NXL_BLOCK_SIZE, Header->fixed.keys.recovery_cek, 32);
			if (NT_SUCCESS(Status)) {
				RtlCopyMemory(Header->fixed.keys.recovery_cek_checksum, ContentKeyCheckSum, 32);
                Header->fixed.keys.mode_and_flags |= KF_RECOVERY_KEY_ENABLED;
            }
            else {
                RtlZeroMemory(Header->fixed.keys.recovery_cek, 32);
                RtlZeroMemory(Header->fixed.keys.recovery_cek_checksum, 32);
            }
        }

        // Section Header
        Status = NXLInitSectionHeader(Header, Context->Token, SectionBlob);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        //
        //  OKay, the header is ready, create target file
        //
        Status = NkFltCreateFileEx2(Filter,
									Instance,
									OutFileHandle,
									OutFileObject,
									GENERIC_READ | GENERIC_WRITE,
									FileName,
									&Result,
									FILE_ATTRIBUTE_NORMAL,
									FILE_SHARE_READ,
									Overwrite ? FILE_OVERWRITE_IF : FILE_CREATE,
									FILE_NON_DIRECTORY_FILE,
									0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        FileSize.QuadPart = Header->fixed.file_info.content_offset;
        Status = NkFltSetFileSize(Instance, *OutFileObject, FileSize);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = NkFltSyncWriteFile(Instance, *OutFileObject, &ByteOffset, sizeof(NXL_HEADER_2), Header, 0, &BytesWritten);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

    try_exit: NOTHING;
    }
    finally {

        if (NULL != Header) {
            ExFreePool(Header);
            Header = NULL;
        }

        if (!NT_SUCCESS(Status)) {
            NXLFreeContext(Context);
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLOpen(
        _In_ PFLT_INSTANCE Instance,
        _In_ PFILE_OBJECT FileObject,
        _In_reads_bytes_(32) const UCHAR* Token,
        _Out_ PNXL_CONTEXT Context
        )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    try {

		UCHAR CekBuffer[64] = { 0 };
		UCHAR CekChecksumBuf[32 + 4] = { 0 };
		UCHAR Checksum[32] = { 0 };

        // Validate Header Checksum
        Status = NXLValidateHeaderChecksum(Instance, FileObject, Token);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        // Read UDID
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.duid), 16, Context->Udid);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read IV Seed
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), 16, Context->IvSeed);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read CEK
        RtlZeroMemory(CekBuffer, 64);
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.token_cek), 64, CekBuffer);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        // Now decrypt CEK
        Status = NkAesDecrypt2(Token, 32, Context->IvSeed, 0, NXL_BLOCK_SIZE, CekBuffer, 32);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Make sure the decryption succeeded
		RtlCopyMemory(CekChecksumBuf + sizeof(ULONG), CekBuffer, 32);
		*(ULONG*)CekChecksumBuf = 32;
		Status = NkHmacSha256Hash(CekChecksumBuf, sizeof(ULONG) + 32, Token, 32, Checksum);
		if (!NT_SUCCESS(Status)) {
			try_return(Status);
		}

        if (32 != RtlCompareMemory(CekBuffer + 32, Checksum, 32)) {
            try_return(Status = STATUS_WRONG_PASSWORD); // The Token/Password is not correct
        }

        // Read Algorithm (KeySize)
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.algorithm), sizeof(ULONG), &Context->KeySize);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        Context->KeySize = (NXL_ALGORITHM_AES256 == Context->KeySize) ? 32 : 16;
        // Read Block Size
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.block_size), sizeof(ULONG), &Context->BlockSize);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        // Read Content Offset
        Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.content_offset), sizeof(ULONG), &Context->ContentOffset);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
		// Read ContentLength Offset
		Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, dynamic.content_length), sizeof(ULONGLONG), &Context->ContentLength);
		if (!NT_SUCCESS(Status)) {
			try_return(Status);
		}

        // Fill the context
        Context->PageSize = NXL_PAGE_SIZE;
        RtlCopyMemory(Context->Token, Token, 32);
        RtlCopyMemory(Context->ContentKey, CekBuffer, 32);

    try_exit: NOTHING;
    }
    finally {

        if (!NT_SUCCESS(Status)) {
            NXLFreeContext(Context);
        }
    }

    return Status;
}

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
    )
{
    // STATUS_DECRYPTION_FAILED
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER NByteOffset = { 0, 0 };

    PAGED_CODE();

    *BytesRead = 0;
    NByteOffset.QuadPart = ByteOffset->QuadPart + Context->ContentOffset;

    if (!IS_ALIGNED(ByteOffset->LowPart, Context->BlockSize)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!IS_ALIGNED(Length, Context->BlockSize)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (0 == Length) {
        return STATUS_SUCCESS;
    }

    try {

        Status = NkFltSyncReadFile(Instance, FileObject, ByteOffset, Length, Buffer, 0, BytesRead);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        if (0 == *BytesRead) {
            try_return(Status);
        }

        Status = NkAesDecrypt2(Context->ContentKey, Context->KeySize, Context->IvSeed, ByteOffset->QuadPart, Context->BlockSize, Buffer, *BytesRead);
        if (!NT_SUCCESS(Status)) {
            try_return(Status = STATUS_DECRYPTION_FAILED);
        }

    try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}

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
    )
{
    // STATUS_ENCRYPTION_FAILED
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR SwapBuffer = NULL;
    LARGE_INTEGER NByteOffset = { 0, 0 };

    PAGED_CODE();

    *BytesWritten = 0;
    NByteOffset.QuadPart = ByteOffset->QuadPart + Context->ContentOffset;

    if (!IS_ALIGNED(ByteOffset->LowPart, Context->BlockSize)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!IS_ALIGNED(Length, Context->BlockSize)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (0 == Length) {
        return STATUS_SUCCESS;
    }

    try {

        SwapBuffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, Length, TAG_TEMP);
        if (NULL == SwapBuffer) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory(SwapBuffer, Buffer, Length);
        Status = NkAesEncrypt2(Context->ContentKey, Context->KeySize, Context->IvSeed, ByteOffset->QuadPart, Context->BlockSize, SwapBuffer, Length);
        if (!NT_SUCCESS(Status)) {
            try_return(Status = STATUS_ENCRYPTION_FAILED);
        }

        Status = NkFltSyncWriteFile(Instance, FileObject, ByteOffset, Length, SwapBuffer, 0, BytesWritten);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

    try_exit: NOTHING;
    }
    finally {
        if (NULL != SwapBuffer) {
            ExFreePool(SwapBuffer);
            SwapBuffer = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLQuerySection(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _Out_ NXL_SECTION_2* SectionInfo
    )
{
    ULONG SectionId = -1;

    PAGED_CODE();

    return NXLFindSectionEx(Instance, FileObject, SectionName, &SectionId, SectionInfo);
}

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
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    try {

        const ULONG BufferSize = *Length;
        ULONG SectionId = -1;
        ULONG RawDataSize = 0;
        ULONG CompressedDataLength = 0;
        ULONG OriginalDataLength = 0;
        BOOLEAN SectionCompressed = FALSE;
        BOOLEAN SectionEncrypted = FALSE;

        Status = NXLFindSectionEx(Instance, FileObject, SectionName, &SectionId, SectionInfo);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        CompressedDataLength = SectionInfo->compressed_size;
        OriginalDataLength = SectionInfo->data_size;
        SectionCompressed = BooleanFlagOn(SectionInfo->flags, NXL_SECTION_FLAG_COMPRESSED);
        SectionEncrypted = BooleanFlagOn(SectionInfo->flags, NXL_SECTION_FLAG_ENCRYPTED);

        // Empty section
        if (OriginalDataLength == 0) {
            *Length = 0;
            try_return(Status = STATUS_SUCCESS);
        }

        if (SectionCompressed) {

            ASSERT(0 != CompressedDataLength);
            if (0 == CompressedDataLength) {
                *Length = 0;
                try_return(Status = STATUS_FILE_CORRUPT_ERROR);
            }

            RawDataSize = SectionEncrypted ? ROUND_TO_SIZE(CompressedDataLength, AES_BLOCK_SIZE) : CompressedDataLength;
        }
        else {

            ASSERT(0 == CompressedDataLength);
			RawDataSize = SectionEncrypted ? ROUND_TO_SIZE(OriginalDataLength, AES_BLOCK_SIZE) : OriginalDataLength;
        }

        *Length = RawDataSize;
        if (0 == BufferSize) {
            // Query size only
            try_return(Status = STATUS_SUCCESS);
        }

        if (RawDataSize > BufferSize) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        Status = NXLReadHeaderField(Instance, FileObject, SectionInfo->offset, RawDataSize, Buffer);
        if (!NT_SUCCESS(Status)) {
            // Fail to read raw data
            *Length = 0;
        }

    try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}

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
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR TempBuffer = NULL;

    PAGED_CODE();

    try {

        const ULONG BufferSize = *Length;
        ULONG SectionId = -1;
        NXL_SECTION_2 SectionInfo = { 0 };
        ULONG RawDataSize = 0;
        ULONG CompressedDataLength = 0;
        ULONG OriginalDataLength = 0;
        BOOLEAN SectionCompressed = FALSE;
        BOOLEAN SectionEncrypted = FALSE;
        PUCHAR RawData = NULL;
        UCHAR Checksum[32] = { 0 };

        Status = NXLGetRawSectionData(Instance, FileObject, SectionName, &SectionInfo, NULL, &RawDataSize);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        CompressedDataLength = SectionInfo.compressed_size;
        OriginalDataLength = SectionInfo.data_size;
        SectionCompressed = BooleanFlagOn(SectionInfo.flags, NXL_SECTION_FLAG_COMPRESSED);
        SectionEncrypted = BooleanFlagOn(SectionInfo.flags, NXL_SECTION_FLAG_ENCRYPTED);

        // Empty section
        if (OriginalDataLength == 0) {
            *Length = 0;
            try_return(Status = STATUS_SUCCESS);
        }

        // Query only
        if (0 == BufferSize) {
            *Length = OriginalDataLength;
            try_return(Status = STATUS_SUCCESS);
        }

        if (OriginalDataLength > BufferSize) {
            *Length = OriginalDataLength;
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        TempBuffer = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) + RawDataSize, TAG_TEMP);
        if (NULL == TempBuffer) {
            *Length = 0;
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }
        RawData = TempBuffer + sizeof(ULONG);

        RtlCopyMemory(TempBuffer, &OriginalDataLength, sizeof(ULONG));
        Status = NXLReadHeaderField(Instance, FileObject, SectionInfo.offset, RawDataSize, RawData);
        if (!NT_SUCCESS(Status)) {
            *Length = 0;
            try_return(Status);
        }

        Status = NkHmacSha256Hash(TempBuffer, sizeof(ULONG) + RawDataSize, Token, 32, Checksum);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        if (32 != RtlCompareMemory(Checksum, SectionInfo.checksum, 32)) {
            try_return(Status = STATUS_DATA_CHECKSUM_ERROR);
        }

        if (SectionEncrypted) {
            ASSERT(IS_ALIGNED(RawDataSize, 32));
            Status = NkAesDecrypt2(Token, 32, IvSeed, 0, NXL_BLOCK_SIZE, RawData, RawDataSize);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }
        }


        if (SectionCompressed) {
            // Plain section data?
            Status = NkDecompressBuffer(RawData, CompressedDataLength, Buffer, BufferSize, Length);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }
            ASSERT(OriginalDataLength == *Length);
        }
        else {
            *Length = OriginalDataLength;
            RtlCopyMemory(Buffer, RawData, OriginalDataLength);
        }

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    }
    finally {
        
        if (NULL != TempBuffer) {
            ExFreePool(TempBuffer);
            TempBuffer = NULL;
        }
    }

    return Status;
}

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
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR TempBuffer = NULL;
    const ULONG OrigLength = Length;

    PAGED_CODE();

    if (OrigLength > NXL_MAX_SECTION_DATA_LENGTH) {
        return STATUS_APP_DATA_LIMIT_EXCEEDED;
    }

    try {

        ULONG SectionId = -1;
        NXL_SECTION_2 SectionInfo = { 0 };
        PUCHAR SectionData = NULL;
        ULONG CompressedLength = 0;

        Status = NXLFindSectionEx(Instance, FileObject, SectionName, &SectionId, &SectionInfo);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        ASSERT(SectionId < 32);

        TempBuffer = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) + SectionInfo.size, TAG_TEMP);
        if (NULL == TempBuffer) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

		RtlZeroMemory(TempBuffer, sizeof(ULONG) + SectionInfo.size);

        SectionData = TempBuffer + sizeof(ULONG);
        RtlCopyMemory(TempBuffer, &OrigLength, sizeof(ULONG));

        if (Length > SectionInfo.size) {
            // Input data's length is greater than section size, compress it
            Status = NkCompressBuffer(Buffer, Length, SectionData, SectionInfo.size, &CompressedLength);
            if (!NT_SUCCESS(Status)) {
                try_return(Status = STATUS_BUFFER_OVERFLOW);
            }
            ASSERT(CompressedLength <= SectionInfo.size);
            Length = CompressedLength;
        }
		else
		{
			RtlCopyMemory(SectionData, Buffer, Length);
		}

        if (EncryptRequired) {
            Length = ROUND_TO_SIZE(Length, AES_BLOCK_SIZE);
            Status = NkAesEncrypt2(Token, 32, IvecSeed, 0, NXL_BLOCK_SIZE, SectionData, Length);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }
        }

        //
        //  Update section information
        //
        SectionInfo.compressed_size = (USHORT)CompressedLength;
        SectionInfo.data_size = (USHORT)OrigLength;
        Status = NkHmacSha256Hash(TempBuffer, sizeof(ULONG) + Length, Token, 32, SectionInfo.checksum);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        // Write data to file
        Status = NXLWriteHeaderField(Instance, FileObject, SectionInfo.offset, SectionInfo.size, SectionData);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        Status = NXLWriteSectionRecord(Instance, FileObject, SectionId, &SectionInfo);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        Status = NXLUpdateHeaderChecksum(Instance, FileObject, Token);

    try_exit: NOTHING;
    }
    finally {
        if (NULL != TempBuffer) {
            ExFreePool(TempBuffer);
            TempBuffer = NULL;
        }
    }

    return Status;
}


VOID
NXLFreeContext(
    _In_ PNXL_CONTEXT Context
    )
{
    if (NULL == Context) {
        return;
    }
    RtlZeroMemory(Context, sizeof(NXL_CONTEXT));
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLUpdateContentLengthInDynamicHeader(
	_In_ PFLT_INSTANCE Instance,
	_In_ PFILE_OBJECT FileObject,
	_In_ ULONGLONG	ContentLength,
	_Inout_opt_ PNXL_CONTEXT Context
)
{
	NTSTATUS Status = STATUS_SUCCESS;

	PAGED_CODE();

	try {

		// write content length
		Status = NXLWriteHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, dynamic.content_length), 8, &ContentLength);
		if (!NT_SUCCESS(Status)) {
			try_return(Status);
		}

		// Fill the context if there is any
		if (Context)
			Context->ContentLength = ContentLength;

	try_exit: NOTHING;
	}
	finally {
		NOTHING;
	}

	return Status;
}

//
//////////////////////////////////////////////////////////////////////////
//  LOCAL ROUTINES
//////////////////////////////////////////////////////////////////////////

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLReadHeaderField(
                   _In_ PFLT_INSTANCE Instance,
                   _In_ PFILE_OBJECT FileObject,
                   _In_ ULONG FieldOffset,
                   _In_ ULONG FieldSize,
                   _Out_writes_bytes_(FieldSize) PVOID Data
                   )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER ByteOffset = { 0, 0 };
    ULONG BytesRead = 0;

    PAGED_CODE();

    ByteOffset.QuadPart = FieldOffset;

    Status = NkFltSyncReadFile(Instance, FileObject, &ByteOffset, FieldSize, Data, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &BytesRead);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return (FieldSize == BytesRead) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLWriteHeaderField(
	_In_ PFLT_INSTANCE Instance,
	_In_ PFILE_OBJECT FileObject,
	_In_ ULONG FieldOffset,
	_In_ ULONG FieldSize,
	_In_reads_bytes_(FieldSize) const VOID* Data
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	LARGE_INTEGER ByteOffset = { 0, 0 };
	ULONG BytesWritten = 0;

	PAGED_CODE();

	ByteOffset.QuadPart = FieldOffset;

	Status = NkFltSyncWriteFile(Instance, FileObject, &ByteOffset, FieldSize, (PVOID)Data, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &BytesWritten);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	return (FieldSize == BytesWritten) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLUpdateHeaderChecksum(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(32) const UCHAR* Token
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR Checksum[32] = { 0 };

    PAGED_CODE();

    Status = NXLCalculateHeaderChecksum(Instance, FileObject, Token, Checksum);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return NXLWriteHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, dynamic.fixed_header_hash), 32, Checksum);
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLCalculateHeaderChecksum(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(32) const UCHAR* Token,
    _Out_writes_bytes_(32) UCHAR* Checksum
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NXL_HEADER_2* Header = NULL;
	UCHAR *RawBuffer = NULL;

    RtlZeroMemory(Checksum, 32);

    try {

        LARGE_INTEGER ByteOffset = { 0, 0 };
        ULONG BytesRead = 0;

		RawBuffer = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) + sizeof(NXL_HEADER_2), TAG_TEMP);
		if (NULL == RawBuffer) {
			try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
		}

		RtlZeroMemory(RawBuffer, sizeof(ULONG) + sizeof(NXL_HEADER_2));

		Header = (NXL_HEADER_2*)(RawBuffer + sizeof(ULONG));

        Status = NkFltSyncReadFile(Instance, FileObject, &ByteOffset, sizeof(NXL_HEADER_2), Header, 0, &BytesRead);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        if (sizeof(NXL_HEADER_2) != BytesRead) {
            try_return(Status = STATUS_FILE_CORRUPT_ERROR);
        }

		*(ULONG*)RawBuffer = sizeof(NXL_FIXED_HEADER);

        Status = NkHmacSha256Hash(RawBuffer, sizeof(ULONG) + sizeof(NXL_FIXED_HEADER), Token, 32, Checksum);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

    try_exit: NOTHING;
    }
    finally {

        if (NULL != RawBuffer) {
            ExFreePoolWithTag(RawBuffer, TAG_TEMP);
            Header = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLValidateHeaderChecksum(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(32) const UCHAR* Token
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NXL_HEADER_2* Header = NULL;
	UCHAR *RawBuffer = NULL;

	UCHAR Checksum[32] = { 0 };

    try {

        LARGE_INTEGER ByteOffset = { 0, 0 };
        ULONG BytesRead = 0;

		RawBuffer = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) + sizeof(NXL_HEADER_2), TAG_TEMP);
		if (NULL == RawBuffer) {
			try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
		}

		RtlZeroMemory(RawBuffer, sizeof(ULONG) + sizeof(NXL_HEADER_2));

		Header = (NXL_HEADER_2*)(RawBuffer + sizeof(ULONG));

        Status = NkFltSyncReadFile(Instance, FileObject, &ByteOffset, sizeof(NXL_HEADER_2), Header, 0, &BytesRead);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }
        if (sizeof(NXL_HEADER_2) != BytesRead) {
            try_return(Status = STATUS_FILE_CORRUPT_ERROR);
        }

		*(ULONG*)RawBuffer = sizeof(NXL_FIXED_HEADER);

		RtlZeroMemory(Checksum, 32);
		Status = NkHmacSha256Hash(RawBuffer, sizeof(ULONG) + sizeof(NXL_FIXED_HEADER), Token, 32, Checksum);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Status = (32 == RtlCompareMemory(Checksum, Header->dynamic.fixed_header_hash, 32)) ? STATUS_SUCCESS : STATUS_DATA_CHECKSUM_ERROR;

    try_exit: NOTHING;
    }
    finally {

        if (NULL != RawBuffer) {
            ExFreePoolWithTag(RawBuffer, TAG_TEMP);
            Header = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLInitSectionHeader(
    _Inout_ NXL_HEADER_2* Header,
    _In_reads_bytes_(32) const UCHAR* Token,
    _In_opt_ PCNXL_CREATE_SECTION_BLOB SectionBlob
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
	
	UCHAR *RawBuffer = NULL;

    PAGED_CODE();

    try {

        ULONG i = 0;
        ULONG Offset = 0;

        // Section Header
        Header->fixed.sections.section_map = SetSectionBit(Header->fixed.sections.section_map, 0);
        RtlCopyMemory(Header->fixed.sections.record[0].name, SECTION_FILEINFO_NAME_BUFFER, 16);
        Header->fixed.sections.record[0].flags = 0;
        Header->fixed.sections.record[0].offset = sizeof(NXL_HEADER_2);
        Header->fixed.sections.record[0].size = NXL_PAGE_SIZE;
        Header->fixed.sections.record[0].data_size = 0;
        RtlZeroMemory(Header->fixed.sections.record[0].checksum, 32);
        Header->fixed.sections.section_map = SetSectionBit(Header->fixed.sections.section_map, 1);
        RtlCopyMemory(Header->fixed.sections.record[1].name, SECTION_POLICY_NAME_BUFFER, 16);
        Header->fixed.sections.record[1].flags = 0;
        Header->fixed.sections.record[1].offset = sizeof(NXL_HEADER_2) + NXL_PAGE_SIZE;
        Header->fixed.sections.record[1].size = NXL_PAGE_SIZE;
        Header->fixed.sections.record[1].data_size = 0;
        RtlZeroMemory(Header->fixed.sections.record[1].checksum, 32);
        Header->fixed.sections.section_map = SetSectionBit(Header->fixed.sections.section_map, 2);
        RtlCopyMemory(Header->fixed.sections.record[2].name, SECTION_FILETAG_NAME_BUFFER, 16);
        Header->fixed.sections.record[2].flags = 0;
        Header->fixed.sections.record[2].offset = sizeof(NXL_HEADER_2) + NXL_PAGE_SIZE * 2;
        Header->fixed.sections.record[2].size = NXL_PAGE_SIZE;
        Header->fixed.sections.record[2].data_size = 0;
        RtlZeroMemory(Header->fixed.sections.record[2].checksum, 32);

        // Get Input Sections
        if (NULL != SectionBlob) {
            for (i = 0; i < (INT)SectionBlob->Count; i++) {
                ASSERT(IS_ALIGNED(SectionBlob->Sections[i].Size, NXL_PAGE_SIZE));
                if (16 == RtlCompareMemory(SectionBlob->Sections[i].Name, SECTION_FILEINFO_NAME_BUFFER, 16)) {
                    // Reset FileInfo Section's default value
                    Header->fixed.sections.record[0].size = SectionBlob->Sections[i].Size;
                }
                else if (16 == RtlCompareMemory(SectionBlob->Sections[i].Name, SECTION_POLICY_NAME_BUFFER, 16)) {
                    // Reset Policy Section's default value
                    Header->fixed.sections.record[1].size = SectionBlob->Sections[i].Size;
                }
                else if (16 == RtlCompareMemory(SectionBlob->Sections[i].Name, SECTION_FILETAG_NAME_BUFFER, 16)) {
                    // Reset Policy Section's default value
                    Header->fixed.sections.record[2].size = SectionBlob->Sections[i].Size;
                }
                else {
                    // Add new section
                    ULONG Id = FindFirstAvaiableSectionBitAndSet(&Header->fixed.sections.section_map);
                    if (Id == 0xFFFFFFFF) {
                        continue;
                    }
                    ASSERT(Id < 32);
                    RtlCopyMemory(Header->fixed.sections.record[Id].name, SectionBlob->Sections[i].Name, 16);
                    Header->fixed.sections.record[Id].flags = 0;
                    Header->fixed.sections.record[Id].offset = 0;
                    Header->fixed.sections.record[Id].size = SectionBlob->Sections[i].Size;
                    Header->fixed.sections.record[Id].data_size = 0;
                    RtlZeroMemory(Header->fixed.sections.record[Id].checksum, 32);
                }
            }
        }

        // Calculate section data offset
        Offset = sizeof(NXL_HEADER_2);
        for (i = 0; i < 32; i++) {
            if (!CheckSectionBit(Header->fixed.sections.section_map, i)) {
                continue;
            }
            Header->fixed.sections.record[i].offset = Offset;
            Offset += Header->fixed.sections.record[i].size;
        }
        // Reset Content Offset
        Header->fixed.file_info.content_offset = Offset;
        ASSERT(IS_ALIGNED(Header->fixed.file_info.content_offset, NXL_PAGE_SIZE));

		RawBuffer = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) + sizeof(NXL_FIXED_HEADER), TAG_TEMP);
		if (!RawBuffer) {
			try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
		}

		RtlZeroMemory(RawBuffer, sizeof(ULONG) + sizeof(NXL_FIXED_HEADER));

		memcpy(RawBuffer + sizeof(ULONG), &Header->fixed, sizeof(NXL_FIXED_HEADER));

		*(ULONG*)RawBuffer = sizeof(NXL_FIXED_HEADER);

        // Set Header Checksum
        Status = NkHmacSha256Hash(RawBuffer, sizeof(ULONG) + sizeof(NXL_FIXED_HEADER), Token, 32, Header->dynamic.fixed_header_hash);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

    try_exit: NOTHING;
    }
    finally {

		if (NULL != RawBuffer) {
			ExFreePoolWithTag(RawBuffer, TAG_TEMP);
			Header = NULL;
		}

	}

    return Status;
}

ULONG
SetSectionBit(
    _In_ ULONG Map,
    _In_ ULONG Id
    )
{
    ASSERT(Id < 32);
    return (Map | MagicMapBits[Id]);
}

ULONG
ClearSectionBit(
    _In_ ULONG Map,
    _In_ ULONG Id
    )
{
    ASSERT(Id < 32);
    return (Map & (~MagicMapBits[Id]));
}

BOOLEAN
CheckSectionBit(
    _In_ ULONG Map,
    _In_ ULONG Id
    )
{
    ASSERT(Id < 32);
    return (0 == (Map & MagicMapBits[Id])) ? FALSE : TRUE;
}

ULONG
FindFirstAvaiableSectionBitAndSet(
    _In_ PULONG Map
    )
{
    ULONG InitMap = *Map;
    for (ULONG i = 0; i < 32; i++) {
        if (0 == (InitMap & MagicMapBits[i])) {
            *Map = (InitMap | MagicMapBits[i]);
            return i;
        }
    }
    
    // Not found?
    return 0xFFFFFFFF;
}


_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLFindSection(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _Out_ PULONG SectionId
    )
{
    return NXLFindSectionEx(Instance, FileObject, SectionName, SectionId, NULL);
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLFindSectionEx(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ const CHAR* SectionName,
    _Out_ PULONG SectionId,
    _Out_opt_ NXL_SECTION_2* SectionInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   SectionMap = 0;
    ULONG   i = 0;
    ULONG   Offset = 0;

    PAGED_CODE();

    // Read Section Map
    Status = NXLReadHeaderField(Instance, FileObject, FIELD_OFFSET(NXL_HEADER_2, fixed.sections), (ULONG)sizeof(ULONG), &SectionMap);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    // Find
    Offset = FIELD_OFFSET(NXL_HEADER_2, fixed.sections.record);
    for (i = 0; i < 32; i++) {

        NXL_SECTION_2   SectionRecord = { 0 };

        if (!CheckSectionBit(SectionMap, i)) {
            continue;
        }
        // Read section record
        Status = NXLReadSectionRecord(Instance, FileObject, i, &SectionRecord);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
        if (!NkEqualStringA(SectionRecord.name, SectionName, TRUE)) {
            continue;
        }
        // Okay, found!
        *SectionId = i;
        if (NULL != SectionInfo) {
            RtlCopyMemory(SectionInfo, &SectionRecord, sizeof(NXL_SECTION_2));
        }
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLReadSectionRecord(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG SectionId,
    _Out_ NXL_SECTION_2* SectionInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Offset = 0;

    PAGED_CODE();
    ASSERT(SectionId < 32);
    
    Offset = FIELD_OFFSET(NXL_HEADER_2, fixed.sections.record) + (ULONG)(SectionId * sizeof(NXL_SECTION_2));
    return NXLReadHeaderField(Instance, FileObject, Offset, (ULONG)sizeof(NXL_SECTION_2), SectionInfo);
}

_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NXLWriteSectionRecord(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG SectionId,
    _In_ const NXL_SECTION_2* SectionInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Offset = 0;

    PAGED_CODE();
    ASSERT(SectionId < 32);

    Offset = FIELD_OFFSET(NXL_HEADER_2, fixed.sections.record) + (ULONG)(SectionId * sizeof(NXL_SECTION_2));
    return NXLWriteHeaderField(Instance, FileObject, Offset, (ULONG)sizeof(NXL_SECTION_2), SectionInfo);
}


_Check_return_
NTSTATUS
NXLVerifySectionChecksum(
    _In_reads_bytes_(16) const UCHAR* Token,
    _In_ const VOID* Buffer,
    _In_ ULONG Length,
    _In_reads_bytes_(32) const UCHAR* Checksum,
    _Out_ PBOOLEAN Match
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR InterChecksum[32] = { 0 };

    *Match = FALSE;

    Status = NkHmacSha256Hash(Buffer, Length, Token, 32, InterChecksum);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (32 == RtlCompareMemory(InterChecksum, Checksum, 32)) {
        *Match = TRUE;
    }

    return STATUS_SUCCESS;
}