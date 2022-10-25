

#ifndef __NX_CRYPT_SHA_H__
#define __NX_CRYPT_SHA_H__

#include "SDLResult.h"

namespace NX {

    namespace crypt {

        SDWLResult CreateSha1(_In_reads_bytes_(data_size) const unsigned char* data,
            _In_ unsigned long data_size,
            _Out_writes_bytes_(20) unsigned char* out_buf);

        SDWLResult CreateHmacSha1(_In_reads_bytes_(data_size) const unsigned char* data,
            _In_ unsigned long data_size,
            _In_reads_bytes_(key_size) const unsigned char* key,
            _In_ unsigned long key_size,
            _Out_writes_bytes_(20) unsigned char* out_buf);

        SDWLResult CreateSha256(_In_reads_bytes_(data_size) const unsigned char* data,
            _In_ unsigned long data_size,
            _Out_writes_bytes_(32) unsigned char* out_buf);

        SDWLResult CreateHmacSha256(_In_reads_bytes_(data_size) const unsigned char* data,
            _In_ unsigned long data_size,
            _In_reads_bytes_(key_size) const unsigned char* key,
            _In_ unsigned long key_size,
            _Out_writes_bytes_(32) unsigned char* out_buf);


    }

}   // NX

#endif
