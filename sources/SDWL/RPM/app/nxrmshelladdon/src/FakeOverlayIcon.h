// FakeOverlayIcon.h : Declaration of the CFakeOverlayIcon

#pragma once
#include "resource.h"       // main symbols

#include <string>
#include <vector>

#include "nxrmshelladdon_i.h"



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CFakeOverlayIcon

class ATL_NO_VTABLE CFakeOverlayIcon :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFakeOverlayIcon, &CLSID_FakeOverlayIcon>,
	public IShellExtInit,
	public IContextMenu,
	public IShellIconOverlayIdentifier,
	public IDispatchImpl<IFakeOverlayIcon, &IID_IFakeOverlayIcon, &LIBID_nxrmshelladdonLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CFakeOverlayIcon() : bNeedBlock(FALSE)
	{
	}

	STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

	STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);

	virtual HRESULT STDMETHODCALLTYPE GetCommandString(_In_  UINT_PTR idCmd, _In_  UINT uType, UINT *pReserved, _Out_cap_(cchMax)  LPSTR pszName, _In_  UINT cchMax);

	STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);

	STDMETHODIMP IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);

	STDMETHODIMP GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int * pIndex, DWORD * pdwFlags);

	STDMETHODIMP GetPriority(int * pIPriority);

	DECLARE_REGISTRY_RESOURCEID(106)


	BEGIN_COM_MAP(CFakeOverlayIcon)
		COM_INTERFACE_ENTRY(IShellExtInit)
		COM_INTERFACE_ENTRY(IContextMenu)
		COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)
		COM_INTERFACE_ENTRY(IFakeOverlayIcon)
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

private:
	BOOL bNeedBlock;
	std::vector<std::wstring> vecSelectedFiles;
};

OBJECT_ENTRY_AUTO(__uuidof(FakeOverlayIcon), CFakeOverlayIcon)
