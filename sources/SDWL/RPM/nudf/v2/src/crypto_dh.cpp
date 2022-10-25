

#include <Windows.h>
#include <assert.h>


#include <nudf\crypto_common.hpp>
#include <nudf\crypto_cert.hpp>
#include <nudf\crypto_dh.hpp>
#include <nudf\conversion.hpp>


using namespace NX::crypto;

static const UCHAR DH2048_X509_PUBLIC_KEY_TEMPLATE[814] = {
    0x30, 0x82, 0x03, 0x2A, 0x30, 0x82, 0x02, 0x1C, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D,
    0x01, 0x03, 0x01, 0x30, 0x82, 0x02, 0x0D, 0x02, 0x82, 0x01, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x82, 0x01, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x02, 0x02, 0x04, 0x00, 0x03, 0x82, 0x01, 0x06, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const ULONG DH2048_X509_PUBLIC_KEY_P_OFFSET = 0x1C;
static const ULONG DH2048_X509_PUBLIC_KEY_G_OFFSET = 0x120;
static const ULONG DH2048_X509_PUBLIC_KEY_Y_OFFSET = 0x22E;


diffie_hellman_key_blob::diffie_hellman_key_blob()
{
}

diffie_hellman_key_blob::diffie_hellman_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y)
{
    set_key_blob(p, g, y);
}

diffie_hellman_key_blob::diffie_hellman_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y, const std::vector<unsigned char>& x)
{
    set_key_blob(p, g, y, x);
}

diffie_hellman_key_blob::diffie_hellman_key_blob(const diffie_hellman_key_blob& other) : _data(other._data)
{
}

diffie_hellman_key_blob::diffie_hellman_key_blob(diffie_hellman_key_blob&& other) : _data(std::move(other._data))
{
}

diffie_hellman_key_blob::~diffie_hellman_key_blob()
{
}

diffie_hellman_key_blob& diffie_hellman_key_blob::operator = (const diffie_hellman_key_blob& other)
{
    if (this != &other) {
        _data = other._data;
    }
    return *this;
}

diffie_hellman_key_blob& diffie_hellman_key_blob::operator = (diffie_hellman_key_blob&& other)
{
    if (this != &other) {
        _data = std::move(other._data);
    }
    return *this;
}

bool diffie_hellman_key_blob::is_private() const
{
    if (empty()) {
        return false;
    }
    assert(_data.size() > sizeof(BCRYPT_DH_KEY_BLOB));
    return (BCRYPT_DH_PRIVATE_MAGIC == ((const BCRYPT_DH_KEY_BLOB*)_data.data())->dwMagic);
}

unsigned long diffie_hellman_key_blob::get_key_length() const
{
    if (empty()) {
        return 0;
    }
    assert(_data.size() > sizeof(BCRYPT_DH_KEY_BLOB));
    return ((const BCRYPT_DH_KEY_BLOB*)_data.data())->cbKey;
}

unsigned long diffie_hellman_key_blob::get_key_bitlength() const
{
    return (8 * get_key_length());
}

std::vector<unsigned char> diffie_hellman_key_blob::get_p() const
{
    if (empty()) {
        return std::vector<unsigned char>();
    }
    const unsigned long key_length = ((const BCRYPT_DH_KEY_BLOB*)_data.data())->cbKey;
    assert(_data.size() >= (sizeof(BCRYPT_DH_KEY_BLOB) + key_length));
    const unsigned char* p = _data.data() + sizeof(BCRYPT_DH_KEY_BLOB);
    return std::vector<unsigned char>(p, p + key_length);
}

std::vector<unsigned char> diffie_hellman_key_blob::get_g() const
{
    if (empty()) {
        return std::vector<unsigned char>();
    }
    const unsigned long key_length = ((const BCRYPT_DH_KEY_BLOB*)_data.data())->cbKey;
    assert(_data.size() >= (sizeof(BCRYPT_DH_KEY_BLOB) + (2 * key_length)));
    const unsigned char* g = _data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + key_length;
    return std::vector<unsigned char>(g, g + key_length);
}

std::vector<unsigned char> diffie_hellman_key_blob::get_x() const
{
    if (!is_private()) {
        return std::vector<unsigned char>();
    }
    const unsigned long key_length = ((const BCRYPT_DH_KEY_BLOB*)_data.data())->cbKey;
    assert(_data.size() >= (sizeof(BCRYPT_DH_KEY_BLOB) + (4 * key_length)));
    const unsigned char* x = _data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + (3 * key_length);
    return std::vector<unsigned char>(x, x + key_length);
}

