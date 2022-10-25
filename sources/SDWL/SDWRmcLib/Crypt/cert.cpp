

#include "..\stdafx.h"

#include "..\common\string.h"
#include "cert.h"


using namespace NX;
using namespace NX::crypt;

CertInfo::CertInfo()
    : _dateStamp({ 0, 0 }), _validFrom({ 0, 0 }), _validThrough({ 0, 0 }), _keySpec(0), _keyBitsLength(0), _selfSigned(false)
{
}

CertInfo::CertInfo(const CertInfo& rhs)
    : _subjectName(rhs._subjectName),
    _issuerName(rhs._issuerName),
    _friendlyName(rhs._friendlyName),
    _signAlgorithm(rhs._signAlgorithm),
    _hashAlgorithm(rhs._hashAlgorithm),
    _serialNumber(rhs._serialNumber),
    _thumbPrint(rhs._thumbPrint),
    _signatureHash(rhs._signatureHash),
    _keyBlob(rhs._keyBlob),
    _dateStamp({ rhs._dateStamp.dwLowDateTime, rhs._dateStamp.dwHighDateTime }),
    _validFrom({ rhs._validFrom.dwLowDateTime, rhs._validFrom.dwHighDateTime }),
    _validThrough({ rhs._validThrough.dwLowDateTime, rhs._validThrough.dwHighDateTime }),
    _keySpec(rhs._keySpec),
    _keyBitsLength(rhs._keyBitsLength),
    _selfSigned(rhs._selfSigned)
{
}

CertInfo::~CertInfo()
{
}

CertInfo& CertInfo::operator = (const CertInfo& rhs)
{
    if (this != &rhs) {
        _subjectName = rhs._subjectName;
        _issuerName = rhs._issuerName;
        _friendlyName = rhs._friendlyName;
        _signAlgorithm = rhs._signAlgorithm;
        _hashAlgorithm = rhs._hashAlgorithm;
        _serialNumber = rhs._serialNumber;
        _thumbPrint = rhs._thumbPrint;
        _signatureHash = rhs._signatureHash;
        _keyBlob = rhs._keyBlob;
        _dateStamp.dwLowDateTime = rhs._dateStamp.dwLowDateTime;
        _dateStamp.dwHighDateTime = rhs._dateStamp.dwHighDateTime;
        _validFrom.dwLowDateTime = rhs._validFrom.dwLowDateTime;
        _validFrom.dwHighDateTime = rhs._validFrom.dwHighDateTime;
        _validThrough.dwLowDateTime = rhs._validThrough.dwLowDateTime;
        _validThrough.dwHighDateTime = rhs._validThrough.dwHighDateTime;
        _keySpec = rhs._keySpec;
        _keyBitsLength = rhs._keyBitsLength;
        _selfSigned = rhs._selfSigned;
    }
    return *this;
}


//
//
//

CertContext::CertContext() : _p(nullptr)
{
}

CertContext::CertContext(PCCERT_CONTEXT p) : _p(p)
{
}

CertContext::~CertContext()
{
    Clear();
}

