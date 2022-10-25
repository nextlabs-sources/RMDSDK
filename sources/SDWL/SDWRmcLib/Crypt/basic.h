

#ifndef __NX_CRYPT_BASIC_H__
#define __NX_CRYPT_BASIC_H__

#include <Bcrypt.h>

#include "SDLResult.h"

namespace NX {

    namespace crypt {

        class Key
        {
        public:
            Key();
            virtual ~Key();

            virtual SDWLResult Generate(ULONG bitslength) = 0;
            virtual SDWLResult Import(const UCHAR* key, ULONG size) = 0;
            virtual SDWLResult Export(PUCHAR key, _Inout_ PULONG size) = 0;

            inline operator BCRYPT_KEY_HANDLE() const { return _h; }
            inline ULONG GetBitsLength() const { return _bitslen; }
            inline bool Empty() const { return (NULL == _h); }

            void Close();
            SDWLResult QueryBitsLength();
            
        protected:
            // Copy/Move is not allowed
            Key(const Key& rhs) {}
            Key(Key&& rhs) {}


        protected:
            BCRYPT_KEY_HANDLE   _h;
            ULONG               _bitslen;
        };
    }

}   // NX

#endif
