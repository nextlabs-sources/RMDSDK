
#include "..\stdafx.h"

#include "sha.h"
#include "provider.h"

using namespace NX;
using namespace NX::crypt;

SDWLResult NX::crypt::CreateSha1(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _Out_writes_bytes_(20) unsigned char* out_buf)
{
    Provider* prov = GetProvider(PROV_SHA1);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (NULL == data || 0 == data_size || NULL == out_buf) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

    NTSTATUS status = 0;
    BCRYPT_HASH_HANDLE  h = NULL;
    SDWLResult res = RESULT(0);

    do {

        status = BCryptCreateHash(*prov, &h, NULL, 0, NULL, 0, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptHashData(h, (PUCHAR)data, data_size, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptFinishHash(h, out_buf, 20, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

    } while (FALSE);

    if (h) {
        BCryptDestroyHash(h); h = NULL;
    }

    return res;
}

SDWLResult NX::crypt::CreateHmacSha1(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _In_reads_bytes_(key_size) const unsigned char* key,
    _In_ unsigned long key_size,
    _Out_writes_bytes_(20) unsigned char* out_buf)
{
    Provider* prov = GetProvider(PROV_HMAC_SHA1);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (NULL == data || 0 == data_size || NULL == out_buf || NULL == key || 0 == key_size) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

    NTSTATUS status = 0;
    BCRYPT_HASH_HANDLE  h = NULL;
    SDWLResult res = RESULT(0);

    do {

        status = BCryptCreateHash(*prov, &h, NULL, 0, (PUCHAR)key, key_size, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptHashData(h, (PUCHAR)data, data_size, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptFinishHash(h, out_buf, 20, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

    } while (FALSE);

    if (h) {
        BCryptDestroyHash(h); h = NULL;
    }

    return res;
}

SDWLResult NX::crypt::CreateSha256(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _Out_writes_bytes_(32) unsigned char* out_buf)
{
    Provider* prov = GetProvider(PROV_SHA256);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (NULL == data || 0 == data_size || NULL == out_buf) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

    NTSTATUS status = 0;
    BCRYPT_HASH_HANDLE  h = NULL;
    SDWLResult res = RESULT(0);

    do {

        status = BCryptCreateHash(*prov, &h, NULL, 0, NULL, 0, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptHashData(h, (PUCHAR)data, data_size, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptFinishHash(h, out_buf, 32, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

    } while (FALSE);

    if (h) {
        BCryptDestroyHash(h); h = NULL;
    }

    return res;
}

SDWLResult NX::crypt::CreateHmacSha256(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _In_reads_bytes_(key_size) const unsigned char* key,
    _In_ unsigned long key_size,
    _Out_writes_bytes_(32) unsigned char* out_buf)
{
    Provider* prov = GetProvider(PROV_HMAC_SHA256);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (NULL == data || 0 == data_size || NULL == out_buf || NULL == key || 0 == key_size) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

    NTSTATUS status = 0;
    BCRYPT_HASH_HANDLE  h = NULL;
    SDWLResult res = RESULT(0);

    do {

        status = BCryptCreateHash(*prov, &h, NULL, 0, (PUCHAR)key, key_size, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptHashData(h, (PUCHAR)data, data_size, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

        status = BCryptFinishHash(h, out_buf, 32, 0);
        if (0 != status) {
            res = RESULT(status);
            break;
        }

    } while (FALSE);

    if (h) {
        BCryptDestroyHash(h); h = NULL;
    }

    return res;
}