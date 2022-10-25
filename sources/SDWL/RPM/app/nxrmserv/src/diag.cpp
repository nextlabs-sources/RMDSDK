


#include <Windows.h>
#include <assert.h>
#include <TlHelp32.h>


#include <nudf/eh.hpp>
#include <nudf/debug.hpp>
#include <nudf/conversion.hpp>
#include <nudf/string.hpp>
#include <nudf/crypto.hpp>
#include <nudf/zip.hpp>
#include <nudf/filesys.hpp>
#include <nudf/winutil.hpp>
#include <nudf/service.hpp>

#include "nxrmserv.hpp"
#include "serv.hpp"
#include "session.hpp"
#include "global.hpp"
#include "diag.hpp"


extern rmserv* SERV;

//
//
//


diagtool::diagtool()
{
}

diagtool::~diagtool()
{
}

bool diagtool::generate(const std::wstring& user_sid, const std::wstring& tenant_id, const std::wstring& user_id, const std::wstring& target_dir, std::wstring& dbgfile)
{
    SYSTEMTIME st = { 0 };
    GetLocalTime(&st);

    const std::wstring& dbgfilename = NX::string_formater(L"NXT-DBG-%s-%04d%02d%02d-%02d%02d%02d%03d", GLOBAL.get_host().dns_host_name().c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    _tempdir = NX::win::get_windows_temp_directory();
    _tempdir += L"\\";
    _tempdir += dbgfilename;
    _user_sid = user_sid;
    _tenant_id = tenant_id;
    _user_id = user_id;

    std::wstring generated_file = target_dir;
    if (!boost::algorithm::ends_with(generated_file, L"\\")) {
        generated_file += L"\\";
    }
    generated_file += dbgfilename;
    generated_file += L".zip";

    ::DeleteFileW(_tempdir.c_str());
    if (!::CreateDirectoryW(_tempdir.c_str(), NULL)) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			LOGDEBUG(NX::string_formater(L"CreateDirectory failed:: %s"), _tempdir.c_str());
			return false;
		}
    }

    bool result = false;
	LOGDEBUG(NX::string_formater(L"diagtool::generate: user_sid: %s"), user_sid);

    do {

        try {

            dump_system_summary();
            dump_services_summary();
            dump_software_summary();
            dump_rmc_summary();
            dump_rmc_debuglog();
            dump_rmc_user_profiles();
			collect_windows_event_logs();

            NX::win::security_attribute sa(std::vector<NX::win::explicit_access>({
                NX::win::explicit_access(user_sid, GENERIC_ALL, TRUSTEE_IS_USER, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
                NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
                NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
            }));
            
            result = NX::ZIP::zip(_tempdir, generated_file, &sa);

        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }

        result = true;

    } while (false);

    if (result) {
        dbgfile = dbgfilename;
    }
    NX::fs::remove_directory(_tempdir, true);
    return result;
}

void diagtool::dump_system_summary()
{
    NX::json_value v = NX::json_value::create_object(true);
    v[L"host"] = NX::json_value(GLOBAL.get_host_name());

    // set os information
    v[L"os"] = NX::json_value::create_object(true);
    NX::json_value& os_value = v[L"os"];
    report_os_info(os_value);

    // set processor information
    v[L"processor"] = NX::json_value::create_object(true);
    NX::json_value& processor_value = v[L"processor"];
    report_processor_info(processor_value);

    // set memory information
    v[L"memory"] = NX::json_value::create_object(true);
    NX::json_value& memory_value = v[L"memory"];
    report_memory_info(memory_value);

    // set network information
    v[L"network adapters"] = NX::json_value::create_object(true);
    NX::json_value& network_value = v[L"network adapters"];
    report_network_adapters_info(network_value);

    // set drives information
    v[L"drives"] = NX::json_value::create_object(true);
    NX::json_value& drives_value = v[L"drives"];
    report_drives_info(drives_value);

    // set active process information
    v[L"active processes"] = NX::json_value::create_object(true);
    NX::json_value& process_value = v[L"active processes"];
    report_processes_info(process_value);
    
    const std::wstring& ws = v.serialize();
    const std::string& s = NX::conversion::utf16_to_utf8(ws);
    GLOBAL.generate_file(std::wstring(_tempdir + L"\\system_summary.json"), s, true);
}


