#pragma once

#include <ShObjIdl.h>  // #include <ShObjIdl_core.h>   release32 may get error
#include <Shlwapi.h>
#include <atlbase.h>
#include <OleDlg.h>

// for user32!DispatchMessage
typedef LRESULT(WINAPI* Hooked_DispatchMessageW_Signature)(MSG* lpMsg) ;

// for ole32!CoCreateInstance
// for combase!CoCreateInstance
typedef HRESULT(WINAPI* Hooked_CoCreateInstance_Signature)(
	IN REFCLSID		rclsid,
	IN LPUNKNOWN	pUnkOuter,
	IN DWORD		dwClsContext,
	IN REFIID		riid,
	OUT LPVOID FAR* ppv);

// for ole32!OleGetClipboard -- fix bug 65316
typedef HRESULT(WINAPI* Hooked_OleGetClipboard_Signature)(LPDATAOBJECT* ppDataObj);

// for ole32!RegisterDragDrop
typedef HRESULT(WINAPI* Hooked_RegisterDragDrop_Signature)(IN HWND hwnd, IN LPDROPTARGET pDropTarget);

// for ole32!OleCreateFromFile
typedef HRESULT(WINAPI* Hooked_OleCreateFromFile_Signature)(IN REFCLSID        rclsid, IN LPCOLESTR       lpszFileName,
	IN REFIID          riid, IN DWORD           renderopt, IN LPFORMATETC     lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE       pStg, OUT LPVOID* ppvObj);

// for old32!OleCreateLinkToFile
typedef HRESULT(WINAPI* Hooked_OleCreateLinkToFile_Signature)(IN LPCOLESTR lpszFileName, IN REFIID riid,
	IN DWORD renderopt, IN LPFORMATETC lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj);

// for shell32!DragQueryFileW
typedef UINT(WINAPI* Hooked_DragQueryFileW_Signature)(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);

// for OleDlg, standard ole-style dialog
typedef UINT(WINAPI* Hooked_OleUIInsertObjectA_Signature)(LPOLEUIINSERTOBJECTA);
typedef UINT(WINAPI* Hooked_OleUIInsertObjectW_Signature)(LPOLEUIINSERTOBJECTW);

