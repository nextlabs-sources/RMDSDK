

#ifndef __NXSERV_CONFIG_HPP__
#define __NXSERV_CONFIG_HPP__

#include <nudf/rwlock.hpp>
#include <map>
#include <set>
#include <string>

// Forward class
//class rmsession;

class client_config
{
public:
    client_config();
    client_config(const std::wstring& serial);
    client_config(const client_config& other);

    inline bool empty() const { return _serial_number.empty(); }
    inline const std::wstring& get_serial_number() const { return _serial_number; }
    inline unsigned long get_heartbeat_interval() const { return _heartbeat_interval; }
    inline unsigned long get_log_interval() const { return _log_interval; }
    inline const std::wstring& get_bypass_filter() const { return _bypass_filter; }
    inline bool is_protection_enabled() const { return _protection_enabled; }
    inline const std::wstring& get_protection_filter() const { return _protection_filter; }
	//const std::wstring& get_mark_dir() const { return _markDir; }
	void set_heartbeat_interval(unsigned long heartbeat_interval) { _heartbeat_interval = heartbeat_interval; }

    void clear();
    client_config& operator = (const client_config& other);
    bool operator == (const std::wstring& serial_number) const;
    std::wstring serialize() const;
    void deserialize(const std::wstring& s);

private:
    std::wstring    _serial_number;
    unsigned long   _heartbeat_interval;
    unsigned long   _log_interval;
    std::wstring    _bypass_filter;
    bool            _protection_enabled;
    std::wstring    _protection_filter;
	std::wstring    _clientId;
	//std::wstring    _markDir;

    friend class rmsession;
};

class client_dir_config
{
public:
	client_dir_config();
	client_dir_config(const client_dir_config& other);

	const std::wstring& get_sdklib_dir() const { return _sdklibDir; }
	void set_sdklib_dir(const std::wstring& sdklibDir) { _sdklibDir = sdklibDir; }
	//const std::wstring& get_mark_dir() const { return _markDir; }
	//void set_mark_dir(const std::wstring& markDir) { _markDir = markDir; }
	const std::wstring& get_router() const { return _router; }
	void set_router(const std::wstring& router) { _router = router; }
	const std::wstring& get_tenant_id() const { return _tenant; }
	void set_tenant_id(const std::wstring& tenant) { _tenant = tenant; }
	const std::wstring& get_workingfolder() const { return _workingfolder; }
	const std::wstring& get_tempfolder() const { return _tempfolder; }

	void clear();
	client_dir_config& operator = (const client_dir_config& other);
	std::wstring serialize() const;
	void deserialize(const std::wstring& s);

private:

	//std::wstring    _clientId;
	std::wstring    _sdklibDir;
	// std::wstring    _markDir;
	std::wstring    _router;
	std::wstring    _tenant;
	std::wstring    _workingfolder;
	std::wstring    _tempfolder;

	friend class rmsession;
};

class client_local_config
{
public:
    client_local_config();
    client_local_config(const client_local_config& other);

    inline __int64 get_last_update_time() const { return _last_update_time; }
    inline __int64 get_last_heartbeat_time() const { return _last_heartbeat_time; }
    inline unsigned long get_check_update_interval() const { return _check_update_interval; }

    void clear();
    client_local_config& operator = (const client_local_config& other);
    std::wstring serialize() const;
    void deserialize(const std::wstring& s);

private:
    __int64 _last_update_time;
    __int64 _last_heartbeat_time;
    unsigned long _check_update_interval;

    friend class rmsession;
};

class classify_config
{
public:
    classify_config();
    classify_config(const std::wstring& serial, const std::wstring& content);
    classify_config(const classify_config& other);

    inline bool empty() const { return _serial_number.empty(); }
    inline const std::wstring& get_serial_number() const { return _serial_number; }
    inline const std::wstring& get_content() const { return _content; }

    void clear();
    classify_config& operator = (const classify_config& other);
    bool operator == (const std::wstring& serial_number) const;
    std::wstring serialize() const;
    void deserialize(const std::wstring& s);

private:
    std::wstring    _serial_number;
    std::wstring    _content;

    friend class rmsession;
};

#define NOWISE          0
#define _CLOCKWISE       1
#define _ANTICLOCKWISE   2

class watermark_config
{
public:
    watermark_config();
    watermark_config(const watermark_config& other);
    virtual ~watermark_config();

    inline bool empty() const { return _serial_number.empty(); }
    inline const std::wstring& get_serial_number() const { return _serial_number; }
    inline const std::wstring& get_message() const { return _message; }
    inline int get_transparency_ratio() const { return _transparency_ratio; }
    inline const std::wstring& get_font_name() const { return _message; }
    inline int get_font_size() const { return _font_size; }
    inline COLORREF get_font_color() const { return _font_color; }
    inline int get_rotation() const { return _rotation; }

    void clear();
    watermark_config& operator = (const watermark_config& other);
    bool operator == (const std::wstring& serial_number) const;
    std::wstring serialize() const;
    void deserialize(const std::wstring& s);

private:
    std::wstring    _serial_number;
    std::wstring    _message;
    int             _transparency_ratio;
    std::wstring    _font_name;
    int             _font_size;
    COLORREF        _font_color;
    int             _rotation;

    friend class rmsession;
};

class app_whitelist_config
{
public:
    app_whitelist_config();
    app_whitelist_config(const app_whitelist_config& other);

    const std::set<std::wstring>& get_registered_apps() const { return _registeredApps; }
    const std::map<std::wstring, std::vector<unsigned char>>& get_trusted_apps() const { return _trustedApps; }

    void clear();
    app_whitelist_config& operator = (const app_whitelist_config& other);
    std::wstring serialize() const;
    void deserialize(const std::wstring& s);

private:
    std::set<std::wstring>   _registeredApps;
    std::map<std::wstring, std::vector<unsigned char>>  _trustedApps;

    friend class rmsession;
};

class core_context_cache
{
public:
    core_context_cache();
    ~core_context_cache();

    std::vector<unsigned __int64> find(const std::wstring& module_path, unsigned __int64 module_checksum);
    void insert(const std::wstring& module_path, unsigned __int64 module_checksum, const std::vector<unsigned __int64>& context);

    bool load(const std::string& s);
    bool serialize(std::string& s);

private:
    std::map<std::wstring, std::map<unsigned __int64, std::vector<unsigned __int64>>> _map;
    NX::utility::CRwLock _lock;
};



#endif