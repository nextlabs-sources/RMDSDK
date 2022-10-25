

#include <Windows.h>
#include <assert.h>
#include <Wtsapi32.h>
#define SECURITY_WIN32
#include <security.h>
#include <Sspi.h>
#include <Softpub.h>
#include <Msi.h>

#include <string>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\filesys.hpp>
#include <nudf\winutil.hpp>
#include <nudf\string.hpp>
#include <nudf\dbglog.hpp>
#include <nudf\filesys.hpp>
#include <nudf\json.hpp>
#include <nudf\xml.hpp>
#include <nudf\resutil.hpp>
#include <nudf/shared/rightsdef.h>

#include "nxrmserv.hpp"
#include "nxrmdrvman.h"
#include "nxrmres.h"
#include "global.hpp"
#include "diag.hpp"
#include "serv.hpp"

#include <experimental/filesystem>
#include "registry_service_impl.h"
#include "app_whitelist_config.h"


DECLARE_GLOBAL_LOG_INSTANCE()
DECLARE_LOG_CALLBACKS()


rmserv* SERV = nullptr;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

typedef struct _SYSTEM_HANDLE
{
	DWORD       dwProcessId;
	BYTE		bObjectType;
	BYTE		bFlags;
	WORD		wValue;
	PVOID       pAddress;
	DWORD GrantedAccess;
}SYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	DWORD         dwCount;
	SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION, **PPSYSTEM_HANDLE_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemHandleInformation = 0X10,
} SYSTEM_INFORMATION_CLASS;

typedef NTSTATUS(WINAPI *PNtQuerySystemInformation)
(IN	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT	PVOID					 SystemInformation,
	IN	ULONG					 SystemInformationLength,
	OUT	PULONG					 ReturnLength OPTIONAL);

// This is the SHA-256 checksum of the shared secret for the security string.
static const std::vector<unsigned char> secret_sha256 = {
    0x70, 0x32, 0x13, 0x06, 0xFC, 0xE0, 0x09, 0x16,
    0x0E, 0x20, 0xA6, 0x6D, 0x33, 0x1F, 0x1C, 0x75,
    0xDF, 0xF1, 0xD3, 0xA0, 0xCF, 0x49, 0x41, 0xF3,
    0x97, 0xEA, 0x86, 0x7B, 0xAE, 0x0D, 0xF6, 0x12
};

void GenerateRandomId(WCHAR* str)
{
	int i = 0;
	WCHAR digits[] = L"0123456789ABCDEF";
	for (i = 0; i < 16; i++)
	{
		int r = std::rand();
		str[i] = digits[r % 16];
	}
	str[i] = 0;
}

rmserv::rmserv() : NX::win::service_instance(NXRMS_SERVICE_NAME), _ready(false), _server_mode(false)
{
	::InitializeCriticalSection(&_lock);
	::InitializeCriticalSection(&_pdplock);
    ::InitializeCriticalSection(&_rpmlock);
    assert(nullptr == SERV);
	SERV = this;
	_serv_conf.load();
	WCHAR deviceid[256];
	DWORD dwSize = sizeof(deviceid);
	if (!GetComputerNameExW(ComputerNameDnsFullyQualified, deviceid, &dwSize))
	{
		GenerateRandomId(deviceid);
	}
	_deviceid = deviceid;
}

rmserv::~rmserv()
{
    assert(nullptr != SERV);
    SERV = nullptr;
    ::DeleteCriticalSection(&_lock);
	::DeleteCriticalSection(&_pdplock);
    ::DeleteCriticalSection(&_rpmlock);
}

bool rmserv::initialize()
{
    //
    // the first thing is to initialize log system
    //
    init_log();
    
    return true;
}

void rmserv::clear()
{
    set_ready_flag(false);

    if (_serv_flt.is_running()) {
        _serv_flt.stop();
    }
    if (_serv_core.is_running()) {
        _serv_core.stop();
    }
}

std::wstring rmserv::get_dirs()
{
    ::EnterCriticalSection(&_rpmlock);
    std::wstring str;

	for (size_t i = 0; i < m_markDir.size(); i++)
	{
		str += std::get<0>(m_markDir[i]);
		str += std::get<1>(m_markDir[i]);
		str += std::get<2>(m_markDir[i]);
		str += std::get<3>(m_markDir[i]);
		str += L'<';
		str += std::get<4>(m_markDir[i]);
		str += L'<';
        str += std::get<5>(m_markDir[i]);
        str += L'<';
        str += std::get<6>(m_markDir[i]);
        str += L'<';
        str += std::get<7>(m_markDir[i]);
        str += L'<';
    }
    ::LeaveCriticalSection(&_rpmlock);

	return std::move(str);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
std::wstring rmserv::get_sanctuary_dirs()
{
	std::wstring str;

	for (size_t i = 0; i < m_sanctuaryDir.size(); i++)
	{
		str += m_sanctuaryDir[i].first;
		str += L'<';
		str += m_sanctuaryDir[i].second;
		str += L'<';
	}

	return std::move(str);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

void rmserv::parse_dirs(const std::wstring& str)
{
    ::EnterCriticalSection(&_rpmlock);
    
    std::wstringstream f(str);
	std::wstring s, s2, s_wid, s_rmsuid, s_option;
	m_markDir.clear();
	while (std::getline(f, s, L'<'))
	{
		if (s.size() < 3)
			continue;

		wchar_t ext = s[s.size() - 1];
		s.pop_back();
		wchar_t overwrite = s[s.size() - 1];
		s.pop_back();
		wchar_t autoProtect = s[s.size() - 1];
		s.pop_back();

		std::getline(f, s2, L'<');
        std::getline(f, s_wid, L'<');
        std::getline(f, s_rmsuid, L'<');
        std::getline(f, s_option, L'<');

		if (std::experimental::filesystem::exists(s) == false)
			continue;

		m_markDir.push_back(std::make_tuple(s, autoProtect, overwrite, ext, s2, s_wid, s_rmsuid, s_option));
	}
    
    ::LeaveCriticalSection(&_rpmlock);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
void rmserv::parse_sanctuary_dirs(const std::wstring& str)
{
	std::wstringstream f(str);
	std::wstring s, s2;
	m_sanctuaryDir.clear();
	while (std::getline(f, s, L'<'))
	{
		std::getline(f, s2, L'<');
		m_sanctuaryDir.push_back(std::make_pair(s, s2));
	}
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

void rmserv::InitMarkDirecory()
{
    ::EnterCriticalSection(&_rpmlock);
    for (size_t i = 0; i < m_markDir.size(); i++)
	{
		LOGDEBUG(NX::string_formater(L"InitMarkDirecory: dirs: %s", std::get<0>(m_markDir[i]).c_str()));

		if (!_serv_flt.insert_safe_dir(std::get<0>(m_markDir[i]) + std::get<2>(m_markDir[i]) + std::get<3>(m_markDir[i])))
			LOGERROR(NX::string_formater(L"insert_safe_dir failed,  %s", std::get<0>(m_markDir[i]).c_str()));
	}
    ::LeaveCriticalSection(&_rpmlock);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
void rmserv::InitSanctuaryDirecory()
{
	for (size_t i = 0; i < m_sanctuaryDir.size(); i++)
	{
		LOGDEBUG(NX::string_formater(L"InitSanctuaryDirecory: dirs: %s, fileTags: %s", m_sanctuaryDir[i].first.c_str(), m_sanctuaryDir[i].second.c_str()));

		if (!_serv_flt.insert_sanctuary_dir(m_sanctuaryDir[i].first))
			LOGERROR(NX::string_formater(L"insert_sanctuary_dir failed,  %s", m_sanctuaryDir[i].first.c_str()));
	}
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

std::string rmserv::process_request(unsigned long session_id, unsigned long process_id, const std::string& s)
{
    std::string response;

    try {
		
        NX::json_value request = NX::json_value::parse(s);

        const int request_code = request.as_object().at(L"code").as_int32();
#ifndef _DEBUG         
		LOGDEBUG(NX::string_formater(" process_request: %d ", request_code));	
#else
		LOGDEBUG(NX::string_formater(" process_request: %d, %s ", request_code, s.c_str()));
#endif

        switch (request_code)
        {
        case 1:
            response = on_request_query_general_status(session_id, process_id);
            break;
        case 2:
            response = on_request_query_product_info(session_id, process_id);
            break;
        case 3:
            response = on_request_query_rms_settings(session_id, process_id);
            break;
        case 4:
            response = on_request_verify_security(session_id, process_id, request.as_object().at(L"parameters"));
            break;
        case 5:
            response = on_request_query_rmc_settings(session_id, process_id);
            break;
        case 6:
            response = on_request_query_log_settings(session_id, process_id);
            break;
        case 7:
            response = on_request_query_login_urls(session_id, process_id);
            break;
        case 8:
            response = on_request_finalize_login(session_id, process_id, request.as_object().at(L"parameters"));
            break;
        case 9:
            response = on_request_query_login_status(session_id, process_id);
            break;
        case 10:
            response = on_request_check_profile_update(session_id, process_id);
            break;
        case 11:
            response = on_request_check_software_update(session_id, process_id);
            break;
        case 12:
            response = on_request_set_log_settings(session_id, process_id, request.as_object().at(L"parameters").as_object());
            break;
        case 13:
            response = on_request_set_default_membership(session_id, process_id, request.as_object().at(L"parameters").as_object());
            break;
        case 14:
            response = on_request_set_default_security_mode(session_id, process_id, request.as_object().at(L"parameters").as_object().at(L"securityMode").as_int32());
            break;
        case 15:
            response = on_request_register_nodification_handler(session_id, process_id, (HWND)(ULONG_PTR)request.as_object().at(L"parameters").as_object().at(L"WindowHandle").as_uint32());
            break;
        case 16:
            response = on_request_log_sharing_transaction(session_id, process_id, request.as_object().at(L"parameters"));
            break;
        case 17:
            response = on_request_logout(session_id, process_id);
            break;
        case 18:
            response = on_request_collect_debugdump(session_id, process_id);
            break;
        case 19:
            response = on_request_activity_notify(session_id, process_id, request.as_object().at(L"parameters"));
            break;
        case 20:
            response = on_request_set_dwm_status(session_id, process_id, request.as_object().at(L"parameters"));
            break;
        case 21:
            response = on_request_query_activity_records(session_id, process_id, request.as_object().at(L"parameters"));
            break;
		case 22:
			response = on_request_set_service_stop_no_security(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 23:
			response = on_request_insert_directory(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 24:
			response = on_request_remove_directory(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 25:
			response = on_request_set_clientid(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 26:
			response = on_request_stop_service_no_security(session_id, process_id);
			break;
		case 27:
			response = on_request_set_policy_bundle(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 28:
			response = on_request_set_service_stop(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 29:
			response = on_request_set_user_attr(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 30:
			response = on_request_set_cached_token(session_id, process_id, request.as_object().at(L"parameters"));
			break;  
		case 31:
			response = on_request_delete_cached_token(session_id, process_id);
			break;
		case 32:
			response = on_request_delete_file_token(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 33:
			response = on_request_get_secret_dir(session_id, process_id);
			break;
		case 34:
			response = on_request_set_router(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 35:
			response = on_request_register_app(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 36:
			response = on_request_unregister_app(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 37:
			response = on_request_notify_rmx_status(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 38:
			response = on_request_add_trusted_process(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 39:
			response = on_request_remove_trusted_process(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 40:
			response = on_request_rpm_directory(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 41:
			response = on_request_user_info(session_id, process_id);
			break;
		case 42:
			response = on_request_delete_file(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 43:
			response = on_request_get_rights(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 44:
			response = on_request_get_file_status(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 45:
			response = on_request_set_app_registry(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 46:
			response = on_request_launch_pdp(session_id, process_id);
			break;
		case 47:
			response = on_request_copy_file(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 48:
			response = on_request_is_app_registered(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 49:
			response = on_request_get_protected_profiles_dir(session_id, process_id);
			break;
		case 50:
			response = on_request_pop_new_token(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 51:
			response = on_request_find_file_token(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 52:
			response = on_request_remove_cached_token(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 53:
			response = on_request_add_activity_log(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 54:
			response = on_request_add_metadata(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 55:
			response = on_request_login(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 56:
			response = on_request_logout(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 57:
			response = on_request_notify_message(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 58:
			response = on_request_read_file_tags(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 59:
			response = on_request_delete_folder(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 60:
			response = on_request_add_trusted_app(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 61:
			response = on_request_remove_trusted_app(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 62:
			response = on_request_lock_localfile(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 63:
			response = on_request_run_registry_command(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 64:
			response = on_request_opened_file_rights(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 65:
			response = on_request_set_file_attributes(session_id, process_id, request.as_object().at(L"parameters"));
			break;
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		case 66:
			response = on_request_insert_sanctuary_directory(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 67:
			response = on_request_remove_sanctuary_directory(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		case 68:
			response = on_request_is_sanctuary_directory(session_id, process_id, request.as_object().at(L"parameters"));
			break;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
        case 69:
            response = on_request_get_file_attributes(session_id, process_id, request.as_object().at(L"parameters"));
            break;
        case 70:
            response = on_request_windows_encrypt_file(session_id, process_id, request.as_object().at(L"parameters"));
            break;
        case 71:
            response = on_request_get_rpm_directory(session_id, process_id, request.as_object().at(L"parameters"));
            break;
		case 72:
			response = on_request_query_apiuser(session_id, process_id, request.as_object().at(L"parameters"));
			break;
		default:
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_FUNCTION, "Unknown request code");
            break;
        }
    }
    catch (const NX::json_exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = "{\"code\":87, \"message\":\"Unknown request data\"}";
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        int le = GetLastError();
        if (0 == le) {
            le = ERROR_UNKNOWN_ERROR;
        }
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", le, e.what());
    }


	LOGDEBUG(NX::string_formater(L"Service Response: %S", response.c_str()));

    return std::move(response);
}

BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		return FALSE;
	}

	return TRUE;
}

void PromotePermission()
{
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hToken;
	if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken))
	{
		SetPrivilege(hToken, L"SeTcbPrivilege", TRUE);
		SetPrivilege(hToken, L"SeImpersonatePrivilege", TRUE);
		SetPrivilege(hToken, L"SeEnableDelegationPrivilege", TRUE);
		SetPrivilege(hToken, L"SeDelegateSessionUserImpersonatePrivilege", TRUE);
		CloseHandle(hToken);
	}
	CloseHandle(hProcess);
}
void rmserv::on_start(int argc, const wchar_t** argv)
{
    LOGINFO(L" ");
    LOGINFO(L" ");
    LOGINFO(L" ");
    LOGINFO(L"***********************************************************");
    LOGINFO(L"*                 NextLabs Rights Management              *");
    LOGINFO(L"***********************************************************");
    LOGINFO(L" ");

    if (_serv_conf.get_delay_seconds() != 0) {
        LOGINFO(NX::string_formater(L"Delay %d seconds ...", _serv_conf.get_delay_seconds()));
        Sleep(1000 * _serv_conf.get_delay_seconds());
    }
    //
    // initialize global data
    //
    LOGDEBUG(L"Initializing SSPI ...");
    initialize_sspi();
    LOGDEBUG(L"  -> Done");
    LOGDEBUG(L"Initializing Global varibales ...");
    GLOBAL.initialize();
    LOGDEBUG(L"  -> Done");
    
    // init keys: master key, etc.
    LOGDEBUG(L"Initializing keys ...");
    init_keys();
    LOGDEBUG(L"  -> Done");

	nx::capp_whiltelist_config::getInstance()->check_register_file();

	// init router config information
    const std::wstring router_conf_file(GLOBAL.get_config_dir() + L"\\router.json");
    const std::wstring register_xmlfile(GLOBAL.get_config_dir() + L"\\register.xml");
    if (NX::fs::exists(router_conf_file)) {
        LOGDEBUG(NX::string_formater(L"load router_conf_file for router config: %s", router_conf_file.c_str()));
        _router_conf.load_from_file(router_conf_file);
    }
    else if (NX::fs::exists(register_xmlfile)) {
        LOGDEBUG(NX::string_formater(L"load register_xmlfile for router config: %s", register_xmlfile.c_str()));
        _router_conf.load_from_xmlfile(register_xmlfile);
    }
    else {
        LOGDEBUG(L"Router config file doesn't exist, use default router and tenant");
    }
    LOGDEBUG(L"ROUTER INFORMATION");
    LOGDEBUG(NX::string_formater(L"  -> router: %s", _router_conf.get_router().c_str()));
    LOGDEBUG(NX::string_formater(L"  -> tenant: %s", _router_conf.get_tenant_id().c_str()));

	// init service config information
    if (NX::fs::exists(register_xmlfile)) {
        LOGDEBUG(NX::string_formater(L"load register_xmlfile for service config: %s", register_xmlfile.c_str()));
        _serv_conf.load_from_xmlfile(register_xmlfile);
    }

    // Log initialize information
    log_init_info();

    // emulate browser
    emulate_browser();

	if (_serv_conf.get_no_vhd() == false)
	{
		// Init VHD
		LOGDEBUG(L"init_vhd");
		if (!init_vhd()) {
			LOGDEBUG(L"driver nxrmvhd initialization failed");
			throw std::exception("driver nxrmvhd initialization failed");
		}
	}

    // Start NXRM drivers
    //   nxrmdrv.sys
	LOGDEBUG(L"init_drvcore");
    if (!init_drvcore()) {
		LOGDEBUG(L"driver nxrmdrv initialization failed");
        throw std::exception("driver nxrmdrv initialization failed");
    }
    //   nxrmflt.sys
	LOGDEBUG(L"init_drvflt");
    if (!init_drvflt()) {
		LOGDEBUG(L"driver nxrmflt initialization failed");
        throw std::exception("driver nxrmflt initialization failed");
    }

    // Init core context
	LOGDEBUG(L"init_core_context_cache");
    init_core_context_cache();

	// ready to handle request
    set_ready_flag(true);

    // Collect all the existing session id
    const std::vector<unsigned long>& existing_session_list = winsession_manager::find_existing_session();
    if (GLOBAL_LOG_ACCEPT(NX::dbg::LL_DEBUG)) {
        std::wstring str_existing_session;
        std::for_each(existing_session_list.begin(), existing_session_list.end(), [&](const unsigned long& session_id) {
            if (!str_existing_session.empty()) {
                str_existing_session += L", ";
            }
            str_existing_session += std::to_wstring((int)session_id);
        });
        LOGDEBUG(NX::string_formater(L"Existing Session: %s", str_existing_session.c_str()));
    }

    // Add existing sessions to session_manager
    std::for_each(existing_session_list.begin(), existing_session_list.end(), [&](const unsigned long& session_id) {
        _win_session_manager.add_session(session_id);
    });

	//ReadDirectoryRegKey();

    // Figure out PDP dir (e.g. "C:\P\N\Policy Controller") from path of
    // nxrmserv.exe (e.g. "C:\P\N\SkyDRM\RPM\bin\nxrmserv.exe").
    const process_record proc_record(GetCurrentProcessId());
    std::wstring image_path = proc_record.get_image_path();
    size_t pos;
    pos = image_path.rfind(L'\\');
    pos = image_path.rfind(L'\\', pos - 1);
	pos = image_path.rfind(L'\\', pos - 1);
	pos = image_path.rfind(L'\\', pos - 1);
    image_path.erase(pos + 1, std::wstring::npos);
    m_pdpDir = image_path + L"Policy Controller";

    InitializePDP();

	_ipc_server.listen(L"nxrmservice");

	// login with API user for service mode
	if (_serv_conf.get_auth_type() == 1)
	{
		// server mode with API user
		init_server_mode();
	}
}

void rmserv::init_server_mode()
{
	_win_session_manager.add_session(0); // session "0" for server mode
	std::shared_ptr<winsession> sp = _win_session_manager.get_session(0);
	sp->get_rm_session().ensure_rs_executor(true);
	NX::json_value login_result;
	if (sp->get_rm_session().get_rs_executor()->request_login_apiuser(
		_serv_conf.get_api_app_id(),
		_serv_conf.get_api_app_key(),
		_serv_conf.get_api_email(),
		_serv_conf.get_api_private_cert(),
		login_result
	)) {
		_server_mode = true;
		// login API user OK
		on_request_finalize_login(0, 0, login_result);

		// update client config
		std::wstring router = SERV->get_router_config().get_router();
		sp->get_rm_session().update_client_dir_config(router, LOGIN_INFO::routerAddr);
		std::wstring tenant = SERV->get_router_config().get_tenant_id();
		sp->get_rm_session().update_client_dir_config(tenant, LOGIN_INFO::tenantId);
		sp->get_rm_session().update_client_dir_config(_serv_conf.get_api_working_folder(), LOGIN_INFO::sdklibFolder);
		sp->get_rm_session().update_client_dir_config(_serv_conf.get_api_working_folder(), LOGIN_INFO::workingFolder);
		sp->get_rm_session().update_client_dir_config(_serv_conf.get_api_working_folder(), LOGIN_INFO::tempFolder);

		_server_login_data = NX::conversion::utf16_to_utf8(login_result.serialize());
        LOGDEBUG(NX::string_formater(L"init_server_mode successfully!"));
    }
	else
	{
		// login API user failed for server mode
		_win_session_manager.remove_session(0); 
        LOGDEBUG(NX::string_formater(L"init_server_mode failed."));
    }
}

void rmserv::InitializePDP()
{
	if (m_pdpDir.size() < 2)
		return;
	SDWLResult res = m_PDP.Initialize(m_pdpDir);
	LOGDEBUG(NX::string_formater(L"PDP Initialize: %s, status: %d, %hs", m_pdpDir.c_str(), res.GetCode(), res.GetMsg().c_str()));
}

void rmserv::on_stop() noexcept
{
    // clear ready flag
    set_ready_flag(false);

    // Shutdown session manager
    _win_session_manager.clear();

    // clean up VHDs
    _vhd_manager.clear();

    _serv_core.stop();
    _serv_flt.stop();

	_ipc_server.shutdown();
    //
    // the last thing: shutdown log system
    //
    GLOBAL_LOG_SHUTDOWN();
}


void rmserv::on_pause()
{
}

void rmserv::on_resume()
{
}

void rmserv::on_preshutdown() noexcept
{
    _win_session_manager.clear();
}

void rmserv::on_shutdown() noexcept
{
}

long rmserv::on_session_logon(_In_ WTSSESSION_NOTIFICATION* wtsn) noexcept
{
    if (wtsn->dwSessionId != 0 && wtsn->dwSessionId != 0xFFFFFFFF && is_service_ready()) {
        _win_session_manager.add_session(wtsn->dwSessionId);

		if (is_server_mode() == true)
		{
			std::shared_ptr<winsession> sp = _win_session_manager.get_session(0);
			get_fltserv().on_user_logon(wtsn->dwSessionId, sp->get_rm_session().get_profile().get_preferences().get_default_member());
		}
    }

    return 0;
}

long rmserv::on_session_logoff(_In_ WTSSESSION_NOTIFICATION* wtsn) noexcept
{
    if (wtsn->dwSessionId != 0 && wtsn->dwSessionId != 0xFFFFFFFF && is_service_ready()) {
        _win_session_manager.remove_session(wtsn->dwSessionId);
    }

    return 0;
}

std::wstring rmserv::get_config_dir()
{
	if (get_service_conf().get_no_vhd())
	{
		return GLOBAL.get_config_dir();
	} 
	else
	{
		return get_vhd_manager().get_config_volume().get_config_dir();
	}
}

std::wstring rmserv::get_profiles_dir()
{
	if (get_service_conf().get_no_vhd())
	{
		return GLOBAL.get_profiles_dir();
	} else
	{
		return get_vhd_manager().get_config_volume().get_profiles_dir();
	}
}

std::string rmserv::on_request_query_general_status(unsigned long session_id, unsigned long process_id)
{
    static NX::win::system_default_language syslang;
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

#ifdef _DEBUG
    static const wchar_t* releaseType = L"Debug";
#else
    static const wchar_t* releaseType = L"Release";
#endif

#ifdef _WIN64
    static const wchar_t* cpuArch = L"x64";
#else
    static const wchar_t* cpuArch = L"x86";
#endif

    try {

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data", NX::json_value::create_object())
        }), false);
        
        NX::json_object& data_object = v[L"data"].as_object();


        data_object[L"os"] = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"name", NX::json_value(GLOBAL.get_windows_info().windows_name())),
            std::pair<std::wstring, NX::json_value>(L"version", NX::json_value(GLOBAL.get_windows_info().windows_version())),
            std::pair<std::wstring, NX::json_value>(L"build", NX::json_value((int)GLOBAL.get_windows_info().build_number())),
            std::pair<std::wstring, NX::json_value>(L"type", NX::json_value(GLOBAL.get_windows_info().is_processor_x86() ? L"x86" : L"x64")),
            std::pair<std::wstring, NX::json_value>(L"language", NX::json_value(syslang.name()))
        }), false);

        data_object[L"product"] = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"name", NX::json_value(GLOBAL.get_product_name())),
            std::pair<std::wstring, NX::json_value>(L"version", NX::json_value(GLOBAL.get_product_version_string())),
            std::pair<std::wstring, NX::json_value>(L"releaseType", NX::json_value(releaseType)),
            std::pair<std::wstring, NX::json_value>(L"cpuarch", NX::json_value(cpuArch)),
            std::pair<std::wstring, NX::json_value>(L"directory", NX::json_value(GLOBAL.get_product_dir()))
        }), false);

        data_object[L"rmsSettings"] = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"serverUrl", NX::json_value(sp->get_rm_session().get_rms_url())),
            std::pair<std::wstring, NX::json_value>(L"tenantId", NX::json_value(sp->get_rm_session().get_tenant_id()))
        }), false);

        data_object[L"logSettings"] = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"logLevel", NX::json_value((int)_serv_conf.get_log_level())),
            std::pair<std::wstring, NX::json_value>(L"logRotation", NX::json_value((int)_serv_conf.get_log_rotation())),
            std::pair<std::wstring, NX::json_value>(L"logSize", NX::json_value((int)_serv_conf.get_log_size()))
        }), false);

        const bool logonStatus = sp->get_rm_session().is_logged_on();
        const std::wstring expire_time = logonStatus ? sp->get_rm_session().get_profile().get_token().get_expire_time().serialize(true, false) : L"";
        data_object[L"logonStatus"] = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"status", NX::json_value(logonStatus)),
            std::pair<std::wstring, NX::json_value>(L"expireTime", NX::json_value(expire_time)),
            std::pair<std::wstring, NX::json_value>(L"logonUser", NX::json_value::create_object())
        }), false);

        if (logonStatus) {
            NX::json_object& user_object = data_object[L"logonStatus"].as_object().at(L"logonUser").as_object();
            user_object[L"id"] = NX::json_value(sp->get_rm_session().get_profile().get_id());
            user_object[L"name"] = NX::json_value(sp->get_rm_session().get_profile().get_name());
            user_object[L"email"] = NX::json_value(sp->get_rm_session().get_profile().get_email());
            user_object[L"defaultMember"] = NX::json_value(sp->get_rm_session().get_profile().get_preferences().get_default_member());
            user_object[L"defaultTenant"] = NX::json_value(sp->get_rm_session().get_profile().get_preferences().get_default_tenant());
            user_object[L"securityMode"] = NX::json_value(sp->get_rm_session().get_profile().get_preferences().get_secure_mode());


            std::shared_ptr<policy_bundle> sp_bundle = sp->get_rm_session().get_policy_bundle();
            if (sp_bundle != nullptr) {
                sp_bundle->get_issue_time();
            }
            const NX::time::datetime_special_1 tm_last_update(sp->get_rm_session().get_local_config().get_last_update_time());
			const NX::time::datetime_special_1 tm_last_heartbeat_time(sp->get_rm_session().get_local_config().get_last_heartbeat_time());
            const std::wstring s_policy_time = (sp_bundle == nullptr) ? L"Ad hoc" : sp_bundle->get_issue_time();
			const std::wstring s_last_update = tm_last_update.empty() ? (tm_last_heartbeat_time.empty() ? L"N/A" : tm_last_heartbeat_time.serialize(true)) : tm_last_update.serialize(true);
            data_object[L"policyStatus"] = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                std::pair<std::wstring, NX::json_value>(L"policyId", NX::json_value(s_policy_time)),
                std::pair<std::wstring, NX::json_value>(L"lastUpdateTime", NX::json_value(s_last_update))
            }), false);
        }

        const std::wstring& ws = v.serialize();
        response = NX::conversion::utf16_to_utf8(ws);
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_query_product_info(unsigned long session_id, unsigned long process_id)
{
    std::string response;

#ifdef _DEBUG
    static const wchar_t* releaseType = L"Debug";
#else
    static const wchar_t* releaseType = L"Release";
#endif

#ifdef _WIN64
    static const wchar_t* cpuArch = L"x64";
#else
    static const wchar_t* cpuArch = L"x86";
#endif

    try {

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data", 
                NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                    std::pair<std::wstring, NX::json_value>(L"name", NX::json_value(GLOBAL.get_product_name())),
                    std::pair<std::wstring, NX::json_value>(L"version", NX::json_value(GLOBAL.get_product_version_string())),
                    std::pair<std::wstring, NX::json_value>(L"releaseType", NX::json_value(releaseType)),
                    std::pair<std::wstring, NX::json_value>(L"cpuArch", NX::json_value(cpuArch)),
                    std::pair<std::wstring, NX::json_value>(L"directory", NX::json_value(GLOBAL.get_product_dir()))
                }), false))
        }), false);

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }
    
    return std::move(response);
}

std::string rmserv::on_request_query_rms_settings(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    try {

        int error_code = 0;
        std::wstring error_message(L"succeed");
        std::wstring tenant_id;
        std::wstring rms_url;

        std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
        if (sp != nullptr) {
            tenant_id = sp->get_rm_session().get_tenant_id();
            rms_url = sp->get_rm_session().get_rms_url();
            if (tenant_id.empty() || rms_url.empty()) {
                error_code = ERROR_INVALID_DATA;
                error_message = NX::string_formater(L"Invalid tenant (%s) or RMS URL (%s)", tenant_id.c_str(), rms_url.c_str());
            }
        }
        else {
            error_code = ERROR_NO_SUCH_LOGON_SESSION;
            error_message = L"Session not found";
        }

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value(error_code)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(error_message)),
            std::pair<std::wstring, NX::json_value>(L"data", 
                NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                    std::pair<std::wstring, NX::json_value>(L"tenantId", NX::json_value(tenant_id)),
                    std::pair<std::wstring, NX::json_value>(L"serverUrl", NX::json_value(rms_url))
                }), false))
        }), false);

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_verify_security(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;

    try {
        if (VerifySecurityString(parameters, response))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
        }
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    LOGDEBUG(NX::string_formater(" on_request_verify_security: response: %s", response.c_str()));
    return std::move(response);
}

std::string rmserv::on_request_query_rmc_settings(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {

        const bool logonStatus = sp->get_rm_session().is_logged_on();
        const int default_securityMode = sp->get_rm_session().get_profile().get_preferences().get_secure_mode();

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data",
            NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                std::pair<std::wstring, NX::json_value>(L"protectionEnabled", NX::json_value(true)),
                std::pair<std::wstring, NX::json_value>(L"defaultSecurityMode", NX::json_value(default_securityMode)),
                std::pair<std::wstring, NX::json_value>(L"fileTypeBlacklist", NX::json_value(GLOBAL.get_filetype_blacklist())),
                std::pair<std::wstring, NX::json_value>(L"fileTypeWhitelist", NX::json_value(sp->get_rm_session().get_client_config().get_protection_filter()))
            }), false))
        }), false);

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_query_log_settings(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data",
            NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                std::pair<std::wstring, NX::json_value>(L"logonUrl", NX::json_value((int)_serv_conf.get_log_level())),
                std::pair<std::wstring, NX::json_value>(L"logRotation", NX::json_value((int)_serv_conf.get_log_rotation())),
                std::pair<std::wstring, NX::json_value>(L"logSize", NX::json_value((int)_serv_conf.get_log_size()))
            }), false))
        }), false);

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_query_login_urls(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {

        std::wstring url(sp->get_rm_session().get_rms_url());
        if (boost::algorithm::iends_with(url, L"/rs/")) {
            url = url.substr(0, url.length() - 3);
        }
		LOGDEBUG(NX::string_formater(L" on_request_query_login_urls: login URL (%s)", url.c_str()));

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data",
            NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                std::pair<std::wstring, NX::json_value>(L"logonUrl", NX::json_value(url))
            }), false))
        }), false);

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

	LOGDEBUG(NX::string_formater(" on_request_query_login_urls: response: %s", response.c_str()));
    return std::move(response);
}

