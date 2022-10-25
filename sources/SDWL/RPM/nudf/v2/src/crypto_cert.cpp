
#include <Windows.h>
#include <assert.h>

#include <nudf\string.hpp>
#include <nudf\crypto_cert.hpp>


using namespace NX;
using namespace NX::cert;


cert::store::store() : _h(NULL)
{
}

cert::store::store(HCERTSTORE h) : _h(h)
{
}

cert::store::~store()
{
    close();
}

void cert::store::close()
{
    if (NULL != _h) {
        (VOID)CertCloseStore(_h, 0);
        _h = NULL;
    }
}


cert::system_store::system_store(const std::wstring& name, bool machine_store)
    : store(CertOpenStore(CERT_STORE_PROV_SYSTEM,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        NULL,
        (machine_store ? CERT_SYSTEM_STORE_LOCAL_MACHINE : CERT_SYSTEM_STORE_CURRENT_USER) | CERT_STORE_NO_CRYPT_RELEASE_FLAG | CERT_STORE_OPEN_EXISTING_FLAG,
        name.c_str()))
{
}

cert::system_store::~system_store()
{
}

void cert::system_store::open(const std::wstring& name, bool machine_store)
{
    close();
    attach(CertOpenStore(CERT_STORE_PROV_SYSTEM,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        NULL,
        (machine_store ? CERT_SYSTEM_STORE_LOCAL_MACHINE : CERT_SYSTEM_STORE_CURRENT_USER) | CERT_STORE_NO_CRYPT_RELEASE_FLAG | CERT_STORE_OPEN_EXISTING_FLAG,
        name.c_str()));
}

cert::system_personal_store::system_personal_store(bool machine_store) : system_store(L"My", machine_store) {}
cert::system_trust_root_store::system_trust_root_store(bool machine_store) : system_store(L"Root", machine_store) {}
cert::system_intermediate_store::system_intermediate_store(bool machine_store) : system_store(L"CertificateAuthority", machine_store) {}
cert::system_trustpublisher_store::system_trustpublisher_store(bool machine_store) : system_store(L"TrustedPublisher", machine_store) {}
cert::system_trustpeople_store::system_trustpeople_store(bool machine_store) : system_store(L"TrustedPeople", machine_store) {}
cert::system_otherpeople_store::system_otherpeople_store(bool machine_store) : system_store(L"AddressBook", machine_store) {}
cert::system_thirdparty_root_store::system_thirdparty_root_store(bool machine_store) : system_store(L"AuthRoot", machine_store) {}
cert::system_revoked_store::system_revoked_store(bool machine_store) : system_store(L"Disallowed", machine_store) {}


cert::memory_store::memory_store() : store(CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL)) {}


cert::pcks12_file_store::pcks12_file_store() : store()
{
}

cert::pcks12_file_store::pcks12_file_store(const std::wstring& file, const wchar_t* password) : store()
{
    (void)open(file, password);
}

bool cert::pcks12_file_store::open(const std::wstring& file, const wchar_t* password)
{
    std::vector<unsigned char> cert_blob = get_cert_blob(file);
    if (cert_blob.empty()) {
        return false;
    }

    CRYPT_DATA_BLOB crypt_data_blob = { (ULONG)cert_blob.size(), cert_blob.data() };
    return (NULL != attach(PFXImportCertStore(&crypt_data_blob, password, CRYPT_EXPORTABLE)));
}

std::vector<unsigned char> cert::pcks12_file_store::get_cert_blob(const std::wstring& file)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    std::vector<unsigned char> blob;

    h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != h) {

        const ULONG file_size = GetFileSize(h, NULL);
        if (file_size != INVALID_FILE_SIZE) {

            if (file_size != 0) {

                ULONG bytes_read = 0;
                blob.resize(file_size, 0);
                if (!::ReadFile(h, blob.data(), (ULONG)blob.size(), &bytes_read, NULL)) {
                    blob.clear();
                }
            }
            else {
                SetLastError(ERROR_INVALID_DATA);
            }
        }

        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return std::move(blob);
}



cert_info::cert_info() : _key_spec(0), _key_bits_length(0), _self_signed(false)
{
    memset(&_date_stamp, 0, sizeof(_date_stamp));
    memset(&_valid_from, 0, sizeof(_valid_from));
    memset(&_valid_through, 0, sizeof(_valid_through));
}

cert_info::cert_info(const cert_info& other)
{
    other._Copy_to(*this);
}

