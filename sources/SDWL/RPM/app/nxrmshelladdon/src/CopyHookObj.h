// CopyHookObj.h : Declaration of the CCopyHookObj

#pragma once
#include "resource.h"       // main symbols



#include "nxrmshelladdon_i.h"



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CCopyHookObj

class ATL_NO_VTABLE CCopyHookObj :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCopyHookObj, &CLSID_CopyHookObj>,
	public ICopyHook,
	public IDispatchImpl<ICopyHookObj, &IID_ICopyHookObj, &LIBID_nxrmshelladdonLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CCopyHookObj()
	{
	}

	STDMETHOD_(UINT, CopyCallback) (HWND hwnd, UINT wFunc, UINT wFlags, LPCWSTR pszSrcFile, DWORD dwSrcAttribs, LPCWSTR pszDestFile, DWORD dwDestAttribs);

	DECLARE_REGISTRY_RESOURCEID(107)


	BEGIN_COM_MAP(CCopyHookObj)
		COM_INTERFACE_ENTRY_IID(IID_IShellCopyHook, CCopyHookObj)
		COM_INTERFACE_ENTRY(ICopyHookObj)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:



};

OBJECT_ENTRY_AUTO(__uuidof(CopyHookObj), CCopyHookObj)

