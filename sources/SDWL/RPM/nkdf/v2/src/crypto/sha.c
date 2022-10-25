

#include <ntifs.h>
#include <Bcrypt.h>

#include <nkdf/basic/defines.h>
#include <nkdf/crypto/provider.h>
#include <nkdf/crypto/sha.h>




_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkSha1Hash(
           _In_reads_bytes_(Size) const VOID* Data,
           _In_ ULONG Size,
           _Out_writes_bytes_(20) PVOID Hash
           )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    BCRYPT_HASH_HANDLE  Handle = NULL;
    PUCHAR              pbHashObject = NULL;

    const NXALGOBJECT* provider = NkCryptoGetProvider(PROV_SHA1);


    if (NULL == Hash) {
        return STATUS_INVALID_PARAMETER;
    }
    if (NULL == Data || 0 == Size) {
        return STATUS_INVALID_PARAMETER;
    }
    RtlZeroMemory(Hash, 20);

    // Parameters check
    if (!NkCryptoInitialized()) {
        return STATUS_DEVICE_NOT_READY;
    }

    ASSERT(20 == provider->HashLength);

    try {

        ULONG cbResult = 0;
        
        pbHashObject = ExAllocatePoolWithTag(NonPagedPool, provider->ObjectLength, TAG_TEMP);
        if (!pbHashObject) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = BCryptCreateHash(provider->Handle,
                                  &Handle,
                                  pbHashObject,
                                  provider->ObjectLength,
                                  NULL,
                                  0,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptHashData(Handle,
                                (PUCHAR)Data,
                                Size,
                                0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptFinishHash(Handle,
                                  Hash,
                                  20,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

try_exit: NOTHING;
    }
    finally {

        if (NULL != Handle) {
            (VOID)BCryptDestroyHash(Handle);
            Handle = NULL;
        }

        if (NULL != pbHashObject) {
            ExFreePool(pbHashObject);
            pbHashObject = NULL;
        }
    }

    return Status;
}


_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkHmacSha1Hash(
               _In_reads_bytes_(Size) const VOID* Data,
               _In_ ULONG Size,
               _In_reads_bytes_(KeySize) const VOID* Key,
               _In_ ULONG KeySize,
               _Out_writes_bytes_(20) PVOID Hash
               )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    BCRYPT_HASH_HANDLE  Handle = NULL;
    PUCHAR              pbHashObject = NULL;

    const NXALGOBJECT* provider = NkCryptoGetProvider(PROV_HMAC_SHA1);


    if (NULL == Hash) {
        return STATUS_INVALID_PARAMETER;
    }
    if (NULL == Data || 0 == Size) {
        return STATUS_INVALID_PARAMETER;
    }
    if (NULL == Key || 0 == KeySize) {
        return STATUS_INVALID_PARAMETER;
    }
    RtlZeroMemory(Hash, 20);

    // Parameters check
    if (!NkCryptoInitialized()) {
        return STATUS_DEVICE_NOT_READY;
    }

    ASSERT(20 == provider->HashLength);

    try {

        ULONG cbResult = 0;
        
        pbHashObject = ExAllocatePoolWithTag(NonPagedPool, provider->ObjectLength, TAG_TEMP);
        if (!pbHashObject) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = BCryptCreateHash(provider->Handle,
                                  &Handle,
                                  pbHashObject,
                                  provider->ObjectLength,
                                  (PUCHAR)Key,
                                  KeySize,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptHashData(Handle,
                                (PUCHAR)Data,
                                Size,
                                0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptFinishHash(Handle,
                                  Hash,
                                  20,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

try_exit: NOTHING;
    }
    finally {

        if (NULL != Handle) {
            (VOID)BCryptDestroyHash(Handle);
            Handle = NULL;
        }

        if (NULL != pbHashObject) {
            ExFreePool(pbHashObject);
            pbHashObject = NULL;
        }
    }

    return Status;
}

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkSha256Hash(
             _In_reads_bytes_(Size) const VOID* Data,
             _In_ ULONG Size,
             _Out_writes_bytes_(32) PVOID Hash
             )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    BCRYPT_HASH_HANDLE  Handle = NULL;
    PUCHAR              pbHashObject = NULL;

    const NXALGOBJECT* provider = NkCryptoGetProvider(PROV_HMAC_SHA256);


    if (NULL == Hash) {
        return STATUS_INVALID_PARAMETER;
    }
    if (NULL == Data || 0 == Size) {
        return STATUS_INVALID_PARAMETER;
    }
    RtlZeroMemory(Hash, 32);

    // Parameters check
    if (!NkCryptoInitialized()) {
        return STATUS_DEVICE_NOT_READY;
    }

    ASSERT(32 == provider->HashLength);

    try {

        ULONG cbResult = 0;

        pbHashObject = ExAllocatePoolWithTag(NonPagedPool, provider->ObjectLength, TAG_TEMP);
        if (!pbHashObject) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = BCryptCreateHash(provider->Handle,
                                  &Handle,
                                  pbHashObject,
                                  provider->ObjectLength,
                                  NULL,
                                  0,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptHashData(Handle,
                                (PUCHAR)Data,
                                Size,
                                0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }


        Status = BCryptFinishHash(Handle,
                                  Hash,
                                  32,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

try_exit: NOTHING;
    }
    finally {

        if (NULL != Handle) {
            (VOID)BCryptDestroyHash(Handle);
            Handle = NULL;
        }

        if (NULL != pbHashObject) {
            ExFreePool(pbHashObject);
            pbHashObject = NULL;
        }
    }

    return Status;
}


_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkHmacSha256Hash(
                 _In_reads_bytes_(Size) const VOID* Data,
                 _In_ ULONG Size,
                 _In_reads_bytes_(KeySize) const VOID* Key,
                 _In_ ULONG KeySize,
                 _Out_writes_bytes_(32) PVOID Hash
                 )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    BCRYPT_HASH_HANDLE  Handle = NULL;
    PUCHAR              pbHashObject = NULL;

    const NXALGOBJECT* provider = NkCryptoGetProvider(PROV_HMAC_SHA256);


    if (NULL == Hash) {
        return STATUS_INVALID_PARAMETER;
    }
    if (NULL == Data || 0 == Size) {
        return STATUS_INVALID_PARAMETER;
    }
    RtlZeroMemory(Hash, 32);

    // Parameters check
    if (!NkCryptoInitialized()) {
        return STATUS_DEVICE_NOT_READY;
    }

    ASSERT(32 == provider->HashLength);

    try {

        ULONG cbResult = 0;

        pbHashObject = ExAllocatePoolWithTag(NonPagedPool, provider->ObjectLength, TAG_TEMP);
        if (!pbHashObject) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = BCryptCreateHash(provider->Handle,
                                  &Handle,
                                  pbHashObject,
                                  provider->ObjectLength,
                                  (PUCHAR)Key,
                                  KeySize,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = BCryptHashData(Handle,
                                (PUCHAR)Data,
                                Size,
                                0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }


        Status = BCryptFinishHash(Handle,
                                  Hash,
                                  32,
                                  0);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

try_exit: NOTHING;
    }
    finally {

        if (NULL != Handle) {
            (VOID)BCryptDestroyHash(Handle);
            Handle = NULL;
        }

        if (NULL != pbHashObject) {
            ExFreePool(pbHashObject);
            pbHashObject = NULL;
        }
    }

    return Status;
}