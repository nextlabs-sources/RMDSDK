

#include <windows.h>

#include <string>

#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\string.hpp>
#include <nudf\ntapi.hpp>
#include <nudf\json.hpp>
#include <nudf\conversion.hpp>
#include <nudf\winutil.hpp>

#include "serv.hpp"
#include "config.hpp"
#include "nxrmserv.hpp"


extern rmserv* SERV;

client_config::client_config()
{
	_heartbeat_interval = 120;
	_log_interval = 300; // every 5 minutes to upload log/metadata
}

client_config::client_config(const std::wstring& serial) : _serial_number(serial)
{
	_heartbeat_interval = 120;
	_log_interval = 300; // every 5 minutes to upload log/metadata
}

client_config::client_config(const client_config& other) : _serial_number(other._serial_number)
{
}

void client_config::clear()
{
    _serial_number.clear();
    _heartbeat_interval = 120;
    _log_interval = 300; // every 5 minutes to upload log/metadata
    _bypass_filter.clear();
    _protection_enabled = false;
    _protection_filter.clear();
	_clientId.clear();
}

client_config& client_config::operator = (const client_config& other)
{
    if (this != &other) {
        _serial_number = other._serial_number;
        _heartbeat_interval = other._heartbeat_interval;
        _log_interval = other._log_interval;
        _bypass_filter = other._bypass_filter;
        _protection_enabled = other._protection_enabled;
        _protection_filter = other._protection_filter;
		_clientId = other._clientId;
		//_markDir = other._markDir;
    }
    return *this;
}

bool client_config::operator == (const std::wstring& serial_number) const
{
    return (0 == _wcsicmp(_serial_number.c_str(), serial_number.c_str()));
}

std::wstring client_config::serialize() const
{
    std::wstring    s;

    try {

        NX::json_value v = NX::json_value::create_object();
        v[L"serialNumber"] = NX::json_value(_serial_number);
        v[L"heartbeatInterval"] = NX::json_value((unsigned int)_heartbeat_interval);
        v[L"logInterval"] = NX::json_value((unsigned int)_log_interval);
        v[L"bypassFilter"] = NX::json_value(_bypass_filter);
        v[L"protectionEnabled"] = NX::json_value(_protection_enabled);
        v[L"protectionFilter"] = NX::json_value(_protection_filter);
		v[L"clientId"] = NX::json_value(_clientId);
        s = v.serialize();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    return std::move(s);
}

void client_config::deserialize(const std::wstring& s)
{
    try {

        NX::json_value v = NX::json_value::parse(s);
        _serial_number = v.as_object().at(L"serialNumber").as_string();
        _heartbeat_interval = v.as_object().at(L"heartbeatInterval").as_int32();
        _log_interval = v.as_object().at(L"logInterval").as_int32();
        _bypass_filter = v.as_object().at(L"bypassFilter").as_string();
        _protection_enabled = v.as_object().at(L"protectionEnabled").as_boolean();
        if (_protection_enabled) {
            _protection_filter = v.as_object().at(L"protectionFilter").as_string();
        }
		else {
			_protection_filter = L"[^.]";
		}
		
		try {
			_clientId = v.as_object().at(L"clientId").as_string();
		}
		catch (std::exception& e) {
			UNREFERENCED_PARAMETER(e);
		}
		
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
    }
}

// ***
client_dir_config::client_dir_config() 
{
}

client_dir_config::client_dir_config(const client_dir_config& other)
{
}

void client_dir_config::clear()
{
	_sdklibDir.clear();
	//_markDir.clear();
	_router.clear();
	_tenant.clear();
	_workingfolder.clear();
	_tempfolder.clear();
}

client_dir_config& client_dir_config::operator = (const client_dir_config& other)
{
	if (this != &other) 
	{
		_sdklibDir = other._sdklibDir;
		//_markDir = other._markDir;
		_router = other._router;
		_tenant = other._tenant;
		_workingfolder = other._workingfolder;
		_tempfolder = other._tempfolder;
    }
	return *this;
}


std::wstring client_dir_config::serialize() const
{
	std::wstring    s;

	try {
		NX::json_value v = NX::json_value::create_object();
		v[L"sdklibDir"] = NX::json_value(_sdklibDir);
		//v[L"markDir"] = NX::json_value(_markDir);
		v[L"router"] = NX::json_value(_router);
		v[L"tenant"] = NX::json_value(_tenant);
		v[L"workingfolder"] = NX::json_value(_workingfolder);
		v[L"tempfolder"] = NX::json_value(_tempfolder);
        s = v.serialize();
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
	}

	return std::move(s);
}

void client_dir_config::deserialize(const std::wstring& s)
{
	try {
		NX::json_value v = NX::json_value::parse(s);
		
			_sdklibDir = v.as_object().at(L"sdklibDir").as_string();
			//_markDir = v.as_object().at(L"markDir").as_string();
			_router = v.as_object().at(L"router").as_string();
			_tenant = v.as_object().at(L"tenant").as_string();
			_workingfolder = v.as_object().at(L"workingfolder").as_string();
			_tempfolder = v.as_object().at(L"tempfolder").as_string();
    }
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		clear();
	}
}

