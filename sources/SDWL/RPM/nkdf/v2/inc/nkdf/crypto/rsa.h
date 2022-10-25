
/**
 * \file <nkdf/crypto/rsa.h>
 * \brief Header file for rsa crypto routines
 *
 * This header file declares rsa crypto routines
 *
 * \author Gavin Ye
 * \version 1.0.0.0
 * \date 10/6/2014
 *
 */


#ifndef __NKDF_CRYPTO_RSA_H__
#define __NKDF_CRYPTO_RSA_H__

#include <bcrypt.h>


NTSTATUS
NRsaCreateKey(
    _In_ ULONG BitsLength,
    _Out_ BCRYPT_KEY_HANDLE* ph
    );

NTSTATUS
NRsaLoadKey(
    _In_reads_bytes_(Size) const VOID* Key,
    _In_ ULONG Size,
    _Out_ BCRYPT_KEY_HANDLE* ph,
    _Out_opt_ PBOOLEAN IsPrivateKey
    );

NTSTATUS
NRsaExportPublicKey(
    _In_ BCRYPT_KEY_HANDLE h,
    _Out_writes_bytes_opt_(*Size) VOID* Key,
    _Inout_ PULONG Size
    );

NTSTATUS
NRsaExportPrivateKey(
    _In_ BCRYPT_KEY_HANDLE h,
    _Out_writes_bytes_opt_(*Size) VOID* Key,
    _Inout_ PULONG Size
    );

NTSTATUS
NRsaCloseKey(
    _In_ BCRYPT_KEY_HANDLE h
    );


typedef enum {
    RSA_ENCRYPT = 0,
    RSA_DECRYPT,
    RSA_SIGN,
    RSA_VERIFY
} RSAOPERATOR;

NTSTATUS
NRsaGetOutputSize(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_ RSAOPERATOR Operator,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _Out_ PULONG OutputSize
    );

NTSTATUS
NRsaEncrypt(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _Out_writes_bytes_(OutSize) VOID* CipherData,
    _Inout_ PULONG OutSize
    );

NTSTATUS
NRsaDecrypt(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* CipherData,
    _In_ ULONG DataSize,
    _Out_writes_bytes_(OutSize) VOID* Data,
    _Inout_ PULONG OutSize
    );


NTSTATUS
NRsaSign(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _Out_writes_bytes_(OutSize) VOID* Signature,
    _Inout_ PULONG OutSize
    );

NTSTATUS
NxRsaVerify(
    _In_ BCRYPT_KEY_HANDLE h,
    _In_reads_bytes_(DataSize) const VOID* Data,
    _In_ ULONG DataSize,
    _In_reads_bytes_(SignatureSize) const VOID* Signature,
    _In_ ULONG SignatureSize
    );





#endif  // #ifndef __NKDF_CRYPTO_RSA_H__