SDWLResult CertContext::Create(const std::wstring& x500_name, unsigned long key_spec, bool strong, bool machineKeySet, _In_opt_ CertContext* issuer, const SYSTEMTIME& expire_date)
{
    HCRYPTPROV              hp = NULL;
    HCRYPTKEY               hk = NULL;
    PCRYPT_KEY_PROV_INFO    ckpi = NULL;
    std::vector<UCHAR>      ckpiBuf;
    CERT_NAME_BLOB          nameBlob;
    std::vector<UCHAR>      nameBuf;
    SDWLResult                  res = RESULT(0);

    do {

        // Get key provider information
        if (NULL != issuer && !issuer->Empty()) {
            ULONG ckpiSize = 0;
            if (CertGetCertificateContextProperty(*issuer, CERT_KEY_PROV_INFO_PROP_ID, NULL, &ckpiSize) && 0 != ckpiSize) {
                ckpiBuf.resize(ckpiSize, 0);
                if (!CertGetCertificateContextProperty(*issuer, CERT_KEY_PROV_INFO_PROP_ID, ckpiBuf.data(), &ckpiSize)) {
                    ckpiBuf.clear();
                }
                else {
                    ckpi = (PCRYPT_KEY_PROV_INFO)ckpiBuf.data();
                }
            }
        }

        // Acquire provider
		ULONG contextFlags = machineKeySet ? (CRYPT_MACHINE_KEYSET | CRYPT_SILENT) : CRYPT_SILENT;
        if (!CryptAcquireContextW(&hp, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, contextFlags)) {

            if (GetLastError() != NTE_BAD_KEYSET && GetLastError() != NTE_BAD_KEY_STATE) {
                // Error other than bad keyset
                res = RESULT(GetLastError());
                break;
            }

			if (GetLastError() == NTE_BAD_KEY_STATE) {
				// If KEY_STATE error, delete it, and try to create new one later
				if (!CryptAcquireContextW(&hp, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, contextFlags | CRYPT_DELETEKEYSET)) {
					// Error other than bad keyset
					res = RESULT(GetLastError());
					break;
				}
			}

            // Keyset doesn't exist
            if (!CryptAcquireContextW(&hp, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, CRYPT_NEWKEYSET | contextFlags)) {
                res = RESULT(GetLastError());
                break;
            }
        }

        // Set subject name blob
        if (!CertStrToNameW(X509_ASN_ENCODING, x500_name.c_str(), CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG, NULL, NULL, &nameBlob.cbData, NULL)) {
            res = RESULT(GetLastError());
            break;
        }

        nameBuf.resize(nameBlob.cbData, 0);
        nameBlob.pbData = nameBuf.data();
        if (!CertStrToNameW(X509_ASN_ENCODING, x500_name.c_str(), CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG, NULL, nameBlob.pbData, &nameBlob.cbData, NULL)) {
            res = RESULT(GetLastError());
            break;
        }

        // Generate key
        const unsigned long key_flags = (strong ? 0x08000000/*2048 Bits Key*/ : RSA1024BIT_KEY) | CRYPT_EXPORTABLE;
        if (!CryptGenKey(hp, key_spec, key_flags, &hk)) {
            res = RESULT(GetLastError());
            break;
        }

        // Prepare algorithm structure
        CRYPT_ALGORITHM_IDENTIFIER sign_alg;
        memset(&sign_alg, 0, sizeof(sign_alg));
        // Since Microsoft stop supporting SHA1 since 2016, we only support SHA256 now
        sign_alg.pszObjId = szOID_RSA_SHA256RSA;

        _p = CertCreateSelfSignCertificate(hp, &nameBlob, 0, ckpi, &sign_alg, NULL, (PSYSTEMTIME)&expire_date, 0);
        if (NULL == _p) {
            res = RESULT(GetLastError());
            break;
        }

    } while (FALSE);

    if (NULL != hk) {
        CryptDestroyKey(hp);
        hk = NULL;
    }
    if (NULL != hp) {
        CryptReleaseContext(hp, 0);
        hp = NULL;
    }

    return res;
}

SDWLResult CertContext::Create(const unsigned char* pb, unsigned long cb)
{
    _p = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pb, cb);
    if (NULL == _p)
        return RESULT(GetLastError());

    return RESULT(0);
}

SDWLResult CertContext::Create(const std::wstring& file)
{
    std::vector<UCHAR> certBlob;
    SDWLResult res = GetCertBinaryBlob(file, certBlob);
    if (!res) {
        return res;
    }
    return Create(certBlob.data(), (ULONG)certBlob.size());
}