bool rmserv::VerifySecurityString(const NX::json_value& parameters, std::string &failResponse)
{
    std::string secret;

    if (!parameters.as_object().has_field(L"security"))
    {
        failResponse = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "security string not found");
        return false;
    }

    secret = NX::conversion::utf16_to_utf8(parameters.as_object().at(L"security").as_string());

    std::vector<unsigned char> sha256_result;
    sha256_result.resize(256 / CHAR_BIT, 0);
    if (!NX::crypto::sha256((const unsigned char *) secret.data(), (ULONG)secret.size(), sha256_result.data()))
    {
        DWORD lastErr = GetLastError();
        LOGERROR(NX::string_formater(L"Failed while hashing secret, lastErr=%08X", lastErr));
        failResponse = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", lastErr, "Failed while hashing secret");
        return false;
    }

    if (sha256_result != secret_sha256)
    {
        failResponse = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_ACCESS_DENIED, "security string not match");
        return false;
    }

    return true;
}

std::string rmserv::on_request_finalize_login(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }
	if (is_server_mode() && session_id != 0)
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "Machine is under server mode");
		return std::move(response);
	}

    try {
		if (session_id != 0 && !VerifySecurityString(parameters, response))
		{
			return std::move(response);
		}

		// logout if user is still login
		std::string  rets = on_request_logout(session_id, process_id);
		NX::json_value request = NX::json_value::parse(rets);
		const int request_code = request.as_object().at(L"code").as_int32();
		if (rets.find("succeed") == std::string::npos)
		{
			if (request_code != ERROR_LOGON_NOT_GRANTED)
				return rets;
		}			

		// Check whether there is a user logged in
		//  1. if no user is logged in, excute login operation.
		//  2. if there is a user logged in, check whether the logged-in server is consisitent with currenlty login server
		//      1. if they are consistent, excute login operation.
		//      2. if they are inconsistent, return an error.
		bool canlogon = true;
		std::wstring haslogon_url = L"";
		std::vector<unsigned long> ids = _win_session_manager.get_all_session_id();
		for (int i = 0; i < ids.size(); i++)
		{
			if (ids[i] != session_id)
			{
				std::shared_ptr<winsession> _spwin = _win_session_manager.get_session(ids[i]);
				if (_spwin != nullptr)
				{
					if (_spwin->get_rm_session().is_logged_on())
					{
						haslogon_url = _spwin->get_rm_session().get_rms_url();
						break;
					}
				}
			}
		}

		if (!haslogon_url.empty())
		{
			const NX::json_object& logon_data = parameters.as_object();
			std::wstring tenant_url = logon_data.at(L"defaultTenantUrl").as_string();
			if (haslogon_url.compare(tenant_url) != 0)
			{
				canlogon = false;
			}
		}
		
		if (!canlogon)
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 101, "Logon failed");
			return std::move(response);
		}

		// for switch server
		//SERV->get_router_config().ReadRegKey();
		sp->get_rm_session().initialize();

		if (!sp->get_rm_session().logon(parameters)) {
            int le = GetLastError();
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "Logon failed");
			LOGDEBUG(NX::string_formater(" process_request on_request_finalize_login: failed, response (%s)", response.c_str()));
		}
        else {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
			LOGDEBUG(NX::string_formater(" process_request on_request_finalize_login: response (%s)", response.c_str()));

			// successfully login
			std::vector<std::pair<std::wstring, std::wstring>> cmds = sp->get_rm_session().get_callback_cmds();
			for(size_t i = 0; i < cmds.size(); i++)
			{
				sp->execute(cmds[i].first, cmds[i].second);
				LOGDEBUG(NX::string_formater(" login successfully, run program (%s)", cmds[i].first.c_str()));
			}
        }
		sp->get_rm_session().clear_callback_cmds();
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

	LOGDEBUG(NX::string_formater(" on_request_finalize_login: response: %s", response.c_str()));
    return std::move(response);
}

std::string rmserv::on_request_set_user_attr(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	if (is_server_mode())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "Machine is under server mode");
		return std::move(response);
	}
	try {
		if (!sp->get_rm_session().on_set_user_attributes(parameters)) {
			int le = GetLastError();
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "Logon failed");
			//LOGDEBUG(NX::string_formater(" process_request on_request_set_user_attr: failed, response (%s)", response.c_str()));
		}
		else {
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
			//LOGDEBUG(NX::string_formater(" process_request on_request_set_user_attr: response (%s)", response.c_str()));
		}

	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_set_user_attr: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_pop_new_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		encrypt_token token;
		if (parameters.as_object().has_field(L"membershipid"))
		{
			std::wstring membershipid = parameters.as_object().at(L"membershipid").as_string();
			token = sp->get_rm_session().popup_new_token(membershipid);
		}

		if (token.empty()) {
			int le = GetLastError();
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "acquire cached token failed");
			LOGDEBUG(NX::string_formater(" process_request on_request_pop_new_token: failed, response (%s)", response.c_str()));
		}
		else {
			NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

				std::pair<std::wstring, NX::json_value>(L"code", NX::json_value(0)),
				std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
				std::pair<std::wstring, NX::json_value>(L"token",
				NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
						std::pair<std::wstring, NX::json_value>(L"id", NX::json_value(NX::conversion::to_wstring(token.get_token_id()))),
						std::pair<std::wstring, NX::json_value>(L"value", NX::json_value(NX::conversion::to_wstring(token.get_token_value()))),
						std::pair<std::wstring, NX::json_value>(L"otp", NX::json_value(NX::conversion::to_wstring(token.get_token_otp())))
					}), false))
				}), false);

			response = NX::conversion::utf16_to_utf8(v.serialize());
			// LOGDEBUG(NX::string_formater(" process_request on_request_pop_new_token: response (%s)", response.c_str()));
		}

	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	// LOGDEBUG(NX::string_formater(" on_request_pop_new_token: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_set_cached_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters) 
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		if (!sp->get_rm_session().insert_cached_token(parameters)) {
			int le = GetLastError();
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "set cached token failed");
			LOGDEBUG(NX::string_formater(" process_request on_request_set_cached_token: failed, response (%s)", response.c_str()));
		}
		else {
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
			LOGDEBUG(NX::string_formater(" process_request on_request_set_cached_token: response (%s)", response.c_str()));
		}

	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_set_cached_token: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_delete_cached_token(unsigned long session_id, unsigned long process_id) 
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		if (!sp->get_rm_session().delete_user_cached_tokens()) {
			int le = GetLastError();
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "delete cached token failed");
			LOGDEBUG(NX::string_formater(" process_request on_request_delete_cached_token: failed, response (%s)", response.c_str()));
		}
		else {
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
			LOGDEBUG(NX::string_formater(" process_request on_request_delete_cached_token: response (%s)", response.c_str()));
		}

	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_delete_cached_token: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_delete_file_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		std::wstring filename;
		unsigned long ret = 0;
		std::string  msg = "Delete file token failed";
		if (parameters.as_object().has_field(L"filePath"))
		{
			filename = parameters.as_object().at(L"filePath").as_string();
			ret = sp->get_rm_session().request_delete_file_token(filename);
			if (ret)
			{
				if (ret == ERROR_NOT_FOUND)
					msg = "File not found";
				else if (ret == ERROR_INVALID_DATA)
					msg = "File is not valid";
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ret, msg.c_str());
				LOGDEBUG(NX::string_formater("  on_request_delete_file_token: failed, response (%s)", response.c_str()));
			}
			else {
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
			}
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: no filePath data");
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_delete_file_token: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_find_file_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		encrypt_token token;
		time_t ttl = 0;
		if (parameters.as_object().has_field(L"duid"))
		{
			std::wstring duid = parameters.as_object().at(L"duid").as_string();
			token = sp->get_rm_session().find_file_token(duid, ttl);
		}

		if (token.empty()) {
			int le = GetLastError();
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "find token failed");
			LOGDEBUG(NX::string_formater(" process_request on_request_find_token: failed, response (%s)", response.c_str()));
		}
		else {
			NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

				std::pair<std::wstring, NX::json_value>(L"code", NX::json_value(0)),
				std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
				std::pair<std::wstring, NX::json_value>(L"token",
				NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
						std::pair<std::wstring, NX::json_value>(L"id", NX::json_value(NX::conversion::to_wstring(token.get_token_id()))),
						std::pair<std::wstring, NX::json_value>(L"value", NX::json_value(NX::conversion::to_wstring(token.get_token_value()))),
						std::pair<std::wstring, NX::json_value>(L"otp", NX::json_value(NX::conversion::to_wstring(token.get_token_otp()))),
						std::pair<std::wstring, NX::json_value>(L"ttl", NX::json_value(ttl))
					}), false))
				}), false);

			response = NX::conversion::utf16_to_utf8(v.serialize());
			// LOGDEBUG(NX::string_formater(" process_request on_request_find_token: response (%s)", response.c_str()));
		}

	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	// LOGDEBUG(NX::string_formater(" on_request_find_token: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_remove_cached_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		bool result = false;
		if (parameters.as_object().has_field(L"duid"))
		{
			std::wstring duid = parameters.as_object().at(L"duid").as_string();
			result = sp->get_rm_session().delete_file_token(duid, 0);
		}

		if (result == false) {
			int le = GetLastError();
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "remove cached token failed");
			LOGDEBUG(NX::string_formater(" process_request on_request_remove_cached_token: failed, response (%s)", response.c_str()));
		}
		else {
			NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

				std::pair<std::wstring, NX::json_value>(L"code", NX::json_value(0)),
				std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed"))
				}), false);

			response = NX::conversion::utf16_to_utf8(v.serialize());
			LOGDEBUG(NX::string_formater(" process_request on_request_remove_cached_token: response (%s)", response.c_str()));
		}

	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_remove_cached_token: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_add_activity_log(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
    try {
        NX::win::session_token st(session_id);
        NX::win::impersonate_object impersonobj(st);

        sp->get_rm_session().request_add_activity_log(parameters);
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_add_activity_log: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_add_metadata(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    LOGDEBUG(NX::string_formater(" process_request on_request_add_metadata."));
    
    std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {

		if (sp->get_rm_session().request_add_metadata(parameters))
		    response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
        else
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "failed to sync metadata");
    }
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_add_metadata: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_lock_localfile(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {

		sp->get_rm_session().request_lock_localfile(parameters);
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_lock_localfile: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_query_login_status(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {

        std::wstring url(sp->get_rm_session().get_rms_url());
        if (boost::algorithm::iends_with(url, L"/rs/")) {
            url = url.substr(0, url.length() - 3);
        }

        const bool logonStatus = sp->get_rm_session().is_logged_on();
        const std::wstring expire_time = logonStatus ? sp->get_rm_session().get_profile().get_token().get_expire_time().serialize(true) : L"";

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data",
                NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                    std::pair<std::wstring, NX::json_value>(L"status", NX::json_value(logonStatus)),
                    std::pair<std::wstring, NX::json_value>(L"expireTime", NX::json_value(expire_time)),
                    std::pair<std::wstring, NX::json_value>(L"logonUser", NX::json_value::create_object())
                }), false))
        }), false);

        if (logonStatus) {

            NX::json_object& data_object = v.as_object().at(L"data").as_object();
            NX::json_object& user_object = data_object.at(L"logonUser").as_object();

            user_object[L"id"] = NX::json_value(sp->get_rm_session().get_profile().get_id());
            user_object[L"name"] = NX::json_value(sp->get_rm_session().get_profile().get_name());
            user_object[L"email"] = NX::json_value(sp->get_rm_session().get_profile().get_name());
            user_object[L"preferences"] = NX::json_value::create_object();
            user_object[L"memberships"] = NX::json_value::create_array();

            NX::json_object& preferences = user_object[L"preferences"].as_object();
            preferences[L"defaultTenant"] = NX::json_value(sp->get_rm_session().get_profile().get_preferences().get_default_tenant());
            preferences[L"defaultMember"] = NX::json_value(sp->get_rm_session().get_profile().get_preferences().get_default_member());
            preferences[L"securityMode"] = NX::json_value(sp->get_rm_session().get_profile().get_preferences().get_secure_mode());

            //NX::json_array& memberships = user_object[L"memberships"].as_array();
            //const std::map<std::wstring, user_membership>& memberships_map = sp->get_rm_session().get_profile().get_memberships();
            //std::for_each(memberships_map.begin(), memberships_map.end(), [&](const std::pair<std::wstring, user_membership>& item) {
            //    NX::json_value membership_object = NX::json_value::create_object();
            //    membership_object.as_object()[L"id"] = NX::json_value(item.second.get_id());
            //    membership_object.as_object()[L"name"] = NX::json_value(item.second.get_name());
            //    membership_object.as_object()[L"type"] = NX::json_value(item.second.get_type());
            //    membership_object.as_object()[L"tenantId"] = NX::json_value(item.second.get_tenant_id());
            //    memberships.push_back(membership_object);
            //});
        }

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_check_profile_update(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    if (!sp->get_rm_session().is_logged_on()) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
        return std::move(response);
    }

    try {

        sp->get_rm_session().get_timer().force_heartbeat();

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data",
            NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                std::pair<std::wstring, NX::json_value>(L"profile_changed", NX::json_value(false)),
                std::pair<std::wstring, NX::json_value>(L"policy_changed", NX::json_value(false))
            }), false))
        }), false);

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_check_software_update(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    try {

        NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({

            std::pair<std::wstring, NX::json_value>(L"code", NX::json_value((int)0)),
            std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
            std::pair<std::wstring, NX::json_value>(L"data",
            NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                std::pair<std::wstring, NX::json_value>(L"hasUpdate", NX::json_value(false)),
                std::pair<std::wstring, NX::json_value>(L"newVersion", NX::json_value(L"9.1.0")),
                std::pair<std::wstring, NX::json_value>(L"checksum", NX::json_value(L"DDDDF9F2636532519D3E5BF3C6B51C46"))
            }), false))
        }), false);

        response = NX::conversion::utf16_to_utf8(v.serialize());
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_CALL_NOT_IMPLEMENTED, "Not implement yet");

    return std::move(response);
}

std::string rmserv::on_request_set_log_settings(unsigned long session_id, unsigned long process_id, const NX::json_object& parameters)
{
    std::string response;

    try {

        bool changed = false;
        const bool permanentChange = parameters.at(L"permanentChange").as_boolean();

        if (parameters.has_field(L"logLevel")) {
            const unsigned long newLogLevel = parameters.at(L"logLevel").as_int32();
            if (0 != newLogLevel && newLogLevel != _serv_conf.get_log_level()) {
                // Change log level
                _serv_conf.set_log_level(newLogLevel);
                changed = true;
            }
        }

        if (parameters.has_field(L"logSize")) {
            const unsigned long newlogSize = parameters.at(L"logSize").as_int32();
            if (0 != newlogSize && newlogSize != _serv_conf.get_log_size()) {
                // Change log level
                _serv_conf.set_log_size(newlogSize);
                changed = true;
            }
        }

        if (parameters.has_field(L"logRotation")) {
            const unsigned long newlogRotation = parameters.at(L"logRotation").as_int32();
            if (0 != newlogRotation && newlogRotation != _serv_conf.get_log_rotation()) {
                // Change log rotation count
                _serv_conf.set_log_rotation(newlogRotation);
                changed = true;
            }
        }

        if (changed && permanentChange) {
            _serv_conf.apply();
        }

        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_set_default_membership(unsigned long session_id, unsigned long process_id, const NX::json_object& parameters)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    if (!sp->get_rm_session().is_logged_on()) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
        return std::move(response);
    }

    try {

        const __int64 userId = parameters.at(L"userId").as_int64();
        const std::wstring defaultMembership = parameters.at(L"defaultMember").as_string();

        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_set_default_security_mode(unsigned long session_id, unsigned long process_id, int mode)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    if (!sp->get_rm_session().is_logged_on()) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
        return std::move(response);
    }

    try {

        //response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
        sp->get_rm_session().get_profile().get_preferences();
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_CALL_NOT_IMPLEMENTED, "Not implement yet");
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }
    
    return std::move(response);
}

std::string rmserv::on_request_register_nodification_handler(unsigned long session_id, unsigned long process_id, HWND h)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {

            LOGINFO(NX::string_formater(L"RMC App register itself (pid = %d, notify HWND = %p)", process_id, h));
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }


    return std::move(response);
}

std::string rmserv::on_request_log_sharing_transaction(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    if (!sp->get_rm_session().is_logged_on()) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
        return std::move(response);
    }

    try {
        
        std::vector<std::wstring> failed_files;
        const NX::json_array& sharedDocuments = parameters.as_object().at(L"sharedDocuments").as_array();
        const size_t sharedDocumentsCount = (int)sharedDocuments.size();

        std::for_each(sharedDocuments.begin(), sharedDocuments.end(), [&](const NX::json_value& doc) {

            try {

                const std::wstring& shared_file_duid = doc.as_object().at(L"duid").as_string();
                const std::wstring& shared_file = doc.as_object().at(L"filePath").as_string();
                const std::wstring& shared_info = doc.serialize();
                int shareType = 0;
                if (doc.as_object().has_field(L"shareType")) {
                    try {
                        shareType = doc.as_object().at(L"shareType").as_int32();
                    }
                    catch (const std::exception& e) {
                        UNREFERENCED_PARAMETER(e);
                        shareType = 0;
                    }
                }

                if (!shared_file.empty()) {
                    if (!sp->get_rm_session().upload_sharing_transition(process_id, shareType, shared_file, shared_info)) {
                        failed_files.push_back(shared_file_duid);
                    }
                }
            }
            catch (const std::exception& e) {
                LOGERROR(e.what());
            }
        });

        if (failed_files.empty()) {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
        }
        else {
            NX::json_value vresponse = NX::json_value::create_object();
            vresponse[L"code"] = NX::json_value((int)ERROR_NOT_ALL_ASSIGNED);
            vresponse[L"message"] = NX::json_value(L"not all documents can be shared");
            vresponse[L"rejectList"] = NX::json_value::create_array();
            std::for_each(failed_files.begin(), failed_files.end(), [&](const std::wstring& duid) {
                vresponse[L"rejectList"].as_array().push_back(NX::json_value(duid));
            });
            response = NX::conversion::utf16_to_utf8(vresponse.serialize());
        }

        if (sharedDocumentsCount != 0 && sharedDocumentsCount != failed_files.size()) {
            sp->get_rm_session().get_timer().force_uploadlog();
        }
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

bool rmserv::IsPDPProcess(ULONG processId)
{
	const process_record& proc_record = GLOBAL.safe_find_process(processId);
	if (proc_record.empty())
		return false;
	
	std::wstring dbginfo;
	const std::wstring image_path(proc_record.get_image_path());
	const wchar_t* image_name = wcsrchr(image_path.c_str(), L'\\');
	image_name = (NULL == image_name) ? image_path.c_str() : (image_name + 1);
	dbginfo = image_name;
	std::transform(dbginfo.begin(), dbginfo.end(), dbginfo.begin(), tolower);
	if (dbginfo.find(L"cepdpman.exe") != std::string::npos)
		return true;
	
	return false;
}

bool rmserv::IsPDPProcess(std::vector<std::pair<unsigned long, std::wstring>>& protected_processes)
{
	std::wstring dbginfo;
	size_t count = 0;

	for (int i = 0; i < (int)protected_processes.size(); i++) 
	{
		const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
		const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
		pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
		//LOGDEBUG(NX::string_formater(L"IsPDPProcess:  pname: %s", pname));
		LOGDEBUG(NX::string_formater(L"IsPDPProcess:  process: %s", item.second.c_str()));
		dbginfo = pname;
		std::transform(dbginfo.begin(), dbginfo.end(), dbginfo.begin(), tolower);
		if (dbginfo.find(L"cepdpman.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"tsvncache.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"vmtoolsd.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmdapp.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmtray.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmviewer.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmprint.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmserv.exe") != std::string::npos)
			count++;
	}

	if (protected_processes.size() == count)
		return true;
	else
		return false;
}

bool rmserv::IsPDPProcessEx(std::vector<std::pair<unsigned long, std::wstring>>& protected_processes)
{
	std::wstring dbginfo;
	size_t count = 0;

	for (int i = 0; i < (int)protected_processes.size(); i++)
	{
		const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
		const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
		pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
		//LOGDEBUG(NX::string_formater(L"IsPDPProcess:  pname: %s", pname));
		LOGDEBUG(NX::string_formater(L"IsPDPProcess:  process: %s", item.second.c_str()));
		dbginfo = pname;
		std::transform(dbginfo.begin(), dbginfo.end(), dbginfo.begin(), tolower);
		if (dbginfo.find(L"cepdpman.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"tsvncache.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"vmtoolsd.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmdapp.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmtray.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmprint.exe") != std::string::npos)
			count++;
		if (dbginfo.find(L"nxrmserv.exe") != std::string::npos)
			count++;
	}

	if (protected_processes.size() == count)
		return true;
	else
		return false;
}

void rmserv::GetNonPDPProcessId(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes, std::set<unsigned long>& pidSet)
{
	for (int i = 0; i < (int)protected_processes.size(); i++)
	{
		const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
		const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
		pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
		std::wstring dbginfo = pname;
		std::transform(dbginfo.begin(), dbginfo.end(), dbginfo.begin(), tolower);
		if (dbginfo.find(L"cepdpman.exe") == std::wstring::npos && 
			dbginfo.find(L"nxrmdapp.exe") == std::wstring::npos && 
			dbginfo.find(L"nxrmtray.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmviewer.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmprint.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmserv.exe") == std::wstring::npos &&
			dbginfo.find(L"tsvncache.exe") == std::wstring::npos &&
			dbginfo.find(L"vmtoolsd.exe") == std::wstring::npos)
		{
			pidSet.insert(item.first);
		}
	}
}


void rmserv::GetNonPDPProcessIdEx(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes, std::set<unsigned long>& pidSet)
{
	for (int i = 0; i < (int)protected_processes.size(); i++)
	{
		const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
		const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
		pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
		std::wstring dbginfo = pname;
		std::transform(dbginfo.begin(), dbginfo.end(), dbginfo.begin(), tolower);
		if (dbginfo.find(L"cepdpman.exe") == std::wstring::npos &&
			dbginfo.find(L"tsvncache.exe") == std::wstring::npos &&
			dbginfo.find(L"vmtoolsd.exe") == std::wstring::npos  &&
			dbginfo.find(L"nxrmdapp.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmtray.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmprint.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmserv.exe") == std::wstring::npos)
		{
			pidSet.insert(item.first);
		}
	}
}

std::wstring rmserv::GetNonPDPProcesses(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes)
{
    std::wstring nonPDPProcesses;
    for (int i = 0; i < (int)protected_processes.size(); i++)
    {
        const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
        const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
        pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
        std::wstring dbginfo = pname;
        std::transform(dbginfo.begin(), dbginfo.end(), dbginfo.begin(), tolower);
        if (dbginfo.find(L"cepdpman.exe") == std::wstring::npos &&
            dbginfo.find(L"nxrmdapp.exe") == std::wstring::npos &&
            dbginfo.find(L"nxrmtray.exe") == std::wstring::npos &&
            dbginfo.find(L"nxrmviewer.exe") == std::wstring::npos &&
            dbginfo.find(L"nxrmprint.exe") == std::wstring::npos &&
            dbginfo.find(L"nxrmserv.exe") == std::wstring::npos &&
            dbginfo.find(L"tsvncache.exe") == std::wstring::npos &&
            dbginfo.find(L"vmtoolsd.exe") == std::wstring::npos)
        {
            if (nonPDPProcesses.size() == 0)
                nonPDPProcesses += dbginfo;
            else
                nonPDPProcesses += L" " + dbginfo;
        }
    }

    return nonPDPProcesses;
}

std::wstring rmserv::GetNonPDPProcessesEx(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes)
{
	std::wstring nonPDPProcesses;
	for (int i = 0; i < (int)protected_processes.size(); i++)
	{
		const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
		const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
		pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
		std::wstring dbginfo = pname;
		std::transform(dbginfo.begin(), dbginfo.end(), dbginfo.begin(), tolower);
		if (dbginfo.find(L"cepdpman.exe") == std::wstring::npos &&
			dbginfo.find(L"tsvncache.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmdapp.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmtray.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmprint.exe") == std::wstring::npos &&
			dbginfo.find(L"nxrmserv.exe") == std::wstring::npos &&
			dbginfo.find(L"vmtoolsd.exe") == std::wstring::npos)
		{
			if (nonPDPProcesses.size() == 0)
				nonPDPProcesses += dbginfo;
			else
				nonPDPProcesses += L" " + dbginfo;
		}
	}

	return nonPDPProcesses;
}

bool rmserv::IsFileInMarkDirOpened(const std::set<unsigned long>& pidSet, const std::wstring& markedDir)
{
	if (pidSet.empty())
	{
		return false;
	}

	std::vector<std::wstring> openFiles;
	if (!EnumerateOpenedFiles(pidSet, openFiles))
	{
		return true;
	}

	if (openFiles.empty())
	{
		return false;
	}

	for (auto it = openFiles.begin(); it != openFiles.end(); it++)
	{
		if (it->size() > markedDir.size() && boost::algorithm::istarts_with(*it, markedDir))
		{
			return true;
		}
	}

	return false;
}

bool rmserv::EnumerateOpenedFiles(const std::set<unsigned long>& pidSet, std::vector<std::wstring>& openFiles)
{
	HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
	PNtQuerySystemInformation NtQuerySystemInformation = (PNtQuerySystemInformation)GetProcAddress(hModule, "NtQuerySystemInformation");
	if (NULL == NtQuerySystemInformation)
	{
		return false;
	}

	PSYSTEM_HANDLE_INFORMATION pSysHandleInformation = new SYSTEM_HANDLE_INFORMATION;
	DWORD size = sizeof(SYSTEM_HANDLE_INFORMATION);
	DWORD needed = 0;
	NTSTATUS status = NtQuerySystemInformation(SystemHandleInformation, pSysHandleInformation, size, &needed);
	if (!NT_SUCCESS(status))
	{
		if (0 == needed)
		{
			return false;
		}

		delete pSysHandleInformation;
		size = needed + 1024;
		pSysHandleInformation = (PSYSTEM_HANDLE_INFORMATION)new BYTE[size];
		status = NtQuerySystemInformation(SystemHandleInformation, pSysHandleInformation, size, &needed);
		if (!NT_SUCCESS(status))
		{
			delete pSysHandleInformation;
			return false;
		}
	}

	int nFileType;
	NX::win::WINBUILDNUM buildNum = GLOBAL.get_windows_info().build_number();
	if (buildNum >= NX::win::WBN_WIN10_MAY2019UPDATE)
	{
		nFileType = 37;
	}
	else if (buildNum >= NX::win::WBN_WIN10_FALLCREATORSUPDATE)
	{
		nFileType = 36;
	}
    else if (buildNum >= NX::win::WBN_WIN10_ANNIVERSARYUPDATE)
    {
        nFileType = 34;
    }
    else if (buildNum >= NX::win::WBN_WIN10)
	{
		nFileType = 35;
	}
    else if (buildNum >= NX::win::WBN_WIN81)
    {
        nFileType = 30;
    }
    else
	{
		nFileType = 28;
	}

	for (DWORD i = 0; i < pSysHandleInformation->dwCount; i++)
	{
		SYSTEM_HANDLE& sh = pSysHandleInformation->Handles[i];
		if (sh.bObjectType == nFileType && pidSet.find(sh.dwProcessId) != pidSet.end())
		{
			HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, sh.dwProcessId);
			if (hProcess)
			{
				HANDLE hDup = 0;
				BOOL b = DuplicateHandle(hProcess, (HANDLE)sh.wValue, GetCurrentProcess(), &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
				if (b)
				{
					if (FILE_TYPE_DISK == GetFileType(hDup))
					{
						wchar_t Path[MAX_PATH];
						DWORD dwRet = GetFinalPathNameByHandleW(hDup, Path, MAX_PATH - 1, FILE_NAME_NORMALIZED);
						if (dwRet > 0)
						{
							std::transform(Path, Path + MAX_PATH, Path, towlower);

							LOGINFO(NX::string_formater(L"Enumerate opened files in trusted processes (pid = %ld, file is %s)", sh.dwProcessId, Path));

							if (boost::algorithm::starts_with(Path, L"\\\\?\\"))
							{
								openFiles.push_back(Path + 4);
							}
							else
							{
								openFiles.push_back(Path);
							}
						}
					}
					CloseHandle(hDup);
				}
				CloseHandle(hProcess);
			}
		}

	}

	delete pSysHandleInformation;

	return true;
}

bool rmserv::CheckFileOpen()
{
	std::string response;
	std::vector<unsigned long> ids = _win_session_manager.get_all_session_id();

	for (size_t i = 0; i < ids.size(); i++)
	{
		if (IsFileOpen(ids[i], response))
		{
			LOGERROR(NX::string_formater(L"%s", NX::conversion::utf8_to_utf16(response).c_str()));
			return true;;
		}
	}

	return true;
}

bool rmserv::IsFileOpen(unsigned long session_id, std::string& response)
{
	std::vector<std::pair<unsigned long, std::wstring>> protected_processes;
	if (GLOBAL.get_process_cache().does_protected_process_exist(session_id)) {
		protected_processes = GLOBAL.get_process_cache().find_all_protected_process(session_id);
	}

	if (protected_processes.empty())
		return false;
	if (IsPDPProcess(protected_processes))
		return false;

	bool bFoundNXL = false;
	std::set<unsigned long> pidSet;
	GetNonPDPProcessId(protected_processes, pidSet);
	for (int i = 0; i < m_markDir.size(); i++)
	{
		std::wstring folderpath = std::get<0>(m_markDir[i]);
		if (IsFileInMarkDirOpened(pidSet, folderpath + L'\\'))
		{
			bFoundNXL = true;
			break;
		}
	}

	if (bFoundNXL)
	{
		std::wstring dbginfo;
		std::wstring str;
		response = "{\"code\":170, \"message\":\"Fail to log out due to process(es) with opened NXL file\", \"data\": {"; // ERROR_BUSY = 170L
		dbginfo = L"Logout was denied because following protected process(es) are still running:\r\n";
		for (int i = 0; i < (int)protected_processes.size(); i++)
		{
			const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
			const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
			pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
			str = pname;
			if (str.find(L"cepdpman.exe") != std::string::npos)
				continue;

			if (0 != i)
				response.append(" ,");

			response.append("\"");
			response.append(NX::conversion::to_string((int)item.first));
			response.append("\": \"");
			response.append(NX::conversion::utf16_to_utf8(pname));
			response.append("\"");
			if (NX::dbg::LL_INFO <= NxGlobalLog.get_accepted_level()) {
				dbginfo.append(L"    ");
				dbginfo.append(pname);
				dbginfo.append(L" (");
				dbginfo.append(NX::conversion::to_wstring((int)item.first));
				dbginfo.append(L")\r\n");
			}
		}
		response.append("}}");

		return true;
	}

	return false;
}

std::string rmserv::on_request_logout(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    if (!sp->get_rm_session().is_logged_on()) {
		LOGDEBUG(L"on_request_logout: not login");
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
        return std::move(response);
    }
	if (is_server_mode())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "Machine is under server mode");
		return std::move(response);
	}

    try {

        std::vector<std::pair<unsigned long, std::wstring>> protected_processes;

        if (GLOBAL.get_process_cache().does_protected_process_exist(session_id)) {
            protected_processes = GLOBAL.get_process_cache().find_all_protected_process(session_id);
        }
        
        if(protected_processes.empty() || IsPDPProcess(protected_processes)) {
            // No protected process running
            sp->get_rm_session().logout(false);
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
        }
		else {
			std::set<unsigned long> pidSet;
			GetNonPDPProcessId(protected_processes, pidSet);
			int j = 0;
			std::wstring dbginfo;
			response = "{\"code\":170, \"message\":\"Fail to log out because following protected process(es) are still running \", \"data\": {"; // ERROR_BUSY = 170L
			dbginfo = L"Logout was denied because following protected process(es) are still running:\r\n";
			for (int i = 0; i < (int)protected_processes.size(); i++) {
				const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
				if (pidSet.find(item.first) == pidSet.end()) continue;
				const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
				pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
				if (0 != j) {
					response.append(" ,");
				}
				j++;
				response.append("\"");
				response.append(NX::conversion::to_string((int)item.first));
				response.append("\": \"");
				response.append(NX::conversion::utf16_to_utf8(pname));
				response.append("\"");
				if (NX::dbg::LL_INFO <= NxGlobalLog.get_accepted_level()) {
					dbginfo.append(L"    ");
					dbginfo.append(pname);
					dbginfo.append(L" (");
					dbginfo.append(NX::conversion::to_wstring((int)item.first));
					dbginfo.append(L")\r\n");
				}
			}
			response.append("}}");
			LOGINFO(dbginfo);
		}
    }
    catch (const std::exception& e) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
    }

    return std::move(response);
}