std::vector<unsigned char> diffie_hellman_key_blob::get_y() const
{
    if (empty()) {
        return std::vector<unsigned char>();
    }
    const unsigned long key_length = ((const BCRYPT_DH_KEY_BLOB*)_data.data())->cbKey;
    assert(_data.size() >= (sizeof(BCRYPT_DH_KEY_BLOB) + (3 * key_length)));
    const unsigned char* g = _data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + (2 * key_length);
    return std::vector<unsigned char>(g, g + key_length);
}

bool diffie_hellman_key_blob::encode_public_key(std::vector<unsigned char>& encoded_pubkey)
{
    if (empty()) {
        return false;
    }
    
    if (128 == get_key_length()) {
        return false;
    }
    else if (256 == get_key_length()) {
        encoded_pubkey.resize(sizeof(DH2048_X509_PUBLIC_KEY_TEMPLATE), 0);
        memcpy(encoded_pubkey.data(), DH2048_X509_PUBLIC_KEY_TEMPLATE, sizeof(DH2048_X509_PUBLIC_KEY_TEMPLATE));
        memcpy(encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_P_OFFSET, get_p_ptr(), get_key_length());
        memcpy(encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_G_OFFSET, get_g_ptr(), get_key_length());
        memcpy(encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_Y_OFFSET, get_y_ptr(), get_key_length());
        return true;
    }
    else {
        return false;
    }
}

bool diffie_hellman_key_blob::encode_public_key(std::wstring& encoded_pubkey)
{
    std::vector<unsigned char> encoded_pubkey_der;
    encoded_pubkey.clear();
    if (encode_public_key(encoded_pubkey_der)) {
        encoded_pubkey = NX::conversion::to_base64(encoded_pubkey_der);
    }
    return encoded_pubkey.empty() ? false : true;
}

bool diffie_hellman_key_blob::decode_public_key(const std::vector<unsigned char>& encoded_pubkey)
{
    if (encoded_pubkey.size() == sizeof(DH2048_X509_PUBLIC_KEY_TEMPLATE)) {
        const std::vector<unsigned char> p(encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_P_OFFSET, encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_P_OFFSET + 256);
        const std::vector<unsigned char> g(encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_G_OFFSET, encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_G_OFFSET + 256);
        const std::vector<unsigned char> y(encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_Y_OFFSET, encoded_pubkey.data() + DH2048_X509_PUBLIC_KEY_Y_OFFSET + 256);
        set_key_blob(p, g, y);
        return true;
    }
    else {
        return false;
    }
}

bool diffie_hellman_key_blob::decode_public_key(const std::wstring& encoded_pubkey)
{
    const std::vector<unsigned char>& encoded_pubkey_der = NX::conversion::from_base64(encoded_pubkey);
    return decode_public_key(encoded_pubkey_der);
}

bool diffie_hellman_key_blob::set_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y)
{
    if (p.size() != 128 && p.size() != 256) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (p.size() != g.size() || p.size() != y.size()) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    const unsigned long blob_size = sizeof(BCRYPT_DH_KEY_BLOB) + (unsigned long)(3 * p.size());
    _data.resize(blob_size, 0);
    ((BCRYPT_DH_KEY_BLOB*)_data.data())->dwMagic = BCRYPT_DH_PUBLIC_MAGIC;
    ((BCRYPT_DH_KEY_BLOB*)_data.data())->cbKey = (unsigned long)p.size();
    memcpy(_data.data() + sizeof(BCRYPT_DH_KEY_BLOB), p.data(), p.size());
    memcpy(_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + p.size(), g.data(), g.size());
    memcpy(_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + p.size() + g.size(), y.data(), y.size());
    return true;
}

bool diffie_hellman_key_blob::set_key_blob(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g, const std::vector<unsigned char>& y, const std::vector<unsigned char>& x)
{
    if (p.size() != 128 && p.size() != 256) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (p.size() != g.size() || p.size() != y.size() || p.size() != x.size()) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    const unsigned long blob_size = sizeof(BCRYPT_DH_KEY_BLOB) + (unsigned long)(4 * p.size());
    _data.resize(blob_size, 0);
    ((BCRYPT_DH_KEY_BLOB*)_data.data())->dwMagic = BCRYPT_DH_PRIVATE_MAGIC;
    ((BCRYPT_DH_KEY_BLOB*)_data.data())->cbKey = (unsigned long)p.size();
    memcpy(_data.data() + sizeof(BCRYPT_DH_KEY_BLOB), p.data(), p.size());
    memcpy(_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + p.size(), g.data(), g.size());
    memcpy(_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + p.size() + g.size(), y.data(), y.size());
    memcpy(_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + p.size() + g.size() + y.size(), x.data(), x.size());
    return true;
}