SDWLResult CertContext::CreateFromEmbeddedSignature(const std::wstring& host_file)
{
    DWORD dwEncoding = 0, dwContentType = 0, dwFormatType = 0;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;

    if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                          host_file.c_str(),
                          CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                          CERT_QUERY_FORMAT_FLAG_BINARY,
                          0,
                          &dwEncoding,
                          &dwContentType,
                          &dwFormatType,
                          &hStore,
                          &hMsg,
                          NULL)) {
        return RESULT(GetLastError());
    }

    assert(NULL != hStore);
    assert(NULL != hMsg);

    SDWLResult res = RESULT(0);

    do {

        DWORD signerInfoSize = 0;
        std::vector<UCHAR> signerData;
        PCMSG_SIGNER_INFO signerInfo = NULL;
        CERT_INFO certInf = { 0 };

        if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &signerInfoSize)) {
            res = RESULT(GetLastError());
            break;
        }

        // Allocate memory for signer information.
        signerData.resize(signerInfoSize, 0);
        signerInfo = reinterpret_cast<PCMSG_SIGNER_INFO>(signerData.data());
        // Get Signer Information.
        if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)signerInfo, &signerInfoSize)) {
            res = RESULT(GetLastError());
            break;
        }

        memset(&certInf, 0, sizeof(certInf));
        certInf.Issuer = signerInfo->Issuer;
        certInf.SerialNumber = signerInfo->SerialNumber;

        _p = CertFindCertificateInStore(hStore, dwEncoding, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&certInf, NULL);
        if (NULL == _p) {
            res = RESULT(GetLastError());
            break;
        }

    } while (FALSE);

    CryptMsgClose(hMsg); hMsg = NULL;
    CertCloseStore(hStore, 0); hStore = NULL;
    return res;
}

SDWLResult CertContext::Export(_In_ const std::wstring& file, _In_ bool base64_format)
{
    if (Empty()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    SDWLResult res = RESULT(0);
    HANDLE hf = INVALID_HANDLE_VALUE;
    std::string s;


    do {

        const unsigned char* pbCertData = _p->pbCertEncoded;
        unsigned long cbCertData = _p->cbCertEncoded;

        if (base64_format) {

            unsigned long size = 0;
            CryptBinaryToStringA(_p->pbCertEncoded, _p->cbCertEncoded, CRYPT_STRING_BASE64HEADER, NULL, &size);
            if (0 == size) {
                res = RESULT(GetLastError());
                break;
            }

            s.resize(size, 0);
            if (!CryptBinaryToStringA(_p->pbCertEncoded, _p->cbCertEncoded, CRYPT_STRING_BASE64HEADER, (char*)s.data(), &size))
            {
                res = RESULT(GetLastError());
                s.clear();
                break;
            }

            pbCertData = (const unsigned char*)s.c_str();
            cbCertData = (unsigned long)s.length();
        }

        hf = ::CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hf) {
            res = RESULT(GetLastError());
            break;
        }

        unsigned long bytes_written = 0;

        if (!::WriteFile(hf, pbCertData, cbCertData, &bytes_written, NULL)) {
            res = RESULT(GetLastError());
            break;
        }
        
    } while (FALSE);

    if (INVALID_HANDLE_VALUE != hf) {
        CloseHandle(hf);
        hf = INVALID_HANDLE_VALUE;
        if (!res) {
            ::DeleteFileW(file.c_str());
        }
    }

    return res;
}

SDWLResult CertContext::Duplicate(CertContext* pcc)
{
    if (Empty()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }

    pcc->Attach(CertDuplicateCertificateContext(_p));
    if (pcc->Empty()) {
        return RESULT(GetLastError());
    }

    return RESULT(0);
}

void CertContext::Clear()
{
    if (nullptr != _p) {
        CertFreeCertificateContext(_p);
        _p = nullptr;
    }
}

