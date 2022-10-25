
#include "..\stdafx.h"

#include "..\common\macros.h"
#include "aes.h"
#include "provider.h"


using namespace NX;
using namespace NX::crypt;


AesKey::AesKey() : Key()
{
}

AesKey::~AesKey()
{
}

SDWLResult AesKey::GenerateRandom(PUCHAR data, ULONG size)
{
    //Provider* prov = GetProvider(PROV_AES);
    //if (NULL == prov) {
    //    return RESULT(ERROR_INVALID_HANDLE);
    //}
    //if (!prov->Opened()) {
    //    return RESULT(ERROR_INVALID_HANDLE);
    //}

    if (size != 16 && size != 24 && size != 32) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

    memset(data, 0, size);
    LONG status = BCryptGenRandom(NULL, data, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (0 != status) {
        return RESULT2(status, "BCryptGenRandom failed");
    }

	return RESULT(0);
}

SDWLResult AesKey::Generate(ULONG bitslength)
{
    Provider* prov = GetProvider(PROV_AES);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (bitslength != 128 && bitslength != 192 && bitslength != 256) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

    UCHAR key[32];
    memset(key, 0, 32);
    LONG status = BCryptGenRandom(*prov, key, 32, 0);
    if (0 != status) {
        return RESULT2(status, "BCryptGenRandom failed");
    }

    return Import(key, bitslength/8);
}

SDWLResult AesKey::Import(const UCHAR* key, ULONG size)
{
    Provider* prov = GetProvider(PROV_AES);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (size != 16 && size != 24 && size != 32) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

	Close();


    const ULONG KeyblobSize = (ULONG)sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + size;
    std::vector<UCHAR>  buf;
    buf.resize(KeyblobSize, 0);

    BCRYPT_KEY_DATA_BLOB_HEADER* header = (BCRYPT_KEY_DATA_BLOB_HEADER*)buf.data();
    ((BCRYPT_KEY_DATA_BLOB_HEADER*)buf.data())->cbKeyData = (ULONG)size;
    ((BCRYPT_KEY_DATA_BLOB_HEADER*)buf.data())->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    ((BCRYPT_KEY_DATA_BLOB_HEADER*)buf.data())->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    memcpy(buf.data() + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), key, size);

    LONG status = BCryptImportKey(*prov, NULL, BCRYPT_KEY_DATA_BLOB, &_h, NULL, 0, buf.data(), KeyblobSize, 0);
    if (0 != status) {
        return RESULT2(status, "BCryptImportKey failed");
    }

    _bitslen = size * 8;
    return RESULT(0);
}

SDWLResult AesKey::Export(PUCHAR key, _Inout_ PULONG size)
{
    Provider* prov = GetProvider(PROV_AES);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (Empty()) {
        return RESULT(ERROR_INVALID_DATA);
    }

    const ULONG KeySize = _bitslen / 8;

    if (*size < KeySize) {
        return RESULT(ERROR_BUFFER_OVERFLOW);
    }

    const ULONG KeyblobSize = (ULONG)sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + KeySize;
    DWORD cbResult = 0;
    std::vector<UCHAR>  buf;
    buf.resize(KeyblobSize, 0);

    LONG status = BCryptExportKey(_h, NULL, BCRYPT_KEY_DATA_BLOB, buf.data(), KeyblobSize, &cbResult, 0);
    if (0 != status) {
        return RESULT2(status, "BCryptExportKey failed");
    }

    memcpy(key, buf.data() + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), KeySize);
    *size = KeySize;
    return RESULT(0);
}

SDWLResult NX::crypt::AesEncrypt(_In_ const AesKey& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec)
{
    Provider* prov = GetProvider(PROV_AES);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (key.Empty()) {
        return RESULT2(ERROR_INVALID_PARAMETER, "Invalid key");
    }

    const ULONG key_size = key.GetBitsLength() / 8;
    const ULONG required_out_size = ROUND_TO_SIZE(in_size, 16);
    const ULONG out_buffer_size = *out_size;

    if (NULL == out_buf || 0 == out_buffer_size) {
        *out_size = required_out_size;
        return RESULT(0);
    }

    if (out_buffer_size < required_out_size) {
        *out_size = required_out_size;
        return RESULT(ERROR_BUFFER_OVERFLOW);
    }

    UCHAR ivec_buf[16] = { 0 };
    if (ivec)
        memcpy(ivec_buf, ivec, 16);
    else
        memset(ivec_buf, 0, 16);

    LONG status = BCryptEncrypt(key, (PUCHAR)in_buf, in_size, NULL, ivec_buf, 16, out_buf, required_out_size, out_size, 0);
    if (0 != status) {
        *out_size = 0;
        return RESULT2(status, "BCryptEncrypt failed");
    }

    return RESULT(0);
}

