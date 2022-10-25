
#include <Windows.h>
#include <assert.h>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\string.hpp>
#include <nudf\resutil.hpp>
#include <nudf\filesys.hpp>
#include <nudf\handyutil.hpp>

#include <boost/algorithm/string.hpp>

// from fltman
#include "nxrmflt.h"
#include "nxrmfltman.h"

#include "nxrmserv.hpp"
#include "nxlfmthlp.hpp"
#include "nxrmres.h"
#include "global.hpp"
#include "serv.hpp"
#include "serv_flt.hpp"


extern rmserv* SERV;

//
//  class drvflt_serv::drv_request
//

drvflt_serv::flt_request::flt_request(unsigned long type, void* msg, unsigned long length, void* msg_context) : _type(0), _context(NULL)
{
    unsigned long required_size = 0;

    // check size
    switch (type)
    {
    case NXRMFLT_MSG_TYPE_CHECK_RIGHTS:
        if (length < sizeof(NXRM_CHECK_RIGHTS_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_CHECK_RIGHTS_NOTIFICATION)", length));
            return;
        }
        break;
    case NXRMFLT_MSG_TYPE_BLOCK_NOTIFICATION:
        if (length < sizeof(NXRM_BLOCK_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_BLOCK_NOTIFICATION)", length));
            return;
        }
        break;
    case NXRMFLT_MSG_TYPE_FILE_ERROR_NOTIFICATION:
        if (length < sizeof(NXRM_FILE_ERROR_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_FILE_ERROR_NOTIFICATION)", length));
            return;
        }
        break;
    case NXRMFLT_MSG_TYPE_PURGE_CACHE_NOTIFICATION:
        if (length < sizeof(NXRM_PURGE_CACHE_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_PURGE_CACHE_NOTIFICATION)", length));
            return;
        }
        break;
    case NXRMFLT_MSG_TYPE_PROCESS_NOTIFICATION:
        if (length < sizeof(NXRM_PROCESS_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_PROCESS_NOTIFICATION)", length));
            return;
        }
        break;
    case NXRMFLT_MSG_TYPE_QUERY_TOKEN:
        if (length < sizeof(NXRM_QUERY_TOKEN_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_QUERY_TOKEN_NOTIFICATION)", length));
            return;
        }
        break;
    case NXRMFLT_MSG_TYPE_ACQUIRE_TOKEN:
        if (length < sizeof(NXRM_ACQUIRE_TOKEN_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_ACQUIRE_TOKEN_NOTIFICATION)", length));
            return;
        }
        break;
    case NXRMFLT_MSG_TYPE_ACTIVITY_LOG:
        if (length < sizeof(NXRM_ACTIVITY_LOG_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_ACTIVITY_LOG_NOTIFICATION)", length));
            return;
        }
        break;
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    case NXRMFLT_MSG_TYPE_CHECK_TRUST:
        if (length < sizeof(NXRM_CHECK_TRUST_NOTIFICATION)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            LOGERROR(NX::string_formater(L"Invalid drv_flt request size: %d != sizeof(NXRM_CHECK_TRUST_NOTIFICATION)", length));
            return;
        }
        break;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    default:
        LOGERROR(NX::string_formater(L"Unknown drv_flt request type (%d)", type));
        break;
    }

    _request.resize(length, 0);
    memcpy(&_request[0], msg, length);
    _type = type;
    _context = msg_context;
}


//
//
//

drvflt_serv::drvflt_serv() : _h(NULL), _running(false)
{
    ::InitializeCriticalSection(&_queue_lock);
    _request_events[0] = ::CreateEventW(NULL, TRUE, FALSE, NULL);   // stop event
    _request_events[1] = ::CreateSemaphoreW(NULL, 0, LONG_MAX, NULL); ;   // request coming semaphore
}

drvflt_serv::~drvflt_serv()
{
    stop();

    if (NULL != _request_events[0]) {
        CloseHandle(_request_events[0]);
        _request_events[0] = NULL;
    }
    if (NULL != _request_events[1]) {
        CloseHandle(_request_events[1]);
        _request_events[1] = NULL;
    }
    ::DeleteCriticalSection(&_queue_lock);
}

