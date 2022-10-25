#pragma once
#include "Shlobj.h"

class Proxy_IFileDialog : public IFileDialog
{
public:
	Proxy_IFileDialog(IFileDialog* pIFileDialog) :m_pProxied(pIFileDialog){}
	~Proxy_IFileDialog() { if(m_pProxied) m_pProxied->Release(); }

public:

	STDMETHODIMP QueryInterface(REFIID riid,void **ppvObject);

	STDMETHODIMP_(ULONG) AddRef();

	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP Show(_In_opt_  HWND hwndOwner);

	STDMETHODIMP SetFileTypes(/* [in] */ UINT cFileTypes,
		/* [size_is][in] */ __RPC__in_ecount_full(cFileTypes) const COMDLG_FILTERSPEC* rgFilterSpec) {
		return m_pProxied->SetFileTypes(cFileTypes, rgFilterSpec);
	}

	STDMETHODIMP SetFileTypeIndex(/* [in] */ UINT iFileType) {
		return m_pProxied->SetFileTypeIndex(iFileType);
	}

	STDMETHODIMP GetFileTypeIndex(/* [out] */ __RPC__out UINT *piFileType) {
		return m_pProxied->GetFileTypeIndex(piFileType);
	}

	STDMETHODIMP Advise(/* [in] */ __RPC__in_opt IFileDialogEvents *pfde,
		/* [out] */ __RPC__out DWORD *pdwCookie) {
		return m_pProxied->Advise(pfde, pdwCookie);
	}

	STDMETHODIMP Unadvise(/* [in] */ DWORD dwCookie) {
		return m_pProxied->Unadvise(dwCookie);
	}

	STDMETHODIMP SetOptions(/* [in] */ FILEOPENDIALOGOPTIONS fos) {
		return m_pProxied->SetOptions(fos);
	}


	STDMETHODIMP GetOptions(/* [out] */ __RPC__out FILEOPENDIALOGOPTIONS *pfos) {
		return m_pProxied->GetOptions(pfos);
	}

	STDMETHODIMP SetDefaultFolder(/* [in] */ __RPC__in_opt IShellItem *psi) {
		return m_pProxied->SetDefaultFolder(psi);
	}

	STDMETHODIMP SetFolder(/* [in] */ __RPC__in_opt IShellItem *psi) {
		return m_pProxied->SetFolder(psi);
	}

	STDMETHODIMP GetFolder(/* [out] */ __RPC__deref_out_opt IShellItem **ppsi) {
		return m_pProxied->GetFolder(ppsi);
	}

	STDMETHODIMP GetCurrentSelection(/* [out] */ __RPC__deref_out_opt IShellItem **ppsi) {
		return m_pProxied->GetCurrentSelection(ppsi);
	}

	STDMETHODIMP SetFileName(/* [string][in] */ __RPC__in_string LPCWSTR pszName) {
		return m_pProxied->SetFileName(pszName);
	}

	STDMETHODIMP GetFileName(/* [string][out] */ __RPC__deref_out_opt_string LPWSTR *pszName) {
		return m_pProxied->GetFileName(pszName);
	}

	STDMETHODIMP SetTitle(/* [string][in] */ __RPC__in_string LPCWSTR pszTitle) {
		return m_pProxied->SetTitle(pszTitle);
	}

	STDMETHODIMP SetOkButtonLabel(/* [string][in] */ __RPC__in_string LPCWSTR pszText) {
		return m_pProxied->SetOkButtonLabel(pszText);
	}

	STDMETHODIMP SetFileNameLabel(/* [string][in] */ __RPC__in_string LPCWSTR pszLabel) {
		return m_pProxied->SetFileNameLabel(pszLabel);
	}

	STDMETHODIMP GetResult(/* [out] */ __RPC__deref_out_opt IShellItem **ppsi) {
		return m_pProxied->GetResult(ppsi);
	}

	STDMETHODIMP AddPlace(/* [in] */ __RPC__in_opt IShellItem *psi,
		/* [in] */ FDAP fdap) {
		return m_pProxied->AddPlace(psi, fdap);
	}

	STDMETHODIMP SetDefaultExtension(/* [string][in] */ __RPC__in_string LPCWSTR pszDefaultExtension) {
		return m_pProxied->SetDefaultExtension(pszDefaultExtension);
	}

	STDMETHODIMP Close(/* [in] */ HRESULT hr) {
		return m_pProxied->Close(hr);
	}

	STDMETHODIMP SetClientGuid(/* [in] */ __RPC__in REFGUID guid) {
		return m_pProxied->SetClientGuid(guid);
	}

	STDMETHODIMP ClearClientData() {
		return m_pProxied->ClearClientData();
	}

	STDMETHODIMP SetFilter(/* [in] */ __RPC__in_opt IShellItemFilter *pFilter) {
		return m_pProxied->SetFilter(pFilter);
	}

private:
	IFileDialog		*m_pProxied;

};


class Proxy_IFileSaveDialog : public Proxy_IFileDialog
{
public:
	Proxy_IFileSaveDialog(IFileSaveDialog* pIFileDialog) :Proxy_IFileDialog(pIFileDialog), m_pProxied(pIFileDialog) {};
	~Proxy_IFileSaveDialog() { m_pProxied->Release(); };

public:
	virtual HRESULT STDMETHODCALLTYPE SetSaveAsItem(IShellItem* psi) { return m_pProxied->SetSaveAsItem(psi); }

	virtual HRESULT STDMETHODCALLTYPE SetProperties(IPropertyStore* pStore) { return m_pProxied->SetProperties(pStore); }

	virtual HRESULT STDMETHODCALLTYPE SetCollectedProperties(IPropertyDescriptionList* pList, BOOL fAppendDefault) { 
		return m_pProxied->SetCollectedProperties(pList, fAppendDefault); }

	virtual HRESULT STDMETHODCALLTYPE GetProperties(IPropertyStore** ppStore) { return m_pProxied->GetProperties(ppStore); }

	virtual HRESULT STDMETHODCALLTYPE ApplyProperties(IShellItem* psi, IPropertyStore* pStore, HWND hwnd, IFileOperationProgressSink* pSink) {
		return m_pProxied->ApplyProperties(psi, pStore, hwnd, pSink);
	}

public: // override
	STDMETHODIMP QueryInterface(/* [in] */ __RPC__in REFIID riid,
		/* [annotation][iid_is][out] */	_COM_Outptr_  void** ppvObject);

	STDMETHODIMP Show(_In_opt_  HWND hwndOwner);

private:
	IFileSaveDialog		*m_pProxied;
};
