

#include <Windows.h>
#include <assert.h>

#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\filesys.hpp>
#include <nudf\string.hpp>
#include <nudf\resutil.hpp>
#include <nudf\handyutil.hpp>
#include <nudf\shared\officelayout.h>

// from drvman
#include "nxrmdrv.h"
#include "nxrmdrvman.h"
#include "nxrmflt.h"

#include "nxrmserv.hpp"
#include "nxrmres.h"
#include "global.hpp"
#include "serv.hpp"
#include "serv_core.hpp"




extern rmserv* SERV;

#define MAX_WORKER_THREAD_COUNT 20

//
//  class drvcore_serv::drv_request
//

drvcore_serv::drv_request::drv_request(unsigned long type, void* msg, unsigned long length, void* msg_context)
{
    unsigned long required_size = 0;

    switch (type)
    {
    case NXRMDRV_MSG_TYPE_CHECKOBLIGATION:
        required_size = (unsigned long)sizeof(CHECK_OBLIGATION_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_GET_CUSTOMUI:
        required_size = (unsigned long)sizeof(OFFICE_GET_CUSTOMUI_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_SAVEAS_FORECAST:
        required_size = (unsigned long)sizeof(SAVEAS_FORECAST_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_GET_CLASSIFYUI:
        required_size = (unsigned long)sizeof(GET_CLASSIFY_UI_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_BLOCK_NOTIFICATION:
        required_size = (unsigned long)sizeof(BLOCK_NOTIFICATION_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_CHECK_PROTECT_MENU:
        required_size = (unsigned long)sizeof(CHECK_PROTECT_MENU_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_UPDATE_PROTECTEDMODEAPPINFO:
        required_size = (unsigned long)sizeof(UPDATE_PROTECTEDMODEAPPINFO_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_QUERY_PROTECTEDMODEAPPINFO:
        required_size = (unsigned long)sizeof(QUERY_PROTECTEDMODEAPPINFO_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_GET_CTXMENUREGEX:
        required_size = (unsigned long)sizeof(QUERY_CTXMENUREGEX_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_UPDATE_CORE_CTX:
        required_size = (unsigned long)sizeof(UPDATE_CORE_CTX_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_QUERY_CORE_CTX:
        required_size = (unsigned long)sizeof(QUERY_CORE_CTX_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_QUERY_SERVICE:
        required_size = (unsigned long)sizeof(QUERY_SERVICE_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_UPDATE_DWM_WINDOW:
        required_size = (unsigned long)sizeof(UPDATE_DWM_WINDOW_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_UPDATE_OVERLAY_WINDOW:
        required_size = (unsigned long)sizeof(UPDATE_OVERLAY_WINDOW_REQUEST);
        break;
    case NXRMDRV_MSG_TYPE_CHECK_PROCESS_RIGHTS:
        required_size = (unsigned long)sizeof(CHECK_PROCESS_RIGHTS_REQUEST);
        break;
    default:
        break;
    }

    if (0 == required_size) {
        SetLastError(ERROR_INVALID_PARAMETER);
        LOGERROR(NX::string_formater(L"Unknown drv_core request type (%d)", type));
    }

    _request.resize(required_size, 0);
    memcpy(&_request[0], msg, min(required_size, length));
    _type    = type;
    _context = msg_context;
}



//
//  class drvcore_serv
//

drvcore_serv::drvcore_serv() : _running(false)
{
    ::InitializeCriticalSection(&_request_list_lock);
    _request_events[0] = ::CreateEventW(NULL, TRUE, FALSE, NULL);   // stop event
	_request_events[1] = ::CreateSemaphoreW(NULL, 0, LONG_MAX, NULL); ;   // request coming semaphore
}

drvcore_serv::~drvcore_serv()
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
    ::DeleteCriticalSection(&_request_list_lock);
}

void drvcore_serv::start()
{
    static const std::wstring drvman_dll_file(GLOBAL.get_bin_dir() + L"\\nxrmdrvman.dll");

    try {

        unsigned long result = 0;

        // load DLL
        _dll.load(drvman_dll_file);

        // create manager
        _h = _dll.create_manager(drvcore_serv::drv_callback, NxGlobalLogWrite, NxGlobalLogAccept, this);
        if (_h == NULL) {            
            throw NX::exception(ERROR_MSG2("DRVSERV", "fail to create drvman manager (%d)", GetLastError()));
        }

		//set anti-tampering flag after manager created.
#ifdef _DEBUG
		result = _dll.enable_antitampering(FALSE);
#else
		result = _dll.enable_antitampering(SERV->get_service_conf().is_enable_antitampering());
		if (!SERV->get_service_conf().is_enable_antitampering()) {
			LOGINFO(L"DRVSERV: anti-tampering disabled");
		}
#endif
		// start NXRMDRV manager
        result = _dll.start(_h);
        if (0 != result) {
            CloseHandle(_h);
            _h = NULL;
            throw NX::exception(ERROR_MSG2("DRVSERV", "fail to start drvman manager (%d)", result));
        }

        // Set running flag before start worker thread
        _running = true;

        // start worker thread
        int nthreads = 0;
        for (int i = 0; i < 8; i++) {
            try {
                _threads.push_back(std::thread(drvcore_serv::worker, this));
                nthreads++;
            }
            catch (std::exception& e) {
                LOGWARNING(ERROR_MSG2("DRVSERV", "fail to start worker thread #%d (%s)", i, e.what()));
            }
        }

        if (0 == nthreads) {
            throw NX::exception(ERROR_MSG2("DRVSERV", "none worker thread has been started"));
        }

        LOGINFO(NX::string_formater(L"DRVSERV: %d worker threads have been started", nthreads));
    }
    catch (std::exception& e) {
        if (_h != NULL) {
            _dll.stop(_h);
            CloseHandle(_h);
            _h = NULL;
        }
        LOGERROR(e.what());
    }
}

void drvcore_serv::stop()
{
    LOGDEBUG(NX::string_formater(L"drvcore_serv::stop, stop core service"));

    if (!_running) {
        return;
    }

    assert(_running);
    _running = false;

    LOGDEBUG(NX::string_formater(L"drvcore_serv::stop, before stop worker threads"));
    // stop all the request handler threads
    SetEvent(_request_events[0]);
    std::for_each(_threads.begin(), _threads.end(), [&](std::thread& t) {
        if (t.joinable()) {
            LOGDEBUG(NX::string_formater(L"drvcore_serv::stop, before wait for thread to exit [%d]", t.get_id()));
            t.join();
        }
        });
    _threads.clear();
    ResetEvent(_request_events[0]);

    LOGDEBUG(NX::string_formater(L"drvcore_serv::stop, before stop DRVMAN manager"));
    // stop DRVMAN manager
    _dll.stop(_h);
    _dll.close_manager(_h);
    _h = NULL;

    LOGDEBUG(NX::string_formater(L"drvcore_serv::stop, before unload DLL of DRVMAN"));
    // unload DLL
    _dll.unload();
    LOGDEBUG(NX::string_formater(L"drvcore_serv::stop, end"));
}

std::wstring drvcore_serv::drvctl_query_process_info(unsigned long process_id) noexcept
{
    NXRM_PROCESS_ENTRY entry = { 0 };
    std::wstring process_name;

    unsigned long ret = _dll.query_process_info(_h, process_id, &entry);
    if (0 == ret) {
        process_name = entry.process_path;
    }
    else {
        LOGDEBUG(NX::string_formater(L"Query process information failed (pid: %d, error: %d)", process_id, ret));
    }

    return std::move(process_name);
}

void drvcore_serv::drvctl_set_overlay_windows(unsigned long session_id, const std::vector<unsigned long>& hwnds)
{
    unsigned long data_size = (unsigned long)(sizeof(ULONG) * hwnds.size());
    _dll.set_overlay_protected_windows(_h, session_id, (unsigned long*)(hwnds.empty() ? NULL : hwnds.data()), &data_size);
}

void drvcore_serv::drvctl_set_overlay_windows2(unsigned long session_id)
{
    std::vector<unsigned long> hwnds;
    const std::vector<process_protected_window>& protected_windows = GLOBAL.get_process_cache().get_protected_windows(0);   // use process id 0 to collect all the windows
    std::for_each(protected_windows.begin(), protected_windows.end(), [&](const process_protected_window& window) {
        hwnds.push_back(window.get_hwnd());
    });
    drvctl_set_overlay_windows(session_id, hwnds);

    if(NX::dbg::LL_DETAIL <= NxGlobalLog.get_accepted_level()) {
        std::wstring loginfo(NX::string_formater(L"Set overlay window to driver (session %d): ", session_id));
        bool is_first = true;
        std::for_each(hwnds.begin(), hwnds.end(), [&](unsigned long h) {
            if (is_first) {
                is_first = false;
            }
            else {
                loginfo.append(L", ");
            }
            loginfo.append(NX::conversion::to_wstring(h));
        });
        LOGDETAIL(loginfo);
    }
}

void drvcore_serv::drvctl_set_overlay_bitmap_status(unsigned long session_id, bool ready)
{
    _dll.set_overlay_bitmap_status(_h, session_id, ready);
}

void drvcore_serv::worker(drvcore_serv* serv) noexcept
{
    while (serv->is_running()) {

        // wait
        unsigned wait_result = ::WaitForMultipleObjects(2, serv->_request_events, FALSE, INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            // stop event
            LOGDEBUG(NX::string_formater(L"DRVMAN worker thread (%d) quit", GetCurrentThreadId()));
            break;
        }

        //
        if (wait_result != (WAIT_OBJECT_0 + 1)) {
            // error
            LOGWARNING(NX::string_formater(L"DRVMAN worker thread (%d) wait error (result = %d, error = %d)", GetCurrentThreadId(), wait_result, GetLastError()));
            break;
        }

        assert(wait_result == (WAIT_OBJECT_0 + 1));

        // new request comes
        std::shared_ptr<drvcore_serv::drv_request>  request;
        ::EnterCriticalSection(&serv->_request_list_lock);
        if (!serv->_request_list.empty()) {
            request = serv->_request_list.front();
            assert(request != nullptr);
            serv->_request_list.pop();
        }
        ::LeaveCriticalSection(&serv->_request_list_lock);

        if (request == nullptr) {
            continue;
        }

        // handle request
        try {

            switch (request->get_type())
            {
            case NXRMDRV_MSG_TYPE_CHECKOBLIGATION:
                serv->on_check_obligations(request.get());
                break;
            case NXRMDRV_MSG_TYPE_GET_CUSTOMUI:
                serv->on_get_custom_ui(request.get());
                break;
            case NXRMDRV_MSG_TYPE_SAVEAS_FORECAST:
                serv->on_save_as_forecast(request.get());
                break;
            case NXRMDRV_MSG_TYPE_GET_CLASSIFYUI:
                serv->on_get_classify_ui(request.get());
                break;
            case NXRMDRV_MSG_TYPE_BLOCK_NOTIFICATION:
                serv->on_block_notification(request.get());
                break;
            case NXRMDRV_MSG_TYPE_CHECK_PROTECT_MENU:
                serv->on_check_protect_menu(request.get());
                break;
            case NXRMDRV_MSG_TYPE_UPDATE_PROTECTEDMODEAPPINFO:
                serv->on_update_protected_mode_app_info(request.get());
                break;
            case NXRMDRV_MSG_TYPE_QUERY_PROTECTEDMODEAPPINFO:
                serv->on_query_protected_mode_app_info(request.get());
                break;
            case NXRMDRV_MSG_TYPE_GET_CTXMENUREGEX:
                serv->on_get_context_menu_regex(request.get());
                break;
            case NXRMDRV_MSG_TYPE_UPDATE_CORE_CTX:
                serv->on_update_core_context(request.get());
                break;
            case NXRMDRV_MSG_TYPE_QUERY_CORE_CTX:
                serv->on_query_core_context(request.get());
                break;
            case NXRMDRV_MSG_TYPE_QUERY_SERVICE:
                serv->on_query_service(request.get());
                break;
            case NXRMDRV_MSG_TYPE_UPDATE_DWM_WINDOW:
                serv->on_update_dwm_window(request.get());
                break;
            case NXRMDRV_MSG_TYPE_UPDATE_OVERLAY_WINDOW:
                serv->on_update_overlay_window(request.get());
                break;
            case NXRMDRV_MSG_TYPE_CHECK_PROCESS_RIGHTS:
                serv->on_check_process_rights(request.get());
                break;
            default:
                // unknown type
                throw NX::exception(ERROR_MSG("DRVSERV", "Unknown drvman request %08X", request->get_type()));
                break;
            }
        }
        catch (std::exception& e) {
            LOGWARNING(e.what());
        }
        catch (NX::structured_exception& e) {
            LOGCRITICAL(e.exception_message());
        }
        catch (...) {
            LOGCRITICAL(ERROR_MSG("DRVSERV", "Unknown exception when processing request"));
        }
    }
}

unsigned long __stdcall drvcore_serv::drv_callback(unsigned long type, void* msg, unsigned long length, void* msg_context, void* user_context)
{
    unsigned long result = 0;
    drvcore_serv* serv = (drvcore_serv*)user_context;
    
    std::shared_ptr<drv_request> request(new drv_request(type, msg, length, msg_context));
    if (request != NULL && !request->empty()) {
        ::EnterCriticalSection(&serv->_request_list_lock);
        serv->_request_list.push(request);
        ::LeaveCriticalSection(&serv->_request_list_lock);
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

static std::vector<unsigned char> create_watermark_obligation_response(const std::wstring& image_file, int transparency)
{
    std::vector<unsigned char> response;

    if (!image_file.empty()) {
        NXRM_OBLIGATION* ob = NULL;
        const std::wstring& param_image = NX::string_formater(L"%s=%s", OB_OVERLAY_PARAM_IMAGE, image_file.c_str());
        const std::wstring& param_transparency = NX::string_formater(L"%s=%d", OB_OVERLAY_PARAM_TRANSPARENCY, transparency);
        response.resize(sizeof(NXRM_OBLIGATION) + param_image.length() * 2 + 2 + param_transparency.length() * 2 + 4, 0);
        ob = (NXRM_OBLIGATION*)response.data();
        ob->NextOffset = 0;
        ob->Id = OB_ID_OVERLAY;
        memcpy(ob->Params, param_image.c_str(), param_image.length() * 2);
        memcpy(ob->Params + param_image.length() + 1, param_transparency.c_str(), param_transparency.length() * 2);
    }

    return std::move(response);
}

void drvcore_serv::on_check_obligations(drv_request* request)
{
    const CHECK_OBLIGATION_REQUEST* req = (const CHECK_OBLIGATION_REQUEST*)request->get_request();

    assert(request->get_type() == NXRMDRV_MSG_TYPE_CHECKOBLIGATION);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: CHECK_OBLIGATION%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  EvaluationId: %d", req->EvaluationId));
    LOGDETAIL(NX::string_formater(L"  FileName:     %s", req->FileName));
    LOGDETAIL(NX::string_formater(L"  TempPath:     %s", req->TempPath));

    if (request_canceled) {
        return;
    }

    std::vector<unsigned char>  response;

    try {

        ULONG session_id = 0;
        ProcessIdToSessionId(req->ProcessId, &session_id);

        std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(session_id);
        if (sp == nullptr) {
            throw std::exception("Session not exists");
        }

        if (!sp->get_rm_session().is_logged_on()) {
            throw std::exception("user not sign in");
        }
        
        process_record record = GLOBAL.safe_find_process(req->ProcessId);
        if (!boost::algorithm::iends_with(record.get_image_path(), L"\\explorer.exe")) {

            // Ignore explorer
            const std::wstring watermark_image_file(sp->get_rm_session().get_temp_profile_dir() + L"\\watermark.png");
            const int watermark_transparency = sp->get_rm_session().get_watermark_config().get_transparency_ratio();

            if (0xFFFFFFFFFFFFFFFF == req->EvaluationId) {
                // Ask for global obligation
                response = create_watermark_obligation_response(watermark_image_file, watermark_transparency);
            }
            else {
                // Ask for specific obligation
                std::shared_ptr<eval_result> result = sp->get_rm_session().get_eval_cache().get(req->EvaluationId);
                if (result != nullptr) {
                    auto pos = result->get_obligations().find(L"WATERMARK");
                    if (pos != result->get_obligations().end()) {
                        response = create_watermark_obligation_response(watermark_image_file, watermark_transparency);
                    }
                }
                else {
                    LARGE_INTEGER lid;
                    lid.QuadPart = req->EvaluationId;
                    LOGDETAIL(NX::string_formater(L"Cannot find evaluation result (Id: %08X%08X)", lid.HighPart, lid.LowPart));
                }
            }
        }

    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    _dll.post_check_obligations_response(_h, request->get_context(), response.empty() ? NULL : response.data(), (unsigned long)response.size());
}

void drvcore_serv::on_get_custom_ui(drv_request* request)
{
    const OFFICE_GET_CUSTOMUI_REQUEST* req = (const OFFICE_GET_CUSTOMUI_REQUEST*)request->get_request();
    OFFICE_GET_CUSTOMUI_RESPONSE  response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_GET_CUSTOMUI);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: GET_CUSTOMUI%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:      %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:       %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  OfficeVersion:  %s", req->OfficeVersion));
    LOGDETAIL(NX::string_formater(L"  OfficeLanguage: %04X", req->OfficeLanguageId));
    LOGDETAIL(NX::string_formater(L"  OfficeProduct:  %04X", req->OfficeProduct));
    LOGDETAIL(NX::string_formater(L"  TempPath:       %s", req->TempPath));

    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));

    try {


        std::string custom_ui_xml;



        //(VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_default.xml"), OFFICE_LAYOUT_XML, false);
        //(VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_word14.xml"), WORD_LAYOUT_XML_14, false);
        //(VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_excel14.xml"), EXCEL_LAYOUT_XML_14, false);
        //(VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_powerpnt14.xml"), POWERPNT_LAYOUT_XML_14, false);
        //(VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_word15.xml"), WORD_LAYOUT_XML_15, false);
        //(VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_excel15.xml"), EXCEL_LAYOUT_XML_15, false);
        //(VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_powerpnt15.xml"), POWERPNT_LAYOUT_XML_15, false);

        std::wstring custom_ui_file = GLOBAL.get_config_dir() + L"\\custom_ui";

        const std::wstring& process_image = drvctl_query_process_info(req->ProcessId);

        if (boost::algorithm::istarts_with(req->OfficeVersion, L"14")) {
            if (boost::algorithm::iends_with(process_image, L"\\WINWORD.EXE")) {
                custom_ui_xml = WORD_LAYOUT_XML_14;
            }
            else if (boost::algorithm::iends_with(process_image, L"\\EXCEL.EXE")) {
                custom_ui_xml = EXCEL_LAYOUT_XML_14;
            }
            else if (boost::algorithm::iends_with(process_image, L"\\POWERPNT.EXE")) {
                custom_ui_xml = POWERPNT_LAYOUT_XML_14;
            }
            else {
                custom_ui_xml = OFFICE_LAYOUT_XML;
            }
        }
        else if (boost::algorithm::istarts_with(req->OfficeVersion, L"15")) {
            if (boost::algorithm::iends_with(process_image, L"\\WINWORD.EXE")) {
                custom_ui_xml = WORD_LAYOUT_XML_15;
            }
            else if (boost::algorithm::iends_with(process_image, L"\\EXCEL.EXE")) {
                custom_ui_xml = EXCEL_LAYOUT_XML_15;
            }
            else if (boost::algorithm::iends_with(process_image, L"\\POWERPNT.EXE")) {
                custom_ui_xml = POWERPNT_LAYOUT_XML_15;
            }
            else {
                custom_ui_xml = OFFICE_LAYOUT_XML;
            }
        }
        else {
            if (boost::algorithm::iends_with(process_image, L"\\WINWORD.EXE")) {
                custom_ui_xml = WORD_LAYOUT_XML_15;
            }
            else if (boost::algorithm::iends_with(process_image, L"\\EXCEL.EXE")) {
                custom_ui_xml = EXCEL_LAYOUT_XML_15;
            }
            else if (boost::algorithm::iends_with(process_image, L"\\POWERPNT.EXE")) {
                custom_ui_xml = POWERPNT_LAYOUT_XML_15;
            }
            else {
                custom_ui_xml = OFFICE_LAYOUT_XML;
            }
        }

        const std::wstring& temp_file = GLOBAL.get_temp_file_name(req->TempPath);
        if (temp_file.empty()) {
            LOGERROR(NX::string_formater("Fail to create temp file name for customized office layout XML file (%d)", GetLastError()));
        }
        else {
            if (!GLOBAL.generate_file(temp_file, custom_ui_xml, true)) {
                LOGERROR(NX::string_formater("Fail to generate customized office layout XML file (%d)", GetLastError()));
            }
			else {
				memcpy(response.CustomUIFileName,
					   temp_file.c_str(),
					   min(sizeof(response.CustomUIFileName) - sizeof(WCHAR), temp_file.length() * sizeof(WCHAR)));
			}
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    _dll.post_office_get_custom_ui_response(_h, request->get_context(), &response);
}

void drvcore_serv::on_save_as_forecast(drv_request* request)
{
    const SAVEAS_FORECAST_REQUEST* req = (const SAVEAS_FORECAST_REQUEST*)request->get_request();

    assert(request->get_type() == NXRMDRV_MSG_TYPE_SAVEAS_FORECAST);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: SAVEAS_FORECAST%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:%d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId: %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  Source:   %s", req->SourceFileName));
    LOGDETAIL(NX::string_formater(L"  Target:   %s", req->SaveAsFileName));

    if (request_canceled) {
        return;
    }

    try {
        unsigned long result = SERV->get_fltserv().fltctl_set_saveas_forecast(req->ProcessId, req->SourceFileName, req->SaveAsFileName);
        if (0 != result) {
            LOGWARNING(NX::string_formater(L"Fail to set SAVEAS forecast (%d)", result));
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    _dll.post_save_as_forecast_response(_h, request->get_context());
}

void drvcore_serv::on_get_classify_ui(drv_request* request)
{
    const GET_CLASSIFY_UI_REQUEST* req = (const GET_CLASSIFY_UI_REQUEST*)request->get_request();
    GET_CLASSIFY_UI_RESPONSE response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_GET_CLASSIFYUI);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: GET_CLASSIFYUI%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  LanguageId:   %04X", req->LanguageId));
    LOGDETAIL(NX::string_formater(L"  TempPath:     %s", req->TempPath));

    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));

    try {

        std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
        if (sp == nullptr) {
            throw std::exception("Session not exists");
        }

        if (!sp->get_rm_session().is_logged_on()) {
            throw std::exception("user not sign in");
        }

        // get it from user info

    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    _dll.post_get_classify_ui_response(_h, request->get_context(), &response);
}

void drvcore_serv::on_block_notification(drv_request* request)
{
    const BLOCK_NOTIFICATION_REQUEST* req = (const BLOCK_NOTIFICATION_REQUEST*)request->get_request();

    assert(request->get_type() == NXRMDRV_MSG_TYPE_BLOCK_NOTIFICATION);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: BLOCK_NOTIFICATION%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  NoteType:     %d", req->Type));
    LOGDETAIL(NX::string_formater(L"  FileName1:    %s", req->FileName));
    LOGDETAIL(NX::string_formater(L"  FileName2:    %s", req->FileName2));

    if (request_canceled) {
        return;
    }

    // Send notify
    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
    if (sp == nullptr) {
        // Session doesn't exist
        LOGWARNING(NX::string_formater(L"Fail to notify session %d (Session does not exist)", req->SessionId));
        _dll.post_block_notification_response(_h, request->get_context());
        return;
    }

    // send notification
    NX::fs::dos_filepath file_path(req->FileName);
    std::wstring    operation;
    std::wstring    info;
    const std::wstring file(file_path.file_name().full_name());
    int act_operation = 0;

    switch (req->Type)
    {
    case NxrmdrvSaveFileBlocked:
        act_operation = ActEdit;
        operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_SAVE, 64, LANG_NEUTRAL, L"save");
        info = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s this file (%s)", operation.c_str(), file.c_str());
        break;
    case NXrmdrvPrintingBlocked:
        act_operation = ActPrint;
        operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_PRINT, 64, LANG_NEUTRAL, L"print");
        info = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s this file (%s)", operation.c_str(), file.c_str());
        break;
    case NxrmdrvEmbeddedOleObjBlocked:
        act_operation = ActCopyContent;
		operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_INSERT, 64, LANG_NEUTRAL, L"insert content");
		info = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED_FROM, 1024, LANG_NEUTRAL, L"You don't have permission to %s from this file (%s)", operation.c_str(), file.c_str());
		break;
    case NxrmdrvSendMailBlocked:
        act_operation = ActCopyContent;
        operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_EMAIL, 64, LANG_NEUTRAL, L"email");
        info = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s this file (%s)", operation.c_str(), file.c_str());
        break;
    case NxrmdrvExportSlidesBlocked:
        act_operation = ActCopyContent;
        operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_EXPORT, 64, LANG_NEUTRAL, L"export");
        info = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s this file (%s)", operation.c_str(), file.c_str());
        break;
    case NxrmdrvSaveAsToUnprotectedVolume:
        act_operation = ActCopyContent;
        info = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_SAVEAS_DENIED, 1024, LANG_NEUTRAL, L"File (%s) cannot be saved to unprotected location", file.c_str());
        break;
    case NxrmdrvAdobeHookIsNotReady:
        info = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_NOTIFY_ADOBE_PLUGIN_NOT_READY, 1024, LANG_NEUTRAL, L"Rights Management for Adobe is not ready, please wait...");
        break;
	case NxrmdrvAdobeSendEmailBlocked:
		info = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_NOTIFY_BLOCK_ADOBE_SENDEMAIL, 1024, LANG_NEUTRAL, L"Right Management blocked Adobe sending Email in Protected Mode");
		break;
    default:
        operation = NX::RES::LoadMessage(GLOBAL.get_res_module(), IDS_OPERATION_DEFAULT, 64, LANG_NEUTRAL, L"access");
        info = NX::RES::LoadMessageEx(GLOBAL.get_res_module(), IDS_NOTIFY_OPERATION_DENIED, 1024, LANG_NEUTRAL, L"You don't have permission to %s this file (%s)", operation.c_str(), file.c_str());
        break;
    }

    // post response
    _dll.post_block_notification_response(_h, request->get_context());

    // send notify
    sp->get_rm_session().get_app_manager().send_popup_notification(info);

    // log activity
    if (0 != act_operation) {
        NX::NXL::document_context context(file_path.path());
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
                                                                  file_path.path(),
                                                                  proc_record.get_image_path(),
                                                                  proc_record.get_pe_file_info()->get_image_publisher(),
                                                                  std::wstring()));

                sp->get_rm_session().audit_activity(ActEdit, ActDenied, NX::conversion::to_wstring(context.get_duid()), context.get_owner_id(), image_name, file_path.path());
            }
        }
    }
}

void drvcore_serv::on_check_protect_menu(drv_request* request)
{
    const CHECK_PROTECT_MENU_REQUEST* req = (const CHECK_PROTECT_MENU_REQUEST*)request->get_request();
    CHECK_PROTECT_MENU_RESPONSE       response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_CHECK_PROTECT_MENU);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: CHECK_PROTECT_MENU%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));

    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));

    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
    if (sp == nullptr) {
        // Session doesn't exist
        LOGWARNING(NX::string_formater(L"Fail to check protect menu for session %d (Session does not exist)", req->SessionId));
        _dll.post_check_protect_menu_response(_h, request->get_context(), &response);
        return;
    }

    response.EnableProtectMenu = 0;
    if (sp->get_rm_session().is_logged_on() && sp->get_rm_session().get_client_config().is_protection_enabled()) {
        response.EnableProtectMenu = 1;
    }

    _dll.post_check_protect_menu_response(_h, request->get_context(), &response);
}