CertInfo CertContext::GetCertInfo()
{
    CertInfo certInf;

    if (!Empty()) {
        certInf._selfSigned = IsSelfSigned();
        certInf._keySpec = GetKeySpec();
        certInf._keyBitsLength = GetKeyBitsLength();
        certInf._subjectName = GetSubjectName();
        certInf._issuerName = GetIssuerName();
        certInf._friendlyName = GetFriendlyName();
        certInf._serialNumber = GetSerialNumber();
        certInf._thumbPrint = GetThumbprint();
        (void)GetKeyBlob(certInf._keyBlob);
        GetDateStamp(certInf._dateStamp);
        GetAlgorithms(certInf._signAlgorithm, certInf._hashAlgorithm);
        GetValidTime(certInf._validFrom, certInf._validThrough);
    }

    return certInf;
}

std::wstring CertContext::GetSubjectName()
{
    std::wstring s;
    if (!Empty()) {
        if (!CertGetNameStringW(_p, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NX::wstring_buffer(s, MAX_PATH), MAX_PATH)) {
            s.clear();
        }
    }
    return s;
}

std::wstring CertContext::GetIssuerName()
{
    std::wstring s;
    if (!Empty()) {
        if (!CertGetNameStringW(_p, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NX::wstring_buffer(s, MAX_PATH), MAX_PATH)) {
            s.clear();
        }
    }
    return s;
}

std::wstring CertContext::GetFriendlyName()
{
    const std::vector<unsigned char>& prop = GetProperty(CERT_FRIENDLY_NAME_PROP_ID);
    if (!prop.empty() && prop.size() >= sizeof(wchar_t)) {
        const wchar_t* p = (const wchar_t*)prop.data();
        const size_t len = prop.size() / sizeof(wchar_t);
        return std::wstring(p, p + len);
    }
    return std::wstring();
}

void CertContext::GetAlgorithms(std::wstring& sign_algorithm, std::wstring& hash_algorithm)
{
    if (Empty()) {
        sign_algorithm.clear();
        hash_algorithm.clear();
        return;
    }

    const std::vector<unsigned char>& prop = GetProperty(CERT_SIGN_HASH_CNG_ALG_PROP_ID);
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

std::vector<UCHAR> CertContext::GetSerialNumber()
{
    if (!Empty()) {
        const unsigned long size = _p->pCertInfo->SerialNumber.cbData;
        const unsigned char* data = _p->pCertInfo->SerialNumber.pbData;
        if (0 != size && NULL != data) {
            return std::vector<UCHAR>(data, data + size);
        }
    }
    return std::vector<UCHAR>();
}

std::vector<UCHAR> CertContext::GetThumbprint()
{
    if (Empty()) {
        return std::vector<UCHAR>();
    }

    return GetProperty(CERT_SHA1_HASH_PROP_ID);
}

std::vector<UCHAR> CertContext::GetSignatureHash()
{
    if (Empty()) {
        return std::vector<UCHAR>();
    }

    return GetProperty(CERT_SIGNATURE_HASH_PROP_ID);
}

SDWLResult CertContext::GetKeyBlob(std::vector<UCHAR>& blob)
{
    if (Empty()) {
        return RESULT(ERROR_INVALID_DATA);
    }

    if (HasPrivateKey()) {
        return GetPrivateKeyBlob(blob);
    }
    else {
        return GetPublicKeyBlob(blob);
    }
}

void CertContext::GetDateStamp(FILETIME& date_stamp)
{
    const std::vector<UCHAR>& prop = GetProperty(CERT_DATE_STAMP_PROP_ID);
    if (prop.size() == sizeof(FILETIME)) {
        memcpy(&date_stamp, prop.data(), sizeof(FILETIME));
    }
    else {
        memset(&date_stamp, 0, sizeof(FILETIME));
    }
}

void CertContext::GetValidTime(FILETIME& valid_from, FILETIME& valid_through)
{
    if (!Empty()) {
        memcpy(&valid_from, &_p->pCertInfo->NotBefore, sizeof(FILETIME));
        memcpy(&valid_through, &_p->pCertInfo->NotAfter, sizeof(FILETIME));
    }
    else {
        memset(&valid_from, 0, sizeof(FILETIME));
        memset(&valid_through, 0, sizeof(FILETIME));
    }
}

ULONG CertContext::GetKeySpec()
{
    unsigned long key_spec = 0;
    const std::vector<unsigned char>& prop = GetProperty(CERT_KEY_SPEC_PROP_ID);
    if (prop.size() == sizeof(unsigned long)) {
        key_spec = *((unsigned long*)prop.data());
    }
    return key_spec;
}

ULONG CertContext::GetKeyBitsLength()
{
    if (Empty()) {
        return 0;
    }
    return CertGetPublicKeyLength(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &(_p->pCertInfo->SubjectPublicKeyInfo));
}

bool CertContext::IsSelfSigned()
{
    if (Empty()) {
        return false;
    }

    return CertCompareCertificateName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &(_p->pCertInfo->Issuer), &(_p->pCertInfo->Subject)) ? true : false;
}

