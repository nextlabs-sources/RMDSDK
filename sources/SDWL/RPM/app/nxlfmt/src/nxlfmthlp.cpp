

#include <Windows.h>
#include <assert.h>

#include <nudf\bits.hpp>
#include <nudf\crypto.hpp>
#include <nudf\json.hpp>
#include <nudf\conversion.hpp>
#include <nudf\handyutil.hpp>
#include <nudf\string.hpp>
#include <nudf\zip.hpp>

#include "nxlfmthlp.hpp"

using namespace NX;


static const UCHAR ZeroAgreement[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

namespace NXLIMPL {

bool NReadFile(const std::wstring& file, unsigned long field_offset, _Out_writes_bytes_(field_size) void* field_data, unsigned long field_size);
bool NReadFile(HANDLE h, unsigned long field_offset, _Out_writes_bytes_(field_size) void* field_data, unsigned long field_size);
bool NWriteFile(HANDLE h, unsigned long field_offset, _In_reads_bytes_(field_size) const void* field_data, unsigned long field_size);
bool ReadSectionRecord(HANDLE h, const std::string& section_name, _Out_writes_bytes_(sizeof(NXL_SECTION_2)) NXL_SECTION_2* section_record);
bool WriteSectionRecord(HANDLE h, const std::string& section_name, _In_reads_bytes_(sizeof(NXL_SECTION_2)) const NXL_SECTION_2* section_record);
bool ReadSectionData(HANDLE h, const std::string& section_name, std::vector<unsigned char>& section_data, _In_reads_bytes_opt_(32) const unsigned char* file_token, _In_reads_bytes_opt_(16) const unsigned char* ivec_seed);
bool WriteSectionData(HANDLE h, const std::string& section_name, const std::vector<unsigned char>& section_data, _In_reads_bytes_opt_(32) const unsigned char* file_token, _In_reads_bytes_opt_(16) const unsigned char* ivec_seed);

}

NXL::section_record::section_record()
{
    memset(this, 0, sizeof(NXL_SECTION_2));
}

NXL::section_record::section_record(const section_record& other)
{
    memcpy(this, &other, sizeof(NXL_SECTION_2));
}

NXL::section_record::~section_record()
{
}

void NXL::section_record::clear()
{
    memset(this, 0, sizeof(NXL_SECTION_2));
}

NXL::section_record& NXL::section_record::operator = (const NXL::section_record& other)
{
    if (this != &other) {
        memcpy(this, &other, sizeof(NXL_SECTION_2));
    }
    return *this;
}


NXL::document_context::document_context()
{
    memset(this, 0, sizeof(NXL_HEADER_2));
}

NXL::document_context::document_context(const std::wstring& file)
{
    memset(this, 0, sizeof(NXL_HEADER_2));
    load(file);
}

NXL::document_context::document_context(HANDLE h)
{
    memset(this, 0, sizeof(NXL_HEADER_2));
    load(h);
}

NXL::document_context::document_context(const NXL::document_context& other)
{
    memcpy(this, &other, sizeof(NXL_HEADER_2));
}

NXL::document_context::document_context(NXL::document_context&& other)
{
    memcpy(this, &other, sizeof(NXL_HEADER_2));
    memset(&other, 0, sizeof(NXL_HEADER_2));
}

NXL::document_context::~document_context()
{
}

NXL::document_context& NXL::document_context::operator = (const NXL::document_context& other)
{
    if (this != &other) {
        memcpy(this, &other, sizeof(NXL_HEADER_2));
    }
    return *this;
}

NXL::document_context& NXL::document_context::operator = (NXL::document_context&& other)
{
    if (this != &other) {
        memcpy(this, &other, sizeof(NXL_HEADER_2));
        memset(&other, 0, sizeof(NXL_HEADER_2));
    }
    return *this;
}

void NXL::document_context::clear()
{
    memset(this, 0, sizeof(NXL_HEADER_2));
}

bool NXL::document_context::empty() const
{
    return (0 == memcmp(ZeroAgreement, fixed.file_info.duid, 16));
}

bool NXL::document_context::load(const std::wstring& file)
{
    HANDLE h = INVALID_HANDLE_VALUE;

    h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    bool ret = load(h);
    CloseHandle(h);
    return ret;
}

bool NXL::document_context::load(HANDLE h)
{
    ULONG bytes_read = 0;

    SetFilePointer(h, 0, NULL, FILE_BEGIN);
    if (!::ReadFile(h, this, (unsigned long)sizeof(NXL_HEADER_2), &bytes_read, NULL)) {
        return false;
    }
    if (bytes_read != (unsigned long)sizeof(NXL_HEADER_2)) {
        clear();
        return false;
    }

    bool result = false;

    if (fixed.signature.magic.code == NXL_MAGIC_CODE_1) {
        SetLastError(ERROR_EVT_VERSION_TOO_OLD);
    }
    else if (fixed.signature.magic.code == NXL_MAGIC_CODE_2) {
        result = (HIWORD(fixed.signature.version) >= 2) ? true : false;
    }
    else {
        result = false;
    }

    if (!result) {
        clear();
    }

	read_policy_section(h);
	read_tag_section(h);
	read_info_section(h);

    return result;
}

bool NXL::document_context::validate_checksum(const unsigned char* file_token) const
{
    std::vector<unsigned char> buf;
    const unsigned long header_size = (unsigned long)sizeof(NXL_FIXED_HEADER);
    unsigned char checksum[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    buf.resize(4 + header_size, 0);
    memcpy(buf.data(), &header_size, 4);
    memcpy(buf.data() + 4, &fixed, header_size);

    if (!NX::crypto::hmac_sha256(buf.data(), (unsigned long)buf.size(), file_token, 32, checksum)) {
        return false;
    }

    return (0 == memcmp(dynamic.fixed_header_hash, checksum, 32));
}

std::wstring NXL::document_context::get_message() const
{
    const std::string& s = NX::safe_buffer_to_string((const unsigned char*)fixed.signature.message, 244);
    return NX::conversion::utf8_to_utf16(s);
}

std::vector<unsigned char> NXL::document_context::get_duid() const
{
    if (empty()) {
        return std::vector<unsigned char>();
    }
    std::vector<unsigned char> buf;
    buf.resize(16, 0);
    memcpy(buf.data(), fixed.file_info.duid, 16);
    return std::move(buf);
}

std::wstring NXL::document_context::get_owner_id() const
{
    if (empty()) {
        return std::wstring();
    }
    const std::string& s = safe_buffer_to_string((const unsigned char*)fixed.file_info.owner_id, 256);
    return NX::conversion::utf8_to_utf16(s);
}

std::string NXL::document_context::get_tenant() const
{
	if (empty()) {
		return std::string();
	}
	const std::string& ownerid = safe_buffer_to_string((const unsigned char*)fixed.file_info.owner_id, 256);
	auto pos = ownerid.rfind('@');
	std::string tenant = ownerid.substr(pos + 1);
	return (pos == std::string::npos) ? std::string() : ownerid.substr(pos + 1);
}

std::vector<unsigned char> NXL::document_context::get_agreement0() const
{
    if (empty()) {
        return std::vector<unsigned char>();
    }
    if (0 == memcmp(fixed.keys.public_key1, ZeroAgreement, 256)) {
        return std::vector<unsigned char>();
    }
    return std::vector<unsigned char>(fixed.keys.public_key1, fixed.keys.public_key1 + 256);
}

std::vector<unsigned char> NXL::document_context::get_agreement1() const
{
    if (empty()) {
        return std::vector<unsigned char>();
    }
    if (0 == memcmp(fixed.keys.public_key2, ZeroAgreement, 256)) {
        return std::vector<unsigned char>();
    }
    return std::vector<unsigned char>(fixed.keys.public_key2, fixed.keys.public_key2 + 256);
}

bool NXL::document_context::read_policy_section(HANDLE h)
{
	NXL::section_record record = find_section(NXL_SECTION_NAME_FILEPOLICY);
	if (record.empty())
		return false;
	std::vector<unsigned char> buf;
	buf.resize(record.data_size + 4);
	if (!NXLIMPL::NReadFile(h, record.offset, buf.data(), record.data_size))
		return false;

	m_policy = std::string(buf.begin(), buf.end());
	return true;
}

bool NXL::document_context::read_info_section(HANDLE h)
{
	NXL::section_record record = find_section(NXL_SECTION_NAME_FILEINFO);
	if (record.empty())
		return false;
	std::vector<unsigned char> buf;
	buf.resize(record.data_size + 4);
	if (!NXLIMPL::NReadFile(h, record.offset, buf.data(), record.data_size))
		return false;

	m_info = std::string(buf.begin(), buf.end());
	return true;
}

bool NXL::document_context::read_tag_section(HANDLE h)
{
	NXL::section_record record = find_section(NXL_SECTION_NAME_FILETAG);
	if (record.empty())
		return false;
	std::vector<unsigned char> buf;
	buf.resize(record.data_size+4);
	if (!NXLIMPL::NReadFile(h, record.offset, buf.data(), record.data_size)) 
		return false;
	
	m_tags = std::string(buf.begin(), buf.end());
	return true;
}

NXL::section_record NXL::document_context::find_section(const std::string& name) const
{
    if (empty()) {
        SetLastError(ERROR_INVALID_DATA);
        return NXL::section_record();
    }

    for (int i = 0; i < 32; i++) {
        if (!NX::check_bit(fixed.sections.section_map, i)) {
            continue;
        }
        if (fixed.sections.record[i].name[0] == 0 || fixed.sections.record[i].size == 0 || fixed.sections.record[i].offset == 0) {
            continue;
        }
        if (0 == _stricmp(name.c_str(), fixed.sections.record[i].name)) {
            NXL::section_record record;
            memcpy(&record, &fixed.sections.record[i], sizeof(NXL_SECTION_2));
            return record;
        }
    }

    // Not found
    SetLastError(ERROR_NOT_FOUND);
    return NXL::section_record();
}

bool NXL::document_context::find_default_section(NXL::section_record& fileinfo, NXL::section_record& filepolicy, NXL::section_record& filetag) const
{
    if (empty()) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }

    for (int i = 0; i < 32; i++) {

        if (!NX::check_bit(fixed.sections.section_map, i)) {
            continue;
        }
        if (fixed.sections.record[i].name[0] == 0 || fixed.sections.record[i].size == 0 || fixed.sections.record[i].offset == 0) {
            continue;
        }
        if (0 == _stricmp(NXL_SECTION_NAME_FILEINFO, fixed.sections.record[i].name)) {
            memcpy(&fileinfo, &fixed.sections.record[i], sizeof(NXL_SECTION_2));
        }
        else if (0 == _stricmp(NXL_SECTION_NAME_FILEPOLICY, fixed.sections.record[i].name)) {
            memcpy(&filepolicy, &fixed.sections.record[i], sizeof(NXL_SECTION_2));
        }
        else if (0 == _stricmp(NXL_SECTION_NAME_FILETAG, fixed.sections.record[i].name)) {
            memcpy(&filetag, &fixed.sections.record[i], sizeof(NXL_SECTION_2));
        }
        else {
            ; // NOTHING
        }
    }

    return true;
}

bool NXL::document_context::read_section_string(HANDLE h, const NXL::section_record& record, const unsigned char* file_token, std::string& data) const
{
    std::vector<unsigned char> buf;
    if (!read_section_data(h, record, file_token, buf)) {
        return false;
    }
    if (!buf.empty()) {
        data = NX::safe_buffer_to_string(buf.data(), buf.size());
    }
    return true;
}

bool NXL::document_context::read_section_data(HANDLE h, const NXL::section_record& record, const unsigned char* file_token, std::vector<unsigned char>& data) const
{
    std::vector<unsigned char> buf;
    unsigned long bytes_to_read = 0;

    if (empty()) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }

    if (NULL == h || INVALID_HANDLE_VALUE == h) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    if (record.name[0] == 0 || record.offset == 0 || record.size == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    
    bytes_to_read = record.is_compressed() ? record.compressed_size : record.data_size;
    if (record.is_encrypted()) {
        bytes_to_read = RoundToSize32(bytes_to_read, 16);
    }

    buf.resize(4 + bytes_to_read, 0);
    *((unsigned long*)buf.data()) = (unsigned long)record.data_size;
    unsigned char* raw_data = (0 == bytes_to_read) ? NULL : (buf.data() + 4);
    unsigned long raw_data_size = bytes_to_read;

    if (bytes_to_read != 0) {
        unsigned long bytes_read = 0;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(h, record.offset, NULL, FILE_BEGIN)) {
            return false;
        }
        if (!ReadFile(h, buf.data() + 4, bytes_to_read, &bytes_read, NULL)) {
            return false;
        }
        if (bytes_read != bytes_to_read) {
            return false;
        }
    }

    // calculate checksum
    unsigned char checksum[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (!NX::crypto::hmac_sha256(buf.data(), (unsigned long)buf.size(), file_token, 32, checksum)) {
        return false;
    }
    if (0 != memcmp(checksum, record.checksum, 32)) {
        SetLastError(ERROR_DATA_CHECKSUM_ERROR);
        return false;
    }

    // Empty section, return
    if (raw_data_size == 0) {
        assert(record.data_size == 0);
        return true;
    }

    // Not empty? decrypt or decompress
    if (record.is_encrypted()) {
        NX::crypto::aes_key key;
        key.import_key(file_token, 32);
        if (!NX::crypto::aes_decrypt(key, raw_data, raw_data_size, fixed.keys.iv_seed, 0, NXL_BLOCK_SIZE)) {
            return false;
        }
    }

    // return data
    if (record.is_compressed()) {
        data = NX::ZIP::decompress_buffer(raw_data, record.compressed_size, record.data_size);
    }
    else {
        data.resize(record.data_size, 0);
        memcpy(data.data(), raw_data, record.data_size);
    }
    return (!data.empty());
}


bool NXL::IsValidFile(const std::wstring& file)
{
    bool result = false;
    if (!NXL::IsValidFileEx(file, &result, nullptr)) {
        return false;
    }
    return result;
}

bool NXL::IsValidFile(HANDLE h)
{
    bool result = false;
    if (!NXL::IsValidFileEx(h, &result, nullptr)) {
        return false;
    }
    return result;
}

bool NXL::IsValidFileEx(const std::wstring& file, bool* result, unsigned long* version)
{
    HANDLE h = INVALID_HANDLE_VALUE;

    h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    bool ret = NXL::IsValidFileEx(h, result, version);
    CloseHandle(h); h = INVALID_HANDLE_VALUE;
    return ret;
}

bool NXL::IsValidFileEx(HANDLE h, bool* result, unsigned long* version)
{
    NXL_SIGNATURE_LITE   signature = { 0 };
    unsigned long bytes_read = 0;
    LARGE_INTEGER file_size = { 0 };

    *result = false;
    if (nullptr != version) {
        *version = 0;
    }

    if (INVALID_SET_FILE_POINTER == SetFilePointer(h, 0, NULL, FILE_BEGIN)) {
        return false;
    }
    if (!ReadFile(h, &signature, sizeof(signature), &bytes_read, NULL)) {
        return false;
    }
    if(bytes_read != sizeof(signature)) {
        return false;
    }
    if (!GetFileSizeEx(h, &file_size)) {
        return false;
    }

    if (signature.magic.code == NXL_MAGIC_CODE_1) {
        *result = false;
        if (nullptr != version) {
            *version = 0x00010000;
        }
    }
    else if (signature.magic.code == NXL_MAGIC_CODE_2) {
        *result = (HIWORD(signature.version) >= 2 && file_size.QuadPart > sizeof(NXL_HEADER_2)) ? true : false;
        if (nullptr != version) {
            *version = signature.version;
        }
    }
    else {
        *result = false;
    }

    return true;
}

bool NXL::GetFileInfo(HANDLE h, std::map<std::wstring, std::wstring>& file_info, _In_reads_bytes_opt_(32) const unsigned char* file_token)
{
    bool ret = false;
    std::vector<unsigned char> buf;
    unsigned char ivec_seed[16] = { 0 };


    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), ivec_seed, 16)) {
        return false;
    }

    if (!NXLIMPL::ReadSectionData(h, NXL_SECTION_NAME_FILEINFO, buf, file_token, ivec_seed)) {
        return false;
    }

    if (buf.empty()) {
        return true;
    }

    const std::wstring& s = NX::conversion::utf8_to_utf16(std::string(buf.begin(), buf.end()));
    try {

        NX::json_value info = NX::json_value::parse(s);
        const NX::json_object& object = info.as_object();
        std::for_each(object.begin(), object.end(), [&](const std::pair<std::wstring, json_value>& item) {
            try {
                std::wstring item_name = item.first;
                if (!item_name.empty()) {
                    std::transform(item_name.begin(), item_name.end(), item_name.begin(), tolower);
                    const std::wstring& item_value = item.second.as_string();
                    file_info[item_name] = item_value;
                }
            }
            catch (std::exception& e) {
                UNREFERENCED_PARAMETER(e); // Ignore
            }
        });

        ret = true;
    }
    catch (std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        SetLastError(ERROR_INVALID_DATA);
        ret = false;
    }

    return ret;
}