void drvcore_serv::on_update_protected_mode_app_info(drv_request* request)
{
    const UPDATE_PROTECTEDMODEAPPINFO_REQUEST* req = (const UPDATE_PROTECTEDMODEAPPINFO_REQUEST*)request->get_request();
    UPDATE_PROTECTEDMODEAPPINFO_RESPONSE       response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_UPDATE_PROTECTEDMODEAPPINFO);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: UPDATE_PROTECTEDMODEAPPINFO%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  EvaluationId: %d", req->EvaluationId));
    LOGDETAIL(NX::string_formater(L"  RightsMask:   %08X%08X", (unsigned long)(req->RightsMask >> 32), (unsigned long)req->RightsMask));
    LOGDETAIL(NX::string_formater(L"  CustomRights: %08X%08X", (unsigned long)(req->CustomRights >> 32), (unsigned long)req->CustomRights));
    LOGDETAIL(NX::string_formater(L"  ActiveDocFileName: %s", req->ActiveDocFileName));

    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));
    response.Ack = 1;
}

void drvcore_serv::on_query_protected_mode_app_info(drv_request* request)
{
    const QUERY_PROTECTEDMODEAPPINFO_REQUEST* req = (const QUERY_PROTECTEDMODEAPPINFO_REQUEST*)request->get_request();
    QUERY_PROTECTEDMODEAPPINFO_RESPONSE       response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_QUERY_PROTECTEDMODEAPPINFO);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: QUERY_PROTECTEDMODEAPPINFO%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));

    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));

    try {
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}

