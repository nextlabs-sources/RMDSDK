/*
Abstraction:
	there are the transfred codes to impl anti ole-link and embed feature,
	as designed, if nxl file want to be inserted into target process, deny it.
*/

#include "pch.h"
#include "util.hpp"
#include "ole_api_signature.h"
#include "Global.h"
#include<madCHook.h> 

#include <sstream>


Hooked_DispatchMessageW_Signature		Hooked_DispatchMessageW_Next = NULL;
Hooked_CoCreateInstance_Signature		Hooked_CoCreateInstance_Next = NULL;
Hooked_RegisterDragDrop_Signature		Hooked_RegisterDragDrop_Next = NULL;
Hooked_OleCreateFromFile_Signature		Hooked_OleCreateFromFile_Next = NULL;
Hooked_OleCreateLinkToFile_Signature	Hooked_OleCreateLinkToFile_Next = NULL;
Hooked_DragQueryFileW_Signature			Hooked_DragQueryFileW_Next = NULL;
Hooked_OleUIInsertObjectA_Signature		Hooked_OleUIInsertObjectA_Next = NULL;
Hooked_OleUIInsertObjectW_Signature		Hooked_OleUIInsertObjectW_Next = NULL;
Hooked_OleGetClipboard_Signature        Hooked_OleGetClipboard_Next = NULL;


LRESULT WINAPI Hooked_DispatchMessageW(MSG* lpMsg);

HRESULT WINAPI Hooked_CoCreateInstance(
	IN REFCLSID		rclsid,
	IN LPUNKNOWN	pUnkOuter,
	IN DWORD		dwClsContext,
	IN REFIID		riid,
	OUT LPVOID FAR* ppv);
HRESULT WINAPI  Hooked_RegisterDragDrop(IN HWND hwnd, IN LPDROPTARGET pDropTarget);

HRESULT WINAPI Hooked_OleGetClipboard(LPDATAOBJECT* ppDataObj);

HRESULT WINAPI Hooked_OleCreateFromFile(IN REFCLSID        rclsid, IN LPCOLESTR       lpszFileName,
	IN REFIID          riid, IN DWORD           renderopt, IN LPFORMATETC     lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE       pStg, OUT LPVOID* ppvObj);

HRESULT WINAPI Hooked_OleCreateLinkToFile(IN LPCOLESTR lpszFileName, IN REFIID riid,
	IN DWORD renderopt, IN LPFORMATETC lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj);

UINT WINAPI Hooked_DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);

UINT WINAPI Hooked_OleUIInsertObjectA(LPOLEUIINSERTOBJECTA);

UINT WINAPI Hooked_OleUIInsertObjectW(LPOLEUIINSERTOBJECTW);




