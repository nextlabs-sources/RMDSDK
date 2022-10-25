#pragma once
#include <Ole2.h>

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
	virtual HRESULT __stdcall DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
	virtual HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
	virtual HRESULT __stdcall DragLeave(void) override;
	virtual HRESULT __stdcall Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

private:
	IDropTarget* m_pProxied;
};