cert_info::cert_info(cert_info&& other)
{
    other._Move_to(*this);
}

cert_info::~cert_info()
{
}

cert_info& cert_info::operator = (const cert_info& other)
{
    if (this != &other) {
        other._Copy_to(*this);
    }
    return *this;
}

cert_info& cert_info::operator = (cert_info&& other)
{
    if (this != &other) {
        other._Move_to(*this);
    }
    return *this;
}

bool cert_info::is_legacy_rsa_public_key_blob() const
{
    if (_key_blob.size() < (sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY))) {
        return false;
    }

    const PUBLICKEYSTRUC* pubkey_struc = (const PUBLICKEYSTRUC*)_key_blob.data();
    const RSAPUBKEY* rsa_pubkey = (const RSAPUBKEY*)(_key_blob.data() + sizeof(PUBLICKEYSTRUC));
    if (pubkey_struc->bType != PUBLICKEYBLOB) {
        return false;
    }
    if (pubkey_struc->aiKeyAlg != CALG_RSA_KEYX && pubkey_struc->aiKeyAlg != CALG_RSA_SIGN) {
        return false;
    }
    if (rsa_pubkey->magic != 0x31415352 /*RSA1*/) {
        return false;
    }

    return true;
}

bool cert_info::is_legacy_rsa_private_key_blob() const
{
    if (_key_blob.size() < (sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY))) {
        return false;
    }

    const PUBLICKEYSTRUC* pubkey_struc = (const PUBLICKEYSTRUC*)_key_blob.data();
    const RSAPUBKEY* rsa_pubkey = (const RSAPUBKEY*)(_key_blob.data() + sizeof(PUBLICKEYSTRUC));
    if (pubkey_struc->bType != PRIVATEKEYBLOB) {
        return false;
    }
    if (pubkey_struc->aiKeyAlg != CALG_RSA_KEYX && pubkey_struc->aiKeyAlg != CALG_RSA_SIGN) {
        return false;
    }
    if (rsa_pubkey->magic != 0x32415352 /*RSA2*/) {
        return false;
    }

    return true;
}

void cert_info::_Move_to(cert_info& target)
{
    target._self_signed = _self_signed;
    target._key_spec = _key_spec;
    target._key_bits_length = _key_bits_length;
    target._subject_name = std::move(_subject_name);
    target._issuer_name = std::move(_issuer_name);
    target._friendly_name = std::move(_friendly_name);
    target._serial_number = std::move(_serial_number);
    target._sign_algorithm = std::move(_sign_algorithm);
    target._hash_algorithm = std::move(_hash_algorithm);
    target._thumbprint = std::move(_thumbprint);
    target._key_blob = std::move(_key_blob);
    memcpy(&target._date_stamp, &_date_stamp, sizeof(_date_stamp));
    memcpy(&target._valid_from, &_valid_from, sizeof(_valid_from));
    memcpy(&target._valid_through, &_valid_through, sizeof(_valid_through));
    _key_spec = 0;
    _key_bits_length = 0;
    memset(&_date_stamp, 0, sizeof(_date_stamp));
    memset(&_valid_from, 0, sizeof(_valid_from));
    memset(&_valid_through, 0, sizeof(_valid_through));
}

void cert_info::_Copy_to(cert_info& target) const
{
    target._self_signed = _self_signed;
    target._key_spec = _key_spec;
    target._key_bits_length = _key_bits_length;
    target._subject_name = _subject_name;
    target._issuer_name = _issuer_name;
    target._friendly_name = _friendly_name;
    target._serial_number = _serial_number;
    target._sign_algorithm = _sign_algorithm;
    target._hash_algorithm = _hash_algorithm;
    target._thumbprint = _thumbprint;
    target._key_blob = _key_blob;
    memcpy(&target._date_stamp, &_date_stamp, sizeof(_date_stamp));
    memcpy(&target._valid_from, &_valid_from, sizeof(_valid_from));
    memcpy(&target._valid_through, &_valid_through, sizeof(_valid_through));
}

cert_context::cert_context() :_p(nullptr)
{
}

cert_context::cert_context(PCCERT_CONTEXT p) :_p(p)
{
}

cert_context::~cert_context()
{
    clear();
}

void cert_context::clear()
{
    if (NULL != _p) {
        CertFreeCertificateContext(_p);
        _p = NULL;
    }
}