void drvcore_serv::on_get_context_menu_regex(drv_request* request)
{
    const QUERY_CTXMENUREGEX_REQUEST* req = (const QUERY_CTXMENUREGEX_REQUEST*)request->get_request();
    QUERY_CTXMENUREGEX_RESPONSE       response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_GET_CTXMENUREGEX);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: GET_CTXMENUREGEX%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));

    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));

    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
    if (sp == nullptr) {
        // Session doesn't exist
        LOGWARNING(NX::string_formater(L"Fail to get context menu regex for session %d (Session does not exist)", req->SessionId));
        _dll.post_get_context_menu_regex_response(_h, request->get_context(), &response);
        return;
    }

    wcsncpy_s(response.CtxMenuRegEx, 1024, sp->get_rm_session().get_client_config().get_protection_filter().c_str(), _TRUNCATE);
    _dll.post_get_context_menu_regex_response(_h, request->get_context(), &response);
}

void drvcore_serv::on_update_core_context(drv_request* request)
{
    const UPDATE_CORE_CTX_REQUEST* req = (const UPDATE_CORE_CTX_REQUEST*)request->get_request();

    assert(request->get_type() == NXRMDRV_MSG_TYPE_UPDATE_CORE_CTX);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: UPDATE_CORE_CTX%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  ModulePath:   %s", req->ModuleFullPath));
    LOGDETAIL(NX::string_formater(L"  RightsChecksum: %08X%08X", (unsigned long)(req->ModuleChecksum >> 32), (unsigned long)req->ModuleChecksum));

    if (request_canceled) {
        return;
    }

    const std::wstring module_path(req->ModuleFullPath);
    std::vector<unsigned __int64> module_context(req->CtxData, req->CtxData + 64);
    const unsigned __int64 module_checksum = req->ModuleChecksum;

    _dll.post_update_core_context_response(_h, request->get_context());

    SERV->get_core_context_cache().insert(module_path, module_checksum, module_context);
    SERV->save_core_context_cache();

}

