

#include <Windows.h>

#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\string.hpp>
#include <nudf\dbglog.hpp>
#include <nudf\ntapi.hpp>

#include "global.hpp"
#include "nxrmvhddef.h"
#include "vhd_manager.hpp"



vhd_info::vhd_info() : _volume_id(-1)
{
}

vhd_info::vhd_info(const vhd_info& other) : _volume_id(other._volume_id), _volume_name(other._volume_name)
{
}

vhd_info::vhd_info(vhd_info&& other) : _volume_id(other._volume_id), _volume_name(std::move(other._volume_name))
{
}

vhd_info::~vhd_info()
{
}

vhd_info& vhd_info::operator = (const vhd_info& other)
{
    if (this != &other) {
        _volume_id = other._volume_id;
        _volume_name = other._volume_name;
    }
    return *this;
}

vhd_info& vhd_info::operator = (vhd_info&& other)
{
    if (this != &other) {
        _volume_id = other._volume_id;
        _volume_name = std::move(other._volume_name);
    }
    return *this;
}

void vhd_info::clear()
{
    _volume_id = -1;
    _volume_name.clear();
}


config_vhd_info::config_vhd_info() : vhd_info()
{
}

config_vhd_info::config_vhd_info(const vhd_info& other) : vhd_info(other), _dir_config(other.get_volume_name() + L"\\config"), _dir_profiles(other.get_volume_name() + L"\\profiles")
{
}

config_vhd_info::config_vhd_info(const config_vhd_info& other) : vhd_info(other), _dir_config(other._dir_config), _dir_profiles(other._dir_profiles)
{
}

config_vhd_info::config_vhd_info(config_vhd_info&& other) : vhd_info(other), _dir_config(std::move(other._dir_config)), _dir_profiles(std::move(other._dir_profiles))
{
}

config_vhd_info::~config_vhd_info()
{
}

config_vhd_info& config_vhd_info::operator = (const vhd_info& other)
{
    if (this != &other) {
        vhd_info::operator=(other);
        _dir_config = get_volume_name() + L"\\config";
        _dir_profiles = get_volume_name() + L"\\profiles";
    }
    return *this;
}

config_vhd_info& config_vhd_info::operator = (const config_vhd_info& other)
{
    if (this != &other) {
        vhd_info::operator=(other);
        _dir_config = other._dir_config;
        _dir_profiles = other._dir_profiles;
    }
    return *this;
}

config_vhd_info& config_vhd_info::operator = (config_vhd_info&& other)
{
    if (this != &other) {
        vhd_info::operator=(other);
        _dir_config = std::move(other._dir_config);
        _dir_profiles = std::move(other._dir_profiles);
    }
    return *this;
}

void config_vhd_info::clear()
{
    vhd_info::clear();
    _dir_config.clear();
    _dir_profiles.clear();
}





vhd_manager::vhd_manager()
{
}

vhd_manager::~vhd_manager()
{
    clear();
}