bool cert_context::export_to_file(_In_ const std::wstring& file, _In_ bool base64_format)
{
    if (empty()) {
        return false;
    }

    bool   result = false;
    HANDLE hf = INVALID_HANDLE_VALUE;
    std::string s;

    do {

        const unsigned char* pbCertData = _p->pbCertEncoded;
        unsigned long cbCertData = _p->cbCertEncoded;

        if (base64_format) {

            unsigned long size = 0;
            CryptBinaryToStringA(_p->pbCertEncoded,
                _p->cbCertEncoded,
                CRYPT_STRING_BASE64HEADER,
                NULL,
                &size);

            s.resize(size, 0);
            if (!CryptBinaryToStringA(_p->pbCertEncoded,
                _p->cbCertEncoded,
                CRYPT_STRING_BASE64HEADER,
                (char*)s.data(),
                &size)) {
                s.clear();
                break;
            }

            pbCertData = (const unsigned char*)s.c_str();
            cbCertData = (unsigned long)s.length();
        }

        hf = ::CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hf) {
            break;
        }

        unsigned long bytes_written = 0;

        if (!::WriteFile(hf, pbCertData, cbCertData, &bytes_written, NULL)) {
            break;
        }

        result = true;

    } while (FALSE);

    if (INVALID_HANDLE_VALUE != hf) {
        CloseHandle(hf);
        hf = INVALID_HANDLE_VALUE;
        if (!result) {
            ::DeleteFileW(file.c_str());
        }
    }

    return result;
}

bool cert_context::create(const std::wstring& x500_name,
    unsigned long key_spec,
    bool strong,
    _In_opt_ cert_context* issuer,
    const SYSTEMTIME& expire_date)
{
    HCRYPTPROV              hp = NULL;
    HCRYPTKEY               hk = NULL;
    PCRYPT_KEY_PROV_INFO    ckpi = NULL;
    std::vector<unsigned char> ckpi_buf;
    CERT_NAME_BLOB          nameblob;
    std::vector<unsigned char> namebuf;

    do {

        // Get key provider information
        if (NULL != issuer && !issuer->empty()) {
            ULONG ckpi_size = 0;
            if (CertGetCertificateContextProperty(*issuer, CERT_KEY_PROV_INFO_PROP_ID, NULL, &ckpi_size) && 0 != ckpi_size) {
                ckpi_buf.resize(ckpi_size, 0);
                if (!CertGetCertificateContextProperty(*issuer, CERT_KEY_PROV_INFO_PROP_ID, ckpi_buf.data(), &ckpi_size)) {
                    ckpi_buf.clear();
                }
                else {
                    ckpi = (PCRYPT_KEY_PROV_INFO)ckpi_buf.data();
                }
            }
        }

        // Acquire provider
        if (!CryptAcquireContextW(&hp, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET | CRYPT_SILENT)) {

            if (GetLastError() != NTE_BAD_KEYSET) {
                // Error other than bad keyset
                break;
            }

            // Keyset doesn't exist
            if (!CryptAcquireContextW(&hp, NULL,MS_ENHANCED_PROV_W, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET | CRYPT_SILENT)) {
                break;
            }
        }

        // Set subject name blob
        if (!CertStrToNameW(X509_ASN_ENCODING, x500_name.c_str(), CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG, NULL, NULL, &nameblob.cbData, NULL)) {
            break;;
        }

        namebuf.resize(nameblob.cbData, 0);
        nameblob.pbData = namebuf.data();
        if (!CertStrToName(X509_ASN_ENCODING, x500_name.c_str(), CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG, NULL, nameblob.pbData, &nameblob.cbData, NULL)) {
            break;
        }

        // Generate key
        const unsigned long key_flags = (strong ? 0x08000000/*2048 Bits Key*/ : RSA1024BIT_KEY) | CRYPT_EXPORTABLE;
        if (!CryptGenKey(hp, key_spec, key_flags, &hk)) {
            break;
        }

        // Prepare algorithm structure
        CRYPT_ALGORITHM_IDENTIFIER sign_alg;
        memset(&sign_alg, 0, sizeof(sign_alg));
        // Since Microsoft stop supporting SHA1 since 2016, we only support SHA256 now
        sign_alg.pszObjId = szOID_RSA_SHA256RSA;

        _p = CertCreateSelfSignCertificate(NULL, &nameblob, 0, ckpi, &sign_alg, NULL, (PSYSTEMTIME)&expire_date, 0);

    } while (FALSE);

    if (NULL != hk) {
        CryptDestroyKey(hp);
        hk = NULL;
    }
    if (NULL != hp) {
        CryptReleaseContext(hp, 0);
        hp = NULL;
    }

    return empty() ? false : true;
}

