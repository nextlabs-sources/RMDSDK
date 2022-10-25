
#include "stdafx.h"
#include "Shlobj.h"
#include "utils.h"
#include "madCHook.h"
#include "hook.h"
#include "osrmx.h"


nx::utils::recursion_control rc; // control hook

//
NXHOOK_NAMESPACE
//
namespace {
	bool match_prefilterd_str_for_createwidnow(const wchar_t* target_str) {
		if (target_str == NULL) {
			return false;
		}

#pragma warning(push)
#pragma warning(disable:4302 4311)
		unsigned int l = (unsigned int)(void*)target_str;
		unsigned int r = (unsigned int)(1 << 16);
		
		if (l <r ) {
			return false;
		}
#pragma warning(pop)


		static const std::vector<std::wstring> predefined{
			L"GDI+ Window",
			L"GDI+ Hook Window Class",
			L"tooltips_class32",
			L"WorkerW",
			L"IME",
			L"#32769",
			L"SysDragImage",
			L"Auto-Suggest Dropdown",
			L"SHELLDLL_DefView",
			L"SysListView32",
			L"SysHeader32"
			L"KbxLabelClass"
			// sublime
			L"com.sublimetext.three.timer_window",
			//chrome
			L"Base_PowerMessageWindow",
			L"Chrome_SystemMessageWindow",
			// qt
			L"Qt5QWindowToolTipSaveBits",
			L"Qt5QWindowPopupDropShadowSaveBits",
			//autodesk dwgviewr.exe
			L"AdSplashWindowClass"
			//L"",
		};

		for (auto s : predefined) {
			if (0 == s.compare(target_str)) {
				return true;
			}
		}		
		return false;
	}
}


BOOL  APIENTRY Hooked_GetSaveFileNameW(LPOPENFILENAMEW arg) {
	DEVLOG(L"on Hooked_GetSaveFileNameW\n");
	if (rc.is_disabled()) {
		return Hooked_GetSaveFileNameW_Next(arg);
	}
	utils::recursion_control_auto auto_disable(rc);

	if (on_hook_saveas_dlg_will_open()) {
		return Hooked_GetSaveFileNameW_Next(arg);
	}
	else {
		return false;
	}
}