void drvcore_serv::on_query_core_context(drv_request* request)
{
    const QUERY_CORE_CTX_REQUEST* req = (const QUERY_CORE_CTX_REQUEST*)request->get_request();
    QUERY_CORE_CTX_RESPONSE       response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_QUERY_CORE_CTX);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: QUERY_CORE_CTX%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  ModulePath:   %s", req->ModuleFullPath));
    LOGDETAIL(NX::string_formater(L"  RightsChecksum: %08X%08X", (unsigned long)(req->ModuleChecksum >> 32), (unsigned long)req->ModuleChecksum));

    if (request_canceled) {
        return;
    }

    const std::wstring module_path(req->ModuleFullPath);
    const unsigned __int64 module_checksum = req->ModuleChecksum;

    memset(&response, 0, sizeof(response));
    const std::vector<unsigned __int64>& module_context = SERV->get_core_context_cache().find(module_path, module_checksum);
    if (module_context.size() == 64) {
        response.ModuleChecksum = module_checksum;
        memcpy(response.CtxData, module_context.data(), sizeof(unsigned __int64) * 64);
    }

    _dll.post_query_core_context_response(_h, request->get_context(), &response);
}

void drvcore_serv::on_query_service(drv_request* request)
{
    const QUERY_SERVICE_REQUEST* req = (const QUERY_SERVICE_REQUEST*)request->get_request();
    QUERY_SERVICE_RESPONSE       response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_QUERY_SERVICE);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: QUERY_SERVICE%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  Query:        %S", req->Data));


    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));

    try {

        const std::string& response_data = SERV->process_request(req->SessionId, req->ProcessId, std::string((const char*)req->Data));

        if (response_data.length() < MAX_SERVICE_DATA_LENGTH) {
            memcpy(response.Data, response_data.c_str(), response_data.length());
        }
        else {
            sprintf_s((char*)response.Data, MAX_SERVICE_DATA_LENGTH, "{\"code\":%d, \"message\":\"%s\"}", ERROR_BUFFER_OVERFLOW, "Response exceeds buffer size");
            LOGERROR(NX::string_formater(L"Service response is too long (%d)", response_data.length()));
        }
    }
    catch (const std::exception& e) {
        sprintf_s((char*)response.Data, MAX_SERVICE_DATA_LENGTH, "{\"code\":%d, \"message\":\"%s\"}", ERROR_UNKNOWN_ERROR, e.what());
    }

    _dll.post_query_service_response(_h, request->get_context(), &response);
}

