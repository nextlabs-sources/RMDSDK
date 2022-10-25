

#include "..\stdafx.h"

#include "basic.h"

using namespace NX;
using namespace NX::crypt;

Key::Key() : _h(NULL), _bitslen(0)
{
}

Key::~Key()
{
    Close();
}

void Key::Close()
{
    if (NULL != _h) {
        BCryptDestroyKey(_h);
        _h = NULL;
    }
}

SDWLResult Key::QueryBitsLength()
{
    ULONG cbResult = 0;

    if (NULL == _h) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    LONG status = BCryptGetProperty(_h, BCRYPT_KEY_LENGTH, (PUCHAR)&_bitslen, sizeof(ULONG), &cbResult, 0);
    if (0 != status) {
        _bitslen = 0;
        return RESULT2(status, "BCryptGetProperty failed");
    }

    return RESULT(0);
}