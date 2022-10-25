//

#include <Windows.h>
#include <userenv.h>
#include <assert.h>
#include <Wtsapi32.h>
#include <TlHelp32.h>
#include <Shlwapi.h>
#include <string>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\winutil.hpp>
#include <nudf\string.hpp>
#include <nudf\dbglog.hpp>
#include <nudf\json.hpp>
#include <nudf\crypto.hpp>
#include <nudf\ntapi.hpp>
#include <nudf\asyncpipe.hpp>
#include <nudf\xml.hpp>
#include <nudf\bitmap.hpp>
#include <nudf\resutil.hpp>
#include <nudf\shared\rightsdef.h>
#include <nudf\filesys.hpp>
#include<nudf\uri.hpp>

#include <nxlfmthlp.hpp>

#include "nxrmserv.hpp"
#include "serv.hpp"
#include "nxrmflt.h"
#include "nxrmres.h"
#include "global.hpp"
#include "rsapi.hpp"
#include "logactivity.hpp"
#include "session.hpp"
#include "upgrade.hpp"
#include "networkstate.hpp"
#include "app_whitelist_config.h"
#include "cproperties.h"

#include <chrono>
#include <thread>

#pragma comment(lib, "Userenv.lib")

using namespace NX::dbg;
using namespace NXLOGDB;


extern rmserv* SERV;

// Max db size is 64 mega bytes
#define MAX_DB_SIZE     64
#define DEFAULT_TOKEN_EXPIRED_DAYS	15

class audit_logdb_conf : public NXLOGDB::db_conf
{
public:
    audit_logdb_conf(const std::string& description)
        : NXLOGDB::db_conf( description,
                            MAX_DB_SIZE,
                            NXLOGDB::BLOCK_1MB,
                            NXLOGDB::SECTOR_4096,
                            NXLOGDB::BLOCK_1MB,
                            NXLOGDB::record_layout(std::vector<NXLOGDB::field_definition>({
                                                                                                NXLOGDB::field_definition("timestamp", FIELD_INTEGER, FIELD_INTEGER_SIZE, FIELD_FLAG_SEQUENTIAL),
                                                                                                NXLOGDB::field_definition("device_type", FIELD_INTEGER, FIELD_INTEGER_SIZE, 0),
                                                                                                NXLOGDB::field_definition("operation", FIELD_INTEGER, FIELD_INTEGER_SIZE, 0),
                                                                                                NXLOGDB::field_definition("result", FIELD_INTEGER, FIELD_INTEGER_SIZE, 0),
                                                                                                NXLOGDB::field_definition("user_id", FIELD_INTEGER, FIELD_INTEGER_SIZE, 0),
                                                                                                NXLOGDB::field_definition("duid", FIELD_CHAR, 32, 0),
                                                                                                NXLOGDB::field_definition("owner", FIELD_CHAR, 64, 0),
                                                                                                NXLOGDB::field_definition("device_id", FIELD_CHAR, 64, 0),
                                                                                                NXLOGDB::field_definition("app_name", FIELD_CHAR, 64, 0),
                                                                                                NXLOGDB::field_definition("file_path", FIELD_CHAR, MAX_PATH, 0)
                                                                                            }
                            )))
    {
    }
    virtual ~audit_logdb_conf()
    {
    }
};

class audit_logdb_record : public NXLOGDB::db_record
{
public:
    audit_logdb_record() : NXLOGDB::db_record()
    {
    }

    audit_logdb_record(__int64 timestamp,
                       int device_type,
                       int user_operation,
                       int result,
                       __int64 user_id,
                       const std::wstring& duid,
                       const std::wstring& owner,
                       const std::wstring& device_id,
                       const std::wstring& app_name,
                       const std::wstring& file_path
                       ) : NXLOGDB::db_record(0xFFFFFFFF,
                                              std::vector<NXLOGDB::field_value>({
                                                                                    NXLOGDB::field_value(timestamp),
                                                                                    NXLOGDB::field_value(device_type),
                                                                                    NXLOGDB::field_value(user_operation),
                                                                                    NXLOGDB::field_value(result),
                                                                                    NXLOGDB::field_value(user_id),
                                                                                    NXLOGDB::field_value(duid, 32),
                                                                                    NXLOGDB::field_value(owner, 64),
                                                                                    NXLOGDB::field_value(device_id, 64),
                                                                                    NXLOGDB::field_value(app_name, 64),
                                                                                    NXLOGDB::field_value(file_path, MAX_PATH)
                                                                                }),
                                               std::vector<unsigned char>())
    {
    }

    virtual ~audit_logdb_record()
    {
    }
};

static std::map<std::wstring, std::wstring> get_environment_variables()
{
	std::map<std::wstring, std::wstring> variables;

	const wchar_t* sys_envs = ::GetEnvironmentStringsW();
	if (NULL == sys_envs) {
		return variables;
	}

	while (sys_envs[0] != L'\0') {
		std::wstring line(sys_envs);
		sys_envs += (line.length() + 1);
		auto pos = line.find(L'=');
		std::wstring var_name;
		std::wstring var_value;
		if (pos != std::wstring::npos) {
			var_name = line.substr(0, pos);
			var_value = line.substr(pos + 1);
		}
		else {
			var_name = line;
		}
		if (0 == _wcsicmp(var_name.c_str(), L"APPDATA")
			|| 0 == _wcsicmp(var_name.c_str(), L"LOCALAPPDATA")
			|| 0 == _wcsicmp(var_name.c_str(), L"USERPROFILE")
			|| 0 == _wcsicmp(var_name.c_str(), L"USERNAME")
			) {
			std::transform(var_name.begin(), var_name.end(), var_name.begin(), toupper);
		}
		variables[var_name] = var_value;
	}

	return std::move(variables);
}

static std::vector<wchar_t> create_environment_block(const std::map<std::wstring, std::wstring>& variables)
{
	std::vector<wchar_t> env_block;

	std::for_each(variables.begin(), variables.end(), [&](const std::pair<std::wstring, std::wstring>& var) {
		std::wstring line = var.first;
		line += L"=";
		line += var.second;
		std::for_each(line.begin(), line.end(), [&](const wchar_t& c) {
			env_block.push_back(c);
		});
		env_block.push_back(L'\0');
	});
	env_block.push_back(L'\0');

	return std::move(env_block);
}

//
//  class winsession
//

winsession::winsession() : _session_id(-1), _rm_session(this)
{
    //_rm_session._winsession = this;
}

winsession::winsession(unsigned long session_id) : _session_id(session_id), _rm_session(this)
{
    //_rm_session._winsession = this;
    init();
}

winsession::~winsession()
{
}

void winsession::clear()
{
    _rm_session.clear();
    _session_id = -1;
    _user_name.clear();
    _user_sid.clear();
    _user_dirs.clear();
    _temp_profiles_dir.clear();
    _protected_profiles_dir.clear();
}

void winsession::init()
{
    if (-1 == _session_id) {
        return;
    }

	if (is_server_mode() == TRUE)
	{
		// SYSTEM Service Account
		_user_sid = L"SID_SYSTEM";
		_user_name = L"SYSTEM";
		_temp_profiles_dir = GLOBAL.get_profiles_dir() + L"\\" + _user_sid;

		if (SERV->get_service_conf().get_no_vhd())
		{
			_protected_profiles_dir = GLOBAL.get_profiles_dir() + L"\\" + _user_sid;
			_profiles_dir = GLOBAL.get_profiles_dir();
		}
		else
		{
			_protected_profiles_dir = SERV->get_vhd_manager().get_config_volume().get_profiles_dir() + L"\\" + _user_sid;
			//Bug 57262 - registeredApps only available with current Windows user
			_profiles_dir = SERV->get_vhd_manager().get_config_volume().get_profiles_dir();
		}

		LOGDEBUG(NX::string_formater(L"winsession::init: _protected_profiles_dir: %s", _protected_profiles_dir.c_str()));
		LOGDEBUG(NX::string_formater(L"   _temp_profiles_dir: %s", _temp_profiles_dir.c_str()));
		LOGDEBUG(NX::string_formater(L"   config_dir: %s", GLOBAL.get_config_dir().c_str()));

		BOOL Existing = FALSE;
		if (!PathIsDirectory(_protected_profiles_dir.c_str()))
		{
			if (!NT::CreateDirectory(_protected_profiles_dir.c_str(), NULL, FALSE, &Existing))
			{
				DWORD ret = GetLastError();
				if (ret != ERROR_ALREADY_EXISTS)
				{
					LOGERROR(NX::string_formater(L"Fail to create protected profiles folder, error = %d (%d, %s, %s)", GetLastError(), _session_id, _user_name.c_str(), _protected_profiles_dir.c_str()));
				}
			}
		}

		if (!Existing) {
			LOGINFO(NX::string_formater(L"Protected profile folder doesn't exist, create it: (%d, %s, %s)", _session_id, _user_name.c_str(), _protected_profiles_dir.c_str()));
		}

		if (is_appstream())
		{
			set_appstream_default_rpm_folder(L"c:\\programdata");
		}
		else
		{
			std::wstring tmpDir = SERV->get_service_conf().get_api_working_folder();
			tmpDir += L"\\NextLabs";
			if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
				::CreateDirectoryW(tmpDir.c_str(), NULL);
			tmpDir += L"\\SkyDRM";
			if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
				::CreateDirectoryW(tmpDir.c_str(), NULL);
			tmpDir += L"\\Intermediate";
			if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
				::CreateDirectoryW(tmpDir.c_str(), NULL);
			_inter_dir = tmpDir + L'0'; // overwrite option
			_inter_dir = _inter_dir + L'0'; // ext option

			LOGINFO(NX::string_formater(L"winsession::init (session_id: %d, user name: %s, Intermediate Path: %s)", _session_id, _user_name.c_str(), _inter_dir.c_str()));

			SERV->get_fltserv().insert_safe_dir(_inter_dir);
		}

		if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(_temp_profiles_dir.c_str())) {
			LOGINFO(NX::string_formater(L"Temp profiles folder doesn't exist, create it: (%d, %s, %s)", _session_id, _user_name.c_str(), _temp_profiles_dir.c_str()));
			if (!::CreateDirectoryW(_temp_profiles_dir.c_str(), NULL)) {
				LOGERROR(NX::string_formater(L"Fail to create temp profiles folder for current user, error = %d (%d, %s, %s)", GetLastError(), _session_id, _user_name.c_str(), _temp_profiles_dir.c_str()));
			}
			std::wstring temp_dir_desktop = _temp_profiles_dir + L"\\Desktop";
			std::wstring temp_dir_appdata = _temp_profiles_dir + L"\\AppData";
			std::wstring temp_dir_appdata_local = temp_dir_appdata + L"\\Local";
			std::wstring temp_dir_appdata_roaming = temp_dir_appdata + L"\\Roaming";
			::CreateDirectoryW(temp_dir_desktop.c_str(), NULL);
			::CreateDirectoryW(temp_dir_appdata.c_str(), NULL);
			::CreateDirectoryW(temp_dir_appdata_local.c_str(), NULL);
			::CreateDirectoryW(temp_dir_appdata_roaming.c_str(), NULL);
		}

		_rm_session.initialize();

		// add dir after load_client_dir_config
		SERV->AddDirectoryConfig(_inter_dir);
		_rm_session.update_client_dir_config(SERV->get_dirs(), LOGIN_INFO::RPMFolder);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		_rm_session.update_client_dir_config(SERV->get_sanctuary_dirs(), LOGIN_INFO::sanctuaryFolder);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

		return;
	}

    NX::win::session_token st(_session_id);
    if (st.empty()) {
        LOGERROR(NX::string_formater(L"Fail to get session (%d) token, error = %d", _session_id, GetLastError()));
        return;
    }

    _user_dirs = NX::win::user_dirs(st);
    _user_sid = st.get_user().id();
    _user_name = st.get_user().name();
    _temp_profiles_dir = GLOBAL.get_profiles_dir() + L"\\" + _user_sid;

	if (SERV->get_service_conf().get_no_vhd())
	{
		_protected_profiles_dir = GLOBAL.get_profiles_dir() + L"\\" + _user_sid;
		_profiles_dir = GLOBAL.get_profiles_dir();
	}
	else
	{
		_protected_profiles_dir = SERV->get_vhd_manager().get_config_volume().get_profiles_dir() + L"\\" + _user_sid;
		//Bug 57262 - registeredApps only available with current Windows user
		_profiles_dir = SERV->get_vhd_manager().get_config_volume().get_profiles_dir();
	}

	LOGDEBUG(NX::string_formater(L"winsession::init: _protected_profiles_dir: %s", _protected_profiles_dir.c_str()));
	LOGDEBUG(NX::string_formater(L"   _temp_profiles_dir: %s", _temp_profiles_dir.c_str()));
	LOGDEBUG(NX::string_formater(L"   config_dir: %s", GLOBAL.get_config_dir().c_str()));

    BOOL Existing = FALSE;
	if (!PathIsDirectory(_protected_profiles_dir.c_str()))
	{
		if (!NT::CreateDirectory(_protected_profiles_dir.c_str(), NULL, FALSE, &Existing))
		{
			DWORD ret = GetLastError();
			if (ret != ERROR_ALREADY_EXISTS)
			{
				LOGERROR(NX::string_formater(L"Fail to create protected profiles folder, error = %d (%d, %s, %s)", GetLastError(), _session_id, _user_name.c_str(), _protected_profiles_dir.c_str()));
			}
		}
	}

    if (!Existing) {
        LOGINFO(NX::string_formater(L"Protected profile folder doesn't exist, create it: (%d, %s, %s)", _session_id, _user_name.c_str(), _protected_profiles_dir.c_str()));
    }

    NX::win::security_attribute sa(std::vector<NX::win::explicit_access>({
            NX::win::explicit_access(_user_sid, GENERIC_ALL, TRUSTEE_IS_USER, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
            NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
            NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT)
        }));

    if (is_appstream())
    {
        set_appstream_default_rpm_folder(_user_dirs.get_programdata());
    }
    else
    {
        std::wstring tmpDir = _user_dirs.get_local_appdata();
        tmpDir += L"\\NextLabs";
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
            ::CreateDirectoryW(tmpDir.c_str(), &sa);
        tmpDir += L"\\SkyDRM";
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
            ::CreateDirectoryW(tmpDir.c_str(), &sa);
        tmpDir += L"\\Intermediate";
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
            ::CreateDirectoryW(tmpDir.c_str(), &sa);
        _inter_dir = tmpDir + L'0'; // overwrite option
        _inter_dir = _inter_dir + L'0'; // ext option

        LOGINFO(NX::string_formater(L"winsession::init (session_id: %d, user name: %s, Intermediate Path: %s)", _session_id, _user_name.c_str(), _inter_dir.c_str()));

        SERV->get_fltserv().insert_safe_dir(_inter_dir);
    }

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(_temp_profiles_dir.c_str())) {
        LOGINFO(NX::string_formater(L"Temp profiles folder doesn't exist, create it: (%d, %s, %s)", _session_id, _user_name.c_str(), _temp_profiles_dir.c_str()));
        if (!::CreateDirectoryW(_temp_profiles_dir.c_str(), &sa)) {
            LOGERROR(NX::string_formater(L"Fail to create temp profiles folder for current user, error = %d (%d, %s, %s)", GetLastError(), _session_id, _user_name.c_str(), _temp_profiles_dir.c_str()));
        }
        std::wstring temp_dir_desktop = _temp_profiles_dir + L"\\Desktop";
        std::wstring temp_dir_appdata = _temp_profiles_dir + L"\\AppData";
        std::wstring temp_dir_appdata_local = temp_dir_appdata + L"\\Local";
        std::wstring temp_dir_appdata_roaming = temp_dir_appdata + L"\\Roaming";
        ::CreateDirectoryW(temp_dir_desktop.c_str(), &sa);
        ::CreateDirectoryW(temp_dir_appdata.c_str(), &sa);
        ::CreateDirectoryW(temp_dir_appdata_local.c_str(), &sa);
        ::CreateDirectoryW(temp_dir_appdata_roaming.c_str(), &sa);
    }

    impersonate_init(st);

    
    _rm_session.initialize();

	// Insert auto-protect option
	_inter_dir.insert(_inter_dir.size() - 2, 1, '0');
	// add dir after load_client_dir_config
    std::wstring wsid = get_windows_user_sid();
    std::wstring tenantid = get_rm_session().get_tenant_id();
    std::wstring userid = get_rm_session().get_profile().get_email();
    userid = tenantid + L"\\" + userid;
    SERV->AddDirectoryConfig(_inter_dir, SDRmRPMFolderConfig::RPMFOLDER_ADD, L"{}", wsid, userid);

	_rm_session.update_client_dir_config(SERV->get_dirs(), LOGIN_INFO::RPMFolder);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	_rm_session.update_client_dir_config(SERV->get_sanctuary_dirs(), LOGIN_INFO::sanctuaryFolder);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

    static const std::wstring tray_app_path(GLOBAL.get_bin_dir() + L"\\nxrmtray.exe");
    _rm_session.get_app_manager().start(_session_id, tray_app_path);
}

void winsession::impersonate_init(HANDLE token_handle)
{
    NX::win::impersonate_object imp(token_handle);

    if (!imp.is_impersonated()) {
        LOGERROR(NX::string_formater(L"Fail to impersonate session (%d) user, error = %d", _session_id, GetLastError()));
        return;
    }

    // Get Windows User Profile Directory

}

bool winsession::is_appstream()
{
    std::wstring aminame;
    std::wstring machine_image = LR"(SOFTWARE\Amazon\MachineImage)";

    NX::win::reg_key key;
    if (!key.exist(HKEY_LOCAL_MACHINE, machine_image.c_str()))
    {
        LOGINFO(NX::string_formater(L"winsession::is_appstream SOFTWARE\\Amazon\\MachineImage not exist!"));
        return false;
    }

    NX::win::reg_key::reg_position pos = NX::win::reg_key::reg_position::reg_default;
    key.open(HKEY_LOCAL_MACHINE, machine_image, pos, true);
    try
    {
        key.read_value(L"AMIName", aminame);
    }
    catch (const std::exception&)
    {
        LOGINFO(NX::string_formater(L"winsession::is_appstream SOFTWARE\\Amazon\\MachineImage\\AMIName value not exist!"));
        key.close();
        return false;
    }

    key.close();
    LOGINFO(NX::string_formater(L"winsession::is_appstream (session_id: %d, user name: %s, AMIName: %s)", _session_id, _user_name.c_str(), aminame.c_str()));

    return !aminame.empty();
}

void winsession::set_appstream_default_rpm_folder(const std::wstring& program_data)
{
    bool bInsert = false;
    DWORD dwErr = 0;
    std::wstring tmpDir = program_data;
    tmpDir += L"\\NextLabs";

    do {
        NX::win::security_attribute sa(std::vector<NX::win::explicit_access>({
        NX::win::explicit_access(_user_sid, GENERIC_ALL, TRUSTEE_IS_USER, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
        NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
        NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
        NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT)
            }));


        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
        {
            if (!::CreateDirectoryW(tmpDir.c_str(), &sa))
            {
                dwErr = ::GetLastError();
                break;
            }
        }

        tmpDir += L"\\SkyDRM";
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
        {
            if (!::CreateDirectoryW(tmpDir.c_str(), &sa))
            {
                dwErr = ::GetLastError();
                break;
            }
        }

        tmpDir += L"\\Intermediate";
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(tmpDir.c_str()))
        {
            if (!::CreateDirectoryW(tmpDir.c_str(), &sa))
            {
                dwErr = ::GetLastError();
                break;
            }
        }

        _inter_dir = tmpDir + L'0'; // overwrite option
        _inter_dir = _inter_dir + L'0'; // ext option

        bInsert = SERV->get_fltserv().insert_safe_dir(_inter_dir);

        std::wstring wsid = get_windows_user_sid();
        std::wstring tenantid = get_rm_session().get_tenant_id();
        std::wstring userid = get_rm_session().get_profile().get_email();
        userid = tenantid + L"\\" + userid;
        SERV->AddDirectoryConfig(_inter_dir, SDRmRPMFolderConfig::RPMFOLDER_ADD, L"{}", wsid, userid);
    } while (false);

    LOGINFO(NX::string_formater(L"set_appstream_default_rpm_folder (session_id: %d, user name: %s, _inter_dir Path: %s, dwErr:%d, bInsert:%d)",
        _session_id, _user_name.c_str(), _inter_dir.c_str(), dwErr, bInsert));
}

void winsession::execute(std::wstring &app, std::wstring &param)
{
	if (-1 == _session_id) {
		return;
	}

	if (app.size() <= 0)
		return;

	HANDLE          tk = NULL;
	STARTUPINFOW    si;
	PROCESS_INFORMATION pi;
	DWORD dwError = 0;

	if (_session_id != -1) {

		if (!WTSQueryUserToken(_session_id, &tk)) {
			LOGERROR(NX::string_formater(L"fail to get session token, error = %d", GetLastError()));
			return;
		}
		assert(NULL != tk);
		if (NULL == tk) {
			LOGERROR(NX::string_formater(L"fail to get session token, error = %d", GetLastError()));
			return;
		}
	}

    // Duplicate token
	HANDLE hTokenDup = NULL;
	if (!::DuplicateTokenEx(tk, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hTokenDup))
	{
		dwError = GetLastError();
		CloseHandle(tk);
		throw NX::exception(WIN32_ERROR_MSG(ERROR_INVALID_CLUSTER_IPV6_ADDRESS, "Fail to duplicate token"));
	}

	// Create env variables info
	LPVOID pEnv = NULL;
	if (!::CreateEnvironmentBlock(&pEnv, hTokenDup, FALSE))
	{
		dwError = GetLastError();

		::DestroyEnvironmentBlock(pEnv);
		CloseHandle(tk);
		CloseHandle(hTokenDup);

		throw NX::exception(WIN32_ERROR_MSG(ERROR_INVALID_CLUSTER_IPV6_ADDRESS, "Fail to create environment block"));
	}

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(STARTUPINFO);
	si.lpDesktop = L"WinSta0\\Default";
	if (!::CreateProcessAsUserW(tk, app.c_str(), (LPWSTR)param.c_str(), NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT, 
		pEnv, NULL, &si, &pi)) {
		dwError = GetLastError();
		::DestroyEnvironmentBlock(pEnv);
		CloseHandle(tk);
		LOGERROR(NX::string_formater(L"fail to create process %s, error = %d", app.c_str(), GetLastError()));
		return;
	}

	::DestroyEnvironmentBlock(pEnv);
	CloseHandle(pi.hThread);
	CloseHandle(tk);
}

//
//  class winsession_manager
//

winsession_manager::winsession_manager()
{
    ::InitializeCriticalSection(&_lock);
}

winsession_manager::~winsession_manager()
{
    ::DeleteCriticalSection(&_lock);
}

void winsession_manager::add_session(unsigned long session_id)
{
    ::EnterCriticalSection(&_lock);
    auto pos = _map.find(session_id);
    if (pos == _map.end()) {
        try {
            std::shared_ptr<winsession> sp(new winsession(session_id));
            _map[session_id] = std::shared_ptr<winsession>(sp);
        }
        catch (std::exception& e) {
            LOGERROR(NX::conversion::utf8_to_utf16(e.what()));
        }
    }
    ::LeaveCriticalSection(&_lock);
}

void winsession_manager::remove_session(unsigned long session_id)
{
    std::shared_ptr<winsession> sp;

    ::EnterCriticalSection(&_lock);
    auto pos = _map.find(session_id);
    if (pos != _map.end()) {
        sp = (*pos).second;
        _map.erase(pos);
    }
    ::LeaveCriticalSection(&_lock);
    sp->get_rm_session().get_app_manager().shutdown();
}

std::shared_ptr<winsession> winsession_manager::get_session(unsigned long session_id) const
{
    std::shared_ptr<winsession> result;

    ::EnterCriticalSection(&_lock);
	// try to use server mode session
	auto pos = _map.find(0);
    if (pos != _map.end()) {
        result = (*pos).second;
    }
	else
	{
		pos = _map.find(session_id);
		if (pos != _map.end()) {
			result = (*pos).second;
		}
	}
    ::LeaveCriticalSection(&_lock);

    return result;
}

void winsession_manager::clear()
{
    ::EnterCriticalSection(&_lock);
    _map.clear();
    ::LeaveCriticalSection(&_lock);
}

bool winsession_manager::empty()
{
    bool result = true;
    ::EnterCriticalSection(&_lock);
    result = _map.empty();
    ::LeaveCriticalSection(&_lock);
    return result;
}

std::vector<unsigned long> winsession_manager::get_all_session_id()
{
	std::vector<unsigned long> ids;
	for (auto& session : _map)
	{
		ids.push_back(session.first);
	}
	return ids;
}

std::vector<unsigned long> winsession_manager::find_existing_session()
{
    PWTS_SESSION_INFOW  ssinf = NULL;
    DWORD               count = 0;
    std::vector<unsigned long> session_list;

    if (WTSEnumerateSessionsW(NULL, 0, 1, &ssinf, &count) && NULL != ssinf) {
        for (int i = 0; i < (int)count; i++) {
            if (ssinf[i].State == WTSActive && 0 != ssinf[i].SessionId) {
                session_list.push_back(ssinf[i].SessionId);
            }
        }
        WTSFreeMemory(ssinf);
        ssinf = NULL;
    }

    return std::move(session_list);
}


//
//  class rmsession_timer
//

rmsession_timer::rmsession_timer()
    : NX::timer(30000),  // base interval is 5 minutes
    _rmsession(nullptr),
    _heartbeat_count(0),
    _log_count(0),
    _checkupdate_count(0),
	_force_flags(0)
{
}

rmsession_timer::~rmsession_timer()
{
}

#define TIMER_FLAG_FORCE_HEARTBEAT      0x00000001
#define TIMER_FLAG_FORCE_UPLOADLOG      0x00000002
#define TIMER_FLAG_FORCE_CHECKUPGRADE   0x00000004

void rmsession_timer::force_heartbeat()
{
    _force_flags |= TIMER_FLAG_FORCE_HEARTBEAT;
    force_trigger();
}

void rmsession_timer::force_uploadlog()
{
    _force_flags |= TIMER_FLAG_FORCE_UPLOADLOG;
    force_trigger();
}

void rmsession_timer::force_checkupdate()
{
    _force_flags |= TIMER_FLAG_FORCE_CHECKUPGRADE;
    force_trigger();
}

void rmsession_timer::force_all()
{
    _force_flags |= (TIMER_FLAG_FORCE_HEARTBEAT | TIMER_FLAG_FORCE_UPLOADLOG | TIMER_FLAG_FORCE_CHECKUPGRADE);
    force_trigger();
}

void rmsession_timer::on_timer()
{
    // Check & clear flags
    const bool flag_force_heartbeat = (TIMER_FLAG_FORCE_HEARTBEAT == (_force_flags & TIMER_FLAG_FORCE_HEARTBEAT));
    bool flag_force_uploadlog = (TIMER_FLAG_FORCE_UPLOADLOG == (_force_flags & TIMER_FLAG_FORCE_UPLOADLOG));
    const bool flag_force_checkupdate = (TIMER_FLAG_FORCE_CHECKUPGRADE == (_force_flags & TIMER_FLAG_FORCE_CHECKUPGRADE));
    if (flag_force_heartbeat) {
        _force_flags &= ~TIMER_FLAG_FORCE_HEARTBEAT;
    }
    if (flag_force_uploadlog) {
        _force_flags &= ~TIMER_FLAG_FORCE_UPLOADLOG;
    }
    if (flag_force_checkupdate) {
        _force_flags &= ~TIMER_FLAG_FORCE_CHECKUPGRADE;
    }

	// Save user data to file
	//		however, with DB (SQLite) solution, this is not required

	// save cached token for no-network
	_rmsession->save_user_cached_tokens();
	_rmsession->save_locked_duids();

	unsigned long  heartbeat_interval = _rmsession->get_client_config().get_heartbeat_interval();
    if (flag_force_heartbeat || ((heartbeat_interval > 1) && _heartbeat_count >= heartbeat_interval)) {
        _heartbeat_count = 0;
        _force_flags &= (~TIMER_FLAG_FORCE_HEARTBEAT);
        on_heartbeat();

		if (_rmsession->get_client_config().get_heartbeat_interval() == 0) {
			_rmsession->get_client_config().set_heartbeat_interval(120);
		}

        if (flag_force_heartbeat) {
            flag_force_uploadlog = true;
        }
    }
    else {
        _heartbeat_count += get_interval() / 1000;
    }

    if (flag_force_uploadlog || (_log_count >= _rmsession->get_client_config().get_log_interval())) {
        _log_count = 0;
        on_log();
    }
    else {
        _log_count += get_interval() / 1000;
    }

    if (flag_force_checkupdate || (_checkupdate_count >= _rmsession->get_local_config().get_check_update_interval())) {
        _checkupdate_count = 0;
        on_check_upgrade();
    }
    else {
        _checkupdate_count += get_interval() / 1000;
    }
}

