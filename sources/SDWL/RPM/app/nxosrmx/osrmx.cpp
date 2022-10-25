#include "stdafx.h"
#include <assert.h>
#include <Shlwapi.h>
#include <VersionHelpers.h>
#include <boost/algorithm/string.hpp>
#include "nudf/filesys.hpp"
#include "global_data_model.h"
#include "utils.h"
#include "osrmx.h"
#include "hook.h"
#include "sdk.h"
#include "config.h"
#include "filedialog.h"
#include "droptarget.h"
#include <mapi.h>
//
//  define global param
//
Global global;

static bool bHooked = false;

std::atomic_flag deamon_worker_thread_flag = ATOMIC_FLAG_INIT;


#if 0
static void deamon_update_worker() {
	uint32_t u32Second = 2;
	while (!global.shutdown_deamon)
	{
		try {
			// need find a better singal
			std::this_thread::sleep_for(std::chrono::seconds(u32Second));

			nx::rm::RighMask mask = 0;
			nx::overlay::WatermarkParam view_overlay;
			nx::overlay::WatermarkParam print_overlay;
			if (nx::rm::get_right_watermark(mask, view_overlay, print_overlay))
			{
				global.only_open_normal_file = false;

				// merge global
				global.nxlfileActionRights.enable_view = true;
				global.nxlfileActionRights.enable_save_as = false;   // first version, ban save_as
				global.nxlfileActionRights.enable_edit = (mask & OSRMX_RIGHT_EDIT);
				global.nxlfileActionRights.enable_print = (mask & OSRMX_RIGHT_PRINT);
				global.nxlfileActionRights.enable_clipboard = (mask & OSRMX_RIGHT_CLIPBOARD);
				global.nxlfileActionRights.enable_screen_capture = (mask & OSRMX_RIGHT_SCREENCAPTURE);

				if (mask & OSRMX_RIGHT_OBLIGATION_VIEW_OVERLAY) {
					global.default_watermark = view_overlay;

				}
				if (mask & OSRMX_RIGHT_OBLIGATION_PRINT_OVERLAY) {
					global.has_obligation_print_watermark = true;
					global.default_print_watermark = print_overlay;
				}

				u32Second = 5;
			}
		}
		catch (...) {

		}
	}
}

#else

void print_bool_debug_info(const std::wstring& strPrefix, bool bValue)
{
	std::wstring strMsg = strPrefix + L" : " + std::to_wstring(bValue) + L"\n";
	::OutputDebugStringW(strMsg.c_str());
}

void print_right_debug_info()
{
	::OutputDebugStringW(L"#######ENTER OSRMX RIGHTS INFO#######\n");

	print_bool_debug_info(L"normal file", global.only_open_normal_file);

	::OutputDebugStringW(L"\n");

	print_bool_debug_info(L"whitelist enable_view", global.whitelistActionRights.enable_view);
	print_bool_debug_info(L"whitelist enable_save_as", global.whitelistActionRights.enable_save_as);
	print_bool_debug_info(L"whitelist enable_edit", global.whitelistActionRights.enable_edit);
	print_bool_debug_info(L"whitelist enable_print", global.whitelistActionRights.enable_print);
	print_bool_debug_info(L"whitelist enable_clipboard", global.whitelistActionRights.enable_clipboard);
	print_bool_debug_info(L"whitelist enable_screen_capture", global.whitelistActionRights.enable_screen_capture);

	::OutputDebugStringW(L"\n");

	print_bool_debug_info(L"nxlfile enable_view", global.nxlfileActionRights.enable_view);
	print_bool_debug_info(L"nxlfile enable_save_as", global.nxlfileActionRights.enable_save_as);
	print_bool_debug_info(L"nxlfile enable_edit", global.nxlfileActionRights.enable_edit);
	print_bool_debug_info(L"nxlfile enable_print", global.nxlfileActionRights.enable_print);
	print_bool_debug_info(L"nxlfile enable_clipboard", global.nxlfileActionRights.enable_clipboard);
	print_bool_debug_info(L"nxlfile enable_screen_capture", global.nxlfileActionRights.enable_screen_capture);

	::OutputDebugStringW(L"#######LEAVE OSRMX RIGHTS INFO#######\n");
}

void update_current_process_right_watermark()
{
	nx::rm::RighMask mask = 0;
	nx::overlay::WatermarkParam view_overlay;
	nx::overlay::WatermarkParam print_overlay;
	if (nx::rm::get_right_watermark(mask, view_overlay, print_overlay))
	{
		global.only_open_normal_file = false;

		// merge global
		global.nxlfileActionRights.enable_view = true;
		global.nxlfileActionRights.enable_save_as = false;   // first version, ban save_as
		global.nxlfileActionRights.enable_edit = (mask & OSRMX_RIGHT_EDIT);
		global.nxlfileActionRights.enable_print = (mask & OSRMX_RIGHT_PRINT);
		global.nxlfileActionRights.enable_clipboard = (mask & OSRMX_RIGHT_CLIPBOARD);
		global.nxlfileActionRights.enable_screen_capture = (mask & OSRMX_RIGHT_SCREENCAPTURE);

		if (mask & OSRMX_RIGHT_OBLIGATION_VIEW_OVERLAY) {
			global.default_watermark = view_overlay;

		}
		if (mask & OSRMX_RIGHT_OBLIGATION_PRINT_OVERLAY) {
			global.default_print_watermark = print_overlay;
			global.has_obligation_print_watermark = true;
		}
	}
	print_right_debug_info();
}