void drvcore_serv::on_update_dwm_window(drv_request* request)
{
    const UPDATE_DWM_WINDOW_REQUEST* req = (const UPDATE_DWM_WINDOW_REQUEST*)request->get_request();

    assert(request->get_type() == NXRMDRV_MSG_TYPE_UPDATE_DWM_WINDOW);

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: UPDATE_DWM_WINDOW"));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  HWND:         %08X", req->hWnd));
    LOGDETAIL(NX::string_formater(L"  Operation:    %s", (NXRMDRV_DWM_WINDOW_ADD == req->Op) ? L"Add" : L"Delete"));

    // set response
    _dll.post_update_dwm_window_response(_h, request->get_context());

    if (NXRMDRV_DWM_WINDOW_ADD == req->Op) {
        GLOBAL.get_process_cache().insert_protected_window(req->ProcessId, req->hWnd, PROTECT_MODE::pm_unknown);
        LOGDETAIL(NX::string_formater(L"Insert DWM window (%08X) from process (%d), mode is unknown", req->hWnd, req->ProcessId));
    }
    else {
        GLOBAL.get_process_cache().remove_protected_window(req->ProcessId, req->hWnd);
        LOGDETAIL(NX::string_formater(L"Remove DWM window (%08X) from process (%d)", req->hWnd, req->ProcessId));
    }

    // update
    process_record record = GLOBAL.safe_find_process(req->ProcessId);
    const unsigned __int64 flags = record.get_flags();
    if(flags64_on(flags, NXRM_PROCESS_FLAG_WITH_NXL_OPENED)) {
        drvctl_set_overlay_windows2(req->SessionId);
    }
}