bool setup_anti_ole_link_embed()
{
	// using api hook to generate events
    if (is_win8andabove())
    {
        //combase!CoCreateInstance
        if (!HookAPI("combase", "CoCreateInstance", (PVOID)Hooked_CoCreateInstance, (PVOID*)&Hooked_CoCreateInstance_Next)) {
            return false;
        }
    }
    else
    {
        //ole32!CoCreateInstance
        if (!HookAPI("ole32", "CoCreateInstance", (PVOID)Hooked_CoCreateInstance, (PVOID*)&Hooked_CoCreateInstance_Next)) {
            return false;
        }
    }

    if (!HookAPI("user32", "DispatchMessageW", (PVOID)Hooked_DispatchMessageW, (PVOID*)&Hooked_DispatchMessageW_Next)) {
        return false;
    }

    // using hook DispatchMEssgeW can do this in lower level
    // 3/16/2020, it seems hooking at high level is better
    if (!HookAPI("ole32", "RegisterDragDrop", (PVOID)Hooked_RegisterDragDrop, (PVOID*)&Hooked_RegisterDragDrop_Next)) {
        return false;
    }

	if (!HookAPI("ole32", "OleGetClipboard", (PVOID)Hooked_OleGetClipboard, (PVOID*)&Hooked_OleGetClipboard_Next)) {
		return false;
	}

    if (!HookAPI("ole32", "OleCreateFromFile", (PVOID)Hooked_OleCreateFromFile, (PVOID*)&Hooked_OleCreateFromFile_Next)) {
        return false;
    }

    if (!HookAPI("ole32", "OleCreateLinkToFile", (PVOID)Hooked_OleCreateLinkToFile, (PVOID*)&Hooked_OleCreateLinkToFile_Next)) {
        return false;
    }
    if (!HookAPI("shell32", "DragQueryFileW", (PVOID)Hooked_DragQueryFileW, (PVOID*)&Hooked_DragQueryFileW_Next)) {
        return false;
    }

    // for Hook some OLE standard dialog
    if (!HookAPI("OleDlg", "OleUIInsertObjectA", (PVOID)Hooked_OleUIInsertObjectA, (PVOID*)&Hooked_OleUIInsertObjectA_Next)) {
        return false;
    }
    if (!HookAPI("OleDlg", "OleUIInsertObjectW", (PVOID)Hooked_OleUIInsertObjectW, (PVOID*)&Hooked_OleUIInsertObjectW_Next)) {
        return false;
    }

    // for others 
    return true;
}

namespace {
	bool is_nxl(const std::wstring& path) {
		if (path.length() < 4) {
			return false;
		}
		return is_nxl_file(path);
	}

	bool is_filter_nxl(const std::wstring &path) {
		if (is_outlook_process()) {
			return false;
		}

		return is_nxl(path);
	}

	bool is_contained_nxl_file(IDataObject* pDataObj) {
		if (!pDataObj) {
			return false;
		}
		HRESULT hr = E_FAIL;
		FORMATETC format{ 0 };
		format.cfFormat = CF_HDROP;
		format.tymed = TYMED_HGLOBAL;
		format.dwAspect = DVASPECT_CONTENT;
		format.lindex = -1;
		STGMEDIUM medium;
		hr = pDataObj->GetData(&format, &medium);
		if (FAILED(hr)) {
			return false;
		}
		HDROP hdrop = (HDROP)::GlobalLock(medium.hGlobal);
		if (hdrop == NULL) {
			return false;
		}
		scope_guard sg([&medium]() {
			::GlobalUnlock(medium.hGlobal);
			});
		recursion_control_auto auto_disable(global.rc);
		int total = ::DragQueryFileW(hdrop, -1, NULL, 0);

		for (int i = 0; i < total; i++) {
			wchar_t buf[0x400] = { 0 };
			::DragQueryFileW(hdrop, i, buf, 0x400);
			if (is_filter_nxl(wstr_tolower(buf))) {
				return true;
			}
		}

		return false;
	}

	bool is_contained_nxl_file(HDROP hDrop) {
		int total = ::DragQueryFileW(hDrop, -1, NULL, 0);
		for (int i = 0; i < total; i++) {
			wchar_t buf[0x400] = { 0 };
			::DragQueryFileW(hDrop, i, buf, 0x400);
			if (is_filter_nxl(wstr_tolower(buf))) {
				return true;
			}
		}
		return false;
	}

	static void parse_dirs(std::vector<std::wstring>& markDir, const WCHAR* str)
	{
		std::wstringstream f(str);
		std::wstring s;
		while (std::getline(f, s, L'<'))
		{
			markDir.push_back(s + L"\\");
		}
	}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	bool is_sanctuary_file(const std::wstring &wstrPath) {
		bool result = false;

		const std::wstring path = wstrPath + L"\\";
		HKEY hKey = NULL;
		if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\NextLabs\\SkyDRM", 0, KEY_READ | KEY_WOW64_64KEY, &hKey))
		{
			DWORD cbData = 0;
			LPCWSTR name = L"sanctuaryfolder";
			if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, NULL, NULL, &cbData) && 0 != cbData)
			{
				LPBYTE lpData = new BYTE[cbData];
				if (ERROR_SUCCESS == RegQueryValueExW(hKey, name, NULL, NULL, lpData, &cbData))
				{
					std::vector<std::wstring> markDir;
					parse_dirs(markDir, (const WCHAR*)lpData);

					for (std::wstring& dir : markDir)
					{
						if (dir.size() == path.size())
						{
							if (path == dir)
							{
								result = TRUE;
								break;
							}
						}
						else if (dir.size() < path.size())
						{
							if (path.substr(0, dir.size()) == dir)
							{
								result = TRUE;
								break;
							}
						}
					}
				}

				delete[]lpData;

			}