std::string rmserv::on_request_collect_debugdump(unsigned long session_id, unsigned long process_id)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    if (!sp->get_rm_session().is_logged_on()) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
        return std::move(response);
    }

    const std::wstring audit_log_file(sp->get_rm_session().get_temp_profile_dir() + L"\\audit.json");

    try {

        diagtool dt;
        std::wstring dbgfile;
        std::wstring notify_message;

        sp->get_rm_session().export_audit_log();

        sp->get_rm_session().get_app_manager().send_popup_notification(L"Generating debug dump ......");
        if (dt.generate(sp->get_windows_user_sid(), sp->get_rm_session().get_tenant_id(), NX::conversion::to_wstring(sp->get_rm_session().get_profile().get_id()), sp->get_user_dirs().get_desktop(), dbgfile)) {
            notify_message = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_DBGLOG_COLLECTED, 1024, LANG_NEUTRAL, L"Debug data file (%s) has been generated on your desktop", dbgfile.c_str());
            response = NX::string_formater("L{\"code\":%d, \"message\":\"%s\"}", 0, notify_message.c_str());
        }
        else {
            notify_message = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_NOTIFY_DBGLOG_COLLECT_FAILED, 1024, LANG_NEUTRAL, L"Fail to collect debug data");
            response = NX::string_formater("L{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, notify_message.c_str());
        }

        sp->get_rm_session().get_app_manager().send_popup_notification(notify_message);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to generate debug dump\"}", ERROR_INVALID_DATA);
    }

    ::DeleteFileW(audit_log_file.c_str());

    return std::move(response);
}