void diagtool::dump_services_summary()
{
    NX::json_value v = NX::json_value::create_object(true);
    report_services_info(v);
    
    const std::wstring& ws = v.serialize();
    const std::string& s = NX::conversion::utf16_to_utf8(ws);
    GLOBAL.generate_file(std::wstring(_tempdir + L"\\services_summary.json"), s, true);
}


void diagtool::dump_software_summary()
{
#ifdef _WIN64
    NX::json_value v = NX::json_value::create_object(true);
#else
    NX::json_value v = NX::json_value::create_array();
#endif
    report_software_info(v);

    const std::wstring& ws = v.serialize();
    const std::string& s = NX::conversion::utf16_to_utf8(ws);
    GLOBAL.generate_file(std::wstring(_tempdir + L"\\software_summary.json"), s, true);
}

void diagtool::dump_rmc_summary()
{
    NX::json_value v = NX::json_value::create_object(true);
    const std::wstring win_dir = NX::win::get_windows_directory();
    const std::wstring winsys_dir = NX::win::get_system_directory();
    std::vector<std::wstring> rmc_files = {
        std::wstring(winsys_dir + L"\\drivers\\nxrmdrv.sys"),
        std::wstring(winsys_dir + L"\\drivers\\nxrmflt.sys"),
        std::wstring(winsys_dir + L"\\drivers\\nxrmvhd.sys"),
#ifdef _WIN64
        std::wstring(winsys_dir + L"\\nxrmcore64.dll"),
        std::wstring(win_dir + L"\\SysWOW64\\nxrmcore.dll")
#else
        std::wstring(winsys_dir + L"\\nxrmcore.dll")
#endif
    };
    rmc_files.push_back(GLOBAL.get_product_dir());

    v[L"name"] = NX::json_value(GLOBAL.get_product_name());
    v[L"version"] = NX::json_value(GLOBAL.get_product_version_string());
    v[L"directory"] = NX::json_value(GLOBAL.get_product_dir());
    v[L"router"] = NX::json_value(SERV->get_router_config().get_router());
    v[L"tenant"] = NX::json_value(SERV->get_router_config().get_tenant_id());
    v[L"files"] = create_files_report(rmc_files);

    const std::wstring& ws = v.serialize();
    const std::string s = NX::conversion::utf16_to_utf8(ws);
    GLOBAL.generate_file(std::wstring(_tempdir + L"\\rmc_summary.json"), s, true);
}

void diagtool::dump_rmc_debuglog()
{
    const std::wstring filter(GLOBAL.get_product_dir() + L"\\DebugDump*");
    find_data wfd;
    HANDLE h = ::FindFirstFileW(filter.c_str(), &wfd);
    if (INVALID_HANDLE_VALUE == h) {
        return;
    }

    const std::wstring logdir(_tempdir + L"\\DebugDump");
    CreateDirectoryW(logdir.c_str(), NULL);

    do {
        const std::wstring source(GLOBAL.get_product_dir() + L"\\" + wfd.cFileName);
        const std::wstring target(logdir + L"\\" + wfd.cFileName);
        CopyFile(source.c_str(), target.c_str(), FALSE);
    } while (FindNextFileW(h, &wfd));

    FindClose(h);
    h = INVALID_HANDLE_VALUE;
}