			RegCloseKey(hKey);

		}

		return result;
	}

	bool is_contained_sanctuary_file(IDataObject* pDataObj) {
		if (!pDataObj) {
			return false;
		}
		HRESULT hr = E_FAIL;
		FORMATETC format{ 0 };
		format.cfFormat = CF_HDROP;
		format.tymed = TYMED_HGLOBAL;
		format.dwAspect = DVASPECT_CONTENT;
		format.lindex = -1;
		STGMEDIUM medium;
		hr = pDataObj->GetData(&format, &medium);
		if (FAILED(hr)) {
			return false;
		}
		HDROP hdrop = (HDROP)::GlobalLock(medium.hGlobal);
		if (hdrop == NULL) {
			return false;
		}
		scope_guard sg([&medium]() {
			::GlobalUnlock(medium.hGlobal);
		});
		recursion_control_auto auto_disable(global.rc);

		int total = ::DragQueryFileW(hdrop, -1, NULL, 0);
		for (int i = 0; i < total; i++) {
			wchar_t buf[0x400] = { 0 };
			::DragQueryFileW(hdrop, i, buf, 0x400);
			std::wstring wstrPath = wstr_tolower(buf);
			if (is_sanctuary_file(wstrPath)) {
				return true;
			}
		}

		return false;
	}

	bool is_contained_sanctuary_file(HDROP hDrop) {
		int total = ::DragQueryFileW(hDrop, -1, NULL, 0);
		for (int i = 0; i < total; i++) {
			wchar_t buf[0x400] = { 0 };
			::DragQueryFileW(hDrop, i, buf, 0x400);
			std::wstring wstrPath = wstr_tolower(buf);
			if (is_sanctuary_file(wstrPath)) {
				return true;
			}
		}

		return false;
	}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
}

namespace {
	bool on_hook_ole_object_will_be_inserted(const wchar_t* p);