bool CertContext::HasPrivateKey()
{
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE h = NULL;
    ULONG   keySpec = 0;
    BOOL    callerFree = FALSE;

    if (Empty()) {
        return false;
    }

    // searching for cert with private key
    if (CryptAcquireCertificatePrivateKey(_p, CRYPT_ACQUIRE_COMPARE_KEY_FLAG, NULL, &h, &keySpec, &callerFree)) {
        if (callerFree) {
            CryptReleaseContext(h, 0);
            h = NULL;
        }
        return true;
    }

    return false;
}

SDWLResult CertContext::GetPrivateKeyBlob(std::vector<UCHAR>& blob)
{
    HCRYPTKEY   hk = NULL;
    HCRYPTPROV  hp = NULL;
    DWORD       keySpec = 0;
    BOOL        callerFree = FALSE;
    DWORD       keySize = 0;
    SDWLResult res = RESULT(0);

    do {

        // searching for cert with private key
        if (!CryptAcquireCertificatePrivateKey(_p, CRYPT_ACQUIRE_COMPARE_KEY_FLAG, NULL, &hp, &keySpec, &callerFree)) {
            res = RESULT(GetLastError());
            break;
        }

        // get private key
        if (!CryptGetUserKey(hp, keySpec, &hk)) {
            res = RESULT(GetLastError());
            break;
        }

        // export key
        if (!CryptExportKey(hk, NULL, PRIVATEKEYBLOB, 0, NULL, &keySize) && ERROR_MORE_DATA != GetLastError()) {
            res = RESULT(GetLastError());
            break;
        }

        if (0 == keySize) {
            res = RESULT(ERROR_NOT_FOUND);
            break;
        }

        blob.resize(keySize, 0);
        if (!CryptExportKey(hk, NULL, PRIVATEKEYBLOB, 0, blob.data(), &keySize)) {
            res = RESULT(GetLastError());
            blob.clear();
            break;
        }

    } while (FALSE);

    if (NULL != hk) {
        CryptDestroyKey(hk);
        hk = NULL;
    }
    if (NULL != hp) {
        if (callerFree) {
            CryptReleaseContext(hp, 0);
        }
        hp = NULL;
    }
    if (!res)
        return res;

    return RESULT(0);
}

SDWLResult CertContext::GetPublicKeyBlob(std::vector<UCHAR>& blob)
{
    if (!Empty()) {

        blob = DecodeObject(X509_ASN_ENCODING,
                            RSA_CSP_PUBLICKEYBLOB,
                            (const UCHAR*)_p->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                            _p->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                            0);
        return blob.empty() ? RESULT(GetLastError()) : RESULT(0);
    }
    else {
        return RESULT(ERROR_INVALID_DATA);
    }
}

std::vector<UCHAR> CertContext::GetProperty(ULONG id)
{
    std::vector<UCHAR> prop;
    ULONG size = 0;

    if (!Empty()) {
        if (CertGetCertificateContextProperty(_p, id, NULL, &size) && 0 != size) {
            prop.resize(size, 0);
            if (!CertGetCertificateContextProperty(_p, id, prop.data(), &size)) {
                prop.clear();
            }
        }
    }

    return prop;
}