client_local_config::client_local_config() : _last_update_time(0), _last_heartbeat_time(0), _check_update_interval(86400)
{
}

client_local_config::client_local_config(const client_local_config& other) : _last_update_time(other._last_update_time), _last_heartbeat_time(other._last_heartbeat_time), _check_update_interval(other._check_update_interval)
{
}

void client_local_config::clear()
{
    _last_update_time = 0;
    _last_heartbeat_time = 0;
    _check_update_interval = 0;
}

client_local_config& client_local_config::operator = (const client_local_config& other)
{
    if (this != &other) {
        _last_update_time = other._last_update_time;
        _last_heartbeat_time = other._last_heartbeat_time;
        _check_update_interval = other._check_update_interval;
    }
    return *this;
}

std::wstring client_local_config::serialize() const
{
    std::wstring s;
    try {
        NX::json_value v = NX::json_value::create_object();
        v[L"lastUpdateAt"] = NX::json_value(_last_update_time);
        v[L"lastHeartbeatAt"] = NX::json_value(_last_heartbeat_time);
        v[L"updateInterval"] = NX::json_value((UINT)_check_update_interval);
		//v[L"clientId"] = NX::json_value(SERV->get_client_id());
        s = v.serialize();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
    return std::move(s);
}

void client_local_config::deserialize(const std::wstring& s)
{
    try {

        clear();

        NX::json_value v = NX::json_value::parse(s);

        if (v.as_object().has_field(L"lastUpdateAt")) {
            _last_update_time = v.as_object().at(L"lastUpdateAt").as_number().to_int64();
        }
        if (v.as_object().has_field(L"lastHeartbeatAt")) {
            _last_heartbeat_time = v.as_object().at(L"lastHeartbeatAt").as_number().to_int64();
        }
        if (v.as_object().has_field(L"updateInterval")) {
            _check_update_interval = v.as_object().at(L"updateInterval").as_number().to_uint32();
        }
		try {
			if (v.as_object().has_field(L"clientId")) {
				std::wstring clientId = v.as_object().at(L"clientId").as_string();
				//SERV->set_client_id(clientId);
			}
		}
		catch (std::exception& e) {
			UNREFERENCED_PARAMETER(e);
		}
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
    }
}




classify_config::classify_config()
{
}

classify_config::classify_config(const std::wstring& serial, const std::wstring& content) : _serial_number(serial), _content(content)
{
}

classify_config::classify_config(const classify_config& other) : _serial_number(other._serial_number), _content(other._content)
{
}

void classify_config::clear()
{
    _serial_number.clear();
    _content.clear();
}

classify_config& classify_config::operator = (const classify_config& other)
{
    if (this != &other) {
        _serial_number = other._serial_number;
        _content = other._content;
    }
    return *this;
}

bool classify_config::operator == (const std::wstring& serial_number) const
{
    return (0 == _wcsicmp(_serial_number.c_str(), serial_number.c_str()));
}

std::wstring classify_config::serialize() const
{
    std::wstring    s;

    try {

        NX::json_value v = NX::json_value::create_object();
        v[L"serialNumber"] = NX::json_value(_serial_number);
        v[L"content"] = NX::json_value(_content);
        s = v.serialize();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    return std::move(s);
}

void classify_config::deserialize(const std::wstring& s)
{
    try {

        NX::json_value v = NX::json_value::parse(s);
        _serial_number = v.as_object().at(L"serialNumber").as_string();
        _content = v.as_object().at(L"content").as_string();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
    }
}



watermark_config::watermark_config() : _transparency_ratio(0), _font_size(0), _font_color(RGB(0, 0, 0)), _rotation(0)
{
}

watermark_config::watermark_config(const watermark_config& other)
    : _serial_number(other._serial_number), _message(other._message), _transparency_ratio(other._transparency_ratio), _font_name(other._font_name),
    _font_size(other._font_size), _font_color(other._font_color), _rotation(other._rotation)
{
}

watermark_config::~watermark_config()
{
}

void watermark_config::clear()
{
    _serial_number.clear();
    _message.clear();
    _transparency_ratio = 0;
    _font_name.clear();
    _font_size = 0;
    _font_color = RGB(0, 0, 0);
    _rotation = 0;
}

watermark_config& watermark_config::operator = (const watermark_config& other)
{
    if (this != &other) {
        _serial_number = other._serial_number;
        _message = other._message;
        _transparency_ratio = other._transparency_ratio;
        _font_name = other._font_name;
        _font_size = other._font_size;
        _font_color = other._font_color;
        _rotation = other._rotation;
    }
    return *this;
}

bool watermark_config::operator == (const std::wstring& serial_number) const
{
    return (0 == _wcsicmp(_serial_number.c_str(), serial_number.c_str()));
}

std::wstring watermark_config::serialize() const
{
    std::wstring    s;

    try {

        NX::json_value v = NX::json_value::create_object();
        v[L"serialNumber"] = NX::json_value(_serial_number);
        v[L"message"] = NX::json_value(_message);
        v[L"transparencyRatio"] = NX::json_value(_transparency_ratio);
        v[L"fontName"] = NX::json_value(_font_name);
        v[L"fontSize"] = NX::json_value(_font_size);
        v[L"fontColor"] = NX::json_value((unsigned int)_font_color);
        v[L"rotation"] = NX::json_value(_rotation);
        s = v.serialize();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    return std::move(s);
}

void watermark_config::deserialize(const std::wstring& s)
{
    try {

        NX::json_value v = NX::json_value::parse(s);
        _serial_number = v.as_object().at(L"serialNumber").as_string();
        _message = v.as_object().at(L"message").as_string();
        _transparency_ratio = v.as_object().at(L"transparencyRatio").as_int32();
        _font_name = v.as_object().at(L"fontName").as_string();
        _font_size = v.as_object().at(L"fontSize").as_int32();
        _font_color = v.as_object().at(L"fontColor").as_uint32();
        _rotation = v.as_object().at(L"rotation").as_int32();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
    }
}



app_whitelist_config::app_whitelist_config()
{
}

app_whitelist_config::app_whitelist_config(const app_whitelist_config& other)
{
}

void app_whitelist_config::clear()
{
    _registeredApps.clear();
    _trustedApps.clear();
}

app_whitelist_config& app_whitelist_config::operator = (const app_whitelist_config& other)
{
    if (this != &other)
    {
        _registeredApps = other._registeredApps;
        _trustedApps = other._trustedApps;
    }
    return *this;
}

std::wstring app_whitelist_config::serialize() const
{
    std::wstring s;

    try {
        NX::json_value v = NX::json_value::create_object();

        v[L"registeredApps"] = NX::json_value::create_array();
        NX::json_array& arr = v[L"registeredApps"].as_array();
        for (const auto& app : _registeredApps) {
            arr.push_back(NX::json_value(app));
        }

        v[L"trustedApps"] = NX::json_value::create_array();
        NX::json_array& arr2 = v[L"trustedApps"].as_array();
        for (const auto& app : _trustedApps) {
            NX::json_value w = NX::json_value::create_object();
            w[L"path"] = NX::json_value(app.first);
            w[L"hash"] = NX::json_value(NX::conversion::to_base64(app.second));
            arr2.push_back(w);
        }

        s = v.serialize();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    return std::move(s);
}

void app_whitelist_config::deserialize(const std::wstring& s)
{
    try {
        NX::json_value v = NX::json_value::parse(s);

        _registeredApps.clear();
        const NX::json_array& arr = v[L"registeredApps"].as_array();
        for (const auto& app : arr) {
            _registeredApps.insert(app.as_string());
        }

        _trustedApps.clear();
        const NX::json_array& arr2 = v[L"trustedApps"].as_array();
        for (const auto& app : arr2) {
            std::wstring path = app.as_object().at(L"path").as_string();
            std::vector<unsigned char> hash = NX::conversion::from_base64(app.as_object().at(L"hash").as_string());
            _trustedApps[path] = hash;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
    }
}



core_context_cache::core_context_cache()
{
}

core_context_cache::~core_context_cache()
{
}

std::vector<unsigned __int64> core_context_cache::find(const std::wstring& module_path, unsigned __int64 module_checksum)
{
    std::vector<unsigned __int64> context;
    NX::utility::CRwSharedLocker locker(&_lock);
    auto pos = _map.find(NX::conversion::lower_str<wchar_t>(module_path));
    if (pos != _map.end()) {
        auto context_pos = (*pos).second.find(module_checksum);
        if (context_pos != (*pos).second.end()) {
            context = (*context_pos).second;
        }
    }
    return std::move(context);
}

void core_context_cache::insert(const std::wstring& module_path, unsigned __int64 module_checksum, const std::vector<unsigned __int64>& context)
{
    NX::utility::CRwSharedLocker locker(&_lock);
    const std::wstring& key = NX::conversion::lower_str<wchar_t>(module_path);
    _map[key][module_checksum] = context;
}

bool core_context_cache::load(const std::string& s)
{
    bool result = false;
    NX::utility::CRwExclusiveLocker locker(&_lock);

    if (s.empty()) {
        return true;
    }

    try {

        NX::json_value v = NX::json_value::parse(s);
        std::for_each(v.as_object().begin(), v.as_object().end(), [&](const std::pair<std::wstring, NX::json_value>& item) {
            std::map<unsigned __int64, std::vector<unsigned __int64>>& modmap = _map[item.first];
            std::for_each(item.second.as_object().begin(), item.second.as_object().end(), [&](const std::pair<std::wstring, NX::json_value>& moditem) {
                if (moditem.first.length() != 16) {
                    return;
                }
                unsigned __int64 checksum = std::wcstoull(moditem.first.c_str(), NULL, 16);
                std::vector<unsigned __int64> modcontext;
                std::for_each(moditem.second.as_array().begin(), moditem.second.as_array().end(), [&](const NX::json_value& context_field) {
                    modcontext.push_back(context_field.as_uint64());
                });
                modmap[checksum] = modcontext;
            });
        });

        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool core_context_cache::serialize(std::string& s)
{
    bool result = false;
    NX::utility::CRwSharedLocker locker(&_lock);

    if (_map.empty()) {
        s.clear();
        return true;
    }
 
    try {

        NX::json_value v = NX::json_value::create_object();
        std::for_each(_map.begin(), _map.end(), [&](const std::pair<std::wstring, std::map<unsigned __int64, std::vector<unsigned __int64>>>& item) {
            v[item.first] = NX::json_value::create_object();
            NX::json_value& mod = v[item.first];
            std::for_each(item.second.begin(), item.second.end(), [&](const std::pair<unsigned __int64, std::vector<unsigned __int64>>& moditem) {
                const std::wstring& strchecksum = NX::string_formater(L"%08X%08X", (ULONG)(moditem.first >> 32), (ULONG)(moditem.first));
                mod[strchecksum] = NX::json_value::create_array();
                NX::json_array& vmod = mod[strchecksum].as_array();
                for (auto it = moditem.second.begin(); it != moditem.second.end(); ++it) {
                    vmod.push_back(NX::json_value(*it));
                }
            });
        });

        const std::wstring& ws = v.serialize();
        s = NX::conversion::utf16_to_utf8(ws);
        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        s.clear();
        result = false;
    }

    return result;
}