const unsigned char* diffie_hellman_key_blob::get_p_ptr() const
{
    if (empty()) {
        return nullptr;
    }
    return (_data.data() + sizeof(BCRYPT_DH_KEY_BLOB));
}

const unsigned char* diffie_hellman_key_blob::get_g_ptr() const
{
    if (empty()) {
        return nullptr;
    }
    return (_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + get_key_length());
}

const unsigned char* diffie_hellman_key_blob::get_y_ptr() const
{
    if (empty()) {
        return nullptr;
    }
    return (_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + (2 * get_key_length()));
}

const unsigned char* diffie_hellman_key_blob::get_x_ptr() const
{
    if (!is_private()) {
        return nullptr;
    }
    return (_data.data() + sizeof(BCRYPT_DH_KEY_BLOB) + (3 * get_key_length()));
}




diffie_hellman_key::diffie_hellman_key() : basic_key(), _full_key(false)
{
}

diffie_hellman_key::~diffie_hellman_key()
{
}

void diffie_hellman_key::clear()
{
    basic_key::clear();
    _full_key = false;
}

bool diffie_hellman_key::generate(_In_reads_bytes_(bits_length/8) const unsigned char* p, _In_reads_bytes_(bits_length/8)const unsigned char* g, _In_ unsigned long bits_length)
{
    const provider_object*  provider = GET_PROVIDER(PROV_DH);
    LONG status = 0;
    BCRYPT_ALG_HANDLE   hkey = NULL;
    bool result = false;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    if (bits_length != 1024 && bits_length != 2048) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    clear();

    do {

        status = BCryptGenerateKeyPair(provider->get_handle(), &hkey, bits_length, 0);
        if (0 != status) {
            break;
        }

        const unsigned long dh_size = sizeof(BCRYPT_DH_PARAMETER_HEADER) + bits_length / 8 + bits_length / 8;
        BCRYPT_DH_PARAMETER_HEADER* dh_parameter = NULL;
        std::vector<unsigned char> buf;
        buf.resize(dh_size, 0);
        dh_parameter = (BCRYPT_DH_PARAMETER_HEADER *)buf.data();
        dh_parameter->dwMagic = BCRYPT_DH_PARAMETERS_MAGIC;
        dh_parameter->cbLength = dh_size;
        dh_parameter->cbKeyLength = bits_length / 8;
        memcpy(buf.data() + sizeof(BCRYPT_DH_PARAMETER_HEADER), p, dh_parameter->cbKeyLength);
        memcpy(buf.data() + sizeof(BCRYPT_DH_PARAMETER_HEADER) + dh_parameter->cbKeyLength, g, dh_parameter->cbKeyLength);
        status = BCryptSetProperty(hkey, BCRYPT_DH_PARAMETERS, buf.data(), dh_size, 0);
        if (0 != status) {
            break;
        }

        status = BCryptFinalizeKeyPair(hkey, 0);
        if (0 != status) {
            break;
        }

        set_bits_length(bits_length);
        set_key_handle(hkey); hkey = NULL;
        _full_key = true;
        result = true;

    } while (false);

    if (NULL != hkey) {
        BCryptDestroyKey(hkey);
        hkey = NULL;
    }

    return result;
}

bool diffie_hellman_key::export_key(std::vector<unsigned char>& key)
{
    unsigned long size = 0;

    if (!export_key(NULL, &size)) {
        return false;
    }

    key.resize(size, 0);
    return export_key(key.data(), &size);
}

bool diffie_hellman_key::export_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size)
{
    return export_key_ex(key, size, _full_key);
}

bool diffie_hellman_key::export_public_key(std::vector<unsigned char>& key)
{
    unsigned long size = 0;

    if (!export_key_ex(NULL, &size, false)) {
        return false;
    }

    key.resize(size, 0);
    return export_key_ex(key.data(), &size, false);
}

diffie_hellman_key_blob diffie_hellman_key::export_key()
{
    diffie_hellman_key_blob keyblob;

    if (!export_key(keyblob._data)) {
        keyblob.clear();
    }

    return std::move(keyblob);
}