bool cert_context::create(const unsigned char* pb, unsigned long cb)
{
    return (NULL != attach(CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pb, cb)));
}

bool cert_context::create(const std::wstring& file)
{
    if (is_pe_file(file)) {
        return create_from_pe_file(file);
    }
    else {
        const std::vector<unsigned char>& cert_blob = get_cert_binary_blob(file);
        if (cert_blob.empty()) {
            return false;
        }
        return create(cert_blob.data(), (unsigned long)cert_blob.size());
    }
}

cert_info cert_context::get_cert_info()
{
    cert_info certinf;

    certinf._self_signed = is_self_signed();
    certinf._key_spec = get_key_spec();
    certinf._key_bits_length = get_key_bits_length();
    certinf._subject_name = get_subject_name();
    certinf._issuer_name = get_issuer_name();
    certinf._friendly_name = get_friendly_name();
    certinf._serial_number = get_serial_number();
    certinf._thumbprint = get_thumbprint();
    certinf._key_blob = get_key_blob();
    get_date_stamp(certinf._date_stamp);
    get_algorithms(certinf._sign_algorithm, certinf._hash_algorithm);
    get_valid_time(certinf._valid_from, certinf._valid_through);

    return std::move(certinf);
}

bool cert_context::is_pe_file(const std::wstring& file)
{
    bool result = false;
    HANDLE h = INVALID_HANDLE_VALUE;
    IMAGE_DOS_HEADER dos_header = { 0 };
    IMAGE_NT_HEADERS nt_header = { 0 };
    ULONG bytes_read = 0;

    h = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    do {

        if (!::ReadFile(h, &dos_header, (ULONG)sizeof(dos_header), &bytes_read, NULL)) {
            break;
        }
        if (bytes_read != (ULONG)sizeof(dos_header)) {
            break;
        }
        if (dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
            break;
        }

        if (INVALID_SET_FILE_POINTER == ::SetFilePointer(h, dos_header.e_lfanew, NULL, FILE_BEGIN)) {
            break;
        }
        if (!::ReadFile(h, &nt_header, (ULONG)sizeof(nt_header), &bytes_read, NULL)) {
            break;
        }
        if (bytes_read != (ULONG)sizeof(nt_header)) {
            break;
        }
        if (nt_header.Signature != 0x00004550) {
            break;
        }

        result = true;

    } while (FALSE);
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;

    return result;
}

bool cert_context::create_from_pe_file(const std::wstring& file)
{
    bool result = false;
    DWORD dwEncoding = 0, dwContentType = 0, dwFormatType = 0;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;

    if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                          file.c_str(),
                          CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                          CERT_QUERY_FORMAT_FLAG_BINARY,
                          0,
                          &dwEncoding,
                          &dwContentType,
                          &dwFormatType,
                          &hStore,
                          &hMsg,
                          NULL)) {
        return false;
    }

    assert(NULL != hStore);
    assert(NULL != hMsg);

    do {

        DWORD signer_info_size = 0;
        std::vector<UCHAR> signer_data;
        PCMSG_SIGNER_INFO signer_info = NULL;
        CERT_INFO certinf = { 0 };

        if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &signer_info_size)) {
            break;
        }

        // Allocate memory for signer information.
        signer_data.resize(signer_info_size, 0);
        signer_info = reinterpret_cast<PCMSG_SIGNER_INFO>(signer_data.data());
        // Get Signer Information.
        if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)signer_info, &signer_info_size)) {
            break;
        }

        memset(&certinf, 0, sizeof(certinf));
        certinf.Issuer = signer_info->Issuer;
        certinf.SerialNumber = signer_info->SerialNumber;

        _p = CertFindCertificateInStore(hStore, dwEncoding, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&certinf, NULL);
        result = (NULL != _p);

    } while (FALSE);

    CryptMsgClose(hMsg); hMsg = NULL;
    CertCloseStore(hStore, 0); hStore = NULL;
    return result;
}

