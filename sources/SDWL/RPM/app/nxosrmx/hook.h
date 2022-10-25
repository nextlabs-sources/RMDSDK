/*
	API Hook related declartions and hook library included declaration
*/
#pragma once

#include <mapi.h>

#define NXHOOK_NAMESPACE namespace nx{ namespace hook{
#define END_NXHOOK_NAMESPACE }}


NXHOOK_NAMESPACE

#include<madCHook.h>  // made in ns:  nx::hook

// for saveas UI
__declspec(selectany)
BOOL(APIENTRY* Hooked_GetSaveFileNameW_Next)(LPOPENFILENAMEW) = NULL;
BOOL  APIENTRY Hooked_GetSaveFileNameW(LPOPENFILENAMEW arg);


// for createwindowex
__declspec(selectany)
HWND(WINAPI* Hooked_CreateWindowExW_Next)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) = NULL;

HWND WINAPI Hooked_CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

// for CreateWindowExA
__declspec(selectany)
HWND(WINAPI* Hooked_CreateWindowExA_Next)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) = NULL;

HWND WINAPI Hooked_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);



// for destorywindow
__declspec(selectany)
BOOL(WINAPI* Hooked_DestroyWindow_Next)(HWND hWnd) = NULL;
BOOL WINAPI Hooked_DestroyWindow(HWND hWnd);


// for User32!GetMessageW
__declspec(selectany)
BOOL (WINAPI* Hooked_GetMessageW_Next)(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax)=NULL;
BOOL WINAPI Hooked_GetMessageW(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax);

// for User32!GetMessageA
__declspec(selectany)
BOOL(WINAPI* Hooked_GetMessageA_Next)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) = NULL;
BOOL WINAPI Hooked_GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);

// for User32!TranslateMessage
__declspec(selectany)
BOOL (WINAPI* Hooked_TranslateMessage_Next)(CONST MSG* lpMsg)=NULL;
BOOL WINAPI Hooked_TranslateMessage(CONST MSG* lpMsg);




// for openclipboard
__declspec(selectany)
BOOL(WINAPI* Hooked_OpenClipboard_Next)(HWND hWndNewOwner) = NULL;
BOOL WINAPI Hooked_OpenClipboard(HWND hWndNewOwner);

// for SetClipbaordData
__declspec(selectany)
HANDLE (WINAPI* Hooked_SetClipboardData_Next)(UINT uFormat, HANDLE hMem)=NULL;
HANDLE WINAPI Hooked_SetClipboardData(UINT uFormat, HANDLE hMem);

// for enddoc
__declspec(selectany)
int (WINAPI* Hooked_EndPage_Next)(HDC hdc) = NULL;
int WINAPI Hooked_EndPage(HDC hdc);

// for startdocw
__declspec(selectany)
int (WINAPI* Hooked_StartDocW_Next)(HDC hdc, const DOCINFOW* lpdi) = NULL;
int WINAPI Hooked_StartDocW(HDC hdc, const DOCINFOW* lpdi);

// for startdocw
__declspec(selectany)
int (WINAPI* Hooked_StartDocA_Next)(HDC hdc, const DOCINFOA* lpdi) = NULL;
int WINAPI Hooked_StartDocA(HDC hdc, const DOCINFOA* lpdi);

// for ole32!CoCreateInstance
// for combase!CoCreateInstance
__declspec(selectany)
HRESULT(WINAPI* Hooked_CoCreateInstance_Next)(
	IN REFCLSID		rclsid,
	IN LPUNKNOWN	pUnkOuter,
	IN DWORD		dwClsContext,
	IN REFIID		riid,
	OUT LPVOID FAR* ppv) = NULL;

HRESULT WINAPI Hooked_CoCreateInstance(
	IN REFCLSID		rclsid,
	IN LPUNKNOWN	pUnkOuter,
	IN DWORD		dwClsContext,
	IN REFIID		riid,
	OUT LPVOID FAR* ppv);


// for ole32!RegisterDragDrop
__declspec(selectany)
HRESULT(WINAPI* Hooked_RegisterDragDrop_Next)(IN HWND hwnd, IN LPDROPTARGET pDropTarget);
HRESULT WINAPI  Hooked_RegisterDragDrop(IN HWND hwnd, IN LPDROPTARGET pDropTarget);