// find window not ws_child, 
HWND WINAPI Hooked_CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	DEVLOG(L"on Hooked_CreateWindowExW\n");

	if (rc.is_disabled()) {
		return Hooked_CreateWindowExW_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	// call pre for initialization on Rights Query only
	on_hook_window_created_pre(hWndParent);

	if (hWndParent != NULL) {
		return Hooked_CreateWindowExW_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	// we only care who is not ws_child
	if (dwStyle & WS_CHILDWINDOW) {
		return Hooked_CreateWindowExW_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	if (match_prefilterd_str_for_createwidnow(lpClassName) || match_prefilterd_str_for_createwidnow(lpWindowName)) {
		return Hooked_CreateWindowExW_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	// SAP main window is popued 
	/*if (dwStyle & WS_POPUP) {
		return Hooked_CreateWindowExW_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}*/

	utils::recursion_control_auto auto_disable(rc);

	HWND rt = Hooked_CreateWindowExW_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (rt != NULL) {
		on_hook_window_created(rt);
	}
	return rt;
}

HWND __stdcall Hooked_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	DEVLOG(L"on Hooked_CreateWindowExA\n");

	if (rc.is_disabled()) {
		return Hooked_CreateWindowExA_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	on_hook_window_created_pre(hWndParent);

	if (hWndParent != NULL) {
		return Hooked_CreateWindowExA_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	// we only care who is not ws_child
	if (dwStyle & WS_CHILDWINDOW) {
		return Hooked_CreateWindowExA_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	//if (match_prefilterd_classname_for_createwidnow(lpClassName)) {
	//	return Hooked_CreateWindowExA_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	//}

	// SAP main window is popued 
	/*if (dwStyle & WS_POPUP) {
		return Hooked_CreateWindowExA_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}*/

	utils::recursion_control_auto auto_disable(rc);

	HWND rt = Hooked_CreateWindowExA_Next(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (rt != NULL) {
		on_hook_window_created(rt);
	}
	return rt;
}

BOOL WINAPI Hooked_DestroyWindow(HWND hWnd) {
	DEVLOG(L"on Hooked_DestroyWindow\n");
	if (rc.is_disabled()) {
		return Hooked_DestroyWindow_Next(hWnd);
	}

	utils::recursion_control_auto auto_disable(rc);
	on_hook_window_destroyed(hWnd);
	

	return  Hooked_DestroyWindow_Next(hWnd);
}

BOOL __stdcall Hooked_GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	//DEVLOG(L"on Hooked_GetMessageW\n");
	if (rc.is_disabled()) {
		return Hooked_GetMessageW_Next(lpMsg,hWnd,wMsgFilterMin,wMsgFilterMax);
	}
	if (lpMsg == NULL) {
		return Hooked_GetMessageW_Next(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	}
	auto rt = Hooked_GetMessageW_Next(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	// good place to tell UI thread do some thing
	utils::recursion_control_auto auto_disable(rc);
	on_hook_msg_queue_returned(lpMsg, hWnd);
	return rt;
}

BOOL __stdcall Hooked_GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	DEVLOG(L"on Hooked_GetMessageA\n");
	if (rc.is_disabled()) {
		return Hooked_GetMessageA_Next(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	}
	if (lpMsg == NULL) {
		return Hooked_GetMessageA_Next(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	}
	auto rt = Hooked_GetMessageA_Next(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	// good place to tell UI thread do some thing
	utils::recursion_control_auto auto_disable(rc);
	on_hook_msg_queue_returned(lpMsg, hWnd);
	return rt;
}

BOOL __stdcall Hooked_TranslateMessage(const MSG* lpMsg)
{
	DEVLOG(L"on Hooked_TranslateMessage\n");
	if (rc.is_disabled()) {
		return Hooked_TranslateMessage_Next(lpMsg);
	}
	utils::recursion_control_auto auto_disable(rc);
	// good place to tell UI thread do some thing
	on_hook_msg_queue_returned((MSG*)lpMsg, NULL);
	return Hooked_TranslateMessage_Next(lpMsg);
}

BOOL WINAPI Hooked_OpenClipboard(HWND hWndNewOwner) {
	DEVLOG(L"on Hooked_OpenClipboard\n");
	if (rc.is_disabled()) {
		return Hooked_OpenClipboard_Next(hWndNewOwner);
	}
	utils::recursion_control_auto auto_disable(rc);


	if (on_hook_clipboard_will_open()) {
		return Hooked_OpenClipboard_Next(hWndNewOwner);
	}
	else {
		return false;
	}

}

HANDLE __stdcall Hooked_SetClipboardData(UINT uFormat, HANDLE hMem)
{
	DEVLOG(L"on Hooked_SetClipboardData\n");
	if (rc.is_disabled()) {
		return Hooked_SetClipboardData_Next(uFormat,hMem);
	}
	utils::recursion_control_auto auto_disable(rc);


	if (on_hook_clipboard_will_write()) {
		return Hooked_SetClipboardData_Next(uFormat, hMem);
	}
	else {
		::SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
		return NULL;
	}
}


int WINAPI Hooked_EndPage(HDC hdc) {
	DEVLOG(L"on Hooked_EndPage\n");
	if (rc.is_disabled()) {
		return Hooked_EndPage_Next(hdc);
	}
	utils::recursion_control_auto auto_disable(rc);

	on_hook_print_page(hdc);

	return Hooked_EndPage_Next(hdc);
}

int WINAPI Hooked_StartDocW(HDC hdc, const DOCINFOW* lpdi) {
	DEVLOG(L"on Hooked_StartDocW\n");
	if (rc.is_disabled()) {
		return Hooked_StartDocW_Next(hdc, lpdi);
	}
		
	utils::recursion_control_auto auto_disable(rc);
	if (!on_hook_file_will_print(lpdi->lpszDocName ? lpdi->lpszDocName : L"")) {
		::SetLastError(ERROR_ACCESS_DENIED);
		return 0;
	}

	// as i tested, attach overlay job should be done in Hooked_EndPage
	return Hooked_StartDocW_Next(hdc, lpdi);
}

int __stdcall Hooked_StartDocA(HDC hdc, const DOCINFOA* lpdi)
{
	DEVLOG(L"on Hooked_StartDocA\n");
	if (rc.is_disabled()) {
		return Hooked_StartDocA_Next(hdc, lpdi);
	}
	
	utils::recursion_control_auto auto_disable(rc);
	if (!on_hook_file_will_print(lpdi->lpszDocName ? utils::to_wstr(lpdi->lpszDocName) : L"")) {
		::SetLastError(ERROR_ACCESS_DENIED);
		return 0;
	}
	// as i tested, attach overlay job should be done in Hooked_EndPage
	return Hooked_StartDocA_Next(hdc, lpdi);
}

HRESULT WINAPI Hooked_CoCreateInstance(
	IN REFCLSID		rclsid,
	IN LPUNKNOWN	pUnkOuter,
	IN DWORD		dwClsContext,
	IN REFIID		riid,
	OUT LPVOID FAR* ppv)
{
	HRESULT hr = S_FALSE;

	DEVLOG(L"on Hooked_CoCreateInstance\n");
	if (rc.is_disabled()) {
		return Hooked_CoCreateInstance_Next(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	}
	utils::recursion_control_auto auto_disable(rc);
	rc.process_disable();	// comment by osmond, CoCreateInstance must consider about Mutil_Thread 
	
	if (rclsid == CLSID_FileSaveDialog || riid == IID_IFileSaveDialog) // CLSID_FileSaveDialog vs IID_IFileSaveDialog
	{
		if (on_hook_saveas_dlg_will_open()) {
			hr = Hooked_CoCreateInstance_Next(rclsid, pUnkOuter, dwClsContext, riid, ppv);
		}
		else
		{
			hr = Hooked_CoCreateInstance_Next(rclsid, pUnkOuter, dwClsContext, riid, ppv);
			on_hook_com_object_created(rclsid, riid, ppv);
		}
	} 

	//
	// the original file is to handle VEViewer, which create OLE object of Outlook
	// Finally, we think this is common case to block outlook email
	//
	// else if (global.is_sap_veviewer_process && utils::Is_Outlook_clsid(rclsid))
	//
	// Handle Veviewer Email, It looks that veviewer probably won't send email via 'MAPISendMail' 
	// when user's machine installed Outlook(it will via 'MAPISendMail' when not install outlook).
	// Now find intercept it here, will go on to enter 'MAPISendMail' to handle.
	else if (utils::Is_Outlook_clsid(rclsid))
	{
		if (nx::on_email_will_be_send()) {
			return S_FALSE;
		}
	}
	else
		hr = Hooked_CoCreateInstance_Next(rclsid, pUnkOuter, dwClsContext, riid, ppv);

	if (FAILED(hr)) {
		return hr;
	}
	
	//on_hook_com_object_created(rclsid, riid, ppv);

	return hr;
}

HRESULT __stdcall Hooked_RegisterDragDrop(IN HWND hwnd, IN LPDROPTARGET pDropTarget)
{
	HRESULT rt = E_NOTIMPL;
	DEVLOG(L"on Hooked_RegisterDragDrop\n");
	// by osmond. the orginal api Ole32!RegisterDragDrop will be called in CreateWindowEx,so do not using rc control
	//if (rc.is_disabled()) {
	//	return Hooked_RegisterDragDrop_Next(hwnd, pDropTarget);
	//}
	//utils::recursion_control_auto auto_disable(rc);
	// give client a change to replace pDropTarget
	on_hook_window_register_dragdrop(hwnd, pDropTarget);
	rt= Hooked_RegisterDragDrop_Next(hwnd, pDropTarget);
	return rt;
}

HRESULT __stdcall Hooked_OleCreateFromFile(IN REFCLSID rclsid, IN LPCOLESTR lpszFileName, IN REFIID riid, IN DWORD renderopt, IN LPFORMATETC lpFormatEtc, IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID* ppvObj)
{
	DEVLOG(L"on Hooked_OleCreateFromFile\n");
	if (rc.is_disabled()) {
		return Hooked_OleCreateFromFile_Next(rclsid, lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}
	utils::recursion_control_auto auto_disable(rc);

	if (!lpszFileName) {
		return Hooked_OleCreateFromFile_Next(rclsid, lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}

	if (on_hook_ole_object_will_be_inserted(lpszFileName)) {
		return Hooked_OleCreateFromFile_Next(rclsid, lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}
	else {
		return STG_E_FILENOTFOUND;
	}
}

HRESULT __stdcall Hooked_OleCreateLinkToFile(IN LPCOLESTR lpszFileName, IN REFIID riid, IN DWORD renderopt, IN LPFORMATETC lpFormatEtc, IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID* ppvObj)
{
	DEVLOG(L"on Hooked_OleCreateLinkToFile\n");
	if (rc.is_disabled()) {
		return Hooked_OleCreateLinkToFile_Next(lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}
	utils::recursion_control_auto auto_disable(rc);

	if (!lpszFileName) {
		return Hooked_OleCreateLinkToFile_Next(lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}

	if (on_hook_ole_object_will_be_inserted(lpszFileName)) {
		return Hooked_OleCreateLinkToFile_Next(lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}
	else {
		return STG_E_FILENOTFOUND;
	}
}

ULONG WINAPI Hooked_MAPISENDMAIL(LHANDLE lhSession, ULONG_PTR ulUIParam, lpMapiMessage lpMessage, FLAGS flFlags, ULONG ulReserved)
{
	DEVLOG(L"on Hooked_MAPISENDMAIL\n");
	if (rc.is_disabled()) {
		return Hooked_MAPISENDMAIL_Next(lhSession, ulUIParam, lpMessage, flFlags,ulReserved);
	}
	
	if (on_email_will_be_send()) {
		return Hooked_MAPISENDMAIL_Next(lhSession, ulUIParam, lpMessage, flFlags, ulReserved);
	}
	else {
		return MAPI_E_FAILURE;
	}
}

// Fix bug 64206 that block trusted app to attach into un-trusted Outlook
HRESULT WINAPI Hooked_CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	DEVLOG(L"on Hooked_CreateProcessW\n");
	if (rc.is_disabled()) {
		return Hooked_CreateProcessW_Next(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
			bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	//
	// following is another email method we need to block
	// "outlook.exe /a 'attachment.txt'"
	//

	// and fix bug 64652 VS\Pycharm stopped.
	if (lpApplicationName != NULL) {
		std::wstring appName(lpApplicationName);
		std::transform(appName.begin(), appName.end(), appName.begin(), ::tolower);
		if (appName.find(L"outlook.exe") != std::wstring::npos)
		{
			if (nx::on_email_will_be_send())
			{
				return Hooked_CreateProcessW_Next(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
					bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
			}
			else {
				// not allow
				return false;
			}

		}
	}

	if (lpCommandLine != NULL) {
		std::wstring cmdline(lpCommandLine);
		std::transform(cmdline.begin(), cmdline.end(), cmdline.begin(), ::tolower);
		if (cmdline.find(L"outlook.exe") != std::wstring::npos)
		{
			if (nx::on_email_will_be_send())
			{
				return Hooked_CreateProcessW_Next(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
					bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
			}
			else {
				// not allow
				return false;
			}
		}
	}

	return Hooked_CreateProcessW_Next(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
		bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

HMODULE WINAPI Hooked_LoadLibraryExA(
	LPCSTR lpLibFileName,
	HANDLE hFile,
	DWORD  dwFlags)
{
	DEVLOG(L"on Hooked_LoadLibraryExA\n");
	if (rc.is_disabled()) {
		return Hooked_LoadLibraryExA_Next(lpLibFileName, hFile, dwFlags);
	}
	utils::recursion_control_auto auto_disable(rc);

	if (!nx::on_hook_mapi32dll_will_be_loaded(lpLibFileName)) {
		return NULL;
	}

	return Hooked_LoadLibraryExA_Next(lpLibFileName, hFile, dwFlags);
}

HMODULE WINAPI Hooked_LoadLibraryA(LPCSTR lpLibFileName)
{
	DEVLOG(L"on Hooked_LoadLibraryA\n");
	if (rc.is_disabled()) {
		return Hooked_LoadLibraryA_Next(lpLibFileName);
	}
	utils::recursion_control_auto auto_disable(rc);

	if (!nx::on_hook_mapi32dll_will_be_loaded(lpLibFileName)) {
		return NULL;
	}

	return Hooked_LoadLibraryA_Next(lpLibFileName);
}

BOOL WINAPI Hooked_ShowWindow(HWND hWnd, int nCmdShow)
{
	DEVLOG(L"on Hooked_ShowWindow\n");
	if (rc.is_disabled()) {
		return Hooked_ShowWindow_Next(hWnd, nCmdShow);
	}
	utils::recursion_control_auto auto_disable(rc);
	if (!nx::on_hook_Adobe_Customized_SaveAs_Win_open(hWnd) ||
		!nx::on_hook_BentleyView_Customized_Publish_Win_open(hWnd)) {

		// Must destroy it since have created it(by CreateWindow) and won't display it.
		int ret = DestroyWindow(hWnd);
		if (ret == 0) {
			DEVLOG(L"Destroywindow failed.");
		}

		return false;
	}

	return Hooked_ShowWindow_Next(hWnd, nCmdShow);
} 

int WINAPI Hooked_SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp)
{
	DEVLOG(L"on Hooked_SHFileOperationA \n");
	if (rc.is_disabled()) {
		return Hooked_SHFileOperationA_Next(lpFileOp);
	}
	utils::recursion_control_auto auto_disable(rc);
	if (lpFileOp != NULL) {
		std::string path = lpFileOp->pFrom;
		if (!on_hook_jt2go_shfileopration_delete(utils::utf82utf16(path))) {

			// Delete permanently
			// If do this, will delete decrypted normal file since the host application is trusted, and nxl file still exists,
			// then user will not directly open the nxl file under RPM folder again.
			/*
			lpFileOp->fFlags &= ~FOF_ALLOWUNDO;
			return Hooked_SHFileOperationA_Next(lpFileOp); */

			// The returned value is an illusion for caller, actually we'll notify caller the operation is denied. 
			return 0; 
		}
	}

	return Hooked_SHFileOperationA(lpFileOp);
}

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
BOOL WINAPI Hooked_CopyFileExW(
	LPCWSTR             lpExistingFileName,
	LPCWSTR             lpNewFileName,
	LPPROGRESS_ROUTINE  lpProgressRoutine,
	LPVOID              lpData,
	LPBOOL              pbCancel,
	DWORD               dwCopyFlags)
{
	DEVLOG(L"on Hooked_CopyFileExW\n");
	if (rc.is_disabled()) {
		return Hooked_CopyFileExW_Next(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
	}
	utils::recursion_control_auto auto_disable(rc);

	if (on_hook_copy_file(lpExistingFileName, lpNewFileName)) {
		return (GetLastError() == ERROR_SUCCESS);
	}

	return Hooked_CopyFileExW_Next(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

BOOL WINAPI Hooked_MoveFileExW(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName,
	DWORD   dwFlags)
{
	DEVLOG(L"on Hooked_MoveFileExW\n");
	if (rc.is_disabled()) {
		return Hooked_MoveFileExW_Next(lpExistingFileName, lpNewFileName, dwFlags);
	}
	utils::recursion_control_auto auto_disable(rc);

	if (on_hook_move_file(lpExistingFileName, lpNewFileName)) {
		return (GetLastError() == ERROR_SUCCESS);
	}

	return Hooked_MoveFileExW_Next(lpExistingFileName, lpNewFileName, dwFlags);
}
#endif  // #if 0

//
END_NXHOOK_NAMESPACE
//