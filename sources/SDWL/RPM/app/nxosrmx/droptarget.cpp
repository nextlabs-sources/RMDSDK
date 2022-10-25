#pragma once
#include "stdafx.h"
#include "droptarget.h"
#include "osrmx.h"
#include <Shlobj.h>
#include "sdk.h"

namespace {

	class scope_guard {
	public:
		scope_guard(std::function<void()> fun) {
			m_f = fun;
		}
		~scope_guard() {
			m_f();
		}
	private:
		std::function<void()> m_f;
	};

	bool is_nxl(const std::wstring& path) {
		if (path.length() < 4) {
			return false;
		}
		if (nx::rm::is_nxl_file(path)) {
			return true;
		}
		else {
			if (0 == path.compare(path.length() - 4, 4, L".nxl")) {
				return true;
			}
		}
		return false;
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
		hr= pDataObj->GetData(&format, &medium);
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

		int total = ::DragQueryFileW(hdrop, -1, NULL, 0);
		for (int i = 0; i < total; i++) {
			wchar_t buf[0x400] = { 0 };
			::DragQueryFileW(hdrop, i, buf, 0x400);
			if (is_nxl(nx::utils::wstr_tolower(buf))) {
				return true;
			}
		}

		return false;
	}
}


HRESULT __stdcall Proxcy_IDropTarget::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	if (!m_pProxied) {
		return E_UNEXPECTED;
	}

	//if (is_contained_nxl_file(pDataObj)) {
	//	return S_OK;
	//}
	return m_pProxied->DragEnter(pDataObj, grfKeyState, pt, pdwEffect);

}

HRESULT __stdcall Proxcy_IDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	if (!m_pProxied) {
		return E_UNEXPECTED;
	}

	return m_pProxied->DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT __stdcall Proxcy_IDropTarget::DragLeave(void)
{
	if (!m_pProxied) {
		return E_UNEXPECTED;
	}
	return m_pProxied->DragLeave();
}

HRESULT __stdcall Proxcy_IDropTarget::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	if (!m_pProxied) {
		return E_UNEXPECTED;
	}
	if (is_contained_nxl_file(pDataObj)) {
		nx::rm::notify_message(L"Drop NXL file here is not allowed.");
		return S_OK;
	}
	return m_pProxied->Drop(pDataObj, grfKeyState, pt, pdwEffect);
}