void drvflt_serv::start(const std::wstring& protected_volume_name) noexcept
{
    static const std::wstring drvman_dll_file(GLOBAL.get_bin_dir() + L"\\nxrmfltman.dll");

    try {

        unsigned long result = 0;

        // load dll
        _dll.load(drvman_dll_file);

        // create manager
        _h = _dll.create_manager(drvflt_serv::drv_callback, NxGlobalLogWrite, NxGlobalLogAccept, protected_volume_name.c_str(), this);
        if (_h == NULL) {
            throw NX::exception(ERROR_MSG2("FLTSERV", "fail to create fltman manager (%d)", GetLastError()));
        }

        // start nxrmflt manager
        result = _dll.start_filtering(_h);
        if (0 != result) {
            CloseHandle(_h);
            _h = NULL;
            throw NX::exception(ERROR_MSG2("FLTSERV", "fail to start fltman manager (%d)", result));
        }

        // Set running flag before start worker thread
        _running = true;

        // start worker thread
        int nthreads = 0;
        for (int i = 0; i < 8; i++) {
            try {
                _threads.push_back(std::thread(drvflt_serv::worker, this));
                nthreads++;
            }
            catch (std::exception& e) {
                LOGWARNING(ERROR_MSG2("FLTSERV", "fail to start worker thread #%d (%s)", i, e.what()));
            }
        }

        if (0 == nthreads) {
            throw NX::exception(ERROR_MSG2("FLTSERV", "none worker thread has been started"));
        }

        LOGINFO(NX::string_formater(L"FLTSERV: %d worker threads have been started", nthreads));
    }
    catch (std::exception& e) {
        if (_h != NULL) {
            _dll.stop_filtering(_h);
            CloseHandle(_h);
            _h = NULL;
        }
        LOGERROR(e.what());
    }
}

void drvflt_serv::stop() noexcept
{
    if (!_running) {
        return;
    }

    assert(_running);
    _running = false;

    // stop all the request handler threads
    SetEvent(_request_events[0]);
    std::for_each(_threads.begin(), _threads.end(), [&](std::thread& t) {
        if (t.joinable()) {
            t.join();
        }
    });
    _threads.clear();
    ResetEvent(_request_events[0]);

    // stop drvman manager
    _dll.stop_filtering(_h);
    _dll.close_manager(_h);
    _h = NULL;

    // unload dll
    _dll.unload();
}

bool drvflt_serv::on_user_logon(unsigned long session_id, const std::wstring& default_membership)
{
    NXRMFLT_LOGON_SESSION_CREATED request = { 0 };

    const std::string s = NX::conversion::utf16_to_utf8(default_membership);
    if (s.length() > 255) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    memset(&request, 0, sizeof(request));
    request.SessoinId = session_id;
    strncpy_s(request.Default_OwnerId, 256, s.c_str(), _TRUNCATE);
    return (0 == _dll.set_logon_session_created(_h, &request));
}

bool drvflt_serv::on_user_logoff(unsigned long session_id)
{
    NXRMFLT_LOGON_SESSION_TERMINATED request = { 0 };
    request.SessionId = session_id;
    return (0 == _dll.set_logon_session_terminated(_h, &request));
}

bool drvflt_serv::clean_process_cache(unsigned long process_id)
{
	NXRMFLT_CLEAN_PROCESS_CACHE request = { 0 };
	request.ProcessId = process_id;
	return (0 == _dll.set_clean_process_cache(_h, &request));
}

