
#pragma once
#ifndef __NXRM_VHDMANAGER_HPP__
#define __NXRM_VHDMANAGER_HPP__


#include <boost/noncopyable.hpp>

#include <string>
#include <vector>
#include <queue>
#include <map>

#include "nxrmvhdhlp.hpp"

class vhd_manager;

class vhd_info
{
public:
    vhd_info();
    vhd_info(const vhd_info& other);
    vhd_info(vhd_info&& other);
    virtual ~vhd_info();

    inline bool empty() const { return (-1 == _volume_id || _volume_name.empty()); }
    inline unsigned long get_volume_id() const { return _volume_id; }
    inline const std::wstring& get_volume_name() const { return _volume_name; }

    vhd_info& operator = (const vhd_info& other);
    vhd_info& operator = (vhd_info&& other);
    virtual void clear();

private:
    unsigned long   _volume_id;
    std::wstring    _volume_name;

    friend class vhd_manager;
};

class config_vhd_info : public vhd_info
{
public:
    config_vhd_info();
    config_vhd_info(const vhd_info& other);
    config_vhd_info(const config_vhd_info& other);
    config_vhd_info(config_vhd_info&& other);
    virtual ~config_vhd_info();

    inline const std::wstring& get_config_dir() const { return _dir_config; }
    inline const std::wstring& get_profiles_dir() const { return _dir_profiles; }

    config_vhd_info& operator = (const vhd_info& other);
    config_vhd_info& operator = (const config_vhd_info& other);
    config_vhd_info& operator = (config_vhd_info&& other);
    virtual void clear();

private:
    std::wstring    _dir_config;
    std::wstring    _dir_profiles;

    friend class vhd_manager;
};

class vhd_manager : private boost::noncopyable
{
public:
    vhd_manager();
    virtual ~vhd_manager();

    bool initialize();
    void clear();

    inline const config_vhd_info& get_config_volume() const { return _config_volume; }
    inline const vhd_info& get_shadow_volume() const { return _shadow_volume; }

    bool read_config_file(const std::wstring& path, std::string& content);
    bool write_config_file(const std::wstring& path, const std::string& content);

protected:
    vhd_info load_volume(const std::wstring& file);
    void unload_volume(unsigned long volume_id);
    
private:
    vhd_manager_dll _dll;
    config_vhd_info _config_volume;
    vhd_info        _shadow_volume;
    std::wstring    _dll_file;
    std::wstring    _config_volume_file;
    std::wstring    _shadow_volume_file;
};




#endif  // __NXRM_KEYMANAGER_HPP__