std::string rmserv::on_request_activity_notify(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    if (!sp->get_rm_session().is_logged_on()) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
        return std::move(response);
    }

    try {

        std::wstring file_name;
        std::wstring file_path;
        bool notify_user = false;
        unsigned long language_id = 1033;
        int act_operation = parameters.as_object().at(L"operation").as_int32();
        int act_result = parameters.as_object().at(L"result").as_int32();
        if (parameters.as_object().has_field(L"fileName")) {
            file_name = parameters.as_object().at(L"fileName").as_string();
            const wchar_t* p = wcsrchr(file_name.c_str(), L'\\');
            if (NULL != p) {
                file_name = p + 1;
            }
        }
        if (parameters.as_object().has_field(L"filePath")) {
            file_path = parameters.as_object().at(L"filePath").as_string();
            if (file_name.empty()) {
                const wchar_t* p = wcsrchr(file_path.c_str(), L'\\');
                file_name = (NULL == p) ? file_path : (p + 1);
            }
        }
        if (parameters.as_object().has_field(L"notifyUser")) {
            notify_user = parameters.as_object().at(L"notifyUser").as_boolean();
        }
        if (parameters.as_object().has_field(L"languageId")) {
            language_id = parameters.as_object().at(L"languageId").as_int32();
        }

        // Notify user if necessary
        if (0 == act_result && notify_user && !file_name.empty()) {

            const std::wstring& info_fmt = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s this file (%s)");
            std::wstring info;
            std::wstring info_operation;

            switch (act_operation)
            {
            case ActPrint:      // Print
                info_operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_PRINT, 64, LANG_NEUTRAL, L"print");
                info = NX::string_formater(info_fmt.c_str(), info_operation.c_str(), file_name.c_str());
                break;
            case ActEdit:       // Edit/Save
                info_operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_SAVE, 64, LANG_NEUTRAL, L"save");
                info = NX::string_formater(info_fmt.c_str(), info_operation.c_str(), file_name.c_str());
                break;
            case ActDecrypt:    // Decrypt
                info_operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_EXPORT, 64, LANG_NEUTRAL, L"export");
                info = NX::string_formater(info_fmt.c_str(), info_operation.c_str(), file_name.c_str());
                break;
            case ActProtect:        // Protect
            case ActShare:          // Share
            case ActModifyShare:    // Modify Share
            case ActView:           // View
            case ActDownload:       // Download
            case ActRevoke:         // Revoke
            case ActCopyContent:    // Copy Content
            case ActCaptureScreen:  // Capture Screen
            case ActClassify:       // Classify
            default:                // Unknown
                break;
            }

            if (!info.empty()) {
                sp->get_rm_session().get_app_manager().send_popup_notification(info);
            }
        }

        // Get file context
        if (!file_path.empty()) {

            NX::NXL::document_context context(file_path);
            if (!context.empty()) {

                const process_record& proc_record = GLOBAL.safe_find_process(process_id);
                if (!proc_record.empty()) {
                    const std::wstring image_path(proc_record.get_image_path());
                    const wchar_t* image_name = wcsrchr(image_path.c_str(), L'\\');
                    image_name = (NULL == image_name) ? image_path.c_str() : (image_name + 1);

                    // try to log activity
                    sp->get_rm_session().log_activity(activity_record(context.get_duid(),
                                                                      context.get_owner_id(),
                                                                      sp->get_rm_session().get_profile().get_id(),
                                                                      act_operation,
                                                                      act_result,
                                                                      file_path,
                                                                      proc_record.get_image_path(),
                                                                      proc_record.get_pe_file_info()->get_image_publisher(),
                                                                      std::wstring()));

                    sp->get_rm_session().audit_activity(act_operation, act_result, NX::conversion::to_wstring(context.get_duid()), context.get_owner_id(), image_name, file_path);
                }
            }
        }
        response = NX::string_formater("{\"code\":%d, \"message\":\"succeed\"}", 0);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to log activity\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_set_dwm_status(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {

        const bool enabled = parameters.as_object().at(L"enabled").as_boolean();
        sp->get_rm_session().set_dwm_status(enabled);
        response = NX::string_formater("{\"code\":%d, \"message\":\"succeed\"}", 0);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to set DWM status\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_query_activity_records(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {

        ULONG   count = 0xFFFFFFFF;
        UINT64  start_time = 0;
        UINT64  end_time = 0;
        std::wstring file_pattern;
        std::wstring file_duid;
        int operation_id = -1;
        int operation_result = -1;

        if (parameters.as_object().has_field(L"count")) {
            count = parameters.as_object().at(L"count").as_uint32();
        }
        if (parameters.as_object().has_field(L"criteria")) {
            if (parameters.as_object().has_field(L"start_time")) {
                start_time = parameters.as_object().at(L"start_time").as_uint64();
            }
            if (parameters.as_object().has_field(L"end_time")) {
                end_time = parameters.as_object().at(L"end_time").as_uint64();
            }
            if (parameters.as_object().has_field(L"file")) {
                file_pattern = parameters.as_object().at(L"file").as_string();
            }
            if (parameters.as_object().has_field(L"duid")) {
                file_duid = parameters.as_object().at(L"duid").as_string();
            }
            if (parameters.as_object().has_field(L"operation")) {
                operation_id = parameters.as_object().at(L"operation").as_int32();
            }
            if (parameters.as_object().has_field(L"result")) {
                operation_result = parameters.as_object().at(L"result").as_int32();
            }
        }

        std::wstring outlogfile;
        if (sp->get_rm_session().export_audit_query_result(outlogfile)) {
            NX::json_value v = NX::json_value::create_object();
            v[L"code"] = 0;
            v[L"message"] = NX::json_value(L"succeed");
            v[L"data"] = NX::json_value::create_object();
            v[L"data"].as_object()[L"record_file"] = NX::json_value(outlogfile);
            response = NX::conversion::utf16_to_utf8(v.serialize());
        }
        else {
            response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

bool rmserv::UpdatePDPDirectory(const std::wstring dir)
{
	HKEY hKey;
	LONG lRet;
	std::wstring subkey = L"Software\\NextLabs\\RPM\\Router";
	std::wstring subkey1 = L"PDP";
	LOGDEBUG(NX::string_formater(L"UpdatePDPDirectory: %s", dir.c_str()));

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ | KEY_SET_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
	{
		
		lRet = RegCreateKey(HKEY_LOCAL_MACHINE, subkey.c_str(), &hKey);
		if (lRet != ERROR_SUCCESS)
		{
			LOGDEBUG(NX::string_formater(L"UpdatePDPDirectory: RegCreateKey failed %s", subkey.c_str()));
			return false;
		}
	}

	lRet = RegSetValueEx(hKey, subkey1.c_str(), 0, REG_SZ, (const BYTE *)(dir.c_str()), (DWORD)(dir.size() + 1) * sizeof(WCHAR));
	if (lRet != ERROR_SUCCESS)
	{
		LOGERROR(NX::string_formater(L"UpdatePDPDirectory: RegSetValueEx failed %s", dir.c_str()));
	}

	RegCloseKey(hKey);
	return true;

}

bool rmserv::RefreshDirectory()
{
	HKEY hKey;
	LONG lRet;
	std::wstring subkey = L"Software\\NextLabs\\SkyDRM";
	std::wstring subkey1 = L"securedfolder";
	std::wstring dir = get_dirs();
	LOGDEBUG(NX::string_formater(L"RefreshDirectory: write to registry, %s=\"%s\"", subkey1.c_str(), dir.c_str()));
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	std::wstring subkey2 = L"sanctuaryfolder";
	std::wstring dir2 = get_sanctuary_dirs();
	LOGDEBUG(NX::string_formater(L"RefreshDirectory: write to registry, %s=\"%s\"", subkey2.c_str(), dir2.c_str()));
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_ALL_ACCESS, &hKey);
	if (lRet != ERROR_SUCCESS)
	{
		LOGDEBUG(NX::string_formater(L"RegOpenKeyEx: failed %d", lRet));
		return false;
	}
	
	lRet = RegSetValueEx(hKey, subkey1.c_str(), 0, REG_SZ, (const BYTE *)(dir.c_str()), (DWORD)(dir.size() + 1) * sizeof(WCHAR));
	if (lRet != ERROR_SUCCESS)
	{
		LOGERROR(NX::string_formater(L"RefreshDirectory: RegSetValueEx failed %d", lRet));
	}
	
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	lRet = RegSetValueEx(hKey, subkey2.c_str(), 0, REG_SZ, (const BYTE *)(dir2.c_str()), (DWORD)(dir2.size() + 1) * sizeof(WCHAR));
	if (lRet != ERROR_SUCCESS)
	{
		LOGERROR(NX::string_formater(L"RefreshDirectory: RegSetValueEx failed %d", lRet));
	}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	RegCloseKey(hKey);
	return true;
}

//
// action:
//  1 add
//  0 remove
//  2 check whether my RPM folder
//  3 check whether my MyFolder
//
unsigned long rmserv::AddDirectoryConfig(std::wstring dir, SDRmRPMFolderConfig action, const std::wstring& fileTags, const std::wstring &wsid, const std::wstring &rmsuid, const std::wstring &stroption)
{
	std::transform(dir.begin(), dir.end(), dir.begin(), tolower);
	LOGDEBUG(NX::string_formater(L"AddDirectoryConfig: %s, add: %d, fileTags: %s, windows-sid: %s, rms-uid: %s", dir.c_str(), action, fileTags.c_str(), wsid.c_str(), rmsuid.c_str()));

    // convert filetags, rmsuid to based64
    std::wstring _fileTags_base64 = NX::conversion::to_base64((const unsigned char*)NX::conversion::utf16_to_utf8(fileTags).c_str(), NX::conversion::utf16_to_utf8(fileTags).size());
    unsigned int _option = std::stoi(stroption);

	if (m_markDir.size() == 0)
	{
		if (action == SDRmRPMFolderConfig::RPMFOLDER_ADD)
		{		
			wchar_t ext = dir[dir.size() - 1];
			dir.pop_back();
			wchar_t overwrite = dir[dir.size() - 1];
			dir.pop_back();
			wchar_t autoProtect = dir[dir.size() - 1];
			dir.pop_back();
            ::EnterCriticalSection(&_rpmlock);
            m_markDir.push_back(std::make_tuple(dir, autoProtect, overwrite, ext, _fileTags_base64, wsid, rmsuid, stroption));
            ::LeaveCriticalSection(&_rpmlock);
            return 0;
		}
		else
			return ERROR_NOT_FOUND;
	}

    ::EnterCriticalSection(&_rpmlock);
    if (action == SDRmRPMFolderConfig::RPMFOLDER_ADD)
	{
		wchar_t ext = dir[dir.size() - 1];
		dir.pop_back();
		wchar_t overwrite = dir[dir.size() - 1];
		dir.pop_back();
		wchar_t autoProtect = dir[dir.size() - 1];
		dir.pop_back();

        for (size_t i = 0; i < m_markDir.size(); i++)
		{
			if (std::get<0>(m_markDir[i]) == dir)
			{
				::LeaveCriticalSection(&_rpmlock);
				return ERROR_ACCESS_DENIED;
			}
			else // dir is subfolder of std::get<0>(m_markDir[i])
			{
				std::wstring myFolderDir = boost::iends_with(std::get<0>(m_markDir[i]), L"\\") ? std::get<0>(m_markDir[i]) : std::get<0>(m_markDir[i]) + L"\\";
				std::wstring dir2 = boost::iends_with(dir, L"\\") ? dir : dir + L"\\";
				if (boost::algorithm::istarts_with(dir, myFolderDir)) {
					std::wstring u_option = std::get<7>(m_markDir[i]);
					unsigned int option = std::stoi(u_option);
					if (option & RPMFOLDER_MYFOLDER && _option & RPMFOLDER_MYFOLDER)
					{
						::LeaveCriticalSection(&_rpmlock);
						return ERROR_ACCESS_DENIED;
					}
				}
                else if (boost::algorithm::istarts_with(myFolderDir, dir2))
                {
                    // parent folder also can't not be set as myfolder if there is a folder already set as myfolder
                    std::wstring u_option = std::get<7>(m_markDir[i]);
                    unsigned int option = std::stoi(u_option);
                    if (option & RPMFOLDER_MYFOLDER && _option & RPMFOLDER_MYFOLDER)
                    {
                        ::LeaveCriticalSection(&_rpmlock);
                        return ERROR_ACCESS_DENIED;
                    }
                }
			}
		}

		m_markDir.push_back(std::make_tuple(dir, autoProtect, overwrite, ext, _fileTags_base64, wsid, rmsuid, stroption));
    }
	else if (action == SDRmRPMFolderConfig::RPMFOLDER_REMOVE)
	{
		// remove dir
		for (size_t i = 0; i < m_markDir.size(); i++)
		{
			if (std::get<0>(m_markDir[i]) == dir)
			{
                // 
                // you can't remove the RPM folder which belongs to others
                //
                std::wstring u_wsid = std::get<5>(m_markDir[i]);
                std::wstring u_rmsid = std::get<6>(m_markDir[i]);
                if ((_wcsnicmp(wsid.c_str(), u_wsid.c_str(), wsid.length()) == 0) &&
                    (_wcsnicmp(rmsuid.c_str(), u_rmsid.c_str(), rmsuid.length()) == 0))
                {
                    m_markDir.erase(m_markDir.begin() + i);
                    ::LeaveCriticalSection(&_rpmlock);
                    return 0;
                }
                else
                {
                    ::LeaveCriticalSection(&_rpmlock);
                    return ERROR_ACCESS_DENIED;
                }
			}
		}

        ::LeaveCriticalSection(&_rpmlock);
        return ERROR_NOT_FOUND;
	}
    else if (action == SDRmRPMFolderConfig::RPMFOLDER_CHECK_ADD_MYFOLDER)
    {
        wchar_t ext = dir[dir.size() - 1];
        dir.pop_back();
        wchar_t overwrite = dir[dir.size() - 1];
        dir.pop_back();
        wchar_t autoProtect = dir[dir.size() - 1];
        dir.pop_back();

        for (size_t i = 0; i < m_markDir.size(); i++)
        {
            if (std::get<0>(m_markDir[i]) == dir)
            {
                ::LeaveCriticalSection(&_rpmlock);
                return ERROR_ACCESS_DENIED;
            }
            else // dir is subfolder of std::get<0>(m_markDir[i])
            {
				std::wstring myFolderDir = boost::iends_with(std::get<0>(m_markDir[i]), L"\\") ? std::get<0>(m_markDir[i]) : std::get<0>(m_markDir[i]) + L"\\";
				std::wstring dir2 = boost::iends_with(dir, L"\\") ? dir : dir + L"\\";
                if (boost::algorithm::istarts_with(dir, myFolderDir)) {
                    std::wstring u_option = std::get<7>(m_markDir[i]);
                    unsigned int option = std::stoi(u_option);
                    if (option & RPMFOLDER_MYFOLDER && _option & RPMFOLDER_MYFOLDER)
                    {
                        ::LeaveCriticalSection(&_rpmlock);
                        return ERROR_ACCESS_DENIED;
                    }
                }
                else if (boost::algorithm::istarts_with(myFolderDir, dir2))
                {
                    // parent folder also can't not be set as myfolder if there is a folder already set as myfolder
                    std::wstring u_option = std::get<7>(m_markDir[i]);
                    unsigned int option = std::stoi(u_option);
                    if (option & RPMFOLDER_MYFOLDER && _option & RPMFOLDER_MYFOLDER)
                    {
                        ::LeaveCriticalSection(&_rpmlock);
                        return ERROR_ACCESS_DENIED;
                    }
                }
            }
        }

		::LeaveCriticalSection(&_rpmlock);
        return 0;
    }
    else if (action == SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYRPM)
    {
        for (size_t i = 0; i < m_markDir.size(); i++)
        {
            if (std::get<0>(m_markDir[i]) == dir)
            {
                std::wstring u_wsid = std::get<5>(m_markDir[i]);
                std::wstring u_rmsid = std::get<6>(m_markDir[i]);
                if ((_wcsnicmp(wsid.c_str(), u_wsid.c_str(), wsid.length()) == 0) &&
                    (_wcsnicmp(rmsuid.c_str(), u_rmsid.c_str(), rmsuid.length()) == 0))
                {
                    ::LeaveCriticalSection(&_rpmlock);
                    return 0;
                }
                else if ((_wcsnicmp(wsid.c_str(), u_wsid.c_str(), wsid.length()) == 0) &&
                    ((u_rmsid.rfind(L"\\") == (u_rmsid.size() - 1)) || (u_rmsid == L"")))
                {
                    ::LeaveCriticalSection(&_rpmlock);
                    return 0;
                }
                else if (wsid == L"")
                {
                    ::LeaveCriticalSection(&_rpmlock);
                    return 0;
                }
                else
                {
					::LeaveCriticalSection(&_rpmlock);
                    return ERROR_ACCESS_DENIED;
                }
            }
        }

        ::LeaveCriticalSection(&_rpmlock);
        return ERROR_NOT_FOUND;
    }
    else if (action == SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYFOLDER)
    {
        for (size_t i = 0; i < m_markDir.size(); i++)
        {
            if (std::get<0>(m_markDir[i]) == dir)
            {
                std::wstring u_wsid = std::get<5>(m_markDir[i]);
                std::wstring u_rmsid = std::get<6>(m_markDir[i]);
                std::wstring u_option = std::get<7>(m_markDir[i]);
                if ((_wcsnicmp(wsid.c_str(), u_wsid.c_str(), wsid.length()) == 0) &&
                    (_wcsnicmp(rmsuid.c_str(), u_rmsid.c_str(), rmsuid.length()) == 0))
                {
                    unsigned int option = std::stoi(u_option);
                    if (option & RPMFOLDER_MYFOLDER)
                    {
                        ::LeaveCriticalSection(&_rpmlock);
                        return 0;
                    }
					else 
					{
						::LeaveCriticalSection(&_rpmlock);
						return ERROR_ACCESS_DENIED;
					}
                }
                else
                {
					::LeaveCriticalSection(&_rpmlock);
                    return ERROR_ACCESS_DENIED;
                }
            }
        }

        ::LeaveCriticalSection(&_rpmlock);
        return ERROR_NOT_FOUND;
    }
    else if (action == SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYAPIRPM)
    {
        for (size_t i = 0; i < m_markDir.size(); i++)
        {
            if (std::get<0>(m_markDir[i]) == dir)
            {
                std::wstring u_wsid = std::get<5>(m_markDir[i]);
                std::wstring u_rmsid = std::get<6>(m_markDir[i]);
                std::wstring u_option = std::get<7>(m_markDir[i]);
                if ((_wcsnicmp(wsid.c_str(), u_wsid.c_str(), wsid.length()) == 0) &&
                    (_wcsnicmp(rmsuid.c_str(), u_rmsid.c_str(), rmsuid.length()) == 0))
                {
                    unsigned int option = std::stoi(u_option);
                    if (option & RPMFOLDER_API)
                    {
                        ::LeaveCriticalSection(&_rpmlock);
                        return 0;
                    }
					else {
						::LeaveCriticalSection(&_rpmlock);
						return ERROR_ACCESS_DENIED;
					}
                       
                }
                else
                {
					::LeaveCriticalSection(&_rpmlock);
                    return ERROR_ACCESS_DENIED;
                }
            }
        }

        ::LeaveCriticalSection(&_rpmlock);
        return ERROR_NOT_FOUND;
    }

    ::LeaveCriticalSection(&_rpmlock);
    return 0;
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
unsigned long rmserv::AddSanctuaryDirectoryConfig(std::wstring& dir, bool add, const std::wstring& fileTags)
{
	std::transform(dir.begin(), dir.end(), dir.begin(), tolower);
	LOGDEBUG(NX::string_formater(L"AddSanctuaryDirectoryConfig: %s, add: %d, fileTags: %s", dir.c_str(), add, fileTags.c_str()));

	if (m_sanctuaryDir.size() == 0)
	{
		if (add)
		{
			m_sanctuaryDir.push_back(std::make_pair(dir, fileTags));
			return 0;
		}
		else
			return ERROR_NOT_FOUND;
	}

	if (add)
	{
		for (size_t i = 0; i < m_sanctuaryDir.size(); i++)
		{
			if (m_sanctuaryDir[i].first == dir)
			{
				return ERROR_ALREADY_EXISTS;
			}
		}

		m_sanctuaryDir.push_back(std::make_pair(dir, fileTags));
	}
	else
	{
		// remove dir
		for (size_t i = 0; i < m_sanctuaryDir.size(); i++)
		{
			if (m_sanctuaryDir[i].first == dir)
			{
				m_sanctuaryDir.erase(m_sanctuaryDir.begin() + i);
				return 0;
			}
		}

		return ERROR_NOT_FOUND;
	}

	return 0;
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

std::string rmserv::on_request_insert_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	try {
		std::wstring filepath;

		if (parameters.as_object().has_field(L"filePath"))
		{
			unsigned int iOption = (unsigned int)SDRmRPMFolderOption::RPMFOLDER_NORMAL;
			bool bAutoProtect = false;
			bool bOverwrite = false;
			bool bExt = false;
			if (parameters.as_object().has_field(L"option"))
			{
				iOption = parameters.as_object().at(L"option").as_uint32();
				if (iOption & SDRmRPMFolderOption::RPMFOLDER_AUTOPROTECT)
				{
					bAutoProtect = true;
				}
				if (iOption & SDRmRPMFolderOption::RPMFOLDER_OVERWRITE)
				{
					bOverwrite = true;
				}
				if (iOption & SDRmRPMFolderOption::RPMFOLDER_EXT)
				{
					bExt = true;
				}
			}
            std::wstring stroption = std::to_wstring(iOption);

			std::wstring fileTags;
			if (parameters.as_object().has_field(L"fileTags"))
			{
				fileTags = parameters.as_object().at(L"fileTags").as_string();
			}
			else
			{
				fileTags = L"{}";
			}


			NX::fs::dos_fullfilepath input_filepath(parameters.as_object().at(L"filePath").as_string());
			// Fixed bug 65302 that nxl file can't hide the ".nxl" in RPM folder for long path.
			if (input_filepath.path().size() > 255) {
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 
					ERROR_FILENAME_EXCED_RANGE, "Error: the directory path is too long and exceeds the 255 character limitation");
				return std::move(response);
			}

			// Handle the disk root directory(like E:\)
			auto dosfilepath = input_filepath.path();
			auto pos = dosfilepath.rfind(L"\\");
			if (pos == dosfilepath.size() - 1) {
				std::wstring _dosfilepath = dosfilepath.substr(0, pos);
				if (NX::fs::is_dos_drive_only_path(_dosfilepath)) {
					dosfilepath = _dosfilepath;
				}
			}

			filepath = dosfilepath + (bAutoProtect ? L'1' : L'0');
			filepath += (bOverwrite ? L'1' : L'0');
			filepath += (bExt ? L'1' : L'0');
			
            // do check on MyFolders
            std::wstring wsid = sp->get_windows_user_sid();
            std::wstring tenantid = sp->get_rm_session().get_tenant_id();
            std::wstring userid = sp->get_rm_session().get_profile().get_email();
            userid = tenantid + L"\\" + userid;

            unsigned long val = 0;
            if (iOption & SDRmRPMFolderOption::RPMFOLDER_MYFOLDER) // before add MyFoder, check whehter you can or not
                val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_CHECK_ADD_MYFOLDER, fileTags, wsid, userid, stroption);

            if (val == 0)
            {
                ret = sp->get_rm_session().insert_directory_action(filepath);
                if (ret)
                {
                    //response = NX::string_formater("succeed");
                    response = NX::string_formater("{\"code\": 0, \"message\":\"succeed\"}");
                    unsigned long val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_ADD, fileTags, wsid, userid, stroption);
                    if (val == 0)
                        sp->get_rm_session().update_client_dir_config(get_dirs(), LOGIN_INFO::RPMFolder);
                    else
                    {
                        if (val == ERROR_ALREADY_EXISTS)
                            response = NX::string_formater("{\"code\":%d, \"message\":\"Error: Directory Already Exists\"}", val);
                        else if (val == ERROR_NOT_FOUND)
                            response = NX::string_formater("{\"code\":%d, \"message\":\"Error: Directory Not Found\"}", val);
                        else
                            response = NX::string_formater("{\"code\":%d, \"message\":\"failed\"}", val);
                    }
                }
                else
                    response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to set RPM folder\"}", ERROR_INVALID_DATA);
            }
            else
                response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to set RPM folder\"}", ERROR_INVALID_DATA);
        }
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
std::string rmserv::on_request_insert_sanctuary_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	try {
		std::wstring filepath;
		if (parameters.as_object().has_field(L"filePath"))
		{
			filepath = parameters.as_object().at(L"filePath").as_string();

			std::wstring fileTags;
			if (parameters.as_object().has_field(L"fileTags"))
			{
				fileTags = parameters.as_object().at(L"fileTags").as_string();
			}
			else
			{
				fileTags = L"{}";
			}

			ret = sp->get_rm_session().insert_sanctuary_directory_action(filepath);
			if (ret)
			{
				response = NX::string_formater("{\"code\": 0, \"message\":\"succeed\"}");
				unsigned long val = AddSanctuaryDirectoryConfig(filepath, true, fileTags);
				if (val == 0)
					sp->get_rm_session().update_client_dir_config(get_sanctuary_dirs(), LOGIN_INFO::sanctuaryFolder);
				else
				{
					if (val == ERROR_ALREADY_EXISTS)
						response = NX::string_formater("{\"code\":%d, \"message\":\"Error: Directory Already Exists\"}", val);
					else if (val == ERROR_NOT_FOUND)
						response = NX::string_formater("{\"code\":%d, \"message\":\"Error: Directory Not Found\"}", val);
					else
						response = NX::string_formater("{\"code\":%d, \"message\":\"failed\"}", val);
				}
			}
			else
				response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Invalid data\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

std::string rmserv::on_request_remove_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;
	std::vector<std::pair<unsigned long, std::wstring>> protected_processes;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	if (GLOBAL.get_process_cache().does_protected_process_exist(session_id)) {
		protected_processes = GLOBAL.get_process_cache().find_all_protected_process(session_id);
	}
	
    // do check on MyFolders
    std::wstring wsid = sp->get_windows_user_sid();
    std::wstring tenantid = sp->get_rm_session().get_tenant_id();
    std::wstring userid = sp->get_rm_session().get_profile().get_email();
    userid = tenantid + L"\\" + userid;

	try {
		std::wstring filepath;
		
		if (parameters.as_object().has_field(L"filePath"))
		{
			NX::fs::dos_fullfilepath input_filepath(parameters.as_object().at(L"filePath").as_string());
			filepath = input_filepath.path();
			std::transform(filepath.begin(), filepath.end(), filepath.begin(), tolower);

			// Handle the disk root directory(like E:\)
			auto pos = filepath.rfind(L"\\");
			if (pos == filepath.size() - 1) {
				std::wstring _filepath = filepath.substr(0, pos);
				if (NX::fs::is_dos_drive_only_path(_filepath)) {
					filepath = _filepath;
				}
			}

			std::wstring intermediate_dir = sp->get_intermediate_dir();
			if (intermediate_dir.size() > 3)
			{
				intermediate_dir.pop_back();
				intermediate_dir.pop_back();
				intermediate_dir.pop_back();
			}
			std::transform(intermediate_dir.begin(), intermediate_dir.end(), intermediate_dir.begin(), tolower);
			if (intermediate_dir == filepath)
			{
				// can't remove intermediate_dir
				response = NX::string_formater("{\"code\":%d, \"message\":\"You can't remove intermediate RPM folder [%s].\"}", 170, intermediate_dir.c_str());
				return std::move(response);
			}

			bool bforce = false;
			if (parameters.as_object().has_field(L"bforce"))
			{
				bforce = parameters.as_object().at(L"bforce").as_boolean();
			}

			if (!IsPDPProcessEx(protected_processes))
			{
				if (bforce)
				{
					std::set<unsigned long> pidSet;
					GetNonPDPProcessIdEx(protected_processes, pidSet);
					if (IsFileInMarkDirOpened(pidSet, filepath + L'\\'))
					{
						std::thread t(&rmserv::force_unset_RPM_folder, this, filepath, session_id, process_id);
						t.detach();
                        std::wstring nonPDPProcesses = GetNonPDPProcessesEx(protected_processes);
                        if (nonPDPProcesses.find(L" ") != std::wstring::npos)
                            response = NX::string_formater("{\"code\":%d, \"message\":\"Following trusted process (%s) are running and locking the files in folder. Please close them before reset to regular folder.\"}", 170, NX::conversion::utf16_to_utf8(nonPDPProcesses).c_str());
                        else
                            response = NX::string_formater("{\"code\":%d, \"message\":\"Following trusted process (%s) is running and locking the files in folder. Please close it before reset to regular folder.\"}", 170, NX::conversion::utf16_to_utf8(nonPDPProcesses).c_str());
						return std::move(response);
					}
				}
				else
				{
                    std::wstring nonPDPProcesses = GetNonPDPProcessesEx(protected_processes);
                    if (nonPDPProcesses.find(L" ") != std::wstring::npos)
					    response = NX::string_formater("{\"code\":%d, \"message\":\"Following trusted process (%s) are running. Please close them before reset to regular folder.\"}", 170, NX::conversion::utf16_to_utf8(nonPDPProcesses).c_str());
                    else
                        response = NX::string_formater("{\"code\":%d, \"message\":\"Following trusted process (%s) is running. Please close it before reset to regular folder.\"}", 170, NX::conversion::utf16_to_utf8(nonPDPProcesses).c_str());
                    return std::move(response);
				}
			}

            // check whehter user can remove this RPM folder
            unsigned long val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYRPM, L"{}", wsid, userid);
            if (val == 0 || val == ERROR_NOT_FOUND)
            {
                ret = sp->get_rm_session().remove_directory_action(filepath);

                if (ret)
                {
                    response = NX::string_formater("{\"code\":0, \"message\":\"succeed\"}");
                    unsigned long val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_REMOVE, L"{}", wsid, userid);
                    if (val == 0)
                        sp->get_rm_session().update_client_dir_config(get_dirs(), LOGIN_INFO::RPMFolder);
                    else
                    {
                        if (val == ERROR_NOT_FOUND)
                            response = NX::string_formater("{\"code\":%d, \"message\":\"Directory is not found or it's not a safe folder.\"}", val);
                        else
                            response = NX::string_formater("{\"code\":%d, \"message\":\"Remove directory failed\"}", val);
                    }
                }
                else
                    response = NX::string_formater("{\"code\":%d, \"message\":\"Driver remove directory failed\"}", ERROR_INVALID_DATA);
            }
            else
                response = NX::string_formater("{\"code\":%d, \"message\":\"Remove directory failed\"}", val);
        }
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
std::string rmserv::on_request_remove_sanctuary_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;
	std::vector<std::pair<unsigned long, std::wstring>> protected_processes;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	if (GLOBAL.get_process_cache().does_protected_process_exist(session_id)) {
		protected_processes = GLOBAL.get_process_cache().find_all_protected_process(session_id);
	}

	try {
		std::wstring filepath;

		if (parameters.as_object().has_field(L"filePath"))
		{
			filepath = parameters.as_object().at(L"filePath").as_string();
			std::transform(filepath.begin(), filepath.end(), filepath.begin(), tolower);

			ret = sp->get_rm_session().remove_sanctuary_directory_action(filepath);

			if (ret)
			{
				response = NX::string_formater("{\"code\":0, \"message\":\"succeed\"}");
				unsigned long val = AddSanctuaryDirectoryConfig(filepath, false);
				if (val == 0)
					sp->get_rm_session().update_client_dir_config(get_sanctuary_dirs(), LOGIN_INFO::sanctuaryFolder);
				else
				{
					if (val == ERROR_NOT_FOUND)
						response = NX::string_formater("{\"code\":%d, \"message\":\"Error: Directory Not Found\"}", val);
					else
						response = NX::string_formater("{\"code\":%d, \"message\":\"Remove directory failed\"}", val);
				}
			}
			else
				response = NX::string_formater("{\"code\":%d, \"message\":\"Driver remove directory failed\"}", ERROR_INVALID_DATA);
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Invalid data\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

void rmserv::force_unset_RPM_folder(std::wstring filepath, unsigned long session_id, unsigned long process_id)
{
	HANDLE pHandle = OpenProcess(SYNCHRONIZE, FALSE, process_id);
	LOGDEBUG(L"Begin force_unset_RPM_folder");
	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	DWORD waitRet = -1;
	if (pHandle) {
		waitRet = WaitForSingleObject(pHandle, 30000);
	}
	
	if (pHandle == NULL)
		LOGDEBUG(L"force_unset_RPM_folder: can't get handle for processid %ld", process_id);

	if (waitRet == WAIT_TIMEOUT)
	{
        ::CloseHandle(pHandle);
        LOGDEBUG(L"force_unset_RPM_folder: wait for processid %ld time out, return", process_id);
		return;
	}

	if (pHandle == NULL || waitRet == WAIT_OBJECT_0)
	{
        if (pHandle)
            ::CloseHandle(pHandle);

		filepath = NX::fs::dos_fullfilepath(filepath).path();

		bool ret = false;
		std::vector<std::pair<unsigned long, std::wstring>> protected_processes;
		if (GLOBAL.get_process_cache().does_protected_process_exist(session_id)) {
			protected_processes = GLOBAL.get_process_cache().find_all_protected_process(session_id);
		}

		try 
		{
			if (!IsPDPProcessEx(protected_processes))
			{
				std::set<unsigned long> pidSet;
				GetNonPDPProcessIdEx(protected_processes, pidSet);
				if (IsFileInMarkDirOpened(pidSet, filepath + L'\\'))
				{
					LOGDEBUG(L"force_unset_RPM_folder: remove RPM folder for %s file still opend, return", filepath);
					return;
				}
			}
			ret = sp->get_rm_session().remove_directory_action(filepath);
			if (ret)
			{
                std::wstring wsid = sp->get_windows_user_sid();
                std::wstring tenantid = sp->get_rm_session().get_tenant_id();
                std::wstring userid = sp->get_rm_session().get_profile().get_email();
                userid = tenantid + L"\\" + userid;
                unsigned long val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_REMOVE, L"{}", wsid, userid);
				if (val == 0)
					sp->get_rm_session().update_client_dir_config(get_dirs(), LOGIN_INFO::RPMFolder);
				else
				{
					if (val == ERROR_NOT_FOUND)
					{
						LOGDEBUG(L"force_unset_RPM_folder: AddDirectoryConfig for %s failed", filepath);
						return;
					}
					else
					{
						LOGDEBUG(L"force_unset_RPM_folder: AddDirectoryConfig for %s failed", filepath);
						return;
					}
				}
			}
			else
			{
				LOGDEBUG(L"force_unset_RPM_folder: remove_directory_action for %s failed", filepath);
				return;
			}
		}
		catch (const std::exception& e) {
			LOGDEBUG(L"force_unset_RPM_folder: remove RPM folder for %s cache exception, %s", filepath, e.what());
			return;
		}
	}
}

std::string rmserv::on_request_set_policy_bundle(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	try {
		std::wstring policyData = parameters.serialize();
		std::map<std::wstring, std::wstring> policy_map;

		if (parameters.as_object().has_field(L"policyConfigData"))
		{
			const NX::json_array& policy_array = parameters.as_object().at(L"policyConfigData").as_array();
			for (NX::json_array::const_iterator it = policy_array.begin(); it != policy_array.end(); ++it) 
			{
				std::wstring tokenGroupName = it->as_object().at(L"tokenGroupName").as_string();
				std::wstring bundle = it->as_object().at(L"policyBundle").as_string();
				policy_map[tokenGroupName] = bundle;
				sp->get_rm_session().update_policy(policy_map, policyData);
				LOGDEBUG(NX::string_formater(L"on_request_set_policy_bundle:  tokenGroupName: %s", tokenGroupName.c_str()));
			}
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: no policyConfigData data");
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_set_service_stop(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	try {
		if (!VerifySecurityString(parameters, response))
		{
			return std::move(response);
		}

		bool enable=false;
		if (parameters.as_object().has_field(L"enable"))
		{
			enable = parameters.as_object().at(L"enable").as_boolean();
			ServiceStop(enable);
			
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Cannot find service field");
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_set_service_stop_no_security(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	try {
		bool enable=false;
		if (parameters.as_object().has_field(L"enable"))
		{
			enable = parameters.as_object().at(L"enable").as_boolean();
			ServiceStop(enable);

			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Cannot find service field");
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_stop_service_no_security(unsigned long session_id, unsigned long process_id)
{
	std::string response;
	try {
		long ret = control(SERVICE_CONTROL_STOP, 0, NULL, NULL);

		response = NX::string_formater("{\"code\":%ld, \"message\":\"%s\"}", ret, "succeed");
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_set_clientid(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	
	try {
		
		std::wstring clientid;
		if (parameters.as_object().has_field(L"clientid"))
		{
			clientid = parameters.as_object().at(L"clientid").as_string();			
			_client_id = clientid;
			set_client_id(clientid);
			if (!sp->get_rm_session().update_client_config(clientid))
				LOGERROR(NX::string_formater(L" update_client_config failed:  clientid: %s", clientid.c_str()));
			
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: no clientid data");
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_get_secret_dir(unsigned long session_id, unsigned long process_id)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		std::string intermediate_dir = NX::conversion::utf16_to_utf8(sp->get_intermediate_dir());
		if (intermediate_dir.size() < 3)
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_PATH_NOT_FOUND, "Path not found");
			LOGDEBUG(NX::string_formater("on_request_get_secret_dir: response: %s", response.c_str()));
		}
		else
		{
			intermediate_dir.pop_back();
			intermediate_dir.pop_back();
			intermediate_dir.pop_back();

			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, intermediate_dir.c_str());
			std::wstring s;
			int val = 0;
			NX::json_value v = NX::json_value::create_object();
			v[L"code"] = NX::json_value(val);
			v[L"message"] = NX::json_value(NX::conversion::utf8_to_utf16(intermediate_dir));
			s = v.serialize();
			response = NX::conversion::utf16_to_utf8(s).c_str();
		}
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
		LOGDEBUG(NX::string_formater("on_request_get_secret_dir: response: %s", response.c_str()));
	}

	//LOGDEBUG(NX::string_formater("on_request_get_secret_dir: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_set_router(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (is_server_mode())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "Machine is under server mode");
		return std::move(response);
	}

	try {
		std::wstring router;
		std::wstring tenant;
		std::wstring sdklibfolder;
		std::wstring data;
		if (parameters.as_object().has_field(L"router"))
		{
			router = parameters.as_object().at(L"router").as_string();
			SERV->get_router_config().set_router(router);	
			sp->get_rm_session().update_client_dir_config(router, LOGIN_INFO::routerAddr);
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: no router data");
		if (parameters.as_object().has_field(L"tenant"))
		{
			tenant = parameters.as_object().at(L"tenant").as_string();
			SERV->get_router_config().set_tenant_id(tenant);
			sp->get_rm_session().update_client_dir_config(tenant, LOGIN_INFO::tenantId);
		}
		if (parameters.as_object().has_field(L"sdklibfolder"))
		{
			sdklibfolder = parameters.as_object().at(L"sdklibfolder").as_string();
			sp->get_rm_session().update_client_dir_config(sdklibfolder, LOGIN_INFO::sdklibFolder);
		}
		if (parameters.as_object().has_field(L"workingfolder"))
		{
			data = parameters.as_object().at(L"workingfolder").as_string();
			sp->get_rm_session().update_client_dir_config(data, LOGIN_INFO::workingFolder);
		}
		if (parameters.as_object().has_field(L"tempfolder"))
		{
			data = parameters.as_object().at(L"tempfolder").as_string();
			sp->get_rm_session().update_client_dir_config(data, LOGIN_INFO::tempFolder);
		}

		LOGDEBUG(NX::string_formater(L"on_request_set_router: router: %s, %s, %s", router.c_str(), tenant.c_str(), sdklibfolder.c_str()));
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_is_app_registered(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;
	std::vector<std::pair<unsigned long, std::wstring>> protected_processes;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}


	try {
		std::wstring appPath;

		if (!parameters.as_object().has_field(L"appPath"))
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "appPath not found");
			return std::move(response);
		}

		appPath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"appPath").as_string()).path();
		LOGDEBUG(NX::string_formater(L"on_request_is_app_registered: appPath=%s", appPath.c_str()));

		bool status = false;
		int code = 0;

		if (RegisteredAppsCheckExist(appPath))
		{
			status = true;
		}
		// app in osrmx whitelist should return true
		else if(nx::capp_whiltelist_config::getInstance()->is_app_in_whitelist(appPath))
		{
			LOGDEBUG(NX::string_formater(L"on_request_is_app_registered: osrmx whitelist appPath=%s", appPath.c_str()));
			status = true;
		}

		NX::json_value v = NX::json_value::create_object();
		v[L"code"] = NX::json_value(code);
		v[L"message"] = NX::json_value(L"succeed");
		v[L"register"] = NX::json_value(status);

		std::wstring s = v.serialize();
		response = NX::conversion::utf16_to_utf8(s).c_str();

	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to parse parameters\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_get_protected_profiles_dir(unsigned long session_id, unsigned long process_id)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	//if (!sp->get_rm_session().is_logged_on()) {
	//	response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
	//	return std::move(response);
	//}
	try {
		std::string protected_profiles_dir = NX::conversion::utf16_to_utf8(sp->get_protected_profiles_dir());
		if (protected_profiles_dir.size() < 2)
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_PATH_NOT_FOUND, "Path not found");
			LOGDEBUG(NX::string_formater("on_request_get_protected_profiles_dir: response: %s", response.c_str()));
		}
		else
		{
			std::wstring s;
			int val = 0;
			NX::json_value v = NX::json_value::create_object();
			v[L"code"] = NX::json_value(val);
			v[L"path"] = NX::json_value(sp->get_protected_profiles_dir());
			s = v.serialize();
			response = NX::conversion::utf16_to_utf8(s).c_str();
		}
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
		LOGDEBUG(NX::string_formater("on_request_get_protected_profiles_dir: response: %s", response.c_str()));
	}

	//LOGDEBUG(NX::string_formater("on_request_get_protected_profiles_dir: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_query_apiuser(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	try {
		// if (VerifySecurityString(parameters, response))
		{
			if (is_server_mode() == true)
			{
				std::wstring s;
				NX::json_value v = NX::json_value::create_object();
				v[L"statusCode"] = NX::json_value(200);
				v[L"serverTime"] = NX::json_value((uint64_t)std::time(nullptr));
				v[L"message"] = NX::json_value(L"Authorized");
				v[L"extra"] = NX::json_value::parse(_server_login_data);

				s = v.serialize();
				response = NX::conversion::utf16_to_utf8(s).c_str();
			}
			else
			{
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "User not login");
			}
		}
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_query_apiuser: response: %s", response.c_str()));
	return std::move(response);
}

std::set<std::wstring> rmserv::RegisteredAppsGet(void)
{
    std::set<std::wstring> apps;

    ::EnterCriticalSection(&_lock);
    apps = m_registeredApps;
    ::LeaveCriticalSection(&_lock);
    return apps;
}

bool rmserv::RegisteredAppsCheckExist(const std::wstring& appPath)
{
    bool ret;

    ::EnterCriticalSection(&_lock);

	std::wstring s = NX::fs::dos_fullfilepath(appPath).path();

    ret = (m_registeredApps.find(NX::conversion::wcslower(s)) !=
           m_registeredApps.end());

    ::LeaveCriticalSection(&_lock);
    return ret;
}

void rmserv::RegisteredAppsAdd(const std::wstring& appPath)
{
    ::EnterCriticalSection(&_lock);

    std::wstring s = NX::fs::dos_fullfilepath(appPath).path();

    m_registeredApps.insert(NX::conversion::wcslower(s));//

    ::LeaveCriticalSection(&_lock);
}

bool rmserv::RegisteredAppsRemove(const std::wstring& appPath)
{
    bool ret;

    ::EnterCriticalSection(&_lock);

	std::wstring s = NX::fs::dos_fullfilepath(appPath).path();

    ret = (m_registeredApps.erase(NX::conversion::wcslower(s)) > 0);

    ::LeaveCriticalSection(&_lock);
    return ret;
}

bool rmserv::TrustedProcessIdsCheckExist(unsigned long processId)
{
    bool ret;

/*#ifdef _DEBUG  // test without trust process
	return true;
#endif */

    ::EnterCriticalSection(&_lock);
    ret = (m_trustedProcessIds.find(processId) != m_trustedProcessIds.end());
    ::LeaveCriticalSection(&_lock);
    return ret;
}

void rmserv::TrustedProcessIdsAdd(unsigned long processId, unsigned int sessionId)
{
    LOGDEBUG(NX::string_formater(L"TrustedProcessIdsAdd: processId=%lu", processId));
    ::EnterCriticalSection(&_lock);
    m_trustedProcessIds[processId] = sessionId;
    ::LeaveCriticalSection(&_lock);
}

unsigned int rmserv::GetSessionIdOfProcess(unsigned long processId)
{
	unsigned int sessionId = 0;
	::EnterCriticalSection(&_lock);
	const auto &appEntry = m_trustedProcessIds.find(processId);
	if (appEntry != m_trustedProcessIds.end()) {
		sessionId = appEntry->second;
	}
	::LeaveCriticalSection(&_lock);

	return sessionId;
}

bool rmserv::TrustedProcessIdsRemove(unsigned long processId)
{
    bool ret;

    LOGDEBUG(NX::string_formater(L"TrustedProcessIdsRemove: processId=%lu", processId));
    ::EnterCriticalSection(&_lock);
    ret = (m_trustedProcessIds.erase(processId) > 0);
    ::LeaveCriticalSection(&_lock);
    return ret;
}

void rmserv::EffectiveTrustedAppsResolve(void)
{
    const std::set<std::wstring>& appsInXmlFile = _serv_conf.get_trusted_apps();
    const std::map<std::wstring, std::vector<unsigned char>>& appsInConfig = m_trustedAppsFromConfig;

    m_effectiveTrustedApps.clear();

    if (appsInConfig.empty()) {
        // It is one of the following cases:
        // - The internal config does not exist.
        // - We have failed to load the internal config.
        //
        // In all cases, we take all of the apps specified in the XML file as
        // the effective trusted apps.
        for (const auto &app : appsInXmlFile) {
            std::vector<unsigned char> hash;
            if (IsAppInIgnoreSignatureDirs(app) || GetFileHash(app, hash)) {
                m_effectiveTrustedApps[app] = hash;
            }
        }
    } else {
        // We have loaded the internal config successfully.
        //
        // We resolve between apps in the XML file and apps in the internal
        // config as follow:
        //
        //                             App in XML file?
        //             | No                  | Yes                   |
        //         ----+---------------------+-----------------------+
        //         No  | Untrusted.          | Trusted, get new hash |
        //             |                     | from EXE file.  Will  |
        // App in      |                     | add to config.        |
        // config? ----+---------------------+-----------------------+
        //         Yes | Untrusted.  Will    | Trusted, use existing |
        //             | remove from config. | hash in config.       |
        //         ----+---------------------+-----------------------+
        for (const auto &app : appsInConfig) {
            if (appsInXmlFile.find(app.first) != appsInXmlFile.end()) {
                // Trusted, use existing hash in config.
                m_effectiveTrustedApps.insert(app);
            }
        }

        for (const auto &app : appsInXmlFile) {
            if (appsInConfig.find(app) == appsInConfig.end()) {
                // Trusted, get new hash from EXE file.  Will add to config
                // later.
                std::vector<unsigned char> hash;
                if (IsAppInIgnoreSignatureDirs(app) || GetFileHash(app, hash)) {
                    m_effectiveTrustedApps[app] = hash;
                }
            }
        }
    }
}

bool rmserv::EffectiveTrustedAppsCheckExist(const std::wstring& appPath)
{
	bool ret;

	::EnterCriticalSection(&_lock);

	std::wstring s = NX::fs::dos_fullfilepath(appPath).path();

	ret = m_effectiveTrustedApps.find(NX::conversion::wcslower(s)) != m_effectiveTrustedApps.end();

	::LeaveCriticalSection(&_lock);

	return ret;
}

void rmserv::ProcessNotification(unsigned long processId, const std::wstring& image_path, bool create)
{
	try {
		handle_whilelist_inherite(processId, image_path, create);
		if (!create) {
			ProcessExitNotification(processId, image_path);
			LOGDEBUG(NX::string_formater(L"ProcessNotification: processId=%lu, process=%s", processId, image_path.c_str()));
			TrustedProcessIdsRemove(processId);
		}
		else
		{
			LOGDEBUG(NX::string_formater(L"ProcessNotification: create processId=%lu, process=%s", processId, image_path.c_str()));
		}
	}
	catch (...)
	{
		LOGDEBUG(NX::string_formater(L"ProcessNotification: exception processId=%lu, process=%s", processId, image_path.c_str()));
	}
}

//		"pid":123456,
//			"application" : "C:\natopad.exe",
//			"isAppAllowEdit" : true,
//			"file" : "C:\Allen\work\skydrm\test.txt",
//			"isFileAllowEdit" : false

void rmserv::FileRightsNotification(unsigned long processId, std::wstring& strFile, ULONGLONG ullRights)
{
	if (!nx::capp_whiltelist_config::getInstance()->is_osrmx_active_process(processId))
		return;

	DWORD dwSessionId = 0;
	if (!ProcessIdToSessionId(processId, &dwSessionId))
		return;

	std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(dwSessionId);
	if (nullptr == sp)
		return;

	std::wstring strEvent = L"Global\\EVENT_osrmx_" + std::to_wstring(processId);
	HANDLE hEvent = ::OpenEventW(EVENT_MODIFY_STATE, FALSE, strEvent.c_str());
	LOGDEBUG(NX::string_formater(L"FileRightsNotification : OpenEvent, hEvent : processId=%d", processId));
	if (hEvent)
	{
		::SetEvent(hEvent);
		::CloseHandle(hEvent);
	}

	std::wstring strAppPath = NX::win::get_process_path(processId);
	std::transform(strAppPath.begin(), strAppPath.end(), strAppPath.begin(), ::tolower);
	////////////////////////////////////////
	// special code for adobe reader because of protected mode
	// now notify its parent that there is a NXL file open
	if (strAppPath.find(L"acrord32.exe") != strAppPath.npos)
	{
		DWORD dwParentProcessID = NX::win::get_parent_processid(processId);
		auto parentPath = NX::win::get_process_path(dwParentProcessID);
		std::transform(parentPath.begin(), parentPath.end(), parentPath.begin(), ::tolower);
		if (parentPath.find(L"acrord32.exe") != parentPath.npos)
		{
			// notify parent - acrord32
			std::wstring strEvent = L"Global\\EVENT_osrmx_" + std::to_wstring(dwParentProcessID);
			HANDLE hEvent = ::OpenEventW(EVENT_MODIFY_STATE, FALSE, strEvent.c_str());
			LOGDEBUG(NX::string_formater(L"FileRightsNotification : OpenEvent, hEvent : dwParentProcessID =%d", dwParentProcessID));
			if (hEvent)
			{
				::SetEvent(hEvent);
				::CloseHandle(hEvent);
			}
		}
	}
	////////////////////////////////////////

	bool isFileAllowEdit = false;
	if (ullRights & BUILTIN_RIGHT_EDIT)
		isFileAllowEdit = true;

	bool bAppAllowEdit = nx::capp_whiltelist_config::getInstance()->is_app_can_save(strAppPath);

	NX::json_value v = NX::json_value::create_object();
	v[L"pid"] = NX::json_value((unsigned int)processId);
	v[L"application"] = NX::json_value(strAppPath);
	v[L"isAppAllowEdit"] = NX::json_value(bAppAllowEdit);
	v[L"file"] = NX::json_value(strFile);
	v[L"isFileAllowEdit"] = NX::json_value(isFileAllowEdit);

	std::wstring strJsonMessage = v.serialize();
	sp->get_rm_session().get_app_manager().send_nxlfile_rights_notification(strJsonMessage);
}

//In RPM Service, we shall cache the denied files + process.If same file with
//same process is denied in 5 seconds, we just show one notification.

//Of course, we only cache for applications in our whitelist + some famous RMX
//enabled application(Office, NX, Creo, Solidwork).
void rmserv::handle_nxl_denied_file(unsigned long processId, std::wstring& strFile)
{
	if (!IsProcessTrusted(processId))
	{
		LOGDEBUG(NX::string_formater(L"handle_nxl_denied_file : processId=%lu is not trusted", processId));
		return;
	}

	std::wstring strKey = gen_denied_nxlfile_key(processId, strFile);
	if (strKey.empty())
	{
		return;
	}

	DWORD dwSessionId = 0;
	if (!ProcessIdToSessionId(processId, &dwSessionId))
		return;

	bool bNeedNotify = update_denied_nxlfile_map(strKey);
	if (bNeedNotify)
	{
		uint64_t u64Tick = ::GetTickCount64();
		LOGINFO(NX::string_formater(L"rmserv::handle_nxl_denied_file : key = %s tick=%llu", strKey.c_str(), u64Tick));
		std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(dwSessionId);
		if (nullptr == sp)
		{
			LOGERROR(NX::string_formater(L"rmserv::handle_nxl_denied_file nullptr == sp : key = %s tick=%llu", strKey.c_str(), u64Tick));
			return;
		}

		std::wstring strAppPath = NX::win::get_process_path(processId);
		auto pos = strAppPath.rfind(L"\\");

		std::wstring strAppName;
		if (std::wstring::npos != pos)
		{
			strAppName = strAppPath.substr(pos + 1);
		}

        //
        // fix #63330, not all denied files are the files opened by the user. some files are history files, but application still calls CreateFile on these history files
        //
		//sp->get_rm_session().block_notify(strFile, strAppName);
	}
}

void rmserv::ProcessExitNotification(unsigned long processId, const std::wstring& image_path)
{
	//if (!nx::capp_whiltelist_config::getInstance()->remove_osrmx_process(processId))
		//return;

	nx::capp_whiltelist_config::getInstance()->remove_osrmx_process(processId);

	DWORD dwSessionId = 0;
	if (!ProcessIdToSessionId(processId, &dwSessionId))
	{
		dwSessionId = GetSessionIdOfProcess(processId);
		if (dwSessionId == 0)
			return;
	}

	std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(dwSessionId);
	if (nullptr == sp)
		return;

	std::wstring strAppPath = NX::fs::dos_filepath(image_path).path();// NX::win::get_process_path(processId);

	NX::json_value v = NX::json_value::create_object();
	v[L"pid"] = NX::json_value((unsigned int)processId);
	v[L"application"] = NX::json_value(strAppPath);

	std::wstring strJsonMessage = v.serialize();
	sp->get_rm_session().get_app_manager().send_process_exit_notificatoin(strJsonMessage);
}

bool rmserv::IsAppInIgnoreSignatureDirs(const std::wstring& appPath)
{
    std::wstring ignoreDir;

    ignoreDir = GLOBAL.get_windows_info().system_windows_dir() + L"\\";
    if (boost::algorithm::istarts_with(appPath, ignoreDir))
        return true;

    ignoreDir = GLOBAL.get_windows_info().program_files_dir() + L"\\";
    if (boost::algorithm::istarts_with(appPath, ignoreDir))
        return true;

#ifdef _WIN64
    ignoreDir = GLOBAL.get_windows_info().program_files_x86_dir() + L"\\";
    if (boost::algorithm::istarts_with(appPath, ignoreDir))
        return true;
#endif

    return false;
}

// VerifyEmbeddedSignature()
// Returns true if the file is signed and the signature was verified.
// Returns false otherwise.  *pStatus contains the error code.
//
// Adapted from https://docs.microsoft.com/en-us/windows/desktop/SecCrypto/example-c-program--verifying-the-signature-of-a-pe-file
bool rmserv::VerifyEmbeddedSignature(const std::wstring& sourceFile, LONG *pStatus, unsigned long sessionid)
{
    DWORD dwLastError;

    NX::fs::dos_fullfilepath input_filepath(sourceFile);
    if (IsAppInIgnoreSignatureDirs(input_filepath.path()))
        return true;

	NX::win::session_token st(sessionid);
	NX::win::impersonate_object impersonobj(st);


    // Initialize the WINTRUST_FILE_INFO structure.

    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
	FileData.pcwszFilePath = sourceFile.c_str(); 
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    /*
    WVTPolicyGUID specifies the policy to apply on the file
    WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:

    1) The certificate used to sign the file chains up to a root
    certificate located in the trusted root certificate store. This
    implies that the identity of the publisher has been verified by
    a certification authority.

    2) In cases where user interface is displayed (which this example
    does not do), WinVerifyTrust will check for whether the
    end entity certificate is stored in the trusted publisher store,
    implying that the user trusts content from this publisher.

    3) The end entity certificate has sufficient permission to sign
    code, as indicated by the presence of a code signing EKU or no
    EKU.
    */

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;

    // Initialize the WinVerifyTrust input data structure.

    // Default all fields to 0.
    memset(&WinTrustData, 0, sizeof(WinTrustData));

    WinTrustData.cbStruct = sizeof(WinTrustData);

    // Use default code signing EKU.
    WinTrustData.pPolicyCallbackData = NULL;

    // No data to pass to SIP.
    WinTrustData.pSIPClientData = NULL;

    // Disable WVT UI.
    WinTrustData.dwUIChoice = WTD_UI_NONE;

    // No revocation checking.
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;

    // Verify an embedded signature on a file.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

    // Verify action.
    WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;

    // Verification sets this value.
    WinTrustData.hWVTStateData = NULL;

    // Not used.
    WinTrustData.pwszURLReference = NULL;

    // This is not applicable if there is no UI because it changes
    // the UI to accommodate running applications instead of
    // installing applications.
    WinTrustData.dwUIContext = 0;

    // Set pFile.
    WinTrustData.pFile = &FileData;

	// Set CRL check
	WinTrustData.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;

    // WinVerifyTrust verifies signatures as specified by the GUID
    // and Wintrust_Data.
    *pStatus = WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    switch (*pStatus)
    {
        case ERROR_SUCCESS:
            /*
            Signed file:
                - Hash that represents the subject is trusted.

                - Trusted publisher without any verification errors.

                - UI was disabled in dwUIChoice. No publisher or
                    time stamp chain errors.

                - UI was enabled in dwUIChoice and the user clicked
                    "Yes" when asked to install and run the signed
                    subject.
            */
            LOGINFO(NX::string_formater(L"The file \"%s\" is signed and the signature "
                L"was verified.\n",
                sourceFile.c_str()));
            break;

        case TRUST_E_NOSIGNATURE:
            // The file was not signed or had a signature
            // that was not valid.

            // Get the reason for no signature.
            dwLastError = GetLastError();
            if (TRUST_E_NOSIGNATURE == dwLastError ||
                    TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
                    TRUST_E_PROVIDER_UNKNOWN == dwLastError)
            {
                // The file was not signed.
                LOGERROR(NX::string_formater(L"The file \"%s\" is not signed.\n",
                sourceFile.c_str()));
            }
            else
            {
                // The signature was not valid or there was an error
                // opening the file.
                LOGERROR(NX::string_formater(L"An unknown error occurred trying to "
                    L"verify the signature of the \"%s\" file.\n",
                    sourceFile.c_str()));
            }

            break;

        case TRUST_E_EXPLICIT_DISTRUST:
            // The hash that represents the subject or the publisher
            // is not allowed by the admin or user.
            LOGERROR(NX::string_formater(L"The signature is present, but specifically "
                L"disallowed.\n"));
            break;

        case TRUST_E_SUBJECT_NOT_TRUSTED:
            // The user clicked "No" when asked to install and run.
            LOGERROR(NX::string_formater(L"The signature is present, but not "
                L"trusted.\n"));
            break;

        case CRYPT_E_SECURITY_SETTINGS:
            /*
            The hash that represents the subject or the publisher

            publisher or time stamp errors.
            */
            LOGERROR(NX::string_formater(L"CRYPT_E_SECURITY_SETTINGS - The hash "
                L"representing the subject or the publisher wasn't "
                L"explicitly trusted by the admin and admin policy "
                L"has disabled user trust. No signature, publisher "
                L"or timestamp errors.\n"));
            break;

        default:
            // The UI was disabled in dwUIChoice or the admin policy
            // has disabled user trust. *pStatus contains the
            // publisher or time stamp chain error.
            LOGERROR(NX::string_formater(L"Error is: 0x%x.\n",
                *pStatus));
            break;
    }

    // Any hWVTStateData must be released by a call with close.
    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

    LONG lStatus2 = WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    return *pStatus == 0;
}

bool rmserv::GetFileHash(const std::wstring& sourceFile, std::vector<unsigned char> &hash)
{
    MSIFILEHASHINFO fileHash = {sizeof(MSIFILEHASHINFO)};
    UINT ret = MsiGetFileHash(sourceFile.c_str(), 0, &fileHash);

    if (ret != ERROR_SUCCESS) {
        LOGERROR(NX::string_formater(L"rmserv::GetFileHash: MsiGetFileHash(\"%s\") failed, ret=%u\n", sourceFile.c_str(), ret));
        return false;
    }

    hash.resize(sizeof fileHash.dwData);
    ULONG *p = (ULONG *) hash.data();
    for (size_t i = 0; i < _countof(fileHash.dwData); i++) {
        p[i] = fileHash.dwData[i];
    }

    LOGINFO(NX::string_formater(L"rmserv::GetFileHash: Getting hash of \"%s\" successful\n", sourceFile.c_str()));
    return true;
}

void rmserv::handle_whilelist_inherite(unsigned long ulProcessID, const std::wstring& image_path, bool bCreate)
{
	if (!bCreate)
		return;

	auto appPath = NX::fs::dos_fullfilepath(image_path).path();
	if (appPath.empty())
		return;

	std::transform(appPath.begin(), appPath.end(), appPath.begin(), ::towlower);
	if (nx::capp_whiltelist_config::getInstance()->is_app_in_whitelist(appPath))
	{
		nx::capp_whiltelist_config::getInstance()->insert_osrmx_process(ulProcessID);
		return;
	}

	DWORD dwParentProcessID = NX::win::get_parent_processid(ulProcessID);
	while (true)
	{
		auto parentPath = NX::fs::dos_fullfilepath(NX::win::get_process_path(dwParentProcessID)).path();
		if (parentPath.empty())
			return;

		std::transform(parentPath.begin(), parentPath.end(), parentPath.begin(), ::towlower);
		if (!TrustedProcessIdsCheckExist(dwParentProcessID))
		{
			return;
		}

		if (nx::capp_whiltelist_config::getInstance()->is_app_in_whitelist(parentPath)
			&& nx::capp_whiltelist_config::getInstance()->is_app_can_inherit(parentPath))
		{
			DWORD added_session_id = 0;
			ProcessIdToSessionId(ulProcessID, &added_session_id);
			TrustedProcessIdsAdd(ulProcessID, added_session_id);
			nx::capp_whiltelist_config::getInstance()->insert_osrmx_process(ulProcessID);
			return;
		}

		dwParentProcessID = NX::win::get_parent_processid(dwParentProcessID);
	}
}

std::string rmserv::handle_request_opened_file_rights(unsigned long session_id, unsigned long ulProcessID)
{
	LOGINFO(NX::string_formater(L"rmserv::handle_request_opened_file_rights enter: process-id = %d", ulProcessID));
	std::map<std::wstring, std::wstring> mapFileRights;
	bool bRet = GLOBAL.get_process_cache().get_process_file_rights(ulProcessID, mapFileRights);
	if (!bRet)
	{
		std::string response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "Error: the process not exist");
		return std::move(response);
	}

	std::wstring strArrFileRights;
	for (auto item : mapFileRights)
	{
		strArrFileRights += L",";
		std::wstring strObj = item.second;
		strArrFileRights += strObj;
	}

	std::wstring strJsonArr;
	if (strArrFileRights.size() > 3)
	{
		strJsonArr = strArrFileRights.substr(1);
	}
	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	std::wstring strJson = L"{ \"code\": 0, \"message\":\"success\", \"user\": \"" + sp->get_rm_session().get_profile().get_email() + L"\", \"filerights\": [" + strJsonArr + L"] }";

	LOGINFO(NX::string_formater(L"rmserv::handle_request_opened_file_rights leave: %s", strJson.c_str()));
	return std::move(NX::conversion::utf16_to_utf8(strJson));
}

bool rmserv::update_denied_nxlfile_map(const std::wstring& strKey)
{
	::EnterCriticalSection(&_lock);
	bool bNeedNotify = false;
	uint64_t u64Tick = ::GetTickCount64();
	auto itFind = m_mapDeniedNxlFile.find(strKey);
	if (m_mapDeniedNxlFile.end() == itFind)
	{
		m_mapDeniedNxlFile[strKey] = u64Tick;
		bNeedNotify = true;
	}
	else
	{
		uint64_t u64Distance = u64Tick - itFind->second;
		if (u64Distance > 5000)
		{
			itFind->second = u64Tick;
			bNeedNotify = true;
		}
	}

	std::map<std::wstring, uint64_t>::iterator it = m_mapDeniedNxlFile.begin();
	while (m_mapDeniedNxlFile.end() != it)
	{
		if (u64Tick - it->second > 12000)
		{
			it = m_mapDeniedNxlFile.erase(it);
		}
		else
		{
			it++;
		}
	}
	::LeaveCriticalSection(&_lock);

	return bNeedNotify;
}

std::wstring rmserv::gen_denied_nxlfile_key(unsigned long processId, std::wstring& strFile)
{
	std::wstring strNxlFile = NX::fs::dos_fullfilepath(strFile).path();
	strNxlFile.erase(0, strNxlFile.find_first_not_of(L" "));
	strNxlFile.erase(strNxlFile.find_last_not_of(L" ") + 1);
	if (strNxlFile.empty())
		return L"";

	std::wstring strAppPath = NX::fs::dos_fullfilepath(NX::win::get_process_path(processId)).path();
	strAppPath.erase(0, strAppPath.find_first_not_of(L" "));
	strAppPath.erase(strAppPath.find_last_not_of(L" ") + 1);
	if (strAppPath.empty())
		return L"";

	std::transform(strNxlFile.begin(), strNxlFile.end(), strNxlFile.begin(), ::towlower);
	std::transform(strAppPath.begin(), strAppPath.end(), strAppPath.begin(), ::towlower);

	static std::array<std::wstring, 3> arrApp = { L"winword.exe", L"powerpnt.exe", L"excel.exe"}; //, L"ugraf.exe" , L"xtop.exe", L"sldworks.exe" 
	bool bInWhitelist = nx::capp_whiltelist_config::getInstance()->is_app_in_whitelist(strAppPath);
	if (!bInWhitelist)
	{
		auto pos = strAppPath.rfind(L"\\");
		if (std::wstring::npos == pos)
			return L"";

		std::wstring strAppName = strAppPath.substr(pos + 1);
		for (auto item : arrApp)
		{
			if (0 == strAppName.compare(item))
			{
				std::wstring strKey = strNxlFile + L"|" + strAppPath;
				return std::move(strKey);
			}
		}

		return L"";
	}
	else
	{
		std::wstring strKey = strNxlFile + L"|" + strAppPath;
		return std::move(strKey);
	}
}

bool rmserv::NonPersistentTrustedAppPathsCheckExist(const std::wstring& appPath)
{
    bool ret;

    ::EnterCriticalSection(&_lock);

    std::wstring s = NX::fs::dos_fullfilepath(appPath).path();

    ret = (m_nonPersistentTrustedAppPaths.find(NX::conversion::wcslower(s)) !=
           m_nonPersistentTrustedAppPaths.end());

    ::LeaveCriticalSection(&_lock);
    return ret;
}

void rmserv::NonPersistentTrustedAppPathsAdd(const std::wstring& appPath)
{
    LOGDEBUG(NX::string_formater(L"NonPersistentTrustedAppPathsAdd: appPath=%s", appPath.c_str()));
    ::EnterCriticalSection(&_lock);

	std::wstring s = NX::fs::dos_fullfilepath(appPath).path();

    m_nonPersistentTrustedAppPaths.insert(NX::conversion::wcslower(s));

    ::LeaveCriticalSection(&_lock);
}

bool rmserv::NonPersistentTrustedAppPathsRemove(const std::wstring& appPath)
{
    bool ret;

    LOGDEBUG(NX::string_formater(L"NonPersistentTrustedAppPathsRemove: appPath=%s", appPath.c_str()));
    ::EnterCriticalSection(&_lock);

	std::wstring s = NX::fs::dos_fullfilepath(appPath).path();

    ret = (m_nonPersistentTrustedAppPaths.erase(NX::conversion::wcslower(s)) > 0);

    ::LeaveCriticalSection(&_lock);
    return ret;
}

std::string rmserv::on_request_register_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        if (!VerifySecurityString(parameters, response))
        {
            return std::move(response);
        }

        std::wstring appPath;
        if (!parameters.as_object().has_field(L"appPath"))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "appPath not found");
            return std::move(response);
        }

		appPath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"appPath").as_string()).path();
		LOGDEBUG(NX::string_formater(L"on_request_register_app: appPath=%s", appPath.c_str()));

        LONG status;
        if (!VerifyEmbeddedSignature(appPath, &status, session_id))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", status, "application cannot be verified");
            return std::move(response);
        }

        RegisteredAppsAdd(appPath);
        sp->get_rm_session().update_app_whitelist_config(RegisteredAppsGet());
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to register app\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_unregister_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        if (!VerifySecurityString(parameters, response))
        {
            return std::move(response);
        }

        std::wstring appPath;
        if (!parameters.as_object().has_field(L"appPath"))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "appPath not found");
            return std::move(response);
        }

		appPath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"appPath").as_string()).path();
		LOGDEBUG(NX::string_formater(L"on_request_unregister_app: appPath=%s", appPath.c_str()));

        if (!RegisteredAppsRemove(appPath))
        {
            LOGERROR(NX::string_formater(L"on_request_unregister_app: %s not previously registered", appPath.c_str()));
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NOT_FOUND, "application not previously registered");
            return std::move(response);
        }

        sp->get_rm_session().update_app_whitelist_config(RegisteredAppsGet());
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to unregister app\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_notify_rmx_status(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        if (!VerifySecurityString(parameters, response))
        {
            return std::move(response);
        }

        bool running;
        const process_record& proc_record = GLOBAL.safe_find_process(process_id);
        if (proc_record.empty()) {
            response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to notify RMX status\"}", ERROR_INTERNAL_ERROR);
            return std::move(response);
        }
		const std::wstring image_path = NX::fs::dos_fullfilepath(proc_record.get_image_path()).path();
        if (!nx::capp_whiltelist_config::getInstance()->is_app_in_whitelist(image_path))
        {
            if (!RegisteredAppsCheckExist(image_path) && !EffectiveTrustedAppsCheckExist(image_path))
            {
                response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_ACCESS_DENIED, "caller app not registered");
                return std::move(response);
            }
        }

        if (!parameters.as_object().has_field(L"running"))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "running not found");
            return std::move(response);
        }

        running = parameters.as_object().at(L"running").as_boolean();
        LOGDEBUG(NX::string_formater(L"on_request_notify_rmx_status: running=%s", running ? L"true" : L"false"));

        if (running)
        {
            LONG status;
            if (!VerifyEmbeddedSignature(image_path, &status, session_id))
            {
                response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", status, "application cannot be verified");
                return std::move(response);
            }
            TrustedProcessIdsAdd(process_id, session_id);
        }
        else
        {
            if (!TrustedProcessIdsRemove(process_id))
            {
                LOGERROR(NX::string_formater(L"on_request_notify_rmx_status: RMX not previously running in process ID=%lu", process_id));
                response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NOT_FOUND, "RMX not previously running");
                return std::move(response);
            }
        }

		SERV->get_fltserv().clean_process_cache(process_id);

		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to notify RMX status\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_add_trusted_process(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        if (!VerifySecurityString(parameters, response))
        {
            return std::move(response);
        }

        unsigned long processIdToAdd;
        if (!TrustedProcessIdsCheckExist(process_id))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_ACCESS_DENIED, "caller process not trusted");
            return std::move(response);
        }

        if (!parameters.as_object().has_field(L"processId"))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "processId not found");
            return std::move(response);
        }

        processIdToAdd = parameters.as_object().at(L"processId").as_uint32();
        LOGDEBUG(NX::string_formater(L"on_request_add_trusted_process: processId=%lu", processIdToAdd));

		DWORD added_session_id = 0;
		ProcessIdToSessionId(processIdToAdd, &added_session_id);
		TrustedProcessIdsAdd(processIdToAdd, added_session_id);

		SERV->get_fltserv().clean_process_cache(processIdToAdd);

		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to add trusted process\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_remove_trusted_process(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        if (!VerifySecurityString(parameters, response))
        {
            return std::move(response);
        }

        unsigned long processIdToRemove;
        if (!TrustedProcessIdsCheckExist(process_id))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_ACCESS_DENIED, "caller process not trusted");
            return std::move(response);
        }

        if (!parameters.as_object().has_field(L"processId"))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "processId not found");
            return std::move(response);
        }

        processIdToRemove = parameters.as_object().at(L"processId").as_uint32();
        LOGDEBUG(NX::string_formater(L"on_request_remove_trusted_process: processId=%lu", processIdToRemove));

        if (!TrustedProcessIdsRemove(processIdToRemove))
        {
            LOGERROR(NX::string_formater(L"on_request_notify_rmx_status: process %lu not previously trusted", processIdToRemove));
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NOT_FOUND, "process not previously trusted");
            return std::move(response);
        }

        SERV->get_fltserv().clean_process_cache(processIdToRemove);

		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to remove trusted process\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_add_trusted_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        if (!VerifySecurityString(parameters, response))
        {
            return std::move(response);
        }

        std::wstring appPath;
        if (!TrustedProcessIdsCheckExist(process_id))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_ACCESS_DENIED, "caller process not trusted");
            return std::move(response);
        }

        if (!parameters.as_object().has_field(L"appPath"))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "appPath not found");
            return std::move(response);
        }

        appPath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"appPath").as_string()).path();
        LOGDEBUG(NX::string_formater(L"on_request_add_trusted_app: appPath=%s", appPath.c_str()));

        LONG status;
        if (!VerifyEmbeddedSignature(appPath, &status, session_id))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", status, "application cannot be verified");
            return std::move(response);
        }

        NonPersistentTrustedAppPathsAdd(appPath);
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to add trusted app\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_remove_trusted_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        if (!VerifySecurityString(parameters, response))
        {
            return std::move(response);
        }

        std::wstring appPath;
        if (!TrustedProcessIdsCheckExist(process_id))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_ACCESS_DENIED, "caller process not trusted");
            return std::move(response);
        }

        if (!parameters.as_object().has_field(L"appPath"))
        {
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_PARAMETER, "appPath not found");
            return std::move(response);
        }

        appPath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"appPath").as_string()).path();
        LOGDEBUG(NX::string_formater(L"on_request_remove_trusted_app: appPath=%s", appPath.c_str()));

        if (!NonPersistentTrustedAppPathsRemove(appPath))
        {
            LOGERROR(NX::string_formater(L"on_request_notify_rmx_status: app %s not previously trusted", appPath.c_str()));
            response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NOT_FOUND, "app not previously trusted");
            return std::move(response);
        }

        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to remove trusted app\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