void event_mode_update_rights_watermark()
{
	HANDLE hEvents[2] = { 0 };
	hEvents[0] = global.process_exit_event;
	hEvents[1] = global.nxlfile_open_event;

	while (true)
	{
		try {
			DEVLOG(L"deamon_update_worker > WaitForMultipleObjects \n");
			DWORD dwRet = ::WaitForMultipleObjects(2, hEvents, FALSE, 0xFFFFFFFF);
			if (WAIT_TIMEOUT == dwRet)
			{
				DEVLOG(L"deamon_update_worker > Timeout \n");
			}
			else if (WAIT_OBJECT_0 == dwRet)
			{
				DEVLOG(L"deamon_update_worker > Exit Process : Uninitialize \n");
				break;
			}
			else if ((WAIT_OBJECT_0 + 1) == dwRet)
			{
				DEVLOG(L"deamon_update_worker > WAIT_OBJECT_0 \n");
				update_current_process_right_watermark();
				global.flag_nxl_context_changed.store(true);
			}
		}
		catch (...) {
		}
	}
}

void timer_trigger_update_rights_watermark()
{
	while (true)
	{
		try
		{
			DWORD dwRet = ::WaitForSingleObject(global.process_exit_event, 3000);
			if (WAIT_OBJECT_0 == dwRet)
			{
				DEVLOG(L"timer_trigger_update_rights_watermark > Exit Process : Uninitialize \n");
				break;
			}
			else if (WAIT_TIMEOUT == dwRet)
			{
				DEVLOG(L"timer_trigger_update_rights_watermark > update_current_process_right_watermark \n");
				update_current_process_right_watermark();
				global.flag_nxl_context_changed.store(true);
			}
			else
			{
				std::wstring strMsg = L"timer_trigger_update_rights_watermark , dwRet = " + std::to_wstring(dwRet) + L"\n";
				DEVLOG(strMsg.c_str());
				break;
			}
		}
		catch (...)
		{
		}
	}
}

static void deamon_update_worker() {

	::OutputDebugString(L"**** Enter deamon_update_worker! **** \n");

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// always to check rights first
	//	WaitForMultipleObjects might miss the event from service for the 1st call
	update_current_process_right_watermark();

	if (NULL == global.nxlfile_open_event)
	{
		DEVLOG(L"****Enter timer_trigger_update_rights_watermark**** \n");
		timer_trigger_update_rights_watermark();
	}
	else
	{
		DEVLOG(L"****Enter event_mode_update_rights_watermark**** \n");
		event_mode_update_rights_watermark();
	}
}

static DWORD WINAPI deamon_update_worker_for_edge(LPVOID lpParam) {
	::OutputDebugString(L"**** Enter deamon_update_worker_for_edge! **** \n");

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// always to check rights first
	//	WaitForMultipleObjects might miss the event from service for the 1st call
	update_current_process_right_watermark();

	if (NULL == global.nxlfile_open_event)
	{
		DEVLOG(L"****Enter timer_trigger_update_rights_watermark**** \n");
		timer_trigger_update_rights_watermark();
	}
	else
	{
		DEVLOG(L"****Enter event_mode_update_rights_watermark**** \n");
		event_mode_update_rights_watermark();
	}

	return 1;
}

#endif

static BOOL is_win8andabove(void)
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;

	int op1 = VER_GREATER_EQUAL;

	// Initialize the OSVERSIONINFOEX structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 2;
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op1);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op1);

	return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
}

