
#include <Windows.h>
#include <assert.h>

#include <nudf\crypto_aes.hpp>
#include <nudf\handyutil.hpp>


using namespace NX;
using namespace NX::crypto;




static const size_t keyblob_size_16 = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 16;
static const size_t keyblob_size_24 = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 24;
static const size_t keyblob_size_32 = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 32;


aes_key::aes_key() : basic_key()
{
}

aes_key::~aes_key()
{
}

bool aes_key::generate_random_aes_key(_Out_writes_bytes_(32) unsigned char* keybuf)
{
    LONG status = 0;
    status = BCryptGenRandom(NULL, keybuf, 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return (0 == status);
}

std::vector<unsigned char> aes_key::generate_random_aes_key()
{
    std::vector<unsigned char> keybuf;
    keybuf.resize(32, 0);
    if (!generate_random_aes_key(keybuf.data())) {
        keybuf.clear();
    }
    return std::move(keybuf);
}

bool aes_key::generate(unsigned long bits_length)
{
    const provider_object*  provider = GET_PROVIDER(PROV_AES);
    NTSTATUS status = 0;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    if (bits_length != 128 && bits_length != 192 && bits_length != 256) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    BCRYPT_KEY_HANDLE hkey = NULL;
    std::vector<unsigned char> keybuf;
    const size_t key_size = bits_length / 8;
    keybuf.resize(32, 0);
    if (!aes_key::generate_random_aes_key(keybuf.data())) {
        return false;
    }

    return import_key_only(keybuf.data(), key_size);
}

bool aes_key::export_key(std::vector<unsigned char>& key)
{
    std::vector<unsigned char> key_blob;

    if (!export_key_blob(key_blob)) {
        return false;
    }

    const BCRYPT_KEY_DATA_BLOB_HEADER* p = (const BCRYPT_KEY_DATA_BLOB_HEADER*)key_blob.data();

    assert(BCRYPT_KEY_DATA_BLOB_MAGIC == p->dwMagic);
    assert(BCRYPT_KEY_DATA_BLOB_VERSION1 == p->dwVersion);
    assert(16 == p->cbKeyData || 24 == p->cbKeyData || 32 == p->cbKeyData);

    key.resize(p->cbKeyData, 0);
    memcpy(key.data(), p + 1, p->cbKeyData);
    return true;
}

bool aes_key::export_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size)
{
    std::vector<unsigned char> key_blob;

    if (NULL == size) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (!export_key_blob(key_blob)) {
        return false;
    }


    const BCRYPT_KEY_DATA_BLOB_HEADER* p = (const BCRYPT_KEY_DATA_BLOB_HEADER*)key_blob.data();

    assert(BCRYPT_KEY_DATA_BLOB_MAGIC == p->dwMagic);
    assert(BCRYPT_KEY_DATA_BLOB_VERSION1 == p->dwVersion);
    assert(16 == p->cbKeyData || 24 == p->cbKeyData || 32 == p->cbKeyData);

    if (NULL == key || 0 == *size) {
        *size = p->cbKeyData;
        return true;
    }

    assert(NULL != key);

    if (p->cbKeyData > *size) {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return false;
    }

    memcpy(key, p + 1, p->cbKeyData);
    *size = p->cbKeyData;
    return true;
}

bool aes_key::import_key(const std::vector<unsigned char>& key_or_blob)
{
    return import_key(key_or_blob.data(), key_or_blob.size());
}

