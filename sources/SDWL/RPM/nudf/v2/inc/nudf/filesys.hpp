

#pragma once
#ifndef __NUDF_FILE_SYSTEM_HPP__
#define __NUDF_FILE_SYSTEM_HPP__

#include <Windows.h>
#include <string>
#include <vector>
#include <map>

#include <nudf\conversion.hpp>

namespace NX {

namespace fs {



class drive
{
public:
    drive();
    drive(wchar_t c);
    ~drive();

    inline bool empty() const { return (0 == _letter); }
    inline wchar_t drive_letter() const { return _letter; }
    inline unsigned int type() const { return _type; }
    inline const std::wstring& type_name() const { return _type_name; }
    inline const std::wstring& dos_name() const { return _dos_name; }
    inline const std::wstring& nt_name() const { return _nt_name; }

    bool is_valid() const;
    bool is_removable() const;
    bool is_fixed() const;
    bool is_remote() const;
    bool is_cdrom() const;
    bool is_ramdisk() const;

    void clear();
    drive& operator = (const drive& other);

    class space
    {
    public:
        space() : _total(0), _free(0), _avaliable_free(0) {}
        space(unsigned __int64 total, unsigned __int64 total_free, unsigned __int64 available_free) : _total(total), _free(total_free), _avaliable_free(available_free) {}
        ~space() {}

        inline unsigned __int64 total_bytes() const { return _total; }
        inline unsigned __int64 total_free_bytes() const { return _free; }
        inline unsigned __int64 available_free_bytes() const { return _avaliable_free; }
        inline space& operator = (const space& other)
        {
            if (this != &other) {
                _total = other.total_bytes();
                _free = other.total_free_bytes();
                _avaliable_free = other.available_free_bytes();
            }
            return *this;
        }

    private:
        unsigned __int64    _total;
        unsigned __int64    _free;
        unsigned __int64    _avaliable_free;
    };

    space get_space() const;

protected:
    void normalize();
    unsigned int get_drive_type(const wchar_t drive);
    std::wstring type_to_name(unsigned int type);

private:
    wchar_t         _letter;
    unsigned int    _type;
    std::wstring    _type_name;
    std::wstring    _dos_name;
    std::wstring    _nt_name;
};

class filename
{
public:
    filename();
    filename(const std::wstring& s);
    filename(const std::string& s, bool utf8 = false);
    virtual ~filename();

    inline const std::wstring& full_name() const { return _s; }
    
    std::wstring name_part() const;
    std::wstring extension() const;
    filename& operator = (const filename& other);

    static std::wstring encode(const std::wstring& name);
    static std::wstring decode(const std::wstring& encoded_name);

private:
    std::wstring    _s;
};


class filepath
{
public:
    filepath() {}
    virtual ~filepath() {}
    
    inline bool empty() const { return _s.empty(); }
    inline const std::wstring& path() const { return _s; }
	inline void clear() { _s = L""; }

    filename file_name() const
    {
        auto pos = _s.find_last_of(L'\\');
        return filename((pos == std::wstring::npos) ? L"" : _s.substr(pos + 1));
    }

    std::wstring file_dir() const
    {
        auto pos = _s.find_last_of(L'\\');
        return (pos == std::wstring::npos) ? L"" : _s.substr(0, pos);
    }

protected:
    filepath(const std::wstring& s) : _s(s) {}
    void set_path(const std::wstring& s) { _s = s; }
    // Convert input path to NT path
    virtual std::wstring normalize(const std::wstring& s) = 0;

private:
    std::wstring    _s; // NT path or Dos Path (DosDrive or UNC)
};

class nt_filepath : public filepath
{
public:
    nt_filepath();
    nt_filepath(HANDLE h);
    nt_filepath(const std::wstring& s);
    virtual ~nt_filepath();
    
    nt_filepath& operator = (const nt_filepath& other);

protected:
    // Convert input path to NT path
    virtual std::wstring normalize(const std::wstring& s);
};

class dos_filepath : public filepath
{
public:
    dos_filepath();
    dos_filepath(HANDLE h);
    dos_filepath(const std::wstring& s);
    virtual ~dos_filepath();
    
    dos_filepath& operator = (const dos_filepath& other);

    static dos_filepath get_current_directory();

protected:
    // Convert input path to NT path
    virtual std::wstring normalize(const std::wstring& s);
};

class dos_fullfilepath : public filepath
{
public:
	dos_fullfilepath();
	dos_fullfilepath(HANDLE h);
	dos_fullfilepath(const std::wstring& s, bool _lowcase = false);
	virtual ~dos_fullfilepath();