diffie_hellman_key_blob diffie_hellman_key::export_public_key()
{
    diffie_hellman_key_blob keyblob;

    if (!export_public_key(keyblob._data)) {
        keyblob.clear();
    }

    return std::move(keyblob);
}

bool diffie_hellman_key::export_key_ex(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size, _In_ bool full_key)
{
    const provider_object*  provider = GET_PROVIDER(PROV_DH);
    LONG status = 0;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    const bool query_size = (0 == *size);

    status = BCryptExportKey(get_key(), NULL, full_key ? BCRYPT_DH_PRIVATE_BLOB : BCRYPT_DH_PUBLIC_BLOB, key, *size, size, 0);
    if (query_size) {
        return (0 != *size) ? true : false;
    }

    return (0 == status);
}

bool diffie_hellman_key::import_key(const std::vector<unsigned char>& key)
{
    return import_key(key.data(), key.size());
}

bool diffie_hellman_key::import_key(_In_reads_bytes_(size) const unsigned char* key, size_t size)
{
    const provider_object*  provider = GET_PROVIDER(PROV_RSA);
    LONG status = 0;

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    const BCRYPT_DH_KEY_BLOB* header = (const BCRYPT_DH_KEY_BLOB*)key;
    BCRYPT_KEY_HANDLE hkey = NULL;

    if (size <= sizeof(BCRYPT_DH_KEY_BLOB)) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }

    if (BCRYPT_DH_PUBLIC_MAGIC == header->dwMagic) {

        status = BCryptImportKeyPair(provider->get_handle(),
            NULL,
            BCRYPT_DH_PUBLIC_BLOB,
            &hkey,
            (PUCHAR)key, (ULONG)size,
            0);
        if (0 != status) {
            return false;
        }

        _full_key = false;
        set_key_handle(hkey);
        query_bits_length();
        return true;
    }
    else if (BCRYPT_DH_PRIVATE_MAGIC == header->dwMagic) {

        status = BCryptImportKeyPair(provider->get_handle(),
            NULL,
            BCRYPT_DH_PRIVATE_BLOB,
            &hkey,
            (PUCHAR)key, (ULONG)size,
            0);
        if (0 != status) {
            return false;
        }

        _full_key = true;
        set_key_handle(hkey);
        query_bits_length();
        return true;
    }
    else {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }

    return false;
}

static bool get_y_from_cert_context(PCCERT_CONTEXT p, std::vector<unsigned char>& y)
{
    static const char* szOID_DH_KEY_AGREEMENT = "1.2.840.113549.1.3.1";
    const char* pubkey_algorithm_OID = p->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
    const unsigned long pubkey_size = p->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData;
    const unsigned char* pubkey_data = p->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData;

    if (0 != _stricmp(szOID_DH_KEY_AGREEMENT, pubkey_algorithm_OID)) {
        SetLastError(NTE_BAD_ALGID);
        return false;
    }
    if (pubkey_size < sizeof(DWORD)) {
        SetLastError(NTE_BAD_KEY);
        return false;
    }

    unsigned long y_size = pubkey_size - 4;
    if (y_size == 129) y_size = 128;    // Server may parse extra byte to indicate sign
    if (y_size == 257) y_size = 256;    // Server may parse extra byte to indicate sign
    if (128 != y_size && 256 != y_size) {
        SetLastError(NTE_BAD_KEY);
        return false;
    }

    y.resize(y_size, 0);
    memcpy(y.data(), pubkey_data + (pubkey_size - y_size), y_size);
    return true;
}

bool diffie_hellman_key::get_y_from_cert_file(const std::wstring& file, std::vector<unsigned char>& y)
{
    NX::cert::cert_context context;
    if (!context.create(file)) {
        return false;
    }
    return ::get_y_from_cert_context((PCCERT_CONTEXT)context, y);
}

bool diffie_hellman_key::get_y_from_cert(const std::wstring& base64_cert, std::vector<unsigned char>& y)
{
    const std::vector<unsigned char>& cert_bin = NX::conversion::from_base64(base64_cert);
    if (cert_bin.empty()) {
        SetLastError(NTE_BAD_DATA);
        return false;
    }
    return get_y_from_cert(cert_bin.data(), (unsigned long)cert_bin.size(), y);
}

bool diffie_hellman_key::get_y_from_cert(const unsigned char* pbcert, unsigned long cbcert, std::vector<unsigned char>& y)
{
    NX::cert::cert_context context;
    if (!context.create(pbcert, cbcert)) {
        return false;
    }
    return ::get_y_from_cert_context((PCCERT_CONTEXT)context, y);
}