std::vector<unsigned char> cert_context::get_cert_blob(const std::wstring& file)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    std::vector<unsigned char> blob;

    h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != h) {

        const ULONG file_size = GetFileSize(h, NULL);
        if (file_size != INVALID_FILE_SIZE) {

            if (file_size != 0) {

                ULONG bytes_read = 0;
                blob.resize(file_size, 0);
                if (!::ReadFile(h, blob.data(), (ULONG)blob.size(), &bytes_read, NULL)) {
                    blob.clear();
                }
            }
            else {
                SetLastError(ERROR_INVALID_DATA);
            }
        }

        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return std::move(blob);
}

std::vector<unsigned char> cert_context::get_cert_binary_blob(const std::wstring& file)
{
    std::vector<unsigned char> buf = get_cert_blob(file);
    if (buf.empty()) {
        return buf;
    }

    // Is file content BASE64
    if (0x30 != buf[0]) {
        std::string ls(buf.begin(), buf.end());
        DWORD dwBinarySize = 0;
        if (CryptStringToBinaryA(ls.c_str(), 0, CRYPT_STRING_ANY, NULL, &dwBinarySize, NULL, NULL) && 0 != dwBinarySize) {
            buf.clear();
            buf.resize(dwBinarySize, 0);
            if (!CryptStringToBinaryA(ls.c_str(), 0, CRYPT_STRING_ANY, buf.data(), &dwBinarySize, NULL, NULL)) {
                buf.clear();
            }
        }
    }

    return std::move(buf);
}

std::vector<unsigned char> cert_context::get_property(unsigned long id)
{
    std::vector<unsigned char> prop;
    unsigned long size = 0;

    if (!empty()) {
        if (CertGetCertificateContextProperty(_p, id, NULL, &size) && 0 != size) {
            prop.resize(size, 0);
            if (!CertGetCertificateContextProperty(_p, id, prop.data(), &size)) {
                prop.clear();
            }
        }
    }

    return std::move(prop);
}

std::vector<unsigned char> cert_context::decode_object(unsigned long encoding_type, const char* struc_type, const unsigned char* data, unsigned long data_size, unsigned long flags)
{
    std::vector<unsigned char> output_buf;
    unsigned long output_size = 0;

    // decode object
    if (CryptDecodeObject(encoding_type, struc_type, data, data_size, flags, NULL, &output_size) && 0 != output_size) {
        output_buf.resize(output_size, 0);
        if (!CryptDecodeObject(encoding_type, struc_type, data, data_size, flags, output_buf.data(), &output_size)) {
            output_buf.clear();
        }
    }

    return std::move(output_buf);
}

