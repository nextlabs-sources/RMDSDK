#pragma  once

#include "SDLResult.h"
#include "basic.h"

namespace NX {

    namespace crypt {

        class AesKey : public Key
        {
        public:
            AesKey();
            virtual ~AesKey();

			static SDWLResult GenerateRandom(PUCHAR data, ULONG size);

            virtual SDWLResult Generate(ULONG bitslength);
            virtual SDWLResult Import(const UCHAR* key, ULONG size);
            virtual SDWLResult Export(PUCHAR key, _Inout_ PULONG size);
        };

        SDWLResult AesEncrypt(_In_ const AesKey& key,
            _In_reads_bytes_(in_size) const unsigned char* in_buf,
            _In_ unsigned long in_size,
            _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
            _Inout_ unsigned long* out_size,
            _In_reads_bytes_opt_(16) const unsigned char* ivec);

        SDWLResult AesDecrypt(_In_ const AesKey& key,
            _In_reads_bytes_(in_size) const unsigned char* in_buf,
            _In_ unsigned long in_size,
            _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
            _Inout_ unsigned long* out_size,
            _In_reads_bytes_opt_(16) const unsigned char* ivec);

        void AesGenerateIvec(_In_reads_bytes_opt_(16) const UCHAR* seed,
            unsigned __int64 offset,
            _Out_writes_bytes_(16) PUCHAR ivec);

        SDWLResult AesBlockEncrypt(_In_ const AesKey& key,
            _In_reads_bytes_(in_size) const unsigned char* in_buf,
            _In_ unsigned long in_size,
            _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
            _Inout_ unsigned long* out_size,
            _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
            _In_ const unsigned __int64 block_offset = 0,
            _In_ const unsigned long cipher_block_size = 512);

        SDWLResult AesBlockDecrypt(_In_ const AesKey& key,
            _In_reads_bytes_(in_size) const unsigned char* in_buf,
            _In_ unsigned long in_size,
            _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
            _Inout_ unsigned long* out_size,
            _In_reads_bytes_opt_(16) const unsigned char* ivec_seed,
            _In_ const unsigned __int64 block_offset = 0,
            _In_ const unsigned long cipher_block_size = 512);

    }

}   // NX

