

#include <Windows.h>
#include <assert.h>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\string.hpp>

// from fltman
#include "nxrmflt.h"
#include "nxrmfltman.h"

#include "drvflt.hpp"


using namespace NX;


#define ID_CREATE_MANAGER               1
#define ID_REPLY_CHECKRIGHTS            2
#define ID_STOP_FILTERING               3
#define ID_START_FILTERING              4
#define ID_CLOSE_MANAGER                5
#define ID_SET_SAVEAS_FORECAST          6
#define ID_SET_POLICY_CHANGED           7
#define ID_REPLY_QUERY_TOKEN            9
#define ID_SET_LOGON_SESSION_CREATED    10
#define ID_SET_LOGON_SESSION_TERMINATED 11
#define ID_REPLY_ACQUIRE_TOKEN          12
#define ID_MANAGE_SAFE_DIRECTORY        13
#define ID_DELETE_NXL_FILE				14
#define ID_SET_CLEAN_PROCESS_CACHE		15
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
#define ID_MANAGE_SANCTUARY_DIRECTORY   16
#define ID_REPLY_CHECK_TRUST            17
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

drvflt_man_instance::drvflt_man_instance() : dll_instance<FLTMAN_FUNCTION_NUMBER>(std::vector<function_item>({
    function_item(ID_CREATE_MANAGER, "CreateManager"),
    function_item(ID_REPLY_CHECKRIGHTS, "ReplyCheckRights"),
    function_item(ID_STOP_FILTERING, "StopFiltering"),
    function_item(ID_START_FILTERING, "StartFiltering"),
    function_item(ID_CLOSE_MANAGER, "CloseManager"),
    function_item(ID_SET_SAVEAS_FORECAST, "SetSaveAsForecast"),
    function_item(ID_SET_POLICY_CHANGED, "SetPolicyChanged"),
    function_item(ID_REPLY_QUERY_TOKEN, "ReplyQueryToken"),
    function_item(ID_SET_LOGON_SESSION_CREATED, "SetLogonSessionCreated"),
    function_item(ID_SET_LOGON_SESSION_TERMINATED, "SetLogonSessionTerminated"),
    function_item(ID_REPLY_ACQUIRE_TOKEN, "ReplyAcquireToken"),
    function_item(ID_MANAGE_SAFE_DIRECTORY, "ManageSafeDirectory"),
	//function_item(ID_DELETE_NXL_FILE, "DeleteNxlFile")
	function_item(ID_SET_CLEAN_PROCESS_CACHE, "SetCleanProcessCache"),
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    function_item(ID_MANAGE_SANCTUARY_DIRECTORY, "ManageSanctuaryDirectory"),
    function_item(ID_REPLY_CHECK_TRUST, "ReplyCheckTrust"),
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
}))
{
}

drvflt_man_instance::~drvflt_man_instance()
{
}

void drvflt_man_instance::load(const std::wstring& dll_file)
{
    dll_instance::load(dll_file);

    if (!is_loaded()) {
        throw NX::exception(NX::string_formater("Fail to load nxrmfltman.dll (%S)", dll_file.c_str()));
    }

    std::for_each(begin(), end(), [](const std::pair<unsigned long, function_item>& item) {
        if (nullptr == item.second.get_function_pointer()) {
            throw NX::exception(NX::string_formater("Cannot find function %S in nxrmfltman.dll", item.second.get_name().c_str()));
        }
    });
}

HANDLE drvflt_man_instance::create_manager(void* notify_callback, void* log_callback, void* loglevel_callback, const wchar_t* volume_name, void* context)
{
    typedef HANDLE(WINAPI* NXRMFLT_CREATE_MANAGER)(void*, void*, void*, const wchar_t*, void*);
    return EXECUTE(NXRMFLT_CREATE_MANAGER, *this, ID_CREATE_MANAGER, notify_callback, log_callback, loglevel_callback, volume_name, context);
}

unsigned long drvflt_man_instance::reply_check_rights(HANDLE h, void* context, void* reply)
{
    typedef ULONG(WINAPI* NXRMFLT_REPLY_CHECKRIGHTS)(HANDLE, PVOID, void*);
    return EXECUTE(NXRMFLT_REPLY_CHECKRIGHTS, *this, ID_REPLY_CHECKRIGHTS, h, context, reply);
}

unsigned long drvflt_man_instance::reply_query_token(HANDLE h, void* context, unsigned long status, void* reply)
{
    typedef ULONG(WINAPI* NXRMFLT_REPLY_QUERY_TOKEN)(HANDLE, PVOID, ULONG, void*);
    return EXECUTE(NXRMFLT_REPLY_QUERY_TOKEN, *this, ID_REPLY_QUERY_TOKEN, h, context, status, reply);
}

