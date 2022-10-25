

#include <ntifs.h>

#include <nkdf/basic/defines.h>
#include <nkdf/crypto/provider.h>
#include <nkdf/crypto/rsa.h>
#include <nkdf/crypto/sha.h>




NTSTATUS
NRsaCreateKey(
    _In_ ULONG BitsLength,
    _Out_ BCRYPT_KEY_HANDLE* ph
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    const NXALGOBJECT* provider = NkCryptoGetProvider(PROV_RSA);

    Status = BCryptGenerateKeyPair(provider->Handle, ph, BitsLength, 0);
    if (!NT_SUCCESS(Status)) {
        *ph = NULL;
    }

    return Status;
}

NTSTATUS
NRsaLoadKey(
    _In_reads_bytes_(Size) const VOID* Key,
    _In_ ULONG Size,
    _Out_ BCRYPT_KEY_HANDLE* ph,
    _Out_opt_ PBOOLEAN IsPrivateKey
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    const NXALGOBJECT* provider = NkCryptoGetProvider(PROV_RSA);

    try {

        Status = BCryptImportKeyPair(provider->Handle,
            NULL,
            BCRYPT_RSAPUBLIC_BLOB,
            ph,
            (PUCHAR)Key, Size,
            0);
        if (!NT_SUCCESS(Status)) {
            *ph = NULL;
            try_return(Status);
        }

    try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}

NTSTATUS
NRsaExportPublicKey(
    _In_ BCRYPT_KEY_HANDLE h,
    _Out_writes_bytes_opt_(*Size) VOID* Key,
    _Inout_ PULONG Size
    )
{
    return BCryptExportKey(h, NULL, BCRYPT_RSAPUBLIC_BLOB, Key, *Size, Size, 0);
}

NTSTATUS
NRsaExportPrivateKey(
    _In_ BCRYPT_KEY_HANDLE h,
    _Out_writes_bytes_opt_(*Size) VOID* Key,
    _Inout_ PULONG Size
    )
{

    return BCryptExportKey(h, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, Key, *Size, Size, 0);
}

NTSTATUS
NRsaCloseKey(
    _In_ BCRYPT_KEY_HANDLE h
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    if (NULL != h) {
        Status = BCryptDestroyKey(h);
    }

    return Status;
}

NTSTATUS
NRsaGetOutputSize(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_ RSAOPERATOR Operator,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _Out_ PULONG OutputSize
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    *OutputSize = 0;

    switch (Operator)
    {
    case RSA_ENCRYPT:
        Status = BCryptEncrypt(h, (PUCHAR)Data, DataSize, NULL, NULL, 0, NULL, 0, OutputSize, BCRYPT_PAD_PKCS1);
        break;
    case RSA_DECRYPT:
        Status = BCryptDecrypt(h, (PUCHAR)Data, DataSize, NULL, NULL, 0, NULL, 0, OutputSize, BCRYPT_PAD_PKCS1);
        break;
    case RSA_SIGN:
        {
            UCHAR HashData[20] = { 0 };
            BCRYPT_PKCS1_PADDING_INFO Pkcs1PaddingInfo = { BCRYPT_SHA1_ALGORITHM };
            Status = NkSha1Hash(Data, DataSize, HashData);
            if (NT_SUCCESS(Status)) {
                Status = BCryptSignHash(h, &Pkcs1PaddingInfo, HashData, 20, NULL, 0, OutputSize, BCRYPT_PAD_PKCS1);
                if (!NT_SUCCESS(Status)) {
                    *OutputSize = 0;
                }
            }
        }
        break;
    case RSA_VERIFY:
    default:
        break;
    }

    return Status;
}

NTSTATUS
NRsaEncrypt(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _Out_writes_bytes_opt_(OutSize) VOID* CipherData,
    _Inout_ PULONG OutSize
    )
{
   return BCryptEncrypt(h, (PUCHAR)Data, DataSize, NULL, NULL, 0, (PUCHAR)CipherData, *OutSize, OutSize, BCRYPT_PAD_PKCS1);
}

NTSTATUS
NRsaDecrypt(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* CipherData,
    _In_ ULONG DataSize,
    _Out_writes_bytes_(OutSize) VOID* Data,
    _Inout_ PULONG OutSize
    )
{
    return BCryptDecrypt(h, (PUCHAR)CipherData, DataSize, NULL, NULL, 0, (PUCHAR)Data, *OutSize, OutSize, BCRYPT_PAD_PKCS1);;
}

NTSTATUS
NRsaSign(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _Out_writes_bytes_(OutSize) VOID* Signature,
    _Inout_ PULONG OutSize
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    try {

        UCHAR HashData[20] = { 0 };
        const ULONG HashLength = 20;
        BCRYPT_PKCS1_PADDING_INFO Pkcs1PaddingInfo = { BCRYPT_SHA1_ALGORITHM };
        Status = NkSha1Hash(Data, DataSize, HashData);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptSignHash(h, &Pkcs1PaddingInfo, HashData, HashLength, (PUCHAR)Signature, *OutSize, OutSize, BCRYPT_PAD_PKCS1);
        if (!NT_SUCCESS(Status)) {
            *OutSize = 0;
        }

try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}

NTSTATUS
NxRsaVerify(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _In_reads_bytes_(SignatureSize) const VOID* Signature,
    _In_ ULONG SignatureSize
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    try {

        UCHAR HashData[20] = { 0 };
        const ULONG HashLength = 20;
        BCRYPT_PKCS1_PADDING_INFO Pkcs1PaddingInfo = { BCRYPT_SHA1_ALGORITHM };
        Status = NkSha1Hash(Data, DataSize, HashData);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptVerifySignature(h, &Pkcs1PaddingInfo, HashData, HashLength, (PUCHAR)Signature, SignatureSize, BCRYPT_PAD_PKCS1);

try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}