bool hook_api() {

	using namespace nx;
	hook::InitializeMadCHook();

	//
	//  test for visviwer 
	//

	if (global.is_jt2go_process && !global.is_match_visview_process_ignore_api_hook) {
		// Fixed bug 66504 about jt2go data leak.
		if (!hook::HookAPI("Shell32.dll", "SHFileOperationA", (PVOID)hook::Hooked_SHFileOperationA, (PVOID*)&hook::Hooked_SHFileOperationA_Next)) {
			return false;
		}
	}
	else if (global.is_visview_process && global.is_match_visview_process_ignore_api_hook) {
		//::OutputDebugStringW(L"for visview, bypass all api hook\n");
		bHooked = true;
		return true;
	}

	// COMDLG32!GetSaveFileNameW
	if (!hook::HookAPI("Comdlg32", "GetSaveFileNameW", (PVOID)hook::Hooked_GetSaveFileNameW, (PVOID*)&hook::Hooked_GetSaveFileNameW_Next)) {
		return false;
	}

	//User32!CreateWindowExW
	if (!hook::HookAPI("User32", "CreateWindowExW", (PVOID)hook::Hooked_CreateWindowExW, (PVOID*)&hook::Hooked_CreateWindowExW_Next)) {
		return false;
	}

	//User32!CreateWindowExW
	if (!hook::HookAPI("User32", "CreateWindowExA", (PVOID)hook::Hooked_CreateWindowExA, (PVOID*)&hook::Hooked_CreateWindowExA_Next)) {
		return false;
	}

	// by osmond, since we have double checked whether a window is valid, so no need to do the same routine at hooked_destory_window
	//User32!DestroyWindow
	//if (!hook::HookAPI("User32", "DestroyWindow", (PVOID)hook::Hooked_DestroyWindow, (PVOID*)&hook::Hooked_DestroyWindow_Next)) {
	//	return false;
	//}

	//User32!GetMessageW
	if (!hook::HookAPI("User32", "GetMessageW", (PVOID)hook::Hooked_GetMessageW, (PVOID*)&hook::Hooked_GetMessageW_Next)) {
		return false;
	}

	//User32!GetMessageA  jt2go is a very old software which will continued using ANSI version api
	if (!hook::HookAPI("User32", "GetMessageA", (PVOID)hook::Hooked_GetMessageA, (PVOID*)&hook::Hooked_GetMessageA_Next)) {
		return false;
	}

	if (global.is_sap_veviewer_process || global.is_3d_tool_process) {
		//User32!GetMessageA
		if (!hook::HookAPI("User32", "TranslateMessage", (PVOID)hook::Hooked_TranslateMessage, (PVOID*)&hook::Hooked_TranslateMessage_Next)) {
			return false;
		}
	}


	//User32!OpenClipboard
	//if (!hook::HookAPI("User32", "OpenClipboard", (PVOID)hook::Hooked_OpenClipboard, (PVOID*)&hook::Hooked_OpenClipboard_Next)) {
	//	return false;
	//}

	//User32!SetClipboardData
	if (!hook::HookAPI("User32", "SetClipboardData", (PVOID)hook::Hooked_SetClipboardData, (PVOID*)&hook::Hooked_SetClipboardData_Next)) {
		return false;
	}


	// gdi32!StartDocW
	if (!hook::HookAPI("gdi32", "StartDocW", hook::Hooked_StartDocW, (PVOID*)&hook::Hooked_StartDocW_Next)) {
		return false;
	}

	// gdi32!StartDocA
	if (!hook::HookAPI("gdi32", "StartDocA", hook::Hooked_StartDocA, (PVOID*)&hook::Hooked_StartDocA_Next)) {
		return false;
	}
	//StartDocA()

	// gdi32!EndDoc
	if (!hook::HookAPI("gdi32", "EndPage", hook::Hooked_EndPage, (PVOID*)&hook::Hooked_EndPage_Next)) {
		return false;
	}

	if (is_win8andabove())
	{
		//combase!CoCreateInstance
		if (!hook::HookAPI("combase", "CoCreateInstance", (PVOID)hook::Hooked_CoCreateInstance, (PVOID*)&hook::Hooked_CoCreateInstance_Next)) {
			return false;
		}
	}
	else
	{
		//ole32!CoCreateInstance
		if (!hook::HookAPI("ole32", "CoCreateInstance", (PVOID)hook::Hooked_CoCreateInstance, (PVOID*)&hook::Hooked_CoCreateInstance_Next)) {
			return false;
		}
	}

	if (!hook::HookAPI("ole32", "RegisterDragDrop", (PVOID)hook::Hooked_RegisterDragDrop, (PVOID*)&hook::Hooked_RegisterDragDrop_Next)) {
		return false;
	}

	if (!hook::HookAPI("ole32", "OleCreateFromFile", (PVOID)hook::Hooked_OleCreateFromFile, (PVOID*)&hook::Hooked_OleCreateFromFile_Next)) {
		return false;
	}

	if (!hook::HookAPI("ole32", "OleCreateLinkToFile", (PVOID)hook::Hooked_OleCreateLinkToFile, (PVOID*)&hook::Hooked_OleCreateLinkToFile_Next)) {
		return false;
	}

	if (!hook::HookAPI("MAPI32", "MAPISendMail", (PVOID)hook::Hooked_MAPISENDMAIL, (PVOID*)&hook::Hooked_MAPISENDMAIL_Next)) {
		return false;
	}

	if (!hook::HookAPI("kernelbase", "CreateProcessW", (PVOID)hook::Hooked_CreateProcessW, (PVOID*)&hook::Hooked_CreateProcessW_Next)) {
		return false;
	}

	if (!hook::HookAPI("kernelbase", "LoadLibraryExA",(PVOID)hook::Hooked_LoadLibraryExA,(PVOID*)&hook::Hooked_LoadLibraryExA_Next))
	{
		DEVLOG(L"failed in hook loadlibraryExA\n");
		return false;
	}

	if (!hook::HookAPI("kernelbase", "LoadLibraryA", (PVOID)hook::Hooked_LoadLibraryA, (PVOID*)&hook::Hooked_LoadLibraryA_Next))
	{
		return false;
	}

	if (!hook::HookAPI("User32", "ShowWindow", (PVOID)hook::Hooked_ShowWindow, (PVOID*)&hook::Hooked_ShowWindow_Next))
	{
		return false;
	}

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
	if (!hook::HookAPI("kernel32", "CopyFileExW", (PVOID)hook::Hooked_CopyFileExW, (PVOID*)&hook::Hooked_CopyFileExW_Next))
	{
		return false;
	}

	if (!hook::HookAPI("kernel32", "MoveFileExW", (PVOID)hook::Hooked_MoveFileExW, (PVOID*)&hook::Hooked_MoveFileExW_Next))
	{
		return false;
	}
#endif  // #if 0

	bHooked = true;
	return true;
}

void unhook_api() {
	if (bHooked) {
		nx::hook::FinalizeMadCHook();
	}
}



bool nx::Initialize()
{
	//::MessageBox(NULL, L"Initialize", L"Caption", MB_OK);
	init_common_in_global();

	if (!rm::init_sdk()) {
		if (global.is_adobe_reader_process)
		{
			if (!rm::init_sdk_for_adobe()) {
				DEVLOG(L"failed,rm::init_sdk()");
				return false;
			}
		}
	}
	DEVLOG(L"succeed, init_sdk\n");

	if (!rm::set_as_trused_process()) {
		if (!global.is_adobe_reader_process)
		{
			DEVLOG(L"failed,rm::set_as_trused_process()");
			return false;
		}
	}
	DEVLOG(L"succeed, set_as_trusted_process\n");

	if (!hook_api()) {
		DEVLOG(L"failed,hook_api()");
		return false;
	}
	DEVLOG(L"succeed,hook_api\n");

	//
	//  for easy debug or developing, we stored some param in registry
	//
#ifdef  _DEBUG
	config::load_debug_dev_configruaitons();
	//DEVLOG(L"load debug dev back door config ok\n");
#endif  // _DEBUG

	config::load_app_whitelist_config();

	//
	//  fire a deamon thread, may used to update the global 
	// 
	//global._deamon_update_rights_overlay = std::thread(deamon_update_worker);
	//global._deamon_update_rights_overlay;

	return true;
}

void nx::Uninitialize()
{
	try {
		::SetEvent(global.process_exit_event);

		Gdiplus::GdiplusShutdown(global.gdiplusToken);
		unhook_api();
		// join deamon, mark it release
		global._deamon_update_rights_overlay.join();

		if (global.process_exit_event)
		{
			::CloseHandle(global.process_exit_event);
		}

		if (global.nxlfile_open_event)
		{
			::CloseHandle(global.nxlfile_open_event);
		}
		
		if (global._handle_thread_for_edge) {
			DWORD dwRet = ::WaitForSingleObject(global._handle_thread_for_edge, INFINITE);
			if (WAIT_OBJECT_0 == dwRet)
			{
				DEVLOG(L" The thread from edge process exits! \n");
				::CloseHandle(global._handle_thread_for_edge);
			}
		}
	}
	catch (...) {

	}
	DEVLOG(L"done, nx::Uninitialize\n");
}