std::string rmserv::on_request_set_app_registry(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	try {
		if (!VerifySecurityString(parameters, response))
		{
			return std::move(response);
		}

		std::wstring subkey;
		std::wstring name;
		std::wstring data;
		uint32_t op = 0;
		if (parameters.as_object().has_field(L"subkey"))
		{
			subkey = parameters.as_object().at(L"subkey").as_string();
		}
		else
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: no subkey data");
			return std::move(response);
		}

		if (parameters.as_object().has_field(L"name"))
		{
			name = parameters.as_object().at(L"name").as_string();
		}
		if (parameters.as_object().has_field(L"data"))
		{
			data = parameters.as_object().at(L"data").as_string();
		}
		if (parameters.as_object().has_field(L"operation"))
		{
			op = parameters.as_object().at(L"operation").as_uint32();
		}
		LOGDEBUG(NX::string_formater(L"on_request_set_app_registry: subkey: %s, name: %s, data: %s, op: %d", subkey.c_str(), name.c_str(), data.c_str(), op));
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");

		HKEY hKey;
		long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ | KEY_SET_VALUE, &hKey);
		if (lRet != ERROR_SUCCESS)
		{
			LOGERROR(NX::string_formater(L"RegOpenKeyEx: failed: %d", lRet));
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "RegOpenKeyEx failed");
			return std::move(response);
		}

		if (op == 0)
		{
			lRet = RegSetValueEx(hKey, name.c_str(), 0, REG_SZ, (const BYTE *)(data.c_str()), (DWORD)(data.size() + 1) * sizeof(WCHAR));
			if (lRet != ERROR_SUCCESS)
			{
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", lRet, "RegSetValueEx failed");
				LOGERROR(NX::string_formater(L"RegSetValueEx: failed: %s", name.c_str()));
			}
		}
		else
		{
			lRet = RegDeleteValueW(hKey, name.c_str());
			if (lRet != ERROR_SUCCESS)
			{
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", lRet, "RegDeleteValueW failed");
				LOGERROR(NX::string_formater(L"RegSetValueEx: failed: %s", name.c_str()));
			}
		}

		RegCloseKey(hKey);
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_run_registry_command(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	LOGINFO(NX::string_formater(L"rmserv::on_request_run_registry_command enter"));

	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		//
		// in SCCM mode, there is no windows session if installer call this API to register file handle
		// comment out following session check
		//

		/*response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		LOGERROR(NX::string_formater("Session not found : %s", response.c_str()));
		return std::move(response);*/
	}

	try {
		if (!VerifySecurityString(parameters, response))
		{
			LOGERROR(NX::string_formater("VerifySecurityString failed : %s", response.c_str()));
			return std::move(response);
		}

		NX::json_value  jsonRegistry;
		if (parameters.as_object().has_field(L"registry_call"))
		{
			jsonRegistry = parameters.as_object().at(L"registry_call");
		}
		else
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: not find registry_call");
			LOGERROR(NX::string_formater("registry_call not find : %s", response.c_str()));
			return std::move(response);
		}

		{
			//NX::win::session_token st(session_id);
			//NX::win::impersonate_object impersonobj(st);
			if (sp == nullptr)
			{
				cregistry_service_impl registry_service(L"");
				response = registry_service.handle_registry_command(jsonRegistry);
			}
			else
			{
				cregistry_service_impl registry_service(sp->get_windows_user_sid());
				response = registry_service.handle_registry_command(jsonRegistry);
			}
		}
		return std::move(response);
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\": \"%s\" }", ERROR_INVALID_DATA, e.what());
		LOGERROR(NX::string_formater("exception : %s", response.c_str()));
	}

	return std::move(response);
}

