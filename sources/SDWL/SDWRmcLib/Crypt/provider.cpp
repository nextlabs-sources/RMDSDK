
#include "..\stdafx.h"

#include "provider.h"


using namespace NX::crypt;


class ProvManager
{
public:
    ProvManager()
    {
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_AES)));       // AES
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_RSA)));       // RSA
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_DH)));         // Diffie-Hellman
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_MD5)));       // MD5
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_SHA1)));     // SHA1
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_SHA256))); // SHA256
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_HMAC_MD5)));         // HMAC_MD5
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_HMAC_SHA1)));       // HMAC_SHA1
        _providers.push_back(std::shared_ptr<Provider>(new Provider(PROV_HMAC_SHA256)));   // HMAC_SHA256
        assert(PROV_MAX == _providers.size());
    }

    ~ProvManager() {}

    inline void clear() { _providers.clear(); }
    inline Provider* GetProvider(PROVIDER_ID id) const { return (id < PROV_MAX) ? _providers[id].get() : NULL; }

private:
    std::vector<std::shared_ptr<Provider>> _providers;
};

static ProvManager PROVIDERMGR;


#define IsHashAlg(id)   ((id) >= PROV_MD5 && (id) < PROV_MAX)
#define IsHmacHashAlg(id)   ((id) == PROV_HMAC_MD5 || (id) == PROV_HMAC_SHA1 || (id) == PROV_HMAC_SHA256)

Provider* NX::crypt::GetProvider(PROVIDER_ID id)
{
    return PROVIDERMGR.GetProvider(id);
}


Provider::Provider(PROVIDER_ID id) : _id(id), _h(NULL), _object_length(0), _hash_length(0)
{
    Open();
}

Provider::~Provider()
{
    Close();
}

void Provider::Close()
{
    if (_h) {
        (VOID)BCryptCloseAlgorithmProvider(_h, 0);
        _h = NULL;
        _object_length = 0;
        _hash_length = 0;
    }
}

void Provider::Open()
{
    LONG Status = 0;

    assert(NULL == _h);
    assert(_id < PROV_MAX);

    do {

        Status = BCryptOpenAlgorithmProvider(&_h, GetAlgName(), MS_PRIMITIVE_PROVIDER, IsHmacHashAlg(_id) ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0);
        if (Status != 0) {
            break;
        }

		if (IsHashAlg(_id)) {

			unsigned long returned_length = 0;

			Status = BCryptGetProperty(_h, BCRYPT_OBJECT_LENGTH, (PUCHAR)&_object_length, sizeof(ULONG), &returned_length, 0);
			if (Status != 0) {
				_object_length = 0;
				break;
			}

            Status = BCryptGetProperty(_h, BCRYPT_HASH_LENGTH, (PUCHAR)&_hash_length, sizeof(ULONG), &returned_length, 0);
            if (Status != 0) {
                break;
            }
        }

    } while (FALSE);

    if (0 != Status) {
        // Error happened
        Close();
    }
}

LPCWSTR Provider::GetAlgName() const
{
    LPCWSTR name = L"Unknown";
    switch (_id)
    {
    case PROV_AES:
        name = BCRYPT_AES_ALGORITHM;
        break;
    case PROV_RSA:
        name = BCRYPT_RSA_ALGORITHM;
        break;
    case PROV_DH:
        name = BCRYPT_DH_ALGORITHM;
        break;
    case PROV_MD5:
        name = BCRYPT_MD5_ALGORITHM;
        break;
    case PROV_SHA1:
        name = BCRYPT_SHA1_ALGORITHM;
        break;
    case PROV_SHA256:
        name = BCRYPT_SHA256_ALGORITHM;
        break;
    case PROV_HMAC_MD5:
        name = BCRYPT_MD5_ALGORITHM;
        break;
    case PROV_HMAC_SHA1:
        name = BCRYPT_SHA1_ALGORITHM;
        break;
    case PROV_HMAC_SHA256:
        name = BCRYPT_SHA256_ALGORITHM;
        break;
    default:
        break;
    }
    return name;
}