void initialize_gdiplus()
{
	if (global.gdiplusToken)
		return;

	Gdiplus::Status ret = Gdiplus::GdiplusStartup(&global.gdiplusToken, &global.gdipulsInput, NULL);
	std::wstring strMessage = L"GdiplusStartup initialize Gdi+, status: " + std::to_wstring(ret);
	strMessage += L" current threadid : " + std::to_wstring(::GetCurrentThreadId());
	strMessage += L" main thread id : " + std::to_wstring(global.main_thread_id) + L"\n";
	DEVLOG(strMessage.c_str());
}

void nx::start_deamon_thread(RightsType option)
{
	if (option == RightsType::RightsWithWaterMark)
	{
		global.flag_draw_watermark.store(true);
		static BOOL run_for_watermark_update = false;
		if (run_for_watermark_update == false)
		{
			update_current_process_right_watermark();
			run_for_watermark_update = true;
		}

	}


	if (global._deamon_update_rights_overlay.native_handle() || global._handle_thread_for_edge)
		return;

	if (deamon_worker_thread_flag.test_and_set())
	{
		std::wstring strMsg = L"*** start_deamon_thread deamon_worker_thread_flag threadid = " + std::to_wstring(::GetCurrentThreadId());
		strMsg += L" tick = " + std::to_wstring(::GetTickCount64()) + L"***\n";
		DEVLOG(strMsg.c_str());
		return;
	}

	DWORD dwThreadId = ::GetCurrentThreadId();
	std::wstring strMsg = L"***start a deamon thread to update rights overlay threadid = " + std::to_wstring(::GetCurrentThreadId());
	strMsg += L" tick = " + std::to_wstring(::GetTickCount64()) + L"***\n";
	DEVLOG(strMsg.c_str());

	// For Bajaj solid edge we do special handle and create thread using win api instead of C++ std::thread.
	if (global.is_edge_process) {
		HANDLE hThread = ::CreateThread(NULL, 0, deamon_update_worker_for_edge, NULL, 0, NULL);
		if (hThread != NULL)
		{
			global._handle_thread_for_edge = hThread;
			DEVLOG(L"Create sub thread succeed in Edge.exe process for bajaj!");
		}
	}
	else {
		// For Bajaj environment, Solidedge(edge.exe) cannot open since block here.
		global._deamon_update_rights_overlay = std::thread(deamon_update_worker); 
	}
}

void nx::on_hook_window_created_pre(HWND wnd)
{
	DEVLOG(L"on_hook_window_created_pre\n");

	// extra feature here 
	start_deamon_thread(RightsType::RightsOnly);
}

void nx::on_hook_window_created(HWND wnd)
{
	DEVLOG(L"on_hook_window_created\n");
	// only care about main_thread_ui;
	if (::GetCurrentThreadId() != global.main_thread_id) {
		std::wostringstream ss;
		ss << L"hwnd=" << std::showbase << std::hex << reinterpret_cast<int>(wnd) << std::endl;

		std::wstring strMsg = L"on_hook_window_created::GetCurrentThreadId() != global.main_thread_id, main thread = " + std::to_wstring(global.main_thread_id);
		strMsg += L" , current thread = " + std::to_wstring(::GetCurrentThreadId()) + L" , " + ss.str() + L"\n";
		DEVLOG(strMsg.c_str());

		return;
	}

	// extra feature here 
	initialize_gdiplus();
	start_deamon_thread(RightsType::RightsWithWaterMark);

	HWND target = utils::get_toplevel_window(wnd);
	std::lock_guard<std::recursive_mutex> g(global.mtx_hwnd);
	if (global.main_uis.find(target) != global.main_uis.end()) {
		DEVLOG(L"same window chain, return directly\n");
		return;
	}

#ifdef  _DEBUG
#pragma warning(push)
#pragma warning(disable:4302 4311)

	std::wostringstream ss;
	ss << L"insert a new toplevel window,hwnd=" << std::showbase << std::hex << reinterpret_cast<int>(wnd) << std::endl;
	DEVLOG(ss.str().c_str());

#pragma warning(pop)
#endif  // _DEBUG


	global.main_uis.insert(target);
	if (!global.only_open_normal_file)
	{
		if (!global.whitelistActionRights.enable_screen_capture || !global.nxlfileActionRights.enable_screen_capture)
		{
			// AppStream registry env structure:
			// [HKEY_LOCAL_MACHINE\SOFTWARE\Amazon\MachineImage]
			//    "AMIName" = "Windows_Server-2016-English-Full-Base"
			//	  "AMIVersion" = "2020.12.09"

			auto IsAppStreamEnv = []() {
				nx::utils::Registry::param rp(LR"(SOFTWARE\Amazon\MachineImage)", HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_64KEY);
				nx::utils::Registry r;
				std::wstring aminame;
				return r.get(rp, L"AMIName", aminame) && !aminame.empty();
			};

			if (!IsAppStreamEnv()) 
			{
				// first release, remove prompt user this window has been protected
				//rm::notify_message(config::msg_deny_screen_capture);
				BOOL bRet = ::SetWindowDisplayAffinity(target, WDA_MONITOR);
				std::wstring strMsg = L"*** SetWindowDisplayAffinity";
				if (!bRet)
				{
					DWORD dwErr = ::GetLastError();
					strMsg += L" , error : " + std::to_wstring(dwErr);
				}
				strMsg += L" bRet : " + std::to_wstring(bRet) + L" ***\n";
				DEVLOG(strMsg.c_str());
			}
			else {
				// When target customers use Amazon AppStream, we don't call SetWindowDisplayAffinity(WDA_MONITOR).
				// This is because calling it causes the whole application window to go black.
				//
				// Of course, not calling the function means that screen-capture
				// can no longer be denied even when the user doesn't have the right.
				DEVLOG(L"*** Skip calling SetWindowDisplayAffinity(WDA_MONITOR) in AppStream environment. ***\n");
			}
		}
	}
}