void rmsession_timer::on_stop()
{
    _heartbeat_count = 0;
}

void rmsession_timer::on_heartbeat()
{
    _rmsession->on_heartbeat();
}

void rmsession_timer::on_log()
{
    _rmsession->on_uploadlog();
}

void rmsession_timer::on_check_upgrade()
{
    _rmsession->on_checkupgrade();
}


//
//  class eval_cache
//

eval_cache::eval_cache()
{
}

eval_cache::~eval_cache()
{
}

void eval_cache::push(std::shared_ptr<eval_result>& sp)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    if (_list.size() >= EVAL_CACHE_SIZE) {
        _list.pop_back();
    }
    _list.push_front(sp);
}

void eval_cache::clear()
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    _list.clear();
}

std::shared_ptr<eval_result> eval_cache::get(unsigned __int64 id) const
{
    NX::utility::CRwSharedLocker locker(&_lock);
    auto pos = std::find_if(_list.begin(), _list.end(), [&](const std::shared_ptr<eval_result>& sp) {
        return (id == sp->get_id());
    });
    
    return (pos == _list.end()) ? std::shared_ptr<eval_result>() : (*pos);
}



//
//  class rmsession
//

decrypt_token_cache::decrypt_token_cache()
{
}

decrypt_token_cache::~decrypt_token_cache()
{
}

std::vector<unsigned char> decrypt_token_cache::find(const std::vector<unsigned char>& id, unsigned long level) const
{
	std::vector<unsigned char> token;
	{
		NX::utility::CRwSharedLocker locker(&_lock);
		auto pos = _map.find(token_key(id, level));
		if (pos != _map.end()) {
			time_t curtime = time(NULL);
			if (curtime <= (*pos).first.get_ttl())
			{
				token = (*pos).second;
			}
			else
			{
				// token is expired
			}
		}
	}
	return std::move(token);
}

std::vector<unsigned char> decrypt_token_cache::find(const std::vector<unsigned char>& id, unsigned long level, time_t &ttl, std::vector<unsigned char> &otp) const
{
    std::vector<unsigned char> token;
    {
        NX::utility::CRwSharedLocker locker(&_lock);
        auto pos = _map.find(token_key(id, level));
        if (pos != _map.end()) {
			time_t curtime = time(NULL);
			if (curtime <= (*pos).first.get_ttl())
			{
				token = (*pos).second;
				ttl = (*pos).first.get_ttl();
				otp = (*pos).first.get_otp();
			}
			else
			{
				// token is expired
			}
        }
    }
    return std::move(token);
}

void decrypt_token_cache::delete_token(const std::vector<unsigned char>& id, unsigned long level) 
{	
	NX::utility::CRwSharedLocker locker(&_lock);
	std::map<token_key, std::vector<unsigned char>>::iterator it = _map.find(token_key(id, level));
	if (it != _map.end()) 
	{
		_map.erase(it);
	}
}

void decrypt_token_cache::insert(const std::vector<unsigned char>& id, unsigned long level, const std::vector<unsigned char>& token, time_t ttl, const std::vector<unsigned char>& otp)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
	time_t exptime = ttl;
	if (exptime <= 0)
	{
		time_t curtime = time(NULL);
		struct tm  tm;
		localtime_s(&tm, &curtime);
		tm.tm_mday += DEFAULT_TOKEN_EXPIRED_DAYS;
		exptime = mktime(&tm);
	}
	_map[token_key(id, level, exptime, otp)] = token;
}

void decrypt_token_cache::insert(const std::vector<unsigned char>& id, unsigned long level, const std::vector<unsigned char>& token)
{
	NX::utility::CRwExclusiveLocker locker(&_lock);
	time_t curtime = time(NULL);
	struct tm  tm;
	localtime_s(&tm, &curtime);
	tm.tm_mday += DEFAULT_TOKEN_EXPIRED_DAYS;
	time_t exptime = mktime(&tm);
	_map[token_key(id, level, exptime)] = token;
}

void decrypt_token_cache::clear()
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    _map.clear();
}




//
//  class rmsession
//

#define NAME_POLICY_BUNDLE      L"policyBundle"
#define NAME_CLIENT_CONFIG      L"clientConfig"
#define NAME_POLICY_CONFIG_DATA L"policyConfigData"
#define NAME_CLASSIFY_CONFIG    L"classifyConfig"
#define NAME_WATERMARK_CONFIG   L"watermarkConfig"

rmsession::rmsession() : _winsession(nullptr), _watermark_image_ready(false), _actlogger(this), _metalogger(this), _dwm_enabled(false)
{
    _timer._rmsession = this;
    _upload_serv._s = this;
}

rmsession::rmsession(winsession* p) : _winsession(p), _watermark_image_ready(false), _actlogger(this), _metalogger(this), _dwm_enabled(false)
{
    _timer._rmsession = this;
    _upload_serv._s = this;
}


rmsession::~rmsession()
{
    clear();
}

bool rmsession::init_from_cache()
{
    bool result = false;
    std::string content;
    const std::wstring temp_tenant_dir = _winsession->get_temp_profiles_dir() + L"\\" + _tenant_id;
    const std::wstring protected_tenant_dir = _winsession->get_protected_profiles_dir() + L"\\" + _tenant_id;
    const std::wstring cached_login_status_file = _winsession->get_protected_profiles_dir() + L"\\" + CACHE_LOGIN_STATUS_FILE;
    
    // make sure tenant directory exist
    NT::CreateDirectory(protected_tenant_dir.c_str(), NULL, FALSE, NULL);
    ::CreateDirectoryW(temp_tenant_dir.c_str(), NULL);

    if (!GLOBAL.nt_load_file(cached_login_status_file, content)) {
        return false;
    }

    try {

        NX::json_value json_logon_status = NX::json_value::parse(content);
        const std::wstring cached_tenant_id = json_logon_status.as_object().at(L"tenantId").as_string();
        const __int64 cached_user_id = json_logon_status.as_object().at(L"userId").as_int64();

        if (cached_tenant_id != _tenant_id) {
            LOGDEBUG(NX::string_formater(L"Cached login status tenant is different with current tenant (%s, %s)", cached_tenant_id.c_str(), _tenant_id.c_str()));
            throw std::exception("mismatch tenant");
        }

		// not login at service start - according to meeting conclusion
        //result = inter_logon(json_logon_status.as_object().at(L"loginResult"), true);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    if (!result) {
        _profile.clear();
        _temp_profile_dir.clear();
        _protected_profile_dir.clear();
        NT::DeleteFile(cached_login_status_file.c_str());
    }

    return result;
}

bool rmsession::ensure_rs_executor(bool enable)
{
    if (!_rs_executor.empty() && !enable) {
		//LOGDEBUG(L"_rs_executor is not empty");
        return true;
    }

    if (_tenant_id.empty() || enable)
	{
        _tenant_id = SERV->get_router_config().get_tenant_id();
    }

	LOGDEBUG(NX::string_formater(L"CheckInternetConnection: "));
	if (!CheckInternetConnection(SERV->get_router_config().get_router()))
	{
		LOGDEBUG(NX::string_formater(L"CheckInternetConnection: return false"));
		return false;
	}

	LOGDEBUG(NX::string_formater(L"ensure_rs_executor:  _rms_url: %s", _rms_url.c_str()));
    if (_rms_url.empty() || enable) 
	{
        if (!query_rms_url()) {
            LOGDEBUG(L"Fail to get RMS info");
            return false;
        }
    }
    if (_rms_url.empty()) {
		LOGDEBUG(L"_rms_url is empty");
        return false;
    }

    return _rs_executor.initialize(_rms_url);
}

void rmsession::initialize()
{
	LOGDEBUG(L"rmsession::initialize");
	load_client_dir_config();
	load_app_whitelist_config();
	ensure_rs_executor(true);
    init_from_cache();
}

bool rmsession::query_rms_url()
{
    if (_tenant_id.empty()) {
        _tenant_id = SERV->get_router_config().get_tenant_id();
    }

	LOGDEBUG(NX::string_formater(L"query_rms_url:  _tenant_id: %s", _tenant_id.c_str()));

    if (NX::ROUTER::query_tenant_info(SERV->get_router_config().get_router(), _tenant_id, _rms_url)) {
        return true;
    }

    _rms_url.clear();
    return false;
}

bool rmsession::query_token(_In_ unsigned long process_id, _In_ const std::wstring& owner_id, _In_ const std::vector<unsigned char>& agreement, _In_ const std::vector<unsigned char>& token_id, _In_ int protection_type, _In_ const std::wstring& policy, _In_ const std::wstring& tags, _In_ const unsigned long token_level, _Out_ std::vector<unsigned char>& token_value, _Out_opt_ bool* cached, _Out_opt_ bool* access_denied)
{
    bool result = false;
    int  statusCode = 0;
    
    if (nullptr != access_denied) {
        *access_denied = false;
    }

    // Don't return any token if the process is not trusted.
	if (!SERV->IsProcessTrusted(process_id)) {
		LOGDEBUG(NX::string_formater(L"query_token: process %lu is not trusted", process_id));
		if (nullptr != access_denied) {
			*access_denied = true;
		}
		token_value.clear();
		return false;
	}

    // Found in cache
    token_value = get_token_cache().find(token_id, token_level);
    if (token_value.size() == 32) {
        if (nullptr != cached) {
            *cached = true;
        }
        return true;
    }

    if (nullptr != cached) {
        *cached = false;
    }

    if (!ensure_rs_executor()) {
        return false;
    }

    assert(is_logged_on());

	//LOGDEBUG(NX::string_formater(L"query_token: CheckInternetConnection: "));
	if (!CheckInternetConnection(SERV->get_router_config().get_router()))
	{
		LOGDEBUG(NX::string_formater(L"query_token: CheckInternetConnection: return false"));
		return false;
	}

    if (!_rs_executor.request_retrieve_decrypt_token(process_id, get_profile().get_id(),
                                                     get_tenant_id(),
                                                     owner_id,
                                                     get_profile().get_token().get_ticket(),
                                                     agreement,
                                                     token_id,
                                                     protection_type,
                                                     policy,
                                                     tags,
                                                     token_level,
                                                     token_value,
                                                     &statusCode)) 
    {
        if (403 == statusCode && nullptr != access_denied) {
            *access_denied = true;
        }
        token_value.clear();
        return false;
    }
    if (token_value.size() != 32) {
        token_value.clear();
        return false;
    }
    
    get_token_cache().insert(token_id, token_level, token_value);
	_token_cache_dirty = true;
    return true;
}

void rmsession::clear()
{
    _appmanager.shutdown();
    logout(true);
    _tenant_id.clear();
    _rms_url.clear();
}

std::shared_ptr<policy_bundle> rmsession::get_policy_bundle() const
{
    NX::utility::CRwSharedLocker locker(&_policy_bundle_lock);
    return _policy_bundle;
}

void rmsession::reset_policy_bundle(std::shared_ptr<policy_bundle> sp)
{
    NX::utility::CRwExclusiveLocker locker(&_policy_bundle_lock);
    if (_policy_bundle.get() != sp.get()) {
        _policy_bundle.reset();
        _policy_bundle = sp;
    }
}

encrypt_token rmsession::popup_new_token(const std::wstring & membershipid)
{
	NX::utility::CRwExclusiveLocker locker(&_tokens_lock);

	if (_profile.is_me(membershipid) == false) {
		LOGINFO(NX::string_formater(L"Fail to acquire token because membership not exist (session: %d, member: %s)", _winsession->get_session_id(), membershipid.c_str()));
		return encrypt_token();
	}

	_profile.ensure_sufficient_tokens(membershipid, this);
	const encrypt_token new_token(_profile.pop_token(membershipid));
	if (new_token.empty()) {
		LOGINFO(NX::string_formater(L"Fail to acquire token because membership doesn't have any token (session: %d, member: %s)", _winsession->get_session_id(), membershipid.c_str()));
	}
	else
	{
		// save tokens
		//	after we use the DB (SQLite), this is not needed to save to file
		//	it is very inefficient to save all tokens when popup/request a token
		_profile.save_member_reserved_tokens(membershipid, _protected_profile_dir);
	}

	return new_token;
}

bool rmsession::insert_cached_token(const NX::json_value & cached_token) 
{
	if (!cached_token.is_object()) {
		return false;
	}
	if (!cached_token.as_object().has_field(L"id")) {
		return false;
	}

	if (!cached_token.as_object().has_field(L"value")) {
		return false;
	}

	std::vector<unsigned char> tokenOtp;
	time_t ttl = 0;
	if (cached_token.as_object().has_field(L"otp"))
	{
		tokenOtp = NX::conversion::hex_to_bytes(cached_token.as_object().at(L"otp").as_string());
	}
	if (cached_token.as_object().has_field(L"ttl"))
	{
		ttl = cached_token.as_object().at(L"ttl").as_int64();
	}

	std::vector<unsigned char> tokenId = NX::conversion::hex_to_bytes(cached_token.as_object().at(L"id").as_string());
	std::vector<unsigned char> tokenValue = NX::conversion::hex_to_bytes(cached_token.as_object().at(L"value").as_string());
	_token_cache.insert(tokenId, 0, tokenValue, ttl, tokenOtp);
	_token_cache_dirty = true;

	return true;
}

encrypt_token rmsession::find_file_token(const std::wstring & duid, time_t &ttl)
{
	std::vector<unsigned char> token_id = NX::conversion::hex_to_bytes(duid);
	std::vector<unsigned char> token_otp;
	std::vector<unsigned char> token_value = _token_cache.find(token_id, 0, ttl, token_otp);
	return encrypt_token(0, token_id, token_otp, token_value);
}

bool rmsession::delete_file_token(const std::wstring duid, unsigned long level)
{
	std::vector<unsigned char> tokenId = NX::conversion::hex_to_bytes(duid);
	_token_cache.delete_token(tokenId, level);
	_token_cache_dirty = true;
	return true;
}

bool rmsession::delete_file_token(const NX::json_value & cached_token)
{
	if (!cached_token.is_object()) {
		return false;
	}
	if (!cached_token.as_object().has_field(TOKENLIST_ELEMENT_NAME)) {
		return false;
	}
	const NX::json_value & tokenList = cached_token.as_object().at(TOKENLIST_ELEMENT_NAME);
	if (!tokenList.is_array()) {
		return false;
	}
	const NX::json_array & arrToken = tokenList.as_array();
	for (int i = 0; i < arrToken.size(); ++i) 
	{
		const NX::json_object & token = arrToken[i].as_object();
		std::vector<unsigned char> tokenId = NX::conversion::hex_to_bytes(token.at(TOKENID_ELEMENT_NAME).as_string());
		unsigned __int32 level = token.at(TOKENMAINTAINLEVEL_ELEMENT_NAME).as_uint32();
		//std::vector<unsigned char> tokenValue = NX::conversion::hex_to_bytes(token.at(TOKENKEY_ELEMENT_NAME).as_string());
		_token_cache.delete_token(tokenId, level);
	}
	_token_cache_dirty = true;
	return true;
}

void rmsession::request_add_activity_log(const NX::json_value& log)
{
	const NX::json_object& activity_log = log.as_object();

	std::wstring duid = activity_log.at(L"duid").as_string();
	std::wstring owner_id = activity_log.at(L"ownerid").as_string();
	std::wstring file_path = activity_log.at(L"file_path").as_string();
	std::wstring app_path = activity_log.at(L"app_path").as_string();
	std::wstring app_publisher = activity_log.at(L"company").as_string();
	int operation = activity_log.at(L"operation").as_int32();
	int result = activity_log.at(L"result").as_int32();
	std::vector<unsigned char> doc_duid;

	if (duid.size() == 0)
	{
		// we have case that RMX wants to add log. But RMX is trusted, so it can't read the NXL header to get DUID.
		// we need to read DUID here.
		// load NXL file context
		HANDLE h = ::CreateFileW(NX::fs::dos_fullfilepath(file_path).global_dos_path().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		NX::NXL::document_context context;
		if (INVALID_HANDLE_VALUE == h) {
			LOGWARNING(NX::string_formater(L"Eval: fail to open target file (%d): %s", GetLastError(), file_path.c_str()));
			return;
		}

		if (!context.load(h))
		{
			CloseHandle(h);
			return;
		}

		doc_duid = context.get_duid();
		owner_id = context.get_owner_id();
		CloseHandle(h);
	}
	else
	{
		doc_duid = NX::conversion::hex_to_bytes(duid);
	}

	log_activity2(activity_record(doc_duid,
		owner_id,
		get_profile().get_id(),
		operation,
		result,
		file_path,
		app_path,
		app_publisher,
		std::wstring()));
}

bool rmsession::request_add_metadata(const NX::json_value& log)
{
	std::wstring duid;
	std::wstring params;
	int projectid = 0;
	int type = 0;
	int option = 1;

	if (log.as_object().has_field(L"duid"))
		duid = log.as_object().at(L"duid").as_string();
	if (log.as_object().has_field(L"params"))
		params = log.as_object().at(L"params").as_string();
	if (log.as_object().has_field(L"projectid"))
		projectid = log.as_object().at(L"projectid").as_number().to_uint32();
	if (log.as_object().has_field(L"type"))
		type = log.as_object().at(L"type").as_number().to_uint32();
	if (log.as_object().has_field(L"option"))
		option = log.as_object().at(L"option").as_number().to_uint32();

    LOGDEBUG(NX::string_formater(" rmsession::request_add_metadata: DUID = %s, params = %s", duid.c_str(), params.c_str()));

	return log_metadata(metadata_record(duid,
		projectid,
		params,
		type), option);
}

void rmsession::request_lock_localfile(const NX::json_value& params)
{
	std::wstring duid;
	std::wstring path;
	bool bLock = true;

	if (params.as_object().has_field(L"duid"))
		duid = params.as_object().at(L"duid").as_string();
	if (params.as_object().has_field(L"path"))
		path = params.as_object().at(L"path").as_string();
	if (params.as_object().has_field(L"lock"))
		bLock = params.as_object().at(L"lock").as_boolean();

	if (duid.size() <= 0) return;

	NX::utility::CRwExclusiveLocker locker(&_lockedduids_lock);

	if (bLock)
	{
		// lock the DUID for metadata sync (admin rights purpose)
		std::map<std::wstring, std::wstring>::iterator it;
		it = _locked_duid_map.find(duid);
		if (it != _locked_duid_map.end())
		{
			(*it).second = path;
		}
		else
			_locked_duid_map[duid] = path;

		_locked_duid_dirty = true;
	}
	else
	{ 
		// unlock the DUID, so the metadata sync can be to RMS
		std::map<std::wstring, std::wstring>::iterator it;
		it = _locked_duid_map.find(duid);
		if (it != _locked_duid_map.end())
		{
			_locked_duid_map.erase(it);

			_locked_duid_dirty = true;

			// forcefully to update the metadata here
			_metalogger.upload_log();
		}
	}

	// Save it
}

bool rmsession::logon(const NX::json_value& logon_value)
{
    bool result = false;

    bool bRet = inter_logon(logon_value, false);

	std::thread evalrightsThread(pdp_app_preload, this);
	evalrightsThread.detach();

	return bRet;
}

bool rmsession::is_server_mode()
{
	if (_winsession)
		return _winsession->is_server_mode();

	return false;
}

bool rmsession::inter_logon(const NX::json_value& logon_value, bool cached)
{
    bool result = false;

    try {

        const NX::json_object& logon_data = logon_value.as_object();
        
        const __int64 userId = logon_data.at(L"userId").as_int64();

        result = _profile.create(logon_value);
        if (!result) {
            throw std::exception("Invalid logon data");
        }
        if (_profile.get_token().empty() || (_profile.get_token().expired() && cached)) {
            throw std::exception("token expired");
        }

        // ensure the user profile dir exists
		// _tenant_id should be the same as "defaultTenant"
        _temp_profile_dir = _winsession->get_temp_profiles_dir() + L"\\" + _tenant_id + L"\\" + NX::conversion::to_wstring(userId);
        _protected_profile_dir = _winsession->get_protected_profiles_dir() + L"\\" + _tenant_id + L"\\" + NX::conversion::to_wstring(userId);
		LOGINFO(NX::string_formater(L" inter_logon:  _protected_profile_dir: %s", _protected_profile_dir.c_str()));
		BOOL bret = 0; DWORD dret = 0;
		if (is_server_mode() == TRUE)
		{
			bret = ::CreateDirectoryW(_temp_profile_dir.c_str(), NULL);
		}
		else
		{
			NX::win::security_attribute sa(std::vector<NX::win::explicit_access>({
				NX::win::explicit_access(_winsession->get_windows_user_sid(), GENERIC_ALL, TRUSTEE_IS_USER, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
				NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
				NX::win::explicit_access(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
				}));
			bret = ::CreateDirectoryW(_temp_profile_dir.c_str(), &sa);
		}

		if (!bret)
		{
			dret = GetLastError();
			if (dret != ERROR_ALREADY_EXISTS) // 183
				LOGERROR(NX::string_formater(L"inter_logon: _temp_profile_dir= %s, error= %d", _temp_profile_dir.c_str(), dret));
		}
		bret = NT::CreateDirectory(_protected_profile_dir.c_str(), NULL, FALSE, NULL);
		{
			dret = GetLastError();
			if (dret != ERROR_ALREADY_EXISTS) // 183
				LOGERROR(NX::string_formater(L"inter_logon: _temp_profile_dir= %s, error= %d", _temp_profile_dir.c_str(), dret));
		}

        init_memberships();
		
		load_user_cached_tokens();
		load_locked_duids();
        // Log
        LOGINFO(NX::string_formater(L"User login successfully: %s", cached ? L"(cached)" : L" "));
        LOGINFO(NX::string_formater(L"  -> userId: %I64d, _protected_profile_dir: %s", _profile.get_id(), _protected_profile_dir.c_str()));
        LOGINFO(NX::string_formater(L"  -> userName: %s", _profile.get_name().c_str()));
        LOGINFO(NX::string_formater(L"  -> userEmail: %s", _profile.get_email().c_str()));
        const std::wstring& expire_time_str = _profile.get_token().get_expire_time().serialize(true, false);
        LOGINFO(NX::string_formater(L"  -> expireAt: %s", expire_time_str.c_str()));


        // Load all existing config file
		// when set clientid, update_client_config may fail
		if (!update_client_config(SERV->get_client_id()))
			LOGERROR(NX::string_formater(L"inter_logon: update_client_config failed:  clientid: %s", SERV->get_client_id().c_str()));
		
		load_policy_bundle_2();
		load_tokengroup_resourcetype();
        load_client_config();
        load_local_config();
        load_classify_config();
		LOGINFO(NX::string_formater(L"  -> clientid: %s", SERV->get_client_id().c_str()));
        _actlogger.init();
		_metalogger.init();
		init_logdb();
        SERV->get_fltserv().fltctl_set_policy_changed();
        
        if (load_watermark_config()) {
            // Prepare watermark image files
            const std::wstring watermark_file(_temp_profile_dir + L"\\watermark.png");
            if (!cached || !NX::fs::exists(watermark_file)) {
                if (!generate_watermark_image()) {
                    LOGERROR(NX::string_formater(L"Fail to generate watermark image: %s", watermark_file.c_str()));
                }
                else {
                    LOGDEBUG(NX::string_formater(L"Generate watermark image: %s", watermark_file.c_str()));
                }
            }
        }

        // Notify driver that a new session has logon
        SERV->get_fltserv().on_user_logon(_winsession->get_session_id(), get_profile().get_preferences().get_default_member());
		if (is_server_mode() == TRUE)
		{
			//
			std::vector<unsigned long> sessions = SERV->get_win_session_manager().get_all_session_id();
			for (size_t i = 0; i < sessions.size(); i++)
			{
				SERV->get_fltserv().on_user_logon(sessions[i], get_profile().get_preferences().get_default_member());
			}
		}

        // Logon succeed, notify client app to change status
        _appmanager.send_popup_notification(L"User logon successfully!");
        _appmanager.send_status_changed_notification();

        // Logon succeed from network, cache logon information
        if (!cached) {

            try {

                const std::wstring cached_login_status_file = _winsession->get_protected_profiles_dir() + L"\\" + CACHE_LOGIN_STATUS_FILE;

                NX::json_value cached_status = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                    std::pair<std::wstring, NX::json_value>(L"tenantId", NX::json_value(get_tenant_id())),
                    std::pair<std::wstring, NX::json_value>(L"userId", NX::json_value(_profile.get_id()))
                }), false);

                cached_status.as_object()[L"loginResult"] = logon_value;
                const std::string cached_content = NX::conversion::utf16_to_utf8(cached_status.serialize());

                GLOBAL.nt_generate_file(cached_login_status_file, cached_content, true);
            }
            catch (const std::exception& e) {
                UNREFERENCED_PARAMETER(e);
            }
        }

        // Enable WDM overlay
        if (does_watermark_image_exist()) {
            SERV->get_coreserv().drvctl_set_overlay_bitmap_status(_winsession->get_session_id(), true);
            _watermark_image_ready = true;
        }

		// Stop upload server if it's running
		_upload_serv.stop();

        // Start upload server
        _upload_serv.start();

		// update PDP config file and DAP config file
		if (ensure_rs_executor()) {
			int client_heartbeat = 0; bool adhoc = false; bool dapenable = false; std::wstring system_bucket_name; std::wstring icenetUrl;
			result = _rs_executor.get_tenant_preference(get_profile().get_id(), get_profile().get_token().get_ticket(), get_profile().get_default_tenantId(), &client_heartbeat, &adhoc, &dapenable, system_bucket_name, icenetUrl);
			if (result && !icenetUrl.empty()) {
				std::wstring pdpurl = icenetUrl + L"/dabs";
				if (SERV->StopPDPMan())
				{
					if (!update_pdp_url(pdpurl)) {
						LOGDEBUG(NX::string_formater(L"Failed to update PDP DABSLocation: %s", pdpurl.c_str()));
					}
					//if (!update_dap_configfile(_rms_url)) {
					//	LOGDEBUG(NX::string_formater(L"Failed to update DAP config file: %s", _rms_url.c_str()));
					//}

					//if (!update_dap_configfile2(_rms_url)) {
					//	LOGDEBUG(NX::string_formater(L"Failed to update DAP DynamicAttributeService.properties config file: %s", _rms_url.c_str()));
					//}

					//if (!change_dap_filename(dapenable)){
					//	LOGDEBUG(NX::string_formater(L"Failed to change DAP file name"));
					//}

					if (!SERV->StartPDPMan()){
						LOGDEBUG(NX::string_formater(L"Failed to start PDP main"));

                        {//Fix bug 71966, cepdpman.exe stoped
                            std::thread st([]() {
                                for (int i = 0; i < 3; i++)
                                {
                                    std::this_thread::sleep_for(std::chrono::seconds(i+1));
                                    if (SERV->StartPDPMan())
                                    {
                                        LOGDEBUG(NX::string_formater(L"After Failed to start PDP main, Try to start PDP successfully, tried times:%d", i));
                                        break;
                                    }

                                    LOGDEBUG(NX::string_formater(L"Failed to start PDP main 2, tried times:%d", i));
                                }
                            });

                            st.detach();
                        }
					}
				}
                else
                {
                    {//Fix bug 71966, cepdpman.exe stoped
                        std::thread st([]() {
                            LOGDEBUG(NX::string_formater(L"StartPDPMan failed ... "));
                            //wait cepdpman.exe exit
                            std::this_thread::sleep_for(std::chrono::seconds(2));

                            //try to start cepdpman.exe
                            for (int i = 0; i < 3; i++)
                            {
                                std::this_thread::sleep_for(std::chrono::seconds(i + 1));
                                if (SERV->StartPDPMan())
                                {
                                    LOGDEBUG(NX::string_formater(L"After StopPDPMan failed, Try to start PDP successfully, tried times:%d", i));
                                    break;
                                }

                                LOGDEBUG(NX::string_formater(L"After StopPDPMan failed, Failed to start PDP main 2, tried times:%d", i));
                            }
                        });

                        st.detach();
                    }
                }
			}
		}

        // Initiate heartbeat
        _timer.start();
        _timer.force_all();
    }
    catch (const std::exception& e) {
        LOGWARNING(NX::string_formater(L"rmsession::logon error -> %S", e.what()));
        result = false;
        _profile.clear();
        _temp_profile_dir.clear();
        _protected_profile_dir.clear();
    }

    return result;
}