bool vhd_manager::initialize()
{
    _dll_file = GLOBAL.get_bin_dir() + L"\\nxrmvhdmgr.dll";
    _config_volume_file = GLOBAL.get_config_dir() + L"\\config.dat";
    _shadow_volume_file = GLOBAL.get_config_dir() + L"\\shadow.dat";

    _dll.initialize(_dll_file);

    if (!NX::fs::exists(_config_volume_file)) {
        LOGINFO(NX::string_formater(L"Config VHD doesn't exist, try to create a new one"));
        if (!_dll.vhd_create_from_template(vhd_manager_dll::SMALL_VHD, _config_volume_file, std::wstring(), true)) {
            LOGERROR(NX::string_formater(L"Fail to create Config VHD (%d)", GetLastError()));
            return false;
        }
        LOGINFO(NX::string_formater(L"Config VHD has been created"));
    }

    if (!NX::fs::exists(_shadow_volume_file)) {
        LOGINFO(NX::string_formater(L"Shadow VHD doesn't exist, try to create a new one"));
        if (!_dll.vhd_create_from_template(vhd_manager_dll::LARGE_VHD, _shadow_volume_file, std::wstring(), true)) {
            LOGERROR(NX::string_formater(L"Fail to create Shadow VHD (%d)", GetLastError()));
            return false;
        }
        LOGINFO(NX::string_formater(L"Shadow VHD has been created"));
    }

    // Cleanup all the existing volumes
    _dll.vhd_unmount((ULONG)-1);

    // Now try to load VHDs
    _config_volume = load_volume(_config_volume_file);
    if (_config_volume.empty()) {
        LOGERROR(NX::string_formater(L"Fail to load Config VHD (%d), try to create a fresh one", GetLastError()));
		if (!_dll.vhd_create_from_template(vhd_manager_dll::SMALL_VHD, _config_volume_file, std::wstring(), true)) {
			LOGERROR(NX::string_formater(L"Fail to create Config VHD (%d)", GetLastError()));
			return false;
		}
		_config_volume = load_volume(_config_volume_file);
		if (_config_volume.empty()) {
			LOGERROR(NX::string_formater(L"Fail to load Config VHD (%d)", GetLastError()));
			return false;
		}
    }
    LOGINFO(NX::string_formater(L"Config VHD has been loaded"));
    LOGINFO(NX::string_formater(L"    - Id: %d", _config_volume.get_volume_id()));
    LOGINFO(NX::string_formater(L"    - Name: %s", _config_volume.get_volume_name().c_str()));

    // Make sure target directory has been created
    BOOL Existing = FALSE;
    if (!NT::CreateDirectory(_config_volume.get_config_dir().c_str(), NULL, FALSE, &Existing)) {
        LOGERROR(NX::string_formater(L"Fail to create config directory on VHD (%d, %s), try to create a fresh Config VHD", GetLastError(), _config_volume.get_config_dir().c_str()));
        unload_volume(_config_volume.get_volume_id());
        _config_volume.clear();

        if (!_dll.vhd_create_from_template(vhd_manager_dll::SMALL_VHD, _config_volume_file, std::wstring(), true)) {
            LOGERROR(NX::string_formater(L"Fail to re-create Config VHD (%d)", GetLastError()));
            return false;
        }
        _config_volume = load_volume(_config_volume_file);
        if (_config_volume.empty()) {
            LOGERROR(NX::string_formater(L"Fail to load Config VHD (%d) after re-creating VHD", GetLastError()));
            return false;
        }
        if (!NT::CreateDirectory(_config_volume.get_config_dir().c_str(), NULL, FALSE, &Existing)) {
            LOGERROR(NX::string_formater(L"Fail to create config directory on VHD (%d, %s) after re-creating VHD", GetLastError(), _config_volume.get_config_dir().c_str()));
        }
    }
    else {
        if (!Existing) {
            LOGINFO(NX::string_formater(L"Config directory on VHD doesn't exist, create it: %s", _config_volume.get_config_dir().c_str()));
        }
    }
    Existing = FALSE;
    if (!NT::CreateDirectory(_config_volume.get_profiles_dir().c_str(), NULL, FALSE, &Existing)) {
        LOGERROR(NX::string_formater(L"Fail to create profiles directory on VHD (%d, %s)", GetLastError(), _config_volume.get_profiles_dir().c_str()));
    }
    else {
        if (!Existing) {
            LOGINFO(NX::string_formater(L"Profiles directory on VHD doesn't exist, create it: %s", _config_volume.get_profiles_dir().c_str()));
        }
    }

    _shadow_volume = load_volume(_shadow_volume_file);
    if (_shadow_volume.empty()) {
        LOGERROR(NX::string_formater(L"Fail to load Shadow VHD (%d), try to create a fresh one", GetLastError()));
		if (!_dll.vhd_create_from_template(vhd_manager_dll::LARGE_VHD, _shadow_volume_file, std::wstring(), true)) {
			LOGERROR(NX::string_formater(L"Fail to create Shadow VHD (%d)", GetLastError()));
			return false;
		}
		_shadow_volume = load_volume(_shadow_volume_file);
		if (_shadow_volume.empty()) {
			LOGERROR(NX::string_formater(L"Fail to load Shadow VHD (%d)", GetLastError()));
			return false;
		}
    }
    LOGINFO(NX::string_formater(L"Shadow VHD has been loaded"));
    LOGINFO(NX::string_formater(L"    - Id: %d", _shadow_volume.get_volume_id()));
    LOGINFO(NX::string_formater(L"    - Name: %s", _shadow_volume.get_volume_name().c_str()));

    return true;
}