void nx::on_hook_window_destroyed(HWND wnd)
{
	std::lock_guard<std::recursive_mutex> g(global.mtx_hwnd);
	auto iter = global.main_uis.find(wnd);
	if (iter != global.main_uis.end()) {
		rm::clearwatermark(*iter);
		global.main_uis.erase(iter);
	}
}

void handle_screen_capture(const std::set<HWND>& setWnd)
{
	static std::set<HWND> s_setWnd;
	static uint64_t s_u64Tick = ::GetTickCount64();

	uint64_t u64Tick = ::GetTickCount64();
	for (auto h : setWnd)
	{
		s_setWnd.insert(h);
	}

	if (s_setWnd.empty())
	{
		return;
	}

	if (u64Tick - s_u64Tick < 1000 && !global.is_BentleyView_process /* Fix bug 64514 */)
	{
		return;
	}

	std::wstring strMsg = L"***handle_screen_capture ";
	strMsg += L" s_u64Tick : " + std::to_wstring(s_u64Tick) + L" u64Tick : " + std::to_wstring(u64Tick) + L" ***\n";
	DEVLOG(strMsg.c_str());

	s_u64Tick = u64Tick;
	if (global.only_open_normal_file)
	{
		DEVLOG(L"****handle_screen_capture is a normal file****\n");
		return;
	}

	if (global.whitelistActionRights.enable_screen_capture && global.nxlfileActionRights.enable_screen_capture)
	{
		DEVLOG(L"****handle_screen_capture whitelist enable screen capture, nxlfile enable screen capture****\n");
		return;
	}

	for (auto h : s_setWnd)
	{
		if (::IsWindow(h) && ::IsWindowVisible(h))
		{
			auto IsAppStreamEnv = []() {
				nx::utils::Registry::param rp(LR"(SOFTWARE\Amazon\MachineImage)", HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_64KEY);
				nx::utils::Registry r;
				std::wstring aminame;
				return r.get(rp, L"AMIName", aminame) && !aminame.empty();
			};

			if (!IsAppStreamEnv()) {
				// reset anti-screen protection
				BOOL bRet = ::SetWindowDisplayAffinity(h, WDA_MONITOR);
				std::wstring strMsg = L"***handle_screen_capture SetWindowDisplayAffinity";
				if (!bRet)
				{
					DWORD dwErr = ::GetLastError();
					strMsg += L" , error : " + std::to_wstring(dwErr);
				}
				strMsg += L" bRet : " + std::to_wstring(bRet) + L" hwnd : " + std::to_wstring((uint64_t)h) + L" ***\n";
				DEVLOG(strMsg.c_str());
			}
			else {
				// When target customers use Amazon AppStream, we don't call SetWindowDisplayAffinity(WDA_MONITOR).
				// This is because calling it causes the whole application window to go black.
				//
				// Of course, not calling the function means that screen-capture
				// can no longer be denied even when the user doesn't have the right.
				DEVLOG(L"***handle_screen_capture!Skip calling SetWindowDisplayAffinity(WDA_MONITOR) in AppStream environment. ***\n");
			}
		}
	}

	s_setWnd.clear();
}

void nx::on_hook_msg_queue_returned(LPMSG lpMsg, HWND hWnd)
{
	// only care about main_thread_ui;
	if (::GetCurrentThreadId() != global.main_thread_id) {
		return;
	}
	// nxl files context not changed by SDK, that means we dont need to modify UI-derectors
	if (!global.flag_nxl_context_changed.exchange(false)) {
		std::set<HWND> cache;
		handle_screen_capture(cache);
		return;
	}

	std::set<HWND> cache;
	{
		std::lock_guard<std::recursive_mutex> g(global.mtx_hwnd);
		// amend main_uis;
		std::set<HWND> tobeDel;
		for (HWND h : global.main_uis)
		{
			if (!::IsWindow(h) || utils::has_child_style(h)) {
				tobeDel.insert(h);
				rm::clearwatermark(h);
			}
		}
		if (!tobeDel.empty()) {
			for (auto h : tobeDel) {
				global.main_uis.erase(h);
			}
		}
		// copy from global
		cache = global.main_uis;
	}

	//refresh and update all, may need to attach new overlay
	if (!global.only_open_normal_file)
	{
		for (auto h : cache)
		{
			if (::IsWindow(h) && ::IsWindowVisible(h))
			{
				rm::setwatermark(h, global.default_watermark);
			}
		}
	}

	handle_screen_capture(cache);
}

bool nx::on_hook_clipboard_will_open()
{
	if (global.only_open_normal_file)
	{
		return true;
	}

	if (global.whitelistActionRights.enable_clipboard && global.nxlfileActionRights.enable_clipboard) {
		return true;
	}
	else {
		rm::notify_message(config::msg_deny_using_clipboard);
		SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
		return false;
	}
}

bool nx::on_hook_clipboard_will_write()
{
	if (global.only_open_normal_file)
	{
		return true;
	}

	if (global.whitelistActionRights.enable_clipboard && global.nxlfileActionRights.enable_clipboard) {
		return true;
	}
	else {
		rm::notify_message(config::msg_deny_using_clipboard);
		SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
		return false;
	}
}

void nx::on_hook_print_page(HDC hdc)
{
	if (global.has_obligation_print_watermark) {
		initialize_gdiplus();

		nx::overlay::draw_overlay(hdc, global.default_print_watermark);
	}
}

bool nx::on_hook_file_will_print(const std::wstring& path)
{
	if (global.only_open_normal_file)
	{
		return true;
	}

	// for allow
	if (global.whitelistActionRights.enable_print && global.nxlfileActionRights.enable_print)
	{
		std::wstring str{ L"file will be print:" };
		str += path;
		str += L"\n";
		DEVLOG(str.c_str());

		// -feature, send allow print log
		nx::rm::add_print_log(path, true);
		return true;
	}

	// - feature, deny
	SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
	rm::notify_message(config::msg_deny_using_print);
	// - feature, send log
	nx::rm::add_print_log(path, false);
	return false;
}