bool NXL::GetFilePolicy(HANDLE h, std::wstring& file_policy, _In_reads_bytes_opt_(32) const unsigned char* file_token)
{
    std::vector<unsigned char> buf;
    unsigned char ivec_seed[16] = { 0 };

    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), ivec_seed, 16)) {
        return false;
    }

    if (!NXLIMPL::ReadSectionData(h, NXL_SECTION_NAME_FILEPOLICY, buf, file_token, ivec_seed)) {
        return false;
    }

    if (!buf.empty()) {
        file_policy = NX::conversion::utf8_to_utf16(std::string(buf.begin(), buf.end()));
    }

    return true;
}

bool NXL::GetFileTags(HANDLE h, std::multimap<std::wstring, std::wstring>& file_tags, _In_reads_bytes_opt_(32) const unsigned char* file_token)
{
    bool ret = false;
    std::vector<unsigned char> buf;
    unsigned char ivec_seed[16] = { 0 };

    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), ivec_seed, 16)) {
        return false;
    }

    if (!NXLIMPL::ReadSectionData(h, NXL_SECTION_NAME_FILETAG, buf, file_token, ivec_seed)) {
        return false;
    }

    if (buf.empty()) {
        return true;
    }

    const std::wstring& s = NX::conversion::utf8_to_utf16(std::string(buf.begin(), buf.end()));
    try {

        NX::json_value info = NX::json_value::parse(s);
        const NX::json_object& object = info.as_object();
        std::for_each(object.begin(), object.end(), [&](const std::pair<std::wstring, json_value>& item) {
            try {
                std::wstring item_name = item.first;
                if (!item_name.empty()) {
                    std::transform(item_name.begin(), item_name.end(), item_name.begin(), tolower);
                    const std::wstring& item_value = item.second.as_string();
                    file_tags.insert(std::pair<std::wstring, std::wstring>(item_name, item_value));
                }
            }
            catch (std::exception& e) {
                UNREFERENCED_PARAMETER(e); // Ignore
            }
        });

        ret = true;
    }
    catch (std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        SetLastError(ERROR_INVALID_DATA);
        ret = false;
    }

    return ret;
}

