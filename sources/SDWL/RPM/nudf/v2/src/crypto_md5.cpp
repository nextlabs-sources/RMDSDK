
#include <Windows.h>
#include <assert.h>

#include <nudf\crypto_md5.hpp>


using namespace NX;
using namespace NX::crypto;


bool NX::crypto::md5(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _Out_writes_bytes_(16) unsigned char* out_buf)
{
    const provider_object*  provider = GET_PROVIDER(PROV_MD5);
    NTSTATUS status = 0;
    BCRYPT_HASH_HANDLE  h = NULL;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    if (NULL == data || 0 == data_size || NULL == out_buf) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (0 != BCryptCreateHash(provider->get_handle(), &h, NULL, 0, NULL, 0, 0)) {
        return false;
    }

    if (0 != BCryptHashData(h, (PUCHAR)data, data_size, 0)) {
        BCryptDestroyHash(h); h = NULL;
        return false;
    }

    if (0 != BCryptFinishHash(h, out_buf, 16, 0)) {
        BCryptDestroyHash(h); h = NULL;
        return false;
    }

    BCryptDestroyHash(h); h = NULL;
    return true;
}

bool NX::crypto::hmac_md5(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _In_reads_bytes_(key_size) const unsigned char* key,
    _In_ unsigned long key_size,
    _Out_writes_bytes_(16) unsigned char* out_buf)
{
    const provider_object*  provider = GET_PROVIDER(PROV_HMAC_MD5);
    NTSTATUS status = 0;
    BCRYPT_HASH_HANDLE  h = NULL;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    if (NULL == data || 0 == data_size || NULL == out_buf || NULL == key || 0 == key_size) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (0 != BCryptCreateHash(provider->get_handle(), &h, NULL, 0, (PUCHAR)key, key_size, 0)) {
        return false;
    }

    if (0 != BCryptHashData(h, (PUCHAR)data, data_size, 0)) {
        BCryptDestroyHash(h); h = NULL;
        return false;
    }

    if (0 != BCryptFinishHash(h, out_buf, 16, 0)) {
        BCryptDestroyHash(h); h = NULL;
        return false;
    }

    BCryptDestroyHash(h); h = NULL;
    return true;
}