std::string rmserv::on_request_opened_file_rights(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	LOGINFO(NX::string_formater(L"rmserv::on_request_process_opened_file_rights enter"));

	std::string response;
	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		LOGERROR(NX::string_formater("Session not found : %s", response.c_str()));
		return std::move(response);
	}

	try {
		if (!VerifySecurityString(parameters, response))
		{
			LOGERROR(NX::string_formater("VerifySecurityString failed : %s", response.c_str()));
			return std::move(response);
		}

		unsigned long ulProcess = 0;
		if (parameters.as_object().has_field(L"process"))
		{
			ulProcess = parameters.as_object().at(L"process").as_uint32();
			response = handle_request_opened_file_rights(session_id, ulProcess);
		}
		else
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: not find process");
			LOGERROR(NX::string_formater("error invalid data, response : %s", response.c_str()));
			return std::move(response);
		}
		return std::move(response);
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\": \"%s\" }", ERROR_INVALID_DATA, e.what());
		LOGERROR(NX::string_formater("exception : %s", response.c_str()));
	}

	return std::move(response);
}

std::string rmserv::on_request_windows_encrypt_file(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters) {
	LOGINFO(NX::string_formater(L"rmserv::on_request_windows_encrypt_file enter"));
	std::string response;
	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		LOGERROR(NX::string_formater("Session not found : %s", response.c_str()));
		return std::move(response);
	}

	if (!sp->get_rm_session().is_logged_on())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}

	try {
		
		if (parameters.as_object().has_field(L"filePath")) {
			std::wstring fpath = parameters.as_object().at(L"filePath").as_string();
			std::wstring filepath = NX::fs::dos_fullfilepath(fpath).global_dos_path();
			// run under current user
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);

			if (EncryptFileW(filepath.c_str())) {

				NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
				std::pair<std::wstring, NX::json_value>(L"code", NX::json_value(0)),
				std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
				}), false);

				response = NX::conversion::utf16_to_utf8(v.serialize());
				LOGDEBUG(NX::string_formater(" process_request on_request_windows_encrypt_file: response (%s)", response.c_str()));
			}
			else {
				int le = GetLastError();
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", le, "failed to get file attributes");
				LOGDEBUG(NX::string_formater(" process_request on_request_windows_encrypt_file: failed, response (%s)", response.c_str()));
			}
		}
		else {
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: not find process");
			LOGERROR(NX::string_formater("error invalid data, response : %s", response.c_str()));
		}

		return std::move(response);
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\": \"%s\" }", ERROR_INVALID_DATA, e.what());
		LOGERROR(NX::string_formater("exception : %s", response.c_str()));
	}

	LOGDEBUG(NX::string_formater(" on_request_windows_encrypt_file: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_get_file_attributes(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters) {
	LOGINFO(NX::string_formater(L"rmserv::on_request_get_file_attributes enter"));
	std::string response;
	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		LOGERROR(NX::string_formater("Session not found : %s", response.c_str()));
		return std::move(response);
	}

	if (!sp->get_rm_session().is_logged_on())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}

	try {
		if (parameters.as_object().has_field(L"filePath")) {
			std::wstring fpath = parameters.as_object().at(L"filePath").as_string();
			std::wstring filepath = NX::fs::dos_fullfilepath(fpath).global_dos_path();
			// run under current user
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);
			DWORD res = GetFileAttributes(filepath.c_str());
			if (INVALID_FILE_ATTRIBUTES != res) {

				NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
					std::pair<std::wstring, NX::json_value>(L"code", NX::json_value(0)),
					std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
					std::pair<std::wstring, NX::json_value>(L"fileAttributes",NX::json_value((long long)res))
					}), false);

				response = NX::conversion::utf16_to_utf8(v.serialize());
				LOGDEBUG(NX::string_formater(" process_request on_request_get_file_attributes: response (%s)", response.c_str()));
			}
			else {
				int le = GetLastError();
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", le, "failed to get file attributes");
				LOGDEBUG(NX::string_formater(" process_request on_request_get_file_attributes: failed, response (%s)", response.c_str()));
			}
		}
		else
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: not find process");
			LOGERROR(NX::string_formater("error invalid data, response : %s", response.c_str()));
			return std::move(response);
		}

		return std::move(response);
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\": \"%s\" }", ERROR_INVALID_DATA, e.what());
		LOGERROR(NX::string_formater("exception : %s", response.c_str()));
	}

	LOGDEBUG(NX::string_formater(" on_request_get_file_attributes: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_set_file_attributes(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	LOGINFO(NX::string_formater(L"rmserv::on_request_set_file_attributes enter"));

	std::string response;
	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		LOGERROR(NX::string_formater("Session not found : %s", response.c_str()));
		return std::move(response);
	}

	if (!sp->get_rm_session().is_logged_on())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}

	try {
		if (parameters.as_object().has_field(L"filePath") && parameters.as_object().has_field(L"fileAttributes"))
		{
			std::wstring fpath = parameters.as_object().at(L"filePath").as_string();
			std::wstring filepath = NX::fs::dos_fullfilepath(fpath).global_dos_path();
			DWORD fileAttributes = parameters.as_object().at(L"fileAttributes").as_uint64();
			// run under current user
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);
			if (SetFileAttributesW(filepath.c_str(), fileAttributes)) {
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_SUCCESS, "Set file Attributes success");
			}
			else 
			{
				response = NX::string_formater("{\"code\":%u, \"message\":\"%s\"}", GetLastError(), "Error: set file Attributes failed");
			}
		}
		else
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, "Error: not find process");
			LOGERROR(NX::string_formater("error invalid data, response : %s", response.c_str()));
			return std::move(response);
		}
		return std::move(response);
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\": \"%s\" }", ERROR_INVALID_DATA, e.what());
		LOGERROR(NX::string_formater("exception : %s", response.c_str()));
	}

	return std::move(response);
}

void rmserv::init_log()
{
    const NX::fs::module_path image_path(NULL);
    const NX::fs::dos_filepath parent_dir(image_path.file_dir());
    std::wstring logfile_path = parent_dir.file_dir() + L"\\DebugDump.txt";
    GLOBAL_LOG_CREATE(logfile_path, (NX::dbg::LOGLEVEL)_serv_conf.get_log_level(), _serv_conf.get_log_size() * 1048576 /*Size * 1MB*/, 10);
}

std::string rmserv::on_request_rpm_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;
	std::vector<std::pair<unsigned long, std::wstring>> protected_processes;
	uint32_t status = 0;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	try {
		std::wstring filepath;
		if (parameters.as_object().has_field(L"filePath"))
		{
			auto dosfilepath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"filePath").as_string()).path();
			filepath = boost::iends_with(dosfilepath, L"\\") ? dosfilepath : dosfilepath + L'\\';
			std::transform(filepath.begin(), filepath.end(), filepath.begin(), tolower);

			std::set<std::tuple<std::wstring, SDRmRPMFolderOption, std::wstring>> dirsSet;
			for (int i = 0; i < m_markDir.size(); i++)
			{
				auto path = boost::iends_with(std::get<0>(m_markDir[i]), L"\\") ? std::get<0>(m_markDir[i]) : std::get<0>(m_markDir[i]) + L"\\";
				dirsSet.insert(std::make_tuple(path,
											   (SDRmRPMFolderOption)(std::stoi(std::get<7>(m_markDir[i]))),
											   std::get<4>(m_markDir[i])));
			}

			SDRmRPMFolderOption option;
			std::wstring fileTags;
            std::wstring hit_rpmfolder;

			for (const auto& dir : dirsSet)
			{
				if (std::get<0>(dir).size() == filepath.size())
				{
					if (boost::algorithm::equals(filepath, std::get<0>(dir)))
					{
						status |= NXRMFLT_SAFEDIRRELATION_SAFE_DIR;
						option = std::get<1>(dir);
                        std::vector<unsigned char> _fileTags_vec = NX::conversion::from_base64(std::get<2>(dir));
                        std::string _fileTags = std::string(_fileTags_vec.begin(), _fileTags_vec.end());
						fileTags = NX::conversion::utf8_to_utf16(_fileTags);
                        hit_rpmfolder = std::get<0>(dir);
					}
				}
				else if (std::get<0>(dir).size() < filepath.size())
				{
					if (boost::algorithm::starts_with(filepath, std::get<0>(dir)))
					{
						status |= NXRMFLT_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR;
						option = std::get<1>(dir);
                        std::vector<unsigned char> _fileTags_vec = NX::conversion::from_base64(std::get<2>(dir));
                        std::string _fileTags = std::string(_fileTags_vec.begin(), _fileTags_vec.end());
                        fileTags = NX::conversion::utf8_to_utf16(_fileTags);
                        hit_rpmfolder = std::get<0>(dir);
                    }
				}
				else
				{
					if (boost::algorithm::starts_with(std::get<0>(dir), filepath))
					{
						status |= NXRMFLT_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR;
					}
				}
			}

			std::wstring s;
			NX::json_value v = NX::json_value::create_object();
			v[L"code"] = NX::json_value(ERROR_SUCCESS);
			v[L"message"] = NX::json_value(L"succeed");
			v[L"dirstatus"] = NX::json_value(status);

			if (status & (NXRMFLT_SAFEDIRRELATION_SAFE_DIR | NXRMFLT_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR))
			{
                //
                // check whehter this RPM folder is current user's MyFolder
                //
                std::wstring wsid = sp->get_windows_user_sid();
                std::wstring tenantid = sp->get_rm_session().get_tenant_id();
                std::wstring userid = sp->get_rm_session().get_profile().get_email();
                userid = tenantid + L"\\" + userid;

                std::wstring hit_path = NX::fs::dos_fullfilepath(hit_rpmfolder).path();
                std::transform(hit_path.begin(), hit_path.end(), hit_path.begin(), tolower);


				// Handle the disk root directory(like E:\)
				auto pos = hit_path.rfind(L"\\");
				if (pos == hit_path.size() - 1) {
					std::wstring _hit_path = hit_path.substr(0, pos);
					if (NX::fs::is_dos_drive_only_path(_hit_path)) {
						hit_path = _hit_path;
					}
				}


                unsigned long val = AddDirectoryConfig(hit_path, SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYRPM, L"{}", wsid, userid);
                if (val == 0)
                {
                    v[L"option"] = NX::json_value(option);
                    if (option & RPMFOLDER_AUTOPROTECT)
                    {
                        v[L"filetags"] = NX::json_value(fileTags);
                    }
                }
                else
                {
                    option = (SDRmRPMFolderOption)((unsigned int)option & ~((unsigned int)RPMFOLDER_AUTOPROTECT | RPMFOLDER_MYFOLDER));
                    v[L"option"] = NX::json_value(option);
                }
			}

			s = v.serialize();
			response = NX::conversion::utf16_to_utf8(s).c_str();
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_get_rpm_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
    std::string response;
    bool ret = false;
    std::vector<std::pair<unsigned long, std::wstring>> protected_processes;
    uint32_t status = 0;

    std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
    if (sp == nullptr) {
        response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
        return std::move(response);
    }

    try {
        SDRmRPMFolderQuery option;
        if (parameters.as_object().has_field(L"option"))
        {
            option = (SDRmRPMFolderQuery) parameters.as_object().at(L"option").as_int32();

            std::wstring wsid = sp->get_windows_user_sid();
            std::wstring tenantid = sp->get_rm_session().get_tenant_id();
            std::wstring userid = sp->get_rm_session().get_profile().get_email();
            userid = tenantid + L"\\" + userid;

            std::vector<std::wstring> rpmfolders;
            for (int i = 0; i < m_markDir.size(); i++)
            {
                std::wstring filepath = std::get<0>(m_markDir[i]);
                if (option == SDRmRPMFolderQuery::RPMFOLDER_MYFOLDERS)
                {
                    unsigned long val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYFOLDER, L"{}", wsid, userid);
                    if (val == 0)
                    {
                        rpmfolders.push_back(filepath);
                    }
                }
                else if (option == SDRmRPMFolderQuery::RPMFOLDER_MYAPIFOLDERS)
                {
                    unsigned long val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYAPIRPM, L"{}", wsid, userid);
                    if (val == 0)
                    {
                        rpmfolders.push_back(filepath);
                    }
                }
                else if (option == SDRmRPMFolderQuery::RPMFOLDER_MYRPMFOLDERS)
                {
                    unsigned long val = AddDirectoryConfig(filepath, SDRmRPMFolderConfig::RPMFOLDER_CHECK_MYRPM, L"{}", wsid, userid);
                    if (val == 0)
                    {
                        rpmfolders.push_back(filepath);
                    }
                }
                else // SDRmRPMFolderQuery::RPMFOLDER_ALLFOLDERS
                {
                    rpmfolders.push_back(filepath);
                }
            }

            std::wstring s;
            NX::json_value v = NX::json_value::create_object();
            v[L"code"] = NX::json_value(ERROR_SUCCESS);
            v[L"message"] = NX::json_value(L"succeed");
            v[L"folders"] = NX::json_value::create_array();

            NX::json_array& folders = v[L"folders"].as_array();
            for (const auto& dir : rpmfolders)
            {
                NX::json_value dir_object = NX::json_value::create_object();
                dir_object.as_object()[L"path"] = NX::json_value(dir);
                folders.push_back(dir_object);
            }

            s = v.serialize();
            response = NX::conversion::utf16_to_utf8(s).c_str();
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
    }

    return std::move(response);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
std::string rmserv::on_request_is_sanctuary_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;
	std::vector<std::pair<unsigned long, std::wstring>> protected_processes;
	uint32_t status = 0;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	try {
		std::wstring filepath;
		if (parameters.as_object().has_field(L"filePath"))
		{
			filepath = parameters.as_object().at(L"filePath").as_string() + L'\\';
			std::transform(filepath.begin(), filepath.end(), filepath.begin(), tolower);

			std::set<std::pair<std::wstring, std::wstring>> dirsSet;
			for (size_t i = 0; i < m_sanctuaryDir.size(); i++)
			{
				dirsSet.insert(std::make_pair(m_sanctuaryDir[i].first + L'\\', m_sanctuaryDir[i].second));
			}

			std::wstring fileTags;

			for (const auto& dir : dirsSet)
			{
				if (dir.first.size() == filepath.size())
				{
					if (boost::algorithm::equals(filepath, dir.first))
					{
						status |= NXRMFLT_SANCTUARYDIRRELATION_SANCTUARY_DIR;
						fileTags = dir.second;
					}
				}
				else if (dir.first.size() < filepath.size())
				{
					if (boost::algorithm::starts_with(filepath, dir.first))
					{
						status |= NXRMFLT_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR;
						fileTags = dir.second;
					}
				}
				else
				{
					if (boost::algorithm::starts_with(dir.first, filepath))
					{
						status |= NXRMFLT_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR;
					}
				}
			}

			std::wstring s;
			NX::json_value v = NX::json_value::create_object();
			v[L"code"] = NX::json_value(ERROR_SUCCESS);
			v[L"message"] = NX::json_value(L"succeed");
			v[L"dirstatus"] = NX::json_value(status);
			v[L"filetags"] = NX::json_value(fileTags);
			s = v.serialize();
			response = NX::conversion::utf16_to_utf8(s).c_str();
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Invalid data\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

std::string rmserv::on_request_user_info(unsigned long session_id, unsigned long process_id)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	/*if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	} */
	try {
		std::wstring router; std::wstring tenant; std::wstring workingfolder; std::wstring tempfolder; std::wstring sdklibfolder;
		int ret = sp->get_rm_session().get_client_dir_info(router, tenant, workingfolder, tempfolder, sdklibfolder);
		std::string res = NX::string_formater("{\"code\":%d, \"message\":{\"router\":\"%s\", \"tenant\":\"%s\", \"workingfolder\":\"%s\", \"tempfolder\":\"%s\", \"sdklibfolder\":\"%s\"} }",
			ret, NX::conversion::utf16_to_utf8(router).c_str(), NX::conversion::utf16_to_utf8(tenant).c_str(), NX::conversion::utf16_to_utf8(workingfolder).c_str(), NX::conversion::utf16_to_utf8(tempfolder).c_str(), NX::conversion::utf16_to_utf8(sdklibfolder).c_str());

		std::wstring s;
		NX::json_value v = NX::json_value::create_object();
		v[L"code"] = NX::json_value(ret);
		v[L"router"] = NX::json_value(router);
		v[L"tenant"] = NX::json_value(tenant);
		v[L"workingfolder"] = NX::json_value(workingfolder);
		v[L"tempfolder"] = NX::json_value(tempfolder);
		v[L"sdklibfolder"] = NX::json_value(sdklibfolder);
		v[L"logined"] = NX::json_value(sp->get_rm_session().is_logged_on());
		s = v.serialize();
		response = NX::conversion::utf16_to_utf8(s).c_str();
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater("on_request_user_info: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_launch_pdp(unsigned long session_id, unsigned long process_id)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	
	try {
		SDWLResult res;
		res = m_PDP.StartPDPMan();
		if (!res)
		{
			LOGDEBUG(NX::string_formater(L"on_request_launch_pdp: cannot start cepdpman"));
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_ACCESS_DENIED, "cannot start cepdpman");
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater("on_request_launch_pdp: response: %s", response.c_str()));
	return std::move(response);
}

void force_delete_file(std::wstring _filepath, unsigned long session_id, unsigned long process_id)
{
	NX::win::session_token st(session_id);
	NX::win::impersonate_object impersonobj(st);
	std::wstring filepath = NX::fs::dos_fullfilepath(_filepath).global_dos_path();

	for (int i = 0; i < 5; i++)
	{
		if (!NX::fs::exists(filepath)) {
			LOGDEBUG(NX::string_formater(L"force_delete_file: normal file not exists, %s", filepath.c_str()));
			return;
		}

		DWORD wait_milliseconds = 5000;
		do {
			HANDLE h = ::CreateFileW(filepath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
			if (INVALID_HANDLE_VALUE != h) {
				CloseHandle(h);
				LOGDEBUG(NX::string_formater(L"force_delete_file: get file handle and close to delete, %s", filepath.c_str()));
				return;
			}
			// Failed
			if (ERROR_FILE_NOT_FOUND == GetLastError()) {
				LOGDEBUG(NX::string_formater(L"force_delete_file: get file handle failed as file not exists, %s", filepath.c_str()));
				return;
			}

			// It is access denied, this might because the file is opened by another program
			// Wait and try again
			Sleep(200);
			if (INFINITE != wait_milliseconds) {
				wait_milliseconds = (wait_milliseconds > 200) ? (wait_milliseconds - 200) : 0;
			}

		} while (0 != wait_milliseconds);
	}

	LOGDEBUG(NX::string_formater(L"force_delete_file: timeout to delete the file, %s", filepath.c_str()));
}

std::string rmserv::on_request_delete_file(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) 
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) 
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}

	try {
		std::wstring filepath;
		std::wstring fpath;
		unsigned long ret = 0;
		bool bret = false;
		if (parameters.as_object().has_field(L"filePath"))
		{
			// waiting for driver to implement
			//bret = sp->get_rm_session().delete_nxl_file(filepath);
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);

            fpath = parameters.as_object().at(L"filePath").as_string();
			filepath = NX::fs::dos_fullfilepath(fpath).global_dos_path();
			LOGDEBUG(NX::string_formater(L"on_request_delete_file: to_unlimitedpath, %s", filepath.c_str()));


			// For read-only file, we shall reset the read-only attribute first, or else will
			// Access denied when delete it(Fix bug 64930)
			DWORD attr = GetFileAttributesW(filepath.c_str());
			if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_READONLY))
			{
				attr &= ~FILE_ATTRIBUTE_READONLY;
				SetFileAttributes(filepath.c_str(), attr);
			}

			if (!::DeleteFileW(filepath.c_str()))
			{
				ret = GetLastError();
				LOGDEBUG(NX::string_formater("{\"code\":%d, \"message\":\"Fail to delete file\"}", ret)); 

				if (ret == ERROR_FILE_NOT_FOUND)
				{
					response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to delete file\"}", ret);
					LOGDEBUG(NX::string_formater(L"on_request_delete_file: DeleteFileW, %llu", ret));
				}
				else {
					// New a thread to delete file in case that normal file is locked
					response = NX::string_formater("{\"code\":%d, \"message\":\"Will delete the file later. Can't guarrantee it can be deleted.\"}", 0);
					std::thread t(force_delete_file, filepath, session_id, process_id);
					t.detach();
				}
			}
			else
			{
				response = NX::string_formater("{\"code\":%d, \"message\":\"succeed\"}", 0);
				if (NX::fs::exists(filepath)) {
					LOGDEBUG(NX::string_formater(L"on_request_delete_file: normal file still exists"));
					// New a thread to delete file in case that normal file is locked
					std::thread t(force_delete_file, filepath, session_id, process_id);
					t.detach();
				}
			}
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to delete file\"}", ERROR_FILE_NOT_FOUND);
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

void force_delete_folder(std::wstring _filepath, unsigned long session_id, unsigned long process_id)
{
	NX::win::session_token st(session_id);
	NX::win::impersonate_object impersonobj(st);

	std::wstring filepath = NX::fs::dos_fullfilepath(_filepath).global_dos_path();
	for (int i = 0; i < 5; i++)
	{
		if (!NX::fs::exists(filepath)) {
			LOGDEBUG(NX::string_formater(L"force_delete_folder: normal folder not exists, %s", filepath.c_str()));
			return;
		}

		DWORD wait_milliseconds = 5000;
		do {

			try {
				std::experimental::filesystem::remove_all(filepath.c_str());
				LOGDEBUG(NX::string_formater(L"force_delete_folder: std::experimental::filesystem::remove_all OK, %s", filepath.c_str()));
				return;
			}
			catch (...)
			{
				// error happen when delete folder, retry
			}

			// Failed
			if (!NX::fs::exists(filepath)) {
				LOGDEBUG(NX::string_formater(L"force_delete_folder: normal folder not exists, %s", filepath.c_str()));
				return;
			}
			// It is access denied, this might because the file is opened by another program
			// Wait and try again
			Sleep(200);
			if (INFINITE != wait_milliseconds) {
				wait_milliseconds = (wait_milliseconds > 200) ? (wait_milliseconds - 200) : 0;
			}

		} while (0 != wait_milliseconds);
	}

	LOGDEBUG(NX::string_formater(L"force_delete_file: timeout to delete the file, %s", filepath.c_str()));
}