bool aes_key::import_key(_In_reads_bytes_(size) const unsigned char* key_or_blob, size_t size)
{

    if (size == 16 || size == 24 || size == 32) {
        return import_key_only(key_or_blob, size);
    }
    else if (size == keyblob_size_16 || size == keyblob_size_24 || size == keyblob_size_32) {
        return import_key_blob(key_or_blob, size);
    }
    else {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
}

bool aes_key::import_key_blob(_In_reads_bytes_(size) const unsigned char* key_blob, size_t size)
{
    const provider_object*  provider = GET_PROVIDER(PROV_AES);
    NTSTATUS status = 0;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    
    if (size != keyblob_size_16 && size != keyblob_size_24 && size != keyblob_size_32) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    const BCRYPT_KEY_DATA_BLOB_HEADER* p = (const BCRYPT_KEY_DATA_BLOB_HEADER*)key_blob;
    if (BCRYPT_KEY_DATA_BLOB_MAGIC != p->dwMagic || BCRYPT_KEY_DATA_BLOB_VERSION1 != p->dwVersion) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    if (p->cbKeyData != 16 && p->cbKeyData != 24 && p->cbKeyData != 32) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    BCRYPT_KEY_HANDLE hkey = NULL;
    const unsigned long bits_length = (unsigned long)(p->cbKeyData * 8);

    status = BCryptImportKey(provider->get_handle(), NULL, BCRYPT_KEY_DATA_BLOB, &hkey, NULL, 0, (PUCHAR)key_blob, (unsigned long)size, 0);
    if (0 != status) {
        return false;
    }

    set_key_handle(hkey);
    set_bits_length(bits_length);
    return true;
}

bool aes_key::import_key_only(_In_reads_bytes_(size) const unsigned char* key, size_t size)
{
    const provider_object*  provider = GET_PROVIDER(PROV_AES);
    NTSTATUS status = 0;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    if (size != 16 && size != 24 && size != 32) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    BCRYPT_KEY_HANDLE hkey = NULL;
    std::vector<unsigned char> aes_blob;

    aes_blob.resize(sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 32, 0);
    BCRYPT_KEY_DATA_BLOB_HEADER* p = (BCRYPT_KEY_DATA_BLOB_HEADER*)aes_blob.data();
    p->cbKeyData = (ULONG)size;
    p->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    p->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    memcpy(aes_blob.data() + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), key, size);

    status = BCryptImportKey(provider->get_handle(), NULL, BCRYPT_KEY_DATA_BLOB, &hkey, NULL, 0, aes_blob.data(), (unsigned long)aes_blob.size(), 0);
    if (0 != status) {
        return false;
    }

    set_key_handle(hkey);
    set_bits_length((unsigned long)(8 * size));
    return true;
}

bool aes_key::export_key_blob(std::vector<unsigned char>& keyblob)
{
    NTSTATUS status = 0;
    ULONG cbResult = 0;
    
    if (empty()) {
        SetLastError(ERROR_NOT_FOUND);
        return false;
    }

    status = BCryptExportKey(get_key(), NULL, BCRYPT_KEY_DATA_BLOB, NULL, 0, &cbResult, 0);
    if (0 == cbResult) {
        if (0 != GetLastError()) {
            SetLastError(ERROR_NOT_FOUND);
        }
        return false;
    }

    keyblob.resize(cbResult, 0);
    status = BCryptExportKey(get_key(), NULL, BCRYPT_KEY_DATA_BLOB, keyblob.data(), (ULONG)keyblob.size(), &cbResult, 0);
    if (0 != status) {
        keyblob.clear();
        return false;
    }

    return true;
}


static void gen_ivec(_In_reads_bytes_opt_(16) const unsigned char* seed, unsigned __int64 offset, _Out_writes_bytes_(16) unsigned char* ivec)
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