SDWLResult NX::crypt::AesDecrypt(_In_ const AesKey& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec)
{
    Provider* prov = GetProvider(PROV_AES);
    if (NULL == prov) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (!prov->Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    if (key.Empty()) {
        return RESULT2(ERROR_INVALID_PARAMETER, "Invalid key");
    }

    if (!IS_ALIGNED(in_size, 16)) {
        // input data is NOT aligned with key size
        return RESULT2(ERROR_INVALID_PARAMETER, "Input data is not aligned");
    }

    const ULONG key_size = key.GetBitsLength() / 8;
    const ULONG required_out_size = ROUND_TO_SIZE(in_size, 16);
    const ULONG out_buffer_size = *out_size;

    if (NULL == out_buf || 0 == out_buffer_size) {
        *out_size = required_out_size;
        return RESULT(0);
    }

    if (out_buffer_size < required_out_size) {
        *out_size = required_out_size;
        return RESULT(ERROR_BUFFER_OVERFLOW);
    }

    UCHAR ivec_buf[16] = { 0 };
    if (ivec)
        memcpy(ivec_buf, ivec, 16);
    else
        memset(ivec_buf, 0, 16);

    LONG status = BCryptDecrypt(key, (PUCHAR)in_buf, in_size, NULL, ivec_buf, 16, out_buf, out_buffer_size, out_size, 0);
    if (0 != status) {
        *out_size = 0;
        return RESULT2(status, "BCryptDecrypt failed");
    }

    return RESULT(0);
}

void NX::crypt::AesGenerateIvec(_In_reads_bytes_opt_(16) const UCHAR* seed, unsigned __int64 offset, _Out_writes_bytes_(16) PUCHAR ivec)
{
    if (NULL == seed) {
        memset(ivec, 0, 16);
        memcpy(ivec, &offset, 8);
    }
    else {
        memcpy(ivec, seed, 16);
        if (offset) {
            offset = (offset - 1) * 31;
        }
        ((unsigned __int64*)ivec)[0] ^= offset;
        ((unsigned __int64*)ivec)[1] ^= offset;
    }
}

SDWLResult NX::crypt::AesBlockEncrypt(_In_ const AesKey& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
    _In_ const unsigned __int64 block_offset,
    _In_ const unsigned long cipher_block_size)
{
    if (0 == cipher_block_size || !IS_ALIGNED(cipher_block_size, 512)) {
        return RESULT2(ERROR_INVALID_PARAMETER, "Invalid cipher block size");
    }

    if (!IS_ALIGNED(block_offset, cipher_block_size)) {
        return RESULT2(ERROR_INVALID_PARAMETER, "Block offset is not aligned with cipher block size");
    }

    const ULONG required_out_size = ROUND_TO_SIZE(in_size, 16);
    ULONG out_buffer_size = *out_size;

    if (NULL == out_buf || 0 == *out_size) {
        *out_size = required_out_size;
        return RESULT(0);
    }

    if (out_buffer_size < required_out_size) {
        *out_size = required_out_size;
        return RESULT(ERROR_BUFFER_OVERFLOW);
    }

	*out_size = 0;

	unsigned __int64 currentOffset = block_offset;
    while (in_size) {

        const ULONG bytes_to_encrypt = min(in_size, cipher_block_size);
        ULONG output_size = out_buffer_size;
        UCHAR ivec[16] = { 0 };
        AesGenerateIvec(ivec_seed, currentOffset, ivec);
        SDWLResult res = AesEncrypt(key, in_buf, bytes_to_encrypt, out_buf, &output_size, ivec);
        if (!res)
            return res;

        in_buf += bytes_to_encrypt;
        out_buf += output_size;
        in_size -= bytes_to_encrypt;
        out_buffer_size -= bytes_to_encrypt;
        *out_size = *out_size + output_size;
		currentOffset += cipher_block_size;
    }

    return RESULT(0);
}

SDWLResult NX::crypt::AesBlockDecrypt(_In_ const AesKey& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
    _In_ const unsigned __int64 block_offset,
    _In_ const unsigned long cipher_block_size)
{
    if (0 == cipher_block_size || !IS_ALIGNED(cipher_block_size, 512)) {
        return RESULT2(ERROR_INVALID_PARAMETER, "Invalid cipher block size");
    }

    if (!IS_ALIGNED(block_offset, cipher_block_size)) {
        return RESULT2(ERROR_INVALID_PARAMETER, "Block offset is not aligned with cipher block size");
    }

    if (!IS_ALIGNED(in_size, 16)) {
        // input data is NOT aligned with key size
        return RESULT2(ERROR_INVALID_PARAMETER, "Input data is not aligned");
    }

    const ULONG required_out_size = ROUND_TO_SIZE(in_size, 16);
    ULONG out_buffer_size = *out_size;

    if (NULL == out_buf || 0 == *out_size) {
        *out_size = required_out_size;
        return RESULT(0);
    }

    if (out_buffer_size < required_out_size) {
        *out_size = required_out_size;
        return RESULT(ERROR_BUFFER_OVERFLOW);
    }

	*out_size = 0;

	unsigned __int64 currentOffset = block_offset;
    while (in_size) {

        const ULONG bytes_to_decrypt = min(in_size, cipher_block_size);
        ULONG output_size = out_buffer_size;
        UCHAR ivec[16] = { 0 };
        AesGenerateIvec(ivec_seed, currentOffset, ivec);
        SDWLResult res = AesDecrypt(key, in_buf, bytes_to_decrypt, out_buf, &output_size, ivec);
        if (!res)
            return res;

        in_buf += bytes_to_decrypt;
        out_buf += output_size;
        in_size -= bytes_to_decrypt;
        out_buffer_size -= bytes_to_decrypt;
        *out_size = *out_size + output_size;
		currentOffset += cipher_block_size;
    }

    return RESULT(0);
}