void diagtool::collect_windows_event_logs()
{
	HANDLE hEventLog = NULL;
	DWORD status = ERROR_SUCCESS;

	hEventLog = OpenEventLogW(NULL, L"System");
	if (NULL != hEventLog)
	{
		const std::wstring filename(_tempdir + L"\\system.evt");
		BackupEventLogW(hEventLog, filename.c_str());
		CloseEventLog(hEventLog);
	}

	hEventLog = OpenEventLogW(NULL, L"Application");
	if (NULL != hEventLog)
	{
		const std::wstring filename(_tempdir + L"\\application.evt");
		BackupEventLogW(hEventLog, filename.c_str());
		CloseEventLog(hEventLog);
	}

	hEventLog = OpenEventLogW(NULL, L"Setup");
	if (NULL != hEventLog)
	{
		const std::wstring filename(_tempdir + L"\\setup.evt");
		BackupEventLogW(hEventLog, filename.c_str());
		CloseEventLog(hEventLog);
	}

	hEventLog = OpenEventLogW(NULL, L"Security");
	if (NULL != hEventLog)
	{
		const std::wstring filename(_tempdir + L"\\security.evt");
		BackupEventLogW(hEventLog, filename.c_str());
		CloseEventLog(hEventLog);
	}
}

void diagtool::dump_rmc_user_profiles()
{
    // dump normal files

    // dump protected files
    // \Device\NsVolume9\profiles\<SID>\<TENNAT>\<USERID>\

    const std::wstring context_cache_file(SERV->get_config_dir() + L"\\core_context.json");
    const std::wstring protected_profiles_dir(SERV->get_profiles_dir() + L"\\" + _user_sid + L"\\" + _tenant_id + L"\\" + _user_id);
    const std::wstring cached_login_status_file(SERV->get_profiles_dir() + L"\\" + _user_sid + L"\\" + CACHE_LOGIN_STATUS_FILE);
    const std::wstring logon_user_policy_bundle(protected_profiles_dir + L"\\" + POLICY_BUNDLE_FILE);
    const std::wstring logon_user_client_config(protected_profiles_dir + L"\\" + CLIENT_CONFIG_FILE);
    const std::wstring logon_user_client_local_config(protected_profiles_dir + L"\\" + CLIENT_LOCAL_CONFIG_FILE);
    const std::wstring logon_user_classify_config(protected_profiles_dir + L"\\" + CLASSIFY_CONFIG_FILE);
    const std::wstring logon_user_watermark_config(protected_profiles_dir + L"\\" + WATERMARK_CONFIG_FILE);


    std::string content;

    // cached core context
    if (GLOBAL.nt_load_file(context_cache_file, content)) {
        const std::wstring newFile(_tempdir + L"\\core_context.json");
        GLOBAL.generate_file(newFile, content, true);
    }

    // cached login status
    if (GLOBAL.nt_load_file(cached_login_status_file, content)) {
        const std::wstring newFile(_tempdir + L"\\" + CACHE_LOGIN_STATUS_FILE);
        GLOBAL.generate_file(newFile, content, true);
    }

    // cached policy bundle
    if (GLOBAL.nt_load_file(logon_user_policy_bundle, content)) {
        const std::wstring newFile(_tempdir + L"\\" + POLICY_BUNDLE_FILE);
        GLOBAL.generate_file(newFile, content, true);
    }

    // cached client config
    if (GLOBAL.nt_load_file(logon_user_client_config, content)) {
        const std::wstring newFile(_tempdir + L"\\" + CLIENT_CONFIG_FILE);
        GLOBAL.generate_file(newFile, content, true);
    }

    // cached client local config
    if (GLOBAL.nt_load_file(logon_user_client_local_config, content)) {
        const std::wstring newFile(_tempdir + L"\\" + CLIENT_LOCAL_CONFIG_FILE);
        GLOBAL.generate_file(newFile, content, true);
    }

    // cached classify config
    if (GLOBAL.nt_load_file(logon_user_classify_config, content)) {
        const std::wstring newFile(_tempdir + L"\\" + CLASSIFY_CONFIG_FILE);
        GLOBAL.generate_file(newFile, content, true);
    }

    // cached watermark config
    if (GLOBAL.nt_load_file(logon_user_watermark_config, content)) {
        const std::wstring newFile(_tempdir + L"\\" + WATERMARK_CONFIG_FILE);
        GLOBAL.generate_file(newFile, content, true);
    }

    // Register XML
    const std::wstring register_xml_file(GLOBAL.get_config_dir() + L"\\register.xml");
    const std::wstring new_register_xml_file(_tempdir + L"\\register.xml");
    CopyFileW(register_xml_file.c_str(), new_register_xml_file.c_str(), FALSE);

    // watermark file
    const std::wstring watermark_image_file(GLOBAL.get_profiles_dir() + L"\\" + _user_sid + L"\\" + _tenant_id + L"\\" + _user_id + L"\\watermark.png");
    const std::wstring new_watermark_image_file(_tempdir + L"\\watermark.png");
    CopyFileW(watermark_image_file.c_str(), new_watermark_image_file.c_str(), FALSE);

    // activity file
    const std::wstring activity_log_file(GLOBAL.get_profiles_dir() + L"\\" + _user_sid + L"\\" + _tenant_id + L"\\" + _user_id + L"\\activity.log");
    const std::wstring new_activity_log_file(_tempdir + L"\\activity.log");
    CopyFileW(activity_log_file.c_str(), new_activity_log_file.c_str(), FALSE);

    // audit log
    const std::wstring audit_log_file(GLOBAL.get_profiles_dir() + L"\\" + _user_sid + L"\\" + _tenant_id + L"\\" + _user_id + L"\\audit.json");
    const std::wstring new_audit_log_file(_tempdir + L"\\audit.json");
    CopyFileW(audit_log_file.c_str(), new_audit_log_file.c_str(), FALSE);
}

