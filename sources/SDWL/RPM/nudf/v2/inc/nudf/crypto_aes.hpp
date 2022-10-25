

#ifndef __NX_CRYPTO_AES_HPP__
#define __NX_CRYPTO_AES_HPP__


#include <Bcrypt.h>
#include <Wincrypt.h>


#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include <nudf\crypto_common.hpp>

namespace NX {

namespace crypto {


class aes_key : public basic_key
{
public:
    aes_key();
    virtual ~aes_key();

    static bool generate_random_aes_key(_Out_writes_bytes_(32) unsigned char* keybuf);
    static std::vector<unsigned char> generate_random_aes_key();

    virtual bool generate(unsigned long bits_length);
    virtual bool export_key(std::vector<unsigned char>& key);
    virtual bool export_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size);
    virtual bool import_key(const std::vector<unsigned char>& key_or_blob);
    virtual bool import_key(_In_reads_bytes_(size) const unsigned char* key_or_blob, size_t size);

protected:
    bool import_key_blob(_In_reads_bytes_(size) const unsigned char* key_blob, size_t size);
    bool import_key_only(_In_reads_bytes_(size) const unsigned char* key, size_t size);
    bool export_key_blob(std::vector<unsigned char>& keyblob);
};

bool aes_encrypt(_In_ const aes_key& key,
                 _In_reads_bytes_(in_size) const unsigned char* in_buf,
                 _In_ unsigned long in_size,
                 _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
                 _Inout_ unsigned long* out_size,
                 _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
                 _In_ const unsigned __int64 block_offset = 0,
                 _In_ unsigned long cipher_block_size = 0);

bool aes_encrypt(_In_ const aes_key& key,
                 _Inout_updates_bytes_(size) unsigned char* buf,
                 _In_ unsigned long size,
                 _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
                 _In_ const unsigned __int64 block_offset = 0,
                 _In_ unsigned long cipher_block_size = 0);

bool aes_decrypt(_In_ const aes_key& key,
                 _In_reads_bytes_(in_size) const unsigned char* in_buf,
                 _In_ unsigned long in_size,
                 _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
                 _Inout_ unsigned long* out_size,
                 _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
                 _In_ const unsigned __int64 block_offset = 0,
                 _In_ unsigned long cipher_block_size = 0);

bool aes_decrypt(_In_ const aes_key& key,
                 _Inout_updates_bytes_(size) unsigned char* buf,
                 _Inout_ unsigned long size,
                 _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
                 _In_ const unsigned __int64 block_offset = 0,
                 _In_ unsigned long cipher_block_size = 0);

}   // NX::crypto
}   // NX


#endif