#pragma once
#include "stdafx.h"

//
//   this is the core unique global stucture to representing the os_rmx need abstractions
//   dependencies, rights, and others
//

typedef struct tagActionRight {
	bool enable_view;
	bool enable_edit;
	bool enable_save_as;
	bool enable_print;
	bool enable_clipboard;
	bool enable_screen_capture;

	tagActionRight():
		enable_view(true),
		enable_edit(true),
		enable_save_as(true),
		enable_print(true),
		enable_clipboard(true),
		enable_screen_capture(true)
	{
	}

} ACTION_RIGHT;

typedef enum _RightsType {
	RightsWithWaterMark = 0,
	RightsOnly
} RightsType;

typedef struct _Global {
	_Global() :
		this_dll_module((HMODULE)INVALID_HANDLE_VALUE),
		process_exit_event(NULL),
		nxlfile_open_event(NULL),
		process_id(-1),
		gdiplusToken(NULL),
		shutdown_deamon(false),
		has_obligation_print_watermark(false),
		only_open_normal_file(true),
		_handle_thread_for_edge(NULL),
		is_office_process(false),
		is_sap_veviewer_process(false),
		is_BentleyView_process(false),
		is_adobe_reader_process(false),
		is_adobe_acrobat_process(false),
		is_3d_tool_process(false),
		is_visview_process(false),
		is_jt2go_process(false),
		is_edge_process(false),
		is_match_visview_process_ignore_api_hook(false),
		flag_nxl_context_changed(false),
		flag_draw_watermark(false)
	{
	}

	// global data here
	HMODULE this_dll_module;
	HANDLE	process_exit_event;
	HANDLE	nxlfile_open_event;
	DWORD process_id;
	DWORD main_thread_id;
	std::string process_path;
	std::wstring process_wpath;
	std::string process_name;
	std::wstring process_wname;
	std::recursive_mutex mtx_hwnd;
	std::set<HWND> main_uis;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartupInput gdipulsInput;
	nx::overlay::WatermarkParam default_watermark;
	nx::overlay::WatermarkParam default_print_watermark;

	ACTION_RIGHT		whitelistActionRights;
	ACTION_RIGHT		nxlfileActionRights;

	bool	has_obligation_print_watermark;
	//
	bool	shutdown_deamon;
	bool	only_open_normal_file;
	std::thread _deamon_update_rights_overlay;
	HANDLE _handle_thread_for_edge;


	bool	is_office_process;
	bool	is_sap_veviewer_process;
	bool    is_BentleyView_process;
	bool	is_adobe_reader_process;
	bool    is_adobe_acrobat_process;
	bool	is_3d_tool_process;
	bool    is_visview_process;
	bool    is_jt2go_process;
	bool    is_edge_process;

	bool    is_match_visview_process_ignore_api_hook;

	std::atomic_bool flag_nxl_context_changed;
	std::atomic_bool flag_draw_watermark;

	std::wstring ex_user_email;
}Global;

extern Global global;


void init_common_in_global();