bool NXL::SetFileInfo(HANDLE h, const std::map<std::wstring, std::wstring>& file_info, _In_reads_bytes_opt_(32) const unsigned char* file_token)
{
    bool ret = false;
    unsigned char ivec_seed[16] = { 0 };

    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), ivec_seed, 16)) {
        return false;
    }

    try {

        NX::json_value object = NX::json_value::create_object(false);
        std::for_each(file_info.begin(), file_info.end(), [&](const std::pair<std::wstring, std::wstring>& item) {
            std::wstring name = item.first;
            std::transform(name.begin(), name.end(), name.begin(), tolower);
            object[name] = NX::json_value(item.second);
        });

        std::string s = NX::conversion::utf16_to_utf8(object.serialize());
        ret = NXLIMPL::WriteSectionData(h, NXL_SECTION_NAME_FILEINFO, std::vector<unsigned char>(s.begin(), s.end()), file_token, ivec_seed);
        if (ret) {
            ret = UpdateHeaderChecksum(h, file_token);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        ret = false;
    }

    return ret;
}

bool NXL::SetFilePolicy(HANDLE h, const std::wstring& file_policy, _In_reads_bytes_opt_(32) const unsigned char* file_token)
{
    bool ret = false;
    unsigned char ivec_seed[16] = { 0 };

    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), ivec_seed, 16)) {
        return false;
    }

    try {

        std::string s = NX::conversion::utf16_to_utf8(file_policy);
        ret = NXLIMPL::WriteSectionData(h, NXL_SECTION_NAME_FILEPOLICY, std::vector<unsigned char>(s.begin(), s.end()), file_token, ivec_seed);
        if (ret) {
            ret = UpdateHeaderChecksum(h, file_token);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        ret = false;
    }

    return ret;
}

