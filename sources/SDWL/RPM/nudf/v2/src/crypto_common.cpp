

#include <Windows.h>
#include <assert.h>

#include <boost/noncopyable.hpp>

#include <nudf\crypto_common.hpp>


using namespace NX;
using namespace NX::crypto;


class provider_manager
{
public:
    provider_manager();
    ~provider_manager();

    void clear();

    inline provider_object* get_provider(PROVIDER_ID id) const { return (id < PROV_MAX) ? _providers[id].get() : NULL; }

private:
    std::vector<std::shared_ptr<provider_object>> _providers;
};

static provider_manager PROVIDERMGR;


basic_key::basic_key() : _h(NULL), _bits_length(0)
{
}

basic_key::~basic_key()
{
    clear();
}

void basic_key::clear()
{
    if (NULL != _h) {
        BCryptDestroyKey(_h);
        _h = NULL;
    }

    _bits_length = 0;
}

bool basic_key::query_bits_length()
{
    ULONG cbResult = 0;

    if (NULL == _h) {
        return false;
    }

    if (0 != BCryptGetProperty(_h, BCRYPT_KEY_LENGTH, (PUCHAR)&_bits_length, sizeof(ULONG), &cbResult, 0)) {
        _bits_length = 0;
        return false;
    }

    return true;
}


provider_object::provider_object(const std::wstring& name, unsigned long flags, bool is_hash_alg) : _name(name), _flags(flags), _is_hash_alg(is_hash_alg), _h(NULL), _object_length(0), _hash_length(0), _first_time_try(true)
{
}

provider_object::~provider_object()
{
    clear();
}

bool provider_object::init()
{
    LONG Status = 0;

    assert(!_name.empty());
    assert(NULL == _h);

    if (_first_time_try) {
        _first_time_try = false;
    }

    Status = BCryptOpenAlgorithmProvider(&_h, _name.c_str(), MS_PRIMITIVE_PROVIDER, _flags);
    if (Status != 0) {
        return false;
    }

    if (_is_hash_alg) {

        unsigned long returned_length = 0;

        Status = BCryptGetProperty(_h, BCRYPT_OBJECT_LENGTH, (PUCHAR)&_object_length, sizeof(ULONG), &returned_length, 0);
        if (Status != 0) {
            clear();
            return false;
        }

        Status = BCryptGetProperty(_h, BCRYPT_HASH_LENGTH, (PUCHAR)&_hash_length, sizeof(ULONG), &returned_length, 0);
        if (Status != 0) {
            clear();
            return false;
        }
    }

    return true;
}

void provider_object::clear()
{
    if (NULL != _h) {
        (VOID)BCryptCloseAlgorithmProvider(_h, 0);
        _h = NULL;
        _object_length = 0;
        _hash_length = 0;
    }
}

provider_manager::provider_manager()
{
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_AES_ALGORITHM, 0, false)));    // AES
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_RSA_ALGORITHM, 0, false)));    // RSA
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_DH_ALGORITHM, 0, false)));     // Diffie-Hellman
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_MD5_ALGORITHM, 0, true)));     // MD5
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_SHA1_ALGORITHM, 0, true)));    // SHA1
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_SHA256_ALGORITHM, 0, true)));  // SHA256
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_MD5_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG, true)));     // HMAC_MD5
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_SHA1_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG, true)));    // HMAC_SHA1
    _providers.push_back(std::shared_ptr<provider_object>(new provider_object(BCRYPT_SHA256_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG, true)));  // HMAC_SHA256
    assert(PROV_MAX == _providers.size());
}

provider_manager::~provider_manager()
{
    clear();
}

void provider_manager::clear()
{
    _providers.clear();
}

const provider_object* NX::crypto::GET_PROVIDER(PROVIDER_ID id)
{
    provider_object* p = PROVIDERMGR.get_provider(id);
    if (NULL == p) {
        return NULL;
    }

    if (p->is_first_time_try()) {
        p->init();
    }

    return p;
}