std::vector<UCHAR> CertContext::DecodeObject(ULONG encoding_type, const CHAR* struc_type, const UCHAR* data, ULONG data_size, ULONG flags)
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

    return output_buf;
}

SDWLResult CertContext::GetCertBlob(const std::wstring& file, std::vector<UCHAR>& blob)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    SDWLResult res = RESULT(0);

    do {

        h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            res = RESULT(GetLastError());
            break;
        }
    
        const ULONG fileSize = GetFileSize(h, NULL);
        if (fileSize == INVALID_FILE_SIZE) {
            res = RESULT(GetLastError());
            break;
        }
        if (0 == fileSize) {
            res = RESULT(ERROR_INVALID_DATA);
            break;
        }

        blob.resize(fileSize, 0);
        ULONG bytesRead = 0;
        if (!::ReadFile(h, blob.data(), (ULONG)blob.size(), &bytesRead, NULL)) {
            res = RESULT(GetLastError());
            break;
        }

    } while (FALSE);

    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
    if (!res) {
        blob.clear();
        return res;
    }

    return RESULT(0);
}

SDWLResult CertContext::GetCertBinaryBlob(const std::wstring& file, std::vector<UCHAR>& blob)
{
    std::vector<unsigned char> buf;
    
    SDWLResult res = GetCertBlob(file, buf);
    if (!res) {
        return res;
    }

    // Is file content BASE64
    if (0x30 != buf[0]) {
        std::string ls(buf.begin(), buf.end());
        DWORD dwBinarySize = 0;
        if (!CryptStringToBinaryA(ls.c_str(), 0, CRYPT_STRING_ANY, NULL, &dwBinarySize, NULL, NULL)) {
            return RESULT(GetLastError());
        }
        if (0 == dwBinarySize) {
            return RESULT(ERROR_INVALID_DATA);
        }
        blob.resize(dwBinarySize, 0);
        if (!CryptStringToBinaryA(ls.c_str(), 0, CRYPT_STRING_ANY, blob.data(), &dwBinarySize, NULL, NULL)) {
            blob.clear();
            return RESULT(GetLastError());
        }
        if (dwBinarySize != (ULONG)blob.size())
            blob.resize(dwBinarySize);
    }

    return RESULT(0);
}


CertStore::CertStore() : _h(NULL)
{
}

CertStore::CertStore(HCERTSTORE h) : _h(h)
{
}

CertStore::~CertStore()
{
    Close();
}

void CertStore::Close()
{
    if (NULL != _h) {
        (VOID)CertCloseStore(_h, 0);
        _h = NULL;
    }
}

SDWLResult CertStore::EnumCertificates(ENUMCERTPROC EnumProc, PVOID context)
{
    PCCERT_CONTEXT cert = NULL;

    if (!Opened()) {
        return RESULT2(ERROR_INVALID_HANDLE, "Store is not opened");
    }

    while (NULL != (cert = CertEnumCertificatesInStore(_h, cert))) {

        BOOL bStop = FALSE;
        CertContext cc(cert);
        EnumProc(cc, context, &bStop);
        cc.Detach();    // Detach here to avoid auto-free
        if (bStop) {
            // If manually stop, we need to free current cert
            CertFreeCertificateContext(cert);
            cert = NULL;
            break;
        }
    }

    assert(NULL == cert);
    return RESULT(0);
}

SDWLResult CertStore::AddCertificate(PCCERT_CONTEXT cc, DWORD addDisposition, CertContext* pcc)
{
    if (!Opened()) {
        return RESULT(ERROR_INVALID_HANDLE);
    }
    if (NULL == cc) {
        return RESULT(ERROR_INVALID_PARAMETER);
    }

    PCCERT_CONTEXT dupcc = NULL;

    if (!CertAddCertificateContextToStore(_h, cc, addDisposition, pcc ? (&dupcc) : NULL)) {
        return RESULT2(GetLastError(), "CertAddCertificateContextToStore failed");
    }

    if (pcc) {
        pcc->Attach(dupcc);
    }

    return RESULT(0);
}


