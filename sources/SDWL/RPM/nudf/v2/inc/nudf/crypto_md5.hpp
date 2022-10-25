

#ifndef __NX_CRYPTO_MD5_HPP__
#define __NX_CRYPTO_MD5_HPP__


#include <Bcrypt.h>
#include <Wincrypt.h>


#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include <nudf\crypto_common.hpp>

namespace NX {

namespace crypto {


bool md5(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _Out_writes_bytes_(16) unsigned char* out_buf);

bool hmac_md5(_In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _In_reads_bytes_(key_size) const unsigned char* key,
    _In_ unsigned long key_size,
    _Out_writes_bytes_(16) unsigned char* out_buf);

}   // NX::crypto
}   // NX


#endif