

#ifndef __NX_CRYPTO_DIFFIE_HELLMAN_HPP__
#define __NX_CRYPTO_DIFFIE_HELLMAN_HPP__


#include <Bcrypt.h>
#include <Wincrypt.h>


#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include <nudf\crypto_common.hpp>

namespace NX {

namespace crypto {


class diffie_hellman_key;


class diffie_hellman_key_blob
{
public:
    diffie_hellman_key_blob();
    diffie_hellman_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y);
    diffie_hellman_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y, const std::vector<unsigned char>& x);
    diffie_hellman_key_blob(const diffie_hellman_key_blob& other);
    diffie_hellman_key_blob(diffie_hellman_key_blob&& other);
    virtual ~diffie_hellman_key_blob();

    inline bool empty() const { return _data.empty(); }
    inline void clear() { _data.clear(); }

    diffie_hellman_key_blob& operator = (const diffie_hellman_key_blob& other);
    diffie_hellman_key_blob& operator = (diffie_hellman_key_blob&& other);

    bool is_private() const;
    unsigned long get_key_length() const;
    unsigned long get_key_bitlength() const;
    std::vector<unsigned char> get_p() const;
    std::vector<unsigned char> get_g() const;
    std::vector<unsigned char> get_x() const;
    std::vector<unsigned char> get_y() const;

    bool encode_public_key(std::vector<unsigned char>& encoded_pubkey);
    bool encode_public_key(std::wstring& encoded_pubkey);
    bool decode_public_key(const std::vector<unsigned char>& encoded_pubkey);
    bool decode_public_key(const std::wstring& encoded_pubkey);

private:
    bool set_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y);
    bool set_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y, const std::vector<unsigned char>& x);

    const unsigned char* get_p_ptr() const;
    const unsigned char* get_g_ptr() const;
    const unsigned char* get_y_ptr() const;
    const unsigned char* get_x_ptr() const;

private:
    std::vector<unsigned char> _data;

    friend class diffie_hellman_key;
};

class diffie_hellman_key : public basic_key
{
public:
    diffie_hellman_key();
    virtual ~diffie_hellman_key();

    virtual void clear();
    virtual bool generate(_In_reads_bytes_(bits_length / 8) const unsigned char* p, _In_reads_bytes_(bits_length / 8) const unsigned char* g, _In_ unsigned long bits_length);
    virtual bool export_key(std::vector<unsigned char>& key);
    virtual bool export_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size);
    virtual bool import_key(const std::vector<unsigned char>& key);
    virtual bool import_key(_In_reads_bytes_(size) const unsigned char* key, size_t size);

    inline bool has_private_key() const { return _full_key; }

    static bool get_y_from_cert_file(const std::wstring& file, std::vector<unsigned char>& y);
    static bool get_y_from_cert(const std::wstring& base64_cert, std::vector<unsigned char>& y);
    static bool get_y_from_cert(const unsigned char* pbcert, unsigned long cbcert, std::vector<unsigned char>& y);

    diffie_hellman_key_blob export_key();
    diffie_hellman_key_blob export_public_key();
    bool export_public_key(std::vector<unsigned char>& key);

protected:
    virtual bool generate(unsigned long bits_length) { return false; }
    bool export_key_ex(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size, _In_ bool full_key);

private:

    bool _full_key;
};


}   // NX::crypto
}   // NX


#endif