bool drvflt_serv::insert_safe_dir(const std::wstring& dir)
{
    return (0 == _dll.manage_safe_directory(_h, 1, dir.c_str()));
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool drvflt_serv::insert_sanctuary_dir(const std::wstring& dir)
{
    return (0 == _dll.manage_sanctuary_directory(_h, 1, dir.c_str()));
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

bool drvflt_serv::remove_safe_dir(const std::wstring& dir)
{
    return (0 == _dll.manage_safe_directory(_h, 0, dir.c_str()));
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool drvflt_serv::remove_sanctuary_dir(const std::wstring& dir)
{
    return (0 == _dll.manage_sanctuary_directory(_h, 0, dir.c_str()));
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

bool drvflt_serv::delete_nxl_file(const std::wstring& filename)
{
	return (0 == _dll.delete_nxl_file(_h, 0, filename.c_str()));
}

unsigned long drvflt_serv::fltctl_set_saveas_forecast(unsigned long process_id, const wchar_t* source, const wchar_t* target)
{
    return _dll.set_save_as_forecast(_h, process_id, source, target);
}

unsigned long drvflt_serv::fltctl_set_policy_changed()
{
    return _dll.set_policy_changed(_h);
}

void drvflt_serv::worker(drvflt_serv* serv) noexcept
{
    while (serv->is_running()) {

        // wait
        unsigned wait_result = ::WaitForMultipleObjects(2, serv->_request_events, FALSE, INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            // stop event
            break;
        }

        //
        if (wait_result != (WAIT_OBJECT_0 + 1)) {
            // error
            LOGWARNING(NX::string_formater(L"FLTMAN worker thread wait error (result = %d, error = %d)", wait_result, GetLastError()));
            break;
        }

        assert(wait_result == (WAIT_OBJECT_0 + 1));

        // new request comes
        std::shared_ptr<drvflt_serv::flt_request>  request;
        ::EnterCriticalSection(&serv->_queue_lock);
        if (!serv->_request_queue.empty()) {
            request = serv->_request_queue.front();
            assert(request != nullptr);
            serv->_request_queue.pop();
        }
        ::LeaveCriticalSection(&serv->_queue_lock);

        if (request == nullptr) {
            continue;
        }

        // handle request
        try {
			//LOGDEBUG(NX::string_formater(L"drvflt_serv::worker: type = %08x", request->type()));
            switch (request->type())
            {
            case NXRMFLT_MSG_TYPE_CHECK_RIGHTS:
                serv->on_check_rights(request.get());
                break;
            case NXRMFLT_MSG_TYPE_BLOCK_NOTIFICATION:
                serv->on_block_notification(request.get());
                break;
            case NXRMFLT_MSG_TYPE_FILE_ERROR_NOTIFICATION:
                serv->on_file_error_notification(request.get());
                break;
            case NXRMFLT_MSG_TYPE_PURGE_CACHE_NOTIFICATION:
                serv->on_purge_cache_notification(request.get());
                break;
            case NXRMFLT_MSG_TYPE_PROCESS_NOTIFICATION:
                serv->on_process_notification(request.get());
                break;
            case NXRMFLT_MSG_TYPE_QUERY_TOKEN:
				serv->on_query_token(request.get());
                break;
            case NXRMFLT_MSG_TYPE_ACQUIRE_TOKEN:
				serv->on_acquire_token(request.get());
                break;
            case NXRMFLT_MSG_TYPE_ACTIVITY_LOG:
                serv->on_log_activity(request.get());
                break;
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
            case NXRMFLT_MSG_TYPE_CHECK_TRUST:
                serv->on_check_trust(request.get());
                break;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
            default:
                // unknown type
                throw NX::exception(ERROR_MSG("FLTSERV", "Unknown fltman request %08X", request->type()));
                break;
            }
        }
        catch (std::exception& e) {
            LOGWARNING(e.what());
        }
        catch (NX::structured_exception& e) {
            LOGCRITICAL(e.exception_message());
        }
#ifndef _DEBUG
        catch (...) {
            LOGCRITICAL(ERROR_MSG("FLTSERV", "Unknown exception when processing request %08X", request->type()));
        }
#endif
    }
}

unsigned long __stdcall drvflt_serv::drv_callback(unsigned long type, void* msg, unsigned long length, void* msg_context, void* user_context)
{
    unsigned long result = 0;
    drvflt_serv* serv = (drvflt_serv*)user_context;
    
    std::shared_ptr<flt_request> request(new flt_request(type, msg, length, msg_context));
    if (request != NULL && !request->empty()) {
        ::EnterCriticalSection(&serv->_queue_lock);
        serv->_request_queue.push(request);
        ::LeaveCriticalSection(&serv->_queue_lock);
		//
		// release semaphore outside the lock
		//
		if (!::ReleaseSemaphore(serv->_request_events[1], 1, NULL)) {
			LOGWARNING(NX::string_formater(L"Release semaphore (error = %d)", GetLastError()));
		}
	}
    else {
        result = GetLastError();
        if (0 == result) result = ERROR_INVALID_PARAMETER;
    }

    return result;
}

void drvflt_serv::on_check_rights(flt_request* request)
{
    const NXRM_CHECK_RIGHTS_NOTIFICATION* req = (const NXRM_CHECK_RIGHTS_NOTIFICATION*)request->request();
    NXRMFLT_CHECK_RIGHTS_REPLY  reply = { 0 };
    ULONG session_id = 0;

    assert(request->type() == NXRMFLT_MSG_TYPE_CHECK_RIGHTS);

    ProcessIdToSessionId(req->ProcessId, &session_id);

    LOGDETAIL(NX::string_formater(L"FLTSERV Request: CHECK_RIGHTS"));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", session_id));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  FileName:     %s", req->FileName));

    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(session_id);
    if (sp == nullptr) {
        memset(&reply, 0, sizeof(reply));
        reply.RightsMask |= RIGHTS_NOT_CACHE;
        _dll.reply_check_rights(_h, request->context(), &reply);
        if (0 != session_id) {
            LOGINFO(NX::string_formater(L"Fail to check rights because session (%d) context not exist (processId: %d, file: %s)", session_id, req->ProcessId, req->FileName));
        }
        else {
            LOGDETAIL(NX::string_formater(L"Fail to check rights because session (%d) context not exist (processId: %d, file: %s)", session_id, req->ProcessId, req->FileName));
        }
        return;
    }

	if (!sp->get_rm_session().pre_rights_evaluate(req->FileName, req->ProcessId, session_id))
	{
		memset(&reply, 0, sizeof(reply));
		reply.RightsMask |= RIGHTS_NOT_CACHE;
		_dll.reply_check_rights(_h, request->context(), &reply);

		LOGINFO(NX::string_formater(L"Fail to check rights, pre_rights_evaluate session : %d ,process : %d, file : %s", session_id, req->ProcessId, req->FileName));
		return;
	}

	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> vecRightsWatermarks;
    // evaluate
    //if (!sp->get_rm_session().rights_evaluate(req->FileName, req->ProcessId, session_id, &reply.EvaluationId, &reply.RightsMask, &reply.CustomRights))
	if(!sp->get_rm_session().rights_evaluate_with_watermark(req->FileName, req->ProcessId, session_id, &reply.EvaluationId, &reply.RightsMask, &reply.CustomRights, vecRightsWatermarks))
	{
        memset(&reply, 0, sizeof(reply));
        reply.RightsMask |= RIGHTS_NOT_CACHE;

        std::wstring strNxlFile = req->FileName;
        SERV->handle_nxl_denied_file(req->ProcessId, strNxlFile);
    }
    else
    {
        std::wstring strNxlFile = req->FileName;
		std::wstring strJson = sp->get_rm_session().file_rights_watermark_to_json(strNxlFile, reply.RightsMask, vecRightsWatermarks);
        GLOBAL.get_process_cache().insert_process_file_rights(req->ProcessId, strNxlFile, strJson);
        if (reply.RightsMask == 0)
        {
            // NO RIGHTS ON THE FILE
            SERV->handle_nxl_denied_file(req->ProcessId, strNxlFile);
        }
        SERV->FileRightsNotification(req->ProcessId, strNxlFile, reply.RightsMask);
    }
    _dll.reply_check_rights(_h, request->context(), &reply);

    // update process cache
    if (!flags64_on(reply.RightsMask, RIGHTS_NOT_CACHE)) {
        GLOBAL.get_process_cache().set_process_rights(req->ProcessId, reply.RightsMask);
    }
}

void drvflt_serv::on_block_notification(flt_request* request)
{
    const NXRM_BLOCK_NOTIFICATION* req = (const NXRM_BLOCK_NOTIFICATION*)request->request();

    assert(request->type() == NXRMFLT_MSG_TYPE_BLOCK_NOTIFICATION);

    LOGDETAIL(NX::string_formater(L"FLTSERV Request: BLOCK_NOTIFICATION"));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  Reason:       %08X", req->Reason));
    LOGDETAIL(NX::string_formater(L"  FileName:     %s", req->FileName));

    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
    if (sp == nullptr) {
        LOGINFO(NX::string_formater(L"Fail to send block notification because session (%d) context not exist (processId: %d, file: %s)", req->SessionId, req->ProcessId, req->FileName));
        return;
    }
	LOGDEBUG(NX::string_formater(" on_block_notification: ProcessId: %d,  %s", req->ProcessId, req->FileName));

    std::wstring operation_name;
    std::wstring message;
    NX::fs::dos_filepath full_path(req->FileName);
    const std::wstring filename(full_path.file_name().full_name());
    int act_operation = 0;

    switch (req->Reason)
    {
    case nxrmfltDeniedWritesOpen:
        act_operation = ActEdit;
        message = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_READONLY_MODE, 1024, LANG_NEUTRAL, L"The file (%s) will be opened in read only mode because you don't have write permission", filename.c_str());
        break;
    case nxrmfltDeniedSaveAsOpen:
        act_operation = ActCopyContent;
        operation_name = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_OPERATION_OVERWRITE, 256, LANG_NEUTRAL, L"overwrite");
        message = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s file %s", operation_name.c_str(), filename.c_str());
        break;
    case nxrmfltSaveAsToUnprotectedVolume:
        act_operation = ActCopyContent;
        message = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_SAVEAS_DENIED, 1024, LANG_NEUTRAL, L"File (%s) cannot be saved to unprotected location", filename.c_str());
        break;
    default:
        operation_name = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_OPERATION_DEFAULT, 256, LANG_NEUTRAL, L"operate");
        message = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s file %s", operation_name.c_str(), filename.c_str());
        break;
    }

    sp->get_rm_session().get_app_manager().send_popup_notification(message);

    // Log activity
    if (0 != act_operation) {
        NX::NXL::document_context context(full_path.path());
        if (!context.empty()) {

            const process_record& proc_record = GLOBAL.safe_find_process(req->ProcessId);
            if (!proc_record.empty()) {
                const std::wstring image_path(proc_record.get_image_path());
                const wchar_t* image_name = wcsrchr(image_path.c_str(), L'\\');
                image_name = (NULL == image_name) ? image_path.c_str() : (image_name + 1);

                // try to log activity
                sp->get_rm_session().log_activity(activity_record(context.get_duid(),
                                                                  context.get_owner_id(),
                                                                  sp->get_rm_session().get_profile().get_id(),
                                                                  ActEdit,
                                                                  ActDenied,
                                                                  full_path.path(),
                                                                  proc_record.get_image_path(),
                                                                  proc_record.get_pe_file_info()->get_image_publisher(),
                                                                  std::wstring()));

                sp->get_rm_session().audit_activity(ActEdit, ActDenied, NX::conversion::to_wstring(context.get_duid()), context.get_owner_id(), image_name, full_path.path());
            }
        }
    }
}