void vhd_manager::clear()
{
    if (!_config_volume.empty()) {
        unload_volume(_config_volume.get_volume_id());
        _config_volume.clear();
    }
    if (!_shadow_volume.empty()) {
        unload_volume(_shadow_volume.get_volume_id());
        _shadow_volume.clear();
    }
    _dll.clear();
}

vhd_info vhd_manager::load_volume(const std::wstring& file)
{
    unsigned long disk_id;
    wchar_t name_buf[MAX_PATH] = { 0 };
    unsigned long buf_size = MAX_PATH * 2;

    if (!_dll.vhd_mount(file, std::wstring(), false, 0, &disk_id)) {
        LOGERROR(NX::string_formater(L"mhdmgr!mount failed (%d)", GetLastError()));
        return vhd_info();
    }

   memset(name_buf, 0, sizeof(name_buf));
   if (!_dll.vhd_get_property(disk_id, VHD_PROPERTY_NT_NAME, name_buf, &buf_size)) {
        LOGERROR(NX::string_formater(L"mhdmgr!get volume name failed (%d)", GetLastError()));
       _dll.vhd_unmount(disk_id);
       return vhd_info();
   }

   vhd_info vhdi;
   vhdi._volume_id = disk_id;
   vhdi._volume_name = name_buf;
   return vhdi;
}

void vhd_manager::unload_volume(unsigned long volume_id)
{
    _dll.vhd_unmount(volume_id);
}

bool vhd_manager::read_config_file(const std::wstring& path, std::string& content)
{
    std::wstring full_path;

    if (boost::algorithm::istarts_with(path, _config_volume.get_volume_name())) {
        full_path = path;
    }
    else {
        full_path = _config_volume.get_volume_name();
        full_path += path;
    }

    if (INVALID_FILE_ATTRIBUTES == NT::GetFileAttributes(full_path.c_str())) {
        return false;
    }

    HANDLE h = NULL;
    LARGE_INTEGER file_size = { 0, 0 };
    bool result = false;

    do {

        h = NT::CreateFile(full_path.c_str(), FILE_GENERIC_READ, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL);
        if (NULL == h) {
            break;
        }

        if (!NT::GetFileSize(h, &file_size)) {
            break;
        }

        if (file_size.QuadPart == 0) {
            result = true;
            break;
        }

        if (file_size.HighPart != 0) {
            SetLastError(ERROR_FILE_TOO_LARGE);
            break;
        }
        if (((int)file_size.LowPart) < 0) {
            SetLastError(ERROR_FILE_TOO_LARGE);
            break;
        }

        unsigned long bytes_read = 0;
        LARGE_INTEGER bytes_offset = { 0, 0 };
        std::vector<unsigned char> buf;
        buf.resize(file_size.LowPart, 0);
        if (!NT::ReadFile(h, buf.data(), file_size.LowPart, &bytes_read, &bytes_offset)) {
            break;
        }

        content = std::move(std::string(buf.begin(), buf.end()));
        result = true;

    } while (FALSE);

    if (NULL != h) {
        NT::CloseHandle(h);
        h = NULL;
    }
    return result;
}

bool vhd_manager::write_config_file(const std::wstring& path, const std::string& content)
{
    std::wstring full_path;

    if (boost::algorithm::istarts_with(path, _config_volume.get_volume_name())) {
        full_path = path;
    }
    else {
        full_path = _config_volume.get_volume_name();
        full_path += path;
    }

    HANDLE h = NULL;
    bool result = false;

    do {

        h = NT::CreateFile(full_path.c_str(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE, NULL);
        if (NULL == h) {
            break;
        }

        if (content.length() == 0) {
            result = true;
            break;
        }

        unsigned long bytes_written = 0;
        LARGE_INTEGER bytes_offset = { 0, 0 };
        if (!NT::WriteFile(h, (PVOID)content.c_str(), (unsigned long)content.length(), &bytes_written, &bytes_offset)) {
            break;
        }

        result = true;

    } while (FALSE);

    if (NULL != h) {
        NT::CloseHandle(h);
        h = NULL;
    }
    return result;
}