void drvcore_serv::on_update_overlay_window(drv_request* request)
{
    const UPDATE_OVERLAY_WINDOW_REQUEST* req = (const UPDATE_OVERLAY_WINDOW_REQUEST*)request->get_request();

    assert(request->get_type() == NXRMDRV_MSG_TYPE_UPDATE_OVERLAY_WINDOW);

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: UPDATE_OVERLAY_WINDOW%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));
    LOGDETAIL(NX::string_formater(L"  ThreadId:     %d", req->ThreadId));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  HWND:         %08X", req->hWnd));
    LOGDETAIL(NX::string_formater(L"  Operation:    %s", (NXRMDRV_OVERLAY_WINDOW_ADD == req->Op) ? L"Add" : L"Delete"));

    if (request_canceled) {
        return;
    }

    // Set response
    _dll.post_update_overlay_window_response(_h, request->get_context());

    // update

    process_record record = GLOBAL.safe_find_process(req->ProcessId);
    unsigned __int64 flags = record.get_flags();

    if (NXRMDRV_OVERLAY_WINDOW_ADD == req->Op) {

        if (!flags64_on(flags, NXRM_PROCESS_FLAG_WITH_NXL_OPENED)) {
            flags |= NXRM_PROCESS_FLAG_WITH_NXL_OPENED;
            GLOBAL.get_process_cache().set_process_flags(req->ProcessId, NXRM_PROCESS_FLAG_WITH_NXL_OPENED);
        }
        GLOBAL.get_process_cache().insert_protected_window(req->ProcessId, req->hWnd, PROTECT_MODE::pm_yes);
        LOGDETAIL(NX::string_formater(L"Insert overlay window (%08X) from process (%d), mode is overlay enabled", req->hWnd, req->ProcessId));
    }
    else {
        GLOBAL.get_process_cache().remove_protected_window(req->ProcessId, req->hWnd);
        LOGDETAIL(NX::string_formater(L"Remove overlay window (%08X) from process (%d)", req->hWnd, req->ProcessId));
    }

    // update
    if (flags64_on(flags, NXRM_PROCESS_FLAG_WITH_NXL_OPENED)) {
        drvctl_set_overlay_windows2(req->SessionId);
    }
}

