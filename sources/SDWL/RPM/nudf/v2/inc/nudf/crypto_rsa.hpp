

#ifndef __NX_CRYPTO_RSA_HPP__
#define __NX_CRYPTO_RSA_HPP__


#include <Bcrypt.h>
#include <Wincrypt.h>


#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include <nudf\crypto_common.hpp>

namespace NX {

namespace crypto {


class rsa_key : public basic_key
{
public:
    rsa_key();
    virtual ~rsa_key();

    inline bool has_private_key() const { return (_key_type > RSAPUBLIC); }

    virtual void clear();
    virtual bool generate(unsigned long bits_length);
    virtual bool export_key(std::vector<unsigned char>& key);
    virtual bool export_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size);
    virtual bool import_key(const std::vector<unsigned char>& key);
    virtual bool import_key(_In_reads_bytes_(size) const unsigned char* key, size_t size);

    bool export_public_key(std::vector<unsigned char>& key);
    bool export_public_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size);

    typedef enum RSAKEYTYPE {
        RSAEMPTY = 0,
        RSAPUBLIC,
        RSAPRIVATE,
        RSAFULLPRIVATE
    } RSAKEYTYPE;

private:
    bool import_bcrypt_key(_In_reads_bytes_(size) const unsigned char* key, size_t size);
    bool import_legacy_key(_In_reads_bytes_(size) const unsigned char* key, size_t size);
    bool is_legacy_key(_In_reads_bytes_(size) const unsigned char* key, size_t size);

private:
    RSAKEYTYPE      _key_type;
    unsigned long   _key_spec;
};


bool rsa_encrypt(const rsa_key& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size);

bool rsa_decrypt(const rsa_key& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size);


bool rsa_sign(const rsa_key& key,
    _In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _Out_writes_bytes_opt_(*signature_size) unsigned char* signature,
    _Inout_ unsigned long* signature_size);

bool rsa_verify(const rsa_key& key,
    _In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _In_reads_bytes_(signature_size) const unsigned char* signature,
    _In_ unsigned long signature_size);

bool rsasha256_sign(const std::vector<unsigned char>& pkcs8_key,
	_In_reads_bytes_(data_size) const unsigned char* data,
	_In_ unsigned long data_size,
	_Out_writes_bytes_opt_(*signature_size) unsigned char* signature,
	_Inout_ unsigned long* signature_size);


}   // NX::crypto
}   // NX


#endif