void diagtool::create_files_report(NX::json_value& parent, const std::wstring& file)
{
    find_data wfd;
    HANDLE h = ::FindFirstFileW(file.c_str(), &wfd);
    if (INVALID_HANDLE_VALUE == h) {
        return;
    }
    FindClose(h);
    h = INVALID_HANDLE_VALUE;

    if (FILE_ATTRIBUTE_DIRECTORY == (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)) {
        const std::wstring& dir_key = NX::string_formater(L"[%s]", wfd.cFileName);
        parent[wfd.cFileName] = directory_report(file);
    }
    else {
        parent[wfd.cFileName] = file_report(file, wfd);
    }
}

NX::json_value diagtool::create_files_report(const std::vector<std::wstring>& files)
{
    NX::json_value v = NX::json_value::create_object();
    std::for_each(files.begin(), files.end(), [&](const std::wstring& file) {
        create_files_report(v, file);
    });
    return std::move(v);
}

NX::json_value diagtool::directory_report(const std::wstring& dir)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    std::vector<std::wstring> sub_dirs;
    find_data wfd;

    NX::json_value v = NX::json_value::create_object();

    const std::wstring current_dir(boost::algorithm::iends_with(dir, L"\\") ? dir : (dir + L"\\"));
    const std::wstring filter(current_dir + L"*");

    const std::wstring win_user_profile_dir(GLOBAL.get_profiles_dir() + L"\\" + _user_sid);
    const bool is_win_profile_dir = (0 == _wcsicmp(win_user_profile_dir.c_str(), dir.c_str()));

    h = ::FindFirstFileW(filter.c_str(), &wfd);
    if (INVALID_HANDLE_VALUE == h) {
        return v;
    }

    do {

        if (0 == _wcsicmp(wfd.cFileName, L".") || 0 == _wcsicmp(wfd.cFileName, L"..")) {
            continue;
        }

        if (is_win_profile_dir && (0 == _wcsicmp(wfd.cFileName, L"AppData") || 0 == _wcsicmp(wfd.cFileName, L"Desktop") || 0 == _wcsicmp(wfd.cFileName, L"Local"))) {
            continue;
        }

        const std::wstring file(current_dir + wfd.cFileName);

        if (FILE_ATTRIBUTE_DIRECTORY == (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)) {
            sub_dirs.push_back(wfd.cFileName);
        }
        else {
            v[wfd.cFileName] = file_report(file, wfd);
        }

        wfd.clear();

    } while (FindNextFileW(h, &wfd));

    FindClose(h);
    h = INVALID_HANDLE_VALUE;

    std::for_each(sub_dirs.begin(), sub_dirs.end(), [&](const std::wstring& dir_name) {
        const std::wstring& dir_key = NX::string_formater(L"[%s]", dir_name.c_str());
        v[dir_key] = directory_report(std::wstring(current_dir + dir_name));
    });

    return std::move(v);
}