CertSystemStore::CertSystemStore() : CertStore()
{
}

CertSystemStore::~CertSystemStore()
{
}

SDWLResult CertSystemStore::OpenPersonalStore(bool machineStore)
{
    return Open(L"My", machineStore);
}

SDWLResult CertSystemStore::OpenTrustRootStore(bool machineStore)
{
    return Open(L"Root", machineStore);
}

SDWLResult CertSystemStore::OpenInterMediateStore(bool machineStore)
{
    return Open(L"CertificateAuthority", machineStore);
}

SDWLResult CertSystemStore::OpenTrustPublisherStore(bool machineStore)
{
    return Open(L"TrustedPublisher", machineStore);
}

SDWLResult CertSystemStore::OpenTrustPeopleStore(bool machineStore)
{
    return Open(L"TrustedPeople", machineStore);
}

SDWLResult CertSystemStore::OpenOtherPeopleStore(bool machineStore)
{
    return Open(L"AddressBook", machineStore);
}

SDWLResult CertSystemStore::OpenThirdPartyRootStore(bool machineStore)
{
    return Open(L"AuthRoot", machineStore);
}

SDWLResult CertSystemStore::OpenRevokedStore(bool machineStore)
{
    return Open(L"Disallowed", machineStore);
}

SDWLResult CertSystemStore::Open(const std::wstring& name, bool machineStore)
{
    Close();
    const DWORD flags = machineStore ? CERT_SYSTEM_STORE_LOCAL_MACHINE : CERT_SYSTEM_STORE_CURRENT_USER;
    HCERTSTORE h = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                 X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                 NULL,
                                 flags | CERT_STORE_NO_CRYPT_RELEASE_FLAG | CERT_STORE_OPEN_EXISTING_FLAG,
                                 name.c_str());
    if (NULL == h) {
        return RESULT2(GetLastError(), "CertOpenStore failed");
    }

    CertStore::Attach(h);
    return RESULT(0);
}

CertMemoryStore::CertMemoryStore() : CertStore(CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL))
{
}

CertMemoryStore::~CertMemoryStore()
{
}

CertPcks12FileStore::CertPcks12FileStore() : CertStore()
{
}

CertPcks12FileStore::~CertPcks12FileStore()
{
}

SDWLResult CertPcks12FileStore::OpenPfxCert(const std::wstring& file, const std::wstring& password, bool exportable)
{
    std::vector<UCHAR> blob;

    Close();

    SDWLResult res = ReadCertContent(file, blob);
    if (!res) {
        return res;
    }

    CRYPT_DATA_BLOB cryptDataBlob = { (ULONG)blob.size(), blob.data() };
    HCERTSTORE h = PFXImportCertStore(&cryptDataBlob, password.c_str(), CRYPT_EXPORTABLE);
    if (NULL == h) {
        return RESULT(GetLastError());
    }

    CertStore::Attach(h);
    return RESULT(0);
}

SDWLResult CertPcks12FileStore::ReadCertContent(const std::wstring& file, std::vector<UCHAR>& blob)
{
    HANDLE h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return RESULT(GetLastError());
    }

    SDWLResult res = RESULT(0);

    do {

        const ULONG fileSize = GetFileSize(h, NULL);
        if (fileSize == INVALID_FILE_SIZE) {
            res = RESULT(GetLastError());
            break;
        }
        if (0 == fileSize) {
            res = RESULT(ERROR_INVALID_DATA);
            blob.clear();
            break;
        }

        ULONG bytesRead = 0;
        blob.resize(fileSize, 0);
        if (!::ReadFile(h, blob.data(), fileSize, &bytesRead, NULL)) {
            res = RESULT(GetLastError());
            blob.clear();
            break;
        }

    } while (FALSE);

    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
    return res;
}