bool NXL::SetFileTag(HANDLE h, const std::unordered_map<std::wstring, std::wstring>& file_info, _In_reads_bytes_opt_(32) const unsigned char* file_token)
{
    bool ret = false;
    unsigned char ivec_seed[16] = { 0 };

    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), ivec_seed, 16)) {
        return false;
    }

    try {

        NX::json_value object = NX::json_value::create_object(false);
        std::for_each(file_info.begin(), file_info.end(), [&](const std::pair<std::wstring, std::wstring>& item) {
            std::wstring name = item.first;
            std::transform(name.begin(), name.end(), name.begin(), tolower);
            object[name] = NX::json_value(item.second);
        });

        std::string s = NX::conversion::utf16_to_utf8(object.serialize());
        ret = NXLIMPL::WriteSectionData(h, NXL_SECTION_NAME_FILETAG, std::vector<unsigned char>(s.begin(), s.end()), file_token, ivec_seed);
        if (ret) {
            ret = UpdateHeaderChecksum(h, file_token);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        ret = false;
    }

    return ret;
}

bool NXL::UpdateHeaderChecksum(HANDLE h, _In_reads_bytes_opt_(32) const unsigned char* file_token)
{
    std::vector<unsigned char> buf;
    const unsigned long header_size = sizeof(NXL_FIXED_HEADER);

    buf.resize(4 + header_size);
    memcpy(buf.data(), &header_size, 4);
    if (!NXLIMPL::NReadFile(h, 0, buf.data() + 4, header_size)) {
        return false;
    }

    unsigned char checksum[32] = { 0 };
    memset(&checksum, 0, sizeof(checksum));
    if (!NX::crypto::hmac_sha256(buf.data(), (ULONG)buf.size(), file_token, 32, checksum)) {
        return false;
    }

    return NXLIMPL::NWriteFile(h, FIELD_OFFSET(NXL_HEADER_2, dynamic.fixed_header_hash), checksum, 32);
}