NX::json_value diagtool::file_report(const std::wstring& file, const find_data& wfd)
{
    NX::json_value v = NX::json_value::create_object();
    v[L"size"] = NX::json_value(wfd.get_filesize());
    v[L"attributes"] = NX::json_value(build_file_attributes(wfd.dwFileAttributes));
    v[L"creation time"] = NX::json_value(build_file_time(&wfd.ftCreationTime));
    v[L"last access time"] = NX::json_value(build_file_time(&wfd.ftLastAccessTime));
    v[L"last write time"] = NX::json_value(build_file_time(&wfd.ftLastWriteTime));

    NX::win::pe_file pef(file);
    if (!pef.empty()) {
        v[L"PE"] = NX::json_value::create_object();
        NX::json_value& peobject = v[L"PE"];

        peobject[L"checksum"] = NX::json_value(NX::conversion::to_wstring(pef.get_image_checksum()));
        peobject[L"image base"] = NX::json_value(NX::conversion::to_wstring(pef.get_image_base()));
        peobject[L"type"] = NX::json_value(pef.is_exe() ? L"EXE" : (pef.is_dll() ? L"DLL" : (pef.is_driver() ? L"DRIVER" : L"Unknown")));
        peobject[L"architecture"] = NX::json_value(pef.is_x86_image() ? L"x86" : (pef.is_x64_image() ? L"x64" : (pef.is_ia64_image() ? L"IA64" : L"Unknown")));

        if (!pef.get_image_publisher().empty()) {
            peobject[L"signature"] = NX::json_value::create_object();
            NX::json_value& pesignature = peobject[L"signature"];
            pesignature[L"publisher"] = NX::json_value(pef.get_image_publisher());
        }

        peobject[L"filever"] = NX::json_value::create_object();
        NX::json_value& pefilever = peobject[L"filever"];
        pefilever[L"Company Name"] = NX::json_value(pef.get_file_version().get_company_name());
        pefilever[L"Product Name"] = NX::json_value(pef.get_file_version().get_product_name());
        pefilever[L"Product Version"] = NX::json_value(pef.get_file_version().get_product_version_string());
        pefilever[L"File Original Name"] = NX::json_value(pef.get_file_version().get_file_name());
        pefilever[L"File Description"] = NX::json_value(pef.get_file_version().get_file_description());
        pefilever[L"File Version"] = NX::json_value(pef.get_file_version().get_file_version_string());
        switch (pef.get_file_version().get_file_type())
        {
        case VFT_APP:
            pefilever[L"File Type"] = NX::json_value(L"Application");
            break;
        case VFT_DLL:
            pefilever[L"File Type"] = NX::json_value(L"Dynamic Link Library");
            break;
        case VFT_DRV:
            pefilever[L"File Type"] = NX::json_value(L"Driver");
            break;
        case VFT_FONT:
            pefilever[L"File Type"] = NX::json_value(L"Font");
            break;
        case VFT_STATIC_LIB:
            pefilever[L"File Type"] = NX::json_value(L"Static Library");
            break;
        case VFT_VXD:
            pefilever[L"File Type"] = NX::json_value(L"Vxd");
            break;
        }
    }

    return std::move(v);
}