void rmsession::logout(bool keep_logon_data)
{
    const std::wstring cached_login_status_file = _winsession->get_protected_profiles_dir() + L"\\" + CACHE_LOGIN_STATUS_FILE;
    
    // Disable WDM overlay
    _watermark_image_ready = false;
    SERV->get_coreserv().drvctl_set_overlay_bitmap_status(_winsession->get_session_id(), false);

	std::string response;
	if (SERV->IsFileOpen(_winsession->get_session_id(), response))
		return;

    if (is_logged_on()) {

        // stop timer
        _timer.stop();

        // Stop upload server
        _upload_serv.stop();

        _actlogger.clear();
		_metalogger.clear();

        // Notify driver that user has logged off
        SERV->get_fltserv().on_user_logoff(_winsession->get_session_id());

        // Save local config file
        const std::wstring local_config_file(_protected_profile_dir + L"\\" + CLIENT_LOCAL_CONFIG_FILE);
        const std::wstring& content = _local_config.serialize();
        GLOBAL.nt_generate_file(local_config_file, NX::conversion::utf16_to_utf8(content), true);
		LOGDEBUG(NX::string_formater(L"logout: local_config: (%s)", content.c_str()));
		const std::wstring dir_config_file(_winsession->get_protected_profiles_dir() + L"\\" + CLIENT_DIR_CONFIG_FILE);
		const std::wstring& content1 = _client_dir_config.serialize();
		GLOBAL.nt_generate_file(dir_config_file, NX::conversion::utf16_to_utf8(content1), true);
		LOGDEBUG(NX::string_formater(L"  _client_dir_config: (%s)", content1.c_str()));

		save_locked_duids();
		_locked_duid_map.clear();
		// Save unused tokens back to file
		_profile.save_member_reserved_tokens(_protected_profile_dir);

		if (!_token_cache.getTokenMap().empty()) {
			save_user_cached_tokens();
		}
    }

    _eval_cache.clear();
    _token_cache.clear();
    clear_logdb();
	_rs_executor.clear();
	_rms_url.clear();
	_tenant_id.clear();

    if (!keep_logon_data) {
        NT::DeleteFile(cached_login_status_file.c_str());
    }
    reset_policy_bundle();
    _profile.clear();
    _temp_profile_dir.clear();
    _protected_profile_dir.clear();
    _client_config.clear();
    _local_config.clear();
    _watermark_config.clear();
    _config_serial_numbers.clear();
}

bool rmsession::get_document_eval_info(_In_ HANDLE h, _In_ NX::NXL::document_context& context, _In_ const unsigned char* file_token, _Out_ eval_data& ed, _Out_ std::shared_ptr<policy_bundle>& policy)
{
    NX::NXL::section_record fileinfo_record;
    NX::NXL::section_record filepolicy_record;
    NX::NXL::section_record filetag_record;
    
    ed.push_data(L"resource.duid", NX::generic_value(NX::conversion::to_wstring(context.get_duid())));
    ed.push_data(L"resource.owner", NX::generic_value(context.get_owner_id()));

    if (!context.find_default_section(fileinfo_record, filepolicy_record, filetag_record)) {
        return false;
    }
    if (fileinfo_record.empty() || filepolicy_record.empty() || filetag_record.empty()) {
        return false;
    }

    std::string content;

    // Get file info
    if (!context.read_section_string(h, fileinfo_record, file_token, content)) {
        LOGDEBUG(NX::string_formater(L"Fail to read file info section (%d)", GetLastError()));
        return false;
    }
    if (!content.empty()) {
        try {
            NX::json_value v = NX::json_value::parse(content);
            std::for_each(v.as_object().begin(), v.as_object().end(), [&](const std::pair<std::wstring, NX::json_value>& item) {
                const std::wstring name(L"resource." + NX::conversion::lower_str<wchar_t>(item.first));
                const NX::generic_value& value = json_to_generic(item.second);
                if (!name.empty() && !value.is_null()) {
                    ed.push_data(name, value);
                }
            });
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }
    }
    content.clear();

    // Get file tags
    if (!context.read_section_string(h, filetag_record, file_token, content)) {
        LOGDEBUG(NX::string_formater(L"Fail to read file tag section (%d)", GetLastError()));
        return false;
    }
    if (!content.empty()) {
        try {
			LOGDEBUG(NX::string_formater("get_document_eval_info: tags: %s", content.c_str()));
            NX::json_value v = NX::json_value::parse(content);
            std::for_each(v.as_object().begin(), v.as_object().end(), [&](const std::pair<std::wstring, NX::json_value>& item) {
                const std::wstring name(L"resource." + NX::conversion::lower_str<wchar_t>(item.first));
                const NX::generic_value& value = json_to_generic(item.second);
                if (!name.empty() && !value.is_null()) {
                    ed.push_data(name, value);
                }
            });
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }
    }
    content.clear();

    // Get file policy
    if (!context.read_section_string(h, filepolicy_record, file_token, content)) {
        LOGDEBUG(NX::string_formater(L"Fail to read file policy section (%d)", GetLastError()));
        return false;
    }
    if (!content.empty()) {
        policy = policy_bundle::parse(NX::conversion::utf8_to_utf16(content));
    }

    return true;
}

bool rmsession::get_document_eval_info(_In_ const unsigned long process_id, _In_ const std::wstring _file, _Out_ std::shared_ptr<policy_bundle>& policy,
	_Out_ std::string &adpolicy, _Out_ std::string &tags, _Out_ std::string &info, _Out_ std::string &duid, _Out_ std::string &creator, _Out_ std::string &tokengroup, _Out_ DWORD &attributes)
{
	HANDLE h = INVALID_HANDLE_VALUE;
	{
		NX::win::session_token st(_winsession->get_session_id());
		NX::win::impersonate_object impersonobj(st);

		NX::fs::dos_fullfilepath file(_file);
		attributes = ::GetFileAttributes(file.global_dos_path().data());
		h = ::CreateFileW(file.global_dos_path().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == h) {
			return false;
		}
	}
	
	NX::NXL::document_context context;
	{
		NX::win::session_token st(_winsession->get_session_id());
		NX::win::impersonate_object impersonobj(st);

		// load NXL file context
		if (!context.load(h)) {
			CloseHandle(h);
			return false;
		}
	}
	
	NX::NXL::section_record fileinfo_record;
	NX::NXL::section_record filepolicy_record;
	NX::NXL::section_record filetag_record;

	duid = NX::conversion::to_string(context.get_duid());
	creator = NX::conversion::utf16_to_utf8(context.get_owner_id());
	tokengroup = context.get_tenant();

	const std::wstring doc_policy = NX::conversion::utf8_to_utf16(context.get_policy());
	const std::wstring doc_tags = NX::conversion::utf8_to_utf16(context.get_tags());
	const std::wstring doc_info = NX::conversion::utf8_to_utf16(context.get_info());

	info = NX::conversion::utf16_to_utf8(doc_info);
	adpolicy = NX::conversion::utf16_to_utf8(doc_policy);
	tags = NX::conversion::utf16_to_utf8(doc_tags);
	if (doc_policy.size() > 2)
		policy = policy_bundle::parse(doc_policy);

	/*
	const std::wstring& ownerid = context.get_owner_id();
	std::vector<unsigned char> file_token;
	bool token_access_denied = false;
	int protection_type = (doc_policy.empty() || doc_policy == L"{}") ? 1 : 0;   // 1 = central, 0 = ad-hoc
	bool token_from_cache = false;
	if (!query_token(process_id, ownerid, context.get_agreement0(), context.get_duid(), protection_type, doc_policy, doc_tags, context.get_token_level(), file_token, &token_from_cache, &token_access_denied)) {
		// fail to get token
		CloseHandle(h);
		return true;
	}
	if (file_token.size() != 32) {
		CloseHandle(h);
		return true;
	}

	if (!context.find_default_section(fileinfo_record, filepolicy_record, filetag_record)) {
		CloseHandle(h);
		return true;
	}
	if (fileinfo_record.empty() || filepolicy_record.empty() || filetag_record.empty()) {
		CloseHandle(h);
		return true;
	}

	std::string content;

	// Get file info
	if (!context.read_section_string(h, fileinfo_record, file_token.data(), content)) {
		CloseHandle(h);
		return true;
	}
	if (!content.empty()) {
		info = content;
	}
	content.clear();

	// Get file tags
	if (!context.read_section_string(h, filetag_record, file_token.data(), content)) {
		CloseHandle(h);
		return true;
	}
	if (!content.empty()) {
		tags = content;
	}
	content.clear();

	// Get file policy
	if (!context.read_section_string(h, filepolicy_record, file_token.data(), content)) {
		CloseHandle(h);
		return true;
	}
	if (!content.empty()) {
		adpolicy = content;
		policy = policy_bundle::parse(NX::conversion::utf8_to_utf16(content));
	}
	*/
	CloseHandle(h);

	return true;
}

static std::wstring get_process_commandline(unsigned long process_id)
{
    HANDLE h = NULL;
    std::wstring commandline;

    typedef NTSTATUS (WINAPI* NtQueryInformationProcess_t)(
        _In_      HANDLE           ProcessHandle,
        _In_      PROCESSINFOCLASS ProcessInformationClass,
        _Out_     PVOID            ProcessInformation,
        _In_      ULONG            ProcessInformationLength,
        _Out_opt_ PULONG           ReturnLength
    );

    static NtQueryInformationProcess_t PtrNtQueryInformationProcess = (NtQueryInformationProcess_t)::GetProcAddress(LoadLibraryW(L"ntdll.dll"), "NtQueryInformationProcess");

    if (NULL == PtrNtQueryInformationProcess) {
        return commandline;
    }
    
    do {

        PROCESS_BASIC_INFORMATION pbi = {0};
        ULONG_PTR returned_length = 0;

        h = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
        if (NULL == h) {
            break;
        }

        LONG status = PtrNtQueryInformationProcess(h, ProcessBasicInformation, &pbi, sizeof(pbi), (PULONG)&returned_length);
        if (0 != status) {
            break;
        }

        PEB peb = {0};
        if (!ReadProcessMemory(h, pbi.PebBaseAddress, &peb, sizeof(PEB), &returned_length)) {
            break;
        }

        RTL_USER_PROCESS_PARAMETERS upp = { 0 };
        if (!ReadProcessMemory(h, peb.ProcessParameters, &upp, sizeof(RTL_USER_PROCESS_PARAMETERS), &returned_length)) {
            break;
        }

        if (0 == upp.CommandLine.Length) {
            break;
        }

        std::vector<wchar_t> buf;
        buf.resize((upp.CommandLine.Length + 3) / 2, 0);
        if (!ReadProcessMemory(h, upp.CommandLine.Buffer, buf.data(), upp.CommandLine.Length, &returned_length)) {
            break;
        }

        commandline = buf.data();

    } while(false);

    if (h != NULL) {
        CloseHandle(h);
        h = NULL;
    }

    return std::move(commandline);
}

std::vector<std::pair<std::wstring, std::wstring>> ImportJsonTags(const std::string &str)
{
	std::vector<std::pair<std::wstring, std::wstring>> attrs;
	const std::wstring& s = NX::conversion::utf8_to_utf16(str);
	try {

		NX::json_value info = NX::json_value::parse(s);
		const NX::json_object& object = info.as_object();
		for (auto& item : object)
		{
			std::wstring key = item.first;
			const NX::json_array& policy_array = item.second.as_array();
			for (auto& att : policy_array)
			{
				std::wstring val = att.as_string();
				attrs.push_back(std::pair<std::wstring, std::wstring>(key, val));
				LOGDEBUG(NX::string_formater(L" attrs: key: %s, val: %s", key.c_str(), val.c_str()));
			}

		}
	}
	catch (std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		SetLastError(ERROR_INVALID_DATA);
		
	}
	return attrs;
}

//{
//	"application": "SkyDRM",
//		"fileStatus" : 1,
//		"message" : "Protected successfully.",
//		"msgtype" : 0,
//		"operation" : "Protect",
//		"result" : 1,
//		"target" : "normal-5-2019-12-27-06-44-01.txt.nxl"
//}

void rmsession::block_notify(const std::wstring& file, const std::wstring& appname)
{
    static const std::wstring operation_name = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_OPERATION_OPEN, 256, LANG_NEUTRAL, L"open");
    NX::fs::dos_fullfilepath dospath(file);
    const std::wstring fn = dospath.file_name().full_name();
    const std::wstring block_message = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You do not have permission to view this file.");

	NX::json_value v = NX::json_value::create_object();
	v[L"application"] = NX::json_value(appname);
	v[L"fileStatus"] = NX::json_value(1);
	v[L"message"] = NX::json_value(block_message);
	v[L"msgtype"] = NX::json_value(1);
	v[L"operation"] = NX::json_value(operation_name);
	v[L"result"] = NX::json_value(1);
	v[L"target"] = NX::json_value(fn);

	std::wstring strJsonMessage = v.serialize();
	get_app_manager().send_popup_notification(strJsonMessage, true);
}

bool rmsession::pre_rights_evaluate(const std::wstring& file, unsigned long process_id, unsigned long session_id)
{
	LOGINFO1(NX::string_formater(L"begin pre_rights_evaluate session : %d ,process : %d, file : %s", session_id, process_id, file.c_str()));

	const process_record& proc_record = GLOBAL.safe_find_process(process_id);
	if (proc_record.empty())
	{
		LOGINFO1(NX::string_formater(L"end pre_rights_evaluate proc_record.empty, session : %d ,process : %d, file : %s",
			session_id, process_id, file.c_str()));
		return true;
	}

	const std::wstring strAppPath(proc_record.get_image_path());
	std::wstring strAppName;
	std::size_t pos = strAppPath.rfind(L"\\");
	if (std::wstring::npos == pos)
	{
		LOGINFO1(NX::string_formater(L"end pre_rights_evaluate not find \\, session : %d ,process : %d, file : %s",
			session_id, process_id, file.c_str()));
		return true;
	}

	strAppName = strAppPath.substr(pos + 1);
	std::transform(strAppName.begin(), strAppName.end(), strAppName.begin(), ::towlower);
	if (0 != strAppName.compare(L"winword.exe"))
	{
		LOGINFO1(NX::string_formater(L"end pre_rights_evaluate session : %d ,process : %d, file : %s, apppath : %s",
			session_id, process_id, file.c_str(), strAppPath.c_str()));
		return true;
	}

	static std::array<std::wstring, 9> arrImageExtension = {
		L".tiff", L".tif", L".jpg", L".png",L".gif",
		L".bmp", L".jpeg", L".dxf",  L".eps" };

	bool bImageExt = false;
	std::wstring strFilePath = NX::fs::dos_fullfilepath(file).path();
	std::transform(strFilePath.begin(), strFilePath.end(), strFilePath.begin(), ::towlower);
	for (auto item : arrImageExtension)
	{
		std::size_t pos = strFilePath.rfind(item);
		if (std::wstring::npos == pos)
		{
			continue;
		}

		size_t stLen = pos + item.length();
		if (strFilePath.length() == stLen)
		{
			bImageExt = true;
			break;
		}
	}

	LOGINFO1(NX::string_formater(L"end pre_rights_evaluate image type : %d, session : %d, process : %d, file : %s",
		bImageExt, session_id, process_id, file.c_str()));

	return !bImageExt;
}

void rmsession::adhoc_obligation_to_watermark(unsigned long long rights, const std::string& policy, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& rightsAndWatermarks)
{
	std::string json_policy = policy;
	NX::json_value json_context = NX::json_value::parse(json_policy);

	if (!json_context.as_object().has_field(L"policies"))
		return;

	NX::json_array arr_policies = json_context.as_object().at(L"policies").as_array();
	for (auto polices_item : arr_policies)
	{
		NX::json_array arr_obligations = polices_item.as_object().at(L"obligations").as_array();
		std::vector<SDR_WATERMARK_INFO> vecWatermarkInfo;
		for (auto obligation_item : arr_obligations)
		{
			SDR_WATERMARK_INFO watermarkInfo;
			watermarkInfo.fontSize = 0;
			watermarkInfo.transparency = 0;
			watermarkInfo.rotation = WATERMARK_ROTATION::NOROTATION;
			watermarkInfo.repeat = false;

			NX::json_value obj_value = obligation_item.as_object().at(L"value");

			if (obj_value.as_object().has_field(L"text"))
			{
				std::wstring userid = this->get_profile().get_email();
				auto tempText = NX::conversion::utf16_to_utf8(obj_value.as_object().at(L"text").as_string());

#pragma region  converter to run time watermark
				int begin = -1;
				int end = -1;
				std::string converted_wmtext;
				std::string wmtext = tempText;
				for (size_t i = 0; i < wmtext.size(); i++)
				{
					if (wmtext.data()[i] == '$')
					{
						if (begin >= 0)
						{
							std::string s2 = wmtext.substr(begin, (i - begin));
							converted_wmtext.append(s2);
						}
						begin = (int)i;
					}
					else if (wmtext.data()[i] == ')')
					{
						end = (int)i;
						if (begin >= 0)
						{
							std::string s1 = wmtext.substr(begin, end - begin + 1);
							std::string s2 = wmtext.substr(begin, end - begin + 1);
							std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
							bool has_premacro = false;

							if (s2 == "$(user)")
							{
								converted_wmtext.append(NX::conversion::utf16_to_utf8(userid)); // userName
								converted_wmtext.append(" ");
								has_premacro = true;
							}
							else if (s2 == "$(date)")
							{
								std::string strdate;
								time_t rawtime;
								struct tm timeinfo;
								char date_buffer[256] = { 0 };
								char time_buffer[256] = { 0 };
								time(&rawtime);
								localtime_s(&timeinfo, &rawtime);
								strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", &timeinfo);
								strdate = std::string(date_buffer);

								converted_wmtext.append(strdate);
								has_premacro = true;
							}
							else if (s2 == "$(time)")
							{
								std::string strtime;
								time_t rawtime;
								struct tm timeinfo;
								char time_buffer[256] = { 0 };
								time(&rawtime);
								localtime_s(&timeinfo, &rawtime);
								strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &timeinfo);
								strtime = std::string(time_buffer);

								converted_wmtext.append(strtime);
								has_premacro = true;
							}
							else if (s2 == "$(break)")
							{
								converted_wmtext.append("\n");
								has_premacro = true;
							}

							if (!has_premacro)
							{
								converted_wmtext.append(s1);
							}
						}
						else
						{
							// wrong format
							converted_wmtext.push_back(wmtext.data()[i]);
						}
						begin = -1;
						end = -1;
					}
					else
					{
						if (begin < 0)
						{
							converted_wmtext.push_back(wmtext.data()[i]);
						}
					}
				}

#pragma endregion

				watermarkInfo.text = converted_wmtext;
			}

			if (obj_value.as_object().has_field(L"fontname"))
			{
				watermarkInfo.fontName = NX::conversion::utf16_to_utf8(obj_value.as_object().at(L"fontname").as_string());
			}

			if (obj_value.as_object().has_field(L"fontcolor"))
			{
				watermarkInfo.fontColor = NX::conversion::utf16_to_utf8(obj_value.as_object().at(L"fontcolor").as_string());
			}

			if (obj_value.as_object().has_field(L"fontsize"))
			{
				watermarkInfo.fontSize = obj_value.as_object().at(L"fontsize").as_int32();
			}

			if (obj_value.as_object().has_field(L"transparency"))
			{
				watermarkInfo.transparency = obj_value.as_object().at(L"transparency").as_int32();
			}

			if (obj_value.as_object().has_field(L"rotation"))
			{
				watermarkInfo.rotation = (WATERMARK_ROTATION)obj_value.as_object().at(L"rotation").as_uint32();
			}

			if (obj_value.as_object().has_field(L"repeat"))
			{
				watermarkInfo.repeat = obj_value.as_object().at(L"repeat").as_boolean();
			}

			vecWatermarkInfo.push_back(watermarkInfo);
		}

		unsigned long long police_rights = rights;
		if (police_rights & RIGHT_VIEW)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_VIEW, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_EDIT)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_EDIT, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_PRINT)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_PRINT, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_CLIPBOARD)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_CLIPBOARD, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_SAVEAS)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_SAVEAS, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_DECRYPT)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_DECRYPT, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_SCREENCAPTURE)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_SCREENCAPTURE, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_SEND)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_SEND, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_CLASSIFY)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_CLASSIFY, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_SHARE)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_SHARE, vecWatermarkInfo));
		}

		if (police_rights & RIGHT_DOWNLOAD)
		{
			rightsAndWatermarks.push_back(std::make_pair(RIGHT_DOWNLOAD, vecWatermarkInfo));
		}
	}
}

std::wstring rmsession::get_creo_xtop_apppath()
{
	Sleep(5000);
	NX::win::reg_key key;
	std::wstring strKey = L"SOFTWARE\\NextLabs\\CreoRMX";
	if (!key.exist(HKEY_LOCAL_MACHINE, strKey))
	{
		LOGINFO(NX::string_formater(L"rmsession::get_creo_xtop_apppath SOFTWARE\\NextLabs\\CreoRMX not exist!"));
		return L"";
	}

	std::wstring strCoreCommonDir;
	NX::win::reg_key::reg_position pos = NX::win::reg_key::reg_position::reg_default;
	key.open(HKEY_LOCAL_MACHINE, strKey, pos, true);
	try
	{
		key.read_value(L"CreoCommonDir", strCoreCommonDir);
	}
	catch (const std::exception&)
	{
		LOGINFO(NX::string_formater(L"rmsession::get_creo_xtop_apppath SOFTWARE\\NextLabs\\CreoRMX\\CreoCommonDir value not exist!"));
		key.close();
		return L"";
	}

	key.close();

	if (strCoreCommonDir.empty())
	{
		LOGINFO(NX::string_formater(L"rmsession::get_creo_xtop_apppath SOFTWARE\\NextLabs\\CreoRMX\\CreoCommonDir value is empty!"));
		return L"";
	}

	std::wstring strAppPath;
	std::size_t stPos = strCoreCommonDir.find_last_of(L"\\");
	if (stPos != strCoreCommonDir.length() - 1) // means Common folder path don't have last slash 
	{
		strAppPath = strCoreCommonDir + L"\\x86e_win64\\obj\\xtop.exe";
	}
	else 
	{
		strAppPath = strCoreCommonDir + L"x86e_win64\\obj\\xtop.exe";
	}
	LOGINFO(NX::string_formater(L"rmsession::get_creo_xtop_apppath xtop.exe path is %s", strAppPath.c_str()));

	return strAppPath;
}


void rmsession::pdp_app_preload(rmsession* pSession)
{
	LOGINFO(NX::string_formater(L"begin rmsession::pdp_app_preload pSession : %p", pSession));
	if (NULL == pSession)
		return;

	std::wstring appPath = pSession->get_creo_xtop_apppath();
	std::wstring resourceName = appPath;
	if (appPath.empty() || resourceName.empty())
	{
		LOGINFO(NX::string_formater(L"end rmsession::pdp_app_preload apppath or sourcefilepath is empty!"));
		return;
	}

	LOGINFO(NX::string_formater(L"rmsession::pdp_app_preload apppath:%s, resourceName:%s", appPath.c_str(), resourceName.c_str()));

	std::wstring name = pSession->get_profile().get_name();
	std::wstring email = pSession->get_profile().get_email();

	std::wstring resourceType = L"fso";

	std::vector<std::pair<std::wstring, std::wstring>> attrs;
	std::vector<std::pair<std::wstring, std::wstring>> userAttrs = pSession->get_profile().get_userAttributes();

	std::wstring bundle = L"";
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsAndWatermarks;

	bool bRet = SERV->PDPEvalRights(name, email, email, appPath, resourceName, resourceType, attrs, userAttrs, bundle, &rightsAndWatermarks, nullptr);

	LOGINFO(NX::string_formater(L"end rmsession::pdp_app_preload pSession : %d", bRet));
}

unsigned long rmsession::request_delete_file_token(const std::wstring& file)
{
	LOGDEBUG(NX::string_formater(L"request_delete_file_token: file: %s", file.c_str()));

	HANDLE h = ::CreateFileW(NX::fs::dos_fullfilepath(file).global_dos_path().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == h) {
		LOGWARNING(NX::string_formater(L"Eval: fail to open target file (%d): %s", GetLastError(), file.c_str()));
		return ERROR_NOT_FOUND;
	}
	// load NXL file context
	NX::NXL::document_context context;
	if (!context.load(h))
	{
		CloseHandle(h);
		return ERROR_INVALID_DATA;
	}

	std::vector<unsigned char> doc_duid = context.get_duid();
	unsigned long  level = context.get_token_level();
	_token_cache.delete_token(doc_duid, level);
	_token_cache_dirty = true;

	CloseHandle(h);
	return 0;
}

unsigned int rmsession::is_nxl_file(const std::wstring& file)
{
	LOGDEBUG(NX::string_formater(L"is_nxl_file: file: %s", file.c_str()));

	NX::win::session_token st(_winsession->get_session_id());
	NX::win::impersonate_object impersonobj(st);

	HANDLE h = ::CreateFileW(NX::fs::dos_fullfilepath(file).global_dos_path().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == h) {
		LOGWARNING(NX::string_formater(L"Eval: fail to open target file (%d): %s", GetLastError(), file.c_str()));
		return ERROR_NOT_FOUND;
	}
	// load NXL file context
	NX::NXL::document_context context;
	if (!context.load(h))
	{
		CloseHandle(h);
		return ERROR_INVALID_DATA;
	}

	std::vector<unsigned char> doc_duid = context.get_duid();
	unsigned long  level = context.get_token_level();
	const std::wstring& ownerid = context.get_owner_id();

	if (ownerid.empty())
	{
		CloseHandle(h);
		return ERROR_INVALID_DATA;
	}

	CloseHandle(h);
	return 0;
}

