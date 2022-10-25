

#pragma once
#ifndef __NXRM_PROCESS_HPP__
#define __NXRM_PROCESS_HPP__


#include <string>
#include <map>

#include <nudf\winutil.hpp>
#include <nudf\rwlock.hpp>



typedef enum RIGHT_ID {
    _RIGHT_VIEW = 0,
    _RIGHT_EDIT,
    _RIGHT_PRINT,
    _RIGHT_CLIPBOARD,
    _RIGHT_SAVEAS,
    _RIGHT_DECRYPT,
    _RIGHT_SCREENCAP,
    _RIGHT_SEND,
    _RIGHT_CLASSIFY,
    _RIGHT_SHARE,
    _RIGHT_DOWNLOAD,
    _RIGHT_MAX
} RIGHT_ID;

class process_forbidden_rights
{
public:
    process_forbidden_rights();
    ~process_forbidden_rights();

    void disable_right(RIGHT_ID id);
    void enable_right(RIGHT_ID id, bool force = false);
    void set_rights(unsigned __int64 desired_rights);

    void increase_forbidden_count(RIGHT_ID id);
    void decrease_forbidden_count(RIGHT_ID id);

    bool is_right_forbidden(RIGHT_ID id) const;

    unsigned __int64 get_allowed_rights() const;

    void clear();
    process_forbidden_rights& operator = (const process_forbidden_rights& other);

    inline const unsigned short* get_rights_counter() const { return _rights_counter; }

private:
    unsigned short  _rights_counter[_RIGHT_MAX];
};

typedef enum PROTECT_MODE {
    pm_unknown = -1,
    pm_no = 0,
    pm_yes = 1
} PROTECT_MODE;
class process_protected_window
{
public:

    process_protected_window() : _hwnd(0), _mode(pm_unknown) {}
    process_protected_window(unsigned long hwnd, PROTECT_MODE mode = pm_unknown) : _hwnd(hwnd), _mode(mode)
    {
    }
    ~process_protected_window() {}

    inline PROTECT_MODE get_mode() const { return _mode; }
    inline unsigned long get_hwnd() const { return _hwnd; }

    inline void change_mode(PROTECT_MODE mode) { _mode = mode; }

    inline bool empty() const { return (0 == _hwnd); }
    inline void clear() { _mode = pm_unknown; _hwnd = 0; }
    inline process_protected_window& operator = (const process_protected_window& other)
    {
        if (this != &other) {
            _mode = other.get_mode();
            _hwnd = other.get_hwnd();
        }
        return *this;
    }

private:
    PROTECT_MODE    _mode;
    unsigned long   _hwnd;
};

class process_record
{
public:
    process_record();
    process_record(unsigned long process_id, unsigned __int64 flags = 0);
    process_record(unsigned long process_id, unsigned long session_id, const std::wstring& image_path, unsigned __int64 flags = 0);
    virtual ~process_record();

    inline unsigned long get_process_id() const { return _process_id; }
    inline unsigned long get_session_id() const { return _session_id; }
    inline unsigned __int64 get_flags() const { return _flags; }
    inline const std::wstring& get_image_path() const { return _image_path; }
    inline const std::shared_ptr<NX::win::pe_file>& get_pe_file_info() const { return _pe_file_info; }

    inline void set_flags(unsigned __int64 f) { _flags |= f; }
    inline void clear_flags(unsigned __int64 f) { _flags &= (~f); }
    inline void reset_flags(unsigned __int64 f) { _flags = f; }

    inline const process_forbidden_rights& get_forbidden_rights() const { return _forbidden_rights; }
    inline process_forbidden_rights& get_forbidden_rights() { return _forbidden_rights; }

    inline const std::map<unsigned long, process_protected_window>& get_protected_windows() const { return _protected_windows; }
    inline std::map<unsigned long, process_protected_window>& get_protected_windows() { return _protected_windows; }

    inline bool empty() const { return (0 == _process_id); }

    process_record& operator = (const process_record& other);
    void clear();

	void insert_file_rights(const std::wstring& filePath, const std::wstring& strJsonRightsWatermark);
	void remove_file_rights(const std::wstring& filePath);
	std::map<std::wstring, std::wstring> get_file_rights();
    
public:
    static std::wstring get_image_path_from_pid(unsigned long process_id);
    static unsigned long get_session_id_from_pid(unsigned long process_id);
    static std::wstring normalize_image_path(const std::wstring& image_path);

private:
    // basic properties
    unsigned long       _process_id;
    unsigned long       _session_id;
    unsigned __int64    _flags;
    std::wstring        _image_path;
    std::shared_ptr<NX::win::pe_file>   _pe_file_info;

    // rights management properties
    process_forbidden_rights    _forbidden_rights;
    std::map<unsigned long, process_protected_window>   _protected_windows;

	std::map<std::wstring, std::wstring>		_map_file_rights_watermark;
};

class process_cache
{
public:
    process_cache();
    virtual ~process_cache();
    
    bool empty() const;
    void clear();

    process_record find(unsigned long process_id);
    void insert(const process_record& record);
    void remove(unsigned long process_id);

    void reset_process_flags(unsigned long process_id, unsigned __int64 f);
    void set_process_flags(unsigned long process_id, unsigned __int64 f);
    void clear_process_flags(unsigned long process_id, unsigned __int64 f);

    void forbid_process_right(unsigned long process_id, RIGHT_ID id);
    void enable_process_right(unsigned long process_id, RIGHT_ID id, bool force);
    void set_process_rights(unsigned long process_id, unsigned __int64 desired_rights);

    void insert_protected_window(unsigned long process_id, unsigned long hwnd, PROTECT_MODE mode = pm_unknown);
    void change_protected_window_mode(unsigned long process_id, unsigned long hwnd, PROTECT_MODE mode);
    void remove_protected_window(unsigned long process_id, unsigned long hwnd);
    void clear_protected_windows(unsigned long process_id);
    std::vector<process_protected_window> get_protected_windows(unsigned long process_id);

    bool does_protected_process_exist(unsigned long session_id);
    std::vector<std::pair<unsigned long, std::wstring>> find_all_protected_process(unsigned long session_id);

	void insert_process_file_rights(unsigned long process_id, const std::wstring& file_path, const std::wstring& strJsonFileRightsWatermark);
	void remove_process_file_rights(unsigned long process_id, const std::wstring& file_path);
	bool get_process_file_rights(unsigned long process_id, std::map<std::wstring, std::wstring>& mapFileRights);

private:
    std::map<unsigned long, process_record>  _map;
    mutable NX::utility::CRwLock    _lock;
};



#endif