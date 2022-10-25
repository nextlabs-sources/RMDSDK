
#include <Windows.h>

#include <nudf\crypto_sha.hpp>
#include <nudf\crypto_rsa.hpp>


using namespace NX;
using namespace NX::crypto;

rsa_key::rsa_key() : basic_key(), _key_type(RSAEMPTY), _key_spec(0)
{
}

rsa_key::~rsa_key()
{
}

void rsa_key::clear()
{
    basic_key::clear();
    _key_type = RSAEMPTY;
    _key_spec = 0;
}

bool rsa_key::generate(unsigned long bits_length)
{
    const provider_object*  provider = GET_PROVIDER(PROV_RSA);
    LONG status = 0;
    BCRYPT_ALG_HANDLE   hkey = NULL;

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

    status = BCryptGenerateKeyPair(provider->get_handle(), &hkey, bits_length, 0);
    if (0 != status) {
        return false;
    }

    set_bits_length(bits_length);
    set_key_handle(hkey);
    _key_type = RSAFULLPRIVATE;
    return true;
}

bool rsa_key::export_key(std::vector<unsigned char>& key)
{
    unsigned long size = 0;
    if (!export_key(NULL, &size)) {
        return false;
    }
    key.resize(size, 0);
    if (!export_key(key.data(), &size)) {
        key.clear();
        return false;
    }

    return true;
}

bool rsa_key::export_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size)
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

    const bool query_size = (0 == *size);

    if (!has_private_key()) {
        SetLastError(ERROR_NOT_FOUND);
        return false;
    }

    status = BCryptExportKey(get_key(), NULL, BCRYPT_RSAFULLPRIVATE_BLOB, key, *size, size, 0);
    if (query_size) {
        return (0 != *size) ? true : false;
    }

    return (0 == status);
}

bool rsa_key::export_public_key(std::vector<unsigned char>& key)
{
    unsigned long size = 0;
    if (!export_public_key(NULL, &size)) {
        return false;
    }
    key.resize(size, 0);
    if (!export_public_key(key.data(), &size)) {
        key.clear();
        return false;
    }

    return true;
}

bool rsa_key::export_public_key(_Out_writes_bytes_opt_(*size) unsigned char* key, _Out_ unsigned long* size)
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

    const bool query_size = (0 == *size);

    status = BCryptExportKey(get_key(), NULL, BCRYPT_RSAPUBLIC_BLOB, key, *size, size, 0);
    if (query_size) {
        return (0 != *size) ? true : false;
    }

    return (0 == status);
}

bool rsa_key::import_key(const std::vector<unsigned char>& key)
{
    const provider_object*  provider = GET_PROVIDER(PROV_RSA);

    if (NULL == provider) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (provider->empty()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }

    return import_key(key.data(), key.size());
}

bool rsa_key::import_key(_In_reads_bytes_(size) const unsigned char* key, size_t size)
{
	bool b_imported = false;
    if (is_legacy_key(key, size)) {
		b_imported = import_legacy_key(key, size);
    }
    else {
		b_imported = import_bcrypt_key(key, size);
    }

	return b_imported;
}

bool rsa_key::import_bcrypt_key(_In_reads_bytes_(size) const unsigned char* key, size_t size)
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

    //const BCRYPT_RSAPRIVATE_BLOB* 
    const BCRYPT_RSAKEY_BLOB* key_blob = (const BCRYPT_RSAKEY_BLOB*)key;
    BCRYPT_KEY_HANDLE   hkey = NULL;

    if (BCRYPT_RSAPUBLIC_MAGIC == key_blob->Magic) {

        status = BCryptImportKeyPair(provider->get_handle(),
                                     NULL,
                                     BCRYPT_RSAPUBLIC_BLOB,
                                     &hkey,
                                     (PUCHAR)key, (ULONG)size,
                                     0);
        if (0 != status) {
            return false;
        }
        query_bits_length();
        set_key_handle(hkey);
        _key_type = RSAPUBLIC;
        return true;
    }
    else if (BCRYPT_RSAPRIVATE_MAGIC == key_blob->Magic) {

        status = BCryptImportKeyPair(provider->get_handle(),
                                     NULL,
                                     BCRYPT_RSAPRIVATE_BLOB,
                                     &hkey,
                                     (PUCHAR)key, (ULONG)size,
                                     0);
        if (0 != status) {
            return false;
        }
        query_bits_length();
        set_key_handle(hkey);
        _key_type = RSAPRIVATE;
        return true;
    }
    else if (BCRYPT_RSAFULLPRIVATE_MAGIC == key_blob->Magic) {

        status = BCryptImportKeyPair(provider->get_handle(),
                                     NULL,
                                     BCRYPT_RSAPRIVATE_BLOB,
                                     &hkey,
                                     (PUCHAR)key, (ULONG)size,
                                     0);
        if (0 != status) {
            return false;
        }
        query_bits_length();
        set_key_handle(hkey);
        _key_type = RSAFULLPRIVATE;
        return true;
    }
    else {
        SetLastError(ERROR_NOT_SUPPORTED);
        return false;
    }    
}