bool nx::on_hook_saveas_dlg_will_open(bool bnotify)
{
	if (global.only_open_normal_file)
	{
		return true;
	}

	if (global.whitelistActionRights.enable_save_as && global.nxlfileActionRights.enable_save_as)
	{
		return true;
	}

	if (bnotify)
		rm::notify_message(config::msg_deny_show_saveas_dialog);
	return false;
}

void nx::on_hook_window_register_dragdrop(HWND hwnd, LPDROPTARGET& pDropTarget)
{
	auto p = new Proxcy_IDropTarget(pDropTarget);
	pDropTarget = p;
}

// require return false, as deny insert the ole object
bool nx::on_hook_ole_object_will_be_inserted(const wchar_t* p)
{
	if (!p) {
		return true;
	}
	std::wstring path = p;
	if (path.length() < 4) {
		return true;
	}

	if (0 == path.compare(path.length() - 4, 4, L".nxl")) {
		nx::rm::notify_message(L"Embedding NextLabs protected file into another file is not supported.");
		return false;
	}

	if (nx::rm::is_nxl_file(path)) {
		nx::rm::notify_message(L"Embedding NextLabs protected file into another file is not supported.");
		return false;
	}

	return true;
}

bool nx::on_email_will_be_send()
{
	// For the normal file of SancDir, will be looked to have "View" and "Print" rights in default by design,
	// then can acquire the rights by "RPMGetOpenedFileRights" so the flag "only_open_normal_file" is FALSE.
	if (global.only_open_normal_file)
	{
		return true;
	}

	// For this release, we will block this action directly and ignore rights; for the case that user has rights,
	// we should encrypt this file first then allow user to send out, which will be fixed later.
	nx::rm::notify_message(nx::config::msg_deny_common_msg);
	return false;

	/*
	if (global.whitelistActionRights.enable_save_as && global.nxlfileActionRights.enable_clipboard)
	{
		return true;
	}
	else {
		nx::rm::notify_message(nx::config::msg_deny_common_msg);
		return false;
	}

	return true; */
}

// Fix bug 64922
bool nx::on_hook_mapi32dll_will_be_loaded(const std::string& libName)
{
	if (global.only_open_normal_file)
	{
		return true;
	}

	if (0 == _strnicmp(libName.c_str(), "MAPI32.DLL", libName.length())) {
		// notify
		nx::rm::notify_message(nx::config::msg_deny_common_msg);
		return false;
	}

	return true;
}

bool nx::on_hook_BentleyView_Customized_Publish_Win_open(HWND hWnd) {
	if (global.only_open_normal_file)
	{
		return true;
	}

	if (!global.is_BentleyView_process) {
		return true;
	}

	auto is_Publish_imodel_dlg = [](HWND h) {
		char buf[255] = { 0 };
		auto len = ::GetWindowTextA(h, buf, 255);
		return len > 0 && (0 == _stricmp(buf, "Publish iModel(s)"));
	};

	if (is_Publish_imodel_dlg(hWnd)) {
		nx::rm::notify_message(nx::config::msg_deny_common_msg);
		return false; // block display
	}

	return true;
}

bool nx::on_hook_Adobe_Customized_SaveAs_Win_open(HWND hWnd)
{
	if (global.only_open_normal_file)
	{
		return true;
	}

	if (!global.is_adobe_acrobat_process && !global.is_adobe_reader_process) {
		return true;
	}

	auto is_saveAs_dlg = [](HWND h) {
		char buf[255] = { 0 };
		auto len = ::GetWindowTextA(h, buf, 255);
		return len > 0 && (0 == _stricmp(buf, "Save As PDF") || 0 == _stricmp(buf, "Save As"));
	};

	if (is_saveAs_dlg(hWnd)) {
		nx::rm::notify_message(nx::config::msg_deny_common_msg);
		return false; // block display
	}

	return true;
}

void nx::on_hook_com_object_created(const IID& rclsid, const IID& riid, LPVOID* ppv)
{
	if (riid == IID_IFileDialog)
	{
		IFileDialog* pFileDialog = *(IFileDialog**)ppv;
		Proxy_IFileDialog* pCoreIFileDialog = new Proxy_IFileDialog(pFileDialog);

		*ppv = (LPVOID)pCoreIFileDialog;
		return;
	}

	if (riid == IID_IFileSaveDialog)
	{
		IFileSaveDialog* pFileSaveDialog = *(IFileSaveDialog**)ppv;
		Proxy_IFileSaveDialog* pCoreIFileSaveDialog = new Proxy_IFileSaveDialog(pFileSaveDialog);

		*ppv = (LPVOID)pCoreIFileSaveDialog;

		return;
	}

	if (rclsid == CLSID_FileSaveDialog) {

		IFileSaveDialog* pFileSaveDialog = *(IFileSaveDialog**)ppv;
		Proxy_IFileSaveDialog* pCoreIFileSaveDialog = new Proxy_IFileSaveDialog(pFileSaveDialog);

		*ppv = (LPVOID)pCoreIFileSaveDialog;

		return;
	}

}

bool nx::on_hook_jt2go_shfileopration_delete(const std::wstring& path) {
	if (!global.is_jt2go_process) {
		return true;
	}

	bool ret = nx::rm::is_nxlfile_in_rpm_folder(path);
	if (ret) {
		nx::rm::notify_message(L"Deleting NXL file here is not allowed.");
	}

	return !ret;
}



// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
typedef struct _RPMFolderRelation
{
	bool bUnknownRelation : 1;
	bool bNormalFolder : 1;

	// For RPM folder
	struct {
		bool bRPMAncestralFolder : 1;
		bool bRPMFolder : 1;
		bool bRPMInheritedFolder : 1;
	};

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	// For Sanctuary folder
	struct {
		bool bSanctuaryAncestralFolder : 1;
		bool bSanctuaryFolder : 1;
		bool bSanctuaryInheritedFolder : 1;
	};
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	wchar_t BoolToWchar(bool b) const {
		return b ? L't' : L'f';
	}
	std::wstring ToString(void) const {
		return std::wstring() +
			L'{' +
			BoolToWchar(bUnknownRelation) + L',' +
			BoolToWchar(bNormalFolder) + L',' +
			L'{' +
			BoolToWchar(bRPMAncestralFolder) + L',' +
			BoolToWchar(bRPMFolder) + L',' +
			BoolToWchar(bRPMInheritedFolder) +
			L'}' +
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			L',' + L'{' +
			BoolToWchar(bSanctuaryAncestralFolder) + L',' +
			BoolToWchar(bSanctuaryFolder) + L',' +
			BoolToWchar(bSanctuaryInheritedFolder) +
			L'}' +
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			L'}';
	}
} RPMFolderRelation;

typedef enum _RPMFileRelation
{
	UnknownRelation,
	NXLFileInNormalDir,
	NonNXLFileInNormalDir,
	NXLFileInRPMDir,
	NonNXLFileInRPMDir,
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	NXLFileInSanctuaryDir,
	NonNXLFileInSanctuaryDir,
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	RPMFileRelationMax
} RPMFileRelation;


#ifdef  _DEBUG

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

void AssertRPMFolderRelation(RPMFolderRelation r)
{
	if (r.bUnknownRelation)
	{
		assert(!r.bNormalFolder &&
			!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder) &&
			!(r.bSanctuaryAncestralFolder || r.bSanctuaryFolder || r.bSanctuaryInheritedFolder));
	}
	else if (r.bNormalFolder)
	{
		assert(!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder) &&
			!(r.bSanctuaryAncestralFolder || r.bSanctuaryFolder || r.bSanctuaryInheritedFolder));
	}
	else if (r.bRPMFolder || r.bRPMInheritedFolder)
	{
		assert(!(r.bSanctuaryAncestralFolder || r.bSanctuaryFolder || r.bSanctuaryInheritedFolder));
	}
	else if (r.bSanctuaryFolder || r.bSanctuaryInheritedFolder)
	{
		assert(!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder));
	}
	else
	{
		assert(r.bRPMAncestralFolder || r.bSanctuaryAncestralFolder);
	}
}

#else   // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

void AssertRPMFolderRelation(RPMFolderRelation r)
{
	if (r.bUnknownRelation)
	{
		assert(!r.bNormalFolder &&
			!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder));
	}
	else if (r.bNormalFolder)
	{
		assert(!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder));
	}
	else if (r.bRPMFolder || r.bRPMInheritedFolder)
	{
		// Nothing
	}
	else
	{
		assert(r.bRPMAncestralFolder);
	}
}

#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

#endif // _DEBUG

void AssertRPMFileRelation(RPMFileRelation r)
{
	assert(r >= 0 && r < RPMFileRelationMax);
}

RPMFolderRelation GetFolderRelation(const std::wstring& folderPath, SDRmRPMFolderOption *pRpmFolderOption, std::wstring *pFileTags)
{
	if (pFileTags != NULL)
	{
		pFileTags->clear();
	}

	uint32_t dirstatus = 0;
	SDRmRPMFolderOption tempOption;
	std::wstring tempTags;
	SDWLResult res;
	RPMFolderRelation relation = { false,
	                               true,                    // set bNormalFolder to true
	                               {false, false, false},
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	                               {false, false, false}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	};

	// IsRPMFolder and IsSanctuaryFolder only support drive letter paths.  If
	// folderPath is UNC path, it must be a network path which cannot be RPM
	// or Sanctuary dir anyway.
	if (PathIsUNCW(folderPath.c_str()))
	{
		return relation;
	}

	if (!nx::rm::is_rpm_folder(folderPath, dirstatus, tempOption, tempTags))
	{
		std::wstring strMsg = L"IsRPMFolder failed, res=";
		strMsg += res.ToString();
		DEVLOG(strMsg.c_str());

		relation = {
		    true,                   // set bUnknownRelation = true
		    false,
		    {false, false, false},
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		    {false, false, false}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		};
		return relation;
	}
	else
	{
		if (pRpmFolderOption != NULL)
		{
			*pRpmFolderOption = tempOption;
		}

		if (pFileTags != NULL)
		{
			*pFileTags = tempTags;
		}

		if (dirstatus & RPM_SAFEDIRRELATION_SAFE_DIR)
		{
			relation.bRPMFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR)
		{
			relation.bRPMAncestralFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)
		{
			relation.bRPMInheritedFolder = true;
			relation.bNormalFolder = false;
		}
	}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	// Since a folder cannot be both RPM and Sanctuary, we stop here if there
	// is already an RPM relationship so as to avoid spending time calling
	// pInstance->IsSanctuaryFolder().
	if (!relation.bNormalFolder)
	{
		return relation;
	}

	if (!nx::rm::is_sanc_folder(folderPath, dirstatus, tempTags))
	{
		std::wstring strMsg = L"IsSanctuaryFolder failed, res=";
		strMsg += res.ToString();
		DEVLOG(strMsg.c_str());

		relation = {
		    true,                   // set bUnknownRelation = true
		    false,
		    {false, false, false},
		    {false, false, false}
		};
		return relation;
	}
	else
	{
		if (pFileTags != NULL)
		{
			*pFileTags = tempTags;
		}

		if (dirstatus & RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR)
		{
			relation.bSanctuaryFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR)
		{
			relation.bSanctuaryAncestralFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR)
		{
			relation.bSanctuaryInheritedFolder = true;
			relation.bNormalFolder = false;
		}
	}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	return relation;
}