bool NXLIMPL::NReadFile(const std::wstring& file, unsigned long field_offset, _Out_writes_bytes_(field_size) void* field_data, unsigned long field_size)
{
    HANDLE h = INVALID_HANDLE_VALUE;

    h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    bool ret = NXLIMPL::NReadFile(h, field_offset, field_data, field_size);
    CloseHandle(h); h = INVALID_HANDLE_VALUE;
    return ret;
}

bool NXLIMPL::NReadFile(HANDLE h, unsigned long field_offset, _Out_writes_bytes_(field_size) void* field_data, unsigned long field_size)
{
    unsigned long bytes_read = 0;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(h, field_offset, NULL, FILE_BEGIN)) {
        return false;
    }

    if (!ReadFile(h, field_data, field_size, &bytes_read, NULL)) {
        return false;
    }
    if (field_size != bytes_read) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }

    return true;
}

bool NXLIMPL::NWriteFile(HANDLE h, unsigned long field_offset, _In_reads_bytes_(field_size) const void* field_data, unsigned long field_size)
{
    unsigned long bytes_written = 0;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(h, field_offset, NULL, FILE_BEGIN)) {
        return false;
    }

    if (!WriteFile(h, field_data, field_size, &bytes_written, NULL)) {
        return false;
    }
    if (field_size != bytes_written) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }

    return true;
}

