

#ifndef __NX_CRYPTO_CERTIFICATE_HPP__
#define __NX_CRYPTO_CERTIFICATE_HPP__


#include <Bcrypt.h>
#include <Wincrypt.h>


#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include <nudf\crypto_common.hpp>

namespace NX {
    
namespace cert {


class cert_info
{
public:
    cert_info();
    cert_info(const cert_info& other);
    cert_info(cert_info&& other);
    ~cert_info();

    cert_info& operator = (const cert_info& other);
    cert_info& operator = (cert_info&& other);

    inline const std::wstring& get_subject_name() const { return _subject_name; }
    inline const std::wstring& get_issuer_name() const { return _issuer_name; }
    inline const std::wstring& get_friendly_name() const { return _friendly_name; }
    inline const std::wstring& get_sign_algorithm() const { return _sign_algorithm; }
    inline const std::wstring& get_hash_algorithm() const { return _hash_algorithm; }
    inline const std::vector<unsigned char>& get_serial_number() const { return _serial_number; }
    inline const std::vector<unsigned char>& get_thumbprint() const { return _thumbprint; }
    inline const std::vector<unsigned char>& get_signature_hash() const { return _signature_hash; }
    inline const std::vector<unsigned char>& get_key_blob() const { return _key_blob; }
    inline const FILETIME& get_date_stamp() const { return _date_stamp; }
    inline const FILETIME& get_valid_from() const { return _valid_from; }
    inline const FILETIME& get_valid_through() const { return _valid_through; }
    inline unsigned long get_key_spec() const { return _key_spec; }
    inline unsigned long get_key_bits_length() const { return _key_bits_length; }
    inline bool is_self_signed() const { return _self_signed; }

    bool is_legacy_rsa_public_key_blob() const;
    bool is_legacy_rsa_private_key_blob() const;

private:
    void _Move_to(cert_info& target);
    void _Copy_to(cert_info& target) const;

private:
    std::wstring    _subject_name;
    std::wstring    _issuer_name;
    std::wstring    _friendly_name;
    std::wstring    _sign_algorithm;
    std::wstring    _hash_algorithm;
    std::vector<unsigned char> _serial_number;
    std::vector<unsigned char> _thumbprint;
    std::vector<unsigned char> _signature_hash;
    std::vector<unsigned char> _key_blob;
    FILETIME        _date_stamp;
    FILETIME        _valid_from;
    FILETIME        _valid_through;
    unsigned long   _key_spec;
    unsigned long   _key_bits_length;
    bool            _self_signed;

    friend class cert_context;
};

class cert_context
{
public:
    cert_context();
    cert_context(PCCERT_CONTEXT p);
    virtual ~cert_context();


    inline operator PCCERT_CONTEXT() { return _p; }
    inline PCCERT_CONTEXT attach(PCCERT_CONTEXT p) { _p = p;  return _p; }
    inline PCCERT_CONTEXT detach() { PCCERT_CONTEXT p = _p;  _p = nullptr;  return p; }
    inline bool empty() const { return (nullptr == _p); }

    bool create(const std::wstring& x500_name, unsigned long key_spec, bool strong, _In_opt_ cert_context* issuer, const SYSTEMTIME& expire_date);
    bool create(const unsigned char* pb, unsigned long cb);
    bool create(const std::wstring& file);
    bool export_to_file(_In_ const std::wstring& file, _In_ bool base64_format);
    void clear();

    cert_info get_cert_info();
    //
    // certificate properties
    //

    // --> provider/key/algorithm
//    NX::crypto::CRYPT32::crypt_provider get_key_provider() const;
//    NX::crypto::CRYPT32::crypt_provider_info get_key_provider_info() const;
//    unsigned long get_key_spec() const;
//    unsigned long get_public_key_length() const;
//    std::vector<unsigned char> get_public_key_blob() const;
//    NX::crypto::CRYPT32::public_key_blob get_public_key() const;
//    std::vector<unsigned char> get_private_key_blob() const;
//    NX::crypto::CRYPT32::private_key_blob get_private_key() const;
//    bool has_private_key() const;
//    bool is_self_signed() const;
//
//    // --> cert information
//    std::vector<unsigned char> get_serial_number() const;
//    std::vector<unsigned char> get_thumb_print() const;
//    std::wstring get_issuer_name() const;
//    std::wstring get_subject_name() const;
//    FILETIME get_date_stamp() const;
//    std::wstring get_friendly_name() const;
//    FILETIME get_start_time() const;
//    FILETIME get_expire_time() const;
//    std::wstring get_signature_algorithm() const;
//    std::wstring get_hash_algorithm() const;
//    std::vector<unsigned char> get_signature_hash() const;
//
private:
    // Get Properties
    std::wstring get_subject_name();
    std::wstring get_issuer_name();
    std::wstring get_friendly_name();
    void get_algorithms(std::wstring& sign_algorithm, std::wstring& hash_algorithm);
    std::vector<unsigned char> get_serial_number();
    std::vector<unsigned char> get_thumbprint();
    std::vector<unsigned char> get_signature_hash();
    std::vector<unsigned char> get_key_blob();
    void get_date_stamp(FILETIME& date_stamp);
    void get_valid_time(FILETIME& valid_from, FILETIME& valid_through);
    unsigned long get_key_spec();
    unsigned long get_key_bits_length();
    bool is_self_signed();
    bool has_private_key();