void drvflt_serv::on_file_error_notification(flt_request* request)
{
    const NXRM_FILE_ERROR_NOTIFICATION* req = (const NXRM_FILE_ERROR_NOTIFICATION*)request->request();

    assert(request->type() == NXRMFLT_MSG_TYPE_FILE_ERROR_NOTIFICATION);

    LOGDETAIL(NX::string_formater(L"FLTSERV Request: FILE_ERROR_NOTIFICATION"));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  FileName:     %s", req->FileName));

    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
    if (sp == nullptr) {
        LOGINFO(NX::string_formater(L"Fail to send file error notification because session (%d) context not exist (file: %s)", req->SessionId, req->FileName));
        return;
    }

    NX::fs::dos_filepath full_path(req->FileName);
    const std::wstring filename(full_path.file_name().full_name());
    // const std::wstring& message = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_MSG_UNRECOGNIZED_KEY_ID, 1024, LANG_NEUTRAL, L"File (%s) is invalid (%s)", filename.c_str(), req->Status);
    const std::wstring& message = NX::string_formater(L"File error (%d, %s)", req->Status, filename.c_str());
    sp->get_rm_session().get_app_manager().send_popup_notification(message);
}

void drvflt_serv::on_purge_cache_notification(flt_request* request)
{
    const NXRM_PURGE_CACHE_NOTIFICATION* req = (const NXRM_PURGE_CACHE_NOTIFICATION*)request->request();

    assert(request->type() == NXRMFLT_MSG_TYPE_PURGE_CACHE_NOTIFICATION);

    LOGDETAIL(NX::string_formater(L"FLTSERV Request: PURGE_CACHE_NOTIFICATION"));
    LOGDETAIL(NX::string_formater(L"  FileName:     %s", req->FileName));
}

