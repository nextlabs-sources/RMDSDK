
#pragma once
#ifndef __NXRM_KEYMANAGER_HPP__
#define __NXRM_KEYMANAGER_HPP__


#include <ncrypt.h>

#include <string>
#include <vector>
#include <queue>
#include <map>

#include <nudf/crypto.hpp>
#include <nudf/crypto_dh.hpp>

class nxmaster_key
{
public:
    nxmaster_key();
    nxmaster_key(nxmaster_key&& other);
    ~nxmaster_key();

    bool create();
    bool open();
    void clear();
    static bool remove();

    bool generate_cert(const std::wstring& file, std::vector<unsigned char>& thumb_print);

    nxmaster_key& operator = (nxmaster_key&& other);

    inline bool empty() const { return (NULL == _h); }

private:
    nxmaster_key(const nxmaster_key& other);
    nxmaster_key& operator = (const nxmaster_key& other);
    void _Move(nxmaster_key& target);

private:
    NCRYPT_KEY_HANDLE   _h;
};

// Diffie-Hellman Key Exchange
NX::crypto::diffie_hellman_key_blob generate_dh_keyblob();


class encrypt_token
{
public:
    encrypt_token();
    // encrypt_token(unsigned long level, const std::vector<unsigned char>& id, const std::vector<unsigned char>& value);
	encrypt_token(unsigned long level, const std::vector<unsigned char>& id, const std::vector<unsigned char>& otp, const std::vector<unsigned char>& value);
	encrypt_token(const encrypt_token& other);
    encrypt_token(encrypt_token&& other);
    ~encrypt_token();

    void clear();
    encrypt_token& operator = (const encrypt_token& other);
    encrypt_token& operator = (encrypt_token&& other);

    inline bool empty() const { return (_id.empty() || _value.empty()); }
    inline bool valid() const { return ((_id.size() == 16) && (_value.size() == 32)); }
    inline unsigned long size() const { return (unsigned long)_value.size(); }

    inline unsigned long get_token_level() const { return _level; }
    inline const std::vector<unsigned char>& get_token_id() const { return _id; }
    inline const std::vector<unsigned char>& get_token_value() const { return _value; }
	inline const std::vector<unsigned char>& get_token_otp() const { return _otp; }

private:
    unsigned long   _level;
    std::vector<unsigned char>  _id;
	std::vector<unsigned char>  _otp;
	std::vector<unsigned char>  _value;
};



#endif  // __NXRM_KEYMANAGER_HPP__