std::wstring diagtool::build_file_attributes(DWORD dwFileAttributes)
{
    std::wstring s;
    s = (FILE_ATTRIBUTE_DIRECTORY == (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) ? L"d" : L"-";
    s.append((FILE_ATTRIBUTE_READONLY == (dwFileAttributes & FILE_ATTRIBUTE_READONLY)) ? L"r-" : L"rw");
    s.append((FILE_ATTRIBUTE_HIDDEN == (dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) ? L"h" : L"-");
    s.append((FILE_ATTRIBUTE_SYSTEM == (dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) ? L"s" : L"-");
    s.append((FILE_ATTRIBUTE_ENCRYPTED == (dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) ? L"e" : L"-");
    s.append((FILE_ATTRIBUTE_COMPRESSED == (dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)) ? L"c" : L"-");
    s.append((FILE_ATTRIBUTE_REPARSE_POINT == (dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) ? L"l" : L"-");
    return std::move(s);
}

std::wstring diagtool::build_file_time(const FILETIME* ft)
{
    if (ft->dwHighDateTime == 0 && ft->dwLowDateTime == 0) {
        return L"N/A";
    }

    FILETIME local_ft = { 0, 0 };
    SYSTEMTIME st = { 0 };
    FileTimeToLocalFileTime(ft, &local_ft);
    FileTimeToSystemTime(&local_ft, &st);
    return NX::string_formater(L"%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

void diagtool::report_os_info(NX::json_value& v)
{
    NX::win::system_default_language sys_lang;
    v[L"name"] = NX::json_value(GLOBAL.get_windows_info().windows_name());
    v[L"edition"] = NX::json_value(GLOBAL.get_windows_info().windows_edition());
    v[L"build"] = NX::json_value((int)GLOBAL.get_windows_info().build_number());
    v[L"type"] = NX::json_value(GLOBAL.get_windows_info().is_processor_x86() ? L"32 Bits" : L"64 Bits");
    v[L"language"] = NX::json_value(sys_lang.name());
}

void diagtool::report_processor_info(NX::json_value& v)
{
    v[L"CPU"] = NX::json_value(GLOBAL.get_cpu_brand());
    v[L"cores"] = NX::json_value((int)GLOBAL.get_cpu_logical_processors());
}

void diagtool::report_memory_info(NX::json_value& v)
{
    NX::win::hardware::memory_information mem_info;
    v[L"total"] = NX::json_value(NX::string_formater(L"%.1f GB", (mem_info.get_physical_total() / 1024.0)));
    v[L"used"] = NX::json_value(NX::string_formater(L"%d%%", mem_info.get_load()));
}

void diagtool::report_network_adapters_info(NX::json_value& v)
{
    const std::vector<NX::win::hardware::network_adapter_information>& adapters = NX::win::hardware::get_all_network_adapters();
    for (int i = 0; i < (int)adapters.size(); i++) {

        v[adapters[i].get_adapter_name()] = NX::json_value::create_object(true);
        NX::json_value& adapter = v[adapters[i].get_adapter_name()];
        adapter[L"Friendly Name"] = NX::json_value(adapters[i].get_friendly_name());
        adapter[L"Description"] = NX::json_value(adapters[i].get_description());
        adapter[L"IfType"] = NX::json_value(adapters[i].get_if_type_name());
        adapter[L"OperStatus"] = NX::json_value(adapters[i].get_oper_status_name());
        adapter[L"MAC"] = NX::json_value(adapters[i].get_mac_address());
        adapter[L"IPv4"] = NX::json_value(adapters[i].get_ipv4_addresses().empty() ? L"" : adapters[i].get_ipv4_addresses()[0].c_str());
        adapter[L"IPv6"] = NX::json_value(adapters[i].get_ipv6_addresses().empty() ? L"" : adapters[i].get_ipv6_addresses()[0].c_str());
    }
}

void diagtool::report_drives_info(NX::json_value& v)
{
    const std::vector<NX::fs::drive>& drives = NX::fs::get_logic_drives();
    for (int i = 0; i < (int)drives.size(); i++) {

        const std::wstring& drive_letter = NX::string_formater(L"%c:", drives[i].drive_letter());
        v[drive_letter] = NX::json_value::create_object(true);
        NX::json_value& drive_object = v[drive_letter];

        drive_object[L"type"] = NX::json_value(drives[i].type_name());
        drive_object[L"NT name"] = NX::json_value(drives[i].nt_name());

        if (drives[i].is_fixed() || drives[i].is_removable() || drives[i].is_ramdisk()) {
            const NX::fs::drive::space& drive_space = drives[i].get_space();
            if (0 != drive_space.total_bytes()) {
                drive_object[L"Total space"] = NX::json_value(NX::string_formater(L"%d MB", (int)(drive_space.total_bytes() / 1048576)));
                drive_object[L"Free space"] = NX::json_value(NX::string_formater(L"%d MB (%.1f%%)", (int)(drive_space.available_free_bytes() / 1048576), 100 * ((float)drive_space.available_free_bytes() / (float)drive_space.total_bytes())));
            }
            else {
                if (drives[i].is_removable()) {
                    drive_object[L"Free space"] = NX::json_value(L"No media");
                }
            }
        }
    }
}

void diagtool::report_processes_info(NX::json_value& v)
{
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == h) {
        return;
    }

    PROCESSENTRY32W pe32 = { 0 };
    memset(&pe32, 0, sizeof(pe32));
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    if (!Process32First(h, &pe32)) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
        return;
    }

    do {

        try {
            const wchar_t* pwzName = wcsrchr(pe32.szExeFile, L'\\');
            if (NULL == pwzName) {
                pwzName = pe32.szExeFile;
            }
            const std::wstring& process_key = NX::string_formater(L"%s - %d", pwzName, (int)pe32.th32ProcessID);
            v[process_key] = NX::json_value::create_object(true);
            NX::json_value& process = v[process_key];
            process[L"image"] = NX::json_value(pe32.szExeFile);
            process[L"threads"] = NX::json_value((int)pe32.cntThreads);
            process[L"parent process id"] = NX::json_value((int)pe32.th32ParentProcessID);
        }
        catch (std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }

    } while (Process32Next(h, &pe32));


    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
}

void diagtool::report_services_info(NX::json_value& v)
{
    try {

        //
        NX::win::reg_local_machine services_key;
        const std::wstring services_key_path(L"SYSTEM\\CurrentControlSet\\Services");
        services_key.open(services_key_path, NX::win::reg_key::reg_wow64_64, true);

        services_key.enum_sub_keys([&](const wchar_t* key_name) {

            try {

                std::wstring svc_displayname;
                std::wstring svc_imagepath;
                std::wstring svc_start_name;
                std::wstring svc_type_name;
                unsigned long svc_start_id = 0;
                unsigned long svc_type_id = 0;

                const std::wstring svc_key_path(services_key_path + L"\\" + key_name);
                NX::win::reg_local_machine svckey;
                svckey.open(svc_key_path, NX::win::reg_key::reg_wow64_64, true);
                svckey.read_value(L"DisplayName", svc_displayname);
                svckey.read_value(L"ImagePath", svc_imagepath);
                svckey.read_value(L"Start", &svc_start_id);
                svckey.read_value(L"Type", &svc_type_id);

                switch (svc_start_id)
                {
                case 0:
                    svc_start_name = L"Boot";
                    break;
                case 1:
                    svc_start_name = L"System";
                    break;
                case 2:
                    svc_start_name = L"Auto";
                    break;
                case 3:
                    svc_start_name = L"Demand";
                    break;
                case 4:
                    svc_start_name = L"Disabled";
                    break;
                default:
                    svc_start_name = NX::string_formater(L"Unknown (%d)", svc_start_id);
                    break;
                }

                switch (svc_type_id)
                {
                case 1:
                    svc_type_name = L"Kernel driver";
                    break;
                case 2:
                    svc_type_name = L"File system driver";
                    break;
                case 4:
                    svc_type_name = L"Service adapter (Reserved)";
                    break;
                case 8:
                    svc_type_name = L"Recognizer driver";
                    break;
                case 16:
                    svc_type_name = L"Own process service";
                    break;
                case 32:
                    svc_type_name = L"Share process service";
                    break;
                default:
                    svc_type_name = NX::string_formater(L"Unknown (%d)", svc_type_id);
                    break;
                }

                NX::win::service_status svcstatus = NX::win::service_control::query_service_status(key_name);

                NX::json_value svc_object = NX::json_value::create_object(true);
                svc_object[L"Name"] = NX::json_value(svc_displayname);
                svc_object[L"Image"] = NX::json_value(svc_imagepath);
                svc_object[L"Type"] = NX::json_value(svc_type_name);
                svc_object[L"Start"] = NX::json_value(svc_start_name);
                if (svc_start_id == 4) {
                    svc_object[L"Status"] = NX::json_value(L"Disabled");
                }
                else {
                    if (svcstatus.empty()) {
                        svc_object[L"Status"] = NX::json_value(L"N/A");
                    }
                    else {
                        svc_object[L"Status"] = NX::json_value(svcstatus.status_name());
                    }
                }

                // put into map
                v[key_name] = svc_object;
            }
            catch (const std::exception& e) {
                UNREFERENCED_PARAMETER(e);
            }

        });
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}

void diagtool::report_software_info(NX::json_value& v)
{
#ifdef _WIN64
    NX::json_value software64 = NX::json_value::create_array();
    report_software_info2(software64, true);
    v[L"x64"] = software64;
    NX::json_value software32 = NX::json_value::create_array();
    report_software_info2(software32, false);
    v[L"x86"] = software32;
#else
    report_software_info2(v, false);
#endif
}

void diagtool::report_software_info2(NX::json_value& v, bool is_wow64)
{

    try {

        //
        NX::win::reg_local_machine uninstallkey;
        const std::wstring uninstall_key_path(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
#ifdef _WIN64
        uninstallkey.open(uninstall_key_path, is_wow64 ? NX::win::reg_key::reg_wow64_32 : NX::win::reg_key::reg_wow64_64, true);
#else
        uninstallkey.open(uninstall_key_path, NX::win::reg_key::reg_default, true);
#endif
        uninstallkey.enum_sub_keys([&](const wchar_t* key_name){

            try {

                std::wstring product_displayname;
                std::wstring product_displayversion;
                std::wstring product_installdate;
                std::wstring product_installlocation;
                std::wstring product_publisher;
                unsigned long product_language_id;

                const std::wstring product_key_path(uninstall_key_path + L"\\" + key_name);
                NX::win::reg_local_machine productkey;
                productkey.open(product_key_path, is_wow64 ? NX::win::reg_key::reg_wow64_32 : NX::win::reg_key::reg_wow64_64, true);
                productkey.read_value(L"DisplayName", product_displayname);
                productkey.read_value(L"DisplayVersion", product_displayversion);
                productkey.read_value(L"InstallDate", product_installdate);
                productkey.read_value(L"InstallLocation", product_installlocation);
                productkey.read_value(L"Publisher", product_publisher);
                productkey.read_value(L"Language", &product_language_id);

                NX::win::language product_language((unsigned short)product_language_id);
                NX::json_value product_object = NX::json_value::create_object(true);
                product_object[L"Name"] = NX::json_value(product_displayname);
                product_object[L"Publisher"] = NX::json_value(product_publisher);
                product_object[L"Version"] = NX::json_value(product_displayversion);
                product_object[L"Language"] = NX::json_value(product_language.name());
                product_object[L"InstallDate"] = NX::json_value(product_installdate);
                product_object[L"InstallLocation"] = NX::json_value(product_installlocation);
                // put into map
                v.as_array().push_back(product_object);
            }
            catch (const std::exception& e) {
                UNREFERENCED_PARAMETER(e);
            }

        });
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}