    std::vector<unsigned char> get_private_key_blob();
    std::vector<unsigned char> get_public_key_blob();

    std::vector<unsigned char> get_property(unsigned long id);
    std::vector<unsigned char> decode_object(unsigned long encoding_type, const char* struc_type, const unsigned char* data, unsigned long data_size, unsigned long flags);

private:
//    void set_property(unsigned long id, const void* pb, unsigned long cb);
//    std::vector<unsigned char> get_property(unsigned long id) const;
//    unsigned long get_property_size(unsigned long id) const;
//    std::vector<unsigned char> decode_object(unsigned long encoding_type, const char* struc_type, const unsigned char* data, unsigned long data_size, unsigned long flags) const;
    bool is_pe_file(const std::wstring& file);
    bool create_from_pe_file(const std::wstring& file);
    std::vector<unsigned char> get_cert_blob(const std::wstring& file);
    std::vector<unsigned char> get_cert_binary_blob(const std::wstring& file);

private:
    // No copy allowed
    cert_context& operator = (const cert_context& other) { return *this; }
    // No move allowed
    cert_context& operator = (cert_context&& other) { return *this; }

private:
    PCCERT_CONTEXT  _p;
};

class store
{
public:
    virtual ~store();

    inline operator HCERTSTORE() { return _h; }
    inline bool opened() const { return (NULL != _h); }
    inline HCERTSTORE attach(HCERTSTORE h) { _h = h; return _h; }
    inline HCERTSTORE detach() { HCERTSTORE dh = _h; _h = NULL; return dh; }

    virtual void close();

protected:
    store();
    store(HCERTSTORE h);

private:
    // No copy is allowed
    store& operator = (const store& other) { return *this; }
    // No move is allowed
    store& operator = (store&& other) { return *this; }

private:
    HCERTSTORE  _h;
};

class system_store : public store
{
public:
    system_store(const std::wstring& name, bool machine_store);
    virtual ~system_store();
protected:
    void open(const std::wstring& name, bool machine_store);
};

class system_personal_store : public system_store
{
public:
    system_personal_store(bool machine_store);
    virtual ~system_personal_store() {}
};

class system_trust_root_store : public system_store
{
public:
    system_trust_root_store(bool machine_store);
    virtual ~system_trust_root_store() {}
};

class system_intermediate_store : public system_store
{
public:
    system_intermediate_store(bool machine_store);
    virtual ~system_intermediate_store() {}
};

class system_trustpublisher_store : public system_store
{
public:
    system_trustpublisher_store(bool machine_store);
    virtual ~system_trustpublisher_store() {}
};

class system_trustpeople_store : public system_store
{
public:
    system_trustpeople_store(bool machine_store);
    virtual ~system_trustpeople_store() {}
};

class system_otherpeople_store : public system_store
{
public:
    system_otherpeople_store(bool machine_store);
    virtual ~system_otherpeople_store() {}
};

class system_thirdparty_root_store : public system_store
{
public:
    system_thirdparty_root_store(bool machine_store);
    virtual ~system_thirdparty_root_store() {}
};

class system_revoked_store : public system_store
{
public:
    system_revoked_store(bool machine_store);
    virtual ~system_revoked_store() {}
};


class memory_store : public store
{
public:
    memory_store();
    virtual ~memory_store() {}
};

class pcks12_file_store : public store
{
public:
    pcks12_file_store();
    pcks12_file_store(const std::wstring& file, const wchar_t* password);
    virtual ~pcks12_file_store() {}
    bool open(const std::wstring& file, const wchar_t* password);

protected:
    std::vector<unsigned char> get_cert_blob(const std::wstring& file);
};



}   // NX::cert
}   // NX


#endif