void drvcore_serv::on_check_process_rights(drv_request* request)
{
    const CHECK_PROCESS_RIGHTS_REQUEST* req = (const CHECK_PROCESS_RIGHTS_REQUEST*)request->get_request();
    CHECK_PROCESS_RIGHTS_RESPONSE response = { 0 };

    assert(request->get_type() == NXRMDRV_MSG_TYPE_CHECK_PROCESS_RIGHTS);
    memset(&response, 0, sizeof(response));

    const bool request_canceled = (0 != _dll.is_request_canceled(_h, request->get_context()));

    LOGDETAIL(NX::string_formater(L"DRVSERV Request: CHECK_PROCESS_RIGHTS%s", request_canceled ? L" (CANCELED)" : L" "));
    LOGDETAIL(NX::string_formater(L"  SessionId:    %d", req->SessionId));
    LOGDETAIL(NX::string_formater(L"  ProcessId:    %d", req->ProcessId));

    if (request_canceled) {
        return;
    }

    memset(&response, 0, sizeof(response));

    std::shared_ptr<winsession> sp = SERV->get_win_session_manager().get_session(req->SessionId);
    if (sp == nullptr) {
        // Session doesn't exist
        LOGWARNING(NX::string_formater(L"Session (%d) does not exist, process %d doesn't have any right", req->SessionId, req->ProcessId));
        _dll.post_check_process_rights_response(_h, request->get_context(), &response);
        return;
    }

    const process_record& record = GLOBAL.safe_find_process(req->ProcessId);
    if (!record.empty()) {
        response.Rights = record.get_forbidden_rights().get_allowed_rights();
    }

    // finally
    _dll.post_check_process_rights_response(_h, request->get_context(), &response);
}
