
#include <ntifs.h>
#include <Bcrypt.h>

#include <nkdf/basic/defines.h>
#include <nkdf/crypto/provider.h>
#include <nkdf/crypto/aes.h>
#include <nkdf/crypto/md5.h>




static NXALGOBJECT Providers[PROV_MAX] = {
    { NULL, BCRYPT_AES_ALGORITHM, 0, FALSE, 0, 0 },  // AES
    { NULL, BCRYPT_RSA_ALGORITHM, 0, FALSE, 0, 0 },  // RSA (BCRYPT_PROV_DISPATCH is not supported on Win7)
    { NULL, BCRYPT_RC4_ALGORITHM, 0, FALSE, 0, 0 },  // RC4
    { NULL, BCRYPT_MD5_ALGORITHM, 0, TRUE, 0, 0 },  // MD5
    { NULL, BCRYPT_SHA1_ALGORITHM, 0, TRUE, 0, 0 },  // SHA1
    { NULL, BCRYPT_SHA256_ALGORITHM, 0, TRUE, 0, 0 },  // SHA256
    { NULL, BCRYPT_MD5_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG, TRUE, 0, 0 },  // HMAC_MD5
    { NULL, BCRYPT_SHA1_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG, TRUE, 0, 0 },  // HMAC_SHA1
    { NULL, BCRYPT_SHA256_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG, TRUE, 0, 0 }   // HMAC_SHA256
};

//  Assign text sections for each routine.
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NkCryptoInit)
#pragma alloc_text(PAGE, NkCryptoCleanup)
#endif


_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NkCryptoInit(
             )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    PAGED_CODE();


    try {

        ULONG ReturnedLength = 0;
		int i = 0;

        for (i = 0; i < PROV_MAX; i++) {

            if (NULL == Providers[i].Handle) {

                Status = BCryptOpenAlgorithmProvider(&Providers[i].Handle, Providers[i].Name, MS_PRIMITIVE_PROVIDER, Providers[i].Flags);
                if (!NT_SUCCESS(Status)) {
                    try_return(Providers[i].Handle = NULL);
                }

                if (Providers[i].IsHashAlgorithm) {

                    Status = BCryptGetProperty(Providers[i].Handle, BCRYPT_OBJECT_LENGTH, (PUCHAR)&Providers[i].ObjectLength, sizeof(ULONG), &ReturnedLength, 0);
                    if (!NT_SUCCESS(Status)) {
                        try_return(Status);
                    }
                    ASSERT(0 != Providers[i].ObjectLength);

                    Status = BCryptGetProperty(Providers[i].Handle, BCRYPT_HASH_LENGTH, (PUCHAR)&Providers[i].HashLength, sizeof(ULONG), &ReturnedLength, 0);
                    if (!NT_SUCCESS(Status)) {
                        try_return(Status);
                    }
                    ASSERT(0 != Providers[i].HashLength);
                }
            }
        }

        //
        // extra initialize
        //

        // For AES Algorithm
        if (NULL != Providers[PROV_AES].Handle) {

            Status = BCryptSetProperty( Providers[PROV_AES].Handle,
                                        BCRYPT_CHAINING_MODE,
                                        (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                                        sizeof(BCRYPT_CHAIN_MODE_CBC),
                                        0);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }
        }

#ifdef DEBUG
        TestAes();
#endif

try_exit: NOTHING;
    }
    finally {
        if (!NT_SUCCESS(Status)) {
            NkCryptoCleanup();
        }
    }

    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NkCryptoCleanup(
                )
{
    int i = 0;
	PAGED_CODE();
    for (i = 0; i < PROV_MAX; i++) {
        if (NULL != Providers[i].Handle) {
            (VOID)BCryptCloseAlgorithmProvider(Providers[i].Handle, 0);
            Providers[i].Handle = NULL;
            Providers[i].ObjectLength = 0;
            Providers[i].HashLength = 0;
        }
    }
}

_Check_return_
BOOLEAN
NkCryptoInitialized(
                    )
{
	int i = 0;

    for (i = 0; i < PROV_MAX; i++) {
        if (NULL == Providers[i].Handle) {
            return FALSE;
        }
    }

    return TRUE;
}

_Check_return_
_IRQL_requires_max_(DISPATCH_LEVEL)
const NXALGOBJECT*
NkCryptoGetProvider(
                    _In_ NKCRYPTO_PROV_ID Id
                    )
{
    ASSERT(Id >= 0 && Id < PROV_MAX);
    return (&Providers[Id]);
}

