

#ifndef __NX_CRYPTO_COMMON_HPP__
#define __NX_CRYPTO_COMMON_HPP__


#include <Bcrypt.h>
#include <Wincrypt.h>


#include <string>
#include <vector>
#include <algorithm>
#include <memory>


namespace NX {

namespace crypto {


typedef enum PROVIDER_ID {
    PROV_AES = 0,       /**< AES Provider    (0) */
    PROV_RSA,           /**< RSA Provider    (1) */
    PROV_DH,            /**< Diffie-Hellman Provider (2) */
    PROV_MD5,           /**< MD5 Provider    (3) */
    PROV_SHA1,          /**< SHA1 Provider   (4) */
    PROV_SHA256,        /**< SHA256 Provider (5) */
    PROV_HMAC_MD5,      /**< MD5 Provider    (6) */
    PROV_HMAC_SHA1,     /**< SHA1 Provider   (7) */
    PROV_HMAC_SHA256,   /**< SHA256 Provider (8) */
    PROV_MAX            /**< Maximum Provider Id Value  (9) */
} PROVIDER_ID;

class provider_object
{
public:
    provider_object(const std::wstring& name, unsigned long flags, bool is_hash_alg);
    ~provider_object();

    inline BCRYPT_ALG_HANDLE get_handle() const { return _h; }
    inline const std::wstring& get_name() const { return _name; }
    inline unsigned long get_flags() const { return _flags; }
    inline unsigned long get_object_length() const { return _object_length; }
    inline unsigned long get_hash_length() const { return _hash_length; }

    inline bool empty() const { return (NULL == _h); }
    inline bool is_first_time_try() const { return _first_time_try; }

    bool init();
    void clear();

private:
    provider_object(const provider_object& other) {}    // no copy
    provider_object(provider_object&& other) {}         // no move
    provider_object& operator = (const provider_object& other) { return *this; }

private:
    BCRYPT_ALG_HANDLE   _h;
    std::wstring  _name;
    unsigned long   _flags;
    bool    _is_hash_alg;
    unsigned long   _object_length;
    unsigned long   _hash_length;
    bool _first_time_try;
};

const provider_object* GET_PROVIDER(PROVIDER_ID id);

class basic_key
{
public:
    basic_key();
    virtual ~basic_key();

    virtual void clear();
    virtual bool generate(unsigned long bits_length) = 0;
    virtual bool export_key(std::vector<unsigned char>& key) = 0;
    virtual bool export_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size) = 0;
    virtual bool import_key(const std::vector<unsigned char>& key) = 0;
    virtual bool import_key(_In_reads_bytes_(size) const unsigned char* key, size_t size) = 0;

    inline bool empty() const { return (NULL == _h); }
    inline unsigned long get_bits_length() const { return _bits_length; }
    inline BCRYPT_KEY_HANDLE get_key() const { return _h; }
    
protected:
    inline void set_key_handle(BCRYPT_KEY_HANDLE h) { _h = h; }
    inline void set_bits_length(unsigned long len) { _bits_length = len; }

    bool query_bits_length();

private:
    basic_key(const basic_key& other) {}    // no copy
    basic_key(basic_key&& other) {}         // no move
    basic_key& operator = (const basic_key& other) { return *this; }    // no copy

private:
    unsigned long   _bits_length;
    BCRYPT_KEY_HANDLE _h;
};



}   // NX::crypto
}   // NX


#endif