bool NXLIMPL::ReadSectionRecord(HANDLE h, const std::string& section_name, _Out_writes_bytes_(sizeof(NXL_SECTION_2)) NXL_SECTION_2* section_record)
{
    NXL_SECTION_HEADER_2 section_header = { 0 };

    memset(&section_header, 0, sizeof(NXL_SECTION_HEADER_2));
    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.sections), &section_header, (unsigned long)sizeof(NXL_SECTION_HEADER_2))) {
        return false;
    }

    for (int i = 0; i < 32; i++) {

        if (NX::check_bit(section_header.section_map, i) && 0 == _stricmp(section_name.c_str(), section_header.record[i].name)) {
            memcpy(section_record, &section_header.record[i], sizeof(NXL_SECTION_2));
            return true;
        }
    }

    SetLastError(ERROR_NOT_FOUND);
    return false;
}

bool NXLIMPL::WriteSectionRecord(HANDLE h, const std::string& section_name, _In_reads_bytes_(sizeof(NXL_SECTION_2)) const NXL_SECTION_2* section_record)
{
    NXL_SECTION_HEADER_2 section_header = { 0 };

    memset(&section_header, 0, sizeof(NXL_SECTION_HEADER_2));
    if (!NXLIMPL::NReadFile(h, FIELD_OFFSET(NXL_HEADER_2, fixed.sections), &section_header, (unsigned long)sizeof(NXL_SECTION_HEADER_2))) {
        return false;
    }

    for (int i = 0; i < 32; i++) {

        if (NX::check_bit(section_header.section_map, i) && 0 == _stricmp(section_name.c_str(), section_header.record[i].name)) {
            const unsigned long offset = (unsigned long)(FIELD_OFFSET(NXL_HEADER_2, fixed.sections.record) + i * sizeof(NXL_SECTION_2));
            return NWriteFile(h, offset, section_record, (unsigned long)sizeof(NXL_SECTION_2));
        }
    }

    SetLastError(ERROR_NOT_FOUND);
    return false;
}