bool NX::crypto::aes_encrypt(_In_ const aes_key& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
    _In_ const unsigned __int64 block_offset,
    _In_ unsigned long cipher_block_size)
{
    const unsigned long key_size = key.get_bits_length() / 8;
    const unsigned long required_out_size = RoundToSize32(in_size, key_size);
    unsigned __int64 ivec_offset = block_offset;
    const unsigned long out_buffer_size = *out_size;
    unsigned long bytes_remained = *out_size;
    *out_size = 0;

    if (key.empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    if (NULL == out_buf || 0 == out_buffer_size) {
        *out_size = required_out_size;
        return true;
    }

    if (out_buffer_size < required_out_size) {
        *out_size = required_out_size;
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return false;
    }

    if (0 == cipher_block_size) {
        cipher_block_size = 512;
    }
    else {
        cipher_block_size = RoundToSize32(cipher_block_size, 512);
    }

    while (in_size != 0) {

        const unsigned long bytes_to_encrypt = (in_size > cipher_block_size) ? cipher_block_size : in_size;
        const unsigned long bytes_encrypt_required = RoundToSize32(bytes_to_encrypt, key_size);
        unsigned long bytes_encrypted = 0;
        unsigned char ivec[16] = { 0 };

        assert(bytes_encrypt_required <= bytes_remained);
        if (bytes_encrypt_required > bytes_remained) {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return false;
        }

        gen_ivec(ivec_seed, ivec_offset, ivec);

        NTSTATUS status = BCryptEncrypt(key.get_key(), (unsigned char*)in_buf, bytes_to_encrypt, NULL, ivec, 16, out_buf, bytes_encrypt_required, &bytes_encrypted, 0);
        if (0 != status) {
            return false;
        }

        assert(bytes_encrypt_required == bytes_encrypted);

        in_buf += bytes_to_encrypt;
        in_size -= bytes_to_encrypt;
        ivec_offset += bytes_to_encrypt;
        out_buf += bytes_encrypted;
        *out_size += bytes_encrypted;
        bytes_remained -= bytes_encrypted;
    }

    return true;
}

bool NX::crypto::aes_encrypt(_In_ const aes_key& key,
    _Inout_updates_bytes_(size) unsigned char* buf,
    _In_ unsigned long size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
    _In_ const unsigned __int64 block_offset,
    _In_ unsigned long cipher_block_size)
{
    return NX::crypto::aes_encrypt(key, buf, size, buf, &size, ivec_seed, block_offset, cipher_block_size);
}

bool NX::crypto::aes_decrypt(_In_ const aes_key& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
    _In_ const unsigned __int64 block_offset,
    _In_ unsigned long cipher_block_size)
{
    const unsigned long key_size = key.get_bits_length() / 8;
    const unsigned long required_out_size = RoundToSize32(in_size, key_size);
    unsigned __int64 ivec_offset = block_offset;
    const unsigned long out_buffer_size = *out_size;
    unsigned long bytes_remained = *out_size;
    *out_size = 0;

    if (key.empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    if (required_out_size != in_size) {
        // input data is NOT aligned with key size
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (NULL == out_buf || 0 == out_buffer_size) {
        *out_size = required_out_size;
        return true;
    }

    if (out_buffer_size < required_out_size) {
        *out_size = required_out_size;
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return false;
    }

    if (0 == cipher_block_size) {
        cipher_block_size = 512;
    }
    else {
        cipher_block_size = RoundToSize32(cipher_block_size, 512);
    }

    
    while (in_size != 0) {

        const unsigned long bytes_to_decrypt = (in_size > cipher_block_size) ? cipher_block_size : in_size;
        unsigned long bytes_decrypted = 0;
        unsigned char ivec[16] = { 0 };

        assert(bytes_to_decrypt <= bytes_remained);
        if (bytes_to_decrypt > bytes_remained) {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return false;
        }

        gen_ivec(ivec_seed, ivec_offset, ivec);

        NTSTATUS status = BCryptDecrypt(key.get_key(), (unsigned char*)in_buf, bytes_to_decrypt, NULL, ivec, 16, out_buf, bytes_to_decrypt, &bytes_decrypted, 0);
        if (0 != status) {
            return false;
        }

        assert(bytes_to_decrypt == bytes_decrypted);

        in_buf += bytes_to_decrypt;
        in_size -= bytes_to_decrypt;
        ivec_offset += bytes_to_decrypt;
        out_buf += bytes_decrypted;
        *out_size += bytes_decrypted;
        bytes_remained -= bytes_decrypted;
    }

    return true;
}

bool NX::crypto::aes_decrypt(_In_ const aes_key& key,
    _Inout_updates_bytes_(size) unsigned char* buf,
    _Inout_ unsigned long size,
    _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
    _In_ const unsigned __int64 block_offset,
    _In_ unsigned long cipher_block_size)
{
    return NX::crypto::aes_decrypt(key, buf, size, buf, &size, ivec_seed, block_offset, cipher_block_size);
}