void drvflt_serv::on_process_notification(flt_request* request)
{
    const NXRM_PROCESS_NOTIFICATION* req = (const NXRM_PROCESS_NOTIFICATION*)request->request();

    assert(request->type() == NXRMFLT_MSG_TYPE_PROCESS_NOTIFICATION);

    NX::fs::dos_filepath image_path(req->ProcessImagePath);

    LOGDETAIL(NX::string_formater(L"FLTSERV Request: PROCESS_NOTIFICATION"));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  Operation:    %s", req->Create ? L"Create" : L"Destroy"));
    LOGDETAIL(NX::string_formater(L"  Flags:        %08X", req->Flags));
    LOGDETAIL(NX::string_formater(L"  ImageName:    %s", req->ProcessImagePath));

	bool ready = false;
    if (req->Create) {		
        GLOBAL.get_process_cache().insert(process_record(req->ProcessId, req->SessionId, image_path.path(), req->Flags));
		//LOGDEBUG(NX::string_formater(L"on_process_notification:  ProcessId: %d,  (%s)", req->ProcessId, req->ProcessImagePath));
    }
    else 
	{
		if (SERV->IsPDPProcess(req->ProcessId))
		{
			LOGDEBUG(NX::string_formater(L"on_process_notification:  ProcessId: %d,  cepdpman closed", req->ProcessId));
			SERV->ensurePDPConnectionReady(ready);
			if (!ready)
			{
				//LOGDEBUG(NX::string_formater(L"*** cannot launch cepdpman ****"));
			}
			else
				LOGDEBUG(NX::string_formater(L"*** launch cepdpman success****"));
		}

		auto process_cache = GLOBAL.get_process_cache().find(req->ProcessId);
		image_path = process_cache.get_image_path();

        GLOBAL.get_process_cache().remove(req->ProcessId);
    }

    SERV->ProcessNotification(req->ProcessId, image_path.path(), req->Create != 0);
}

