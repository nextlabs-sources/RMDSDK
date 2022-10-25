#pragma once

#include "bcrypt.h"

namespace NX {
    namespace crypt {

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

        class Provider
        {
        public:
            Provider(PROVIDER_ID id);
            virtual ~Provider();

            LPCWSTR GetAlgName() const;
            void Close();

            inline bool Opened() const { return (NULL != _h); }
            inline operator BCRYPT_ALG_HANDLE() const { return _h; }
            inline ULONG GetObjectLength() const { return _object_length; }
            inline ULONG GetHashDataLength() const { return _hash_length; }

        protected:
            void Open();

        private:
            // Copy is not allowed
            Provider(const Provider& rhs) {}

        private:
            PROVIDER_ID         _id;
            BCRYPT_ALG_HANDLE   _h;
            unsigned long       _object_length;
            unsigned long       _hash_length;
        };

        Provider* GetProvider(PROVIDER_ID id);

    }
}   // NX