RPMFileRelation GetFileRelation(const std::wstring& filePath, BOOL bVerify, SDRmRPMFolderOption *pRpmFolderOption = NULL, std::wstring *pFileTags = NULL)
{
	NX::fs::dos_fullfilepath input_path(filePath);
	if (bVerify)
	{
		if (PathIsDirectoryW(input_path.global_dos_path().c_str()))
		{
			return NonNXLFileInNormalDir;
		}
	}

	std::wstring folderPath;

	size_t pos = filePath.rfind(L'\\');
	if (pos == std::wstring::npos)
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			return NXLFileInNormalDir;
		}
		else
		{
			return NonNXLFileInNormalDir;
		}
	}
	else
	{
		folderPath = filePath.substr(0, pos);
	}

	RPMFolderRelation folderRelation = GetFolderRelation(folderPath, pRpmFolderOption, pFileTags);
#ifdef  _DEBUG
	AssertRPMFolderRelation(folderRelation);
#endif

	if (folderRelation.bRPMFolder || folderRelation.bRPMInheritedFolder)
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			return NXLFileInRPMDir;
		}

		std::wstring fileWithNxlPath = input_path.global_dos_path() + L".nxl";

		WIN32_FIND_DATAW findFileData = { 0 };
		HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			return NonNXLFileInRPMDir;
		}
		else
		{
			FindClose(hFind);
			return NXLFileInRPMDir;
		}
	}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	else if (folderRelation.bSanctuaryFolder || folderRelation.bSanctuaryInheritedFolder)
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			return NXLFileInSanctuaryDir;
		}
		else
		{
			return NonNXLFileInSanctuaryDir;
		}
	}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	else if (folderRelation.bUnknownRelation)
	{
		return UnknownRelation;
	}
	else
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			return NXLFileInNormalDir;
		}
		else
		{
			return NonNXLFileInNormalDir;
		}
	}
}

bool nx::on_hook_copy_file(const std::wstring& srcFilePath, const std::wstring& destFilePath)
{
	RPMFileRelation srcFileRpmRelation = GetFileRelation(srcFilePath, FALSE);
#ifdef  _DEBUG
	AssertRPMFileRelation(srcFileRpmRelation);
#endif

	NX::fs::dos_fullfilepath desDosFullFilePath(destFilePath);
	std::wstring desFolderPath = desDosFullFilePath.file_dir();

	SDRmRPMFolderOption desFolderRpmOption;
	std::wstring desFolderFileTags;
	RPMFolderRelation desFolderRpmRelation = GetFolderRelation(desFolderPath, &desFolderRpmOption, &desFolderFileTags);
#ifdef  _DEBUG
	AssertRPMFolderRelation(desFolderRpmRelation);
#endif

	if (srcFileRpmRelation == UnknownRelation || desFolderRpmRelation.bUnknownRelation)
	{
		// Error occurred while trying to determine the relationship.
		SetLastError(ERROR_ACCESS_DENIED);
		return true;
	}

	if ((srcFileRpmRelation == NonNXLFileInNormalDir || srcFileRpmRelation == NonNXLFileInRPMDir) &&
		((desFolderRpmRelation.bRPMFolder || desFolderRpmRelation.bRPMInheritedFolder) && (desFolderRpmOption & RPMFOLDER_AUTOPROTECT)))
	{
		SDWLResult res;

		// Generate the destination file by protecting the source file
		std::wstring desTempNxlFilePath = desFolderPath;
		const std::vector<SDRmFileRight> rights;
		const SDR_WATERMARK_INFO watermarkInfo;
		const SDR_Expiration expire;
		const std::string fileTagsUtf8 = NX::conversion::utf16_to_utf8(desFolderFileTags);
		res = nx::rm::protect_file(srcFilePath, desTempNxlFilePath, rights, watermarkInfo, expire, fileTagsUtf8);
		SetLastError(res.GetCode());

		return true;
	}

	return false;
}

bool nx::on_hook_move_file(const std::wstring& srcFilePath, const std::wstring& destFilePath)
{
	RPMFileRelation srcFileRpmRelation = GetFileRelation(srcFilePath, FALSE);
#ifdef  _DEBUG
	AssertRPMFileRelation(srcFileRpmRelation);
#endif

	NX::fs::dos_fullfilepath desDosFullFilePath(destFilePath);
	std::wstring desFolderPath = desDosFullFilePath.file_dir();

	SDRmRPMFolderOption desFolderRpmOption;
	std::wstring desFolderFileTags;
	RPMFolderRelation desFolderRpmRelation = GetFolderRelation(desFolderPath, &desFolderRpmOption, &desFolderFileTags);
#ifdef  _DEBUG
	AssertRPMFolderRelation(desFolderRpmRelation);
#endif

	if (srcFileRpmRelation == UnknownRelation || desFolderRpmRelation.bUnknownRelation)
	{
		// Error occurred while trying to determine the relationship.
		SetLastError(ERROR_ACCESS_DENIED);
		return true;
	}

	if ((srcFileRpmRelation == NonNXLFileInNormalDir || srcFileRpmRelation == NonNXLFileInRPMDir) &&
		((desFolderRpmRelation.bRPMFolder || desFolderRpmRelation.bRPMInheritedFolder) && (desFolderRpmOption & RPMFOLDER_AUTOPROTECT)))
	{
		SDWLResult res;

		// Generate the destination file by protecting the source file
		std::wstring desTempNxlFilePath = desFolderPath;
		const std::vector<SDRmFileRight> rights;
		const SDR_WATERMARK_INFO watermarkInfo;
		const SDR_Expiration expire;
		const std::string fileTagsUtf8 = NX::conversion::utf16_to_utf8(desFolderFileTags);
		res = nx::rm::protect_file(srcFilePath, desTempNxlFilePath, rights, watermarkInfo, expire, fileTagsUtf8);
		if (!res)
		{
			SetLastError(res.GetCode());
			return true;
		}

		// Delete the source file.
		if (DeleteFile(srcFilePath.c_str()))
		{
			SetLastError(ERROR_SUCCESS);
		}
		else
		{
			// Last-error was already set by DeleteFile().
		}

		return true;
	}

	return false;
}
#endif  // #if 0