bool NXLIMPL::ReadSectionData(HANDLE h, const std::string& section_name, std::vector<unsigned char>& section_data, _In_reads_bytes_opt_(32) const unsigned char* file_token, _In_reads_bytes_opt_(16) const unsigned char* ivec_seed)
{
    NXL_SECTION_2 record_info = { 0 };
    std::vector<unsigned char> raw_data;

    memset(&record_info, 0, sizeof(NXL_SECTION_2));
    if (!NXLIMPL::ReadSectionRecord(h, section_name, &record_info)) {
        SetLastError(ERROR_NOT_FOUND);
        return false;
    }

    const bool is_data_compressed = flags32_on(record_info.flags, NXL_SECTION_FLAG_COMPRESSED);
    const bool is_data_encrypted = flags32_on(record_info.flags, NXL_SECTION_FLAG_ENCRYPTED);

    unsigned long raw_data_size = is_data_compressed ? record_info.compressed_size : record_info.data_size;
    if (is_data_encrypted) {
        raw_data_size = RoundToSize32(raw_data_size, 32);
    }

    raw_data.resize(raw_data_size + sizeof(ULONG), 0);
    memcpy(raw_data.data(), &raw_data_size, 4);
    if (raw_data_size != 0) {
        if (!NXLIMPL::NReadFile(h, record_info.offset, raw_data.data() + 4, record_info.data_size)) {
            return false;
        }
    }

    // verify checksum if token present
    if (NULL != file_token) {
        unsigned char data_checksum[32];
        memset(data_checksum, 0, sizeof(data_checksum));
        if (!NX::crypto::hmac_sha256(raw_data.data(), (ULONG)raw_data.size(), file_token, 32, data_checksum)) {
            return false;
        }
        if (0 != memcmp(data_checksum, record_info.checksum, 32)) {
            SetLastError(ERROR_FILE_CORRUPT);
            return false;
        }
    }

    if (is_data_encrypted) {
        assert(IsAligned32(raw_data_size, 32));
        NX::crypto::aes_key kekey;
        if (!kekey.import_key(file_token, 32)) {
            SetLastError(ERROR_INVALID_PASSWORD);
            return false;
        }
        unsigned char ivec_seed[16] = { 0 };
        if (!NXLIMPL::NReadFile(h, record_info.offset, raw_data.data() + 4, record_info.data_size)) {
            return false;
        }
        if (!NX::crypto::aes_decrypt(kekey, raw_data.data(), (ULONG)raw_data.size(), ivec_seed, 0, NXL_BLOCK_SIZE)) {
            return false;
        }
    }

    if (is_data_compressed) {
        assert(record_info.compressed_size != 0);
        assert(record_info.compressed_size <= raw_data_size);
    }
    else {
        
    }

    if (record_info.data_size != 0) {
        section_data.resize(record_info.data_size, 0);
        memcpy(section_data.data(), raw_data.data() + sizeof(ULONG), record_info.data_size);
    }
    return true;
}