	class Proxcy_IDropTarget : public IDropTarget
	{
	public:
		Proxcy_IDropTarget(IDropTarget* pTarget) {
			m_pProxied = pTarget;
			if (m_pProxied) {
				AddRef();
			}
		}
		~Proxcy_IDropTarget() {
			if (m_pProxied) {
				Release();
			}
		}
		// Inherited via IDropTarget
		virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override {
			if (!m_pProxied) {
				return E_NOTIMPL;
			}
			return m_pProxied->QueryInterface(riid, ppvObject);
		}
		virtual ULONG __stdcall AddRef(void) override {
			if (m_pProxied) {
				return m_pProxied->AddRef();
			}
			return 0;
		}
		virtual ULONG __stdcall Release(void) override {
			if (m_pProxied) {
				return m_pProxied->Release();
			}
			return 0;
		}
		virtual HRESULT __stdcall DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) {
			if (!m_pProxied) {
				return E_UNEXPECTED;
			}
			DEVLOG_FUN;
			return m_pProxied->DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
		}
		virtual HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
			if (!m_pProxied) {
				return E_UNEXPECTED;
			}
			DEVLOG_FUN;
			return m_pProxied->DragOver(grfKeyState, pt, pdwEffect);
		}
		virtual HRESULT __stdcall DragLeave(void) override {
			{
				if (!m_pProxied) {
					return E_UNEXPECTED;
				}
				DEVLOG_FUN;
				return m_pProxied->DragLeave();
			}
		}
		virtual HRESULT __stdcall Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
			{
				if (!m_pProxied) {
					return E_UNEXPECTED;
				}
				DEVLOG_FUN;
				if (is_contained_nxl_file(pDataObj)) {
					DEVLOG(L"wait for rm::notify_message impled in OLE Drop\n");
					//nx::rm::notify_message(L"Drop NXL file here is not allowed.");
					return S_OK;
				}

				// drop file is under sanctuary folder,
				// deny.
#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
				if (is_contained_sanctuary_file(pDataObj)) {
					return S_OK;
				}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

				return m_pProxied->Drop(pDataObj, grfKeyState, pt, pdwEffect);
			}
		}

	private:
		IDropTarget* m_pProxied;
	};

	class Events_FileOpenDialog : public IFileDialogEvents {
		int i = 0;
		// Inherited via IFileDialogEvents
		virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override
		{
			if (riid == IID_IFileDialogEvents) {
				*ppvObject = this;
				return S_OK;
			}
			return E_NOTIMPL;
		}
		virtual ULONG __stdcall AddRef(void) override
		{
			return ++i;
		}
		virtual ULONG __stdcall Release(void) override
		{
			return --i; // ignore delete
		}
		virtual HRESULT __stdcall OnFileOk(IFileDialog* pfd) override
		{
			// add new feature, ignroe Dlg' title contains Open
			auto is_ingore = [this]() {
				HWND h = ::GetForegroundWindow();
				char buf[255] = { 0 };
				auto len = ::GetWindowTextA(h, buf, 255);
				if (len < 4) {
					return false;
				}

				Module module;
				std::wstring moduleName = module.GetNameW();
				if (_wcsicmp(moduleName.c_str(),L"microstation.exe") == 0 || _wcsicmp(moduleName.c_str(), L"pwc.exe") == 0) {
					return true;
				}

				if (iconstain(std::string(buf, len), "open") || iconstain(std::string(buf, len), "Upload")) {
					return true;
				}

				// Partial fix bug 67718, ignore this and we will handle it in microstationRmx
				if (iconstain(std::string(buf, len), "Attach Cell Library")) {
					return true;
				}

				return false;
			};

			if (is_ingore()) {
				return S_OK;
			}

			// extract path from current_selection
			CComPtr<IShellItem> spItem = NULL;
			if (FAILED(pfd->GetCurrentSelection(&spItem))) {
				return S_OK;
			}
			wchar_t* path = NULL;
			if (FAILED(spItem->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
				return S_OK;
			}

			std::wstring s(path);
			::CoTaskMemFree(path);
			if (on_hook_ole_object_will_be_inserted(s.c_str())) {
				return S_OK;
			}

			::MessageBox(::GetForegroundWindow(),
				L"The operation is not allowed, please check with administrator for more details.",
				L"Warning:",
				MB_OK | MB_ICONEXCLAMATION
			);

			return S_FALSE;
		}
		virtual HRESULT __stdcall OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder) override
		{
			return E_NOTIMPL;
		}
		virtual HRESULT __stdcall OnFolderChange(IFileDialog* pfd) override
		{
			return E_NOTIMPL;
		}
		virtual HRESULT __stdcall OnSelectionChange(IFileDialog* pfd) override
		{
			return E_NOTIMPL;
		}
		virtual HRESULT __stdcall OnShareViolation(IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse) override
		{
			return E_NOTIMPL;
		}
		virtual HRESULT __stdcall OnTypeChange(IFileDialog* pfd) override
		{
			return E_NOTIMPL;
		}
		virtual HRESULT __stdcall OnOverwrite(IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse) override
		{
			return E_NOTIMPL;
		}
	};

	DWORD dwGlobalEventCookie;

	void on_hook_window_register_dragdrop(HWND hwnd, LPDROPTARGET& pDropTarget)
	{
		auto p = new Proxcy_IDropTarget(pDropTarget);
		pDropTarget = p;
	}

	// true: deny
	bool on_wm_dropfiles(HDROP hDrop) {
		return is_contained_nxl_file(hDrop);
	}

	// require return false, as deny insert the ole object
	bool on_hook_ole_object_will_be_inserted(const wchar_t* p)
	{
		if (!p) {
			return true;
		}
		std::wstring path = p;

		if (is_filter_nxl(path)) {
			//nx::rm::notify_message(L"Embedding NextLabs protected file into another file is not supported.");
			return false;
		}

		return true;
	}

	bool on_hook_ole_object_will_be_inserted(const char* p) {
		if (p == NULL) {
			return true;
		}
		return on_hook_ole_object_will_be_inserted(to_wstr(p).c_str());
	}

	bool on_hook_ole_get_clipboard()
	{
		IDataObject* pdtobj = NULL;
		if (::OleGetClipboard(&pdtobj) != S_OK) return false;

		if (pdtobj == NULL) return false;

		FORMATETC   FmtEtc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM   Stg = { 0 };
		HDROP       hDrop = NULL;

		memset(&Stg, 0, sizeof(Stg));
		Stg.tymed = CF_HDROP;

		// Find CF_HDROP data in pDataObj
		if (FAILED(pdtobj->GetData(&FmtEtc, &Stg))) {
			return false;
		}

		// Get the pointer pointing to real data
		hDrop = (HDROP)GlobalLock(Stg.hGlobal);
		if (NULL == hDrop) {
			ReleaseStgMedium(&Stg);
			return false;
		}

		if (is_contained_nxl_file(hDrop)) {
			return true;
		}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		if (is_contained_sanctuary_file(hDrop)) {
			return true;
		}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

		return false;
	}

	void on_hook_com_object_created(const IID& rclsid, const IID& riid, LPVOID* ppv) {
		if (IsEqualCLSID(rclsid, CLSID_FileOpenDialog)) {
			// give this class CFileOpenDialog a event handler to modify user select nxl file
			if (IsEqualIID(riid, IID_IFileDialog)) {
				CComPtr<IFileDialog>  dlg((IFileDialog*)(*ppv));
				Events_FileOpenDialog* pEvents = new Events_FileOpenDialog();
				dlg->Advise(pEvents, &dwGlobalEventCookie);
			}
			if (IsEqualIID(riid, IID_IFileOpenDialog)) {
				CComPtr<IFileOpenDialog>  dlg((IFileOpenDialog*)(*ppv));
				Events_FileOpenDialog* pEvents = new Events_FileOpenDialog();
				dlg->Advise(pEvents, &dwGlobalEventCookie);
			}
		}		
	}

}