void drvflt_serv::on_query_token(flt_request* request)
{
    const NXRM_QUERY_TOKEN_NOTIFICATION* req = (const NXRM_QUERY_TOKEN_NOTIFICATION*)request->request();
    NXRMFLT_QUERY_TOKEN_REPLY reply = { 0 };
    ULONG status = -1;

    assert(request->type() == NXRMFLT_MSG_TYPE_QUERY_TOKEN);
    
    LOGDETAIL(NX::string_formater(L"FLTSERV Request: QUERY_TOKEN"));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  FileName:     %s", req->FileName));

    memset(&reply, 0, sizeof(reply));

    do {

        std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
        if (sp == nullptr) {
            LOGINFO(NX::string_formater(L"Fail to query token because session (%d) context not exist (file: %s)", req->SessionId, req->FileName));
            status = ERROR_NO_SUCH_LOGON_SESSION;
            break;
        }

        if (!sp->get_rm_session().is_logged_on()) {
            LOGINFO(NX::string_formater(L"Fail to query token because user has not logon (session: %d, file: %s)", req->SessionId, req->FileName));
            status = ERROR_NO_SUCH_LOGON_SESSION;
            break;
        }

        unsigned long token_level = 0;
        unsigned long key_mode_and_flags = 0;
        std::vector<unsigned char> token_id;
        std::wstring policy;
        std::wstring tags;
        std::vector<unsigned char> agreement;
        std::wstring owner_id;

		//
		// Added by Philip Qi
		//
		{
			NX::win::session_token st(req->SessionId);
			NX::win::impersonate_object impersonobj(st);

			if (!get_query_token_context(req->FileName, owner_id, token_id, policy, tags, &token_level, agreement, &key_mode_and_flags)) {
				LOGINFO(NX::string_formater(L"Fail to query token because cannot get token context from file (session: %d, file: %s)", req->SessionId, req->FileName));
				break;
			}
		}
        assert(token_id.size() == 16);
        assert(agreement.size() == 256);

        // try to find token
        int protection_type = (policy.empty() || policy == L"{}") ? 1 : 0;   // 1 = central, 0 = ad-hoc
        std::vector<unsigned char> token_value;
        bool token_from_cache = false;
        if (!sp->get_rm_session().query_token(req->ProcessId, owner_id, agreement, token_id, protection_type, policy, tags, token_level, token_value, &token_from_cache)) {
            LOGINFO(NX::string_formater(L"Fail to query token (session: %d, file: %s)", req->SessionId, req->FileName));
            break;
        }

        // get token from server
        assert(32 == token_value.size());
        memcpy(reply.Token.Token, token_value.data(), 32);
        reply.Token.TokenLevel = token_level;
        reply.Token.TokenSecureMode = (key_mode_and_flags >> 24);
        reply.TokenTTL = (ULONGLONG)((__int64)(sp->get_rm_session().get_profile().get_token().get_expire_time()));
        status = 0;

#ifdef _DEBUG
        const std::wstring& str_token_id = NX::conversion::to_wstring(token_id);
        const std::wstring& str_token = NX::conversion::to_wstring(token_value);
        LOGDETAIL(NX::string_formater(L"FLTSERV: get decrypt token from %s: ", token_from_cache ? L"cache" : L"server"));
        LOGDETAIL(NX::string_formater(L"    File:    %s", req->FileName));
        LOGDETAIL(NX::string_formater(L"    TokenId: %s", str_token_id.c_str()));
        LOGDETAIL(NX::string_formater(L"    Token:   %s", str_token.c_str()));
        LOGDETAIL(NX::string_formater(L"    TokenLevel: %d", token_level));
#endif // _DEBUG


    } while (false);

    _dll.reply_query_token(_h, request->context(), status, &reply);
}