	dos_fullfilepath& operator = (const dos_fullfilepath& other);

	static dos_fullfilepath get_current_directory();
	std::wstring global_dos_path();

protected:
	// Used to deal with case issue
	std::wstring case_normalize(const std::wstring& s, bool _lowcase);

	// Convert input path to NT path
	virtual std::wstring normalize(const std::wstring& s);

private:
	bool lowcase;
};


class module_path : public dos_filepath
{
public:
    module_path();
    module_path(HMODULE h);
    virtual ~module_path();
    module_path& operator = (const module_path& other);
};

bool exists(const std::wstring& file);
bool is_readonly(const std::wstring& file);
bool has_file_attributes(unsigned long attributes, unsigned long flags);
bool remove_file_attributes(const std::wstring& file, unsigned long flags);
bool is_nt_path(const std::wstring& s);
bool is_remote_nt_path(const std::wstring& s);
bool is_unc_path(const std::wstring& s);
bool is_dos_drive_only_path(const std::wstring& s);
bool is_dos_path(const std::wstring& s);
bool is_global_path(const std::wstring& s);
bool is_global_dos_path(const std::wstring& s);
bool is_global_unc_path(const std::wstring& s);
bool is_systemroot_path(const std::wstring& s);
std::wstring change_systemroot_path_to_dos_path(const std::wstring& s);
std::wstring handle_to_nt_path(HANDLE h);
std::wstring handle_to_dos_path(HANDLE h);
std::wstring nt_path_to_dos_path(const std::wstring& s);
std::vector<fs::drive> get_logic_drives();

static const std::wstring EOL;


class file_object
{
public:
    file_object();
    virtual ~file_object();


    inline bool opened() const { return (INVALID_HANDLE_VALUE != _h); }
    inline operator HANDLE() const { return _h; }

    virtual void close();

protected:
    void inter_open_to_read(const std::wstring& file);
    void inter_open_to_write(const std::wstring& file, bool write_through = false);
    void inter_create(const std::wstring& file, bool write_through = false);
    __int64 seek_from_begin(__int64 n);
    __int64 seek_from_end(__int64 n);
    __int64 seek_from_current(__int64 n);
    __int64 seek_to_begin();
    __int64 seek_to_end();
    __int64 seek_to_current();
    __int64 size();
    void resize(__int64 new_size);
    std::vector<unsigned char> inter_read(unsigned long bytes_to_read);
    unsigned long inter_read(unsigned char* buf, unsigned long bytes_to_read);
    unsigned long inter_write(const std::vector<unsigned char>& data);

private:
    HANDLE  _h;
};

std::wstring create_unique_directory(const std::wstring& parent_dir, const std::wstring& name_prefix, bool hide = false);

bool open_file_exclusively(_In_ const std::wstring& file, _In_ DWORD wait_milliseconds);
bool IsFileClosed(_In_ const std::wstring& file);
bool delete_file(const std::wstring& file, bool force);
bool remove_directory(const std::wstring& dir, bool force);
bool remove_directory(const std::wstring& dir, bool force, std::vector<std::wstring>& undeleted_files);

class file_checksum
{
public:
    file_checksum() {}
    virtual ~file_checksum() {}

    virtual bool empty() const = 0;
    virtual bool generate(const std::wstring& file) = 0;
    virtual std::wstring serialize() const = 0;

protected:
    bool load_file_content(const std::wstring& file, std::vector<unsigned char>& content);
};

class file_checksum_crc32 : public file_checksum
{
public:
    file_checksum_crc32();
    file_checksum_crc32(const file_checksum_crc32& other);
    virtual ~file_checksum_crc32();
    
    virtual bool empty() const;
    virtual bool generate(const std::wstring& file);
    inline __int64 get_checksum() const { return _checksum; }
    virtual std::wstring serialize() const;

    file_checksum_crc32& operator = (const file_checksum_crc32& other);

private:
    __int64 _checksum;
};

class file_checksum_sha1 : public file_checksum
{
public:
    file_checksum_sha1();
    file_checksum_sha1(const file_checksum_sha1& other);
    virtual ~file_checksum_sha1();

    virtual bool empty() const;
    virtual bool generate(const std::wstring& file);
    inline const std::vector<unsigned char>& get_checksum() const { return _checksum; }
    virtual std::wstring serialize() const;

    file_checksum_sha1& operator = (const file_checksum_sha1& other);

private:
    std::vector<unsigned char> _checksum;
};


}

}


#endif