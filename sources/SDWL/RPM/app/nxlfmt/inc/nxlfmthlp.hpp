

#ifndef __NXL_FORMAT_HELP_HPP__
#define __NXL_FORMAT_HELP_HPP__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include "nxlfmt.h"

namespace NX {
namespace NXL {


__forceinline
bool is_section_encrypted(const NXL_SECTION_2* p)
{
    return (NXL_SECTION_FLAG_ENCRYPTED == (p->flags & NXL_SECTION_FLAG_ENCRYPTED));
}

__forceinline
bool is_section_compressed(const NXL_SECTION_2* p)
{
    return (NXL_SECTION_FLAG_COMPRESSED == (p->flags & NXL_SECTION_FLAG_COMPRESSED));
}

class section_record : public NXL_SECTION_2
{
public:
    section_record();
    section_record(const section_record& other);
    ~section_record();

    inline bool empty() const { return (0 == this->name[0] || 0 == this->size || 0 == this->offset); }
    inline bool is_encrypted() const { return (NXL_SECTION_FLAG_ENCRYPTED == (this->flags & NXL_SECTION_FLAG_ENCRYPTED)); }
    inline bool is_compressed() const { return (NXL_SECTION_FLAG_COMPRESSED == (this->flags & NXL_SECTION_FLAG_COMPRESSED)); }

    void clear();

    section_record& operator = (const section_record& other);
};

class document_context : public NXL_HEADER_2
{
public:
    document_context();
    document_context(const std::wstring& file);
    document_context(HANDLE h);
    document_context(const document_context& other);
    document_context(document_context&& other);
    ~document_context();

    document_context& operator = (const document_context& other);
    document_context& operator = (document_context&& other);
    bool empty() const;
    void clear();
    bool load(const std::wstring& file);
    bool load(HANDLE h);
    bool validate_checksum(const unsigned char* file_token) const;
    
    // Signature
    std::wstring get_message() const;

    // file info
    std::vector<unsigned char> get_duid() const;
    inline unsigned long get_file_flags() const { return fixed.file_info.flags; }
    inline unsigned long get_file_alignment() const { return fixed.file_info.alignment; }
    inline unsigned long get_file_algorithm() const { return fixed.file_info.algorithm; }
    inline unsigned long get_file_blocksize() const { return fixed.file_info.block_size; }
    inline unsigned long get_file_content_offset() const { return fixed.file_info.content_offset; }
    std::wstring get_owner_id() const;
	std::string get_tenant() const;

    // keys header
    inline unsigned long get_security_mode() const { return (fixed.keys.mode_and_flags >> 24); }
    inline unsigned long get_key_flags() const { return (fixed.keys.mode_and_flags & 0x00FFFFFF); }
    inline const unsigned char* get_iv_seed() const { return fixed.keys.iv_seed; }
    std::vector<unsigned char> get_agreement0() const;
    std::vector<unsigned char> get_agreement1() const;
    inline unsigned long get_token_level() const { return fixed.keys.token_level; }
	std::string get_tags() const { return m_tags;  }
	std::string get_policy() const { return m_policy; }
	std::string get_info() const { return m_info; }

    // sections
    section_record find_section(const std::string& name) const;
    bool find_default_section(section_record& fileinfo, section_record& filepolicy, section_record& filetag) const;
    bool read_section_data(HANDLE h, const section_record& record, const unsigned char* file_token, std::vector<unsigned char>& data) const;
    bool read_section_string(HANDLE h, const section_record& record, const unsigned char* file_token, std::string& data) const;
	bool read_tag_section(HANDLE h);
	bool read_policy_section(HANDLE h);
	bool read_info_section(HANDLE h);

    inline bool has_recovery_key() const { return (KF_RECOVERY_KEY_ENABLED == (KF_RECOVERY_KEY_ENABLED & fixed.keys.mode_and_flags)); }

private:
	std::string m_tags;
	std::string m_policy;
	std::string m_info;

};



bool IsValidFile(const std::wstring& file);
bool IsValidFile(HANDLE h);
bool IsValidFileEx(const std::wstring& file, bool* result, unsigned long* version);
bool IsValidFileEx(HANDLE h, bool* result, unsigned long* version);
bool GetFileInfo(HANDLE h, std::map<std::wstring, std::wstring>& file_info, _In_reads_bytes_opt_(32) const unsigned char* file_token);
bool GetFilePolicy(HANDLE h, std::wstring& file_policy, _In_reads_bytes_opt_(32) const unsigned char* file_token);
bool GetFileTags(HANDLE h, std::multimap<std::wstring, std::wstring>& file_tags, _In_reads_bytes_opt_(32) const unsigned char* file_token);
bool SetFileInfo(HANDLE h, const std::map<std::wstring, std::wstring>& file_info, _In_reads_bytes_opt_(32) const unsigned char* file_token);
bool SetFilePolicy(HANDLE h, const std::wstring& file_policy, _In_reads_bytes_opt_(32) const unsigned char* file_token);
bool SetFileTag(HANDLE h, const std::unordered_map<std::wstring, std::wstring>& file_info, _In_reads_bytes_opt_(32) const unsigned char* file_token);
bool UpdateHeaderChecksum(HANDLE h, _In_reads_bytes_opt_(32) const unsigned char* file_token);

namespace RAW {

    HRESULT __stdcall EncryptFile(const WCHAR *FileName);
    HRESULT __stdcall EncryptFileEx(const WCHAR *FileName, const char *tag, UCHAR *data, USHORT datalen);
    HRESULT __stdcall EncryptFileEx2(HANDLE hFile, const char *tag, UCHAR *data, USHORT datalen);
    HRESULT __stdcall DecryptFile(const WCHAR *FileName);
    HRESULT __stdcall DecryptFile2(HANDLE hFile);
    HRESULT __stdcall CheckRights(const WCHAR *FileName, ULONGLONG *RightMask, ULONGLONG *CustomRightsMask, ULONGLONG *EvaluationId);
    HRESULT __stdcall CheckRightsNoneCache(const WCHAR *FileName, ULONGLONG *RightMask, ULONGLONG *CustomRightsMask, ULONGLONG *EvaluationId);
    HRESULT __stdcall SyncNXLHeader(HANDLE hFile, const char *tag, UCHAR *data, USHORT datalen);
    HRESULT __stdcall ReadTags(const WCHAR *FileName, UCHAR *data, USHORT *datalen);
    HRESULT __stdcall ReadTagsEx(HANDLE hFile, UCHAR *data, USHORT *datalen);
    HRESULT __stdcall IsDecryptedFile(const WCHAR *FileName);
    //
    // use NxrmIsDecryptedFile2 to bypass Windows container which hooks most of APIs
    // Windows container redirect all file I/O to a virtual path
    //
    HRESULT __stdcall IsDecryptedFile2(const WCHAR *FileName);
    HRESULT __stdcall SetSourceFileName(const WCHAR *FileName, const WCHAR *SrcNTFileName);
}   // namespace NX::NXL::RAW

}   // namespace NX::NXL
}   // namespace NX


#endif