bool rsa_key::import_legacy_key(_In_reads_bytes_(size) const unsigned char* key, size_t size)
{
    const PUBLICKEYSTRUC* pubkey_struc = (const PUBLICKEYSTRUC*)key;
    const RSAPUBKEY* rsa_pubkey = (const RSAPUBKEY*)(key + sizeof(PUBLICKEYSTRUC));

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

    //const BCRYPT_RSAPRIVATE_BLOB* 
    const BCRYPT_RSAKEY_BLOB* key_blob = (const BCRYPT_RSAKEY_BLOB*)key;
    BCRYPT_KEY_HANDLE   hkey = NULL;

    if (pubkey_struc->bType == PRIVATEKEYBLOB && rsa_pubkey->magic == 0x32415352 /*RSA2*/) {
        // Private key is valid
        status = BCryptImportKeyPair(provider->get_handle(),
                                     NULL,
                                     LEGACY_RSAPRIVATE_BLOB,
                                     &hkey,
                                     (PUCHAR)key, (ULONG)size,
                                     0);
        if (0 != status) {
            return false;
        }
        query_bits_length();
        set_key_handle(hkey);
        _key_type = RSAPRIVATE;
        return true;
    }
    else {
        // Private key is not valid
        status = BCryptImportKeyPair(provider->get_handle(),
                                     NULL,
                                     LEGACY_RSAPUBLIC_BLOB,
                                     &hkey,
                                     (PUCHAR)key, (ULONG)size,
                                     0);
        if (0 != status) {
            return false;
        }
        query_bits_length();
        set_key_handle(hkey);
        _key_type = RSAPUBLIC;
        return true;
    }
}

bool rsa_key::is_legacy_key(_In_reads_bytes_(size) const unsigned char* key, size_t size)
{
    if (size < (sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY))) {
        return false;
    }

    const PUBLICKEYSTRUC* pubkey_struc = (const PUBLICKEYSTRUC*)key;
    const RSAPUBKEY* rsa_pubkey = (const RSAPUBKEY*)(key + sizeof(PUBLICKEYSTRUC));

    if (pubkey_struc->bType != PUBLICKEYBLOB && pubkey_struc->bType != PRIVATEKEYBLOB) {
        return false;
    }
    if (pubkey_struc->aiKeyAlg != CALG_RSA_KEYX && pubkey_struc->aiKeyAlg != CALG_RSA_SIGN) {
        return false;
    }
    if (rsa_pubkey->magic != 0x31415352 /*RSA1*/ && rsa_pubkey->magic != 0x32415352 /*RSA2*/) {
        return false;
    }

    return true;
}


bool NX::crypto::rsa_encrypt(const rsa_key& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size)
{
    LONG status = 0;
    ULONG required_size = 0;

    status = BCryptEncrypt(key.get_key(), (PUCHAR)in_buf, in_size, NULL, NULL, 0, NULL, 0, &required_size, BCRYPT_PAD_PKCS1);
    if (0 == required_size) {
        return false;
    }

    if (NULL == out_buf || 0 == *out_size) {
        *out_size = required_size;
        return true;
    }

    return (0 == BCryptEncrypt(key.get_key(), (PUCHAR)in_buf, in_size, NULL, NULL, 0, out_buf, *out_size, out_size, BCRYPT_PAD_PKCS1));
}

bool NX::crypto::rsa_decrypt(const rsa_key& key,
    _In_reads_bytes_(in_size) const unsigned char* in_buf,
    _In_ unsigned long in_size,
    _Out_writes_bytes_opt_(*out_size) unsigned char* out_buf,
    _Inout_ unsigned long* out_size)
{
    LONG status = 0;
    ULONG required_size = 0;

    status = BCryptDecrypt(key.get_key(), (PUCHAR)in_buf, in_size, NULL, NULL, 0, NULL, 0, &required_size, BCRYPT_PAD_PKCS1);
    if (0 == required_size) {
        return false;
    }

    if (NULL == out_buf || 0 == *out_size) {
        *out_size = required_size;
        return true;
    }

    return (0 == BCryptDecrypt(key.get_key(), (PUCHAR)in_buf, in_size, NULL, NULL, 0, out_buf, *out_size, out_size, BCRYPT_PAD_PKCS1));
}