std::string rmserv::on_request_delete_folder(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr)
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}

	try {
		std::wstring filepath;
		std::wstring fpath;
		unsigned long ret = 0;
		bool bret = false;
		if (parameters.as_object().has_field(L"filePath"))
		{
			fpath = parameters.as_object().at(L"filePath").as_string();
			filepath = NX::fs::dos_fullfilepath(fpath).global_dos_path();
			LOGDEBUG(NX::string_formater(L"on_request_delete_folder: to_unlimitedpath, %s", filepath.c_str()));

			DWORD attr = GetFileAttributesW(filepath.c_str());
			DWORD dwErr = ::GetLastError();
			if (INVALID_FILE_ATTRIBUTES != attr)
			{
				try {
					std::experimental::filesystem::remove_all(filepath.c_str());
				}
				catch (...) 
				{
					// error happen when delete folder, new a thread to retry
					// New a thread to delete file in case that normal file is locked
					response = NX::string_formater("{\"code\":%d, \"message\":\"Will delete the folder later. Can't guarrantee it can be deleted.\"}", 0);
					std::thread t(force_delete_folder, filepath, session_id, process_id);
					t.detach();
				}
			}
			else
			{
				response = NX::string_formater("{\"code\":%d, \"message\":\"folder not exists\"}", ERROR_FILE_NOT_FOUND);
			}
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to delete folder\"}", ERROR_FILE_NOT_FOUND);
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

void force_copy_file(std::wstring _srcpath, std::wstring _destpath, bool deletesource, unsigned long session_id, unsigned long process_id)
{
	NX::win::session_token st(session_id);
	NX::win::impersonate_object impersonobj(st);

	NX::fs::dos_fullfilepath srcpath(_srcpath);
	NX::fs::dos_fullfilepath destpath(_destpath);
	std::wstring nonnxldestpath = destpath.global_dos_path();
	if (boost::algorithm::iends_with(destpath.path(), L".nxl"))
	{
		nonnxldestpath = destpath.global_dos_path().substr(0, (destpath.path().size() - 4));
	}

	bool bcleaned = false;

	if (!NX::fs::exists(nonnxldestpath)) {
		bcleaned = true;
		LOGDEBUG(NX::string_formater(L"force_copy_file: normal file not exists, %s", nonnxldestpath.c_str()));
	}
	else
	{
		DWORD wait_milliseconds = 5000;
		do {

			HANDLE h = ::CreateFileW(nonnxldestpath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
			if (INVALID_HANDLE_VALUE != h) {
				CloseHandle(h);
				bcleaned = true;
				LOGDEBUG(NX::string_formater(L"force_copy_file: get file handle and close to delete, %s", nonnxldestpath.c_str()));
				break;
			}
			// Failed
			if (ERROR_FILE_NOT_FOUND == GetLastError()) {
				bcleaned = true;
				LOGDEBUG(NX::string_formater(L"force_copy_file: get file handle failed as file not exists, %s", nonnxldestpath.c_str()));
				break;
			}

			// It is access denied, this might because the file is opened by another program
			// Wait and try again
			Sleep(200);
			if (INFINITE != wait_milliseconds) {
				wait_milliseconds = (wait_milliseconds > 200) ? (wait_milliseconds - 200) : 0;
			}

		} while (0 != wait_milliseconds);
	}

	// 
	// Comment out the condition judge, nemas always execute CopyFile even thouth the deleting normal file is timeout.
	// For example, the case that edit & save pdf file in RPM folder with acrobat, the deleting normal file will be timeout,
	// but still perform CopyFile operation.
	//
	// if (bcleaned == true)
	{
		// copy file from src to dest
		if (::CopyFileW(srcpath.global_dos_path().c_str(), destpath.global_dos_path().c_str(), false) == FALSE)
		{
			unsigned long ret = GetLastError();
			LOGDEBUG(NX::string_formater(L"force_copy_file: failed to copy the file, %d", ret));
			return;
		}

		if (deletesource)
		{
			::DeleteFileW(srcpath.global_dos_path().c_str());
		}

		WIN32_FIND_DATA pNextInfo;
		HANDLE h = FindFirstFile(destpath.global_dos_path().c_str(), &pNextInfo);
		if (h != INVALID_HANDLE_VALUE)
		{
			FindClose(h);
		}
	}
	//else
	//{
	//	LOGDEBUG(NX::string_formater(L"force_copy_file: timeout to delete the file, %s", nonnxldestpath.c_str()));
	//	return;
	//}

	LOGDEBUG(NX::string_formater(L"force_copy_file: copy the file, %s", _srcpath.c_str()));
	return;
}

std::string rmserv::on_request_copy_file(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr)
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}

	try {
		std::wstring srcpath;
		std::wstring filepath;
		bool deletesource = false;
		unsigned long ret = 0;
		bool bret = false;

		if (parameters.as_object().has_field(L"filePath") && parameters.as_object().has_field(L"srcPath"))
		{
			srcpath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"srcPath").as_string()).global_dos_path();
			filepath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"filePath").as_string()).global_dos_path();
			if (parameters.as_object().has_field(L"deleteSource"))
			{
				deletesource = parameters.as_object().at(L"deleteSource").as_boolean();
			}

			// run under current user
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);

			std::wstring nonnxlfilepath = filepath;
			if (boost::algorithm::iends_with(filepath, L".nxl"))
			{
				nonnxlfilepath = filepath.substr(0, (filepath.size() - 4));
			}

			if (NX::fs::exists(nonnxlfilepath))
			{
				::DeleteFileW(nonnxlfilepath.c_str());
			}

            // 
            // check source file size, if zero, retry in 10 seconds
            //
            //for sharing violation, give 30s max tolerating
            int max_retry = 50;
            int each_interval_retry_if_sharing_violation = 200;
            // max toleration time is 30 secounds; 
            BOOL bfullfile = false;
            while (max_retry--) {
                HANDLE h = INVALID_HANDLE_VALUE;
                do {
                    h = ::CreateFileW(srcpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (INVALID_HANDLE_VALUE == h) {
                        break;
                    }

                    const ULONG fileSize = GetFileSize(h, NULL);
                    if (fileSize == INVALID_FILE_SIZE) {
                        break;
                    }

                    if (fileSize == 0) {
                        break;
                    }

                    bfullfile = true;
                } while (FALSE);
                ::CloseHandle(h);

                if (bfullfile)
                    break;

                ::Sleep(each_interval_retry_if_sharing_violation);
            }

			response = NX::string_formater("{\"code\":%d, \"message\":\"succeed\"}", 0);
			if (NX::fs::exists(nonnxlfilepath)) {
				LOGDEBUG(NX::string_formater(L"on_request_copy_file: normal file still exists"));
				// New a thread to force copy file in case that normal file is locked
				std::thread t(force_copy_file, srcpath, filepath, deletesource, session_id, process_id);
				t.detach();
			}
			else
			{
				// copy file from src to dest
				if (::CopyFileW(srcpath.c_str(), filepath.c_str(), false) == FALSE)
				{
					ret = GetLastError();
					response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to copy file\"}", ret);
				}
				else
				{
					if (deletesource)
					{
						::DeleteFileW(srcpath.c_str());
					}

					WIN32_FIND_DATA pNextInfo;
					HANDLE h = FindFirstFile(filepath.c_str(), &pNextInfo);
					if (h != INVALID_HANDLE_VALUE)
					{
						FindClose(h);
					}
				}
			}
		}
		else
		{
			response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to copy file\"}", ERROR_FILE_NOT_FOUND);
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_get_rights(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr)
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on())
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}

	try {
		std::wstring filepath;
		int option = 0;
		unsigned long ret = 0;
		bool bret = false;
		if (parameters.as_object().has_field(L"filePath"))
		{
			filepath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"filePath").as_string()).path();
			if (parameters.as_object().has_field(L"option"))
				option = parameters.as_object().at(L"option").as_int32();

			unsigned __int64 evaluate_id = 0;;
			unsigned __int64 rights = 0;
			unsigned __int64 custom_rights = 0;
			bool bcheckowner = true;
			if (option != 0)
				bcheckowner = false;

			//Fix Bug 58592,  cannot get file watermark info
			//bret = sp->get_rm_session().rights_evaluate(filepath, process_id, session_id, &evaluate_id, &rights, &custom_rights, bcheckowner);

			std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> vecRightsWatermarks;
			bret = sp->get_rm_session().rights_evaluate_with_watermark(filepath, process_id, session_id, &evaluate_id, &rights, &custom_rights, vecRightsWatermarks, bcheckowner);
			
			if (bret && sp->get_rm_session().is_ad_hoc())
			{
				// rights = rights & ~BUILTIN_RIGHT_DECRYPT;
				LOGDEBUG(NX::string_formater(L"on_request_get_rights: adhoc: remove decrypt, %llu", rights));
			}
			std::wstring s;
			NX::json_value v = NX::json_value::create_object();
			if (!bret)
			{
				v[L"code"] = NX::json_value(ERROR_INVALID_DATA);
				v[L"message"] = NX::json_value(L"Error Invalid File");
			}
			else
			{
				v[L"code"] = NX::json_value(ERROR_SUCCESS);
				v[L"message"] = NX::json_value(L"succeed");
			}
			
			v[L"rights"] = NX::json_value(rights);

			v[L"rightswatermark"] = NX::json_value::create_array();
			NX::json_array& arrRightsWatermark = v[L"rightswatermark"].as_array();

			if (vecRightsWatermarks.size() > 0)
			{
				for (auto item : vecRightsWatermarks)
				{
					NX::json_value objRightWatermark = NX::json_value::create_object();
					objRightWatermark[L"right"] = NX::json_value(item.first);

					objRightWatermark[L"watermark"] = NX::json_value::create_array();
					NX::json_array& arrWatermark = objRightWatermark[L"watermark"].as_array();
					for (auto watermark : item.second)
					{
						NX::json_value objWatermark = NX::json_value::create_object();
						objWatermark[L"text"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.text));
						objWatermark[L"fontname"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.fontName));
						objWatermark[L"fontcolor"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.fontColor));
						objWatermark[L"fontsize"] = NX::json_value(watermark.fontSize);
						objWatermark[L"transparency"] = NX::json_value(watermark.transparency);
						objWatermark[L"rotation"] = NX::json_value(watermark.rotation);
						objWatermark[L"repeat"] = NX::json_value(watermark.repeat);

						arrWatermark.push_back(objWatermark);
					}
					arrRightsWatermark.push_back(objRightWatermark);
				}
			}

			s = v.serialize();
			response = NX::conversion::utf16_to_utf8(s).c_str();
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to get rights\"}", ERROR_FILE_NOT_FOUND);
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to query log\"}", ERROR_INVALID_DATA);
	}

	return std::move(response);
}

std::string rmserv::on_request_get_file_status(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;
	bool ret = false;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr)
	{
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	
	try {
		unsigned long ret = 0;
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind=NULL;

		if (parameters.as_object().has_field(L"filePath"))
		{
			NX::fs::dos_fullfilepath filepath(parameters.as_object().at(L"filePath").as_string());

			size_t pos = filepath.path().rfind(L'\\');
			std::wstring filename = (pos == std::wstring::npos) ? filepath.path() : filepath.path().substr(pos + 1);
			std::wstring inputpath = filepath.path().substr(0, pos) + L"\\";
			LOGDEBUG(NX::string_formater(L"on_request_get_file_status: file: %s, dir: %s", filepath.path().c_str(), inputpath.c_str()));

			{
				// run under current user
				NX::win::session_token st(session_id);
				NX::win::impersonate_object impersonobj(st);
				// make sure file exist
				hFind = FindFirstFile(filepath.global_dos_path().c_str(), &FindFileData);
				if (hFind != INVALID_HANDLE_VALUE)
					FindClose(hFind);
			}
			uint32_t status = 0;
			
			std::vector<std::wstring> vec;
			for (int i = 0; i < m_markDir.size(); i++)
			{
				auto path = boost::iends_with(std::get<0>(m_markDir[i]), L"\\") ? std::get<0>(m_markDir[i]) : std::get<0>(m_markDir[i]) + L"\\";
				vec.push_back(path);
			}

			for (std::wstring& dir : vec)
			{
				if (dir.size() == inputpath.size())
				{
					if (boost::algorithm::iequals(inputpath, dir))
					{
						status |= NXRMFLT_SAFEDIRRELATION_SAFE_DIR;
					}
				}
				else if (dir.size() < inputpath.size())
				{
					if (boost::algorithm::istarts_with(inputpath, dir))
					{
						status |= NXRMFLT_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR;
					}
				}
				else
				{
					if (boost::algorithm::istarts_with(dir, inputpath))
					{
						status |= NXRMFLT_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR;
					}
				}
			}
			
			unsigned int ret = sp->get_rm_session().is_nxl_file(filepath.global_dos_path());

			unsigned int nxlstate = 0; // not nxl file
			std::wstring s;
			NX::json_value v = NX::json_value::create_object();
			std::wstring msg = L"succeed";

			if (ret)
			{
				if (ret == ERROR_NOT_FOUND)
					msg = L"File not found";
				else if (ret == ERROR_INVALID_DATA)
				{
					msg = L"File is not nxl";
					ret = 0;
				}
			}
			else 
			{
				nxlstate = 1;
			}
			
			v[L"code"] = NX::json_value(ret);
			v[L"message"] = NX::json_value(msg);
			v[L"dirstatus"] = NX::json_value(status);
			v[L"nxlstate"] = NX::json_value(nxlstate);
			LOGDEBUG(NX::string_formater(L"   file_status: ret: %d, dir: 0x%08lX, file: %d", ret, status, nxlstate));

			if (parameters.as_object().has_field(L"FullQuery") && nxlstate == 1)
			{
				// Query full information of the file
				// User Rights
				unsigned __int64 evaluate_id = 0;;
				unsigned __int64 rights = 0;
				unsigned __int64 custom_rights = 0;
				bool bcheckowner = true;

				if (parameters.as_object().has_field(L"CheckOwner")) {
					bcheckowner = parameters.as_object().at(L"CheckOwner").as_boolean();
				}

				std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> vecRightsWatermarks;
				sp->get_rm_session().rights_evaluate_with_watermark(filepath.global_dos_path(), process_id, session_id, &evaluate_id, &rights, &custom_rights, vecRightsWatermarks, bcheckowner);

				v[L"rightswatermark"] = NX::json_value::create_array();
				NX::json_array& arrRightsWatermark = v[L"rightswatermark"].as_array();

				if (vecRightsWatermarks.size() > 0)
				{
					for (auto item : vecRightsWatermarks)
					{
						NX::json_value objRightWatermark = NX::json_value::create_object();
						objRightWatermark[L"right"] = NX::json_value(item.first);

						objRightWatermark[L"watermark"] = NX::json_value::create_array();
						NX::json_array& arrWatermark = objRightWatermark[L"watermark"].as_array();
						for (auto watermark : item.second)
						{
							NX::json_value objWatermark = NX::json_value::create_object();
							objWatermark[L"text"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.text));
							objWatermark[L"fontname"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.fontName));
							objWatermark[L"fontcolor"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.fontColor));
							objWatermark[L"fontsize"] = NX::json_value(watermark.fontSize);
							objWatermark[L"transparency"] = NX::json_value(watermark.transparency);
							objWatermark[L"rotation"] = NX::json_value(watermark.rotation);
							objWatermark[L"repeat"] = NX::json_value(watermark.repeat);

							arrWatermark.push_back(objWatermark);
						}
						arrRightsWatermark.push_back(objRightWatermark);
					}
				}

				std::string tags;
				std::string info;
				std::string adpolicy;
				std::string doc_duid;
				std::string doc_owner_id;
				std::string token_group;
				std::shared_ptr<policy_bundle> policy;
				DWORD attributes;
				if (sp->get_rm_session().get_document_eval_info(process_id, filepath.global_dos_path(), policy, adpolicy, tags, info, doc_duid, doc_owner_id, token_group, attributes))
				{
					v[L"infoext"] = NX::json_value(NX::conversion::utf8_to_utf16(info));
					v[L"policy"] = NX::json_value(NX::conversion::utf8_to_utf16(adpolicy));
					// WaterMark in Header
					if (adpolicy.size() >= tags.size())
					{
						// ad-hoc policy file
						if (vecRightsWatermarks.size() > 0)
						{
							for (auto item : vecRightsWatermarks)
							{
								for (auto watermark : item.second)
								{
									NX::json_value objWatermark = NX::json_value::create_object();
									objWatermark[L"text"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.text));
									objWatermark[L"fontname"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.fontName));
									objWatermark[L"fontcolor"] = NX::json_value(NX::conversion::utf8_to_utf16(watermark.fontColor));
									objWatermark[L"fontsize"] = NX::json_value(watermark.fontSize);
									objWatermark[L"transparency"] = NX::json_value(watermark.transparency);
									objWatermark[L"rotation"] = NX::json_value(watermark.rotation);
									objWatermark[L"repeat"] = NX::json_value(watermark.repeat);
									v[L"watermark"] = objWatermark;
									break;
								}
								break;
							}
						}
					}

					// Expiry
					if (policy && (policy->get_enddate() > 0))
					{
						NX::json_value expiry = NX::json_value::create_object();
						expiry[L"endDate"] = NX::json_value(policy->get_enddate());
						if (policy->get_startdate() > 0)
						{
							expiry[L"startDate"] = NX::json_value(policy->get_startdate());
						}
						v[L"expiry"] = expiry;
					}

					// Tags
					v[L"tags"] = NX::json_value(NX::conversion::utf8_to_utf16(tags));

					// Rights in Header
					if (tags.size() > 2)
						v[L"rights"] = NX::json_value(0);
					else
						v[L"rights"] = NX::json_value(rights);

					// DUID
					v[L"duid"] = NX::json_value(NX::conversion::utf8_to_utf16(doc_duid));

					// Creator
					v[L"creator"] = NX::json_value(NX::conversion::utf8_to_utf16(doc_owner_id));

					// Token Group
					v[L"tokengroup"] = NX::json_value(NX::conversion::utf8_to_utf16(token_group));

					// File Attributes
					v[L"attributes"] = NX::json_value((long long)attributes);
				}
			}

			s = v.serialize();
			response = NX::conversion::utf16_to_utf8(s).c_str();
		}
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"Fail to find file name\"}", ERROR_FILE_NOT_FOUND);
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		response = NX::string_formater("{\"code\":%d, \"message\": %s}", ERROR_INVALID_DATA, e.what());
	}

	return std::move(response);
}

std::string rmserv::on_request_login(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User already sign in");
		return std::move(response);
	}
	if (is_server_mode() == true)
	{
		// server mode, won't allow user login
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "Machine is under server mode");
		return std::move(response);
	}

	try {
		std::wstring command;
		std::wstring param;
		const NX::json_object& params = parameters.as_object();
		if (parameters.as_object().has_field(L"command"))
		{
			command = parameters.as_object().at(L"command").as_string();
			if (parameters.as_object().has_field(L"param"))
				param = parameters.as_object().at(L"param").as_string();
		}
		bool bsent = sp->get_rm_session().get_app_manager().send_logon_notification();
		if (bsent)
		{
			// cache the callback CMD
			std::pair<std::wstring, std::wstring> cmd(command, param);
			sp->get_rm_session().add_callback_cmds(cmd);
		}

		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_add_metadata: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_logout(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session  not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");

		const NX::json_object& params = parameters.as_object();
		int option = 0;
		if (parameters.as_object().has_field(L"option"))
		{
			option = parameters.as_object().at(L"option").as_number().to_uint32();
		}

		if (option == 0)
		{
			// tell Service Mananger - Tray, that I want to logout
			sp->get_rm_session().get_app_manager().send_quit_notification();
		}
		else if (option == 1)
		{
			// ask Service, that can I logout? Is there any protected file still open?
			try {

				std::vector<std::pair<unsigned long, std::wstring>> protected_processes;

				if (GLOBAL.get_process_cache().does_protected_process_exist(session_id)) {
					protected_processes = GLOBAL.get_process_cache().find_all_protected_process(session_id);
				}

				if (protected_processes.empty() || IsPDPProcess(protected_processes)) {
					// No protected process running
				}
				else {
					std::set<unsigned long> pidSet;
					GetNonPDPProcessId(protected_processes, pidSet);
					int j = 0;
					std::wstring dbginfo;
					std::string notifyMessage;

					response = "{\"code\":170, \"message\":\"Fail to log out because following protected process(es) are still running: "; 
					// The notification message json string that sent service manager.
					notifyMessage = "{\"message\":\"Can't log out because the protected process(es) are still running \", \"application\": \""; 
					dbginfo = L"Logout was denied because following protected process(es) are still running:\r\n";
					std::string applications;
					for (int i = 0; i < (int)protected_processes.size(); i++) {
						const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
						if (pidSet.find(item.first) == pidSet.end()) continue;
						const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
						pname = (NULL == pname) ? item.second.c_str() : (pname + 1);

						if (0 != j) {
							applications.append(" ,");
							response.append(" ,");
						}
						j++;
						applications.append(NX::conversion::utf16_to_utf8(pname));

						response.append(NX::conversion::utf16_to_utf8(pname));
						response.append("(");
						response.append(NX::conversion::to_string((int)item.first));
						response.append(")");

						if (NX::dbg::LL_INFO <= NxGlobalLog.get_accepted_level()) {
							dbginfo.append(L"    ");
							dbginfo.append(pname);
							dbginfo.append(L" (");
							dbginfo.append(NX::conversion::to_wstring((int)item.first));
							dbginfo.append(L")\r\n");
						}
					}
					notifyMessage.append(applications);
					notifyMessage.append("\"}");
					response.append("\"}");
					LOGINFO(dbginfo);

					// tell Service Mananger - to popup bubble to prompt user.
					if (j > 0) {
						sp->get_rm_session().get_app_manager().send_popup_notification(NX::conversion::utf8_to_utf16(notifyMessage), true);
					}
				}
			}
			catch (const std::exception& e) {
				response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
			}
		}
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_logout: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_notify_message(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}

	// Comments out this for fixing some bugs(65284,63424) that no any notification if user haven't login,
	// now still send notification to nxrmtray, and let it to popup system bubble if not signed.
	//
	/*
	if (!sp->get_rm_session().is_logged_on()) {

		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}*/

	try {
		//const NX::json_object& notify_message = parameters.as_object();
		bool bsent = sp->get_rm_session().get_app_manager().send_popup_notification(parameters.serialize(), true);

		if (bsent)
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", 0, "succeed");
		else
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", GetLastError(), "Failed to notify user");
	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_notify_message: response: %s", response.c_str()));
	return std::move(response);
}

std::string rmserv::on_request_read_file_tags(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters)
{
	std::string response;

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_NO_SUCH_LOGON_SESSION, "Session not found");
		return std::move(response);
	}
	if (!sp->get_rm_session().is_logged_on()) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_LOGON_NOT_GRANTED, "User not sign in");
		return std::move(response);
	}
	try {
		std::wstring tags;
		bool result = false;
		if (parameters.as_object().has_field(L"path"))
		{
			std::wstring filepath = NX::fs::dos_fullfilepath(parameters.as_object().at(L"path").as_string()).global_dos_path();
			result = sp->get_rm_session().read_file_tags(filepath, process_id, session_id, tags);
		}

		if (result == false) {
			int le = GetLastError();
			response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", (0 == le) ? -1 : le, "failed to read file tags");
			LOGDEBUG(NX::string_formater(" process_request on_request_read_file_tags: failed, response (%s)", response.c_str()));
		}
		else {
			NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
				std::pair<std::wstring, NX::json_value>(L"code", NX::json_value(0)),
				std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(L"succeed")),
				std::pair<std::wstring, NX::json_value>(L"tags",NX::json_value(tags))
				}), false);

			response = NX::conversion::utf16_to_utf8(v.serialize());
			LOGDEBUG(NX::string_formater(" process_request on_request_read_file_tags: response (%s)", response.c_str()));
		}

	}
	catch (const std::exception& e) {
		response = NX::string_formater("{\"code\":%d, \"message\":\"%s\"}", ERROR_INVALID_DATA, e.what());
	}

	LOGDEBUG(NX::string_formater(" on_request_read_file_tags: response: %s", response.c_str()));
	return std::move(response);
}

void rmserv::log_init_info()
{
    NX::win::system_default_language sys_lang;
    NX::win::hardware::memory_information mem_info;
    const std::vector<NX::fs::drive>& drives = NX::fs::get_logic_drives();
    const std::vector<NX::win::hardware::network_adapter_information>& adapters = NX::win::hardware::get_all_network_adapters();

    LOGINFO(L" ");
    LOGINFO(NX::string_formater(L"Product Information"));
    LOGINFO(NX::string_formater(L"    - Name:      %s", GLOBAL.get_product_name().c_str()));
    LOGINFO(NX::string_formater(L"    - Company:   %s", GLOBAL.get_company_name().c_str()));
    LOGINFO(NX::string_formater(L"    - Version:   %s", GLOBAL.get_product_version_string().c_str()));
    LOGINFO(NX::string_formater(L"    - Directory: %s", GLOBAL.get_product_dir().c_str()));
    LOGINFO(NX::string_formater(L"    - Router:    %s", _router_conf.get_router().c_str()));
    LOGINFO(NX::string_formater(L"    - Tenant:    %s", _router_conf.get_tenant_id().c_str()));
    LOGINFO(NX::string_formater(L"OS Information"));
    LOGINFO(NX::string_formater(L"    - Name:      %s %s (Build %d)", GLOBAL.get_windows_info().windows_name().c_str(), GLOBAL.get_windows_info().is_processor_x86() ? L"32 Bits" : L"64 Bits", GLOBAL.get_windows_info().build_number()));
    LOGINFO(NX::string_formater(L"    - Language:  %s", sys_lang.name().c_str()));
    LOGINFO(NX::string_formater(L"Hardware Information"));
    LOGINFO(NX::string_formater(L"    - CPU:       %s", GLOBAL.get_cpu_brand().c_str()));
    LOGINFO(NX::string_formater(L"    - Memory:    %.1f GB", (mem_info.get_physical_total() / 1024.0)));
    LOGINFO(NX::string_formater(L"    - Drives"));
    for (int i = 0; i < (int)drives.size(); i++) {
        LOGINFO(NX::string_formater(L"      #%d: %c:", i, drives[i].drive_letter()));
        LOGINFO(NX::string_formater(L"         > Type: %s", drives[i].type_name().c_str()));
        LOGINFO(NX::string_formater(L"         > Dos Name: %s", drives[i].dos_name().c_str()));
        LOGINFO(NX::string_formater(L"         > NT Name: %s", drives[i].nt_name().c_str()));
        if (drives[i].is_fixed() || drives[i].is_removable() || drives[i].is_ramdisk()) {
            const NX::fs::drive::space& drive_space = drives[i].get_space();
            if (0 != drive_space.total_bytes()) {
                LOGINFO(NX::string_formater(L"         > Total space: %d MB", (int)(drive_space.total_bytes() / 1048576)));
                LOGINFO(NX::string_formater(L"         > Free space: %d MB (%.1f%%)", (int)(drive_space.available_free_bytes() / 1048576), 100 * ((float)drive_space.available_free_bytes() / (float)drive_space.total_bytes())));
            }
            else {
                if (drives[i].is_removable()) {
                    LOGINFO(L"         > No media");
                }
            }
        }
    }

    LOGINFO(NX::string_formater(L"    - Network Adapters"));
    for (int i = 0; i < (int)adapters.size(); i++) {
        const std::wstring& if_type_name = adapters[i].get_if_type_name();
        const std::wstring& oper_status_name = adapters[i].get_oper_status_name();
        LOGINFO(NX::string_formater(L"      #%d: %s", i, adapters[i].get_adapter_name().c_str()));
        LOGINFO(NX::string_formater(L"         > Friendly name: %s", adapters[i].get_friendly_name().c_str()));
        LOGINFO(NX::string_formater(L"         > Description: %s", adapters[i].get_description().c_str()));
        LOGINFO(NX::string_formater(L"         > IfType: %s", if_type_name.c_str()));
        LOGINFO(NX::string_formater(L"         > OperStatus: %s", oper_status_name.c_str()));
        LOGINFO(NX::string_formater(L"         > MAC: %s", adapters[i].get_mac_address().c_str()));
        if (adapters[i].is_connected()) {
            LOGINFO(NX::string_formater(L"         > IPv4: %s", adapters[i].get_ipv4_addresses().empty() ? L"" : adapters[i].get_ipv4_addresses()[0].c_str()));
            LOGINFO(NX::string_formater(L"         > IPv6: %s", adapters[i].get_ipv6_addresses().empty() ? L"" : adapters[i].get_ipv6_addresses()[0].c_str()));
        }
    }
}

bool rmserv::init_keys()
{
    DWORD err = 0;
    bool created = false;

    const std::wstring cert_file(GLOBAL.get_config_dir() + L"\\client.cer");

    if (!_master_key.open()) {

        err = GetLastError();
        if (NTE_BAD_KEYSET != err && NTE_NOT_FOUND != err) {
            LOGERROR(NX::string_formater(L"Fail to open master key %08X", err));
            return false;
        }

        LOGINFO(L"Master doesn't exist, try to create a new one");
        // NOTE: This should be first time run
        // Sanity Check here:

        if (!_master_key.create()) {
            LOGERROR(NX::string_formater(L"Fail to create master key %08X", GetLastError()));
            return false;
        }
        LOGINFO(L"  - Master has been created");
        created = true;
    }

    // Key is ready
    if (created) {
        ::DeleteFileW(cert_file.c_str());
    }

    std::vector<unsigned char> cert_thumbprint;

    if (!NX::fs::exists(cert_file)) {
        if (_master_key.generate_cert(cert_file, cert_thumbprint)) {
            LOGINFO(NX::string_formater(L"Client certificate has been generated: %s", cert_file.c_str()));
        }
        else {
            LOGERROR(NX::string_formater(L"Fail to generate client certificate %08X", GetLastError()));
        }
    }
    else {
        NX::cert::cert_context context;
        if (!context.create(cert_file)) {
            LOGERROR(NX::string_formater(L"Fail to load client certificate %08X", GetLastError()));
        }
        else {
            const NX::cert::cert_info& info = context.get_cert_info();
            cert_thumbprint = info.get_thumbprint();
        }
    }

    if (cert_thumbprint.empty()) {
        LOGWARNING(L"Fail to load client id from client certificate");
    }
    else {
        std::vector<unsigned char> md5_result;
        md5_result.resize(16, 0);
        if (!NX::crypto::md5(cert_thumbprint.data(), (ULONG)cert_thumbprint.size(), md5_result.data())) {
            LOGERROR(NX::string_formater(L"Fail hash certificate thumbprint %08X", GetLastError()));
        }
        else {
            _client_id = NX::conversion::to_wstring(md5_result);
            // Change first 3 characters to  'R', 'M', 'D'
            _client_id[0] = L'R';
            _client_id[1] = L'M';
            _client_id[2] = L'D'; 
			//_client_id = L"914749B0B30A79D7B334C76314BCEB85";
            //LOGINFO(NX::string_formater(L"Load client id from client certificate: %s", _client_id.c_str()));
			LOGINFO(NX::string_formater(L"init_keys: client id : %s", _client_id.c_str()));
        }
    }

    return true;
}

bool rmserv::init_vhd()
{
    bool result = false;

    if (!NX::win::service_control::exists(L"nxrmvhd")) {
        LOGERROR(L"Driver nxrmvhd doesn't exist");
        return false;
    }

    try {

        NX::win::service_control sc;
        sc.open(L"nxrmvhd");
        const NX::win::service_status& status = sc.query_status(true);
        if (!status.is_running()) {
            sc.start(true);
        }

        // start nxrmvhd
        if (!_vhd_manager.initialize()) {
            throw std::exception("fail to initialize nxrmvhd");
        }

        result = true;
    }
    catch (const std::exception& e) {
        LOGERROR(NX::string_formater("Fail to start driver nxrmvhd -> %s", e.what()));
        result = false;
    }

    return result;
}

