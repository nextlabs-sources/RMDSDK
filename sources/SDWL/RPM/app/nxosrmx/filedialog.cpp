#include "stdafx.h"
#include "filedialog.h"
#include "osrmx.h"


//
// base IFileDialog proxy
//
STDMETHODIMP Proxy_IFileDialog::QueryInterface( REFIID riid,  void **ppobj)
{
	HRESULT hRet = S_OK;
	void *punk = NULL;
	*ppobj = NULL;
	do
	{
		if (IID_IUnknown == riid)
		{
			punk = (IUnknown *)this;
		}
		else if (IID_IModalWindow == riid)
		{
			punk = (IModalWindow*)this;
		}
		else if (IID_IFileDialog == riid)
		{
			punk = (IFileDialog*)this;
		}
		//else if (IID_IFileSaveDialog == riid)
		//{
		//	hRet = m_pIFileDialog->QueryInterface(riid, &punk);

		//	if (SUCCEEDED(hRet))
		//	{
		//		CoreIFileSaveDialog *pSaveDialog = new CoreIFileSaveDialog((IFileSaveDialog*)punk);

		//		*ppobj = (void*)pSaveDialog;
		//	}

		//	break;
		//}
		else
		{
			hRet = m_pProxied->QueryInterface(riid, ppobj);
			break;
		}

		AddRef();

		*ppobj = punk;

	} while (FALSE);

	return hRet;
}

STDMETHODIMP_(ULONG) Proxy_IFileDialog::AddRef()
{	
	return m_pProxied->AddRef();
}

STDMETHODIMP_(ULONG) Proxy_IFileDialog::Release()
{
	ULONG uCount = m_pProxied->Release();
	if (!uCount)
	{
		m_pProxied = NULL;
		delete this;
	}
	return uCount;
}

STDMETHODIMP_(HRESULT __stdcall) Proxy_IFileDialog::Show(HWND hwndOwner) { 

	if (!nx::on_hook_saveas_dlg_will_open(false)) {
		return S_FALSE;
	}

	return m_pProxied->Show(hwndOwner); 
}

//
//	IFileSaveDialog Proxy
//
STDMETHODIMP Proxy_IFileSaveDialog::QueryInterface( REFIID riid,  void **ppobj)
{
	HRESULT hRet = S_OK;
	void *punk = NULL;
	*ppobj = NULL;

	if (IID_IUnknown == riid)
	{
		punk = (IUnknown *)this;
	}
	else if (IID_IModalWindow == riid)
	{
		punk = (IModalWindow*)this;
	}
	else if (IID_IFileDialog == riid)
	{
		punk = (IFileDialog*)this;
	}
	else if (IID_IFileSaveDialog == riid)
	{
		punk = (IFileSaveDialog*)this;
	}
	else
	{
		return m_pProxied->QueryInterface(riid, ppobj);
	}

	AddRef();

	*ppobj = punk;

	return hRet;
}

STDMETHODIMP Proxy_IFileSaveDialog::Show(_In_opt_  HWND hwndOwner)
{
	if (!nx::on_hook_saveas_dlg_will_open(false)) {
		return S_FALSE;
	}

	// current version only care about whether you can do saveas or not
	return  m_pProxied->Show(hwndOwner);

	//HRESULT hr = S_OK;
	//HRESULT hrlocal = S_OK;

	//IShellItem *pItem = NULL;

	//WCHAR *FileName = NULL;

	//WCHAR ActiveDocName[MAX_PATH] = { 0 };

	//wchar_t *pszSrcFile = new wchar_t[MAX_PATH];

	//LPWSTR* lpFileName = &pszSrcFile;
	//hr = m_pProxied->GetFileName(lpFileName);

	//IShellItem *pFolder = NULL;
	//hr = m_pProxied->GetFolder(&pFolder);
	//WCHAR *FileName2 = NULL;
	//if (SUCCEEDED(hr))
	//{
	//	pFolder->GetDisplayName(SIGDN_FILESYSPATH, &FileName2);
	//}

	//std::wstring strSrcFilePath = std::wstring(FileName2) + std::wstring(L"\\") + pszSrcFile;
	////delete[] pszSrcFile;
	//CoTaskMemFree(FileName2);
	//FileName2 = NULL;

	// hr =m_pProxied->Show(hwndOwner);

	//if (SUCCEEDED(hr))
	//{
	//	hrlocal = m_pProxied->GetResult(&pItem);

	//	if (SUCCEEDED(hrlocal))
	//	{
	//		hrlocal = pItem->GetDisplayName(SIGDN_FILESYSPATH, &FileName);

	//		if (SUCCEEDED(hrlocal))
	//		{
	//			do
	//			{				
	//				std::wstring originalNXLfilePath = std::wstring(FileName) + std::wstring(L".nxl");
	//				//nx::SaveAsFile(strSrcFilePath, originalNXLfilePath);
	//			} while (FALSE);

	//			::CoTaskMemFree(FileName);

	//			FileName = NULL;
	//		}

	//		pItem->Release();
	//	}
	//}

	//return hr;
}
