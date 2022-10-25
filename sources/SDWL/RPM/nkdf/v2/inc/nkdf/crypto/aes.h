
/**
 * \file <nkdf/crypto/aes.h>
 * \brief Header file for aes crypto routines
 *
 * This header file declares aes crypto routines
 *
 * \author Gavin Ye
 * \version 1.0.0.0
 * \date 10/6/2014
 *
 */


#ifndef __NKDF_CRYPTO_AES_H__
#define __NKDF_CRYPTO_AES_H__

#define AES_BLOCK_SIZE	16

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesGenerateKey(
    _Out_writes_bytes_(KeySize) UCHAR* Key,
    _In_ ULONG KeySize
    );

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesImportKey(
    _In_reads_bytes_(KeySize) const UCHAR* Key,
    _In_ ULONG KeySize,
    _Out_ PHANDLE KeyHandle
    );

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesExportKey(
    _In_ HANDLE KeyHandle,
    _Out_writes_bytes_(*KeySize) UCHAR* Key,
    _Inout_ ULONG* KeySize
    );

VOID
NkAesDestroyKey(
    _In_ HANDLE KeyHandle
    );

NTSTATUS
NkAesGetKeySize(
    _In_ HANDLE KeyHandle,
    _Out_ PULONG KeySize
    );

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesEncrypt(
    _In_ HANDLE KeyHandle,
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    );

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesDecrypt(
    _In_ HANDLE KeyHandle,
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    );

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesEncrypt2(
    _In_reads_bytes_(KeySize) const UCHAR* Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    );

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NkAesDecrypt2(
    _In_reads_bytes_(KeySize) const UCHAR* Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(16) const UCHAR* IvecSeed,
    _In_ ULONGLONG BlockOffset,
    _In_ ULONG BlockSize,
    _Inout_updates_(Size) PVOID Data,
    _In_ ULONG Size
    );



#endif  // #ifndef __NKDF_CRYPTO_AES_H__