bool rmserv::init_drvcore()
{
    bool result = false;

    if (!NX::win::service_control::exists(L"nxrmdrv")) {
        LOGERROR(L"Driver nxrmdrv doesn't exist");
        return false;
    }

    try {
		LOGDEBUG(L"init_drvcore: sc.open");
        NX::win::service_control sc;
        sc.open(L"nxrmdrv");
        const NX::win::service_status& status = sc.query_status(true);
        if (!status.is_running()) {
            sc.start(true);
        }

        _serv_core.start();
		LOGDEBUG(L"init_drvcore: _serv_core.start");

        result = true;
    }
    catch (const std::exception& e) {
        LOGERROR(NX::string_formater("Fail to start driver nxrmdrv - %s", e.what()));
        result = false;
    }

    return result;
}

bool rmserv::init_drvflt()
{
    bool result = false;

    if (!NX::win::service_control::exists(L"nxrmflt")) {
        LOGERROR(L"Driver nxrmflt doesn't exist");
        return false;
    }

    try {

        NX::win::service_control sc;
        sc.open(L"nxrmflt");
        const NX::win::service_status& status = sc.query_status(true);
        if (!status.is_running()) {
            sc.start(true);
        }

        // start nxrmflt
        _serv_flt.start(_vhd_manager.get_config_volume().get_volume_name());

        result = true;
    }
    catch (const std::exception& e) {
        LOGERROR(NX::string_formater("Fail to start driver nxrmflt - > %s", e.what()));
        result = false;
    }

    return result;
}

void rmserv::init_core_context_cache()
{
    const std::wstring context_cache_file(_vhd_manager.get_config_volume().get_config_dir() + L"\\core_context.json");
    std::string s;
    if (!GLOBAL.nt_load_file(context_cache_file, s)) {
        return;
    }
    if (!_corecontext_cache.load(s)) {
        LOGERROR(NX::string_formater(L"Fail to load core context from file (%s)", context_cache_file.c_str()));
        LOGERROR(s.c_str());
    }
}

void rmserv::save_core_context_cache()
{
    const std::wstring context_cache_file(_vhd_manager.get_config_volume().get_config_dir() + L"\\core_context.json");
    std::string s;
    if (!_corecontext_cache.serialize(s)) {
        return;
    }
    if (!GLOBAL.nt_generate_file(context_cache_file, s, true)) {
        LOGERROR(NX::string_formater(L"Fail to write core context (%d)", GetLastError()));
    }
}

void rmserv::initialize_sspi() noexcept
{
    PSecurityFunctionTableW pSft = InitSecurityInterfaceW();
    if (NULL != pSft) {
        ULONG           cPackages = 0;
        PSecPkgInfoW    pPackageInfo = NULL;
        SECURITY_STATUS status = EnumerateSecurityPackagesW(&cPackages, &pPackageInfo);
        if (SEC_E_OK == status && NULL != pPackageInfo) {
            (VOID)FreeContextBuffer(pPackageInfo);
        }
    }
}


void rmserv::set_ready_flag(bool b)
{
    ::EnterCriticalSection(&_lock);
    _ready = b;
    ::LeaveCriticalSection(&_lock);
}

bool rmserv::is_service_ready()
{
    bool b = false;
    ::EnterCriticalSection(&_lock);
    b = _ready;
    ::LeaveCriticalSection(&_lock);
    return b;
}

void rmserv::emulate_browser()
{
    try {

        unsigned long value = 0;
        // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Internet Explorer\Main\FeatureControl\FEATURE_BROWSER_EMULATION
        NX::win::reg_local_machine reg_feature_browser;

        reg_feature_browser.open(L"SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", NX::win::reg_key::reg_wow64_64, false);
        try {
            reg_feature_browser.read_value(L"nxrmtray.exe", &value);
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }

        if (value != 11001) {
            value = 11001;
            reg_feature_browser.set_value(L"nxrmtray.exe", value);
            LOGINFO(L"Set IE's FEATURE_BROWSER_EMULATION code (IE11) for nxrmtray.exe");
        }
    }
    catch (const std::exception& e) {
        LOGERROR(NX::string_formater("Fail to set IE's FEATURE_BROWSER_EMULATION code (IE11) for nxrmtray.exe (%S)", e.what()));
    }
}

bool rmserv::IsInitFinished(bool& finished)
{
	SDWLResult res = m_PDP.IsReadyForEval(finished);
	LOGDEBUG(NX::string_formater(L"IsInitFinished: %d", finished));

	if (res.GetCode() == 0)
		return true;
	else
		return false;
}

bool rmserv::ensurePDPConnectionReady(bool& finished)
{
	LOGDEBUG(NX::string_formater(L"ensurePDPConnectionReady: start, pdpdir: %s", m_pdpDir.c_str()));
	//std::lock_guard<std::mutex> lck(m_mtxPDP);
	::EnterCriticalSection(&_pdplock);
	SDWLResult res = m_PDP.ensurePDPConnectionReady(m_pdpDir, finished);
	::LeaveCriticalSection(&_pdplock);
	LOGDEBUG(NX::string_formater(L"ensurePDPConnectionReady: %d, pdpdir: %s", finished, m_pdpDir.c_str()));

	if (res.GetCode() == 0)
		return true;
	else {
		finished = false;
		return false;
	}
}

bool rmserv::IsProcessTrusted(unsigned long processId)
{
	//
	// Method 1:
	// Check if the PID of this process is a trusted process ID.
	//
	if (TrustedProcessIdsCheckExist(processId)) {
		return true;
	}

	//
	// Method 2:
	// Check if the executable of this process is a persistent trusted
	// application.
	//
	const process_record& proc_record = GLOBAL.safe_find_process(processId);
	if (proc_record.empty()) {
		return false;
	}
	const std::wstring image_path(proc_record.get_image_path());
	const auto &appEntry = m_effectiveTrustedApps.find(NX::conversion::wcslower(image_path));
	if (appEntry != m_effectiveTrustedApps.end()) {
		LOGDEBUG(NX::string_formater(L"rmserv::IsProcessTrusted: %s (PID %lu) found in m_effectiveTrustedApps", image_path.c_str(), processId));

		if (IsAppInIgnoreSignatureDirs(image_path)) {
			LOGDEBUG(NX::string_formater(L"rmserv::IsProcessTrusted: %s (PID %lu) is in ignore-signature dirs", image_path.c_str(), processId));
			return true;
		}

		std::vector<unsigned char> hash;
		GetFileHash(image_path, hash);

		LOGDEBUG(NX::string_formater(L"rmserv::IsProcessTrusted: %s (PID %lu) hash %s", image_path.c_str(), processId, hash==appEntry->second ? L"matches" : L"doesn't match"));
		if (hash == appEntry->second) {
			return true;
		}
	} else {
		LOGDEBUG(NX::string_formater(L"rmserv::IsProcessTrusted: %s (PID %lu) not found in m_effectiveTrustedApps", image_path.c_str(), processId));
	}

	//
	// Method 3:
	// Check if the executable of this process is a non-persistent trusted
	// application.
	//
	bool found = NonPersistentTrustedAppPathsCheckExist(image_path);
	LOGDEBUG(NX::string_formater(L"rmserv::IsProcessTrusted: %s (PID %lu) %s in NonPersistentTrustedAppPaths", image_path.c_str(), processId, found ? L"found" : L"not found"));
	return found;
}

bool rmserv::EvalRights(unsigned long processId,
	const std::wstring &userDispName,
	const std::wstring &useremail,
	const std::wstring &userID,
	const std::wstring &appPath,
	const std::wstring &resourceName,
	const std::wstring &resourceType,
	const std::vector<std::pair<std::wstring, std::wstring>> &attrs,
	const std::vector<std::pair<std::wstring, std::wstring>> &userAttrs,
	const std::wstring &bundle, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> *rightsAndWatermarks,
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> *rightsAndObligations, bool bcheckowner)
{
	LOGDEBUG(L"rmserv::EvalRights: ");

	SDWLResult res;
	for (auto att : attrs)
	{
		std::wstring key = att.first;
		std::wstring val = att.second;
		LOGDEBUG(NX::string_formater(L" attrs: key: %s, val: %s", key.c_str(), val.c_str()));
	}
	LOGDEBUG(NX::string_formater(L" bundle, %s", bundle.c_str()));

	// If the process is not trusted, return success and no rights.
	if (!IsProcessTrusted(processId))
	{
		LOGDEBUG(NX::string_formater(L" EvalRights: process %lu is not trusted", processId));
		rightsAndWatermarks->clear();
		return true;
	}

	res = m_PDP.EvalRights(userDispName, useremail, userID, appPath, resourceName, resourceType, attrs, userAttrs, bundle, rightsAndWatermarks, nullptr, bcheckowner);
	
	if (res.GetCode() == 0)
		return true;
	else
	{
		LOGDEBUG(NX::string_formater(L" EvalRights: failed, code: %d, %hs", res.GetCode(), res.GetMsg().c_str()));

		// Assume that:
		// 1. If the error is CE_RESULT_TIMEDOUT, it must be because PDP
		//    communication is still working fine in general, but evaluation
		//    is taking longer than our timeout value to finish.
		// 2. If evaluation is taking longer than our timeout value to finish,
		//    it must be because a DAP is taking too long to return the
		//    attributes.
		if (res.GetCode() == CE_RESULT_TIMEDOUT)
		{
			DWORD dwSessionId = 0;
			if (ProcessIdToSessionId(processId, &dwSessionId))
			{
				std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(dwSessionId);
				if (nullptr != sp)
				{
					std::vector<std::wstring> appPathComponents;
					NX::utility::Split<wchar_t>(appPath, L'\\', appPathComponents);
					const std::wstring notify_message = NX::string_formater(L"A DAP timed out while evaluating %s.  Please try the operation again.", resourceName.c_str());
					const NX::json_value v = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
						std::pair<std::wstring, NX::json_value>(L"application", NX::json_value(appPathComponents.back())),
						std::pair<std::wstring, NX::json_value>(L"message", NX::json_value(notify_message)),
						}), false);
					sp->get_rm_session().get_app_manager().send_popup_notification(v.serialize(), true);
				}
			}
		}

		return false;
	}
}

bool rmserv::PDPEvalRights(const std::wstring &userDispName,
	const std::wstring &useremail,
	const std::wstring &userID,
	const std::wstring &appPath,
	const std::wstring &resourceName,
	const std::wstring &resourceType,
	const std::vector<std::pair<std::wstring, std::wstring>> &attrs,
	const std::vector<std::pair<std::wstring, std::wstring>> &userAttrs,
	const std::wstring &bundle, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> *rightsAndWatermarks,
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> *rightsAndObligations)
{
	SDWLResult res = m_PDP.EvalRights(userDispName, useremail, userID, appPath, resourceName, resourceType, attrs, userAttrs, bundle, rightsAndWatermarks, nullptr);
	if (res.GetCode() == 0)
		return true;
	else
	{
		LOGDEBUG(NX::string_formater(L" PDPEvalRights: failed, code: %d, %hs", res.GetCode(), res.GetMsg().c_str()));
		return false;
	}
}

bool rmserv::StartPDPMan()
{
	SDWLResult res = m_PDP.StartPDPMan();
	if (res.GetCode() == 0)
		return true;
	else
	{
		LOGDEBUG(NX::string_formater(L" StartPDPMan: failed, code: %d, %hs", res.GetCode(), res.GetMsg().c_str()));
		return false;
	}
}

bool rmserv::StopPDPMan()
{
	SDWLResult res = m_PDP.StopPDPMan();
	if (res.GetCode() == 0)
		return true;
	else
	{
		LOGDEBUG(NX::string_formater(L" StopPDPMan: failed, code: %d, %hs", res.GetCode(), res.GetMsg().c_str()));
		return false;
	}
}

void rmserv::set_trusted_apps_from_config(unsigned long session_id, const std::map<std::wstring, std::vector<unsigned char>>& apps)
{
	m_trustedAppsFromConfig = apps;

	EffectiveTrustedAppsResolve();

	std::shared_ptr<winsession> sp = _win_session_manager.get_session(session_id);
	if (sp == nullptr) {
		LOGERROR(NX::string_formater(L"rmserv::set_trusted_apps_from_config: session not found"));
		return;
	}
	sp->get_rm_session().update_app_whitelist_config(m_effectiveTrustedApps);
}

//
//  class rmserv_conf
//
rmserv_conf::rmserv_conf() : _no_vhd(0), _network_timeout(RMS_DEFAULT_NETWORKTIMEOUT), _delay_seconds(0), _log_level(NX::dbg::LL_DEBUG), _log_size(RMS_DEFAULT_LOGSIZE), _log_rotation(10),_disable_antitampering(false), _disable_autoupgrade_install(false), _api_auth_type(0)
{
}

rmserv_conf::rmserv_conf(const std::wstring& key_path) : _no_vhd(0), _network_timeout(RMS_DEFAULT_NETWORKTIMEOUT), _delay_seconds(0), _log_level(NX::dbg::LL_DEBUG), _log_size(RMS_DEFAULT_LOGSIZE), _log_rotation(10), _api_auth_type(0)
{
    load(key_path);
}

rmserv_conf::rmserv_conf(unsigned long log_level, unsigned long log_size, unsigned long log_rotation, unsigned long delay_time, bool disable_auinstall, bool disable_antitampering)
    : _no_vhd(0), _network_timeout(RMS_DEFAULT_NETWORKTIMEOUT), _delay_seconds(delay_time), _log_level(log_level), _log_size((0 == log_size) ? RMS_DEFAULT_LOGSIZE : log_size), _log_rotation((0 == log_rotation) ? RMS_DEFAULT_LOGROTATION : log_rotation), _api_auth_type(0)
{
	_disable_antitampering = disable_antitampering ? 1 : 0;
	_disable_autoupgrade_install = disable_auinstall ? 1 : 0;
}

rmserv_conf::~rmserv_conf()
{
}

rmserv_conf& rmserv_conf::operator = (const rmserv_conf& other)
{
    if (this != &other) {
        _delay_seconds = other.get_delay_seconds();
		_no_vhd = other.get_no_vhd();
    _network_timeout = other.get_network_timeout();
		_log_level = other.get_log_level();
        _log_size = other.get_log_size();
		_disable_autoupgrade_install = other.is_disable_autoupgrade_install() ? 1 : 0;
		_disable_antitampering = other.is_enable_antitampering() ? 0 : 1;
    }
    return *this;
}

void rmserv_conf::clear()
{
    _trusted_apps.clear();
}

void rmserv_conf::load(const std::wstring& key_path) noexcept
{
    try {

        NX::win::reg_local_machine rgk;

        rgk.open(key_path.empty() ? NXRMS_SERVICE_KEY_PARAMETER : key_path, NX::win::reg_key::reg_wow64_64, true);

        try {
            rgk.read_value(RMS_CONF_DELAY_SECONDS, &_delay_seconds);
        }
        catch (std::exception& e) {
            UNREFERENCED_PARAMETER(e);
            _delay_seconds = 0;
        }

		try {
			rgk.read_value(RMS_CONF_NOVHD, &_no_vhd);
		}
		catch (std::exception& e) {
			UNREFERENCED_PARAMETER(e);
			_no_vhd = 0;
		}

    try {
      rgk.read_value(RMS_CONF_NETWORK_TIMEOUT, &_network_timeout);
    }
    catch (std::exception& e) {
      UNREFERENCED_PARAMETER(e);
      _network_timeout = RMS_DEFAULT_NETWORKTIMEOUT;
    }
    
    try {
			rgk.read_value(RMS_CONF_AUPGRADEINSTALL, &_disable_autoupgrade_install);
		}
		catch (std::exception& e) {
			UNREFERENCED_PARAMETER(e);
			_disable_autoupgrade_install = false;
		}

		try {
			rgk.read_value(RMS_CONF_ANTITAMPRING, &_disable_antitampering);
		}
		catch (std::exception& e) {
			UNREFERENCED_PARAMETER(e);
			_disable_antitampering = false;
		}

        try {
            rgk.read_value(RMS_CONF_LOG_LEVEL, &_log_level);
        }
        catch (std::exception& e) {
            UNREFERENCED_PARAMETER(e);
            _log_level = NX::dbg::LL_DEBUG;
        }

        try {
            rgk.read_value(RMS_CONF_LOG_SIZE, &_log_size);
        }
        catch (std::exception& e) {
            UNREFERENCED_PARAMETER(e);
            _log_size = 0;
        }
        if (_log_size > 64) {
            _log_size = 64;
        }
        if (0 == _log_size) {
            _log_size = RMS_DEFAULT_LOGSIZE;
        }

        try {
            rgk.read_value(RMS_CONF_LOG_ROTATION, &_log_rotation);
        }
        catch (std::exception& e) {
            UNREFERENCED_PARAMETER(e);
            _log_rotation = 0;
        }
        if (_log_rotation > 10) {
            _log_rotation = 10;
        }
        if (0 == _log_rotation) {
            _log_rotation = RMS_DEFAULT_LOGROTATION;
        }
    }
    catch (std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}

void rmserv_conf::load_from_xmlfile(const std::wstring& file)
{
    clear();

    try {
        NX::xml_document xmldoc;

        xmldoc.load_from_file(file);
        std::shared_ptr<NX::xml_node> spRoot = xmldoc.document_root();
        if (spRoot != nullptr) {
            std::shared_ptr<NX::xml_node> spTrustedApps = spRoot->find_child_element(L"TRUSTEDAPPS");
            if (spTrustedApps != nullptr) {
                const auto& children = spTrustedApps->get_children();

                for (const std::shared_ptr<NX::xml_node> spChild : children) {
                    if (spChild->get_name() == L"IMAGEPATH" && spChild->is_text_node()) {
                        const std::wstring& imagePath = spChild->get_text();
                        std::wstring s;

                        if (0 == GetLongPathNameW(imagePath.c_str(), NX::string_buffer<wchar_t>(s, MAX_PATH), MAX_PATH)) {
                            s = imagePath;
                        }

                        _trusted_apps.insert(NX::conversion::wcslower(s));
                    }
                }
            }

			std::shared_ptr<NX::xml_node> spAPIUser = spRoot->find_child_element(L"AUTHN");
			if (spAPIUser != nullptr) {
				const auto& children = spAPIUser->get_children();
				for (const std::shared_ptr<NX::xml_node> spChild : children) {
					if (spChild->get_name() == L"TYPE" && spChild->is_text_node()) {
						std::wstring authtype = spChild->get_text();
						std::transform(authtype.begin(), authtype.end(), authtype.begin(), toupper);
						if (authtype == L"SERVER")
							_api_auth_type = 1;
					}
					if (spChild->get_name() == L"APPID" && spChild->is_text_node()) {
						try
						{
							_api_app_id = std::stoi(spChild->get_text());
						}
						catch (...) {}
					}
					if (spChild->get_name() == L"APPKEY" && spChild->is_text_node()) {
						_api_app_key = spChild->get_text();
					}
					if (spChild->get_name() == L"EMAIL" && spChild->is_text_node()) {
						_api_email = spChild->get_text();
					}
					if (spChild->get_name() == L"PRIVATECERT" && spChild->is_text_node()) {
						_api_privatecert = spChild->get_text();
					}

					_api_working_folder = L"c:\\programdata";
					if (spChild->get_name() == L"WORKING_FOLDER" && spChild->is_text_node()) {
						_api_working_folder = spChild->get_text();
					}
				}
			}

        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}

void rmserv_conf::apply(const std::wstring& key_path)
{
    try {

        unsigned long ul = 0;
        NX::win::reg_local_machine rgk;

        rgk.create(key_path.empty() ? NXRMS_SERVICE_KEY_PARAMETER : key_path, NX::win::reg_key::reg_wow64_64);

        rgk.set_value(RMS_CONF_NOVHD, _no_vhd);
        rgk.set_value(RMS_CONF_NETWORK_TIMEOUT, _network_timeout);
        rgk.set_value(RMS_CONF_DELAY_SECONDS, _delay_seconds);
		rgk.set_value(RMS_CONF_LOG_LEVEL, _log_level);
        rgk.set_value(RMS_CONF_LOG_SIZE, _log_size);
    }
    catch (std::exception& e) {
        throw e;
    }
}


#ifdef _DEBUG
#define DEFAULT_ROUTER  L"https://rmtest.nextlabs.solutions"
#else
#define DEFAULT_ROUTER  L"https://r.skydrm.com"
#endif
#define DEFAULT_TENANT  L"skydrm.com"

router_config::router_config() : _router(DEFAULT_ROUTER), _tenant_id(DEFAULT_TENANT)
{
	//ReadRegKey();
}

router_config::router_config(const router_config& other) : _router(other.get_router()), _tenant_id(other.get_tenant_id())
{
	//ReadRegKey();
}

router_config::router_config(router_config&& other) : _router(std::move(other._router)), _tenant_id(std::move(other._tenant_id))
{
	//ReadRegKey();
}

router_config::~router_config()
{
}

bool router_config::SetRouterRegistry(const std::wstring& router, const std::wstring& tenant)
{
	HKEY hKey;
	LONG lRet;
	DWORD dwValue = 0;
	DWORD bufferSize = 2048;
	DWORD type = REG_SZ;
	std::wstring subkey = L"Software\\NextLabs\\RPM\\Router";
	std::wstring subkey1 = L"CurrentRouter";
	std::wstring subkey2 = L"Tenant";
	std::wstring value;

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ | KEY_SET_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
	{

		lRet = RegCreateKey(HKEY_LOCAL_MACHINE, subkey.c_str(), &hKey);
		if (lRet != ERROR_SUCCESS)
		{
			LOGDEBUG(NX::string_formater(L"SetRouterRegistry: RegCreateKey failed %s", subkey.c_str()));
			return false;
		}
	}

	lRet = RegSetValueEx(hKey, subkey1.c_str(), 0, REG_SZ, (const BYTE *)(router.c_str()), (DWORD)(router.size() + 1) * sizeof(WCHAR));
	if (lRet != ERROR_SUCCESS)
	{
		LOGERROR(NX::string_formater(L"SetRouterRegistry: RegSetValueEx failed %s", router.c_str()));
	}
	lRet = RegSetValueEx(hKey, subkey2.c_str(), 0, REG_SZ, (const BYTE *)(tenant.c_str()), (DWORD)(tenant.size() + 1) * sizeof(WCHAR));
	if (lRet != ERROR_SUCCESS)
	{
		LOGERROR(NX::string_formater(L"SetRouterRegistry: RegSetValueEx failed %s", tenant.c_str()));
	}

	RegCloseKey(hKey);
	return true;
}

router_config& router_config::operator = (const router_config& other)
{
    if (this != &other) 
	{
        _router = other.get_router();
        _tenant_id = other.get_tenant_id();
		//_workingfolder = other.get_workingfolder();
		//_tempfolder = other.get_tempfolder();
		//_pdpDir = other.get_sdklibfolder();
    }
    return *this;
}

router_config router_config::operator = (router_config&& other)
{
    if (this != &other) 
	{
        _router = std::move(other._router);
        _tenant_id = std::move(other._tenant_id);
		//_workingfolder = std::move(other.get_workingfolder());
		//_tempfolder = std::move(other.get_tempfolder());
		//_pdpDir = std::move(other.get_sdklibfolder());
    }
    return *this;
}

void router_config::clear()
{
    _router = DEFAULT_ROUTER;
    _tenant_id = DEFAULT_TENANT;
}

void router_config::load_from_file(const std::wstring& file)
{
    HANDLE h = INVALID_HANDLE_VALUE;

    clear();

    do {

        h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            break;
        }

        DWORD dwSize = GetFileSize(h, NULL);
        if (INVALID_FILE_SIZE == dwSize) {
            break;
        }

        std::string body;
        DWORD dwBytesRead = 0;
        body.resize(dwSize, 0);
        if (!::ReadFile(h, (LPVOID)body.data(), dwSize, &dwBytesRead, NULL)) {
            break;
        }

        try {
            NX::json_value json_body = NX::json_value::parse(body);
            if (json_body.as_object().has_field(L"router")) 
			{
                _router = json_body.as_object().at(L"router").as_string();
                if (_router.empty()) 
                    _router = DEFAULT_ROUTER;                
                else 
                    std::transform(_router.begin(), _router.end(), _router.begin(), tolower);
            }
            if (json_body.as_object().has_field(L"tenant")) 
			{
                _tenant_id = json_body.as_object().at(L"tenant").as_string();
                if (_tenant_id.empty()) 
                    _tenant_id = DEFAULT_TENANT;                
				else
				{
					//std::transform(_tenant_id.begin(), _tenant_id.end(), _tenant_id.begin(), tolower);
				}
                
            }
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }

    } while (FALSE);

    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
}

void router_config::load_from_xmlfile(const std::wstring& file)
{
    HANDLE h = INVALID_HANDLE_VALUE;

    clear();

    try {

        NX::xml_document xmldoc;

        xmldoc.load_from_file(file);
        std::shared_ptr<NX::xml_node> spRoot = xmldoc.document_root();
        if (spRoot != nullptr) {

            // get router
            std::shared_ptr<NX::xml_node> spServer = spRoot->find_child_element(L"SERVER");
            if (spServer != nullptr) {
                _router = spServer->get_text();
                if (_router.empty()) {
                    _router = DEFAULT_ROUTER;
                }
                else {
                    std::transform(_router.begin(), _router.end(), _router.begin(), tolower);
                }
            }
			LOGDEBUG(NX::string_formater(L" load_from_xmlfile: _router: %s", _tenant_id.c_str()));

            // get tenant
            std::shared_ptr<NX::xml_node> spTenant = spRoot->find_child_element(L"TENANTID");
            if (spTenant != nullptr) {
                _tenant_id = spTenant->get_text();
                if (_tenant_id.empty()) {
                    _tenant_id = DEFAULT_ROUTER;
                }
                else {
                    //std::transform(_tenant_id.begin(), _tenant_id.end(), _tenant_id.begin(), tolower);
                }
            }
			LOGDEBUG(NX::string_formater(L" _tenant_id: %s", _tenant_id.c_str()));
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}


void core_context_profile::add_context(const std::wstring& image, unsigned __int64 checksum, const std::vector<unsigned __int64>& data)
{
    const std::wstring& lower_case_image = NX::conversion::lower_str<wchar_t>(image);
    NX::utility::CRwExclusiveLocker locker(&_lock);
    std::map<unsigned __int64, std::vector<unsigned __int64>>& data_map = _context_map[lower_case_image];
    data_map[checksum] = data;
}

std::vector<unsigned __int64> core_context_profile::find_context(const std::wstring& image, unsigned __int64 checksum)
{
    const std::wstring& lower_case_image = NX::conversion::lower_str<wchar_t>(image);
    NX::utility::CRwSharedLocker locker(&_lock);
    auto pos = _context_map.find(lower_case_image);
    if (pos == _context_map.end()) {
        return std::vector<unsigned __int64>();
    }

    auto data_pos = (*pos).second.find(checksum);
    if (data_pos == (*pos).second.end()) {
        return std::vector<unsigned __int64>();
    }
    else {
        return (*data_pos).second;
    }
}

pipe_server::pipe_server() : NX::async_pipe_with_iocp::server(sizeof(QUERY_SERVICE_REQUEST), 5000)
{
}

pipe_server::~pipe_server()
{
}

void pipe_server::on_read(unsigned char* data, unsigned long* size, bool* write_response)
{
	*write_response = false;
	const unsigned long data_size = *size;
	if (0 == data_size) {
		return;
	}

	*size = 0;

	try {
		int process_id = 0;
		int thread_id = 0;
		int session_id = 0;
		std::string request;
		std::string s((const char*)data, (const char*)(data + data_size));
		NX::json_value json_body = NX::json_value::parse(s);
		if (json_body.as_object().has_field(L"process") && json_body.as_object().has_field(L"thread") && 
			json_body.as_object().has_field(L"session") && json_body.as_object().has_field(L"request"))
		{
			process_id = json_body.as_object().at(L"process").as_int32();
			thread_id = json_body.as_object().at(L"thread").as_int32();
			session_id = json_body.as_object().at(L"session").as_int32();
			request = NX::conversion::utf16_to_utf8(json_body.as_object().at(L"request").as_string());
		}

		const std::string& response_data = SERV->process_request(session_id, process_id, request);
		if (response_data.size() > 0 && response_data.size() <= sizeof(QUERY_SERVICE_REQUEST))
		{
			*write_response = true;
			memcpy(data, response_data.c_str(), response_data.size());
			*size = (unsigned long)response_data.size();
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
	}
}