// for ole32!OleCreateFromFile
__declspec(selectany)
HRESULT(WINAPI* Hooked_OleCreateFromFile_Next)(IN REFCLSID        rclsid, IN LPCOLESTR       lpszFileName,
	IN REFIID          riid, IN DWORD           renderopt, IN LPFORMATETC     lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE       pStg, OUT LPVOID* ppvObj) = NULL;
HRESULT WINAPI Hooked_OleCreateFromFile(IN REFCLSID        rclsid, IN LPCOLESTR       lpszFileName,
	IN REFIID          riid, IN DWORD           renderopt, IN LPFORMATETC     lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE       pStg, OUT LPVOID* ppvObj);

//

// for old32!OleCreateLinkToFile
__declspec(selectany)
HRESULT(WINAPI* Hooked_OleCreateLinkToFile_Next)(IN LPCOLESTR lpszFileName, IN REFIID riid,
	IN DWORD renderopt, IN LPFORMATETC lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj)=NULL;

HRESULT WINAPI Hooked_OleCreateLinkToFile(IN LPCOLESTR lpszFileName, IN REFIID riid,
	IN DWORD renderopt, IN LPFORMATETC lpFormatEtc,
	IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj);



// for mapi32!MAPISendEmail
__declspec(selectany)
ULONG (WINAPI*Hooked_MAPISENDMAIL_Next)(
	LHANDLE lhSession,	ULONG_PTR ulUIParam,	lpMapiMessage lpMessage,
	FLAGS flFlags,	ULONG ulReserved	) = NULL;

ULONG WINAPI Hooked_MAPISENDMAIL(
	LHANDLE lhSession,	ULONG_PTR ulUIParam,	lpMapiMessage lpMessage,
	FLAGS flFlags,	ULONG ulReserved);

// for kernelbase!CreateProcessW
__declspec(selectany)
HRESULT (WINAPI*Hooked_CreateProcessW_Next)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) = NULL;

HRESULT WINAPI Hooked_CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

// for kernelbase!LoadLibraryExA
__declspec(selectany)
HMODULE(WINAPI* Hooked_LoadLibraryExA_Next)(
	LPCSTR lpLibFileName,
	HANDLE hFile,
	DWORD  dwFlags
) = NULL;

HMODULE WINAPI Hooked_LoadLibraryExA(
	LPCSTR lpLibFileName,
	HANDLE hFile,
	DWORD  dwFlags
);

// for kernelbase!LoadLibraryA
__declspec(selectany)
HMODULE(WINAPI* Hooked_LoadLibraryA_Next)(LPCSTR lpLibFileName) = NULL;
HMODULE WINAPI Hooked_LoadLibraryA(LPCSTR lpLibFileName);

// for User32!ShowWindow
// Block the adobe customeized save as window by hook this instead of CreateWindow, since can't distinguish
// the Save As window and other windows(e.g. Open File window).
__declspec(selectany)
BOOL (WINAPI* Hooked_ShowWindow_Next)(HWND hWnd,int  nCmdShow) = NULL;
BOOL WINAPI Hooked_ShowWindow(HWND hWnd,int  nCmdShow); 

// for shell32.dll!SHFileOperationA (fix bug 66504)
__declspec(selectany)
int (WINAPI* Hooked_SHFileOperationA_Next)(LPSHFILEOPSTRUCTA lpFileOp) = NULL;
int WINAPI Hooked_SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp);

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
// for kernel32!CopyFileExW
__declspec(selectany)
BOOL(WINAPI* Hooked_CopyFileExW_Next)(
	LPCWSTR             lpExistingFileName,
	LPCWSTR             lpNewFileName,
	LPPROGRESS_ROUTINE  lpProgressRoutine,
	LPVOID              lpData,
	LPBOOL              pbCancel,
	DWORD               dwCopyFlags
) = NULL;

BOOL WINAPI Hooked_CopyFileExW(
	LPCWSTR             lpExistingFileName,
	LPCWSTR             lpNewFileName,
	LPPROGRESS_ROUTINE  lpProgressRoutine,
	LPVOID              lpData,
	LPBOOL              pbCancel,
	DWORD               dwCopyFlags
);

// for kernel32!MoveFileExW
__declspec(selectany)
BOOL(WINAPI* Hooked_MoveFileExW_Next)(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName,
	DWORD   dwFlags
) = NULL;

BOOL WINAPI Hooked_MoveFileExW(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName,
	DWORD   dwFlags
);
#endif  // #if 0

END_NXHOOK_NAMESPACE
//