std::wstring cert_context::get_subject_name()
{
    std::wstring s;
    if (!empty()) {
        if (!CertGetNameStringW(_p, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NX::string_buffer<wchar_t>(s, MAX_PATH), MAX_PATH)) {
            s.clear();
        }
    }
    return std::move(s);
}
std::wstring cert_context::get_issuer_name()
{
    std::wstring s;
    if (!empty()) {
        if (!CertGetNameStringW(_p, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NX::string_buffer<wchar_t>(s, MAX_PATH), MAX_PATH)) {
            s.clear();
        }
    }
    return std::move(s);
}
std::wstring cert_context::get_friendly_name()
{
    const std::vector<unsigned char>& prop = get_property(CERT_FRIENDLY_NAME_PROP_ID);
    if (!prop.empty() && prop.size() >= sizeof(wchar_t)) {
        const wchar_t* p = (const wchar_t*)prop.data();
        const size_t len = prop.size() / sizeof(wchar_t);
        return std::wstring(p, p + len);
    }
    return std::wstring();
}
void cert_context::get_algorithms(std::wstring& sign_algorithm, std::wstring& hash_algorithm)
{
    const std::vector<unsigned char>& prop = get_property(CERT_SIGN_HASH_CNG_ALG_PROP_ID);
    if (!prop.empty() && prop.size() >= sizeof(wchar_t)) {
        const wchar_t* p = (const wchar_t*)prop.data();
        const size_t len = prop.size() / sizeof(wchar_t);
        std::wstring s(p, p + len);

        std::wstring::size_type pos = s.find(L'/');
        if (pos != std::wstring::npos) {
            sign_algorithm = s.substr(0, pos);
            hash_algorithm = s.substr(pos + 1);
        }
    }
}
std::vector<unsigned char> cert_context::get_serial_number()
{
    if (!empty()) {
        const unsigned long size = _p->pCertInfo->SerialNumber.cbData;
        const unsigned char* data = _p->pCertInfo->SerialNumber.pbData;
        if (0 != size && NULL != data) {
            return std::move(std::vector<unsigned char>(data, data + size));
        }
    }
    return std::vector<unsigned char>();
}
std::vector<unsigned char> cert_context::get_thumbprint()
{
    return std::move(get_property(CERT_SHA1_HASH_PROP_ID));
}
std::vector<unsigned char> cert_context::get_signature_hash()
{
    return std::move(get_property(CERT_SIGNATURE_HASH_PROP_ID));
}
std::vector<unsigned char> cert_context::get_key_blob()
{
    if (has_private_key()) {
        return get_private_key_blob();
    }
    else {
        return get_public_key_blob();
    }
}
void cert_context::get_date_stamp(FILETIME& date_stamp)
{
    const std::vector<unsigned char>& prop = get_property(CERT_DATE_STAMP_PROP_ID);
    if (prop.size() == sizeof(FILETIME)) {
        memcpy(&date_stamp, prop.data(), sizeof(FILETIME));
    }
}
void cert_context::get_valid_time(FILETIME& valid_from, FILETIME& valid_through)
{
    memcpy(&valid_from, &_p->pCertInfo->NotBefore, sizeof(FILETIME));
    memcpy(&valid_through, &_p->pCertInfo->NotAfter, sizeof(FILETIME));
}
unsigned long cert_context::get_key_spec()
{
    unsigned long key_spec = 0;
    const std::vector<unsigned char>& prop = get_property(CERT_KEY_SPEC_PROP_ID);
    if (prop.size() == sizeof(unsigned long)) {
        key_spec = *((unsigned long*)prop.data());
    }
    return key_spec;
}
unsigned long cert_context::get_key_bits_length()
{
    if (empty()) {
        return 0;
    }
    return CertGetPublicKeyLength(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &(_p->pCertInfo->SubjectPublicKeyInfo));
}
bool cert_context::is_self_signed()
{
    return (!empty() && CertCompareCertificateName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &(_p->pCertInfo->Issuer), &(_p->pCertInfo->Subject)));
}
bool cert_context::has_private_key()
{
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE h = NULL;
    unsigned long   key_spec = 0;
    BOOL    caller_free = FALSE;

    // searching for cert with private key
    if (CryptAcquireCertificatePrivateKey(_p, CRYPT_ACQUIRE_COMPARE_KEY_FLAG, NULL, &h, &key_spec, &caller_free)) {
        if (caller_free) {
            CryptReleaseContext(h, 0);
            h = NULL;
        }
        return true;
    }

    return false;
}

std::vector<unsigned char> cert_context::get_private_key_blob()
{
    HCRYPTKEY   hk = NULL;
    HCRYPTPROV  hp = NULL;
    DWORD       key_spec = 0;
    BOOL        caller_free = FALSE;
    DWORD       key_size = 0;
    std::vector<unsigned char> key_buf;

    do {

        // searching for cert with private key
        if (!CryptAcquireCertificatePrivateKey(_p, CRYPT_ACQUIRE_COMPARE_KEY_FLAG, NULL, &hp, &key_spec, &caller_free)) {
            break;
        }

        // get private key
        if (!CryptGetUserKey(hp, key_spec, &hk)) {
            break;
        }

        // export key
        if (!CryptExportKey(hk, NULL, PRIVATEKEYBLOB, 0, NULL, &key_size) && ERROR_MORE_DATA != GetLastError()) {
            break;
        }

        if (0 == key_size) {
            SetLastError(ERROR_NOT_FOUND);
            break;
        }

        key_buf.resize(key_size, 0);
        if (!CryptExportKey(hk, NULL, PRIVATEKEYBLOB, 0, key_buf.data(), &key_size)) {
            key_buf.clear();
            break;
        }


    } while (FALSE);

    if (NULL != hk) {
        CryptDestroyKey(hk);
        hk = NULL;
    }
    if (NULL != hp) {
        if (caller_free) {
            CryptReleaseContext(hp, 0);
        }
        hp = NULL;
    }

    return std::move(key_buf);
}

std::vector<unsigned char> cert_context::get_public_key_blob()
{
    if (!empty()) {

        return decode_object(X509_ASN_ENCODING,
                             RSA_CSP_PUBLICKEYBLOB,
                             (const unsigned char*)_p->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                             _p->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                             0);
    }

    return std::vector<unsigned char>();
}
