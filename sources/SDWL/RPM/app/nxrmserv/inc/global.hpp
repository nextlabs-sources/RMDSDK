
#pragma once
#ifndef __NXRMSERV_GLOBAL_HPP__
#define __NXRMSERV_GLOBAL_HPP__


#include <string>
#include <vector>
#include <map>

#include <nudf\dbglog.hpp>
#include <nudf\winutil.hpp>
#include <nudf\rwlock.hpp>

#include "process.hpp"
#include "..\SDWL\SDWRmcLib\Winutil\keym.h"

class nxrm_global
{
public:
    nxrm_global();
    ~nxrm_global();

    //
    //  inline functions
    //
    inline const NX::win::file_version& get_file_version() const { return _file_version; }
    inline const std::wstring& get_company_name() const { return _file_version.get_company_name(); }
    inline const std::wstring& get_product_name() const { return _file_version.get_product_name(); }
    inline const std::wstring& get_product_version_string() const { return _file_version.get_product_version_string(); }
    inline const std::wstring& get_file_name() const { return _file_version.get_file_name(); }
    inline const std::wstring& get_file_version_string() const { return _file_version.get_file_version_string(); }

    // dirs
    inline const std::wstring& get_product_dir() const { return _product_dir; }
    inline const std::wstring& get_bin_dir() const { return _bin_dir; }
#ifdef _AMD64_
    inline const std::wstring& get_bin32_dir() const { return _bin32_dir; }
#endif
    inline const std::wstring& get_working_dir() const { return _working_dir; }
    inline const std::wstring& get_config_dir() const { return _config_dir; }
    inline const std::wstring& get_profiles_dir() const { return _profiles_dir; }

    // modules
    inline HMODULE get_res_module() const { return _resdll; }

    // host
    inline const NX::win::host& get_host() const { return _host; }
    inline const std::wstring& get_host_name() const { return _host.in_domain() ? _host.fqdn_name() : _host.dns_host_name(); }
    inline bool in_domain() const { return _host.in_domain(); }

    // cpu
    inline const NX::win::hardware::processor_information& get_cpu_info() const { return _cpu_info; }
    inline const std::wstring& get_cpu_vender() const { return _cpu_info.get_vendor_name(); }
    inline const std::wstring& get_cpu_brand() const { return _cpu_info.get_processor_brand(); }
    inline unsigned long get_cpu_cores() const { return _cpu_info.get_cores_count(); }
    inline unsigned long get_cpu_logical_processors() const { return _cpu_info.get_logical_processors_count(); }

    // os
    inline const NX::win::windows_info& get_windows_info() const { return _win_info; }

    // rights management data
    inline process_cache& get_process_cache() { return _process_cache; }

    // eas
    inline const NX::win::explicit_access& get_ea_admin() const { return _ea_admin; };
    inline const NX::win::explicit_access& get_ea_everyone_ro() const{ return _ea_everyone_ro; };
    inline const NX::win::explicit_access& get_ea_everyone_rw() const{ return _ea_everyone_rw; };

    // config
    inline const std::wstring& get_filetype_blacklist() const { return _filetype_blacklist; }

    //
    // initialize & cleanup
    //
    void initialize();
    void clear();

	// key manangement for secure file read/write
	void initkey();

    std::wstring get_temp_file_name(const std::wstring& dir);
    std::wstring get_temp_folder_name(const std::wstring& dir, LPSECURITY_ATTRIBUTES sa = NULL);

    bool generate_file(const std::wstring& file, const std::string& content, bool force);
    bool nt_generate_file(const std::wstring& file, const std::string& content, bool force);
    bool load_file(const std::wstring& file, std::string& content);
    bool nt_load_file(const std::wstring& file, std::string& content);

    process_record safe_find_process(unsigned long process_id);

protected:
    void init_dirs();
    void init_office_customized_ui_settings();
    void init_modules();

private:
    // no copy is allowed
    nxrm_global& operator = (const nxrm_global& other) { return *this; }

private:
    NX::win::file_version _file_version;
    NX::win::hardware::processor_information _cpu_info;

    std::wstring    _module_path;
    std::wstring    _product_dir;
    std::wstring    _bin_dir;
#ifdef _AMD64_
    std::wstring    _bin32_dir;
#endif
    std::wstring    _working_dir;
    std::wstring    _config_dir;
    std::wstring    _profiles_dir;
    
    HMODULE         _resdll;

    // hardware & software information
    NX::win::host   _host;
    NX::win::windows_info _win_info;

    // cache
    process_cache   _process_cache;

    const NX::win::explicit_access    _ea_admin;
    const NX::win::explicit_access    _ea_everyone_ro;
    const NX::win::explicit_access    _ea_everyone_rw;

    const std::wstring _filetype_blacklist;

	// key mananger
	NX::RmcKeyManager	_key_manager;
};

extern nxrm_global GLOBAL;

BOOL __stdcall GlobalLogAccept(_In_ ULONG Level);
LONG __stdcall GlobalLogWrite(_In_ LPCWSTR Info);


#endif