bool rmsession::rights_evaluate(const std::wstring& file, unsigned long process_id, unsigned long session_id, _Out_ unsigned __int64* evaluate_id, _Out_ unsigned __int64* rights, _Out_opt_ unsigned __int64* custom_rights, bool bcheckowner)
{
    bool result = false;
    bool is_owner = false;
    std::shared_ptr<policy_bundle> adhoc_policy;
    std::shared_ptr<eval_result> final_result(new eval_result());
    activity_record actrecord;
    LARGE_INTEGER li_eval_id;

    // information
    std::vector<unsigned char> doc_duid;
    std::wstring str_doc_duid;
    std::wstring doc_owner_id;

    static const std::wstring winapp_rundll32(NX::win::get_system_directory() + L"\\rundll32.exe");
    static const std::wstring winapp_dllhost(NX::win::get_system_directory() + L"\\DllHost.exe");
    static const std::wstring winapp_sihost(NX::win::get_system_directory() + L"\\sihost.exe");
    static const std::wstring winapp_runtimebroker(NX::win::get_system_directory() + L"\\runtimebroker.exe");
    static const std::wstring winapp_mspaint(NX::win::get_system_directory() + L"\\mspaint.exe");

	HANDLE h = INVALID_HANDLE_VALUE;

    // ignore application condition?
    bool ignore_app_condition = false;
    // In RMDSDK RPM, we always ignore app association, and always allow
    // unassiciated app to open NXL files.  So we use a default value of
    // "true" here.
    bool ignore_app_association = true;

    // Init
	_ad_hoc = true;
    *evaluate_id = 0;
    *rights = 0;
    li_eval_id.QuadPart = final_result->get_id();
    if (custom_rights != nullptr) {
        *custom_rights = 0;
    }

    if (0 == process_id || 4 == process_id) {
        return false;
    }

	NX::win::session_token st(session_id);
	if (st.empty()) {
		LOGERROR(NX::string_formater(L"Fail to get session (%d) token, error = %d", session_id, GetLastError()));
		return false;
	}

	const std::wstring winuser_sid = st.get_user().id();

    
    // Get process information
    const process_record& proc_record = GLOBAL.safe_find_process(process_id);
    if (proc_record.empty()) {
        return false;
    }

    const std::wstring image_path(proc_record.get_image_path());
    const wchar_t* image_name = wcsrchr(image_path.c_str(), L'\\');
    image_name = (NULL == image_name) ? image_path.c_str() : (image_name + 1);

    if (boost::algorithm::iends_with(image_path, L"\\iexplore.exe") || boost::algorithm::iends_with(image_path, L"\\chrome.exe")) {
        // Internet explorer, never allow it
        LOGDETAIL(L"Prevent Browser from opening NXL file");
        LOGDETAIL(NX::string_formater(L"    - App: %s", image_name));
        LOGDETAIL(NX::string_formater(L"    - File: %s", file.c_str()));
        return false;
    }

    if (boost::algorithm::iends_with(image_path, L"\\explorer.exe")) {
        // Windows explorer
        ignore_app_condition = true;
        ignore_app_association = true;
    }

    // Check associated application
    std::wstring assocated_app;
	const bool is_from_outlook = boost::algorithm::icontains(file, L"content.outlook");
	NX::win::file_association fassoc(file, winuser_sid);
    assocated_app = fassoc.get_executable();

    //
    //  Handle special case for application check
	//  NOTE: bypass this special case checking if the file is in outlook temporary folder.
    //
    if (!ignore_app_condition && !ignore_app_association && !is_from_outlook) { 

        const bool is_image_file = boost::algorithm::istarts_with(fassoc.get_content_type(), L"image");

        if (assocated_app.empty()
            || is_image_file
            || boost::algorithm::iends_with(assocated_app, L".dll")
            || 0 == _wcsicmp(winapp_rundll32.c_str(), assocated_app.c_str())
            || 0 == _wcsicmp(winapp_dllhost.c_str(), assocated_app.c_str())
            || 0 == _wcsicmp(winapp_sihost.c_str(), assocated_app.c_str())
            || 0 == _wcsicmp(winapp_runtimebroker.c_str(), assocated_app.c_str())
            || 0 == _wcsicmp(winapp_mspaint.c_str(), assocated_app.c_str())) {

            if (0 == _wcsicmp(winapp_rundll32.c_str(), image_path.c_str())
                || 0 == _wcsicmp(winapp_dllhost.c_str(), image_path.c_str())
                || 0 == _wcsicmp(winapp_sihost.c_str(), image_path.c_str())
                || 0 == _wcsicmp(winapp_runtimebroker.c_str(), image_path.c_str())
                ) {

				// ignore app
				ignore_app_association = true;
				LOGDETAIL(NX::string_formater(L"Ignore association check for rundll32.exe / dllhost.exe / sihost.exe / runtimebroker.exe -> %s", image_path.c_str()));
            }
        }

        if (boost::algorithm::iends_with(file, L".xml") && (boost::algorithm::iends_with(image_path, L"\\iexplore.exe") || boost::algorithm::iends_with(image_path, L"\\chrome.exe"))) {
            // try to get its command line
            std::wstring browser_cmd = get_process_commandline(process_id);
            if (!browser_cmd.empty()) {
                int argc = 0;
                LPWSTR* argv = CommandLineToArgvW(browser_cmd.c_str(), &argc);
                if (argc == 2 && 0 == _wcsicmp(file.c_str(), argv[1])) {
                    ignore_app_association = true;
                    LOGDETAIL(NX::string_formater(L"Open file using browser: ", file.c_str()));
                }
            }
        }
    }
    
    if (!is_logged_on()) {
        // send notification to ask user logon
        if (!ignore_app_condition) {
            get_app_manager().send_logon_notification();
        }
        return false;
    }


    if ((!ignore_app_association && !assocated_app.empty() && 0 != _wcsicmp(assocated_app.c_str(), proc_record.get_image_path().c_str())) ||
		(!ignore_app_association && assocated_app.empty())) {
        LOGDEBUG(L"Prevent unassociated application from opening NXL file:");
        LOGDEBUG(NX::string_formater(L"  -> File: %s", file.c_str()));
        LOGDEBUG(NX::string_formater(L"  -> Associated App: %s", assocated_app.c_str()));
        LOGDEBUG(NX::string_formater(L"  -> Open App: %s", proc_record.get_image_path().c_str()));
        return false;
    }

	//
	// added by Philip Qi
	// in a inefficiency way, impersonate when 1) opening file 2) read content
	//
	{
		NX::win::session_token st(session_id);
		NX::win::impersonate_object impersonobj(st);

		// Fix the bug that can't view for long file path.
		std::wstring unlimitedFile = NX::fs::dos_fullfilepath(file).global_dos_path();

		h = ::CreateFileW(unlimitedFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		LOGDEBUG(NX::string_formater(L"rights_evaluate: file: %s", unlimitedFile.c_str()));
		if (INVALID_HANDLE_VALUE == h) {
			LOGWARNING(NX::string_formater(L"Eval: fail to open target file (%d): %s", GetLastError(), unlimitedFile.c_str()));
			return false;
		}
	}

    std::vector<unsigned char> file_token;
    eval_data ed((ignore_app_condition || ignore_app_association) ? EVAL_FLAG_IGNORE_APP : EVAL_FLAG_ALL);
    bool token_access_denied = false;
	NX::NXL::document_context context;
	unsigned __int64 enddate = 0;
	unsigned __int64 startdate;

    // Get file information
    do {


		//
		// added by Philip Qi
		// in a inefficiency way, impersonate when 1) opening file 2) read content
		//
		{
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);

			// load NXL file context
			if (!context.load(h)) {
				break;
			}
		}

		//tags = context.get_tags();
        doc_duid = context.get_duid();
        str_doc_duid = NX::conversion::to_wstring(context.get_duid());
        doc_owner_id = context.get_owner_id();

        const std::wstring& ownerid = context.get_owner_id();

        if (ownerid.empty()) {
            break;
        }

        // get token
        const std::wstring doc_policy = NX::conversion::utf8_to_utf16(context.get_policy());
        const std::wstring doc_tags = NX::conversion::utf8_to_utf16(context.get_tags());
        int protection_type = (doc_policy.empty() || doc_policy == L"{}") ? 1 : 0;   // 1 = central, 0 = ad-hoc
        bool token_from_cache = false;
        if (!query_token(process_id, ownerid, context.get_agreement0(), context.get_duid(), protection_type, doc_policy, doc_tags, context.get_token_level(), file_token, &token_from_cache, &token_access_denied)) {

            LOGWARNING(NX::string_formater(L"CheckRights: process (%d, %s) fail to get decrypt token for file (%s)", process_id, image_name, file.c_str()));

            // fail to get token
            break;
        }
        if (file_token.size() != 32) {
            const std::wstring& str_token = NX::conversion::to_wstring(file_token);
            LOGWARNING(NX::string_formater(L"CheckRights: wrong decrypt token for file (%s): ", file.c_str(), str_token.c_str()));
            break;
        }

#ifdef _DEBUG
        const std::wstring& str_token_id = NX::conversion::to_wstring(context.get_duid());
        const std::wstring& str_token = NX::conversion::to_wstring(file_token);
        const wchar_t* from_where = token_from_cache ? L"cache" : L"server";
        LOGDETAIL(NX::string_formater(L"CheckRights: get decrypt token from %s:", from_where));
        LOGDETAIL(NX::string_formater(L"    File:    %s", file.c_str()));
        LOGDETAIL(NX::string_formater(L"    TokenId: %s", str_token_id.c_str()));
        LOGDETAIL(NX::string_formater(L"    Token:   %s", str_token.c_str()));
        LOGDETAIL(NX::string_formater(L"    TokenLevel: %d", context.get_token_level()));
#endif // _DEBUG


        // validate checksum
        if (!context.validate_checksum(file_token.data())) {
            LOGWARNING(NX::string_formater(L"CheckRights: file has a mismatch checksum (%s)", file.c_str()));
            break;
        }

		if (!get_document_eval_info(h, context, file_token.data(), ed, adhoc_policy))
			LOGERROR(NX::string_formater(L"get_document_eval_info: failed: %s", file.c_str()));
		
		if (adhoc_policy != nullptr)
		{
			*rights = adhoc_policy->get_rights();
			enddate = adhoc_policy->get_enddate();
			startdate = adhoc_policy->get_startdate();
		}
		
		LOGDEBUG(NX::string_formater(L" rights_evaluate: adhoc rights (%d), enddate (%llu)", *rights, enddate));

        // Is current user owner?
        if (get_profile().is_me(ownerid))
		{
            // Current user is owner
			if (bcheckowner)
			{
				if (0 == _wcsicmp(ownerid.c_str(), get_profile().get_preferences().get_default_member().c_str())) // only for myvaule, owner is valid
				{
					final_result->add_rights(BUILTIN_RIGHT_ALL);
					if (*rights > 0)  // adhoc only
						*rights = BUILTIN_RIGHT_ALL;
					is_owner = true;
				}
			}
			*evaluate_id = final_result->get_id();

            LOGINFO1(L" ");
            LOGINFO1(L"[Rights Evaluation]");
            LOGINFO1(NX::string_formater(L"  -> Id: %08X%08X", li_eval_id.HighPart, li_eval_id.LowPart));
            LOGINFO1(NX::string_formater(L"  -> User: %s (email: %s, membership: %s)", get_profile().get_name().c_str(), get_profile().get_email().c_str(), context.get_owner_id().c_str()));
            LOGINFO1(NX::string_formater(L"  -> Application%s: %s", ignore_app_condition ? L" (Not Evaluate)" : L"", proc_record.get_image_path().c_str()));
            LOGINFO1(NX::string_formater(L"  -> File: %s", file.c_str()));
            LOGINFO1(NX::string_formater(L"      (Duid: %s, Owner: %s)", str_doc_duid.c_str(), context.get_owner_id().c_str()));
			if (*rights > 0)
				LOGINFO1(L"  -> Grant Rights: Full control (Owner)");
            LOGINFO1(L" ");
            result = true;
            actrecord = activity_record(context.get_duid(), context.get_owner_id(), get_profile().get_id(), ActView, ActAllowed, file, image_path, proc_record.get_pe_file_info()->get_image_publisher(), std::wstring());
            break;
        }


		if (!is_owner && (enddate > 0))
		{
			unsigned __int64 currtime = (uint64_t)std::time(nullptr) * 1000;
			if (currtime > enddate)
			{
				LOGDEBUG(NX::string_formater(L" rights_evaluate: file expired curr: %llu, end: %llu", currtime, enddate));
				return false;
			}
			if (startdate > currtime)
			{
				LOGDEBUG(NX::string_formater(L" rights_evaluate: start date > curr: %llu, end: %llu", currtime, startdate));
				return false;
			}
		}

        result = true;

    } while (FALSE);
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;

    if (!result) {

        if (token_access_denied) {

            if (!ignore_app_condition) {

                log_activity(activity_record(doc_duid,
                                             doc_owner_id,
                                             get_profile().get_id(),
                                             ActView,
                                             ActDenied,
                                             file,
                                             image_path,
                                             proc_record.get_pe_file_info()->get_image_publisher(),
                                             std::wstring()));

                audit_activity(ActView, ActDenied, str_doc_duid, doc_owner_id, image_name, file);

                //fix bug 58062: not file owner to view central policy file for project and system bucket will pop up a error message
                //block_notify(file);
            }
        }

        *evaluate_id = final_result->get_id();
        *rights = 0;

        return token_access_denied;
    }

	//
	// Now removed the conditional judgement of 'is_owner', fix bug 59214
	//
    //if (is_owner) 
	{

        if (!ignore_app_condition) {

            GLOBAL.get_process_cache().set_process_flags(process_id, NXRM_PROCESS_FLAG_WITH_NXL_OPENED);

            log_activity(activity_record(doc_duid,
                                         doc_owner_id,
                                         get_profile().get_id(),
                                         ActView,
                                         ActAllowed,
                                         file,
                                         image_path,
                                         proc_record.get_pe_file_info()->get_image_publisher(),
                                         std::wstring()));

            audit_activity(ActView, ActAllowed, str_doc_duid, doc_owner_id, image_name, file);
        }

        // Don't cache evaluation result because there is no real evaluation
		LOGDEBUG(NX::string_formater(L"ignore_app_condition"));
        //return true;
    }


    ed.push_data(L"user.id", NX::generic_value(get_profile().get_id()));
    ed.push_data(L"user.name", NX::generic_value(get_profile().get_name()));
    ed.push_data(L"user.email", NX::generic_value(get_profile().get_email()));
    ed.push_data(L"application.path", NX::generic_value(proc_record.get_image_path()));
    ed.push_data(L"application.is_associated_app", NX::generic_value((0 == _wcsicmp(assocated_app.c_str(), proc_record.get_image_path().c_str()) ? true : false)));
    std::shared_ptr<NX::win::pe_file> peinfo = proc_record.get_pe_file_info();
    if (peinfo != nullptr) {
        ed.push_data(L"application.signature.publisher", NX::generic_value(peinfo->get_image_publisher()));
        ed.push_data(L"application.name", NX::generic_value(peinfo->get_file_version().get_file_name()));
        ed.push_data(L"application.version", NX::generic_value(peinfo->get_file_version().get_file_version_string()));
        ed.push_data(L"application.company", NX::generic_value(peinfo->get_file_version().get_company_name()));
        ed.push_data(L"application.product", NX::generic_value(peinfo->get_file_version().get_product_name()));
        ed.push_data(L"application.product.version", NX::generic_value(peinfo->get_file_version().get_product_version_string()));
    }
    ed.push_data(L"host.name", NX::generic_value(GLOBAL.get_host_name().c_str()));
    ed.push_data(L"resource.path", NX::generic_value(file.c_str()));

	SYSTEMTIME tTime = { 0 };
	GetSystemTime(&tTime);
	FILETIME fTime = { 0 };
	SystemTimeToFileTime(&tTime, &fTime);

	ULARGE_INTEGER ui;
	ui.LowPart = fTime.dwLowDateTime;
	ui.HighPart = fTime.dwHighDateTime;
	ed.push_data(L"environment.date", NX::generic_value(((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000)));

	if (adhoc_policy != nullptr && !adhoc_policy->empty()) 
		adhoc_policy->evaluate(ed, final_result.get());

	unsigned __int64 right = final_result->get_rights();
	std::string tags = context.get_tags();
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsAndWatermarks;
	if (*rights == 0 )
	{
		// call JPC
		std::wstring name = get_profile().get_name();
		std::wstring email = get_profile().get_email();
		
		std::vector<std::pair<std::wstring, std::wstring>> attrs = ImportJsonTags(tags);
		std::string bundle = "";
		std::string domain_bundle;

		std::wstring tenantName = NX::conversion::utf8_to_utf16(context.get_tenant());
		LOGDEBUG(NX::string_formater(L"central policy,  tenantName (%s)", tenantName.c_str()));

		/*if (tenantName.size() > 0)
		{
			bool bRet = get_policybundle(tenantName, bundle);
			if (!bRet) {
				LOGERROR(NX::string_formater(L"GetPolicyBundle failed: tenantName (%s)", tenantName.c_str()));
			}
		}
		else
			LOGERROR("Cannot find tenant name");

		// handle domain policy, use user default tenant
		std::wstring  defaultTenant = SERV->get_router_config().get_tenant_id();
		if ((defaultTenant.size() > 0) && (defaultTenant.compare(tenantName) != 0))
		{
			LOGDEBUG(NX::string_formater(L" central policy:  default tenant (%s)", defaultTenant.c_str()));

			bool bRet = get_policybundle(defaultTenant, domain_bundle);
			if (!bRet) {
				LOGERROR(NX::string_formater(L"GetPolicyBundle failed: default tenant (%s)", defaultTenant.c_str()));
			}
			else
			{
				bundle += domain_bundle;
			}
		}*/

		// find user id
		std::wstring userIdpid = L"";
		std::vector<std::pair<std::wstring, std::wstring>> attributes = get_profile().get_userAttributes();
		for (auto it = attributes.begin(); attributes.end() != it; it++)
		{
			if ((*it).first.compare(L"idp_unique_id") == 0)
			{
				userIdpid = (*it).second;
			}
		}
		std::wstring userid = L"";
		for (auto it = attributes.begin(); attributes.end() != it; it++)
		{
			if ((*it).first.compare(userIdpid) == 0)
			{
				userid = (*it).second;
				break;
			}
		}
		// if user id not exist, use email
		if (userid == L"")
		{
			userid = email;
		}

		// get file token group resource type
		bool filebRet = false;
		std::string tokenGroupResourcetype;
		if (tenantName.size() > 0)
		{
			filebRet = get_tokengroup_resourcetype(tenantName, tokenGroupResourcetype);
			if (!filebRet) {
				LOGERROR(NX::string_formater(L"GetResourceType failed: tenantName (%s)", tenantName.c_str()));
			}
		}
		else
			LOGERROR("Cannot find tenant name");

		// get default tenant resource type
		bool tenantbRet = false;
		std::string tenantResourcetype;
		std::wstring  defaultTenant = SERV->get_router_config().get_tenant_id();
		if ((defaultTenant.size() > 0) && (defaultTenant.compare(tenantName) != 0))
		{
			LOGDEBUG(NX::string_formater(L" central policy:  default tenant (%s)", defaultTenant.c_str()));

			tenantbRet = get_tokengroup_resourcetype(defaultTenant, tenantResourcetype);
			if (!tenantbRet) {
				LOGERROR(NX::string_formater(L"GetResourceType failed: default tenant (%s)", defaultTenant.c_str()));
			}
		}

		if (!filebRet && !tenantbRet)
		{
			return false;
		}

		bool ready = false;
		SERV->ensurePDPConnectionReady(ready);
		if (!ready) {
			LOGDEBUG(NX::string_formater(L"*** cepdpman is not ready to eval rights"));
			return false;
		}

		// file token group resource type do evaluate right
		bool fileResult = false;
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> fileRightsAndWatermarks;
		if (filebRet)
		{
			fileResult = SERV->EvalRights(process_id, name, email, userid, image_path, file, NX::conversion::utf8_to_utf16(tokenGroupResourcetype), attrs, get_profile().get_userAttributes(), NX::conversion::utf8_to_utf16(bundle), &fileRightsAndWatermarks, nullptr, bcheckowner);
		}

		// default tenant resource type do evaluate right
		bool tenantResult = false;
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> tenantRightsAndWatermarks;
		if (tenantbRet)
		{
			tenantResult = SERV->EvalRights(process_id, name, email, userid, image_path, file, NX::conversion::utf8_to_utf16(tenantResourcetype), attrs, get_profile().get_userAttributes(), NX::conversion::utf8_to_utf16(bundle), &tenantRightsAndWatermarks, nullptr, bcheckowner);
		}

		// Combine rights and remove duplicates
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsCopy;
		for (size_t i = 0; i < fileRightsAndWatermarks.size(); i++)
		{
			rightsCopy.push_back(fileRightsAndWatermarks[i]);
			rightsAndWatermarks.push_back(fileRightsAndWatermarks[i]);
		}
		for (size_t i = 0; i < tenantRightsAndWatermarks.size(); i++)
		{
			bool rightHasExist = false;
			for (size_t k = 0; k < rightsCopy.size(); k++)
			{
				if (tenantRightsAndWatermarks[i].first == rightsCopy[k].first)
				{
					rightHasExist = true;
					break;
				}
			}
			if (!rightHasExist)
			{
				rightsAndWatermarks.push_back(tenantRightsAndWatermarks[i]);
			}
		}

		for (const auto& rightAndWatermarks : rightsAndWatermarks) {
			*rights |= rightAndWatermarks.first;
		}

		if (fileResult || tenantResult)
		{
			result = true;
		}

		_ad_hoc = false;
		LOGDEBUG(NX::string_formater(L" rights_evaluate: central policy,  rights (%d)",  *rights));
	}

    /*std::shared_ptr<policy_bundle> central_policy = get_policy_bundle();
    if ((central_policy != nullptr && !central_policy->empty())) {
        central_policy->evaluate(ed, final_result.get());
    } */

    // set result
    *evaluate_id = final_result->get_id();
    //*rights = final_result->get_rights();
    if (!ignore_app_condition) {
        _eval_cache.push(final_result);
        // set NXRM_PROCESS_FLAG_WITH_NXL_OPENED flag
        if (0 != final_result->get_rights()) {
            GLOBAL.get_process_cache().set_process_flags(process_id, NXRM_PROCESS_FLAG_WITH_NXL_OPENED);
            // Set Overlay Flag if there is watermark obligation
            if (final_result->has_obligation(L"WATERMARK")) {
                GLOBAL.get_process_cache().set_process_flags(process_id, NXRM_PROCESS_FLAG_WITH_OVERLAY_OBLIGATION);
                SERV->get_coreserv().drvctl_set_overlay_windows2(session_id);
            }
        }
        else {
			// handle central policy

            // Notify user if it is blocked

            //Fix bug 58062
            //block_notify(file);
        }
    }


    if (!ignore_app_condition) {

        log_activity(activity_record(doc_duid,
                                     doc_owner_id,
                                     get_profile().get_id(),
                                     ActView,
                                     (0 == *rights) ? ActDenied : ActAllowed,
                                     file,
                                     image_path,
                                     proc_record.get_pe_file_info()->get_image_publisher(),
                                     std::wstring()));

        audit_activity(ActView, (0 == *rights) ? ActDenied : ActAllowed, str_doc_duid, doc_owner_id, image_name, file);
    }


    const std::wstring& granted_rights_str = final_result->get_rights_string();
    LOGINFO1(L" ");
    LOGINFO1(L"[Rights Evaluation]");
    LOGINFO1(NX::string_formater(L"  -> Id: %08X%08X", li_eval_id.HighPart, li_eval_id.LowPart));
    LOGDEBUG(NX::string_formater(L"  -> User: %s (email: %s)", get_profile().get_name().c_str(), get_profile().get_email().c_str()));
    LOGINFO1(NX::string_formater(L"  -> File: %s", file.c_str()));
    LOGINFO1(NX::string_formater(L"      (Duid: %s, Owner: %s)", str_doc_duid.c_str(), doc_owner_id.c_str()));
    LOGINFO1(NX::string_formater(L"  -> Application%s: %s", ignore_app_condition ? L" (Not Evaluate)" : L"", proc_record.get_image_path().c_str()));
    //LOGINFO1(NX::string_formater(L"  -> Grant Rights: %s", granted_rights_str.c_str()));
	LOGINFO1(NX::string_formater(L"  -> Grant Rights: %d", *rights));
	

    if (NX::dbg::LL_DEBUG <= NxGlobalLog.get_accepted_level()) {

        std::vector<std::pair<std::wstring, std::wstring>> user_parameters;
        std::vector<std::pair<std::wstring, std::wstring>> app_parameters;
        std::vector<std::pair<std::wstring, std::wstring>> host_parameters;
        std::vector<std::pair<std::wstring, std::wstring>> resources_parameters;
        std::vector<std::pair<std::wstring, std::wstring>> env_parameters;
        std::vector<std::pair<std::wstring, std::wstring>> other_parameters;

        std::for_each(ed.get_values().begin(), ed.get_values().end(), [&](const std::pair<std::wstring, NX::generic_value>& item) {
            if (boost::algorithm::istarts_with(item.first, L"user.")) {
                user_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
            }
            else if (boost::algorithm::istarts_with(item.first, L"application.")) {
                app_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
            }
            else if (boost::algorithm::istarts_with(item.first, L"host.")) {
                host_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
            }
            else if (boost::algorithm::istarts_with(item.first, L"resource.")) {
                resources_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
            }
            else if (boost::algorithm::istarts_with(item.first, L"environment.")) {
                env_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
            }
            else {
                other_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
            }
        });

        LOGDEBUG(L"  Details:");
        LOGDEBUG(L"    <User>");
        std::for_each(user_parameters.begin(), user_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
            LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
        });
        LOGDEBUG(L"    <Application>");
        std::for_each(app_parameters.begin(), app_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
            LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
        });
        LOGDEBUG(L"    <Host>");
        std::for_each(host_parameters.begin(), host_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
            LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
        });
        LOGDEBUG(L"    <Resource>");
        std::for_each(resources_parameters.begin(), resources_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
            LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
        });
        LOGDEBUG(L"    <Environment>");
        std::for_each(env_parameters.begin(), env_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
            LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
        });
        if (!other_parameters.empty()) {
            LOGDEBUG(L"    <Others>");
            std::for_each(other_parameters.begin(), other_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
                LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
            });
        }
        LOGDEBUG(L"  Obligations:");
        std::for_each(final_result->get_obligations().begin(), final_result->get_obligations().end(), [&](const std::pair<std::wstring, std::shared_ptr<obligation>>& ob) {
            LOGDEBUG(NX::string_formater(L"      - %s", ob.first.c_str()));
        });
        LOGINFO1(L" ");
    }

    return true;
}

// Note: when merge the rights, we should also consider the rights associated watermark.
void MergeRightsWatermark(const std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& fileTokenGroupRightsWatermark,
	const std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& tenantTokenGroupRightsWatermark,
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& result) {

	for (auto& outer : fileTokenGroupRightsWatermark) {

		bool bFind = false;
		std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> findInner;
		for (auto& inner : tenantTokenGroupRightsWatermark) {
			if (outer.first == inner.first) {
				bFind = true;
				findInner = inner;
				break;
			}
		}

		if (!bFind) {
			result.push_back(outer);
		}
		else {
			// If file token group rights no watermark but tenant token group have watermark, should use the latter.
			if (outer.second.size() == 0 && findInner.second.size() > 0) {
				result.push_back(findInner);
			}
			else {
				result.push_back(outer);
			}
		}

	}

	// Add the item that included in "tenantTokenGroupRightsWatermark" but excluded in "fileTokenGroupRightsWatermark"
	for (auto& one : tenantTokenGroupRightsWatermark) {
		bool bFind = false;
		for (auto& two : fileTokenGroupRightsWatermark) {
			if (one.first == two.first) {
				bFind = true;
				break;
			}
		}

		if (!bFind) {
			result.push_back(one);
		}
	}
}