bool NX::crypto::rsa_sign(const rsa_key& key,
    _In_reads_bytes_(data_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _Out_writes_bytes_opt_(*signature_size) unsigned char* signature,
    _Inout_ unsigned long* signature_size)
{
    LONG status = 0;
    UCHAR hash_data[32] = { 0 };
    const ULONG hash_data_size = 32;
    BCRYPT_PKCS1_PADDING_INFO pkcs1_padding_info = { BCRYPT_SHA256_ALGORITHM };
    ULONG required_size = 0;

    memset(hash_data, 0, 32);
    if (!NX::crypto::sha256(data, data_size, hash_data)) {
        return false;
    }
    
    status = BCryptSignHash(key.get_key(), &pkcs1_padding_info, hash_data, 32, NULL, 0, &required_size, BCRYPT_PAD_PKCS1);
    if (0 == required_size) {
        return false;
    }

    if (NULL == signature || 0 == *signature_size) {
        *signature_size = required_size;
        return true;
    }

    return (0 == BCryptSignHash(key.get_key(), &pkcs1_padding_info, hash_data, 32, signature, *signature_size, signature_size, BCRYPT_PAD_PKCS1));
}

bool NX::crypto::rsa_verify(const rsa_key& key,
    _In_reads_bytes_(in_size) const unsigned char* data,
    _In_ unsigned long data_size,
    _In_reads_bytes_(signature_size) const unsigned char* signature,
    _In_ unsigned long signature_size
    )
{
    LONG status = 0;
    UCHAR hash_data[32] = { 0 };
    const ULONG hash_data_size = 32;
    BCRYPT_PKCS1_PADDING_INFO pkcs1_padding_info = { BCRYPT_SHA256_ALGORITHM };

    if (!NX::crypto::sha256(data, data_size, hash_data)) {
        return false;
    }

    return (0 == BCryptVerifySignature(key.get_key(), &pkcs1_padding_info, hash_data, 32, (PUCHAR)signature, signature_size, BCRYPT_PAD_PKCS1));
}

bool NX::crypto::rsasha256_sign(const std::vector<unsigned char>& pkcs8_key,
	_In_reads_bytes_(data_size) const unsigned char* data,
	_In_ unsigned long data_size,
	_Out_writes_bytes_opt_(*signature_size) unsigned char* signature,
	_Inout_ unsigned long* signature_size)
{
	DWORD dwBufferLen = 0, cbKeyBlob = 0, cbSignature = 0;
	LPBYTE pbBuffer = NULL, pbKeyBlob = NULL, pbSignature = NULL;
	HCRYPTPROV hProv = NULL;
	HCRYPTKEY hKey = NULL;
	HCRYPTHASH hHash = NULL;
	bool b_signed = false;

	const char* szPemPrivKey = (const char*)pkcs8_key.data();
	do
	{
		if (!CryptStringToBinaryA(szPemPrivKey, 0, CRYPT_STRING_BASE64HEADER, NULL, &dwBufferLen, NULL, NULL))
		{
			printf("Failed to convert BASE64 private key. Error 0x%.8X\n", GetLastError());
			break;
		}

		pbBuffer = (LPBYTE)LocalAlloc(0, dwBufferLen);
		if (!CryptStringToBinaryA(szPemPrivKey, 0, CRYPT_STRING_BASE64HEADER, pbBuffer, &dwBufferLen, NULL, NULL))
		{
			printf("Failed to convert BASE64 private key. Error 0x%.8X\n", GetLastError());
			break;
		}

		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL, NULL, &cbKeyBlob))
		{
			printf("Failed to parse private key. Error 0x%.8X\n", GetLastError());
			break;
		}

		pbKeyBlob = (LPBYTE)LocalAlloc(0, cbKeyBlob);
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL, pbKeyBlob, &cbKeyBlob))
		{
			printf("Failed to parse private key. Error 0x%.8X\n", GetLastError());
			break;
		}

		// Create a temporary and volatile CSP context in order to import
		// the key and use for signing
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			printf("CryptAcquireContext failed with error 0x%.8X\n", GetLastError());
			break;
		}

		if (!CryptImportKey(hProv, pbKeyBlob, cbKeyBlob, NULL, 0, &hKey))
		{
			printf("CryptImportKey for private key failed with error 0x%.8X\n", GetLastError());
			break;
		}

		// Hash the data
		if (!CryptCreateHash(hProv, CALG_SHA_256, NULL, 0, &hHash))
		{
			printf("CryptCreateHash failed with error 0x%.8X\n", GetLastError());
			break;
		}

		if (!CryptHashData(hHash, (const BYTE*)data, data_size, 0))
		{
			printf("CryptHashData failed with error 0x%.8X\n", GetLastError());
			break;
		}

		// Sign the hash using our imported key
		if (!CryptSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, NULL, &cbSignature))
		{
			printf("CryptSignHash failed with error 0x%.8X\n", GetLastError());
			break;
		}

		pbSignature = (LPBYTE)LocalAlloc(0, cbSignature);
		if (!CryptSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, pbSignature, &cbSignature))
		{
			printf("CryptSignHash failed with error 0x%.8X\n", GetLastError());
			break;
		}

		if (signature)
		{
			// reverse the signed result to be compatible with JAVA
			for (DWORD i = 0; i < cbSignature; i++)
			{
				memcpy(signature + cbSignature - 1 - i, pbSignature + i, 1);
			}
		}

		(*signature_size) = cbSignature;
		b_signed = true;
	} while (false);

	// cleanup
	if (pbBuffer)
		LocalFree(pbBuffer);
	if (pbKeyBlob)
		LocalFree(pbKeyBlob);
	if (pbSignature)
		LocalFree(pbSignature);

	return b_signed;
}