unsigned long drvflt_man_instance::reply_acquire_token(HANDLE h, void* context, unsigned long status, void* reply)
{
    typedef ULONG(WINAPI* NXRMFLT_REPLY_ACQUIRE_TOKEN)(HANDLE, PVOID, ULONG, void*);
    return EXECUTE(NXRMFLT_REPLY_ACQUIRE_TOKEN, *this, ID_REPLY_ACQUIRE_TOKEN, h, context, status, reply);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
unsigned long drvflt_man_instance::reply_check_trust(HANDLE h, void* context, void* reply)
{
    typedef ULONG(WINAPI* NXRMFLT_REPLY_CHECK_TRUST)(HANDLE, PVOID, void*);
    return EXECUTE(NXRMFLT_REPLY_CHECK_TRUST, *this, ID_REPLY_CHECK_TRUST, h, context, reply);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

unsigned long drvflt_man_instance::start_filtering(HANDLE h)
{
    typedef ULONG(WINAPI* NXRMFLT_START_FILTERING)(HANDLE);
    return EXECUTE(NXRMFLT_START_FILTERING, *this, ID_START_FILTERING, h);
}

unsigned long drvflt_man_instance::stop_filtering(HANDLE h)
{
    typedef ULONG(WINAPI* NXRMFLT_STOP_FILTERING)(HANDLE);
    return EXECUTE(NXRMFLT_STOP_FILTERING, *this, ID_STOP_FILTERING, h);
}

unsigned long drvflt_man_instance::close_manager(HANDLE h)
{
    typedef ULONG(WINAPI* NXRMFLT_CLOSE_MANAGER)(HANDLE);
    return EXECUTE(NXRMFLT_CLOSE_MANAGER, *this, ID_CLOSE_MANAGER, h);
}

unsigned long drvflt_man_instance::set_save_as_forecast(HANDLE h, unsigned long process_id, _In_opt_ const wchar_t* source_file, _In_ const wchar_t* target_file)
{
    typedef ULONG(WINAPI* NXRMFLT_SET_SAVEAS_FORECAST)(HANDLE, ULONG, const WCHAR*, const WCHAR*);
    return EXECUTE(NXRMFLT_SET_SAVEAS_FORECAST, *this, ID_SET_SAVEAS_FORECAST, h, process_id, source_file, target_file);
}

unsigned long drvflt_man_instance::manage_safe_directory(HANDLE h, unsigned long op, _In_opt_ const wchar_t* dir)
{
    typedef ULONG(WINAPI* NXRMFLT_MANAGE_SAFE_DIRECTORY)(HANDLE, ULONG, const WCHAR*);
    return EXECUTE(NXRMFLT_MANAGE_SAFE_DIRECTORY, *this, ID_MANAGE_SAFE_DIRECTORY, h, op, dir);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
unsigned long drvflt_man_instance::manage_sanctuary_directory(HANDLE h, unsigned long op, _In_opt_ const wchar_t* dir)
{
    typedef ULONG(WINAPI* NXRMFLT_MANAGE_SANCTUARY_DIRECTORY)(HANDLE, ULONG, const WCHAR*);
    return EXECUTE(NXRMFLT_MANAGE_SANCTUARY_DIRECTORY, *this, ID_MANAGE_SANCTUARY_DIRECTORY, h, op, dir);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

unsigned long drvflt_man_instance::set_policy_changed(HANDLE h)
{
    typedef ULONG(WINAPI* NXRMFLT_SET_POLICY_CHANGED)(HANDLE);
    return EXECUTE(NXRMFLT_SET_POLICY_CHANGED, *this, ID_SET_POLICY_CHANGED, h);
}

unsigned long drvflt_man_instance::set_logon_session_created(HANDLE h, void* create_info)
{
    typedef ULONG(WINAPI* NXRMFLT_SET_LOGON_SESSION_CREATED)(HANDLE, void*);
    return EXECUTE(NXRMFLT_SET_LOGON_SESSION_CREATED, *this, ID_SET_LOGON_SESSION_CREATED, h, create_info);
}

unsigned long drvflt_man_instance::set_logon_session_terminated(HANDLE h, void* terminate_info)
{
    typedef ULONG(WINAPI* NXRMFLT_SET_LOGON_SESSION_TERMINATED)(HANDLE, void*);
    return EXECUTE(NXRMFLT_SET_LOGON_SESSION_TERMINATED, *this, ID_SET_LOGON_SESSION_TERMINATED, h, terminate_info);
}

unsigned long drvflt_man_instance::delete_nxl_file(HANDLE h, unsigned long op, _In_opt_ const wchar_t* file)
{
	typedef ULONG(WINAPI* NXRMFLT_DELETE_NXL_FILE)(HANDLE, ULONG, const WCHAR*);
	//return EXECUTE(NXRMFLT_DELETE_NXL_FILE, *this, ID_DELETE_NXL_FILE, h, op, file);
	return 0;
}

unsigned long drvflt_man_instance::set_clean_process_cache(HANDLE h, void* process_info)
{
	typedef ULONG(WINAPI* NXRMFLT_SET_CLEAN_PROCESS_CACHE)(HANDLE, void*);
	return EXECUTE(NXRMFLT_SET_CLEAN_PROCESS_CACHE, *this, ID_SET_CLEAN_PROCESS_CACHE, h, process_info);
}