void drvflt_serv::on_acquire_token(flt_request* request)
{
    const NXRM_ACQUIRE_TOKEN_NOTIFICATION* req = (const NXRM_ACQUIRE_TOKEN_NOTIFICATION*)request->request();
    NXRMFLT_ACQUIRE_TOKEN_REPLY reply = { 0 };
    ULONG status = -1;

    assert(request->type() == NXRMFLT_MSG_TYPE_ACQUIRE_TOKEN);
    
    LOGDETAIL(NX::string_formater(L"FLTSERV Request: ACQUIRE_TOKEN"));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  MemberId:     %S", req->OwnerId));

    memset(&reply, 0, sizeof(reply));
    
    do {

        std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
        if (sp == nullptr) {
            status = ERROR_NO_SUCH_LOGON_SESSION;
            LOGINFO(NX::string_formater(L"Fail to acquire token because session (%d) context not exist", req->SessionId));
            break;
        }

        if (!sp->get_rm_session().is_logged_on()) {
            status = ERROR_NO_SUCH_LOGON_SESSION;
            LOGINFO(NX::string_formater(L"Fail to query token because user has not logon (session: %d)", req->SessionId));
            break;
        }

        std::string membership_id(req->OwnerId);
        //std::transform(membership_id.begin(), membership_id.end(), membership_id.begin(), tolower);
        const std::wstring wsmembership_id(NX::conversion::utf8_to_utf16(membership_id));

        bool is_me = sp->get_rm_session().get_profile().is_me(wsmembership_id);
        if (is_me == false) {
            LOGINFO(NX::string_formater(L"Fail to query token because membership not exist (session: %d, member: %s)", req->SessionId, wsmembership_id.c_str()));
            break;
        }

        const std::vector<unsigned char> agreement0(sp->get_rm_session().get_profile().get_agreement0(wsmembership_id));
        const std::vector<unsigned char> agreement1(sp->get_rm_session().get_profile().get_agreement1(wsmembership_id));
        const unsigned long default_securemode = sp->get_rm_session().get_profile().get_preferences().get_secure_mode();

        const encrypt_token new_token(sp->get_rm_session().get_profile().pop_token(wsmembership_id));
        if (new_token.empty()) {
            LOGINFO(NX::string_formater(L"Fail to acquire token because membership doesn't have any token (session: %d, member: %s)", req->SessionId, wsmembership_id.c_str()));
            break;
        }


        // fill reply
        memcpy(reply.Udid, new_token.get_token_id().data(), 16);
        memcpy(reply.PublicKey1, agreement0.data(), 256);
        if (!agreement1.empty()) {
            reply.KeyFlags |= KF_RECOVERY_KEY_ENABLED;
            memcpy(reply.PublicKey2, agreement1.data(), 256);
        }
        reply.Token.TokenLevel = new_token.get_token_level();
        reply.Token.TokenSecureMode = default_securemode;
        memcpy(reply.Token.Token, new_token.get_token_value().data(), 32);
        reply.TokenTTL = (ULONGLONG)((__int64)(sp->get_rm_session().get_profile().get_token().get_expire_time()));

        // put it into cache
        sp->get_rm_session().get_token_cache().insert(new_token.get_token_id(), new_token.get_token_level(), new_token.get_token_value());

        status = 0;
        
#ifdef _DEBUG
        const std::wstring& str_token_id = NX::conversion::to_wstring(new_token.get_token_id());
        const std::wstring& str_token = NX::conversion::to_wstring(new_token.get_token_value());
        LOGDETAIL(NX::string_formater(L"FLTSERV: grant token to member %s: ", wsmembership_id.c_str()));
        LOGDETAIL(NX::string_formater(L"    TokenId: %s", str_token_id.c_str()));
        LOGDETAIL(NX::string_formater(L"    Token:   %s", str_token.c_str()));
        LOGDETAIL(NX::string_formater(L"    TokenLevel: %d", new_token.get_token_level()));
#endif // _DEBUG

    } while (false);

    _dll.reply_acquire_token(_h, request->context(), status, &reply);
}