bool NXLIMPL::WriteSectionData(HANDLE h, const std::string& section_name, const std::vector<unsigned char>& section_data, _In_reads_bytes_opt_(32) const unsigned char* file_token, _In_reads_bytes_opt_(16) const unsigned char* ivec_seed)
{
    NXL_SECTION_2 record_info = { 0 };
    std::vector<unsigned char> raw_data;

    memset(&record_info, 0, sizeof(NXL_SECTION_2));
    if (!NXLIMPL::ReadSectionRecord(h, section_name, &record_info)) {
        SetLastError(ERROR_NOT_FOUND);
        return false;
    }

    if (section_data.size() > record_info.size) {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return false;
    }

    record_info.data_size = (unsigned short)section_data.size();
    raw_data.resize(record_info.size + sizeof(ULONG), 0);
    memcpy(raw_data.data(), &record_info.size, sizeof(ULONG));
    if (!section_data.empty()) {
        memcpy(raw_data.data() + sizeof(ULONG), section_data.data(), section_data.size());
    }

    // Calculate data checksum
    if (!NX::crypto::hmac_sha256(raw_data.data(), (ULONG)raw_data.size(), file_token, 32, record_info.checksum)) {
        return false;
    }

    // write record
    if (!NXLIMPL::WriteSectionRecord(h, section_name, &record_info)) {
        return false;
    }

    // Write data
    if (!NXLIMPL::NWriteFile(h, record_info.offset, raw_data.data() + sizeof(UCHAR), record_info.size)) {
        return false;
    }

    return true;
}

void offset_check()
{
    // size
    static_assert(256 == sizeof(NXL_SIGNATURE_2), "signature size");
    static_assert(296 == sizeof(NXL_FILEINFO_2), "file header size");
    static_assert(668 == sizeof(NXL_KEYS), "keys header size");
    static_assert(64 == sizeof(NXL_SECTION_2), "section record size");
    static_assert(2052 == sizeof(NXL_SECTION_HEADER_2), "section header size");
    static_assert(0x28 == sizeof(NXL_DYNAMIC_HEADER), "dynamic header size");
    static_assert(4096 == sizeof(NXL_HEADER_2), "total size");

    // offset
    static_assert(0 == FIELD_OFFSET(NXL_HEADER_2, fixed.signature), "signature header offset");
    static_assert(8 == FIELD_OFFSET(NXL_HEADER_2, fixed.signature.version), "signature.version offset");
    static_assert(0xC == FIELD_OFFSET(NXL_HEADER_2, fixed.signature.message), "signature.message offset");

    static_assert(0x100 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info), "file info header offset");
    static_assert(0x100 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.duid), "file_info.duid offset");
    static_assert(0x110 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.flags), "file_info.flags offset");
    static_assert(0x114 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.alignment), "file_info.alignment offset");
    static_assert(0x118 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.algorithm), "file_info.algorithm offset");
    static_assert(0x11C == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.block_size), "file_info.block_size offset");
    static_assert(0x120 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.content_offset), "file_info.content_offset offset");
    static_assert(0x124 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.owner_id), "file_info.owner_id offset");
    static_assert(0x224 == FIELD_OFFSET(NXL_HEADER_2, fixed.file_info.extended_data_offset), "file_info.extended_data_offset offset");

    static_assert(0x228 == FIELD_OFFSET(NXL_HEADER_2, fixed.keys), "keys header offset");
    static_assert(0x228 == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.mode_and_flags), "keys.mode_and_flags offset");
    static_assert(0x22C == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.iv_seed), "keys.iv_seed offset");
    static_assert(0x23C == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.token_cek), "keys.token_cek offset");
    static_assert(0x25C == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.token_cek_checksum), "keys.token_cek_checksum offset");
    static_assert(0x27C == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.recovery_cek), "keys.recovery_cek offset");
    static_assert(0x29C == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.recovery_cek_checksum), "keys.recovery_cek_checksum offset");
    static_assert(0x2BC == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.public_key1), "keys.public_key1 offset");
    static_assert(0x3BC == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.public_key2), "keys.public_key2 offset");
    static_assert(0x4BC == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.token_level), "keys.token_level offset");
    static_assert(0x4C0 == FIELD_OFFSET(NXL_HEADER_2, fixed.keys.extended_data_offset), "keys.extended_data_offset offset");

    static_assert(0x4C4 == FIELD_OFFSET(NXL_HEADER_2, fixed.sections), "sections header offset");
    static_assert(0x4C4 == FIELD_OFFSET(NXL_HEADER_2, fixed.sections.section_map), "sections.section_map offset");
    static_assert(0x4C8 == FIELD_OFFSET(NXL_HEADER_2, fixed.sections.record), "sections.record offset");

    static_assert(0xCC8 == FIELD_OFFSET(NXL_HEADER_2, fixed.extend_data), "extend_data header offset");

    static_assert(0xFD8 == FIELD_OFFSET(NXL_HEADER_2, dynamic), "sections header offset");
}