LRESULT __stdcall Hooked_DispatchMessageW(MSG* lpMsg)
{
	// Notice, ignore lock_guard_anti_reentrent,
	// must check the code carefully to avoid reentrent;
	if (lpMsg && lpMsg->message == WM_DROPFILES && on_wm_dropfiles((HDROP)lpMsg->wParam)) {
		return 0;
	}
	return Hooked_DispatchMessageW_Next(lpMsg);
}

HRESULT WINAPI Hooked_CoCreateInstance(
	IN REFCLSID		rclsid,
	IN LPUNKNOWN	pUnkOuter,
	IN DWORD		dwClsContext,
	IN REFIID		riid,
	OUT LPVOID FAR* ppv)
{
	HRESULT hr = S_FALSE;

	DEVLOG_FUN;
	if (global.rc.is_disabled()) {
		return Hooked_CoCreateInstance_Next(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	}
	recursion_control_auto auto_disable(global.rc);
	global.rc.process_disable();	// comment by osmond, CoCreateInstance must consider about Mutil_Thread 

	hr = Hooked_CoCreateInstance_Next(rclsid, pUnkOuter, dwClsContext, riid, ppv);

	if (FAILED(hr)) {
		return hr;
	}

	on_hook_com_object_created(rclsid, riid, ppv);

	return hr;
}

HRESULT __stdcall Hooked_OleGetClipboard(LPDATAOBJECT* ppDataObj)
{
	HRESULT hr = S_FALSE;

	DEVLOG_FUN;

	if (global.rc.is_disabled()) {
		return Hooked_OleGetClipboard_Next(ppDataObj);
	}
	recursion_control_auto auto_disable(global.rc);

	if (on_hook_ole_get_clipboard()) {	
		return E_NOTIMPL;
	}
	else {
		return Hooked_OleGetClipboard_Next(ppDataObj);
	}

}

HRESULT __stdcall Hooked_RegisterDragDrop(IN HWND hwnd, IN LPDROPTARGET pDropTarget)
{
	DEVLOG_FUN;
	HRESULT rt = E_NOTIMPL;

	if (global.rc.is_disabled()) {
		return Hooked_RegisterDragDrop_Next(hwnd, pDropTarget);
	}
	recursion_control_auto auto_disable(global.rc);
	// give client a change to replace pDropTarget
	on_hook_window_register_dragdrop(hwnd, pDropTarget);
	rt = Hooked_RegisterDragDrop_Next(hwnd, pDropTarget);
	return rt;
}

HRESULT __stdcall Hooked_OleCreateFromFile(IN REFCLSID rclsid, IN LPCOLESTR lpszFileName, IN REFIID riid, IN DWORD renderopt, IN LPFORMATETC lpFormatEtc, IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID* ppvObj)
{
	DEVLOG_FUN;
	if (global.rc.is_disabled()) {
		return Hooked_OleCreateFromFile_Next(rclsid, lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}
	recursion_control_auto auto_disable(global.rc);

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

HRESULT __stdcall Hooked_OleCreateLinkToFile(IN LPCOLESTR lpszFileName, IN REFIID riid, IN DWORD renderopt,
	IN LPFORMATETC lpFormatEtc, IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID* ppvObj)
{
	DEVLOG_FUN;
	if (global.rc.is_disabled()) {
		return Hooked_OleCreateLinkToFile_Next(lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
	}
	recursion_control_auto auto_disable(global.rc);

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

UINT __stdcall Hooked_DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
	DEVLOG_FUN;

	if (global.rc.is_disabled()) {
		return Hooked_DragQueryFileW_Next(hDrop, iFile, lpszFile, cch);
	}
	recursion_control_auto auto_disable(global.rc);

	if (iFile == -1) {
		return Hooked_DragQueryFileW_Next(hDrop, iFile, lpszFile, cch);
	}

	if (is_contained_nxl_file(hDrop)) {
		return 0;
	}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	if (is_contained_sanctuary_file(hDrop)) {
		return 0;
	}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	return Hooked_DragQueryFileW_Next(hDrop, iFile, lpszFile, cch);
}

UINT __stdcall Hooked_OleUIInsertObjectA(LPOLEUIINSERTOBJECTA st)
{
	DEVLOG_FUN;
	if (global.rc.is_disabled()) {
		return Hooked_OleUIInsertObjectA_Next(st);
	}
	recursion_control auto_disable(global.rc);

	UINT rt = Hooked_OleUIInsertObjectA_Next(st);
	// for user select ok
	if (rt == OLEUI_OK && !on_hook_ole_object_will_be_inserted(st->lpszFile)) {
		return OLEUI_CANCEL;
	}

	return rt;
}

UINT __stdcall Hooked_OleUIInsertObjectW(LPOLEUIINSERTOBJECTW st)
{
	DEVLOG_FUN;
	if (global.rc.is_disabled()) {
		return Hooked_OleUIInsertObjectW_Next(st);
	}
	recursion_control auto_disable(global.rc);
	UINT rt = Hooked_OleUIInsertObjectW_Next(st);
	// for user select ok
	if (rt == OLEUI_OK && !on_hook_ole_object_will_be_inserted(st->lpszFile)) {
		return OLEUI_CANCEL;
	}

	return rt;
}