bool rmsession::rights_evaluate_with_watermark(const std::wstring& file, unsigned long process_id, unsigned long session_id,
	_Out_ unsigned __int64* evaluate_id, _Out_ unsigned __int64* rights, _Out_opt_ unsigned __int64* custom_rights,
	_Out_ std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& vecRightsWatermarks, bool bcheckowner)
{
	bool result = false;
	bool is_owner = false;
	std::shared_ptr<policy_bundle> adhoc_policy;
	std::shared_ptr<eval_result> final_result(new eval_result());
	activity_record actrecord;
	LARGE_INTEGER li_eval_id;

	// information
	std::vector<unsigned char> doc_duid;
	std::wstring str_doc_duid;
	std::wstring doc_owner_id;

	static const std::wstring winapp_rundll32(NX::win::get_system_directory() + L"\\rundll32.exe");
	static const std::wstring winapp_dllhost(NX::win::get_system_directory() + L"\\DllHost.exe");
	static const std::wstring winapp_sihost(NX::win::get_system_directory() + L"\\sihost.exe");
	static const std::wstring winapp_runtimebroker(NX::win::get_system_directory() + L"\\runtimebroker.exe");
	static const std::wstring winapp_mspaint(NX::win::get_system_directory() + L"\\mspaint.exe");

	HANDLE h = INVALID_HANDLE_VALUE;

	// ignore application condition?
	bool ignore_app_condition = false;
	// In RMDSDK RPM, we always ignore app association, and always allow
	// unassiciated app to open NXL files.  So we use a default value of
	// "true" here.
	bool ignore_app_association = true;

	// Init
	_ad_hoc = true;
	*evaluate_id = 0;
	*rights = 0;
	li_eval_id.QuadPart = final_result->get_id();
	if (custom_rights != nullptr) {
		*custom_rights = 0;
	}

	if (0 == process_id || 4 == process_id) {
		return false;
	}

	//
	// if current rmsession is running under service mode, we always return VIEW right if token is granted
	// 
	if (is_server_mode() == TRUE)
	{
		std::vector<unsigned char> file_token;
		bool token_access_denied = false;
		bool file_access_allowed = false;
		NX::NXL::document_context context;

		// Get file information
		do {
			std::wstring unlimitedFile = NX::fs::dos_fullfilepath(file).global_dos_path();

			h = ::CreateFileW(unlimitedFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			LOGDEBUG(NX::string_formater(L"rights_evaluate: file: %s", unlimitedFile.c_str()));
			if (INVALID_HANDLE_VALUE == h) {
				LOGWARNING(NX::string_formater(L"Eval: fail to open target file (%d): %s", GetLastError(), unlimitedFile.c_str()));
				return false;
			}

			// load NXL file context
			if (!context.load(h)) {
				break;
			}

			//tags = context.get_tags();
			doc_duid = context.get_duid();
			str_doc_duid = NX::conversion::to_wstring(context.get_duid());
			doc_owner_id = context.get_owner_id();

			const std::wstring& ownerid = context.get_owner_id();

			if (ownerid.empty()) {
				break;
			}

			// get token
			const std::wstring doc_policy = NX::conversion::utf8_to_utf16(context.get_policy());
			const std::wstring doc_tags = NX::conversion::utf8_to_utf16(context.get_tags());
			int protection_type = (doc_policy.empty() || doc_policy == L"{}") ? 1 : 0;   // 1 = central, 0 = ad-hoc
			bool token_from_cache = false;
			if (!query_token(process_id, ownerid, context.get_agreement0(), context.get_duid(), protection_type, doc_policy, doc_tags, context.get_token_level(), file_token, &token_from_cache, &token_access_denied)) {
				break;
			}
			if (file_token.size() != 32) {
				const std::wstring& str_token = NX::conversion::to_wstring(file_token);
				LOGWARNING(NX::string_formater(L"CheckRights: wrong decrypt token for file (%s): ", file.c_str(), str_token.c_str()));
				break;
			}

			// validate checksum
			if (!context.validate_checksum(file_token.data())) {
				LOGWARNING(NX::string_formater(L"CheckRights: file has a mismatch checksum (%s)", file.c_str()));
				break;
			}

			file_access_allowed = true;
			// token is granted for access, return VIEW right
			if (token_access_denied == false)
			{
				*rights = BUILTIN_RIGHT_VIEW;
			}

		} while (false);

		if (h != INVALID_HANDLE_VALUE)
		{
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
		return file_access_allowed;
	}

	NX::win::session_token st(session_id);
	if (st.empty()) {
		LOGERROR(NX::string_formater(L"Fail to get session (%d) token, error = %d", session_id, GetLastError()));
		return false;
	}

	const std::wstring winuser_sid = st.get_user().id();


	// Get process information
	const process_record& proc_record = GLOBAL.safe_find_process(process_id);
	if (proc_record.empty()) {
		return false;
	}

	const std::wstring image_path(proc_record.get_image_path());
	const wchar_t* image_name = wcsrchr(image_path.c_str(), L'\\');
	image_name = (NULL == image_name) ? image_path.c_str() : (image_name + 1);

	if (boost::algorithm::iends_with(image_path, L"\\iexplore.exe") || boost::algorithm::iends_with(image_path, L"\\chrome.exe")) {
		// Internet explorer, never allow it
		LOGDETAIL(L"Prevent Browser from opening NXL file");
		LOGDETAIL(NX::string_formater(L"    - App: %s", image_name));
		LOGDETAIL(NX::string_formater(L"    - File: %s", file.c_str()));
		return false;
	}

	if (boost::algorithm::iends_with(image_path, L"\\explorer.exe")) {
		// Windows explorer
		ignore_app_condition = true;
		ignore_app_association = true;
	}

	// Check associated application
	std::wstring assocated_app;
	const bool is_from_outlook = boost::algorithm::icontains(file, L"content.outlook");
	NX::win::file_association fassoc(file, winuser_sid);
	assocated_app = fassoc.get_executable();

	//
	//  Handle special case for application check
	//  NOTE: bypass this special case checking if the file is in outlook temporary folder.
	//
	if (!ignore_app_condition && !ignore_app_association && !is_from_outlook) {

		const bool is_image_file = boost::algorithm::istarts_with(fassoc.get_content_type(), L"image");

		if (assocated_app.empty()
			|| is_image_file
			|| boost::algorithm::iends_with(assocated_app, L".dll")
			|| 0 == _wcsicmp(winapp_rundll32.c_str(), assocated_app.c_str())
			|| 0 == _wcsicmp(winapp_dllhost.c_str(), assocated_app.c_str())
			|| 0 == _wcsicmp(winapp_sihost.c_str(), assocated_app.c_str())
			|| 0 == _wcsicmp(winapp_runtimebroker.c_str(), assocated_app.c_str())
			|| 0 == _wcsicmp(winapp_mspaint.c_str(), assocated_app.c_str())) {

			if (0 == _wcsicmp(winapp_rundll32.c_str(), image_path.c_str())
				|| 0 == _wcsicmp(winapp_dllhost.c_str(), image_path.c_str())
				|| 0 == _wcsicmp(winapp_sihost.c_str(), image_path.c_str())
				|| 0 == _wcsicmp(winapp_runtimebroker.c_str(), image_path.c_str())
				) {

				// ignore app
				ignore_app_association = true;
				LOGDETAIL(NX::string_formater(L"Ignore association check for rundll32.exe / dllhost.exe / sihost.exe / runtimebroker.exe -> %s", image_path.c_str()));
			}
		}

		if (boost::algorithm::iends_with(file, L".xml") && (boost::algorithm::iends_with(image_path, L"\\iexplore.exe") || boost::algorithm::iends_with(image_path, L"\\chrome.exe"))) {
			// try to get its command line
			std::wstring browser_cmd = get_process_commandline(process_id);
			if (!browser_cmd.empty()) {
				int argc = 0;
				LPWSTR* argv = CommandLineToArgvW(browser_cmd.c_str(), &argc);
				if (argc == 2 && 0 == _wcsicmp(file.c_str(), argv[1])) {
					ignore_app_association = true;
					LOGDETAIL(NX::string_formater(L"Open file using browser: ", file.c_str()));
				}
			}
		}
	}

	if (!is_logged_on()) {
		// send notification to ask user logon
		if (!ignore_app_condition) {
			get_app_manager().send_logon_notification();
		}
		return false;
	}

	if ((!ignore_app_association && !assocated_app.empty() && 0 != _wcsicmp(assocated_app.c_str(), proc_record.get_image_path().c_str())) ||
		(!ignore_app_association && assocated_app.empty())) {
		LOGDEBUG(L"Prevent unassociated application from opening NXL file:");
		LOGDEBUG(NX::string_formater(L"  -> File: %s", file.c_str()));
		LOGDEBUG(NX::string_formater(L"  -> Associated App: %s", assocated_app.c_str()));
		LOGDEBUG(NX::string_formater(L"  -> Open App: %s", proc_record.get_image_path().c_str()));
		return false;
	}

	//
	// added by Philip Qi
	// in a inefficiency way, impersonate when 1) opening file 2) read content
	//
	{
		NX::win::session_token st(session_id);
		NX::win::impersonate_object impersonobj(st);

		// Fix the bug that can't view for long file path.
		std::wstring unlimitedFile = NX::fs::dos_fullfilepath(file).global_dos_path();

		h = ::CreateFileW(unlimitedFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		LOGDEBUG(NX::string_formater(L"rights_evaluate: file: %s", unlimitedFile.c_str()));
		if (INVALID_HANDLE_VALUE == h) {
			LOGWARNING(NX::string_formater(L"Eval: fail to open target file (%d): %s", GetLastError(), unlimitedFile.c_str()));
			return false;
		}
	}

	std::vector<unsigned char> file_token;
	eval_data ed((ignore_app_condition || ignore_app_association) ? EVAL_FLAG_IGNORE_APP : EVAL_FLAG_ALL);
	bool token_access_denied = false;
	NX::NXL::document_context context;
	unsigned __int64 enddate = 0;
	unsigned __int64 startdate;

	// Get file information
	do {


		//
		// added by Philip Qi
		// in a inefficiency way, impersonate when 1) opening file 2) read content
		//
		{
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);

			// load NXL file context
			if (!context.load(h)) {
				break;
			}
		}

		//tags = context.get_tags();
		doc_duid = context.get_duid();
		str_doc_duid = NX::conversion::to_wstring(context.get_duid());
		doc_owner_id = context.get_owner_id();

		const std::wstring& ownerid = context.get_owner_id();

		if (ownerid.empty()) {
			break;
		}

		// get token
		const std::wstring doc_policy = NX::conversion::utf8_to_utf16(context.get_policy());
		const std::wstring doc_tags = NX::conversion::utf8_to_utf16(context.get_tags());
		int protection_type = (doc_policy.empty() || doc_policy == L"{}") ? 1 : 0;   // 1 = central, 0 = ad-hoc
		bool token_from_cache = false;
		if (!query_token(process_id, ownerid, context.get_agreement0(), context.get_duid(), protection_type, doc_policy, doc_tags, context.get_token_level(), file_token, &token_from_cache, &token_access_denied)) {

			LOGWARNING(NX::string_formater(L"CheckRights: process (%d, %s) fail to get decrypt token for file (%s)", process_id, image_name, file.c_str()));

			// fail to get token
			break;
		}
		if (file_token.size() != 32) {
			const std::wstring& str_token = NX::conversion::to_wstring(file_token);
			LOGWARNING(NX::string_formater(L"CheckRights: wrong decrypt token for file (%s): ", file.c_str(), str_token.c_str()));
			break;
		}

#ifdef _DEBUG
		const std::wstring& str_token_id = NX::conversion::to_wstring(context.get_duid());
		const std::wstring& str_token = NX::conversion::to_wstring(file_token);
		const wchar_t* from_where = token_from_cache ? L"cache" : L"server";
		LOGDETAIL(NX::string_formater(L"CheckRights: get decrypt token from %s:", from_where));
		LOGDETAIL(NX::string_formater(L"    File:    %s", file.c_str()));
		LOGDETAIL(NX::string_formater(L"    TokenId: %s", str_token_id.c_str()));
		LOGDETAIL(NX::string_formater(L"    Token:   %s", str_token.c_str()));
		LOGDETAIL(NX::string_formater(L"    TokenLevel: %d", context.get_token_level()));
#endif // _DEBUG


		// validate checksum
		if (!context.validate_checksum(file_token.data())) {
			LOGWARNING(NX::string_formater(L"CheckRights: file has a mismatch checksum (%s)", file.c_str()));
			break;
		}

		if (!get_document_eval_info(h, context, file_token.data(), ed, adhoc_policy))
			LOGERROR(NX::string_formater(L"get_document_eval_info: failed: %s", file.c_str()));

		if (adhoc_policy != nullptr)
		{
			*rights = adhoc_policy->get_rights();
			enddate = adhoc_policy->get_enddate();
			startdate = adhoc_policy->get_startdate();
		}

		LOGDEBUG(NX::string_formater(L" rights_evaluate: adhoc rights (%d), enddate (%llu)", *rights, enddate));

		// Is current user owner?
		if (get_profile().is_me(ownerid))
		{
			// Current user is owner
			if (bcheckowner)
			{
				if (ownerid.find(L"@") != ownerid.npos) // only for myvaule, owner is valid
				{
					size_t pos = ownerid.find(L"@");
					std::wstring tokengroupname = ownerid.substr(pos + 1);
					if (tokengroupname == _tenant_id)
					{
						final_result->add_rights(BUILTIN_RIGHT_ALL);
						if (*rights > 0)  // adhoc only
							*rights = BUILTIN_RIGHT_ALL;
						is_owner = true;
					}
				}
			}
			*evaluate_id = final_result->get_id();

			LOGINFO1(L" ");
			LOGINFO1(L"[Rights Evaluation]");
			LOGINFO1(NX::string_formater(L"  -> Id: %08X%08X", li_eval_id.HighPart, li_eval_id.LowPart));
			LOGINFO1(NX::string_formater(L"  -> User: %s (email: %s, membership: %s)", get_profile().get_name().c_str(), get_profile().get_email().c_str(), context.get_owner_id().c_str()));
			LOGINFO1(NX::string_formater(L"  -> Application%s: %s", ignore_app_condition ? L" (Not Evaluate)" : L"", proc_record.get_image_path().c_str()));
			LOGINFO1(NX::string_formater(L"  -> File: %s", file.c_str()));
			LOGINFO1(NX::string_formater(L"      (Duid: %s, Owner: %s)", str_doc_duid.c_str(), context.get_owner_id().c_str()));
			if (*rights > 0)
				LOGINFO1(L"  -> Grant Rights: Full control (Owner)");
			LOGINFO1(L" ");
			result = true;
			actrecord = activity_record(context.get_duid(), context.get_owner_id(), get_profile().get_id(), ActView, ActAllowed, file, image_path, proc_record.get_pe_file_info()->get_image_publisher(), std::wstring());
			break;
		}


		if (!is_owner && (enddate > 0))
		{
			unsigned __int64 currtime = (uint64_t)std::time(nullptr) * 1000;
			if (currtime > enddate)
			{
				LOGDEBUG(NX::string_formater(L" rights_evaluate: file expired curr: %llu, end: %llu", currtime, enddate));
				return false;
			}
			if (startdate > currtime)
			{
				LOGDEBUG(NX::string_formater(L" rights_evaluate: start date > curr: %llu, end: %llu", currtime, startdate));
				return false;
			}
		}

		result = true;

	} while (FALSE);
	CloseHandle(h);
	h = INVALID_HANDLE_VALUE;

	if (!result) {
		if (token_access_denied)
		{
			if (!ignore_app_condition)
			{
				log_activity(activity_record(doc_duid,
					doc_owner_id,
					get_profile().get_id(),
					ActView,
					ActDenied,
					file,
					image_path,
					proc_record.get_pe_file_info()->get_image_publisher(),
					std::wstring()));

				audit_activity(ActView, ActDenied, str_doc_duid, doc_owner_id, image_name, file);

				//fix bug 58062: not file owner to view central policy file for project and system bucket will pop up a error message
				//block_notify(file);
			}
		}

		*evaluate_id = final_result->get_id();
		*rights = 0;
		return false;
	}

	//
	// Now removed the conditional judgement of 'is_owner', fix bug 59214
    //
	//if (is_owner) 
	{

		if (!ignore_app_condition) {

			GLOBAL.get_process_cache().set_process_flags(process_id, NXRM_PROCESS_FLAG_WITH_NXL_OPENED);

			log_activity(activity_record(doc_duid,
				doc_owner_id,
				get_profile().get_id(),
				ActView,
				ActAllowed,
				file,
				image_path,
				proc_record.get_pe_file_info()->get_image_publisher(),
				std::wstring()));

			audit_activity(ActView, ActAllowed, str_doc_duid, doc_owner_id, image_name, file);
		}

		// Don't cache evaluation result because there is no real evaluation
		LOGDEBUG(NX::string_formater(L"ignore_app_condition"));
		//return true;
	}


	ed.push_data(L"user.id", NX::generic_value(get_profile().get_id()));
	ed.push_data(L"user.name", NX::generic_value(get_profile().get_name()));
	ed.push_data(L"user.email", NX::generic_value(get_profile().get_email()));
	ed.push_data(L"application.path", NX::generic_value(proc_record.get_image_path()));
	ed.push_data(L"application.is_associated_app", NX::generic_value((0 == _wcsicmp(assocated_app.c_str(), proc_record.get_image_path().c_str()) ? true : false)));
	std::shared_ptr<NX::win::pe_file> peinfo = proc_record.get_pe_file_info();
	if (peinfo != nullptr) {
		ed.push_data(L"application.signature.publisher", NX::generic_value(peinfo->get_image_publisher()));
		ed.push_data(L"application.name", NX::generic_value(peinfo->get_file_version().get_file_name()));
		ed.push_data(L"application.version", NX::generic_value(peinfo->get_file_version().get_file_version_string()));
		ed.push_data(L"application.company", NX::generic_value(peinfo->get_file_version().get_company_name()));
		ed.push_data(L"application.product", NX::generic_value(peinfo->get_file_version().get_product_name()));
		ed.push_data(L"application.product.version", NX::generic_value(peinfo->get_file_version().get_product_version_string()));
	}
	ed.push_data(L"host.name", NX::generic_value(GLOBAL.get_host_name().c_str()));
	ed.push_data(L"resource.path", NX::generic_value(file.c_str()));

	SYSTEMTIME tTime = { 0 };
	GetSystemTime(&tTime);
	FILETIME fTime = { 0 };
	SystemTimeToFileTime(&tTime, &fTime);

	ULARGE_INTEGER ui;
	ui.LowPart = fTime.dwLowDateTime;
	ui.HighPart = fTime.dwHighDateTime;
	ed.push_data(L"environment.date", NX::generic_value(((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000)));

	if (adhoc_policy != nullptr && !adhoc_policy->empty())
		adhoc_policy->evaluate(ed, final_result.get());

	unsigned __int64 right = final_result->get_rights();
	std::string tags = context.get_tags();
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsAndWatermarks;
	if (*rights == 0)
	{
		// call JPC
		std::wstring name = get_profile().get_name();
		std::wstring email = get_profile().get_email();

		std::vector<std::pair<std::wstring, std::wstring>> attrs = ImportJsonTags(tags);
		std::string bundle = "";
		std::string domain_bundle;
		std::wstring tenantName = NX::conversion::utf8_to_utf16(context.get_tenant());
		LOGDEBUG(NX::string_formater(L"central policy,  tenantName (%s)", tenantName.c_str()));

		/*if (tenantName.size() > 0)
		{
			bool bRet = get_policybundle(tenantName, bundle);
			if (!bRet) {
				LOGERROR(NX::string_formater(L"GetPolicyBundle failed: tenantName (%s)", tenantName.c_str()));
			}
		}
		else
			LOGERROR("Cannot find tenant name");

		std::wstring  defaultTenant = SERV->get_router_config().get_tenant_id();
		if ((defaultTenant.size() > 0) && (defaultTenant.compare(tenantName) != 0))
		{
			LOGDEBUG(NX::string_formater(L" central policy:  default tenant (%s)", defaultTenant.c_str()));

			bool bRet = get_policybundle(defaultTenant, domain_bundle);
			if (!bRet) {
				LOGERROR(NX::string_formater(L"GetPolicyBundle failed: default tenant (%s)", defaultTenant.c_str()));
			}

			bundle += domain_bundle;
		}*/
	
		// find user id
		std::wstring userIdpid = L"";
		std::vector<std::pair<std::wstring, std::wstring>> attributes = get_profile().get_userAttributes();
		for (auto it = attributes.begin(); attributes.end() != it; it++)
		{
			if ((*it).first.compare(L"idp_unique_id") == 0)
			{
				userIdpid = (*it).second;
			}
		}

		std::wstring userid = L"";
		for (auto it = attributes.begin(); attributes.end() != it; it++)
		{
			if ((*it).first.compare(userIdpid) == 0)
			{
				userid = (*it).second;
				break;
			}
		}

		// if user id not exist, use email
		if (userid == L"")
		{
			userid = email;
		}

		// get file token group resource type
		bool filebRet = false;
		std::string tokenGroupResourcetype;
		if (tenantName.size() > 0)
		{
			filebRet = get_tokengroup_resourcetype(tenantName, tokenGroupResourcetype);
			if (!filebRet) {
				LOGERROR(NX::string_formater(L"GetResourceType failed: tenantName (%s)", tenantName.c_str()));
			}
		}
		else
			LOGERROR("Cannot find tenant name");

		// get default tenant resource type
		bool tenantbRet = false;
		std::string tenantResourcetype;
		std::wstring  defaultTenant = SERV->get_router_config().get_tenant_id();
		if ((defaultTenant.size() > 0) && (defaultTenant.compare(tenantName) != 0))
		{
			LOGDEBUG(NX::string_formater(L" central policy:  default tenant (%s)", defaultTenant.c_str()));

			tenantbRet = get_tokengroup_resourcetype(defaultTenant, tenantResourcetype);
			if (!tenantbRet) {
				LOGERROR(NX::string_formater(L"GetResourceType failed: default tenant (%s)", defaultTenant.c_str()));
			}
		}

		if (!filebRet && !tenantbRet)
		{
			return false;
		}

		bool ready = false;
		SERV->ensurePDPConnectionReady(ready);
		if (!ready) {
			LOGDEBUG(NX::string_formater(L"*** cepdpman is not ready to eval rights"));
			return false;
		}

		// file token group resource type do evaluate right
		bool fileResult = false;
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> fileRightsAndWatermarks;
		if (filebRet)
		{
			fileResult = SERV->EvalRights(process_id, name, email, userid, image_path, file, NX::conversion::utf8_to_utf16(tokenGroupResourcetype), attrs, get_profile().get_userAttributes(), NX::conversion::utf8_to_utf16(bundle), &fileRightsAndWatermarks, nullptr, bcheckowner);
		}

		// default tenant resource type do evaluate right
		bool tenantResult = false;
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> tenantRightsAndWatermarks;
		if (tenantbRet)
		{
			tenantResult = SERV->EvalRights(process_id, name, email, userid, image_path, file, NX::conversion::utf8_to_utf16(tenantResourcetype), attrs, get_profile().get_userAttributes(), NX::conversion::utf8_to_utf16(bundle), &tenantRightsAndWatermarks, nullptr, bcheckowner);
		}

		MergeRightsWatermark(fileRightsAndWatermarks, tenantRightsAndWatermarks, rightsAndWatermarks);

		for (const auto& rightAndWatermarks : rightsAndWatermarks) {
			*rights |= rightAndWatermarks.first;
		}

		if (fileResult || tenantResult)
		{
			result = true;
		}

		_ad_hoc = false;
		LOGDEBUG(NX::string_formater(L" rights_evaluate: central policy,  rights (%d)", *rights));
	}

	/*std::shared_ptr<policy_bundle> central_policy = get_policy_bundle();
	if ((central_policy != nullptr && !central_policy->empty())) {
		central_policy->evaluate(ed, final_result.get());
	} */

	// set result
	*evaluate_id = final_result->get_id();
	//*rights = final_result->get_rights();
	if (!ignore_app_condition) {
		_eval_cache.push(final_result);
		// set NXRM_PROCESS_FLAG_WITH_NXL_OPENED flag
		if (0 != final_result->get_rights()) {
			GLOBAL.get_process_cache().set_process_flags(process_id, NXRM_PROCESS_FLAG_WITH_NXL_OPENED);
			// Set Overlay Flag if there is watermark obligation
			if (final_result->has_obligation(L"WATERMARK")) {
				adhoc_obligation_to_watermark(*rights, context.get_policy(), rightsAndWatermarks);
				
				GLOBAL.get_process_cache().set_process_flags(process_id, NXRM_PROCESS_FLAG_WITH_OVERLAY_OBLIGATION);
				SERV->get_coreserv().drvctl_set_overlay_windows2(session_id);
			}
		}
		else {
			// handle central policy

			// Notify user if it is blocked

			//Fix bug 58062
			//block_notify(file);
		}
	}


	if (!ignore_app_condition) {

		log_activity(activity_record(doc_duid,
			doc_owner_id,
			get_profile().get_id(),
			ActView,
			(0 == *rights) ? ActDenied : ActAllowed,
			file,
			image_path,
			proc_record.get_pe_file_info()->get_image_publisher(),
			std::wstring()));

		audit_activity(ActView, (0 == *rights) ? ActDenied : ActAllowed, str_doc_duid, doc_owner_id, image_name, file);
	}


	const std::wstring& granted_rights_str = final_result->get_rights_string();
	LOGINFO1(L" ");
	LOGINFO1(L"[Rights Evaluation]");
	LOGINFO1(NX::string_formater(L"  -> Id: %08X%08X", li_eval_id.HighPart, li_eval_id.LowPart));
	LOGDEBUG(NX::string_formater(L"  -> User: %s (email: %s)", get_profile().get_name().c_str(), get_profile().get_email().c_str()));
	LOGINFO1(NX::string_formater(L"  -> File: %s", file.c_str()));
	LOGINFO1(NX::string_formater(L"      (Duid: %s, Owner: %s)", str_doc_duid.c_str(), doc_owner_id.c_str()));
	LOGINFO1(NX::string_formater(L"  -> Application%s: %s", ignore_app_condition ? L" (Not Evaluate)" : L"", proc_record.get_image_path().c_str()));
	//LOGINFO1(NX::string_formater(L"  -> Grant Rights: %s", granted_rights_str.c_str()));
	LOGINFO1(NX::string_formater(L"  -> Grant Rights: %d", *rights));


	if (NX::dbg::LL_DEBUG <= NxGlobalLog.get_accepted_level()) {

		std::vector<std::pair<std::wstring, std::wstring>> user_parameters;
		std::vector<std::pair<std::wstring, std::wstring>> app_parameters;
		std::vector<std::pair<std::wstring, std::wstring>> host_parameters;
		std::vector<std::pair<std::wstring, std::wstring>> resources_parameters;
		std::vector<std::pair<std::wstring, std::wstring>> env_parameters;
		std::vector<std::pair<std::wstring, std::wstring>> other_parameters;

		std::for_each(ed.get_values().begin(), ed.get_values().end(), [&](const std::pair<std::wstring, NX::generic_value>& item) {
			if (boost::algorithm::istarts_with(item.first, L"user.")) {
				user_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
			}
			else if (boost::algorithm::istarts_with(item.first, L"application.")) {
				app_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
			}
			else if (boost::algorithm::istarts_with(item.first, L"host.")) {
				host_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
			}
			else if (boost::algorithm::istarts_with(item.first, L"resource.")) {
				resources_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
			}
			else if (boost::algorithm::istarts_with(item.first, L"environment.")) {
				env_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
			}
			else {
				other_parameters.push_back(std::pair<std::wstring, std::wstring>(item.first, item.second.serialize()));
			}
		});

		LOGDEBUG(L"  Details:");
		LOGDEBUG(L"    <User>");
		std::for_each(user_parameters.begin(), user_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
			LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
		});
		LOGDEBUG(L"    <Application>");
		std::for_each(app_parameters.begin(), app_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
			LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
		});
		LOGDEBUG(L"    <Host>");
		std::for_each(host_parameters.begin(), host_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
			LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
		});
		LOGDEBUG(L"    <Resource>");
		std::for_each(resources_parameters.begin(), resources_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
			LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
		});
		LOGDEBUG(L"    <Environment>");
		std::for_each(env_parameters.begin(), env_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
			LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
		});
		if (!other_parameters.empty()) {
			LOGDEBUG(L"    <Others>");
			std::for_each(other_parameters.begin(), other_parameters.end(), [&](const std::pair<std::wstring, std::wstring>& parameter) {
				LOGDEBUG(NX::string_formater(L"        %s = %s", parameter.first.c_str(), parameter.second.c_str()));
			});
		}
		LOGDEBUG(L"  Obligations:");
		std::for_each(final_result->get_obligations().begin(), final_result->get_obligations().end(), [&](const std::pair<std::wstring, std::shared_ptr<obligation>>& ob) {
			LOGDEBUG(NX::string_formater(L"      - %s", ob.first.c_str()));
		});
		LOGINFO1(L" ");
	}

	vecRightsWatermarks.clear();
	for (auto item : rightsAndWatermarks)
	{
		vecRightsWatermarks.push_back(item);
	}

	return true;
}

