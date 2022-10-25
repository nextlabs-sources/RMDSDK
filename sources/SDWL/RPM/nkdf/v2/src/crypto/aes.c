

#include <ntifs.h>
#include <Bcrypt.h>
#include <fltKernel.h>

#include <nkdf/basic/defines.h>
#include <nkdf/crypto/provider.h>
#include <nkdf/crypto/aes.h>




_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesEncryptEx(
    _In_ HANDLE KeyHandle,
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size,
    _In_ BOOLEAN SecureCheck
    );

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesDecryptEx(
    _In_ HANDLE KeyHandle,
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size,
    _In_ BOOLEAN SecureCheck
    );

VOID
NkAesGenIvec(
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _Out_writes_bytes_(16) UCHAR* Ivec
    );


_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesGenerateKey(
    _Out_writes_bytes_(KeySize) UCHAR* Key,
    _In_ ULONG KeySize
    )
{
    if (KeySize != 16 && KeySize != 24 && KeySize != 32) {
        return STATUS_INVALID_PARAMETER;
    }

    return BCryptGenRandom(NULL, Key, KeySize, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesImportKey(
    _In_reads_bytes_(KeySize) const UCHAR* Key,
    _In_ ULONG KeySize,
    _Out_ PHANDLE KeyHandle
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BCRYPT_KEY_HANDLE hKey = NULL;;
    PUCHAR KeyBlob = NULL;

    try {

        KeyBlob = ExAllocatePoolWithTag(NonPagedPool,
            sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + KeySize,
            TAG_TEMP);
        if (NULL == KeyBlob) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }


        // Fill key BLOB
        RtlZeroMemory(KeyBlob, sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + KeySize);
        ((PBCRYPT_KEY_DATA_BLOB_HEADER)KeyBlob)->cbKeyData = KeySize;
        ((PBCRYPT_KEY_DATA_BLOB_HEADER)KeyBlob)->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
        ((PBCRYPT_KEY_DATA_BLOB_HEADER)KeyBlob)->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
        RtlCopyMemory(KeyBlob + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), Key, KeySize);

        Status = BCryptImportKey(NkCryptoGetProvider(PROV_AES)->Handle,
            NULL,
            BCRYPT_KEY_DATA_BLOB,
            &hKey,
            NULL,
            0,
            KeyBlob,
            sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + KeySize,
            0);
        if (NT_SUCCESS(Status)) {
            *KeyHandle = hKey;
        }

    try_exit: NOTHING;
    }
    finally {

        if (NULL != KeyBlob) {
            ExFreePool(KeyBlob);
            KeyBlob = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesExportKey(
    _In_ HANDLE KeyHandle,
    _Out_writes_bytes_(*KeySize) UCHAR* Key,
    _Inout_ ULONG* KeySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR KeyBlob = NULL;
    const ULONG InBufferSize = *KeySize;

    try {

        ULONG BlobSize = 0;
        const UCHAR* KeyData = NULL;
        ULONG KeyDataSize = 0;

        Status = BCryptExportKey(KeyHandle, NULL, BCRYPT_KEY_DATA_BLOB, NULL, 0, &BlobSize, 0);
        if (0 == BlobSize) {
            try_return(Status);
        }

        KeyBlob = ExAllocatePoolWithTag(NonPagedPool, BlobSize, TAG_TEMP);
        if (NULL == KeyBlob) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = BCryptExportKey(KeyHandle, NULL, BCRYPT_KEY_DATA_BLOB, KeyBlob, BlobSize, &BlobSize, 0);
        if (0 == BlobSize) {
            try_return(Status);
        }

        KeyData = KeyBlob + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER);
        KeyDataSize = ((PBCRYPT_KEY_DATA_BLOB_HEADER)KeyBlob)->cbKeyData;

        if (16 != KeyDataSize && 24 != KeyDataSize && 32 != KeyDataSize) {
            try_return(Status = STATUS_DATA_ERROR);
        }

        *KeySize = KeyDataSize;
        if (InBufferSize < KeyDataSize) {
            try_return(Status = STATUS_BUFFER_OVERFLOW);
        }
        RtlCopyMemory(Key, KeyData, KeyDataSize);
        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    }
    finally {

        if (NULL != KeyBlob) {
            ExFreePool(KeyBlob);
            KeyBlob = NULL;
        }
    }

    return Status;
}

VOID
NkAesDestroyKey(
    _In_ HANDLE KeyHandle
    )
{
    if (NULL != KeyHandle) {
        BCryptDestroyKey(KeyHandle);
    }
}


NTSTATUS
NkAesGetKeySize(
    _In_ HANDLE KeyHandle,
    _Out_ PULONG KeySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG KeyBitsLength = 0;
    ULONG cbResult = 0;

    *KeySize = 0;
    Status = BCryptGetProperty(KeyHandle, BCRYPT_KEY_LENGTH, (PUCHAR)&KeyBitsLength, sizeof(ULONG), &cbResult, 0);
    if (NT_SUCCESS(Status)) {
        *KeySize = KeyBitsLength / 8;
    }

    return Status;
}


_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesEncrypt(
    _In_ HANDLE KeyHandle,
    _In_ const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    )
{
    return NkAesEncryptEx(KeyHandle, IvecSeed, BlockOffset, BlockSize, Data, Size, TRUE);
}


_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesDecrypt(
    _In_ HANDLE KeyHandle,
    _In_ const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    )
{
    return NkAesDecryptEx(KeyHandle, IvecSeed, BlockOffset, BlockSize, Data, Size, TRUE);
}
_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesEncrypt2(
    _In_reads_bytes_(KeySize) const UCHAR* Key,
    _In_ ULONG KeySize,
    _In_ const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE   KeyHandle = NULL;

    if (0 != (BlockOffset % BlockSize)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!IS_ALIGNED(Size, AES_BLOCK_SIZE)) {
        return STATUS_INVALID_PARAMETER;
    }

    try {

        Status = NkAesImportKey(Key, KeySize, &KeyHandle);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        ASSERT(NULL != KeyHandle);

        Status = NkAesEncryptEx(KeyHandle, IvecSeed, BlockOffset, BlockSize, Data, Size, FALSE);

    try_exit: NOTHING;
    }
    finally {

        if (NULL != KeyHandle) {
            NkAesDestroyKey(KeyHandle);
            KeyHandle = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesDecrypt2(
    _In_reads_bytes_(KeySize) const UCHAR* Key,
    _In_ ULONG KeySize,
    _In_ const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE   KeyHandle = NULL;

    if (0 != (BlockOffset % BlockSize)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!IS_ALIGNED(Size, AES_BLOCK_SIZE)) {
        return STATUS_INVALID_PARAMETER;
    }

    try {

        Status = NkAesImportKey(Key, KeySize, &KeyHandle);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        ASSERT(NULL != KeyHandle);

        Status = NkAesDecryptEx(KeyHandle, IvecSeed, BlockOffset, BlockSize, Data, Size, FALSE);

    try_exit: NOTHING;
    }
    finally {

        if (NULL != KeyHandle) {
            NkAesDestroyKey(KeyHandle);
            KeyHandle = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesEncryptEx(
    _In_ HANDLE KeyHandle,
    _In_ const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size,
    _In_ BOOLEAN SecureCheck
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    UCHAR       IV[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


    // Parameters check
    if (NULL == KeyHandle || NULL == Data || 0 == Size || 0 == BlockSize) {
        return STATUS_INVALID_PARAMETER;
    }

    if (SecureCheck) {

        if (0 != (BlockOffset % BlockSize)) {
            return STATUS_INVALID_PARAMETER;
        }

        if (!IS_ALIGNED(Size, AES_BLOCK_SIZE)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    while (Size != 0) {

        ULONG   BytesToEncrypt = min(Size, BlockSize);
        ULONG   OutSize = 0;

        NkAesGenIvec(IvecSeed, BlockOffset, IV);
        Status = BCryptEncrypt(KeyHandle,
            Data,
            BytesToEncrypt,
            NULL,
            IV,
            16,
            Data,
            BytesToEncrypt,
            &OutSize,
            0);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        ASSERT(OutSize == BytesToEncrypt);
        Size -= BytesToEncrypt;
        BlockOffset += BytesToEncrypt;
        Data = (PVOID)(((PUCHAR)Data) + BytesToEncrypt);
    }


    return Status;
}

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesDecryptEx(
    _In_ HANDLE KeyHandle,
    _In_ const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size,
    _In_ BOOLEAN SecureCheck
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    UCHAR       IV[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


    // Parameters check
    if (NULL == KeyHandle || NULL == Data || 0 == Size || 0 == BlockSize) {
        return STATUS_INVALID_PARAMETER;
    }

    if (SecureCheck) {

        if (0 != (BlockOffset % BlockSize)) {
            return STATUS_INVALID_PARAMETER;
        }

        if (!IS_ALIGNED(Size, AES_BLOCK_SIZE)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    while (Size != 0) {

        ULONG   BytesToDecrypt = min(Size, BlockSize);
        ULONG   OutSize = 0;

        NkAesGenIvec(IvecSeed, BlockOffset, IV);
        Status = BCryptDecrypt(KeyHandle,
            Data,
            BytesToDecrypt,
            NULL,
            IV,
            16,
            Data,
            BytesToDecrypt,
            &OutSize,
            0
            );
        ASSERT(OutSize == BytesToDecrypt);
        Size -= BytesToDecrypt;
        BlockOffset += BytesToDecrypt;
        Data = (PVOID)(((PUCHAR)Data) + BytesToDecrypt);
    }

    return Status;
}


VOID
NkAesGenIvec(
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG IvecValue,
    _Out_writes_bytes_(16) UCHAR* Ivec
    )
{
    if (NULL == IvecSeed) {
        RtlZeroMemory(Ivec, 16);
        memcpy(Ivec, &IvecValue, 8);
    }
    else {
        RtlCopyMemory(Ivec, IvecSeed, 16);
		if (IvecValue) {
			IvecValue = (IvecValue - 1) * 31;
		}
        ((ULONGLONG*)Ivec)[0] ^= IvecValue;
        ((ULONGLONG*)Ivec)[1] ^= IvecValue;
    }
}