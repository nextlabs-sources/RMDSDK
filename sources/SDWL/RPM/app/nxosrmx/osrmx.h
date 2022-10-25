#pragma once
#include "stdafx.h"
#include "global_data_model.h"

namespace nx {
	bool Initialize();
	void Uninitialize();

	// talked with PM, first version will ban off SaveAs Feature
	//bool SaveAsFile(const std::wstring& strSrcFile, const std::wstring& strSaveAsNxlFile);

	// notify window life cycle
	void on_hook_window_created(HWND);
	void on_hook_window_created_pre(HWND);
	void on_hook_window_destroyed(HWND);
	void on_hook_msg_queue_returned(LPMSG lpMsg, HWND hWnd);

	// true allow, false deny
	bool on_hook_clipboard_will_open();

	bool on_hook_clipboard_will_write();

	// draw some in hdc if you want to add overlay
	void on_hook_print_page(HDC hdc);
	
	// true allow, false deny
	bool on_hook_file_will_print(const std::wstring& path);
	
	// true allow, false deny
	bool on_hook_saveas_dlg_will_open(bool bnotify = true);

	// an opened file been set as close write
	//void on_hook_after_set_end_of_file(HANDLE hFile);

	void on_hook_window_register_dragdrop( HWND hwnd,  LPDROPTARGET& pDropTarget);

	// true allow, false deny
	bool on_hook_ole_object_will_be_inserted(const wchar_t* path);

	// true allow, false deny
	bool on_email_will_be_send();

	// true allow, false deny
	bool on_hook_mapi32dll_will_be_loaded(const std::string& libName);
	
	// great point to proxy ppv, and detoured specific operation
	// explorer build in window depend on it ,like open/save/print file dialog
	void on_hook_com_object_created(const IID& rclsid, const IID& riid, LPVOID* ppv);

	// true allow, false deny
	bool on_hook_Adobe_Customized_SaveAs_Win_open(HWND hWnd);

	bool on_hook_BentleyView_Customized_Publish_Win_open(HWND hWnd);

	// true allow, false deny
	bool on_hook_jt2go_shfileopration_delete(const std::wstring& path);

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
	// - true if handling is complete (either success or error), and "Next"
	//   should not be invoked.  Call GetLastError() to retrieve error code.
	// - false if handling is not complete, and "Next" should be invoked.
	bool on_hook_copy_file(const std::wstring& srcFilePath, const std::wstring& destFilePath);
	bool on_hook_move_file(const std::wstring& srcFilePath, const std::wstring& destFilePath);
#endif  // #if 0

	void start_deamon_thread(RightsType option = RightsType::RightsWithWaterMark); // 0, default, with watermark; 1, check rights only, no watermark
}