std::wstring rmsession::file_rights_watermark_to_json(const std::wstring& strPath, uint64_t u64Rights,
	const std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& vecRightsWatermarks)
{
	NX::json_value v = NX::json_value::create_object();
	v[L"path"] = NX::json_value(NX::fs::dos_fullfilepath(strPath).path());
	v[L"rights"] = NX::json_value(u64Rights);
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

	std::wstring strJson = v.serialize();
	return std::move(strJson);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool rmsession::trust_evaluate(unsigned long process_id, _Out_ bool* trusted)
{
	*trusted = SERV->IsProcessTrusted(process_id);
	return true;
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

bool rmsession::read_file_tags(const std::wstring& file, unsigned long process_id, unsigned long session_id, std::wstring &tags)
{
	bool result = false;

	// Init
	HANDLE h = INVALID_HANDLE_VALUE;

	//
	// added by Philip Qi
	// in a inefficiency way, impersonate when 1) opening file 2) read content
	//
	{
		NX::win::session_token st(session_id);
		NX::win::impersonate_object impersonobj(st);

		h = ::CreateFileW(NX::fs::dos_fullfilepath(file).global_dos_path().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		LOGDEBUG(NX::string_formater(L"rights_evaluate: file: %s", file.c_str()));
		if (INVALID_HANDLE_VALUE == h) {
			LOGWARNING(NX::string_formater(L"Eval: fail to open target file (%d): %s", GetLastError(), file.c_str()));
			return false;
		}
	}

	NX::NXL::document_context context;

	// Get file information
	do {
		//
		// added by Philip Qi
		// in a inefficiency way, impersonate when 1) opening file 2) read content
		//
		{
			NX::win::session_token st(session_id);
			NX::win::impersonate_object impersonobj(st);

			// load NXL file context
			if (!context.load(h)) {
				break;
			}
		}

		// get token
		const std::wstring doc_policy = NX::conversion::utf8_to_utf16(context.get_policy());
		const std::wstring doc_tags = NX::conversion::utf8_to_utf16(context.get_tags());
		int protection_type = (doc_policy.empty() || doc_policy == L"{}") ? 1 : 0;   // 1 = central, 0 = ad-hoc
		result = true;
		tags = doc_tags;
	} while (FALSE);
	CloseHandle(h);
	h = INVALID_HANDLE_VALUE;
	
	return result;
}

static void remove_escape_characters(std::wstring& s)
{
    if (s.length() != 0) {

        std::wstring ns;
        bool changed = false;
        const wchar_t* const end_pos = s.c_str() + s.length();
        const wchar_t * p = s.c_str();
        while (p != end_pos) {

            wchar_t c = *(p++);
            if (c == L'\\') {
                switch (*p)
                {
                case L'n':
                    c = L'\n';
                    ++p;
                    changed = true;
                    break;
                case L'r':
                    c = L'\r';
                    ++p;
                    changed = true;
                    break;
                case L't':
                    c = L'\t';
                    ++p;
                    changed = true;
                    break;
                case L'\\':
                    c = L'\\';
                    ++p;
                    changed = true;
                    break;
                default:
                    break;
                }
            }
            ns.push_back(c);
        }

        if (changed) {
            s = ns;
        }
    }
}

static std::wstring remove_escape_characters2(const std::wstring& s)
{
    std::wstring ns;

    if (s.empty()) {
        return ns;
    }


    const wchar_t* const end_pos = s.c_str() + s.length();
    const wchar_t * p = s.c_str();


    while (p != end_pos) {

        const wchar_t c = *(p++);
        if (c != L'\\') {
            ns.push_back(c);
            continue;
        }

        assert(c == L'\\');
        if (p == end_pos) {
            break;
        }

        const wchar_t c2 = *(p++);
        switch (c2)
        {
        case L'r':
            // ignore it
            break;
        case L'n':
            ns.push_back(L'\n');
            break;
        case L't':
            ns.push_back(L'\t');
            break;
        case L'\\':
            ns.push_back(L'\\');
            break;
        default:
            ns.push_back(L'\\');
            ns.push_back(c2);
            break;
        }
    }

    return std::move(ns);
}

std::wstring rmsession::normalize_image_text(const std::wstring& expandable_text)
{
    // We only support:
    //  - $(User)
    //  - $(Host)
    std::wstring output;
    const wchar_t* p = expandable_text.c_str();

    while (p != nullptr && 0 != *p) {

        const wchar_t* dollar = wcschr(p, L'$');

        if (dollar == nullptr) {
            output += p;
            p = nullptr;
            break;
        }
        else {
            output += std::wstring(p, dollar);
            p = dollar;

            if (0 == _wcsnicmp(p, L"$(User)", 7)) {
                output += get_profile().get_email();
                p += 7;
            }
            else if (0 == _wcsnicmp(p, L"$(Host)", 7)) {
                output += GLOBAL.get_host_name();
                p += 7;
            }
			else if (0 == _wcsnicmp(p, L"$(Date)", 7)) {
				SYSTEMTIME st;

				GetLocalTime(&st);
				WCHAR	dateFormat[32];
				WCHAR	dateString[32];
				if (GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_LONGDATE, &st, L"YYYY-MM-DD", dateFormat, sizeof(dateFormat), NULL)) {
					swprintf_s(dateString, 32, dateFormat, st.wYear, st.wMonth, st.wDay);
				}
				else {
					swprintf_s(dateString, 32, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
				}

				output += dateString;
				p += 7;
			}
			else if (0 == _wcsnicmp(p, L"$(Time)", 7)) {
				SYSTEMTIME st;

				GetLocalTime(&st);
				WCHAR	timeFormat[32];
				WCHAR	timeString[32];
				if (GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &st, L"HH:mm:ss", timeFormat, sizeof(timeFormat))) {
					swprintf_s(timeString, 32, timeFormat, st.wHour, st.wMinute, st.wMinute);
				}
				else {
					swprintf_s(timeString, 32, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
				}

				output += timeString;
				p += 7;

			}
            else {
                output += L"$";
                ++p;
            }
        }
    }

    output = remove_escape_characters2(output);
    return std::move(output);
}

bool rmsession::generate_watermark_image()
{
    NX::image::CTextBitmap bitmap;

    const std::wstring file(_temp_profile_dir + L"\\watermark.png");
    const std::wstring normalized_text = normalize_image_text(get_watermark_config().get_message());
    int watermark_angle = 0;

    if (get_watermark_config().get_rotation() == _ANTICLOCKWISE) {
        watermark_angle = 45;
    }
    else if (get_watermark_config().get_rotation() == _CLOCKWISE) {
        watermark_angle = -45;
    }
    else {
        watermark_angle = 0;
    }

    if (!bitmap.Create(normalized_text.c_str(), get_watermark_config().get_font_name().c_str(), get_watermark_config().get_font_size(), RGB(255, 255, 255), get_watermark_config().get_font_color(), watermark_angle)) {
        return false;
    }


    ::DeleteFileW(file.c_str());
    if (!bitmap.ToPNGFile(file.c_str())) {
        return false;
    }
    
    NX::win::explicit_access everyone_ea(SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID, GENERIC_READ, SUB_CONTAINERS_AND_OBJECTS_INHERIT);
    if (!NX::win::file_security::grant_access(file, everyone_ea)) {
        LOGWARNING(NX::string_formater(L"Fail to give everyone access to watermark image file (%d)", GetLastError()));
    }
    
    SERV->get_coreserv().drvctl_set_overlay_bitmap_status(_winsession->get_session_id(), true);
    return true;
}

bool rmsession::does_watermark_image_exist()
{
    const std::wstring file(_temp_profile_dir + L"\\watermark.png");
    return NX::fs::exists(file);
}

bool rmsession::upload_data(const upload_object& object)
{
    bool result = false;

    switch (object.get_type())
    {
    case UPLOAD_SHARING_TRANSITION:
        result = _rs_executor.request_post_sharing_trasition(get_profile().get_id(), get_profile().get_token().get_ticket(), object.get_checksum(), object.get_data());
        break;
    default:
        // unknown type
        assert(false);
        LOGWARNING(NX::string_formater("Unknown upload object type (%d)", object.get_type()));
        break;
    }

    return result;
}

bool rmsession::upload_sharing_transition(unsigned long process_id, int shareType, const std::wstring& file, const std::wstring& sharing_info)
{
    const NX::NXL::document_context context(NX::fs::dos_fullfilepath(file).global_dos_path());
    std::vector<unsigned char> file_token;

    if (context.empty()) {
        return false;
    }

    const std::wstring policy = NX::conversion::utf8_to_utf16(context.get_policy());
    const std::wstring tags = NX::conversion::utf8_to_utf16(context.get_tags());
    int protection_type = (policy.empty() || policy == L"{}") ? 1 : 0;   // 1 = central, 0 = ad-hoc
    if (!query_token(process_id, context.get_owner_id(), context.get_agreement0(), context.get_duid(), protection_type, policy, tags, context.get_token_level(), file_token, NULL)) {
        return false;
    }
    if (file_token.size() != 32) {
        return false;
    }

    const std::string& s = NX::conversion::utf16_to_utf8(sharing_info);
    const unsigned long info_size = (unsigned long)s.length();
    std::vector<unsigned char> buf;
    buf.resize(4 + info_size, 0);
    memcpy(buf.data(), &info_size, 4);
    memcpy(buf.data() + 4, s.c_str(), info_size);
    std::vector<unsigned char> checksum;
    checksum.resize(32, 0);

    if (!NX::crypto::hmac_sha256(buf.data(), (unsigned long)buf.size(), file_token.data(), 32, checksum.data())) {
        return false;
    }

    // Succeed. log it
    if (0 == shareType) {
        log_activity(activity_record(context.get_duid(),
                                     context.get_owner_id(),
                                     get_profile().get_id(),
                                     ActProtect,
                                     ActAllowed,
                                     file,
                                     L"Rights Management Desktop",
                                     L"NextLabs Inc",
                                     std::wstring()));
        audit_activity(ActProtect, ActAllowed, NX::conversion::to_wstring(context.get_duid()), context.get_owner_id(), L"explorer.exe", file);
    }
    log_activity(activity_record(context.get_duid(),
                                 context.get_owner_id(),
                                 get_profile().get_id(),
                                 ActShare,
                                 ActAllowed,
                                 file,
                                 L"Rights Management Desktop",
                                 L"NextLabs Inc",
                                 std::wstring()));
    audit_activity(ActShare, ActAllowed, NX::conversion::to_wstring(context.get_duid()), context.get_owner_id(), L"explorer.exe", file);

    return _upload_serv.push(upload_object(UPLOAD_SHARING_TRANSITION, NX::conversion::to_wstring(checksum), sharing_info));
}

bool rmsession::log_metadata(const metadata_record& record, const int option)
{
    if (option == 0)
    {
        LOGDEBUG(NX::string_formater(L"log_metadata: store metadata before checkin to RMS, (%s)", record.get_log().c_str()));
        _metalogger.push(record); // store it and upload later
    }
	else // option == 1, upload immediately
	{
		std::vector<std::string> buf;
		std::vector<std::string> retrylogs;

        LOGDEBUG(NX::string_formater(L"log_metadata: checkin metadata to RMS"));
        buf.push_back(record.get_log());

        bool buploaded = upload_metadata_log(buf, retrylogs);
        if (retrylogs.size() > 0)
        {
            LOGDEBUG(NX::string_formater(L"log_metadata: failed to checkin metadata, store it now, (%s)", record.get_log().c_str()));
            _metalogger.push(record); // store it and upload later
        }

        return buploaded;
	}

    return true;
}

void rmsession::log_activity(const activity_record& record)
{
	// 2019-05-25: Raymond Zeng
	// this function is called by RPM service/driver to log the activities evaluated internally.
	// However, with RMD/RMX, all the evaluation which needs to be logged shall be from these APPs.

    //size_t size = _actlogger.push(record);
    //if (size > 64) {
    //    _timer.force_uploadlog();
    //}
}

void rmsession::log_activity2(const activity_record& record)
{
	size_t size = _actlogger.push(record);
	//if (size > 64) {
	//	_timer.force_uploadlog();
	//}
}

bool rmsession::upload_activity_log(const std::vector<unsigned char>& logs)
{
    return _rs_executor.request_log_activities(get_profile().get_id(), get_profile().get_token().get_ticket(), logs);
}

bool rmsession::upload_metadata_log(const std::vector<std::string>& logs, std::vector<std::string>& retrylogs)
{
    LOGDEBUG(NX::string_formater(L"rmsession::upload_metadata_log: total (%d) record(s)", logs.size()));
    bool bSuccessed = true;
	for (size_t i = 0; i < logs.size(); i++)
	{
		std::string metadata = logs[i];
		std::wstring duid;
		std::wstring params;
		int projectid = 0;
		int type = 0;

		NX::json_value json_metadata = NX::json_value::parse(NX::conversion::utf8_to_utf16(metadata));
		if (json_metadata.as_object().has_field(L"parameters")) {
			NX::json_value json_params = json_metadata.as_object().at(L"parameters");
			duid = json_params.as_object().at(L"duid").as_string();
			params = json_params.as_object().at(L"params").as_string();
			projectid = json_params.as_object().at(L"projectid").as_number().to_uint32();
			type = json_params.as_object().at(L"type").as_number().to_uint32();
		}

        LOGDEBUG(NX::string_formater(L"rmsession::upload_metadata_log: DUID(%s), params(%s)", duid.c_str(), params.c_str()));
        // check whether the {DUID, PATH} is locked or not
		if (_locked_duid_map.find(duid) != _locked_duid_map.end())
		{
			// duid is locked, skip it
			retrylogs.push_back(metadata);
			continue;
		}

		if (type == RECLASSIFY_TYPE::CLASSIFY_NEW_FILE)
		{ 
            if (_rs_executor.request_classify_file(get_profile().get_id(), get_profile().get_token().get_ticket(), get_profile().get_default_tenantId(), duid, params) == false)
                retrylogs.push_back(metadata);
		}
		else if (type == RECLASSIFY_TYPE::RECLASSIFY_TENANT_FILE)
		{
			if (_rs_executor.request_reclassify_file(get_profile().get_id(), get_profile().get_token().get_ticket(), get_profile().get_default_tenantId(), duid, params) == false)
                bSuccessed = false; // retrylogs.push_back(metadata);
		}
		else // type == RECLASSIFY_TYPE::RECLASSIFY_PROJECT_FILE
		{
			if (_rs_executor.request_reclassify_project_file(get_profile().get_id(), get_profile().get_token().get_ticket(), get_profile().get_default_tenantId(), duid, projectid, params) == false)
                bSuccessed = false; // retrylogs.push_back(metadata);
		}
	}

    LOGDEBUG(NX::string_formater(L"rmsession::upload_metadata_log: failed total (%d) record(s)", retrylogs.size()));

    return bSuccessed;
}

bool rmsession::insert_directory_action(const std::wstring& filepath)
{
	// remove the "AutoProtect" option from the [AutoProtect, Overwrite, Ext]
	// options at the end of the string before passing the string to nxrmflt
	// driver, since nxrmflt driver doesn't recognize the option.
	std::wstring tmp = filepath;
	tmp.erase(tmp.size() - 3, 1);

	return SERV->get_fltserv().insert_safe_dir(tmp);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool rmsession::insert_sanctuary_directory_action(const std::wstring& filepath)
{
	return SERV->get_fltserv().insert_sanctuary_dir(filepath);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

bool rmsession::remove_directory_action(const std::wstring& filepath)
{
	return SERV->get_fltserv().remove_safe_dir(filepath);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool rmsession::remove_sanctuary_directory_action(const std::wstring& filepath)
{
	return SERV->get_fltserv().remove_sanctuary_dir(filepath);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

bool rmsession::delete_nxl_file(const std::wstring& filename)
{
	return SERV->get_fltserv().delete_nxl_file(filename);
}

void rmsession::on_heartbeat_v1()
{
    bool result = false;
    const NX::time::datetime current_tm = NX::time::datetime::current_time();	
	LOGDEBUG(L"rmsession::on_heartbeat");

    if (!ensure_rs_executor()) {
        return;
    }

    const std::vector<std::pair<std::wstring, std::wstring>> objects_request({
        std::pair<std::wstring, std::wstring>(NAME_POLICY_BUNDLE, _config_serial_numbers[NAME_POLICY_BUNDLE]),
        std::pair<std::wstring, std::wstring>(NAME_CLIENT_CONFIG, _config_serial_numbers[NAME_CLIENT_CONFIG]),
        std::pair<std::wstring, std::wstring>(NAME_CLASSIFY_CONFIG, _config_serial_numbers[NAME_CLASSIFY_CONFIG]),
        std::pair<std::wstring, std::wstring>(NAME_WATERMARK_CONFIG, _config_serial_numbers[NAME_WATERMARK_CONFIG])
    });

    std::vector<std::pair<std::wstring, std::pair<std::wstring, std::wstring>>> objects_returned;

    result = _rs_executor.request_heartbeat_v1(get_profile().get_id(), get_tenant_id(), get_profile().get_token().get_ticket(), objects_request, objects_returned);
    if (!result) {
        return;
    }

    // Only update timestamp when heartbeat succeed
    _local_config._last_heartbeat_time = (__int64)current_tm;
    if (_local_config._last_update_time == 0) {
        result = true;
        _local_config._last_update_time = (__int64)current_tm;
    }

    std::for_each(objects_returned.begin(), objects_returned.end(), [&](const std::pair<std::wstring, std::pair<std::wstring, std::wstring>>& object) {
        if (0 == _wcsicmp(object.first.c_str(), L"policyBundle")) {
            if (object.second.first.empty()) {
                LOGERROR(L"Heartbeat return a policy bundle without tenant name");
                return;
            }
            if (object.second.second.empty()) {
                LOGDEBUG(L"Heartbeat return a policy bundle without content");
                return;
            }
            if (update_policy(object.second.first, object.second.second)) {
                result = true;
                _local_config._last_update_time = (__int64)current_tm;
            }
        }
        else if (0 == _wcsicmp(object.first.c_str(), L"clientConfig")) {
            if (object.second.first.empty()) {
                LOGERROR(L"Heartbeat return a client config without serial number, ignore it");
                return;
            }
            if (object.second.second.empty()) {
                LOGDEBUG(L"Heartbeat return a client config without content, ignore it");
                return;
            }
            update_client_config(object.second.first, object.second.second);
            result = true;
        }
        else if (0 == _wcsicmp(object.first.c_str(), L"classifyConfig")) {
            if (object.second.first.empty()) {
                LOGERROR(L"Heartbeat return a classify config without serial number, ignore it");
                return;
            }
            if (object.second.second.empty()) {
                LOGDEBUG(L"Heartbeat return a classify config without content, ignore it");
                return;
            }
            update_classify_config(object.second.first, object.second.second);
            result = true;
        }
        else if (0 == _wcsicmp(object.first.c_str(), L"watermarkConfig")) {
            if (object.second.first.empty()) {
                LOGERROR(L"Heartbeat return a watermark config without serial number, ignore it");
                return;
            }
            if (object.second.second.empty()) {
                LOGDEBUG(L"Heartbeat return a watermark config without content, ignore it");
                return;
            }
            update_watermark_config(object.second.first, object.second.second);
            result = true;
        }
        else {
            LOGDEBUG(NX::string_formater(L"Heartbeat received unknown object - %s", object.first.c_str()));
        }
    });

    if (result) {
        get_app_manager().send_status_changed_notification();
        // Save local config file
        const std::wstring local_config_file(_protected_profile_dir + L"\\" + CLIENT_LOCAL_CONFIG_FILE);
        const std::wstring& content = _local_config.serialize();
        GLOBAL.nt_generate_file(local_config_file, NX::conversion::utf16_to_utf8(content), true);
    }
}

void rmsession::on_heartbeat()
{
	bool result = false;
	const NX::time::datetime current_tm = NX::time::datetime::current_time();
	unsigned int heartbeatFrequency = 0;    // in minutes
	LOGDEBUG(L"rmsession::on_heartbeat");

	// check the register.xml changed or not
	nx::capp_whiltelist_config::getInstance()->check_register_file();

	if (!ensure_rs_executor()) {
		return;
	}
	std::map<std::wstring, std::wstring> policy_map;
	std::wstring policyData;
	std::map<std::wstring, std::wstring> tokengroupRT_map;
	std::wstring tokengroupRT_data;
	result = _rs_executor.request_heartbeat(get_profile().get_id(), get_tenant_id(), get_profile().get_token().get_ticket(), policy_map, policyData, &heartbeatFrequency, tokengroupRT_map, tokengroupRT_data);
	if (!result) {
		return;
	}
	int client_heartbeat = 0; bool adhoc = false; bool dapenable = false; std::wstring system_bucket_name; std::wstring icenetUrl;
	result = _rs_executor.get_tenant_preference(get_profile().get_id(), get_profile().get_token().get_ticket(), get_profile().get_default_tenantId(), &client_heartbeat, &adhoc, &dapenable, system_bucket_name, icenetUrl);
	if (result)
	{
		_system_bucket_name = system_bucket_name;
	}
	// Only update timestamp when heartbeat succeed
	_local_config._last_heartbeat_time = (__int64)current_tm;
	_client_config.set_heartbeat_interval(heartbeatFrequency * 60); // convert minutes to seconds
	if (_local_config._last_update_time == 0) {
		result = true;
		_local_config._last_update_time = (__int64)current_tm;
	}
	if (update_policy(policy_map, policyData)) {
		result = true;
		_local_config._last_update_time = (__int64)current_tm;
	}
	if (update_tokengroup_resourcetype(tokengroupRT_map, tokengroupRT_data))
	{
		result = true;
	}
	if (result) {
		get_app_manager().send_status_changed_notification();
		// Save local config file
		const std::wstring local_config_file(_protected_profile_dir + L"\\" + CLIENT_LOCAL_CONFIG_FILE);
		const std::wstring& content = _local_config.serialize();
		GLOBAL.nt_generate_file(local_config_file, NX::conversion::utf16_to_utf8(content), true);
		LOGERROR(NX::string_formater(L"on_heartbeat:  content: %s", content.c_str()));
	}
}

void rmsession::on_uploadlog()
{
    _actlogger.upload_log();
	_metalogger.upload_log();

}

void rmsession::on_checkupgrade()
{
    std::wstring newVersion;
    std::wstring downloadURL;
    std::wstring sha1Checksum;
    
    if (_rs_executor.request_upgrade(_tenant_id, GLOBAL.get_product_version_string(), newVersion, downloadURL, sha1Checksum)) {

		std::wstring installer;
		LOGINFO(NX::string_formater(L"AutoUpdate: New version is found ...\r\n\tVersion: %s\r\n\tUrl: %s\r\n\tChecksum: %s", newVersion.c_str(), downloadURL.c_str(), sha1Checksum.c_str()));

        if (GLOBAL.get_process_cache().does_protected_process_exist((ULONG)-1)) {
            if (NX::dbg::LL_DETAIL > NxGlobalLog.get_accepted_level()) {
                LOGINFO(L"AutoUpdate: Ignore upgrade dure to processes with opened NXL file\r\n  --> Upgrade will be done at next check point.");
            }
            else {
                const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes = GLOBAL.get_process_cache().find_all_protected_process((ULONG)-1);
                std::wstring dbginfo(L"AutoUpdate: Ignore upgrade dure to processes with opened NXL file:\r\n");
                for (int i = 0; i < (int)protected_processes.size(); i++) {
                    const std::pair<unsigned long, std::wstring>& item = protected_processes[i];
                    const wchar_t* pname = wcsrchr(item.second.c_str(), L'\\');
                    pname = (NULL == pname) ? item.second.c_str() : (pname + 1);
                    dbginfo.append(L"    ");
                    dbginfo.append(pname);
                    dbginfo.append(L" (");
                    dbginfo.append(NX::conversion::to_wstring((int)item.first));
                    dbginfo.append(L")\r\n");
                }
                dbginfo.append(L"--> Upgrade will be done at next check point.");
                LOGDETAIL(dbginfo);
            }
            return;
        }

		if (!download_new_version(downloadURL, sha1Checksum, installer)) {
			return;
		}

		if (!SERV->get_service_conf().is_disable_autoupgrade_install()) {
			if (install_new_version(installer)) {
				LOGINFO(NX::string_formater(L"AutoUpdate: Lanuch %s installer compeleted\r\n", newVersion.c_str()));
			}
		}
		else {
			LOGINFO(NX::string_formater(L"AutoUpdate: Skip lanunching installer at %s", installer.c_str()));
		}
	}
	else {
		LOGINFO(L"AutoUpdate: NO new version found.");
	}
}

void rmsession::init_dirs()
{
    assert(is_logged_on());
}

void rmsession::init_classify_config(const std::string& content)
{
    assert(is_logged_on());
}

bool rmsession::init_logdb()
{
    bool result = false;

    if (!is_logged_on()) {
        return false;
    }

    if (_audit_db.opened()) {
        return true;
    }

    const std::wstring dbfile(get_temp_profile_dir() + L"\\audit.db");

    try {

        if (NX::fs::exists(dbfile)) {
            _audit_db.open(dbfile, false);
        }
        else {
            std::wstring description(L"audit db for user ");
            description += get_profile().get_name();
            const audit_logdb_conf new_db_conf(std::string(description.begin(), description.end()));
            _audit_db.create(dbfile, new_db_conf);
        }

        result = true;
    }
    catch (std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        LOGERROR(NX::string_formater(L"Session (%d, user: %I64d, %s) fail to open audit db (%d) (%s)", _winsession->get_session_id(), get_profile().get_id(), get_profile().get_name().c_str(), GetLastError(), dbfile.c_str()));
        result = false;
    }

    return result;
}

void rmsession::clear_logdb()
{
    if (_audit_db.opened()) {
        _audit_db.close();
    }
}

void rmsession::audit_activity(int user_operation,
    int result,
    const std::wstring& duid,
    const std::wstring& owner,
    const std::wstring& app_name,
    const std::wstring& file_path)
{
	return;

    if (!_audit_db.opened()) {
        return;
    }

    NX::time::datetime current_tm = NX::time::datetime::current_time();

    try {
        _audit_db.push_record(audit_logdb_record((__int64)current_tm,
            (int)GLOBAL.get_windows_info().platform_id(),
            user_operation,
            result,
            get_profile().get_id(),
            duid,
            owner,
            SERV->get_client_id(),
            app_name,
            file_path));
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}

bool rmsession::export_audit_log()
{
    if (!_audit_db.opened()) {
        return false;
    }

    bool result = false;
    const std::wstring target_file(get_temp_profile_dir() + L"\\audit.json");

    try {
        _audit_db.export_log2(target_file, true);
        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool rmsession::export_audit_query_result(std::wstring& outfile)
{
    if (!_audit_db.opened()) {
        return false;
    }

    bool result = false;
    GetTempFileNameW(get_temp_profile_dir().c_str(), L"LOG", 0, NX::string_buffer<wchar_t>(outfile, MAX_PATH));

    try {
        _audit_db.export_log2(outfile, true);
        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        outfile.clear();
        result = false;
    }

    return result;
}

void rmsession::init_memberships()
{
	_profile.init_membership_info(_protected_profile_dir, this);
	_profile.init_membership_tokens(_protected_profile_dir, this);
}


bool rmsession::on_set_user_attributes(const NX::json_value& json) {
	bool result = false;

	try {
		const NX::json_object& logon_data = json.as_object();
		const __int64 userId = logon_data.at(L"userId").as_int64();
		result = _profile.create(json);
		if (!result) {
			throw std::exception("Invalid logon data");
		}

		init_memberships();
	}
	catch (const std::exception& e) {
		LOGWARNING(NX::string_formater(L"rmsession::logon error -> %S", e.what()));
		result = false;
	}
	return result;
}

bool rmsession::load_user_cached_tokens() {
	//assert(!_profile.empty());

	// prepare files path
	std::wstring cached_token_file = get_user_cached_tokens_file();

	std::string content;
	if (!GLOBAL.nt_load_file(cached_token_file, content)) {
		LOGDETAIL(NX::string_formater(L"Tokens config file doesn't exist (%s)", cached_token_file.c_str()));
		return false;
	}

	try {
		NX::json_value cached_token = NX::json_value::parse(content);
		if (!cached_token.is_object()) {
			return false;
		}
		if (!cached_token.as_object().has_field(TOKENLIST_ELEMENT_NAME)) {
			return false;
		}
		const NX::json_value & tokenList = cached_token.as_object().at(TOKENLIST_ELEMENT_NAME);
		if (!tokenList.is_array()) {
			return false;
		}
		const NX::json_array & arrToken = tokenList.as_array();
		for (int i = 0; i < arrToken.size(); ++i) {
			const NX::json_object & token = arrToken[i].as_object();
			std::vector<unsigned char> tokenId = NX::conversion::hex_to_bytes(token.at(TOKENID_ELEMENT_NAME).as_string());
			std::vector<unsigned char> tokenOtp;
			if (token.has_field(TOKENOTP_ELEMENT_NAME))
				tokenOtp = NX::conversion::hex_to_bytes(token.at(TOKENOTP_ELEMENT_NAME).as_string());
			time_t tokenTtl = 0;
			if (token.has_field(TOKENTTL_ELEMENT_NAME))
				tokenTtl = token.at(TOKENTTL_ELEMENT_NAME).as_int64();
			unsigned __int32 level = token.at(TOKENMAINTAINLEVEL_ELEMENT_NAME).as_uint32();
			std::vector<unsigned char> tokenValue = NX::conversion::hex_to_bytes(token.at(TOKENKEY_ELEMENT_NAME).as_string());
			_token_cache.insert(tokenId, level, tokenValue, tokenTtl, tokenOtp);
		}
		_token_cache_dirty = false;
		LOGDEBUG(NX::string_formater(L"Load %d cached tokens from config file %s", arrToken.size(), cached_token_file.c_str()));
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
	}

	return true;
}

bool rmsession::delete_user_cached_tokens()
{
	// prepare files path
	_token_cache.clear();
	std::wstring cached_token_file = get_user_cached_tokens_file();
	LOGDEBUG(NX::string_formater(L"delete_user_cached_tokens %s", cached_token_file.c_str()));
	std::string ws = "";
	bool result = GLOBAL.nt_generate_file(cached_token_file, ws, true);
	std::string content;
	result = GLOBAL.nt_load_file(cached_token_file, content);  // verify

	NT::DeleteFile(cached_token_file.c_str());		
	_token_cache_dirty = false;
	return true;
}

std::wstring rmsession::get_user_cached_tokens_file()
{
	std::wstring cached_token_file(_protected_profile_dir);
	cached_token_file += L"\\";
	cached_token_file += L"";
	cached_token_file += L"cached.tokens";
	return cached_token_file;
}

bool rmsession::save_user_cached_tokens()
{
	bool result = false;

	//assert(!_profile.empty());
	//LOGDEBUG(NX::string_formater(L"start save_user_cached_tokens"));
	if (_token_cache_dirty == false)
		return true;

	// prepare files path
	std::wstring cached_token_file = get_user_cached_tokens_file();

	try {
		NX::json_value v = NX::json_value::create_object();
		v[TOKENLIST_ELEMENT_NAME] = NX::json_value::create_array();
		NX::json_array& tokens_array = v.as_object().at(TOKENLIST_ELEMENT_NAME).as_array();
		// Need a RW Locker here
		const std::map<decrypt_token_cache::token_key, std::vector<unsigned char>> & map = _token_cache.getTokenMap();
		time_t curtime = time(NULL);
		for (std::map<decrypt_token_cache::token_key, std::vector<unsigned char>>::const_iterator it = map.cbegin(); it != map.cend(); ++it) {
			// if token is expired, will not save it
			if (curtime <= (*it).first.get_ttl())
			{
				NX::json_value token_object = NX::json_value::create_object();
				token_object[TOKENID_ELEMENT_NAME] = NX::json_value(NX::conversion::to_wstring(it->first.get_id()));
				token_object[TOKENMAINTAINLEVEL_ELEMENT_NAME] = NX::json_value((int)it->first.get_level());
				token_object[TOKENKEY_ELEMENT_NAME] = NX::json_value(NX::conversion::to_wstring(it->second));
				token_object[TOKENOTP_ELEMENT_NAME] = NX::json_value(NX::conversion::to_wstring(it->first.get_otp()));
				token_object[TOKENTTL_ELEMENT_NAME] = NX::json_value(it->first.get_ttl());
				tokens_array.push_back(token_object);
			}
		}

		std::wstring ws = v.serialize();
		result = GLOBAL.nt_generate_file(cached_token_file, std::string(ws.begin(), ws.end()), true);
		_token_cache_dirty = false;

		//LOGDEBUG(NX::string_formater(L"%d cached tokens %s saved to config file %s", map.size(), result ? L"has been" : L"failed to be", cached_token_file.c_str()));
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
	}

	return result;
}

bool rmsession::load_policy_bundle_2()
{
	bool result = false;

    if (!is_logged_on()) {
        return result;
    }

	const std::wstring local_policy_file(_protected_profile_dir + L"\\" + POLICY_BUNDLE_FILE_2);
	std::string content;
	if (!GLOBAL.nt_load_file(local_policy_file, content)) {
		return result;
	}
	if (content.empty()) {
        NT::DeleteFile(local_policy_file.c_str());
		return result;
	}

    try {
		LOGDEBUG(NX::string_formater(L"load_policy_bundle_2:  %s", NX::conversion::utf8_to_utf16(content).c_str()));

		NX::json_value parameters = NX::json_value::parse(NX::conversion::utf8_to_utf16(content));
		std::map<std::wstring, std::wstring> policy_map;

		if (parameters.is_array()) {
			const NX::json_array& policy_array = parameters.as_array();
			for (NX::json_array::const_iterator it = policy_array.cbegin(); it != policy_array.cend(); ++it)
			{
				std::wstring tokenGroupName = it->as_object().at(L"tokenGroupName").as_string();
				std::wstring bundle = L"";
				if (it->as_object().has_field(L"policyBundle"))
				{
					bundle = it->as_object().at(L"policyBundle").as_string();
				}
				policy_map[tokenGroupName] = bundle;
				LOGDEBUG(NX::string_formater(L"load_policy_bundle_2: tokenGroupName: %s", tokenGroupName.c_str()));
			}
			_policy_map = policy_map;
		}

		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(local_policy_file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::load_tokengroup_resourcetype()
{
	bool result = false;

	if (!is_logged_on()) {
		return result;
	}

	const std::wstring local_policy_file(_protected_profile_dir + L"\\" + TOKENGROUP_RESOURCETYPE_FILE);
	std::string content;
	if (!GLOBAL.nt_load_file(local_policy_file, content)) {
		return result;
	}
	if (content.empty()) {
		NT::DeleteFile(local_policy_file.c_str());
		return result;
	}

	try {
		LOGDEBUG(NX::string_formater(L"load_tokengroup_resourcetype:  %s", NX::conversion::utf8_to_utf16(content).c_str()));

		NX::json_value parameters = NX::json_value::parse(NX::conversion::utf8_to_utf16(content));
		std::map<std::wstring, std::wstring> tokengroupRT_map;

		if (parameters.is_object()) {
			for (auto it = _policy_map.cbegin(); it != _policy_map.cend(); ++it)
			{
				std::wstring tokengroupName = it->first;
				if (parameters.as_object().has_field(tokengroupName))
				{
					std::wstring resourcetype = parameters.as_object().at(tokengroupName).as_string();
					tokengroupRT_map[tokengroupName] = resourcetype;
					LOGDEBUG(NX::string_formater(L"load_tokengroup_resourcetype: tokenGroupName: %s", tokengroupName.c_str()));
				}
			}
			_tokengroup_resoucetype_map = tokengroupRT_map;
		}

		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(local_policy_file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::load_policy_bundle()
{
	bool result = false;

    if (!is_logged_on()) {
        return result;
    }

    const std::wstring local_policy_file(_protected_profile_dir + L"\\" + POLICY_BUNDLE_FILE);
	std::string content;
	if (!GLOBAL.nt_load_file(local_policy_file, content)) {
		return result;
	}
	if (content.empty()) {
        NT::DeleteFile(local_policy_file.c_str());
		return result;
	}

    try {

        NX::json_value v = NX::json_value::parse(NX::conversion::utf8_to_utf16(content));
        _config_serial_numbers[NAME_POLICY_BUNDLE] = v.as_object().at(L"serialNumber").as_string();
		NX::utility::CRwSharedLocker locker(&_policy_bundle_lock);
		_policy_bundle = policy_bundle::parse(v.as_object().at(L"bundle").as_object());
        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        NT::DeleteFile(local_policy_file.c_str());
        result = false;
    }

	return result;
}

bool rmsession::load_locked_duids()
{
	bool result = false;

	if (!is_logged_on()) {
		return result;
	}

	NX::utility::CRwExclusiveLocker locker(&_lockedduids_lock);

	const std::wstring local_lockedduid_file(_protected_profile_dir + L"\\" + LOCKED_DUIDS_FILE);
	std::string content;
	if (!GLOBAL.nt_load_file(local_lockedduid_file, content)) {
		return result;
	}
	if (content.empty()) {
		NT::DeleteFile(local_lockedduid_file.c_str());
		return result;
	}

	try {
		LOGDEBUG(NX::string_formater(L"load_locked_duids:  %s", NX::conversion::utf8_to_utf16(content).c_str()));

		NX::json_value duids = NX::json_value::parse(NX::conversion::utf8_to_utf16(content));

		if (!duids.as_object().has_field(LOCKEDDUID_ELEMENT_NAME)) {
			return false;
		}
		const NX::json_value & parameters = duids.as_object().at(LOCKEDDUID_ELEMENT_NAME);
		std::map<std::wstring, std::wstring> duids_map;
		if (parameters.is_array()) {
			const NX::json_array& policy_array = parameters.as_array();
			for (NX::json_array::const_iterator it = policy_array.cbegin(); it != policy_array.cend(); ++it)
			{
				std::wstring duid = it->as_object().at(L"duid").as_string();
				std::wstring path = it->as_object().at(L"path").as_string();
				duids_map[duid] = path;
				LOGDEBUG(NX::string_formater(L"load_locked_duids: DUID %s, path %s", duid.c_str(), path.c_str()));
			}
			_locked_duid_map.clear();
			_locked_duid_map = duids_map;

			_locked_duid_dirty = false;
		}

		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(local_lockedduid_file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::save_locked_duids()
{
	bool result = true;

	if (!is_logged_on()) {
		return result;
	}

	NX::utility::CRwExclusiveLocker locker(&_lockedduids_lock);
	const std::wstring local_lockedduid_file(_protected_profile_dir + L"\\" + LOCKED_DUIDS_FILE);
	if (_locked_duid_dirty == false)
		return result;

	try {
		NX::json_value v = NX::json_value::create_object();
		v[LOCKEDDUID_ELEMENT_NAME] = NX::json_value::create_array();
		NX::json_array& duids_array = v.as_object().at(LOCKEDDUID_ELEMENT_NAME).as_array();
		for (std::map<std::wstring, std::wstring>::const_iterator it = _locked_duid_map.cbegin(); it != _locked_duid_map.cend(); ++it) {
			NX::json_value duid_pair = NX::json_value::create_object();
			duid_pair[L"duid"] = NX::json_value(it->first);
			duid_pair[L"path"] = NX::json_value(it->second);
			duids_array.push_back(duid_pair);
		}

		std::wstring ws = v.serialize();
		LOGDEBUG(NX::string_formater(L"save_locked_duids:  %s", ws.c_str()));
		result = GLOBAL.nt_generate_file(local_lockedduid_file, std::string(ws.begin(), ws.end()), true);
		_locked_duid_dirty = false;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		result = false;
	}

	return result;
}

bool rmsession::load_client_config()
{
    bool result = false;

    if (!is_logged_on()) {
        return result;
    }

    const std::wstring file(_protected_profile_dir + L"\\" + CLIENT_CONFIG_FILE);
    std::string content;
    if (!GLOBAL.nt_load_file(file, content)) {
        return result;
    }
    if (content.empty()) {
        return result;
    }

    try {
		LOGDEBUG(L"load_client_config: ");
        client_config config;
        config.deserialize(NX::conversion::utf8_to_utf16(content));
        if (config.empty()) {
            NT::DeleteFile(file.c_str());
        }
        else {
            _client_config = config;
            _config_serial_numbers[NAME_CLIENT_CONFIG] = config.get_serial_number();
            result = true;
			LOGDEBUG(NX::string_formater(L"load_client_config: clientid: %s", SERV->get_client_id().c_str()));
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        NT::DeleteFile(file.c_str());
        result = false;
    }

    return result;
}

bool rmsession::load_client_dir_config()
{
	bool result = false;

	const std::wstring file(_winsession->get_protected_profiles_dir() + L"\\" + CLIENT_DIR_CONFIG_FILE);
	std::string content;
	if (!GLOBAL.nt_load_file(file, content)) {
		LOGDEBUG(L"load_client_dir_config: failed");
		// return result;
	}
	if (content.empty()) {
		// return result;
	}

	try {
		LOGDEBUG(L"load_client_dir_config: ");
		client_dir_config config;
		config.deserialize(NX::conversion::utf8_to_utf16(content));
		
        std::wstring    markDir;
        const std::wstring rpmfile(_winsession->get_profiles_dir() + L"\\" + RPM_DIR_CONFIG_FILE);
        std::string rpmcontent;
        if (!GLOBAL.nt_load_file(rpmfile, rpmcontent)) {
            LOGDEBUG(L"load_rpm_dir_config: failed");
        }
        markDir = NX::conversion::utf8_to_utf16(rpmcontent);

		_client_dir_config = config;
		result = true;
        std::wstring dir = markDir; // _client_dir_config.get_mark_dir();
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		std::wstring    sanctuaryDir;
		const std::wstring sancfile(_winsession->get_profiles_dir() + L"\\" + SANC_DIR_CONFIG_FILE);
		std::string sanccontent;
		if (!GLOBAL.nt_load_file(sancfile, sanccontent)) {
			LOGDEBUG(L"load_rpm_dir_config: failed");
		}
		sanctuaryDir = NX::conversion::utf8_to_utf16(sanccontent);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		std::wstring router = _client_dir_config.get_router();
		std::wstring tenant = _client_dir_config.get_tenant_id();

		SERV->parse_dirs(markDir);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		SERV->parse_sanctuary_dirs(sanctuaryDir);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

		if (_client_dir_config._router != L"" && SERV->is_central_config() == false)
			SERV->get_router_config().set_router(_client_dir_config._router);
		if (_client_dir_config._tenant != L"" && SERV->is_central_config() == false)
			SERV->get_router_config().set_tenant_id(_client_dir_config._tenant);

		if (markDir.size() > 0)
			SERV->InitMarkDirecory();
		LOGDEBUG(NX::string_formater(L"load_client_dir_config: markDir: %s", dir.c_str()));
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		if (sanctuaryDir.size() > 0)
			SERV->InitSanctuaryDirecory();
		LOGDEBUG(NX::string_formater(L"load_client_dir_config: sanctuaryDir: %s", sanctuaryDir.c_str()));
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::load_app_whitelist_config()
{
	unsigned long session_id = _winsession->get_session_id();
	bool result = false;

	const std::wstring file(_winsession->get_profiles_dir() + L"\\" + APP_WHITELIST_CONFIG_FILE);
	std::string content;
	if (!GLOBAL.nt_load_file(file, content)) {
		LOGDEBUG(L"load_app_whitelist_config: failed");
		SERV->set_trusted_apps_from_config(session_id, std::map<std::wstring, std::vector<unsigned char>>());
		return result;
	}
	if (content.empty()) {
		SERV->set_trusted_apps_from_config(session_id, std::map<std::wstring, std::vector<unsigned char>>());
		return result;
	}

	try {
		LOGDEBUG(L"load_app_whitelist_config: ");
		app_whitelist_config config;
		config.deserialize(NX::conversion::utf8_to_utf16(content));

		_app_whitelist_config = config;
		result = true;

		const std::set<std::wstring>& registeredApps = _app_whitelist_config.get_registered_apps();
		SERV->set_registered_apps(registeredApps);
		LOGDEBUG(NX::string_formater(L"load_app_whitelist_config: registeredApps:"));
		for (const auto& app : registeredApps) {
			LOGDEBUG(NX::string_formater(L"\t%s", app.c_str()));
		}

		const std::map<std::wstring, std::vector<unsigned char>>& trustedApps = _app_whitelist_config.get_trusted_apps();
		SERV->set_trusted_apps_from_config(session_id, trustedApps);
		LOGDEBUG(NX::string_formater(L"load_app_whitelist_config: trustedApps:"));
		for (const auto& app : trustedApps) {
			LOGDEBUG(NX::string_formater(L"\t%s", app.first.c_str()));
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::load_local_config()
{
    bool result = false;

    if (!is_logged_on()) {
        return false;
    }

    const std::wstring file(_protected_profile_dir + L"\\" + CLIENT_LOCAL_CONFIG_FILE);
    std::string content;
    if (!GLOBAL.nt_load_file(file, content)) {
        return false;
    }
    if (content.empty()) {
        return false;
    }

    try {

        client_local_config config;
        config.deserialize(NX::conversion::utf8_to_utf16(content));
        _local_config = config;
        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool rmsession::load_classify_config()
{
    bool result = false;

    if (!is_logged_on()) {
        return result;
    }

    const std::wstring file(_protected_profile_dir + L"\\" + CLASSIFY_CONFIG_FILE);
    std::string content;
    if (!GLOBAL.nt_load_file(file, content)) {
        return result;
    }
    if (content.empty()) {
        NT::DeleteFile(file.c_str());
        return result;
    }

    try {

        classify_config config;
        config.deserialize(NX::conversion::utf8_to_utf16(content));
        if (config.empty()) {
            NT::DeleteFile(file.c_str());
        }
        else {
            _classify_config = config;
            _config_serial_numbers[NAME_CLASSIFY_CONFIG] = config.get_serial_number();
            result = true;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        NT::DeleteFile(file.c_str());
        result = false;
    }

    return result;
}

bool rmsession::load_watermark_config()
{
    bool result = false;

    if (!is_logged_on()) {
        return result;
    }

    const std::wstring file(_protected_profile_dir + L"\\" + WATERMARK_CONFIG_FILE);
    std::string content;
    if (!GLOBAL.nt_load_file(file, content)) {
        return result;
    }
    if (content.empty()) {
        NT::DeleteFile(file.c_str());
        return result;
    }


    try {

        watermark_config config;
        config.deserialize(NX::conversion::utf8_to_utf16(content));
        if (config.empty()) {
            NT::DeleteFile(file.c_str());
        }
        else {
            _watermark_config = config;
            _config_serial_numbers[NAME_WATERMARK_CONFIG] = config.get_serial_number();
            result = true;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        NT::DeleteFile(file.c_str());
        result = false;
    }

    return result;
}

//bool rmsession::get_policybundle(const std::wstring& tokenGroupName, std::string& policyBundle)
//{
//	if (_policy_map.find(tokenGroupName) != _policy_map.end())
//	{
//		policyBundle = NX::conversion::utf16_to_utf8(_policy_map[tokenGroupName]);
//		return true;
//	}
//	return false;
//}

bool rmsession::update_policy(std::map<std::wstring, std::wstring>& policy_map, std::wstring& policyData)
{
	bool result = false;
	_policy_map = policy_map;
	const std::wstring client_config_file(_protected_profile_dir + L"\\" + POLICY_BUNDLE_FILE_2);
	
	result = GLOBAL.nt_generate_file(client_config_file, NX::conversion::utf16_to_utf8(policyData), true);
	LOGDEBUG(NX::string_formater(L"update_policy: result= %d, %s", result, policyData.c_str()));

	return result;
}

bool rmsession::update_policy(const std::wstring& serial_number, const std::wstring& content)
{
    bool result = false;

    try {
		LOGDEBUG(NX::string_formater("update_ policy:  serial: (%s), content: (%s)", serial_number.c_str(), content.c_str() ));
        // parse content
        NX::json_value body = NX::json_value::parse(content);
        // build policy object
        std::shared_ptr<policy_bundle> sp = policy_bundle::parse(body.as_object());
        if (sp != nullptr) {
            reset_policy_bundle(sp);
            result = true;
            // save it
            _config_serial_numbers[NAME_POLICY_BUNDLE] = serial_number;
            NX::json_value bundle = NX::json_value::create_object();
            bundle[L"serialNumber"] = NX::json_value(serial_number);
            bundle[L"bundle"] = body;
            const std::wstring client_config_file(_protected_profile_dir + L"\\" + POLICY_BUNDLE_FILE);
            const std::wstring& s = bundle.serialize();
            GLOBAL.nt_generate_file(client_config_file, NX::conversion::utf16_to_utf8(s), true);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool rmsession::get_tokengroup_resourcetype(const std::wstring& tokenGroupName, std::string& resourcetype)
{
	if (_tokengroup_resoucetype_map.find(tokenGroupName) != _tokengroup_resoucetype_map.end())
	{
		resourcetype = NX::conversion::utf16_to_utf8(_tokengroup_resoucetype_map[tokenGroupName]);
		return true;
	}
	return false;
}

bool rmsession::update_tokengroup_resourcetype(std::map<std::wstring, std::wstring>& tokengroup_resoucetype_map, std::wstring& tokengroup_resoucetype_data)
{
	bool result = false;
	_tokengroup_resoucetype_map = tokengroup_resoucetype_map;
	const std::wstring client_config_file(_protected_profile_dir + L"\\" + TOKENGROUP_RESOURCETYPE_FILE);

	result = GLOBAL.nt_generate_file(client_config_file, NX::conversion::utf16_to_utf8(tokengroup_resoucetype_data), true);
	LOGDEBUG(NX::string_formater(L"update_tokengroup_resourcetype: result= %d, %s", result, tokengroup_resoucetype_data.c_str()));

	return result;
}

bool rmsession::update_pdp_url(std::wstring &icenetUrl)
{
	try {
		NX::xml_document doc;
		std::wstring pdpconfig = SERV->get_pdp_dir() + L"\\config\\commprofile.xml";
		doc.load_from_file(pdpconfig);
		std::shared_ptr<NX::xml_node> spRoot = doc.document_root();
		if (spRoot != nullptr)
		{
			std::shared_ptr<NX::xml_node> dabsLocation = spRoot->find_child_element(L"DABSLocation");
			if (dabsLocation)
			{
				std::wstring url = dabsLocation->get_attribute(L"value");
				if (url.compare(icenetUrl) == 0)
				{
					return true;
				}

				dabsLocation->set_attribute(L"value", icenetUrl);
				doc.to_file(pdpconfig);

				// delete some pdp login file
				std::wstring bundle = SERV->get_pdp_dir() + L"\\bundle.bin";
				std::wstring regist = SERV->get_pdp_dir() + L"\\config\\registration.info";
				std::wstring agent_key = SERV->get_pdp_dir() + L"\\config\\security\\agent-keystore.p12";
				std::wstring agent_secret = SERV->get_pdp_dir() + L"\\config\\security\\agent-secret-keystore.p12";
				std::wstring agent_trust = SERV->get_pdp_dir() + L"\\config\\security\\agent-truststore.p12";
				NX::fs::delete_file(bundle, true);
				NX::fs::delete_file(regist, true);
				NX::fs::delete_file(agent_key, true);
				NX::fs::delete_file(agent_secret, true);
				NX::fs::delete_file(agent_trust, true);
			}
		}

	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}
	return true;
}

bool rmsession::update_dap_configfile(std::wstring &rms_url)
{
	try {
		bool result = false;
		CProperties cprop;
		std::wstring dapconfig = SERV->get_pdp_dir() + L"\\jservice\\config\\DynamicAttributeProviderClient.properties";

		result = cprop.open(NX::conversion::utf16_to_utf8(dapconfig).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"open dap_properties file: result= %d, %s", result, dapconfig.c_str()));
			return result;
		}
		vector<string> vec = cprop.read("DAP_SERVER_URL");
		if (vec.size()>0 && vec[0].compare(NX::conversion::utf16_to_utf8(rms_url)) == 0)
		{
			cprop.close();
			return true;
		}

		result = cprop.modify("DAP_SERVER_URL", NX::conversion::utf16_to_utf8(rms_url).c_str());
		if(!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file DAP_SERVER_URL value: result= %d, %s", result, rms_url.c_str()));
		}

		NX::uri url(rms_url);

		result = cprop.modify("dapHostName", NX::conversion::utf16_to_utf8(url.host()).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file dapHostName value: result= %d, %s", result, url.host().c_str()));
		}
		std::wstring port = L"443";
		if (url.port() > 0)
		{
			port = std::to_wstring(url.port());
		}
		result = cprop.modify("dapHostPort", NX::conversion::utf16_to_utf8(port).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file dapHostPort value: result= %d, %s", result, std::to_wstring(url.port()).c_str()));
		}
		result = cprop.modify("sslProtocol", NX::conversion::utf16_to_utf8(url.scheme()).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file sslProtocol value: result= %d, %s", result, url.scheme().c_str()));
		}
		std::wstring jarpath= SERV->get_pdp_dir() + L"\\jservice\\jar\\SKYDRMDynamicAttributeProvider.jar";
		boost::algorithm::replace_all(jarpath, L"\\", L"/");
		result = cprop.modify("jar-path", NX::conversion::utf16_to_utf8(jarpath).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file jar-path value: result= %d, %s", result, jarpath.c_str()));
		}

		std::wstring trustpath = SERV->get_pdp_dir() + L"\\jservice\\config\\suap-truststore.jks";
		try {
			NX::win::reg_local_machine rgk;
			rgk.open(L"SOFTWARE\\NextLabs\\SkyDRM\\DAP", NX::win::reg_key::reg_wow64_64, true);
			rgk.read_value(L"trust_store", trustpath);
		}
		catch (std::exception& e) {
			UNREFERENCED_PARAMETER(e);
			trustpath = SERV->get_pdp_dir() + L"\\jservice\\config\\suap-truststore.jks";
		}

		boost::algorithm::replace_all(trustpath, L"\\", L"/");
		result = cprop.modify("trust_store", NX::conversion::utf16_to_utf8(trustpath).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file trust_store value: result= %d, %s", result, trustpath.c_str()));
		}

		cprop.close();

		return result;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}
}

bool rmsession::update_dap_configfile2(std::wstring &rms_url)
{
	try {
		bool result = false;
		CProperties cprop;
		std::wstring dapconfig = SERV->get_pdp_dir() + L"\\jservice\\config\\DynamicAttributeService.properties";

		result = cprop.open(NX::conversion::utf16_to_utf8(dapconfig).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"open dap_properties file: result= %d, %s", result, dapconfig.c_str()));
			return result;
		}
		//vector<string> vec = cprop.read("DAP_SERVER_URL");
		//if (vec.size() > 0 && vec[0].compare(NX::conversion::utf16_to_utf8(rms_url)) == 0)
		//{
		//	cprop.close();
		//	return true;
		//}

		//result = cprop.modify("DAP_SERVER_URL", NX::conversion::utf16_to_utf8(rms_url).c_str());
		//if (!result)
		//{
		//	LOGDEBUG(NX::string_formater(L"update dap_properties file DAP_SERVER_URL value: result= %d, %s", result, rms_url.c_str()));
		//}

		NX::uri url(rms_url);

		result = cprop.modify("dapHostName", NX::conversion::utf16_to_utf8(url.host()).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file dapHostName value: result= %d, %s", result, url.host().c_str()));
		}
		std::wstring port = L"443";
		if (url.port() > 0)
		{
			port = std::to_wstring(url.port());
		}
		result = cprop.modify("dapHostPort", NX::conversion::utf16_to_utf8(port).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file dapHostPort value: result= %d, %s", result, std::to_wstring(url.port()).c_str()));
		}
		result = cprop.modify("sslProtocol", NX::conversion::utf16_to_utf8(url.scheme()).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file sslProtocol value: result= %d, %s", result, url.scheme().c_str()));
		}
		std::wstring jarpath = SERV->get_pdp_dir() + L"\\jservice\\jar\\DynamicAttributeService.jar";
		boost::algorithm::replace_all(jarpath, L"\\", L"/");
		result = cprop.modify("jar-path", NX::conversion::utf16_to_utf8(jarpath).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file jar-path value: result= %d, %s", result, jarpath.c_str()));
		}

		std::wstring trustpath = SERV->get_pdp_dir() + L"\\jservice\\config\\suap-truststore.jks";
		try {
			NX::win::reg_local_machine rgk;
			rgk.open(L"SOFTWARE\\NextLabs\\SkyDRM\\DAP", NX::win::reg_key::reg_wow64_64, true);
			rgk.read_value(L"trust_store", trustpath);
		}
		catch (std::exception& e) {
			UNREFERENCED_PARAMETER(e);
			trustpath = SERV->get_pdp_dir() + L"\\jservice\\config\\suap-truststore.jks";
		}

		boost::algorithm::replace_all(trustpath, L"\\", L"/");
		result = cprop.modify("trust_store", NX::conversion::utf16_to_utf8(trustpath).c_str());
		if (!result)
		{
			LOGDEBUG(NX::string_formater(L"update dap_properties file trust_store value: result= %d, %s", result, trustpath.c_str()));
		}

		cprop.close();

		return result;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}
}


bool rmsession::change_dap_filename(bool dap_enable)
{
	bool result = true;
	// If DAPSERVER_ENABLED is true than the property file carries the right extension DynamicAttributeProviderClient.propertie
   // Is false than the property file carries the wrong extension DynamicAttributeProviderClient.bak
	std::wstring pro_file = SERV->get_pdp_dir() + L"\\jservice\\jar\\DynamicAttributeProviderClient.properties";
	std::wstring bak_file = SERV->get_pdp_dir() + L"\\jservice\\jar\\DynamicAttributeProviderClient.bak";
	if (dap_enable)
	{
		if (PathFileExists(bak_file.c_str()))
		{
			int rest = rename(NX::conversion::utf16_to_utf8(bak_file).c_str(), NX::conversion::utf16_to_utf8(pro_file).c_str());
			if (rest != 0)
			{
				LOGERROR(NX::string_formater(L"rename file name %s to %s failed", bak_file.c_str(), pro_file.c_str()));
				result = false;
			}
		}
	}
	else
	{
		if (PathFileExists(pro_file.c_str()))
		{
			int rest = rename(NX::conversion::utf16_to_utf8(pro_file).c_str(), NX::conversion::utf16_to_utf8(bak_file).c_str());
			if (rest != 0)
			{
				LOGERROR(NX::string_formater(L"rename file name %s to %s failed", pro_file.c_str(), bak_file.c_str()));
				result = false;
			}
		}
	}
	return result;
}

bool rmsession::update_client_config(const std::wstring& clientid)
{
	bool result = false;

	const std::wstring file(_protected_profile_dir + L"\\" + CLIENT_CONFIG_FILE);
	std::string content;
	if (!GLOBAL.nt_load_file(file, content)) {
		LOGERROR(NX::string_formater(L"update_client_config: load file failed, file: %s", file.c_str()));
		return result;
	}
	if (content.empty()) {
		return result;
	}

	LOGDEBUG(NX::string_formater(L"update_client_config: clientid: %s", clientid.c_str()));
	try {		
		client_config config;
		config.deserialize(NX::conversion::utf8_to_utf16(content));  // will overwrite _client_id
		if (config.empty()) {
			NT::DeleteFile(file.c_str());
		}
		else {
			config._clientId = clientid;
			//SERV->set_client_id(clientid);  // reset _client_id
			_client_config = config;			
			const std::wstring& s = config.serialize();
			GLOBAL.nt_generate_file(file, NX::conversion::utf16_to_utf8(s), true);
			result = true;			
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(file.c_str());
		result = false;
	}

	return result;
}

int rmsession::get_client_dir_info(std::wstring& router, std::wstring& tenant, std::wstring& workingfolder, std::wstring& tempfolder, std::wstring& sdklibfolder)
{
	int result = ERROR_SUCCESS;
	const std::wstring file(_winsession->get_protected_profiles_dir() + L"\\" + CLIENT_DIR_CONFIG_FILE);
	std::string content;
	client_dir_config config;

	LOGDEBUG(NX::string_formater(L"get_client_dir_info: "));
	if (!GLOBAL.nt_load_file(file, content)) {
		LOGDEBUG(L"load_client_dir_config: failed");
		return result;
	}
	if (content.empty()) {
		return result;
	}

	try {

		config.deserialize(NX::conversion::utf8_to_utf16(content));
		router = config.get_router();
		tenant = config.get_tenant_id();
		workingfolder = config.get_workingfolder();
		tempfolder = config.get_tempfolder();
		sdklibfolder = config.get_sdklib_dir();
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(file.c_str());
		result = 1;
	}
	return result;
}

bool rmsession::update_client_dir_config(const std::wstring& filepath, int no)
{
	bool result = false;

	const std::wstring file(_winsession->get_protected_profiles_dir() + L"\\" + CLIENT_DIR_CONFIG_FILE);
	std::string content;
	client_dir_config config;

	if (no == _LOGIN_INFO::RPMFolder
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
        || no == _LOGIN_INFO::sanctuaryFolder
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
        )
	{
		// for RPM folder changes, we will sync to registry also
		SERV->RefreshDirectory();
	}

	if (!GLOBAL.nt_load_file(file, content)) 
	{
		switch (no)
		{
		case _LOGIN_INFO::RPMFolder:
			// config._markDir = filepath;
            break;
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		case _LOGIN_INFO::sanctuaryFolder:
			//config._sanctuaryDir = filepath;
			break;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		case _LOGIN_INFO::sdklibFolder:
			config._sdklibDir = filepath;
			break;
		case _LOGIN_INFO::routerAddr:
			config._router = filepath;
			break;
		case _LOGIN_INFO::tenantId:
			config._tenant = filepath;
			break;
		case _LOGIN_INFO::workingFolder:
			config._workingfolder = filepath;
			break;
		case _LOGIN_INFO::tempFolder:
			config._tempfolder = filepath;
			break;
        default:
			break;
		}

        if (no == _LOGIN_INFO::RPMFolder)
        {
            const std::wstring rpmfile(_winsession->get_profiles_dir() + L"\\" + RPM_DIR_CONFIG_FILE);
            std::string rpmcontent;

            rpmcontent = NX::conversion::utf16_to_utf8(filepath);
            result = GLOBAL.nt_generate_file(rpmfile, rpmcontent, true);
            LOGDEBUG(NX::string_formater(L"update_client_dir_config: %d, filepath: %s", no, filepath.c_str()));
        }
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		else if (no == _LOGIN_INFO::sanctuaryFolder)
		{
			const std::wstring sancfile(_winsession->get_profiles_dir() + L"\\" + SANC_DIR_CONFIG_FILE);
			std::string sanccontent;

			sanccontent = NX::conversion::utf16_to_utf8(filepath);
			result = GLOBAL.nt_generate_file(sancfile, sanccontent, true);
			LOGDEBUG(NX::string_formater(L"update_client_dir_config: %d, filepath: %s", no, filepath.c_str()));
		} 
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		else
        {
            _client_dir_config = config;
            const std::wstring& s = config.serialize();
            result = GLOBAL.nt_generate_file(file, NX::conversion::utf16_to_utf8(s), true);
            LOGDEBUG(NX::string_formater(L"update_client_dir_config: %d, filepath: %s", no, filepath.c_str()));
        }

		return result;
	}
	if (content.empty()) {
		return result;
	}

	LOGDEBUG(NX::string_formater(L"update_client_dir_config: dir: %s", filepath.c_str()));
	try {
		
		config.deserialize(NX::conversion::utf8_to_utf16(content));		

		switch (no)
		{
		case _LOGIN_INFO::RPMFolder:
			//config._markDir = filepath;
			break;
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		case _LOGIN_INFO::sanctuaryFolder:
			//config._sanctuaryDir = filepath;
			break;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		case _LOGIN_INFO::sdklibFolder:
			config._sdklibDir = filepath;
			break;
		case _LOGIN_INFO::routerAddr:
			config._router = filepath;
			break;
		case _LOGIN_INFO::tenantId:
			config._tenant = filepath;
			break;
		case _LOGIN_INFO::workingFolder:
			config._workingfolder = filepath;
			break;
		case _LOGIN_INFO::tempFolder:
			config._tempfolder = filepath;
			break;
		default:
			break;
		}
		
        if (no == _LOGIN_INFO::RPMFolder)
        {
            const std::wstring rpmfile(_winsession->get_profiles_dir() + L"\\" + RPM_DIR_CONFIG_FILE);
            std::string rpmcontent;

            rpmcontent = NX::conversion::utf16_to_utf8(filepath);
            result = GLOBAL.nt_generate_file(rpmfile, rpmcontent, true);
            LOGDEBUG(NX::string_formater(L"update_client_dir_config: %d, filepath: %s", no, filepath.c_str()));
        }
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		else if (no == _LOGIN_INFO::sanctuaryFolder)
		{
			const std::wstring sancfile(_winsession->get_profiles_dir() + L"\\" + SANC_DIR_CONFIG_FILE);
			std::string sanccontent;

			sanccontent = NX::conversion::utf16_to_utf8(filepath);
			result = GLOBAL.nt_generate_file(sancfile, sanccontent, true);
			LOGDEBUG(NX::string_formater(L"update_client_dir_config: %d, filepath: %s", no, filepath.c_str()));
		}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		else
        {
            _client_dir_config = config;
            const std::wstring& s = config.serialize();
            result = GLOBAL.nt_generate_file(file, NX::conversion::utf16_to_utf8(s), true);
            LOGDEBUG(NX::string_formater(L"update_client_dir_config: %d, filepath: %s", no, filepath.c_str()));
        }
		
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::update_app_whitelist_config(const std::set<std::wstring>& registeredApps)
{
	bool result = false;

	const std::wstring file(_winsession->get_profiles_dir() + L"\\" + APP_WHITELIST_CONFIG_FILE);
	std::string content;
	app_whitelist_config config;

	if (!GLOBAL.nt_load_file(file, content))
	{
		config._registeredApps = registeredApps;

		_app_whitelist_config = config;
		const std::wstring& s = config.serialize();
		result = GLOBAL.nt_generate_file(file, NX::conversion::utf16_to_utf8(s), true);
		LOGDEBUG(NX::string_formater(L"update_app_whitelist_config: registeredApps:"));
		for (const auto& app : registeredApps) {
			LOGDEBUG(NX::string_formater(L"\t%s", app.c_str()));
		}

		return result;
	}
	if (content.empty()) {
		return result;
	}

	LOGDEBUG(NX::string_formater(L"update_app_whitelist_config: registeredApps:"));
	for (const auto& app : registeredApps) {
		LOGDEBUG(NX::string_formater(L"\t%s", app.c_str()));
	}
	try {
		config.deserialize(NX::conversion::utf8_to_utf16(content));
		config._registeredApps = registeredApps;

		_app_whitelist_config = config;
		const std::wstring& s = config.serialize();
		GLOBAL.nt_generate_file(file, NX::conversion::utf16_to_utf8(s), true);
		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::update_app_whitelist_config(const std::map<std::wstring, std::vector<unsigned char>>& trustedApps)
{
	bool result = false;

	const std::wstring file(_winsession->get_profiles_dir() + L"\\" + APP_WHITELIST_CONFIG_FILE);
	std::string content;
	app_whitelist_config config;

	if (!GLOBAL.nt_load_file(file, content))
	{
		config._trustedApps = trustedApps;

		_app_whitelist_config = config;
		const std::wstring& s = config.serialize();
		result = GLOBAL.nt_generate_file(file, NX::conversion::utf16_to_utf8(s), true);
		LOGDEBUG(NX::string_formater(L"update_app_whitelist_config: trustedApps:"));
		for (const auto& app : trustedApps) {
			LOGDEBUG(NX::string_formater(L"\t%s", app.first.c_str()));
		}

		return result;
	}
	if (content.empty()) {
		return result;
	}

	LOGDEBUG(NX::string_formater(L"update_app_whitelist_config: trustedApps:"));
	for (const auto& app : trustedApps) {
		LOGDEBUG(NX::string_formater(L"\t%s", app.first.c_str()));
	}
	try {
		config.deserialize(NX::conversion::utf8_to_utf16(content));
		config._trustedApps = trustedApps;

		_app_whitelist_config = config;
		const std::wstring& s = config.serialize();
		GLOBAL.nt_generate_file(file, NX::conversion::utf16_to_utf8(s), true);
		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		NT::DeleteFile(file.c_str());
		result = false;
	}

	return result;
}

bool rmsession::update_client_config(const std::wstring& serial_number, const std::wstring& content)
{
    bool result = false;

    try {

        // parse content
        NX::json_value body = NX::json_value::parse(content);
        // build client config object
        client_config config;

        config._serial_number = serial_number;
        config._heartbeat_interval = body.as_object().at(L"heartbeatInterval").as_uint32();
        config._log_interval = body.as_object().at(L"logInterval").as_uint32();
        config._bypass_filter = body.as_object().at(L"bypassFilter").as_string();
        config._protection_enabled = body.as_object().at(L"protection").as_object().at(L"enabled").as_boolean();
        if (config._protection_enabled) {
            if (body.as_object().at(L"protection").as_object().has_field(L"filetypeFilter")) {
                config._protection_filter = body.as_object().at(L"protection").as_object().at(L"filetypeFilter").as_string();
            }
			else {
				config._protection_filter = L"[^.]";
			}
        }
		else {
			config._protection_filter = L"[^.]";
		}
		config._clientId = SERV->get_client_id();

        // update runtime config
        _client_config = config;
        result = true;
        // save it
        const std::wstring client_config_file(_protected_profile_dir + L"\\" + CLIENT_CONFIG_FILE);
        const std::wstring& s = config.serialize();
        GLOBAL.nt_generate_file(client_config_file, NX::conversion::utf16_to_utf8(s), true);
        _config_serial_numbers[NAME_CLIENT_CONFIG] = serial_number;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool rmsession::update_classify_config(const std::wstring& serial_number, const std::wstring& content)
{
    bool result = false;

    try {

        // parse content
        // build re-serialize XML data
        NX::xml_document doc;
        doc.load_from_string(content);
        const std::wstring& sxml = doc.serialize();

        classify_config config(serial_number, sxml);

        // save it
        _config_serial_numbers[NAME_CLASSIFY_CONFIG] = serial_number;
        const std::wstring config_file(_protected_profile_dir + L"\\" + CLASSIFY_CONFIG_FILE);
        const std::wstring& s = config.serialize();
        GLOBAL.nt_generate_file(config_file, NX::conversion::utf16_to_utf8(s), true);
        const std::wstring xml_file(_temp_profile_dir + L"\\classify.xml");
        GLOBAL.generate_file(xml_file, NX::conversion::utf16_to_utf8(sxml), true);
        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool rmsession::update_watermark_config(const std::wstring& serial_number, const std::wstring& content)
{
    bool result = false;

    try {

        // parse content
        NX::json_value body = NX::json_value::parse(content);
        // build client config object
        watermark_config config;

        config._serial_number = serial_number;
        config._message = body.as_object().at(L"text").as_string();
        if (body.as_object().has_field(L"transparentRatio")) {
            config._transparency_ratio = 100 - (body.as_object().at(L"transparentRatio").as_int32() % 100);
        }
        else {
            config._transparency_ratio = 30;
        }
        if (body.as_object().has_field(L"fontName")) {
            config._font_name = body.as_object().at(L"fontName").as_string();
        }
        else {
            config._font_name = L"Sitka Text";
        }
        if (body.as_object().has_field(L"fontSize")) {
            config._font_size = body.as_object().at(L"fontSize").as_int32();
        }
        else {
            config._font_size = 36;
        }
        if (body.as_object().has_field(L"fontColor")) {
            const std::wstring colorName = body.as_object().at(L"fontColor").as_string();
            if (colorName.length() == 7 && L'#' == colorName[0]) {
                const std::vector<unsigned char>& rgb = NX::utility::hex_string_to_buffer<wchar_t>(colorName.c_str() + 1);
                if (rgb.size() == 3) {
                    config._font_color = RGB(rgb[0], rgb[1], rgb[2]);
                }
                else {
                    config._font_color = RGB(0, 128, 0);
                }
            }
            else {
                config._font_color = RGB(0, 128, 0);
                LOGDEBUG(NX::string_formater(L"Bad color in water mark config update (%s)", colorName.c_str()));
            }
        }
        else {
            config._font_color = RGB(0, 128, 0);
        }
        if (body.as_object().has_field(L"rotation")) {
            const std::wstring& rotationName = body.as_object().at(L"rotation").as_string();
            config._rotation = (0 == _wcsicmp(rotationName.c_str(), L"Anticlockwise")) ? _ANTICLOCKWISE : _CLOCKWISE;
        }
        else {
            config._rotation = _ANTICLOCKWISE;
        }

        // update runtime config
        _watermark_config = config;
        result = true;
        // save it
        _config_serial_numbers[NAME_WATERMARK_CONFIG] = serial_number;
        const std::wstring client_config_file(_protected_profile_dir + L"\\" + WATERMARK_CONFIG_FILE);
        const std::wstring& s = config.serialize();
        GLOBAL.nt_generate_file(client_config_file, NX::conversion::utf16_to_utf8(s), true);

        // generate watermark image
        const std::wstring watermark_file(_temp_profile_dir + L"\\watermark.png");
        if (!generate_watermark_image()) {
            LOGERROR(NX::string_formater(L"Fail to generate watermark image: %s", watermark_file.c_str()));
        }
        else {
            LOGDEBUG(NX::string_formater(L"Generate watermark image: %s", watermark_file.c_str()));
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        LOGERROR(NX::string_formater("Fail to parse water mark config: %s", e.what()));
        result = false;
    }

    return result;
}

//
//  class rmappinstance
//

// ERROR_MAX_SESSIONS_REACHED
rmappinstance::rmappinstance() : _process_handle(NULL), _process_id(0)
{
}

rmappinstance::rmappinstance(const std::wstring& app_path) : _process_handle(NULL), _process_id(0), _image_path(app_path)
{
}

rmappinstance::~rmappinstance()
{
    quit();
}

HWND rmappinstance::find_target_hwnd(bool wait, unsigned long timeout)
{
    if (_process_id == 0) {
        return NULL;
    }
    return NULL;
}

void rmappinstance::run(unsigned long session_id)
{
    HANDLE          tk = NULL;
    STARTUPINFOW    si;
    PROCESS_INFORMATION pi;
    DWORD dwError = 0;

    if (is_running()) {
        return;
    }

    assert(session_id != 0);
    if (session_id == 0) {
        throw NX::exception(WIN32_ERROR_MSG(ERROR_NO_SUCH_LOGON_SESSION, "fail to get session token"), ERROR_NO_SUCH_LOGON_SESSION);
    }

    _port_name = L"nxrmtray_";
    _port_name += NX::conversion::to_wstring((int)session_id);

    if (session_id != -1) {

        if (!WTSQueryUserToken(session_id, &tk)) {
            throw NX::exception(WIN32_ERROR_MSG(GetLastError(), "fail to get session token"), ERROR_NO_SUCH_LOGON_SESSION);
        }
        assert(NULL != tk);
        if (NULL == tk) {
            throw NX::exception(WIN32_ERROR_MSG(ERROR_NO_SUCH_LOGON_SESSION, "fail to get session token"), ERROR_NO_SUCH_LOGON_SESSION);
        }
    }

	// Duplicate token
	HANDLE hTokenDup = NULL;
	if (!::DuplicateTokenEx(tk, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hTokenDup))
	{
		dwError = GetLastError();
		CloseHandle(tk);
		throw NX::exception(WIN32_ERROR_MSG(ERROR_INVALID_CLUSTER_IPV6_ADDRESS, "Fail to duplicate token"));
	}

	// Create env variables info
	LPVOID pEnv = NULL;
	if (!::CreateEnvironmentBlock(&pEnv, hTokenDup, FALSE))
	{
		dwError = GetLastError();
		::DestroyEnvironmentBlock(pEnv);
		CloseHandle(tk);
		CloseHandle(hTokenDup);

		throw NX::exception(WIN32_ERROR_MSG(ERROR_INVALID_CLUSTER_IPV6_ADDRESS, "Fail to create environment block"));
	}

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    si.lpDesktop = L"WinSta0\\Default";
    if (!::CreateProcessAsUserW(tk, _image_path.c_str(), NULL, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, pEnv, NULL, &si, &pi)) {
        dwError = GetLastError();
		::DestroyEnvironmentBlock(pEnv);
        CloseHandle(tk);
        throw NX::exception(WIN32_ERROR_MSG(ERROR_INVALID_CLUSTER_IPV6_ADDRESS, "fail to create process"));
    }

    _process_handle = pi.hProcess;
    _process_id = pi.dwProcessId;
	::DestroyEnvironmentBlock(pEnv);
    CloseHandle(pi.hThread);
    CloseHandle(tk);
}

void rmappinstance::quit(unsigned long wait_time)
{
    if (is_running()) {
        kill();
    }
}

void rmappinstance::kill()
{
    if (NULL != _process_handle) {
        TerminateProcess(_process_handle, 0);
        CloseHandle(_process_handle);
        _process_handle = NULL;
        _process_id = 0;
    }
}

void rmappinstance::attach(unsigned long process_id)
{
    _process_handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, process_id);
    if (NULL != _process_handle) {
        _process_id = process_id;
    }
}

void rmappinstance::detach()
{
    if (NULL != _process_handle) {
        CloseHandle(_process_handle);
        _process_handle = NULL;
        _process_id = 0;
    }
}


//
//  class rmappmanager
//
rmappmanager::rmappmanager() : _shutdown_event(NULL)
{
    _shutdown_event = ::CreateEventW(NULL, FALSE, FALSE, NULL);
}

rmappmanager::~rmappmanager()
{
    shutdown();

    if (NULL != _shutdown_event) {
        CloseHandle(_shutdown_event);
        _shutdown_event = NULL;
    }
}

void rmappmanager::start(unsigned long session_id, const std::wstring& app_path)
{
    _session_id = session_id;
    _instance = std::shared_ptr<rmappinstance>(new rmappinstance(app_path));
    _daemon_thread = std::thread(rmappmanager::daemon, this);
}

void rmappmanager::shutdown()
{
    if (_daemon_thread.joinable()) {
        SetEvent(_shutdown_event);
        _daemon_thread.join();
    }
}

bool rmappmanager::notify(const std::string& message) 
{
    if (_instance == nullptr) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }
    if (message.empty()) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    if (_instance->get_port_name().empty()) {
        SetLastError(ERROR_INVALID_PORT_ATTRIBUTES);
        return false;
    }

    NX::async_pipe::client pipe_client(4096);

    if (!pipe_client.connect(_instance->get_port_name(), 5000)) {
        LOGWARNING(NX::string_formater(L"Fail to connect to pipe server (%d)", GetLastError()));
        return false;
    }

    const std::vector<unsigned char> data(message.c_str(), message.c_str() + message.length());
    bool result = pipe_client.write(data);
    if (!result) {
        LOGWARNING(NX::string_formater(L"Fail to write %d bytes to pipe server (%d, size: %d)", data.size(), GetLastError()));
    }
    pipe_client.disconnect();
    return result;
}

bool rmappmanager::send_pilicy_changed_notification(const std::wstring& policyId)
{
    if (policyId.empty()) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    std::string msg_body = "{\"code\":1,\"policyId\":\"";
    msg_body += NX::conversion::utf16_to_utf8(policyId);
    msg_body += "\"}";
    return notify(msg_body);
}

bool rmappmanager::send_popup_notification(const std::wstring& message, bool isJsonMessage)
{
    if (message.empty()) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

	// If message is one json string, should not add the pair of backslash '\'.
	std::string msg_body = "";
	if (isJsonMessage)
	{
		msg_body = "{\"code\":100,\"message\":";
		msg_body += NX::conversion::utf16_to_utf8(message);
		msg_body += "}";
	}
	else
	{
		msg_body = "{\"code\":100,\"message\":\"";
		msg_body += NX::conversion::utf16_to_utf8(message);
		msg_body += "\"}";
	}


    return notify(msg_body);
}

bool rmappmanager::send_logon_notification()
{
    return notify("{\"code\":101}");
}

bool rmappmanager::send_quit_notification()
{
    return notify("{\"code\":102}");
}

bool rmappmanager::send_status_changed_notification()
{
    return notify("{\"code\":103}");
}

//{
//	"code":104,
//		"action" : "Open file",
//		"data" : {
//		"pid":123456,
//			"application" : "C:\natopad.exe",
//			"isAppAllowEdit" : true,
//			"file" : "C:\Allen\work\skydrm\test.txt",
//			"isFileAllowEdit" : false
//	}
//}

bool rmappmanager::send_nxlfile_rights_notification(const std::wstring& strJsonMessage)
{
	std::wstring strMessage = strJsonMessage;

	std::string msg_body = "";
	msg_body = "{\"code\":104,\"action\": \"Open file\" , \"data\": ";
	msg_body += NX::conversion::utf16_to_utf8(strMessage);
	msg_body += "}";

	return notify(msg_body);
}

//{
//	"code":104,
//		"action" : "Process exit",
//		"data" : {
//		"pid":123456,
//		"application" : "C:\notepad.exe"
//	}
//}
bool rmappmanager::send_process_exit_notificatoin(const std::wstring& strJsonMessage)
{
	std::wstring strMessage = strJsonMessage;

	std::string msg_body = "";
	msg_body = "{\"code\":104,\"action\": \"Process exit\" , \"data\": ";
	msg_body += NX::conversion::utf16_to_utf8(strMessage);
	msg_body += "}";

	return notify(msg_body);
}

static std::vector<unsigned long> find_process(const std::wstring& process_name, unsigned long session_id, bool find_all)
{
    std::vector<unsigned long> pids;

    PROCESSENTRY32W pe32;

    memset(&pe32, 0, sizeof(pe32));
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) {
        return pids;
    }
    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        hSnapshot = INVALID_HANDLE_VALUE;
        return pids;
    }

    do {
        const wchar_t* name = wcsrchr(pe32.szExeFile, L'\\');
        name = (NULL == name) ? pe32.szExeFile : (name + 1);
        if (0 == _wcsicmp(process_name.c_str(), name)) {
            unsigned long cur_session_id = -1;
            if (ProcessIdToSessionId(pe32.th32ProcessID, &cur_session_id) && cur_session_id == session_id) {
                pids.push_back(pe32.th32ProcessID);
                if (!find_all) {
                    break;
                }
            }
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    hSnapshot = INVALID_HANDLE_VALUE;

    return pids;
}

void rmappmanager::close_existing_app(unsigned long wait_time, bool force)
{
    send_quit_notification();
    if (get_instance() != nullptr) {
        get_instance()->detach();
    }

    if (force) {

        std::vector<unsigned long> pids = find_process(L"nxrmtray.exe", _session_id, false);
        if (!pids.empty()) {

            if (0 != wait_time) {
                Sleep(wait_time);
            }

            pids = find_process(L"nxrmtray.exe", _session_id, false);
            if (!pids.empty()) {

                HANDLE h = ::OpenProcess(PROCESS_TERMINATE, FALSE, pids[0]);
                if (h != NULL) {
                    TerminateProcess(h, 0);
                    CloseHandle(h);
                    h = NULL;
                }
            }
        }
    }
}

void rmappmanager::daemon(rmappmanager* manager)
{
    bool    active = true;
    unsigned long wait_result = WAIT_TIMEOUT;
    HANDLE wait_objects[2] = { manager->_shutdown_event, NULL };

    wait_objects[0] = manager->get_shutdown_event();
    assert(NULL != wait_objects[0]);

    do {

        manager->close_existing_app(2000, true);

        bool session_not_exists = false;

        try {
            manager->get_instance()->run(manager->get_session_id());
        }
        catch (const NX::exception& e) {
            session_not_exists = (ERROR_NO_SUCH_LOGON_SESSION == e.code());
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
        }
        catch (...) {
            ; // Nothing
        }

        if (session_not_exists) {
            // Due to user logout
            break;
        }

        if (NULL == manager->get_instance()->get_process_handle()) {
            LOGERROR(L"daemon fail to start nxrmtray.exe - exit from daemon");
            //break;
        }

        wait_objects[1] = manager->get_instance()->get_process_handle();

        wait_result = ::WaitForMultipleObjects(2, wait_objects, FALSE, INFINITE);
        switch (wait_result)
        {
        case WAIT_OBJECT_0:
            // shutdown
            active = false;
            manager->close_existing_app(2000, false);
            break;

        case (WAIT_OBJECT_0 + 1) :
            // process has been terminated
            manager->get_instance()->detach();
            break;

        default:
            // error
            active = false;
            manager->close_existing_app(2000, false);
			break;
        }

    } while (active);
}