void drvflt_serv::on_log_activity(flt_request* request)
{
    const NXRM_ACTIVITY_LOG_NOTIFICATION* req = (const NXRM_ACTIVITY_LOG_NOTIFICATION*)request->request();
    ULONG status = -1;


    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
    if (sp == nullptr) {
        status = ERROR_NO_SUCH_LOGON_SESSION;
        LOGINFO(NX::string_formater(L"Fail to acquire token because session (%d) context not exist", req->SessionId));
        return;
    }

    if (!sp->get_rm_session().is_logged_on()) {
        status = ERROR_NO_SUCH_LOGON_SESSION;
        LOGINFO(NX::string_formater(L"Fail to query token because user has not logon (session: %d)", req->SessionId));
        return;
    }


    // Log activity
    NX::NXL::document_context context(req->FileName);
    if (!context.empty()) {

        const process_record& proc_record = GLOBAL.safe_find_process(req->ProcessId);
        if (!proc_record.empty()) {
            const std::wstring image_path(proc_record.get_image_path());
            const wchar_t* image_name = wcsrchr(image_path.c_str(), L'\\');
            image_name = (NULL == image_name) ? image_path.c_str() : (image_name + 1);

            // try to log activity
            sp->get_rm_session().log_activity(activity_record(context.get_duid(),
                                                              context.get_owner_id(),
                                                              sp->get_rm_session().get_profile().get_id(),
                                                              req->Op,
                                                              req->Result,
                                                              req->FileName,
                                                              proc_record.get_image_path(),
                                                              proc_record.get_pe_file_info()->get_image_publisher(),
                                                              std::wstring()));

            sp->get_rm_session().audit_activity(req->Op, req->Result, NX::conversion::to_wstring(context.get_duid()), context.get_owner_id(), image_name, req->FileName);
        }
    }
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
void drvflt_serv::on_check_trust(flt_request* request)
{
    const NXRM_CHECK_TRUST_NOTIFICATION* req = (const NXRM_CHECK_TRUST_NOTIFICATION*)request->request();
    NXRMFLT_CHECK_TRUST_REPLY  reply = { 0 };
    ULONG session_id = 0;

    assert(request->type() == NXRMFLT_MSG_TYPE_CHECK_TRUST);

    ProcessIdToSessionId(req->ProcessId, &session_id);

    LOGDETAIL(NX::string_formater(L"FLTSERV Request: CHECK_TRUST"));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", session_id));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));

    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(session_id);
    if (sp == nullptr) {
        memset(&reply, 0, sizeof(reply));
        reply.Trusted = FALSE;
        _dll.reply_check_trust(_h, request->context(), &reply);
        if (0 != session_id) {
            LOGINFO(NX::string_formater(L"Fail to check trust because session (%d) context not exist (processId: %d)", session_id, req->ProcessId));
        }
        else {
            LOGDETAIL(NX::string_formater(L"Fail to check trust because session (%d) context not exist (processId: %d)", session_id, req->ProcessId));
        }
        return;
    }

    // evaluate
    bool Trusted;
    if (!sp->get_rm_session().trust_evaluate(req->ProcessId, &Trusted))
    {
        memset(&reply, 0, sizeof(reply));
        reply.Trusted = FALSE;
    }
    else
    {
        reply.Trusted = Trusted;
    }

	if (Trusted) {
		std::wstring filepath = L""; // cannot get path from req.
		uint64_t rightsMask = 5; // default right: view + print.
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> vecRightsWatermarks; // no watermark.
		std::wstring strJson = sp->get_rm_session().file_rights_watermark_to_json(filepath, rightsMask, vecRightsWatermarks);
		GLOBAL.get_process_cache().insert_process_file_rights(req->ProcessId, filepath, strJson);
		
		SERV->FileRightsNotification(req->ProcessId, filepath, rightsMask);
	}

    _dll.reply_check_trust(_h, request->context(), &reply);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

bool drvflt_serv::get_query_token_context(_In_ const std::wstring& file,
    _Out_ std::wstring& owner_id,
    _Out_ std::vector<unsigned char>& token_id,
    _Out_ std::wstring& policy,
    _Out_ std::wstring& tags,
    _Out_ unsigned long* token_level,
    _Out_ std::vector<unsigned char>& agreement,
    _Out_ unsigned long* key_mode_and_flags)
{
    const std::wstring nxl_file(boost::algorithm::iends_with(file, L".nxl") ? file : (file + L".nxl"));

	// Fix the bug that can't view for long file path.
	const std::wstring unlimitedFilePath = NX::fs::dos_fullfilepath(nxl_file).global_dos_path();

    NX::NXL::document_context nxlcontext;
    if (!nxlcontext.load(unlimitedFilePath)) {
        return false;
    }

    owner_id = nxlcontext.get_owner_id();
    token_id = nxlcontext.get_duid();
    policy = NX::conversion::utf8_to_utf16(nxlcontext.get_policy());
    tags = NX::conversion::utf8_to_utf16(nxlcontext.get_tags());
    *token_level = nxlcontext.get_token_level();
    agreement = nxlcontext.get_agreement0();
    *key_mode_and_flags = nxlcontext.get_security_mode();
    return true;
}