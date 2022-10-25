
#pragma once
#ifndef __NXRMDRV_FLT_HPP__
#define __NXRMDRV_FLT_HPP__

#include <string>
#include "function_dll.hpp"

namespace NX {

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
// it exports 15 functions
#define FLTMAN_FUNCTION_NUMBER  15
#else   // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
// it exports 13 functions
#define FLTMAN_FUNCTION_NUMBER  13
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR


class drvflt_man_instance : public dll_instance<FLTMAN_FUNCTION_NUMBER>
{
public:
    drvflt_man_instance();
    virtual ~drvflt_man_instance();

    virtual void load(const std::wstring& file);
    
    // functions
    HANDLE create_manager(void* notify_callback, void* log_callback, void* loglevel_callback, const wchar_t* volume_name, void* context);
    unsigned long reply_check_rights(HANDLE h, void* context, void* reply);
    unsigned long reply_query_token(HANDLE h, void* context, unsigned long status, void* reply);
    unsigned long reply_acquire_token(HANDLE h, void* context, unsigned long status, void* reply);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    unsigned long reply_check_trust(HANDLE h, void* context, void* reply);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    unsigned long start_filtering(HANDLE h);
    unsigned long stop_filtering(HANDLE h);
    unsigned long close_manager(HANDLE h);
    unsigned long set_save_as_forecast(HANDLE h, unsigned long process_id, _In_opt_ const wchar_t* source_file, _In_ const wchar_t* target_file);
    unsigned long set_policy_changed(HANDLE h);
    unsigned long set_logon_session_created(HANDLE h, void* create_info);
    unsigned long set_logon_session_terminated(HANDLE h, void* terminate_info);
    unsigned long manage_safe_directory(HANDLE h, unsigned long op, _In_opt_ const wchar_t* dir);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    unsigned long manage_sanctuary_directory(HANDLE h, unsigned long op, _In_opt_ const wchar_t* dir);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	unsigned long delete_nxl_file(HANDLE h, unsigned long op, _In_opt_ const wchar_t* file);
	unsigned long set_clean_process_cache(HANDLE h, void* process_info);
};


}


#endif