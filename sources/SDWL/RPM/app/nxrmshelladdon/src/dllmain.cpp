// dllmain.cpp : Implementation of DllMain. 

#include "stdafx.h"
#include "resource.h"
#include "common/celog2/celog.h"
#include "nxrmshelladdon_i.h"
#include "dllmain.h"
#include <Shlobj.h>
#include <map>
#include <set>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "madCHook_helper.h"
#include <nudf/conversion.hpp>
#include <nudf/winutil.hpp>
#include "SDLAPI.h"
#include "nudf/filesys.hpp"

#define CELOG_CUR_MODULE "nxrmshelladdon"
#define CELOG_CUR_FILE CELOG_FILEPATH_RMDSDK_MIN    // temporary

// The path in the source or destination or both was invalid, returned error by SHFileOperationW.
#define DE_INVALIDFILES 0x7C



//
// The definitions below are copied from ShObjIdl.h, so that we can get the
// function pointers in the VTables.
//
#if 1	/* C style interface */

typedef struct IShellExtInitVtbl
{
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        IShellExtInit * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        IShellExtInit * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        IShellExtInit * This);

    HRESULT ( STDMETHODCALLTYPE *Initialize )(
        IShellExtInit * This,
        /* [annotation][unique][in] */
        _In_opt_  PCIDLIST_ABSOLUTE pidlFolder,
        /* [annotation][unique][in] */
        _In_opt_  IDataObject *pdtobj,
        /* [annotation][unique][in] */
        _In_opt_  HKEY hkeyProgID);

    END_INTERFACE
} IShellExtInitVtbl;

interface IShellExtInit_C   /* originally IShellExtInit */
{
    CONST_VTBL struct IShellExtInitVtbl *lpVtbl;
};

typedef struct IFileOperationVtbl
{
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in REFIID riid,
        /* [annotation][iid_is][out] */
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        __RPC__in IFileOperation * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        __RPC__in IFileOperation * This);

    HRESULT ( STDMETHODCALLTYPE *Advise )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IFileOperationProgressSink *pfops,
        /* [out] */ __RPC__out DWORD *pdwCookie);

    HRESULT ( STDMETHODCALLTYPE *Unadvise )(
        __RPC__in IFileOperation * This,
        /* [in] */ DWORD dwCookie);

    HRESULT ( STDMETHODCALLTYPE *SetOperationFlags )(
        __RPC__in IFileOperation * This,
        /* [in] */ DWORD dwOperationFlags);

    HRESULT ( STDMETHODCALLTYPE *SetProgressMessage )(
        __RPC__in IFileOperation * This,
        /* [string][in] */ __RPC__in_string LPCWSTR pszMessage);

    HRESULT ( STDMETHODCALLTYPE *SetProgressDialog )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IOperationsProgressDialog *popd);

    HRESULT ( STDMETHODCALLTYPE *SetProperties )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IPropertyChangeArray *pproparray);

    HRESULT ( STDMETHODCALLTYPE *SetOwnerWindow )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in HWND hwndOwner);

    HRESULT ( STDMETHODCALLTYPE *ApplyPropertiesToItem )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IShellItem *psiItem);

    HRESULT ( STDMETHODCALLTYPE *ApplyPropertiesToItems )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IUnknown *punkItems);

    HRESULT ( STDMETHODCALLTYPE *RenameItem )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IShellItem *psiItem,
        /* [string][in] */ __RPC__in_string LPCWSTR pszNewName,
        /* [unique][in] */ __RPC__in_opt IFileOperationProgressSink *pfopsItem);

    HRESULT ( STDMETHODCALLTYPE *RenameItems )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IUnknown *pUnkItems,
        /* [string][in] */ __RPC__in_string LPCWSTR pszNewName);

    HRESULT ( STDMETHODCALLTYPE *MoveItem )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IShellItem *psiItem,
        /* [in] */ __RPC__in_opt IShellItem *psiDestinationFolder,
        /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszNewName,
        /* [unique][in] */ __RPC__in_opt IFileOperationProgressSink *pfopsItem);

    HRESULT ( STDMETHODCALLTYPE *MoveItems )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IUnknown *punkItems,
        /* [in] */ __RPC__in_opt IShellItem *psiDestinationFolder);

    HRESULT ( STDMETHODCALLTYPE *CopyItem )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IShellItem *psiItem,
        /* [in] */ __RPC__in_opt IShellItem *psiDestinationFolder,
        /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszCopyName,
        /* [unique][in] */ __RPC__in_opt IFileOperationProgressSink *pfopsItem);

    HRESULT ( STDMETHODCALLTYPE *CopyItems )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IUnknown *punkItems,
        /* [in] */ __RPC__in_opt IShellItem *psiDestinationFolder);

    HRESULT ( STDMETHODCALLTYPE *DeleteItem )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IShellItem *psiItem,
        /* [unique][in] */ __RPC__in_opt IFileOperationProgressSink *pfopsItem);

    HRESULT ( STDMETHODCALLTYPE *DeleteItems )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IUnknown *punkItems);

    HRESULT ( STDMETHODCALLTYPE *NewItem )(
        __RPC__in IFileOperation * This,
        /* [in] */ __RPC__in_opt IShellItem *psiDestinationFolder,
        /* [in] */ DWORD dwFileAttributes,
        /* [string][in] */ __RPC__in_string LPCWSTR pszName,
        /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszTemplateName,
        /* [unique][in] */ __RPC__in_opt IFileOperationProgressSink *pfopsItem);

    HRESULT ( STDMETHODCALLTYPE *PerformOperations )(
        __RPC__in IFileOperation * This);

    HRESULT ( STDMETHODCALLTYPE *GetAnyOperationsAborted )(
        __RPC__in IFileOperation * This,
        /* [out] */ __RPC__out BOOL *pfAnyOperationsAborted);

    END_INTERFACE
} IFileOperationVtbl;

interface IFileOperation_C  /* originally IFileOperation */
{
    CONST_VTBL struct IFileOperationVtbl *lpVtbl;
};

#endif 	/* C style interface */



#define PASSCODE    "{6829b159-b9bb-42fc-af19-4a6af3c9fcf6}"

CnxrmshelladdonModule _AtlModule;
HINSTANCE g_hInstance = NULL;
BOOL g_isWin7Above = false;
BOOL g_isDllhost = false;
BOOL g_isAdobe = false;
BOOL g_otherProcess = false;
NX::win::WINBUILDNUM g_winBuildNum;

const std::wstring g_dlgTitle = L"NextLabs SkyDRM";

ISDRmcInstance* pInstance = NULL;

typedef HRESULT(STDAPICALLTYPE* PF_SHCreateItemFromParsingName)(PCWSTR   pszPath, IBindCtx *pbc, REFIID   riid, void     **ppv);
PF_SHCreateItemFromParsingName          Hooked_SHCreateItemFromParsingName_Next = NULL;

static bool isDir2(const std::wstring& path);

static void parse_dirs(std::vector<std::wstring>& markDir, const WCHAR* str)
{
	std::wstringstream f(str);
	std::wstring s;
	while (std::getline(f, s, L'<'))
	{
		s.pop_back();   // Remove Ext option
		s.pop_back();   // Remove Overwrite option
		s.pop_back();   // Remove AutoProtect option
		markDir.push_back(s + L"\\");
		// Skip the file tags string.
		std::getline(f, s, L'<');
        // Skip the wsid string.
        std::getline(f, s, L'<');
        // Skip the rms-user-id string.
        std::getline(f, s, L'<');
        // Skip the RPM optioin string.
        std::getline(f, s, L'<');
    }
}

/** recursion_control
*
*  \brief Class to allow for recursion control.  The current thread or process may be
*         set in an enabled or disable mode.  This mode can be checked to determine
*         if the current context is performing recursion.
*/
class recursion_control : boost::noncopyable
{
public:

	/** initialize
	*
	*  \brief Initialize auto_Support API.
	*/
	recursion_control(void) : disabled_process(false), disabled_thread(), userDefinedData()
	{
		if (InitializeCriticalSectionAndSpinCount(&cs, 0x80004000) == FALSE)
		{
			InitializeCriticalSection(&cs);
		}
	}/* recursion_control */

	~recursion_control()
	{
		DeleteCriticalSection(&cs);
	}

	/** thread_enable
	*
	*  \brief Set hooking state for the current thread.  It is a programming error
	*         to call thread_enable for a thread that has not called
	*         thread_disable.
	*  \sa thread_disable
	*/
	void thread_enable(void)
	{
		EnterCriticalSection(&cs);
		assert(disabled_thread[GetCurrentThreadId()] > 0);
		disabled_thread[GetCurrentThreadId()]--;
		LeaveCriticalSection(&cs);
	}/* thread_enable */

	 /** thread_disable
	 *
	 *  \brief Set hooking state for the current thread.  After this is called the
	 *         current thread is set to disable recursion.
	 *  \sa thread_enable
	 */
	void thread_disable(void)
	{
		DWORD tid = GetCurrentThreadId();
		EnterCriticalSection(&cs);
		/* Initialize thread state to zero when this is the first state change. */
		if (disabled_thread.find(tid) == disabled_thread.end())
		{
			disabled_thread[tid] = 0;
		}
		disabled_thread[tid]++;
		LeaveCriticalSection(&cs);
	}/* thread_disable */

	 /** process_enable
	 *
	 *  \brief Enable hooking/recursion for the current process.
	 *  \sa process_disable
	 */
	void process_enable(void)
	{
		EnterCriticalSection(&cs);
		disabled_process = false;
		LeaveCriticalSection(&cs);
	}/* process_enable */

	 /** process_disable
	 *
	 *  \brief Disable hooking for the current process.
	 *  \sa process_enable
	 */
	void process_disable(void)
	{
		EnterCriticalSection(&cs);
		disabled_process = true;
		LeaveCriticalSection(&cs);
	}/* process_disable */

	 /** is_process_disabled
	 *
	 *  \brief Determine the hook state for the current process.
	 *  \return true if the current process' hooks are disabled, otherwise
	 *        false.
	 *  \sa process_disable, process_enable
	 */
	bool is_process_disabled(void)
	{
		bool result;
		EnterCriticalSection(&cs);
		result = disabled_process;
		LeaveCriticalSection(&cs);
		return result;
	}/* is_process_disabled */

	 /** is_thread_disabled
	 *
	 *  \brief Determine the hook state for the current process.
	 *  \return true if the current thread's hooking is disabled, otherwise
	 *          false.
	 *  \sa thread_disable, thread_enable
	 */
	bool is_thread_disabled(void)
	{
		bool result = false;
		DWORD tid = GetCurrentThreadId();
		EnterCriticalSection(&cs);
		/* If the current thread ID is not in the map it's not been disabled */
		if (disabled_thread.find(tid) != disabled_thread.end())
		{
			if (disabled_thread[tid] > 0)
			{
				result = true;
			}
		}
		LeaveCriticalSection(&cs);
		return result;
	}/* is_thread_disabled */

	 /** is_disabled
	 *
	 *  \brief Determine the hook/recursion state for the current process or thread.
	 *  \return true if hooks are disabled for the current code path,
	 *          otherwise false.
	 *  \sa process_disable, process_enable, thread_disable, thread_enable
	 */
	bool is_disabled(void)
	{
		bool result;
		EnterCriticalSection(&cs);
		result = is_thread_disabled() || is_process_disabled();
		LeaveCriticalSection(&cs);
		return result;
	}/* is_disabled */

	 /** setUserDefinedData
	 *
	 *  \brief Set the user defined data
	 *
	 */
	void setUserDefinedData(PVOID pUserDefinedData)
	{
		DWORD tid = GetCurrentThreadId();
		EnterCriticalSection(&cs);
		userDefinedData[tid] = pUserDefinedData;
		LeaveCriticalSection(&cs);
	}

	/** getUserDefinedData
	*
	*  \brief Get the user defined data
	*
	*/
	PVOID getUserDefinedData(void)
	{
		PVOID pResult = NULL;

		DWORD tid = GetCurrentThreadId();
		EnterCriticalSection(&cs);
		if (userDefinedData.find(tid) != userDefinedData.end())
		{
			pResult = userDefinedData[tid];
		}
		LeaveCriticalSection(&cs);

		return pResult;
	}

private:
	std::map<DWORD, int>  disabled_thread;   /* Thread state is disabled? */
	bool                 disabled_process;  /* Process state is disabled? */
	std::map<DWORD, PVOID>  userDefinedData; /* User defined data */
	CRITICAL_SECTION     cs;                /* Protect state variables */
};/* auto_control */

  /** recursion_control_auto
  *
  *  \brief Handle thread (current context) disablement automatically.  When an
  *         instance of this object is created it will disable recusion for the
  *         current thread until is is destroyed.  Typically this occurs when the
  *         stack is being unwound since an instance of this object would typically
  *         be local to the context.
  */
class recursion_control_auto : boost::noncopyable
{
public:
	/** recursion_control_auto
	*
	*  \brief Construct an auto recursion control object.
	*
	*  \param in_ac (in) Recursion control instance.
	*/
	recursion_control_auto(recursion_control& in_ac) : ac(in_ac)
	{
		ac.thread_disable();
	}/* recursion_control_auto */

	~recursion_control_auto(void)
	{
		ac.thread_enable();
	}/* ~recursion_control_auto */

private:
	recursion_control & ac;

};/* recursion_control_auto */

recursion_control hook_control;

std::wstring GetFilePathFromName(const std::wstring& fullPath)
{
	if (fullPath.empty())
	{
		return fullPath;
	}

	size_t pos = fullPath.rfind(L'\\');
	if (pos == std::wstring::npos)
	{
		return fullPath;
	}
	else
	{
		return fullPath.substr(0, pos + 1);
	}
}

std::wstring GetFinalComponentFromFullPath(const std::wstring& fullPath)
{
	if (fullPath.empty())
	{
		return fullPath;
	}

	size_t pos = fullPath.rfind(L'\\');
	if (pos == std::wstring::npos)
	{
		return fullPath;
	}
	else
	{
		return fullPath.substr(pos + 1);
	}
}

class CFileSysBindData : public IFileSystemBindData
{
public:
	static HRESULT CreateInstance(_In_ const WIN32_FIND_DATAW *pfd, _In_ REFIID riid, void **ppv);

	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, void **ppv)
	{
		*ppv = 0;
		HRESULT hr = E_NOINTERFACE;
		if (riid == IID_IUnknown || riid == IID_IFileSystemBindData)
		{
			*ppv = static_cast<IFileSystemBindData *>(this);
			AddRef();
			hr = S_OK;
		}
		return hr;
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		LONG cRef = InterlockedDecrement(&m_cRef);
		if (cRef == 0) delete this;
		return cRef;
	}

	// *** IFileSystemBindData *** 
	IFACEMETHODIMP SetFindData(_In_ const WIN32_FIND_DATAW *pfd)
	{
		m_fd = *pfd;
		return S_OK;
	}

	IFACEMETHODIMP GetFindData(_Out_ WIN32_FIND_DATAW *pfd)
	{
		*pfd = m_fd;
		return S_OK;
	}

private:
	CFileSysBindData(_In_ const WIN32_FIND_DATAW *pfd) : m_cRef(1)
	{
		m_fd = *pfd;
	}
private:
	LONG m_cRef;
	WIN32_FIND_DATAW m_fd;
};

HRESULT CFileSysBindData::CreateInstance(_In_ const WIN32_FIND_DATAW *pfd, _In_ REFIID riid, void **ppv)
{
	*ppv = 0;
	ATL::CComPtr<IFileSystemBindData> spfsbd;
	HRESULT hr = E_OUTOFMEMORY;
	spfsbd.Attach(new (std::nothrow) CFileSysBindData(pfd));
	if (spfsbd)
	{
		hr = spfsbd->QueryInterface(riid, ppv);
	}
	return hr;
}

HRESULT CreateBindCtxWithOpts(_In_ BIND_OPTS *pbo, IBindCtx **ppbc)
{
	ATL::CComPtr<IBindCtx> spbc;
	HRESULT hr = CreateBindCtx(0, &spbc);
	if (SUCCEEDED(hr))
	{
		hr = spbc->SetBindOptions(pbo);
	}
	*ppbc = SUCCEEDED(hr) ? spbc.Detach() : 0;
	return hr;
}

HRESULT AddFileSysBindCtx(_In_ IBindCtx *pbc, _In_ const WIN32_FIND_DATAW *pfd)
{
	ATL::CComPtr<IFileSystemBindData> spfsbc;
	HRESULT hr = CFileSysBindData::CreateInstance(pfd, IID_PPV_ARGS(&spfsbc));
	if (SUCCEEDED(hr))
	{
		hr = pbc->RegisterObjectParam(STR_FILE_SYS_BIND_DATA, spfsbc);
	}
	return hr;
}

HRESULT CreateFileSysBindCtx(_In_ const WIN32_FIND_DATAW *pfd, IBindCtx **ppbc)
{
	ATL::CComPtr<IBindCtx> spbc;
	BIND_OPTS bo = { sizeof(bo), 0, STGM_CREATE, 0 };
	HRESULT hr = CreateBindCtxWithOpts(&bo, &spbc);
	if (SUCCEEDED(hr))
	{
		hr = AddFileSysBindCtx(spbc, pfd);
	}
	*ppbc = SUCCEEDED(hr) ? spbc.Detach() : 0;
	return hr;
}

HRESULT CreateSimpleShellItemFromPath(_In_ const WIN32_FIND_DATAW *pfd, _In_ PCWSTR pszPath, _In_ REFIID riid, void **ppv)
{
	*ppv = 0;
	ATL::CComPtr<IBindCtx> spbc;
	HRESULT hr = CreateFileSysBindCtx(pfd, &spbc);
	if (SUCCEEDED(hr))
	{
		if (g_isWin7Above)
		{
			hr = SHCreateItemFromParsingName(pszPath, spbc, riid, ppv);
		}
		else
		{
			if (Hooked_SHCreateItemFromParsingName_Next != NULL)
			{
				hr = Hooked_SHCreateItemFromParsingName_Next(pszPath, spbc, riid, ppv);
			}
			else
			{
				hr = SHCreateItemFromParsingName(pszPath, spbc, riid, ppv);
			}
		}
	}
	return hr;
}

#ifdef _DEBUG 

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

void AssertRPMFolderRelation(RPMFolderRelation r)
{
	if (r.bUnknownRelation)
	{
		assert(!r.bNormalFolder &&
			!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder) &&
			!(r.bSanctuaryAncestralFolder || r.bSanctuaryFolder || r.bSanctuaryInheritedFolder));
	}
	else if (r.bNormalFolder)
	{
		assert(!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder) &&
			!(r.bSanctuaryAncestralFolder || r.bSanctuaryFolder || r.bSanctuaryInheritedFolder));
	}
	else if (r.bRPMFolder || r.bRPMInheritedFolder)
	{
		assert(!(r.bSanctuaryAncestralFolder || r.bSanctuaryFolder || r.bSanctuaryInheritedFolder));
	}
	else if (r.bSanctuaryFolder || r.bSanctuaryInheritedFolder)
	{
		assert(!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder));
	}
	else
	{
		assert(r.bRPMAncestralFolder || r.bSanctuaryAncestralFolder);
	}
}

#else   // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

void AssertRPMFolderRelation(RPMFolderRelation r)
{
	if (r.bUnknownRelation)
	{
		assert(!r.bNormalFolder &&
			!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder));
	}
	else if (r.bNormalFolder)
	{
		assert(!(r.bRPMAncestralFolder || r.bRPMFolder || r.bRPMInheritedFolder));
	}
	else if (r.bRPMFolder || r.bRPMInheritedFolder)
	{
		// Nothing 
	}
	else
	{
		assert(r.bRPMAncestralFolder);
	}
}

#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

void AssertRPMFileRelation(RPMFileRelation r)
{
	assert(r >= 0 && r < RPMFileRelationMax);
}

void AssertRPMRelation(bool isFolder, RPMRelation r)
{
	if (isFolder)
	{
		AssertRPMFolderRelation(r.folder);
	}
	else
	{
		AssertRPMFileRelation(r.file);
	}
}

#endif // _DEBUG 

typedef struct _PATHINFO
{
	std::wstring wstrPath;
	bool isFolder;
	RPMRelation rpmRelation;

	// For RPM folder
	struct {
		SDRmRPMFolderOption rpmFolderOption;
	};

	std::wstring fileTags;
}PATHINFO;

RPMFolderRelation GetFolderRelation(const std::wstring& folderPath, SDRmRPMFolderOption *pRpmFolderOption, std::wstring *pFileTags)
{
	CELOG_ENTER;

	if (pFileTags != NULL)
	{
		pFileTags->clear();
	}

	uint32_t dirstatus = 0;
	SDRmRPMFolderOption tempOption = (SDRmRPMFolderOption)0;
	std::wstring tempTags;
	SDWLResult res;
	RPMFolderRelation relation = { false,
								  true,                 // set bNormalFolder to true 
								  {false, false, false},
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
								  {false, false, false}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
	};

	// IsRPMFolder and IsSanctuaryFolder only support drive letter paths.  If 
	// folderPath is UNC path, it must be a network path which cannot be RPM 
	// or Sanctuary dir anyway. 
	std::wstring _filepath = NX::fs::dos_fullfilepath(folderPath).path();
	if (PathIsUNCW(_filepath.c_str()))
	{
		CELOG_RETURN_VAL_T(relation);
	}

	res = pInstance->IsRPMFolder(folderPath, &dirstatus, &tempOption, tempTags);
	if (res.GetCode() != 0)
	{
		CELOG_LOG(CELOG_ERROR, L"IsRPMFolder failed, res=%s\n", res.ToString().c_str());
		relation = {
			true,                   // set bUnknownRelation = true 
			false,
			{false, false, false},
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			{false, false, false}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
		};
		CELOG_RETURN_VAL_T(relation);
	}
	else
	{
		if (pRpmFolderOption != NULL)
		{
			*pRpmFolderOption = tempOption;
		}

		if (pFileTags != NULL)
		{
			*pFileTags = tempTags;
		}

		if (dirstatus & RPM_SAFEDIRRELATION_SAFE_DIR)
		{
			relation.bRPMFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR)
		{
			relation.bRPMAncestralFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)
		{
			relation.bRPMInheritedFolder = true;
			relation.bNormalFolder = false;
		}
	}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
	// Since a folder cannot be both RPM and Sanctuary, we stop here if there 
	// is already an RPM relationship so as to avoid spending time calling 
	// pInstance->IsSanctuaryFolder(). 
	if (!relation.bNormalFolder)
	{
		CELOG_RETURN_VAL_T(relation);
	}

	res = pInstance->IsSanctuaryFolder(folderPath, &dirstatus, tempTags);
	if (res.GetCode() != 0)
	{
		CELOG_LOG(CELOG_ERROR, L"IsSanctuaryFolder failed, res=%s\n", res.ToString().c_str());
		relation = {
			true,                   // set bUnknownRelation = true 
			false,
			{false, false, false},
			{false, false, false}
		};
		CELOG_RETURN_VAL_T(relation);
	}
	else
	{
		if (pFileTags != NULL)
		{
			*pFileTags = tempTags;
		}

		if (dirstatus & RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR)
		{
			relation.bSanctuaryFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR)
		{
			relation.bSanctuaryAncestralFolder = true;
			relation.bNormalFolder = false;
		}
		if (dirstatus & RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR)
		{
			relation.bSanctuaryInheritedFolder = true;
			relation.bNormalFolder = false;
		}
	}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

	CELOG_RETURN_VAL_T(relation);
}

RPMFolderRelation GetFolderRelationUsingParent(const std::wstring& folderPath, SDRmRPMFolderOption *pRpmFolderOption, std::wstring *pFileTags)
{
	CELOG_ENTER;

	RPMFolderRelation relation = {
		false,
		true,                   // set bNormalFolder to true
		{false, false, false},
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		{false, false, false}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	};

	std::wstring::size_type index = folderPath.rfind(L'\\');
	if (index == std::wstring::npos)
	{
		// Can't determine parent path.
		relation.bUnknownRelation = true;
		relation.bNormalFolder = false;
		CELOG_RETURN_VAL_T(relation);
	}

	const std::wstring parentFolderPath = folderPath.substr(0, index);
	RPMFolderRelation parentRelation = GetFolderRelation(parentFolderPath, pRpmFolderOption, pFileTags);

	if (parentRelation.bUnknownRelation)
	{
		relation.bUnknownRelation = true;
		relation.bNormalFolder = false;
	}
	else if (parentRelation.bRPMFolder || parentRelation.bRPMInheritedFolder)
	{
		relation.bRPMInheritedFolder = true;
		relation.bNormalFolder = false;
	}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	else if (parentRelation.bSanctuaryFolder || parentRelation.bSanctuaryInheritedFolder)
	{
		relation.bSanctuaryInheritedFolder = true;
		relation.bNormalFolder = false;
	}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	CELOG_RETURN_VAL_T(relation);
}

RPMFileRelation GetFileRelation(const std::wstring& filePath, BOOL bVerify, SDRmRPMFolderOption *pRpmFolderOption = NULL, std::wstring *pFileTags = NULL)
{
	CELOG_ENUM_ENTER(RPMFileRelation);

	NX::fs::dos_fullfilepath input_path(filePath);
	if (bVerify)
	{
		if (PathIsDirectoryW(input_path.global_dos_path().c_str()))
		{
			CELOG_ENUM_RETURN_VAL(NonNXLFileInNormalDir);
		}
	}

	std::wstring folderPath;

	size_t pos = filePath.rfind(L'\\');
	if (pos == std::wstring::npos)
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			CELOG_ENUM_RETURN_VAL(NXLFileInNormalDir);
		}
		else
		{
			CELOG_ENUM_RETURN_VAL(NonNXLFileInNormalDir);
		}
	}
	else
	{
		folderPath = filePath.substr(0, pos);
	}

	RPMFolderRelation folderRelation = GetFolderRelation(folderPath, pRpmFolderOption, pFileTags);
#ifdef _DEBUG 
	AssertRPMFolderRelation(folderRelation);
#endif 

	if (folderRelation.bRPMFolder || folderRelation.bRPMInheritedFolder)
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			CELOG_ENUM_RETURN_VAL(NXLFileInRPMDir);
		}

		std::wstring fileWithNxlPath = input_path.global_dos_path() + L".nxl";

		WIN32_FIND_DATAW findFileData = { 0 };
		HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			CELOG_ENUM_RETURN_VAL(NonNXLFileInRPMDir);
		}
		else
		{
			FindClose(hFind);
			CELOG_ENUM_RETURN_VAL(NXLFileInRPMDir);
		}
	}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
	else if (folderRelation.bSanctuaryFolder || folderRelation.bSanctuaryInheritedFolder)
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			CELOG_ENUM_RETURN_VAL(NXLFileInSanctuaryDir);
		}
		else
		{
			CELOG_ENUM_RETURN_VAL(NonNXLFileInSanctuaryDir);
		}
	}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
	else if (folderRelation.bUnknownRelation)
	{
		CELOG_ENUM_RETURN_VAL(UnknownRelation);
	}
	else
	{
		if (boost::algorithm::iends_with(input_path.path(), L".nxl"))
		{
			CELOG_ENUM_RETURN_VAL(NXLFileInNormalDir);
		}
		else
		{
			CELOG_ENUM_RETURN_VAL(NonNXLFileInNormalDir);
		}
	}
}

VOID ParseFolder(const std::wstring& strPath, std::vector<PATHINFO>& vecFile)
{
	NX::fs::dos_fullfilepath input_path(strPath, false);

	std::wstring strTmpPath = input_path.global_dos_path() + L"\\*.*";

	WIN32_FIND_DATA FindFileData = { 0 };
	HANDLE hFile = FindFirstFileW(strTmpPath.c_str(), &FindFileData);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		if (StrCmpW(FindFileData.cFileName, L".") == 0 || StrCmpW(FindFileData.cFileName, L"..") == 0)
		{
			continue;
		}

		strTmpPath = input_path.path() + L"\\";
		strTmpPath.append(FindFileData.cFileName);

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ParseFolder(strTmpPath, vecFile);
		}
		else
		{
			SDRmRPMFolderOption rpmFolderOption;
			std::wstring fileTags;
			RPMRelation rpmRelation;
			rpmRelation.file = GetFileRelation(strTmpPath, FALSE, &rpmFolderOption, &fileTags);
			PATHINFO pathInfo = { strTmpPath, FALSE, rpmRelation, {rpmFolderOption}, fileTags };
			vecFile.push_back(pathInfo);
		}
	} while (FindNextFileW(hFile, &FindFileData));
	FindClose(hFile);
}

HRESULT GetPathByShellItem(std::wstring& path, IShellItem* pItem)
{
	if (pItem == NULL)
	{
		return E_FAIL;
	}

	wchar_t* pOutStr = NULL;
	HRESULT hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pOutStr);
	if (SUCCEEDED(hr) && pOutStr != NULL)
	{
		path.assign(pOutStr);
		if (boost::algorithm::iends_with(pOutStr, L"\\"))
		{
			path.erase(path.size() - 1);
		}

		CoTaskMemFree(pOutStr);
	}

	return hr;
}

BOOL GetPathByShellArray(IShellItemArray* pShArray, std::vector<PATHINFO>& vecPathsInfo, std::wstring& strSrcRoot, bool bRecursive, bool bForceRecursive, bool bRPMInfo)
{
	if (pShArray == NULL)
	{
		return FALSE;
	}

	DWORD dwSize = 0;
	HRESULT hr = pShArray->GetCount(&dwSize);
	if (FAILED(hr) && dwSize == 0)
	{
		return FALSE;
	}

	std::wstring fullpath;
	IShellItem* pItem = NULL;
	for (DWORD i = 0; i < dwSize; i++)
	{
		if (FAILED(pShArray->GetItemAt(i, &pItem)))
		{
			continue;
		}
		if (FAILED(GetPathByShellItem(fullpath, pItem)))
		{
			pItem->Release();
			continue;
		}
		pItem->Release();

		NX::fs::dos_fullfilepath _fullpath(fullpath, false);
		if (strSrcRoot.empty())
		{
			strSrcRoot = _fullpath.path();
			strSrcRoot.erase(strSrcRoot.rfind(L'\\'));
		}

		RPMRelation rpmRelation;

		if (PathIsDirectoryW(_fullpath.global_dos_path().c_str()))
		{
			if (bRPMInfo)
			{
				SDRmRPMFolderOption rpmFolderOption;
				std::wstring fileTags;
				rpmRelation.folder = GetFolderRelation(fullpath, &rpmFolderOption, &fileTags);
#ifdef _DEBUG 
				AssertRPMFolderRelation(rpmRelation.folder);
#endif 
				if ((!rpmRelation.folder.bNormalFolder && bRecursive) || bForceRecursive)
				{
					ParseFolder(fullpath, vecPathsInfo);
				}
				else
				{
					PATHINFO pathInfo = { _fullpath.path(), TRUE, rpmRelation, {rpmFolderOption}, fileTags };
					vecPathsInfo.push_back(pathInfo);
				}
			}
			else
			{
				rpmRelation.folder = {
					true,                   // set bUnknownRelation to true 
					false,
					{false, false, false},
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
					{false, false, false}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
				};
				PATHINFO pathInfo = { _fullpath.path(), TRUE, rpmRelation };
				vecPathsInfo.push_back(pathInfo);
			}
		}
		else
		{
			if (bRPMInfo)
			{
				SDRmRPMFolderOption rpmFolderOption;
				std::wstring fileTags;
				rpmRelation.file = GetFileRelation(fullpath, FALSE, &rpmFolderOption, &fileTags);
				PATHINFO pathInfo = { _fullpath.path(), FALSE, rpmRelation, {rpmFolderOption}, fileTags };
				vecPathsInfo.push_back(pathInfo);
			}
			else
			{
				rpmRelation.file = UnknownRelation;
				PATHINFO pathInfo = { _fullpath.path(), FALSE, rpmRelation };
				vecPathsInfo.push_back(pathInfo);
			}
		}
	}

	return TRUE;
}

std::vector<PATHINFO> GetFilePathsFromObject(IUnknown* pObj, std::wstring& strSrcRoot, bool bRecursive, bool bForceRecursive, bool bRPMInfo)
{
	std::vector<PATHINFO> vecPathsInfo;

	ATL::CComPtr<IDataObject> pData = NULL;
	HRESULT hr = pObj->QueryInterface(IID_IDataObject, (void**)&pData);
	if (SUCCEEDED(hr) && pData != NULL)
	{
		ATL::CComPtr<IShellItemArray> pSArray = NULL;
		hr = SHCreateShellItemArrayFromDataObject(pData, IID_IShellItemArray, (void**)&pSArray);
		if (SUCCEEDED(hr))
		{
			GetPathByShellArray(pSArray, vecPathsInfo, strSrcRoot, bRecursive, bForceRecursive, bRPMInfo);
		}
	}
	else if (E_NOINTERFACE == hr) // try IID_IShellItem. 
	{
		ATL::CComPtr<IShellItem> pItem = NULL;
		hr = pObj->QueryInterface(IID_IShellItem, (void**)&pItem);
		if (SUCCEEDED(hr) && pItem != NULL)
		{
			std::wstring path;
			GetPathByShellItem(path, pItem);
			if (path.empty())
			{
				return vecPathsInfo;
			}

			NX::fs::dos_fullfilepath _fullpath(path, false);
			strSrcRoot = _fullpath.path();
			strSrcRoot.erase(strSrcRoot.rfind(L'\\'));

			RPMRelation rpmRelation;

			if (PathIsDirectoryW(_fullpath.global_dos_path().c_str()))
			{
				if (bRPMInfo)
				{
					SDRmRPMFolderOption rpmFolderOption;
					std::wstring fileTags;
					rpmRelation.folder = GetFolderRelation(path, &rpmFolderOption, &fileTags);
#ifdef _DEBUG 
					AssertRPMFolderRelation(rpmRelation.folder);
#endif 
					if ((!rpmRelation.folder.bNormalFolder && bRecursive) || bForceRecursive)
					{
						ParseFolder(path, vecPathsInfo);
					}
					else
					{
						PATHINFO pathInfo = { _fullpath.path(), TRUE, rpmRelation, {rpmFolderOption}, fileTags };
						vecPathsInfo.push_back(pathInfo);
					}
				}
				else
				{
					rpmRelation.folder = {
						true,                   // set bUnknownRelation to true 
						false,
						{false, false, false},
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
						{false, false, false}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
					};
					PATHINFO pathInfo = { _fullpath.path(), TRUE, rpmRelation };
					vecPathsInfo.push_back(pathInfo);
				}
			}
			else
			{
				if (bRPMInfo)
				{
					SDRmRPMFolderOption rpmFolderOption;
					std::wstring fileTags;
					rpmRelation.file = GetFileRelation(path, FALSE, &rpmFolderOption, &fileTags);
					PATHINFO pathInfo = { _fullpath.path(), FALSE, rpmRelation, {rpmFolderOption}, fileTags };
					vecPathsInfo.push_back(pathInfo);
				}
				else
				{
					rpmRelation.file = UnknownRelation;
					PATHINFO pathInfo = { _fullpath.path(), FALSE, rpmRelation };
					vecPathsInfo.push_back(pathInfo);
				}
			}
		}
		else if (E_NOINTERFACE == hr)
		{
			ATL::CComPtr<IShellItemArray> pSArray = NULL;
			hr = pObj->QueryInterface(IID_IShellItemArray, (void**)&pSArray);
			if (SUCCEEDED(hr) && pSArray != NULL)
			{
				GetPathByShellArray(pSArray, vecPathsInfo, strSrcRoot, bRecursive, bForceRecursive, bRPMInfo);
			}
		}
	}

	return vecPathsInfo;
}

void GetRelativeFilePath(const std::wstring& strPath, const std::wstring& strSrcRoot, std::wstring& strRelativePath)
{
	strRelativePath = strPath;

	if (boost::istarts_with(strRelativePath, strSrcRoot))
	{
		strRelativePath.erase(0, strSrcRoot.length() + 1);
	}
}

BOOL DoFileOperation(const std::vector<std::pair<std::wstring, std::wstring>>& vecFileOp, bool bIsCopy, BOOL bRenameOnCollision)
{
	if (vecFileOp.empty())
	{
		return TRUE;
	}

	SHFILEOPSTRUCTW theStruct = { 0 };
	if (bIsCopy)
	{
		if (bRenameOnCollision)
		{
			theStruct.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR | FOF_RENAMEONCOLLISION;
		}
		else
		{
			theStruct.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR;
		}
		theStruct.wFunc = FO_COPY;
	}
	else
	{
		if (bRenameOnCollision)
		{
			theStruct.fFlags = FOF_MULTIDESTFILES | FOF_RENAMEONCOLLISION;
		}
		else
		{
			theStruct.fFlags = FOF_MULTIDESTFILES;
		}
		theStruct.wFunc = FO_MOVE;
	}

	size_t dwSrcLen = 0;
	size_t dwDstLen = 0;
	std::vector<std::pair<std::wstring, std::wstring>>::const_iterator viter = vecFileOp.begin();
	for (; viter != vecFileOp.end(); ++viter)
	{
		dwSrcLen += viter->first.length();
		dwSrcLen += 1; // add end \0 
		dwDstLen += viter->second.length();
		dwDstLen += 1;
	}
	dwSrcLen += 1; // end by \0\0 
	dwDstLen += 1;

	wchar_t* szSrc = new wchar_t[dwSrcLen]();
	wchar_t* szDst = new wchar_t[dwDstLen]();;

	size_t nSrcIndex = 0;
	size_t nDstIndex = 0;
	for (viter = vecFileOp.begin(); viter != vecFileOp.end(); ++viter)
	{
		memcpy_s(szSrc + nSrcIndex, (dwSrcLen - nSrcIndex) * sizeof(WCHAR), viter->first.c_str(), viter->first.length() * sizeof(WCHAR));
		nSrcIndex += viter->first.length() + 1; // 1 -- ??/0??  
		memcpy_s(szDst + nDstIndex, (dwDstLen - nDstIndex) * sizeof(WCHAR), viter->second.c_str(), viter->second.length() * sizeof(WCHAR));
		nDstIndex += viter->second.length() + 1;
	}

	theStruct.pFrom = szSrc;
	theStruct.pTo = szDst;

	int rt = SHFileOperationW(&theStruct);
	if (rt == ERROR_FILENAME_EXCED_RANGE || rt == DE_INVALIDFILES) {
		const std::wstring dlgMsg(L"Some file name(s) would be too long for the destination folder. You can shorten the file name and try again, or try a location that has a shorter path.");
		MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONSTOP);
	}

	BOOL bSuccess = rt == 0 ? TRUE : FALSE; 

	delete[] szSrc;
	delete[] szDst;

	return bSuccess;
}

BOOL DoFileDelete(const std::vector<std::wstring>& vecFileOp, bool bAllowUndo, bool bNoConfirmation)
{
	if (vecFileOp.empty())
	{
		return TRUE;
	}

	SHFILEOPSTRUCTW theStruct = { 0 };
	if (bAllowUndo)
	{
		theStruct.fFlags = FOF_MULTIDESTFILES | FOF_ALLOWUNDO;
	}
	else
	{
		theStruct.fFlags = FOF_MULTIDESTFILES;
	}

	if (bNoConfirmation)
	{
		theStruct.fFlags |= FOF_NOCONFIRMATION;
	}

	theStruct.wFunc = FO_DELETE;

	std::wstring pathBuf;
	for (std::vector<std::wstring>::const_iterator ci = vecFileOp.begin(); ci != vecFileOp.end(); ++ci)
	{
		pathBuf.append(*ci);
		pathBuf.append(L"\0", 1);
	}
	pathBuf.append(L"\0", 1);

	theStruct.pFrom = pathBuf.c_str();

	return SHFileOperationW(&theStruct) == 0 ? TRUE : FALSE;
}

typedef HRESULT(STDMETHODCALLTYPE *PF_COMCopyItem)(IFileOperation * This, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszCopyName, IFileOperationProgressSink *pfopsItem);
typedef HRESULT(STDMETHODCALLTYPE *PF_COMCopyItems)(IFileOperation * This, IUnknown *punkItems, IShellItem *psiDestinationFolder);
typedef HRESULT(STDMETHODCALLTYPE *PF_COMMoveItem)(IFileOperation * This, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszNewName, IFileOperationProgressSink *pfopsItem);
typedef HRESULT(STDMETHODCALLTYPE *PF_COMMoveItems)(IFileOperation * This, IUnknown *punkItems, IShellItem *psiDestinationFolder);
typedef HRESULT(STDMETHODCALLTYPE *PF_COMDeleteItems)(IFileOperation * This, IUnknown *punkItems);
typedef HRESULT(STDMETHODCALLTYPE *PF_COMRenameItem)(IFileOperation * This, IShellItem *psiItem, LPCWSTR pszNewName, IFileOperationProgressSink *pfopsItem);
typedef HRESULT(STDMETHODCALLTYPE *PF_COMInitialize)(IShellExtInit * This, PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);


PF_COMCopyItem pnext_CopyItem = NULL;
HRESULT STDMETHODCALLTYPE Hooked_COMCopyItem(IFileOperation* This, __RPC__in_opt IShellItem *psiItem, __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszCopyName, __RPC__in_opt IFileOperationProgressSink *pfopsItem)
{
	CELOG_ENTER;

	if (hook_control.is_disabled())
	{
		CELOG_RETURN_VAL(pnext_CopyItem(This, psiItem, psiDestinationFolder, pszCopyName, pfopsItem));
	}

	recursion_control_auto auto_disable(hook_control);

	std::wstring srcPath;
	GetPathByShellItem(srcPath, psiItem);

	std::wstring desPath;
	GetPathByShellItem(desPath, psiDestinationFolder);

	RPMFolderRelation desFolderRpmRelation = GetFolderRelation(desPath);
#ifdef _DEBUG
	AssertRPMFolderRelation(desFolderRpmRelation);
#endif
	if (desFolderRpmRelation.bUnknownRelation)
	{
		// Error occurred while trying to determine the relationship.
		CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
	}

	if (isDir2(srcPath))
	{
		RPMFolderRelation srcFolderRpmRelation = GetFolderRelation(srcPath);
#ifdef _DEBUG
		AssertRPMFolderRelation(srcFolderRpmRelation);
#endif

		if (srcFolderRpmRelation.bNormalFolder)
		{
			CELOG_RETURN_VAL(pnext_CopyItem(This, psiItem, psiDestinationFolder, pszCopyName, pfopsItem));
		}

		std::wstring strSrcRoot;
		std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(psiItem, strSrcRoot, true, false, true);
		HRESULT hr;

		for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
		{
			WIN32_FIND_DATAW findFileData = { 0 };

			HANDLE hFind = FindFirstFileW(ci->wstrPath.c_str(), &findFileData);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				continue;
			}
			FindClose(hFind);

			std::wstring strRelativePath;
			GetRelativeFilePath(ci->wstrPath, strSrcRoot, strRelativePath);
			if (strRelativePath.empty())
			{
				continue;
			}

			if (ci->rpmRelation.file == UnknownRelation)
			{
				// Error occurred while trying to determine the relationship.
				CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
			}

			ATL::CComPtr<IShellItem> pFinalSrcFileItem, pFinalDestFolderItem;

			if (ci->rpmRelation.file == NXLFileInRPMDir)
			{
				// Copying NXL file from RPM dir to any dir.
				hr = CreateSimpleShellItemFromPath(&findFileData, (ci->wstrPath + L".nxl").c_str(), IID_IShellItem, (void **) &pFinalSrcFileItem);
			}
			else
			{
				hr = CreateSimpleShellItemFromPath(&findFileData, ci->wstrPath.c_str(), IID_IShellItem, (void **) &pFinalSrcFileItem);
			}

			// Create the destination directory if it doesn't exist.
			std::wstring finalDesFolderPath = desPath + L'\\' + strRelativePath;
			finalDesFolderPath.erase(finalDesFolderPath.rfind(L'\\'));
			if (GetFileAttributesW(finalDesFolderPath.c_str()) == INVALID_FILE_ATTRIBUTES && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
			{
				int ret = SHCreateDirectory(NULL, finalDesFolderPath.c_str());
				if (ret != ERROR_SUCCESS) {

					const std::wstring dlgTitle = L"Copy Protected File";
					const std::wstring dlgMsg = L"Failed to create destination folder.";
					MessageBoxW(NULL, dlgMsg.c_str(), dlgTitle.c_str(), MB_OK | MB_ICONERROR);
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ret));
				}
			}

			hFind = FindFirstFileW(finalDesFolderPath.c_str(), &findFileData);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				continue;
			}
			FindClose(hFind);

			hr = CreateSimpleShellItemFromPath(&findFileData, finalDesFolderPath.c_str(), IID_IShellItem, (void **) &pFinalDestFolderItem);
			hr = pnext_CopyItem(This, pFinalSrcFileItem, pFinalDestFolderItem, NULL, pfopsItem);
		}

		CELOG_RETURN_VAL(hr);
	}
	else
	{
		RPMFileRelation srcFileRpmRelation = GetFileRelation(srcPath, FALSE);
		LPCWSTR newDestFileName = pszCopyName;
		std::wstring newDestFileNameStr;

		if (srcFileRpmRelation == NXLFileInRPMDir && (desFolderRpmRelation.bNormalFolder || desFolderRpmRelation.bRPMAncestralFolder))
		{

			if (pszCopyName == NULL)
			{
				newDestFileNameStr = GetFinalComponentFromFullPath(srcPath);
			}
			else
			{
				newDestFileNameStr = pszCopyName;
			}

			newDestFileNameStr += L".nxl";
			newDestFileName = newDestFileNameStr.c_str();
		}

		CELOG_RETURN_VAL(pnext_CopyItem(This, psiItem, psiDestinationFolder, newDestFileName, pfopsItem));
	}
}

PF_COMCopyItems pnext_CopyItems = NULL;
HRESULT STDMETHODCALLTYPE Hooked_COMCopyItems(IFileOperation* This, __RPC__in_opt IUnknown *punkItems, __RPC__in_opt IShellItem *psiDestinationFolder)
{
	CELOG_ENTER;

	if (hook_control.is_disabled())
	{
		CELOG_RETURN_VAL(pnext_CopyItems(This, punkItems, psiDestinationFolder));
	}

	recursion_control_auto auto_disable(hook_control);

	if (g_otherProcess)
	{
		std::wstring strSrcRoot;
		std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, false, false, false);

		for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
		{
			if (!ci->isFolder)
			{
				std::wstring fileWithNxlPath = ci->wstrPath + L".nxl";

				WIN32_FIND_DATAW findFileData = { 0 };
				HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
				if (hFind != INVALID_HANDLE_VALUE)
				{
					FindClose(hFind);

					if (!boost::algorithm::iends_with(findFileData.cFileName, L".nxl"))
					{
						CELOG_RETURN_VAL(S_OK);
					}
				}
			}
			else
			{
				BOOL bNeedBlock = FALSE;

				HKEY hKey = NULL;
				if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\NextLabs\\SkyDRM", 0, KEY_READ | KEY_WOW64_64KEY, &hKey))
				{
					DWORD cbData = 0;

					if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, NULL, &cbData) && 0 != cbData)
					{
						LPBYTE lpData = new BYTE[cbData];
						if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, lpData, &cbData))
						{
							std::wstring wstrPath = ci->wstrPath + L"\\";
							std::transform(wstrPath.begin(), wstrPath.end(), wstrPath.begin(), tolower);

							std::vector<std::wstring> markDir;
							parse_dirs(markDir, (const WCHAR*)lpData);

							for (std::wstring& dir : markDir)
							{
								if (dir.size() == wstrPath.size())
								{
									if (boost::algorithm::iequals(wstrPath, dir))
									{
										bNeedBlock = TRUE;
										break;
									}
								}
								else if (dir.size() < wstrPath.size())
								{
									if (boost::algorithm::istarts_with(wstrPath, dir))
									{
										bNeedBlock = TRUE;
										break;
									}
								}
								else
								{
									if (boost::algorithm::istarts_with(dir, wstrPath))
									{
										bNeedBlock = TRUE;
										break;
									}
								}
							}
						}

						delete[]lpData;
					}

					RegCloseKey(hKey);
				}

				if (bNeedBlock)
				{
					CELOG_RETURN_VAL(S_OK);
				}
			}
		}
	}

	std::wstring desPath;
	GetPathByShellItem(desPath, psiDestinationFolder);

	SDRmRPMFolderOption desFolderRpmOption = (SDRmRPMFolderOption)0;
	std::wstring desFolderFileTags;
	RPMFolderRelation desFolderRpmRelation = GetFolderRelation(desPath, &desFolderRpmOption, &desFolderFileTags);
#ifdef _DEBUG 
	AssertRPMFolderRelation(desFolderRpmRelation);
#endif 
	if (desFolderRpmRelation.bUnknownRelation)
	{
		// Error occurred while trying to determine the relationship. 
		CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
	}

	const bool bForceRecursive = ((desFolderRpmRelation.bRPMFolder || desFolderRpmRelation.bRPMInheritedFolder) &&
								  (desFolderRpmOption & RPMFOLDER_AUTOPROTECT));

	std::wstring strSrcRoot;
	std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, true, bForceRecursive, true);

	if (vecPathsInfo.empty())
	{
		CELOG_RETURN_VAL(pnext_CopyItems(This, punkItems, psiDestinationFolder));
	}

	ISDRmcInstance *pLoggedInInstance = NULL;
	ISDRmTenant *pLoggedInTenant = NULL;
	ISDRmUser *pLoggedInUser = NULL;
	BOOL bChanged = false;

	std::vector<std::pair<std::wstring, std::wstring>> vecFileOp;

	for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
	{
		CELOG_LOG(CELOG_DEBUG, L"ci = {\"%s\", %s, %s}\n",
			ci->wstrPath.c_str(),
			ci->isFolder ? L"true" : L"false",
			(ci->isFolder ? ci->rpmRelation.folder.ToString() : NX::conversion::to_wstring(ci->rpmRelation.file)).c_str());
		std::wstring fileName;
		std::wstring::size_type index = ci->wstrPath.rfind(L'\\');
		if (index == std::wstring::npos)
		{
			fileName = ci->wstrPath;
		}
		else
		{
			fileName = ci->wstrPath.substr(index);
		}

		std::wstring strRelativePath;
		GetRelativeFilePath(ci->wstrPath, strSrcRoot, strRelativePath);
		if (strRelativePath.empty())
		{
			continue;
		}

#ifdef _DEBUG 
		AssertRPMRelation(ci->isFolder, ci->rpmRelation);
#endif 
		if (!ci->isFolder)
		{
			SDWLResult res;

			if (ci->rpmRelation.file == UnknownRelation)
			{
				// Error occurred while trying to determine the relationship. 
				CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
			}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			// If copying file with .nxl extension from Sanctuary dir to 
			// non-Sanctuary dir, we need to check if the file content is 
			// actually in NXL format.  If it is not, we still need to protect 
			// the .nxl file anyway. 
			uint32_t dirStatus;
			bool bContentInNxlFormat = (ci->rpmRelation.file == NXLFileInRPMDir || ci->rpmRelation.file == NXLFileInSanctuaryDir);  // assume true for .nxl file 
			if (ci->rpmRelation.file == NXLFileInSanctuaryDir &&
				!(desFolderRpmRelation.bSanctuaryFolder || desFolderRpmRelation.bSanctuaryInheritedFolder))
			{
				res = pInstance->RPMGetFileStatus(ci->wstrPath, &dirStatus, &bContentInNxlFormat);
				if (!res)
				{
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
				}
			}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

			if (ci->rpmRelation.file == NXLFileInRPMDir)
			{
				vecFileOp.push_back(std::make_pair(ci->wstrPath + L".nxl", desPath + L'\\' + strRelativePath + L".nxl"));
				bChanged = true;
			}
			else if ((ci->rpmRelation.file == NonNXLFileInNormalDir || ci->rpmRelation.file == NonNXLFileInRPMDir) &&
					 ((desFolderRpmRelation.bRPMFolder || desFolderRpmRelation.bRPMInheritedFolder) && (desFolderRpmOption & RPMFOLDER_AUTOPROTECT)))
			{
				// Copying non-NXL file from normal or RPM dir to RPM dir with auto-protect
				if (pLoggedInInstance == NULL)
				{
					std::string passcode(PASSCODE);
					res = RPMGetCurrentLoggedInUser(passcode, pLoggedInInstance, pLoggedInTenant, pLoggedInUser);
					if (!res)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
					}
				}

				// If destination non-NXL file path is the same as source file path, the user is trying to copy from a dir to the
				// same dir.  Deny it.
				const std::wstring desNonNxlFilePath = desPath + L'\\' + strRelativePath;
				if (boost::algorithm::iequals(ci->wstrPath, desNonNxlFilePath))
				{
					const std::wstring dlgMsg = L"The source and destination filenames are the same.";
					MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONSTOP);
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED));
				}

				// If destination non-NXL file already exists and Overwrite option is not set, Deny it.
				if (!(desFolderRpmOption & RPMFOLDER_OVERWRITE) &&
					GetFileAttributesW(desNonNxlFilePath.c_str()) != INVALID_FILE_ATTRIBUTES)
				{
					const std::wstring dlgMsg = L"The destination file cannot be overwritten.";
					MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
					CELOG_RETURN_VAL(E_ACCESSDENIED);
				}

				// If destination file already exists, need to confirm with user and then delete it first.
				std::wstring desNxlFilePath = desPath + L'\\' + strRelativePath + L".nxl";

				if (GetFileAttributesW(desNxlFilePath.c_str()) != INVALID_FILE_ATTRIBUTES)
				{
					const std::wstring dlgQuestion = desNxlFilePath + L" already exists.  Do you want to overwrite it?";
					int ret = MessageBoxW(NULL, dlgQuestion.c_str(), g_dlgTitle.c_str(), MB_YESNOCANCEL | MB_ICONWARNING);

					if (ret == IDCANCEL)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_CANCELLED));
					}
					else if (ret == IDNO)
					{
						bChanged = true;
						continue;
					}
					else
					{
						if (!DeleteFile(desNxlFilePath.c_str()))
						{
							CELOG_RETURN_VAL(E_ACCESSDENIED);
						}
					}
				}

				// Create the destination directory if it doesn't exist.
				desNxlFilePath.erase(desNxlFilePath.rfind(L'\\'));
				if (GetFileAttributesW(desNxlFilePath.c_str()) == INVALID_FILE_ATTRIBUTES && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
				{
					// Handle the long path issue.
					std::wstring dosfullpath = NX::fs::dos_fullfilepath(desNxlFilePath, false).global_dos_path();
					int ret = ::CreateDirectoryW(dosfullpath.c_str(), NULL);
					if (ret == 0) {
						// Maybe one or more intermediate dir don't exist, we use SHCreateDirectory to create it again, but it won't support long path.
						ret = SHCreateDirectory(NULL, desNxlFilePath.c_str());
						if (ret != ERROR_SUCCESS) {
							SDWLibDeleteRmcInstance(pLoggedInInstance);

							const std::wstring dlgMsg = L"Failed to copy and protect file in destination folder.";
							MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
							CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ret));
						}
					}
				}

				// Generate the destination file by protecting the source file
				const std::vector<SDRmFileRight> rights;
				const SDR_WATERMARK_INFO watermarkInfo;
				const SDR_Expiration expire;
				const std::string fileTagsUtf8 = NX::conversion::utf16_to_utf8(desFolderFileTags);
				res = pLoggedInUser->ProtectFile(ci->wstrPath, desNxlFilePath, rights, watermarkInfo, expire, fileTagsUtf8, pLoggedInUser->GetMembershipID(0), false);
				if (!res)
				{
					SDWLibDeleteRmcInstance(pLoggedInInstance);
                    const std::wstring dlgMsg = L"Failed to copy and protect file in destination folder.";
                    MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
				}

				bChanged = true;
			}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			else if ((ci->rpmRelation.file == NonNXLFileInSanctuaryDir || (ci->rpmRelation.file == NXLFileInSanctuaryDir && !bContentInNxlFormat)) &&
				!(desFolderRpmRelation.bSanctuaryFolder || desFolderRpmRelation.bSanctuaryInheritedFolder))
			{
				// Copying non-NXL file from Sanctuary dir to non-Sanctuary dir 
				if (pLoggedInInstance == NULL)
				{
					std::string passcode(PASSCODE);
					res = RPMGetCurrentLoggedInUser(passcode, pLoggedInInstance, pLoggedInTenant, pLoggedInUser);
					if (!res)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
					}
				}

				// If destination file already exists, need to confirm with user and then delete it first. 
				std::wstring desNxlFilePath = desPath + L'\\' + strRelativePath + L".nxl";

				if (GetFileAttributesW(desNxlFilePath.c_str()) != INVALID_FILE_ATTRIBUTES)
				{
					const std::wstring dlgQuestion = desNxlFilePath + L" already exists.  Do you want to overwrite it?";
					int ret = MessageBoxW(NULL, dlgQuestion.c_str(), g_dlgTitle.c_str(), MB_YESNOCANCEL | MB_ICONWARNING);

					if (ret == IDCANCEL)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_CANCELLED));
					}
					else if (ret == IDNO)
					{
						bChanged = true;
						continue;
					}
					else
					{
						if (!DeleteFile(desNxlFilePath.c_str()))
						{
							CELOG_RETURN_VAL(E_ACCESSDENIED);
						}
					}
				}

				// Create the destination directory if it doesn't exist.
				desNxlFilePath.erase(desNxlFilePath.rfind(L'\\'));
				if (GetFileAttributesW(desNxlFilePath.c_str()) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND)
				{
					// Handle the long path issue.
					std::wstring dosfullpath = NX::fs::dos_fullfilepath(desNxlFilePath, false).global_dos_path();
					int ret = ::CreateDirectoryW(dosfullpath.c_str(), NULL);
					if (ret == 0) {
						// Maybe one or more intermediate dir don't exist, we use SHCreateDirectory to create it again, but it won't support long path.
						ret = SHCreateDirectory(NULL, desNxlFilePath.c_str());
						if (ret != ERROR_SUCCESS) {
							SDWLibDeleteRmcInstance(pLoggedInInstance);

							const std::wstring dlgMsg = L"Failed to copy and protect file in destination folder.";
							MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
							CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ret));
				        }
					}

				}

				// Generate the destination file by protecting the source file 
				const std::vector<SDRmFileRight> rights;
				const SDR_WATERMARK_INFO watermarkInfo;
				const SDR_Expiration expire;
				const std::string fileTagsUtf8 = NX::conversion::utf16_to_utf8(ci->fileTags);
				res = pLoggedInUser->ProtectFile(ci->wstrPath, desNxlFilePath, rights, watermarkInfo, expire, fileTagsUtf8, pLoggedInUser->GetMembershipID(0), false);
				if (!res)
				{
					SDWLibDeleteRmcInstance(pLoggedInInstance);
					const std::wstring dlgMsg = L"Failed to copy and protect file in destination folder.";
					MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
				}

				bChanged = true;
			}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			else
			{
				vecFileOp.push_back(std::make_pair(ci->wstrPath, desPath + L'\\' + strRelativePath));
			}
		}
		else
		{
			vecFileOp.push_back(std::make_pair(ci->wstrPath, desPath + L'\\' + strRelativePath));
		}
	}

	if (pLoggedInInstance != NULL)
	{
		SDWLibDeleteRmcInstance(pLoggedInInstance);
	}

	if (!bChanged)
	{
		CELOG_RETURN_VAL(pnext_CopyItems(This, punkItems, psiDestinationFolder));
	}

	if (boost::iequals(strSrcRoot, desPath))
	{
		DoFileOperation(vecFileOp, true, true);
	}
	else
	{
		DoFileOperation(vecFileOp, true, false);
	}

	CELOG_RETURN_VAL(S_OK);
}

PF_COMMoveItem pnext_MoveItem = NULL;
HRESULT STDMETHODCALLTYPE Hooked_COMMoveItem(IFileOperation* This, __RPC__in_opt IShellItem *psiItem, __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszNewName, __RPC__in_opt IFileOperationProgressSink *pfopsItem)
{
	CELOG_ENTER;

	if (hook_control.is_disabled())
	{
		CELOG_RETURN_VAL(pnext_MoveItem(This, psiItem, psiDestinationFolder, pszNewName, pfopsItem));
	}

	recursion_control_auto auto_disable(hook_control);

	std::wstring srcPath;
	GetPathByShellItem(srcPath, psiItem);

	std::wstring desPath;
	GetPathByShellItem(desPath, psiDestinationFolder);

	RPMFolderRelation desFolderRpmRelation = GetFolderRelation(desPath);
#ifdef _DEBUG
	AssertRPMFolderRelation(desFolderRpmRelation);
#endif
	if (desFolderRpmRelation.bUnknownRelation)
	{
		// Error occurred while trying to determine the relationship.
		CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
	}

	std::pair<std::wstring, std::wstring> deniedFileOp;
	std::pair<std::wstring, std::wstring> fileOp;

	do
	{
		if (isDir2(srcPath))
		{
			if (srcPath.size() > 1 && desPath.size() > 1 && !boost::algorithm::iequals(srcPath.substr(0, 2), desPath.substr(0, 2)))
			{
				RPMFolderRelation srcFolderRpmRelation = GetFolderRelation(srcPath);
#ifdef _DEBUG
				AssertRPMFolderRelation(srcFolderRpmRelation);
#endif
				if (srcFolderRpmRelation.bUnknownRelation)
				{
					// Error occurred while trying to determine the relationship.
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
				}

				if (srcFolderRpmRelation.bRPMFolder || srcFolderRpmRelation.bRPMAncestralFolder)
				{
					deniedFileOp = std::make_pair(srcPath, desPath);
					break;
				}
				else if (srcFolderRpmRelation.bRPMInheritedFolder)
				{
					std::wstring wstrTemp = desPath;
					std::wstring::size_type index = srcPath.rfind(L'\\');
					if (index == std::wstring::npos)
					{
						wstrTemp += srcPath;
					}
					else
					{
						wstrTemp += srcPath.substr(index);
					}

					// If the dir already exists at destination, get the RPM relation of the existing dir.  Else infer the RPM
					// relation using the RPM relation of the destination parent dir.
					RPMFolderRelation desSubFolderRPMRelation;
					if (GetFileAttributesW(wstrTemp.c_str()) != INVALID_FILE_ATTRIBUTES)
					{
						desSubFolderRPMRelation = GetFolderRelation(wstrTemp);
					}
					else
					{
						desSubFolderRPMRelation = GetFolderRelationUsingParent(wstrTemp);
					}
#ifdef _DEBUG
					AssertRPMFolderRelation(desSubFolderRPMRelation);
#endif
					if (desSubFolderRPMRelation.bUnknownRelation)
					{
						// Error occurred while trying to determine the relationship.
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
					}

					if (!desSubFolderRPMRelation.bRPMFolder && !desSubFolderRPMRelation.bRPMInheritedFolder)
					{
						deniedFileOp = std::make_pair(srcPath, desPath);
						break;
					}
				}
			}

			fileOp = std::make_pair(srcPath, desPath);
		}
		else
		{
			RPMFileRelation srcFileRpmRelation = GetFileRelation(srcPath, FALSE);
			LPCWSTR newDestFileName = pszNewName;
			std::wstring newDestFileNameStr;

			if (srcFileRpmRelation == NXLFileInRPMDir && desFolderRpmRelation.bNormalFolder)
			{
				if (pszNewName == NULL)
				{
					newDestFileNameStr = GetFinalComponentFromFullPath(srcPath);
				}
				else
				{
					newDestFileNameStr = pszNewName;
				}

				newDestFileNameStr += L".nxl";
				newDestFileName = newDestFileNameStr.c_str();
			}

			CELOG_RETURN_VAL(pnext_MoveItem(This, psiItem, psiDestinationFolder, newDestFileName, pfopsItem));
		}
	} while (false);

	if (!deniedFileOp.first.empty())
	{
		std::vector<std::pair<std::wstring, std::wstring>> tempVecFileOp;
		tempVecFileOp.push_back(deniedFileOp);

		hook_control.setUserDefinedData((PVOID)&(deniedFileOp.first));
		DoFileOperation(tempVecFileOp, false, false);
		hook_control.setUserDefinedData(NULL);

		SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, GetFilePathFromName(deniedFileOp.first).c_str(), 0);

		// Since we are returning without calling pnext_MoveItem() first, this
		// will cause McAfee File and Removable Media Protection to display a
		// "Catastrophic failure" error dialog.  Currently we don't have a
		// solution for this problem.
		CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
	}

	if (!fileOp.first.empty())
	{
		CELOG_RETURN_VAL(pnext_MoveItem(This, psiItem, psiDestinationFolder, pszNewName, pfopsItem));
	}

	CELOG_RETURN_VAL(S_OK);
}

PF_COMMoveItems pnext_MoveItems = NULL;
HRESULT STDMETHODCALLTYPE Hooked_COMMoveItems(IFileOperation* This, __RPC__in_opt IUnknown *punkItems, __RPC__in_opt IShellItem *psiDestinationFolder)
{
	CELOG_ENTER;

	if (hook_control.is_disabled())
	{
		CELOG_RETURN_VAL(pnext_MoveItems(This, punkItems, psiDestinationFolder));
	}

	recursion_control_auto auto_disable(hook_control);

	if (g_otherProcess)
	{
		std::wstring strSrcRoot;
		std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, false, false, false);

		for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
		{
			if (!ci->isFolder)
			{
				std::wstring fileWithNxlPath = ci->wstrPath + L".nxl";

				WIN32_FIND_DATAW findFileData = { 0 };
				HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
				if (hFind != INVALID_HANDLE_VALUE)
				{
					FindClose(hFind);

					if (!boost::algorithm::iends_with(findFileData.cFileName, L".nxl"))
					{
						CELOG_RETURN_VAL(S_OK);
					}
				}
			}
			else
			{
				BOOL bNeedBlock = FALSE;

				HKEY hKey = NULL;
				if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\NextLabs\\SkyDRM", 0, KEY_READ | KEY_WOW64_64KEY, &hKey))
				{
					DWORD cbData = 0;

					if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, NULL, &cbData) && 0 != cbData)
					{
						LPBYTE lpData = new BYTE[cbData];
						if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, lpData, &cbData))
						{
							std::wstring wstrPath = ci->wstrPath + L"\\";
							std::transform(wstrPath.begin(), wstrPath.end(), wstrPath.begin(), tolower);

							std::vector<std::wstring> markDir;
							parse_dirs(markDir, (const WCHAR*)lpData);

							for (std::wstring& dir : markDir)
							{
								if (dir.size() == wstrPath.size())
								{
									if (boost::algorithm::iequals(wstrPath, dir))
									{
										bNeedBlock = TRUE;
										break;
									}
								}
								else if (dir.size() < wstrPath.size())
								{
									if (boost::algorithm::istarts_with(wstrPath, dir))
									{
										bNeedBlock = TRUE;
										break;
									}
								}
								else
								{
									if (boost::algorithm::istarts_with(dir, wstrPath))
									{
										bNeedBlock = TRUE;
										break;
									}
								}
							}
						}

						delete[]lpData;
					}

					RegCloseKey(hKey);
				}

				if (bNeedBlock)
				{
					CELOG_RETURN_VAL(S_OK);
				}
			}
		}
	}

	std::wstring desPath;
	GetPathByShellItem(desPath, psiDestinationFolder);

	SDRmRPMFolderOption desFolderRpmOption = (SDRmRPMFolderOption)0;
	std::wstring desFolderFileTags;
	RPMFolderRelation desFolderRpmRelation = GetFolderRelation(desPath, &desFolderRpmOption, &desFolderFileTags);
#ifdef _DEBUG 
	AssertRPMFolderRelation(desFolderRpmRelation);
#endif 
	if (desFolderRpmRelation.bUnknownRelation)
	{
		// Error occurred while trying to determine the relationship. 
		CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
	}

	const bool bForceRecursive = ((desFolderRpmRelation.bRPMFolder || desFolderRpmRelation.bRPMInheritedFolder) &&
								  (desFolderRpmOption & RPMFOLDER_AUTOPROTECT));

    if (bForceRecursive)
    {
        // Move file into MyFolder
        // we shall not allow move a RPM folder into another RPM folder
        std::wstring strSrcRoot;
        std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, false, false, true);
        if (vecPathsInfo.empty())
        {
            CELOG_RETURN_VAL(pnext_MoveItems(This, punkItems, psiDestinationFolder));
        }

        for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
        {
            if (ci->isFolder && ci->rpmRelation.folder.bRPMFolder)
            {
                const std::wstring dlgMsg = L"Access denied. You cannot move a NextLabs secured folder.";
                MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONSTOP);
                CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
            }
            else if (ci->isFolder && ci->rpmRelation.folder.bRPMAncestralFolder)
            {
                const std::wstring dlgMsg = L"Access denied. You cannot move a folder which contains NextLabs secured folder.";
                MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONSTOP);
                CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
            }
        }
    }

	std::wstring strSrcRoot;
	std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, bForceRecursive, bForceRecursive, true);

	if (vecPathsInfo.empty())
	{
		CELOG_RETURN_VAL(pnext_MoveItems(This, punkItems, psiDestinationFolder));
	}

	ISDRmcInstance *pLoggedInInstance = NULL;
	ISDRmTenant *pLoggedInTenant = NULL;
	ISDRmUser *pLoggedInUser = NULL;
	BOOL bChanged = false;
	std::vector<std::pair<std::wstring, std::wstring>> deniedVecFileOp;
	std::vector<std::pair<std::wstring, std::wstring>> vecFileOp;

	for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
	{
		CELOG_LOG(CELOG_DEBUG, L"ci = {\"%s\", %s, %s}\n",
			ci->wstrPath.c_str(),
			ci->isFolder ? L"true" : L"false",
			(ci->isFolder ? ci->rpmRelation.folder.ToString() : NX::conversion::to_wstring(ci->rpmRelation.file)).c_str());
		std::wstring strRelativePath;
		GetRelativeFilePath(ci->wstrPath, strSrcRoot, strRelativePath);
		if (strRelativePath.empty())
		{
			continue;
		}

#ifdef _DEBUG 
		AssertRPMRelation(ci->isFolder, ci->rpmRelation);
#endif 
		if (ci->isFolder)
		{
			if (ci->wstrPath.size() > 1 && desPath.size() > 1 && !boost::algorithm::iequals(ci->wstrPath.substr(0, 2), desPath.substr(0, 2)))
			{
				if (ci->rpmRelation.folder.bUnknownRelation)
				{
					// Error occurred while trying to determine the relationship. 
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
				}

				if (ci->rpmRelation.folder.bRPMFolder || ci->rpmRelation.folder.bRPMAncestralFolder
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
					|| ci->rpmRelation.folder.bSanctuaryFolder || ci->rpmRelation.folder.bSanctuaryAncestralFolder
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
					)
				{
					deniedVecFileOp.push_back(std::make_pair(ci->wstrPath, desPath));
					continue;
				}
				else if (ci->rpmRelation.folder.bRPMInheritedFolder)
				{
					std::wstring wstrTemp = desPath;
					std::wstring::size_type index = ci->wstrPath.rfind(L'\\');
					if (index == std::wstring::npos)
					{
						wstrTemp += ci->wstrPath;
					}
					else
					{
						wstrTemp += ci->wstrPath.substr(index);
					}

					// If the dir already exists at destination, get the RPM relation of the existing dir.	Else infer the RPM
					// relation using the RPM relation of the destination parent dir.
					RPMFolderRelation desSubFolderRPMRelation;
					if (GetFileAttributesW(wstrTemp.c_str()) != INVALID_FILE_ATTRIBUTES)
					{
						desSubFolderRPMRelation = GetFolderRelation(wstrTemp);
					}
					else
					{
						desSubFolderRPMRelation = GetFolderRelationUsingParent(wstrTemp);
					}
#ifdef _DEBUG 
					AssertRPMFolderRelation(desSubFolderRPMRelation);
#endif 
					if (desSubFolderRPMRelation.bUnknownRelation)
					{
						// Error occurred while trying to determine the relationship. 
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
					}

					if (!desSubFolderRPMRelation.bRPMFolder && !desSubFolderRPMRelation.bRPMInheritedFolder)
					{
						deniedVecFileOp.push_back(std::make_pair(ci->wstrPath, desPath));
						continue;
					}
				}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
				else if (ci->rpmRelation.folder.bSanctuaryInheritedFolder)
				{
					if (!desFolderRpmRelation.bSanctuaryFolder && !desFolderRpmRelation.bSanctuaryInheritedFolder)
					{
						deniedVecFileOp.push_back(std::make_pair(ci->wstrPath, desPath));
						continue;
					}
				}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			}

			vecFileOp.push_back(std::make_pair(ci->wstrPath, desPath + L'\\' + strRelativePath));
		}
		else
		{
			std::wstring fileName;
			std::wstring::size_type index = ci->wstrPath.rfind(L'\\');
			if (index == std::wstring::npos)
			{
				fileName = ci->wstrPath;
			}
			else
			{
				fileName = ci->wstrPath.substr(index);
			}

			SDWLResult res;

			if (ci->rpmRelation.file == UnknownRelation)
			{
				// Error occurred while trying to determine the relationship. 
				CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
			}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			// If moving file with .nxl extension from Sanctuary dir to 
			// non-Sanctuary dir, we need to check if the file content is 
			// actually in NXL format.  If it is not, we still need to protect 
			// the .nxl file anyway. 
			uint32_t dirStatus;
			bool bContentInNxlFormat = (ci->rpmRelation.file == NXLFileInRPMDir || ci->rpmRelation.file == NXLFileInSanctuaryDir);  // assume true for .nxl file 
			if (ci->rpmRelation.file == NXLFileInSanctuaryDir &&
				!(desFolderRpmRelation.bSanctuaryFolder || desFolderRpmRelation.bSanctuaryInheritedFolder))
			{
				res = pInstance->RPMGetFileStatus(ci->wstrPath, &dirStatus, &bContentInNxlFormat);
				if (!res)
				{
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
				}
			}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

			if (ci->rpmRelation.file == NXLFileInRPMDir)
			{
				vecFileOp.push_back(std::make_pair(ci->wstrPath + L".nxl", desPath + L'\\' + strRelativePath + L".nxl"));
				bChanged = true;
			}
			else if ((ci->rpmRelation.file == NonNXLFileInNormalDir || ci->rpmRelation.file == NonNXLFileInRPMDir) &&
					 ((desFolderRpmRelation.bRPMFolder || desFolderRpmRelation.bRPMInheritedFolder) && (desFolderRpmOption & RPMFOLDER_AUTOPROTECT)))
			{
				// Moving non-NXL file from normal or RPM dir to RPM dir with auto-protect
				if (pLoggedInInstance == NULL)
				{
					std::string passcode(PASSCODE);
					res = RPMGetCurrentLoggedInUser(passcode, pLoggedInInstance, pLoggedInTenant, pLoggedInUser);
					if (!res)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
					}
				}

				// If destination non-NXL file path is the same as source file path, the user is trying to move from a dir to the
				// same dir.  Deny it.
				const std::wstring desNonNxlFilePath = desPath + L'\\' + strRelativePath;
				if (boost::algorithm::iequals(ci->wstrPath, desNonNxlFilePath))
				{
					const std::wstring dlgMsg = L"The source and destination filenames are the same.";
					MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONSTOP);
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED));
				}

				// If destination non-NXL file already exists and Overwrite option is not set, Deny it.
				if (!(desFolderRpmOption & RPMFOLDER_OVERWRITE) &&
					GetFileAttributesW(desNonNxlFilePath.c_str()) != INVALID_FILE_ATTRIBUTES)
				{
					const std::wstring dlgMsg = L"The destination file cannot be overwritten.";
					MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
					CELOG_RETURN_VAL(E_ACCESSDENIED);
				}

				// If destination file already exists, need to confirm with user and then delete it first.
				std::wstring desNxlFilePath = desPath + L'\\' + strRelativePath + L".nxl";

				if (GetFileAttributesW(desNxlFilePath.c_str()) != INVALID_FILE_ATTRIBUTES)
				{
					const std::wstring dlgQuestion = desNxlFilePath + L" already exists.  Do you want to overwrite it?";
					int ret = MessageBoxW(NULL, dlgQuestion.c_str(), g_dlgTitle.c_str(), MB_YESNOCANCEL | MB_ICONWARNING);

					if (ret == IDCANCEL)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_CANCELLED));
					}
					else if (ret == IDNO)
					{
						bChanged = true;
						continue;
					}
					else
					{
						if (!DeleteFile(desNxlFilePath.c_str()))
						{
							CELOG_RETURN_VAL(E_ACCESSDENIED);
						}
					}
				}

				// Create the destination directory if it doesn't exist.
				desNxlFilePath.erase(desNxlFilePath.rfind(L'\\'));

				if (GetFileAttributesW(desNxlFilePath.c_str()) == INVALID_FILE_ATTRIBUTES 
					&& (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
				{
					// Fix bug 69830, handle long path issue.
					std::wstring dosfullpath = NX::fs::dos_fullfilepath(desNxlFilePath, false).global_dos_path();
					int ret = ::CreateDirectoryW(dosfullpath.c_str(), NULL);
					if (ret == 0) {
						// Maybe one or more intermediate dir don't exist, we use SHCreateDirectory to create it again, but it won't support long path.
						ret = SHCreateDirectory(NULL, desNxlFilePath.c_str());
						if (ret != ERROR_SUCCESS) {
							SDWLibDeleteRmcInstance(pLoggedInInstance);

							const std::wstring dlgMsg = L"Failed to move and protect file in destination folder.";
							MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
							CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ret));
						}
					}
				}

				// Generate the destination file by protecting the source file
				const std::vector<SDRmFileRight> rights;
				const SDR_WATERMARK_INFO watermarkInfo;
				const SDR_Expiration expire;
				const std::string fileTagsUtf8 = NX::conversion::utf16_to_utf8(desFolderFileTags);
				res = pLoggedInUser->ProtectFile(ci->wstrPath, desNxlFilePath, rights, watermarkInfo, expire, fileTagsUtf8, pLoggedInUser->GetMembershipID(0), false);
				if (!res)
				{
					SDWLibDeleteRmcInstance(pLoggedInInstance);
					const std::wstring dlgMsg = L"Failed to move and protect file in destination folder.";
					MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
				}

				// Delete the source file.
				BOOL bRet = DeleteFile(NX::fs::dos_fullfilepath(ci->wstrPath).global_dos_path().c_str());

				bChanged = true;
			}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			else if ((ci->rpmRelation.file == NonNXLFileInSanctuaryDir || (ci->rpmRelation.file == NXLFileInSanctuaryDir && !bContentInNxlFormat)) &&
				!(desFolderRpmRelation.bSanctuaryFolder || desFolderRpmRelation.bSanctuaryInheritedFolder))
			{
				// Moving non-NXL file from Sanctuary dir to non-Sanctuary dir 
				if (pLoggedInInstance == NULL)
				{
					std::string passcode(PASSCODE);
					res = RPMGetCurrentLoggedInUser(passcode, pLoggedInInstance, pLoggedInTenant, pLoggedInUser);
					if (!res)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
					}
				}

				// If destination file already exists, need to confirm with user and then delete it first. 
				std::wstring desNxlFilePath = desPath + fileName + L".nxl";

				if (GetFileAttributesW(desNxlFilePath.c_str()) != INVALID_FILE_ATTRIBUTES)
				{
					const std::wstring dlgQuestion = desNxlFilePath + L" already exists.  Do you want to overwrite it?";
					int ret = MessageBoxW(NULL, dlgQuestion.c_str(), g_dlgTitle.c_str(), MB_YESNOCANCEL | MB_ICONWARNING);

					if (ret == IDCANCEL)
					{
						CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_CANCELLED));
					}
					else if (ret == IDNO)
					{
						bChanged = true;
						continue;
					}
					else
					{
						if (!DeleteFile(desNxlFilePath.c_str()))
						{
							CELOG_RETURN_VAL(E_ACCESSDENIED);
						}
					}
				}

				// Generate the destination file by protecting the source file 
				const std::vector<SDRmFileRight> rights;
				const SDR_WATERMARK_INFO watermarkInfo;
				const SDR_Expiration expire;
				desNxlFilePath = desPath;
				const std::string fileTagsUtf8 = NX::conversion::utf16_to_utf8(ci->fileTags);
				res = pLoggedInUser->ProtectFile(ci->wstrPath, desNxlFilePath, rights, watermarkInfo, expire, fileTagsUtf8, pLoggedInUser->GetMembershipID(0), false);
				if (!res)
				{
					SDWLibDeleteRmcInstance(pLoggedInInstance);
					const std::wstring dlgMsg = L"Failed to move and protect file in destination folder.";
					MessageBoxW(NULL, dlgMsg.c_str(), g_dlgTitle.c_str(), MB_OK | MB_ICONERROR);
					CELOG_RETURN_VAL(HRESULT_FROM_WIN32(res.GetCode()));
				}

				// Delete the source file. 
				BOOL bRet = DeleteFile(NX::fs::dos_fullfilepath(ci->wstrPath).global_dos_path().c_str());

				bChanged = true;
			}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			else
			{
				vecFileOp.push_back(std::make_pair(ci->wstrPath, desPath + L'\\' + strRelativePath));
			}
		}
	}

	if (pLoggedInInstance != NULL)
	{
		SDWLibDeleteRmcInstance(pLoggedInInstance);
	}

	for (std::vector<std::pair<std::wstring, std::wstring>>::const_iterator ci = deniedVecFileOp.begin(); ci != deniedVecFileOp.end(); ++ci)
	{
		std::vector<std::pair<std::wstring, std::wstring>> tempVecFileOp;
		tempVecFileOp.push_back(*ci);

		hook_control.setUserDefinedData((PVOID)&(ci->first));
		DoFileOperation(tempVecFileOp, false, false);
		hook_control.setUserDefinedData(NULL);

		SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, GetFilePathFromName(ci->first).c_str(), 0);
	}

	if (vecPathsInfo.size() == vecFileOp.size() && !bChanged)
	{
		CELOG_RETURN_VAL(pnext_MoveItems(This, punkItems, psiDestinationFolder));
	}

	BOOL retFP = DoFileOperation(vecFileOp, false, false);

	if (!vecFileOp.empty())
	{
		SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, GetFilePathFromName(vecFileOp[0].first).c_str(), 0);
	}

	if (bChanged)
	{
		// If source is a directory and it still exists, recursively delete it.
		vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, false, false, false);
		if (!vecPathsInfo.empty() && vecPathsInfo[0].isFolder && retFP == TRUE)
		{
			const std::vector<std::wstring>vecFileOp2 = {vecPathsInfo[0].wstrPath};
			DoFileDelete(vecFileOp2, false, true);
		}
	}

	CELOG_RETURN_VAL(S_OK);
}

PF_COMDeleteItems pnext_DeleteItems = NULL;
HRESULT STDMETHODCALLTYPE Hooked_COMDeleteItems(IFileOperation* This, IUnknown *punkItems)
{
	CELOG_ENTER;

	if (hook_control.is_disabled())
	{
		CELOG_RETURN_VAL(pnext_DeleteItems(This, punkItems));
	}

	recursion_control_auto auto_disable(hook_control);

	if (g_otherProcess)
	{
		std::wstring strSrcRoot;
		std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, false, false, false);

		for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
		{
			if (!ci->isFolder)
			{
				std::wstring fileWithNxlPath = ci->wstrPath + L".nxl";

				WIN32_FIND_DATAW findFileData = { 0 };
				HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
				if (hFind != INVALID_HANDLE_VALUE)
				{
					FindClose(hFind);

					if (!boost::algorithm::iends_with(findFileData.cFileName, L".nxl"))
					{
						CELOG_RETURN_VAL(S_OK);
					}
				}
			}
		}
	}

	bool bPermanentDelete = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	std::wstring strSrcRoot;
	std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(punkItems, strSrcRoot, false, false, true);

	if (vecPathsInfo.empty())
	{
		CELOG_RETURN_VAL(pnext_DeleteItems(This, punkItems));
	}

	bool bChanged = false;
	std::vector<std::wstring> vecFileOp;

	for (std::vector<PATHINFO>::iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
	{
		CELOG_LOG(CELOG_DEBUG, L"ci = {\"%s\", %s, %s}\n",
			ci->wstrPath.c_str(),
			ci->isFolder ? L"true" : L"false",
			(ci->isFolder ? ci->rpmRelation.folder.ToString() : NX::conversion::to_wstring(ci->rpmRelation.file)).c_str());

#ifdef _DEBUG 
		AssertRPMRelation(ci->isFolder, ci->rpmRelation);
#endif 
		if (ci->isFolder ? (ci->rpmRelation.folder.bUnknownRelation) : (ci->rpmRelation.file == UnknownRelation))
		{
			// Error occurred while trying to determine the relationship. 
			CELOG_RETURN_VAL(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
		}

		SDWLResult res;

		if (!ci->isFolder && ci->rpmRelation.file == NXLFileInRPMDir)
		{
			vecFileOp.push_back(ci->wstrPath + L".nxl");
			bChanged = true;
		}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
		else if (!ci->isFolder &&
			(ci->rpmRelation.file == NonNXLFileInSanctuaryDir || ci->rpmRelation.file == NXLFileInSanctuaryDir) &&
			!bPermanentDelete)
		{
			// Change the delete to permanent-delete. 
			vecFileOp.push_back(ci->wstrPath);
			bPermanentDelete = true;
			bChanged = true;
		}
		else if (ci->isFolder &&
			(ci->rpmRelation.folder.bSanctuaryInheritedFolder &&
				!ci->rpmRelation.folder.bSanctuaryAncestralFolder &&
				!ci->rpmRelation.folder.bSanctuaryFolder) &&
			!bPermanentDelete)
		{
			// Change the delete to permanent-delete. 
			vecFileOp.push_back(ci->wstrPath);
			bPermanentDelete = true;
			bChanged = true;
		}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
		else
		{
			vecFileOp.push_back(ci->wstrPath);
		}
	}

	if (bChanged)
	{
		DoFileDelete(vecFileOp, !bPermanentDelete, false);
		SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, GetFilePathFromName(vecFileOp[0]).c_str(), 0);

		CELOG_RETURN_VAL(S_OK);
	}
	else
	{
		CELOG_RETURN_VAL(pnext_DeleteItems(This, punkItems));
	}
}

PF_COMRenameItem pnext_RenameItem = NULL;
HRESULT STDMETHODCALLTYPE Hooked_COMRenameItem(IFileOperation * This, IShellItem *psiItem, LPCWSTR pszNewName, IFileOperationProgressSink *pfopsItem)
{
	if (hook_control.is_disabled())
	{
		return pnext_RenameItem(This, psiItem, pszNewName, pfopsItem);
	}

	recursion_control_auto auto_disable(hook_control);

	std::wstring strSrcRoot;
	std::vector<PATHINFO> vecPathsInfo = GetFilePathsFromObject(psiItem, strSrcRoot, false, false, false);

	for (std::vector<PATHINFO>::const_iterator ci = vecPathsInfo.begin(); ci != vecPathsInfo.end(); ci++)
	{
		if (!ci->isFolder)
		{
			std::wstring fileWithNxlPath = ci->wstrPath + L".nxl";

			WIN32_FIND_DATAW findFileData = { 0 };
			HANDLE hFind = FindFirstFileW(fileWithNxlPath.c_str(), &findFileData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				FindClose(hFind);

				if (!boost::algorithm::iends_with(findFileData.cFileName, L".nxl"))
				{
					return S_OK;
				}
			}
		}
		else if (g_isAdobe)
		{
			BOOL bNeedBlock = FALSE;

			HKEY hKey = NULL;
			if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\NextLabs\\SkyDRM", 0, KEY_READ | KEY_WOW64_64KEY, &hKey))
			{
				DWORD cbData = 0;

				if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, NULL, &cbData) && 0 != cbData)
				{
					LPBYTE lpData = new BYTE[cbData];
					if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"securedfolder", NULL, NULL, lpData, &cbData))
					{
						std::wstring wstrPath = ci->wstrPath + L"\\";
						std::transform(wstrPath.begin(), wstrPath.end(), wstrPath.begin(), tolower);

						std::vector<std::wstring> markDir;
						parse_dirs(markDir, (const WCHAR*)lpData);

						for (std::wstring& dir : markDir)
						{
							if (dir.size() == wstrPath.size())
							{
								if (boost::algorithm::iequals(wstrPath, dir))
								{
									bNeedBlock = TRUE;
									break;
								}
							}
							else if (dir.size() > wstrPath.size())
							{
								if (boost::algorithm::istarts_with(dir, wstrPath))
								{
									bNeedBlock = TRUE;
									break;
								}
							}
						}
					}

					delete[]lpData;
				}

				RegCloseKey(hKey);
			}

			if (bNeedBlock)
			{
				return S_OK;
			}
		}
	}

	return pnext_RenameItem(This, psiItem, pszNewName, pfopsItem);
}

std::vector<std::wstring> query_selected_file2(IDataObject *pdtobj)
{
	std::vector<std::wstring> files;
	FORMATETC   FmtEtc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM   Stg = { 0 };
	HDROP       hDrop = NULL;

	memset(&Stg, 0, sizeof(Stg));
	Stg.tymed = CF_HDROP;

	// Find CF_HDROP data in pDataObj
	if (FAILED(pdtobj->GetData(&FmtEtc, &Stg))) {
		return files;
	}

	// Get the pointer pointing to real data
	hDrop = (HDROP)GlobalLock(Stg.hGlobal);
	if (NULL == hDrop) {
		ReleaseStgMedium(&Stg);
		return files;
	}

	// How many files are selected?
	const int nFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
	if (0 == nFiles) {
		return files;
	}

	for (int i = 0; i < nFiles; i++) {
		wchar_t s[MAX_PATH];
		if (0 != DragQueryFileW(hDrop, i, s, MAX_PATH)) {
			//push all files in MenuFilter will be checked later
			files.push_back(s);
		}
	}

	GlobalUnlock(Stg.hGlobal);
	ReleaseStgMedium(&Stg);

	return files;
}

static bool isDir2(const std::wstring& path)
{
	DWORD dwAttrs = GetFileAttributesW(NX::fs::dos_fullfilepath(path).global_dos_path().c_str());
	if (dwAttrs != INVALID_FILE_ATTRIBUTES) {
		if (FILE_ATTRIBUTE_DIRECTORY & dwAttrs) {
			return true;
		}
	}

	return false;
}

PF_COMInitialize pnext_Initialize = NULL;
HRESULT STDMETHODCALLTYPE Hooked_Initialize(IShellExtInit * This, PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID) {
	CELOG_ENTER;
	if (hook_control.is_disabled())
	{
		return pnext_Initialize(This, pidlFolder, pdtobj, hkeyProgID);
	}
	recursion_control_auto auto_disable(hook_control);
	if (!g_otherProcess) {
		unsigned int dirStatus = 0;
		bool fileStatus = false;
		bool isNeedDeny = false;
		std::vector<std::wstring> vecSelectedFiles = query_selected_file2(pdtobj);
		if (vecSelectedFiles.size() == 0) {
			return pnext_Initialize(This, pidlFolder, pdtobj, hkeyProgID);
		}
		for (std::wstring path : vecSelectedFiles)
		{
			if (isDir2(path)) {
#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
				uint32_t sanc_dirstatus;
				std::wstring sanc_filetags;
				std::wstring sanc_directory = path;
				if (pInstance->IsSanctuaryFolder(sanc_directory, &sanc_dirstatus, sanc_filetags) != 0) {
					continue;
			    }
				if ((sanc_dirstatus) & (RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR | RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR)) {
					isNeedDeny = true;
					break;
				}
				else {
					continue;
				}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
				continue;
			}
			auto res = pInstance->RPMGetFileStatus(path, &dirStatus, &fileStatus);
			if (res.GetCode() == 0) 
			{
				if (dirStatus & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)) {
					if (fileStatus) {
						isNeedDeny = true;
						break;
					}
					else {
						continue;
					}
				}
				else {
					if (fileStatus) {
						isNeedDeny = true;
						break;
					}
					else {
						continue;
					}
				}
			}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			uint32_t sanc_dirstatus;
			std::wstring sanc_filetags;
			std::wstring sanc_wstrPath = path;
			std::wstring sanc_directory = L"";
			sanc_directory = sanc_wstrPath.substr(0, sanc_wstrPath.find_last_of(L"\\/"));
			if (pInstance->IsSanctuaryFolder(sanc_directory, &sanc_dirstatus, sanc_filetags) != 0) {
				continue;
			}
			if ((sanc_dirstatus) & (RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR | RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR)) {
				isNeedDeny = true;
				break;
			}
			else {
				continue;
			}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		}

		if (isNeedDeny) {
			return REGDB_E_CLASSNOTREG;
		}
	}
	return pnext_Initialize(This, pidlFolder, pdtobj, hkeyProgID);
}

typedef HRESULT(WINAPI *PF_CoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID  *ppv);
PF_CoCreateInstance		Hooked_CoCreateInstance_Next = NULL;

bool g_bHookFileOperation = false;
CRITICAL_SECTION g_csHookFileOperation = { 0 };

CRITICAL_SECTION g_csHookAcrobatContextMenu = { 0 };

HRESULT WINAPI Hooked_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	HRESULT hr = Hooked_CoCreateInstance_Next(rclsid, pUnkOuter, dwClsContext, riid, ppv);

	if (SUCCEEDED(hr) && (*ppv) != NULL && ::IsEqualCLSID(rclsid, CLSID_FileOperation) && ::IsEqualIID(riid, IID_IFileOperation))
	{
		if (!g_bHookFileOperation)
		{
			EnterCriticalSection(&g_csHookFileOperation);
			if (!g_bHookFileOperation)
			{
				const IFileOperation_C* pObject = static_cast<IFileOperation_C*>(*ppv);

				LPVOID pCopyItem = pObject->lpVtbl->CopyItem;
				HookCode((LPVOID)pCopyItem, (PVOID)Hooked_COMCopyItem, (LPVOID*)&pnext_CopyItem);

				LPVOID pCopyItems = pObject->lpVtbl->CopyItems;
				HookCode((LPVOID)pCopyItems, (PVOID)Hooked_COMCopyItems, (LPVOID*)&pnext_CopyItems);

				LPVOID pMoveItem = pObject->lpVtbl->MoveItem;
				HookCode((LPVOID)pMoveItem, (PVOID)Hooked_COMMoveItem, (LPVOID*)&pnext_MoveItem);

				LPVOID pMoveItems = pObject->lpVtbl->MoveItems;
				HookCode((LPVOID)pMoveItems, (PVOID)Hooked_COMMoveItems, (LPVOID*)&pnext_MoveItems);

				LPVOID pDeleteItems = pObject->lpVtbl->DeleteItems;
				HookCode((LPVOID)pDeleteItems, (PVOID)Hooked_COMDeleteItems, (LPVOID*)&pnext_DeleteItems);

				g_bHookFileOperation = true;
			}
			LeaveCriticalSection(&g_csHookFileOperation);
		}
	}

	CLSID clsidFilter;
	wchar_t* clsid_str = L"{A6595CD1-BF77-430A-A452-18696685F7C7}";
	CLSIDFromString(clsid_str, &clsidFilter);
	if (IsEqualCLSID(rclsid, clsidFilter))
	{
		HRESULT hr2 = S_OK;
		//::OutputDebugStringW(L"Have found class id {A6595CD1-BF77-430A-A452-18696685F7C7}, means to initial Adobe.Acrobat.ContextMenu");
		//LPVOID* shellExtInit;
		ATL::CComPtr<IShellExtInit> shellExtInit = NULL;
		hr2 = ((IUnknown*)(*ppv))->QueryInterface(IID_IShellExtInit, (void**)&shellExtInit);
		if (SUCCEEDED(hr2) && shellExtInit != NULL) {
			//::OutputDebugStringW(L"as Adobe.Acrobat.ContextMenu witch found also is inherent IShellExtInit");
				EnterCriticalSection(&g_csHookAcrobatContextMenu);
				const IShellExtInit_C* shellExtInit_c = reinterpret_cast<IShellExtInit_C*>(shellExtInit.p);
				LPVOID pInitialize = shellExtInit_c->lpVtbl->Initialize;
				HookCode((LPVOID)pInitialize, (PVOID)Hooked_Initialize, (LPVOID*)&pnext_Initialize);
				LeaveCriticalSection(&g_csHookAcrobatContextMenu);
		}
	}
	return hr;
}

HRESULT STDAPICALLTYPE Hooked_SHCreateItemFromParsingName(PCWSTR   pszPath, IBindCtx *pbc, REFIID   riid, void     **ppv)
{
	if (hook_control.is_disabled())
	{
		if (pszPath != NULL)
		{
			if (boost::algorithm::iends_with(pszPath, L".nxl") && NXLFileInRPMDir == GetFileRelation(pszPath, TRUE))
			{
				std::wstring wstrPath = pszPath;
				wstrPath.pop_back();
				wstrPath.pop_back();
				wstrPath.pop_back();
				wstrPath.pop_back();

				WIN32_FIND_DATAW fd = { 0 };
				HANDLE h = FindFirstFileW(wstrPath.c_str(), &fd);

				if (h != INVALID_HANDLE_VALUE)
				{
					FindClose(h);
					return CreateSimpleShellItemFromPath(&fd, pszPath, riid, ppv);
				}
			}
		}
	}

	return Hooked_SHCreateItemFromParsingName_Next(pszPath, pbc, riid, ppv);
}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
typedef ULONG(WINAPI* PF_MAPISendMailW)(LHANDLE lhSession, ULONG_PTR ulUIParam, lpMapiMessage lpMessage,
	FLAGS flFlags, ULONG ulReserved);
PF_MAPISendMailW Hooked_MAPISendMailW_Next = NULL;

ULONG WINAPI Hooked_MAPISendMailW(LHANDLE lhSession, ULONG_PTR ulUIParam, lpMapiMessage lpMessage,
	FLAGS flFlags, ULONG ulReserved)
{
	std::wstring appName(L"explorer.exe");
	std::wstring msg(L"Share with email is not supported.");
	pInstance->RPMNotifyMessage(appName, L"", msg, 1);
	return MAPI_E_FAILURE;
}
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 


typedef HANDLE(WINAPI* PF_CreateFileW)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
PF_CreateFileW Hooked_CreateFileW_Next = NULL;

HANDLE WINAPI Hooked_CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	if (hook_control.is_disabled())
	{
		if (lpFileName != NULL)
		{
			const std::wstring* pDeniedFolder = (const std::wstring*)hook_control.getUserDefinedData();
			if (pDeniedFolder != NULL)
			{
				if (boost::algorithm::iequals(*pDeniedFolder, lpFileName))
				{
					SetLastError(ERROR_ACCESS_DENIED);
					return INVALID_HANDLE_VALUE;
				}
			}
		}
	}

	return Hooked_CreateFileW_Next(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}



// The code below is to handle deleting files / folders by dragging to Recycle 
// Bin. 
// 
// Below is what we learned by analyzing the disassembly of Explorer code that 
// handles dragging files / folders to Recycle Bin: 
// 
// 1. Dragging to Recycle Bin is handled by TransferDelete() in shell32.dll on 
//    all Windows version / platform combinations. 
// 
// 2. - On Win7 32-bit, TransferDelete() invokes SHFileOperationW() which 
//      invokes SHFileOperationWithAdditionalFlags(). 
//    - On Win7 64-bit and all other Windows versions, TransferDelete() 
//      invokes SHFileOperationWithAdditionalFlags() directly. 
// 
// 3. SHFileOperationWithAdditionalFlags() is an undocumented function.  It 
//    has the following properties: 
// 
//    Version              Exported        Calling conv.   Params  Module 
//    -------              --------        -------------   ------  ------ 
//    Win 7                No              __stdcall       2       shell32 
//    Win 2008 R2          No              __stdcall       2       shell32 
//    Win 8                No              __stdcall       2       shell32 
//    Win 2012             No              __stdcall       2       shell32 
//    Win 8.1              No              __fastcall      3       shell32 
//    Win 2012 R2          No              __fastcall      3       shell32 
//    Win 10 base          Yes             __stdcall       3       windows.storage 
//    Win 10 Nov 2019 Upd. Yes             __stdcall       3       windows.storage 
//    Win 2016             Yes             __stdcall       3       windows.storage 
// 
// 4. SHFileOperationW() is a public function in all Windows versions.  In the 
//    assembly language level, it invokes SHFileOperationWithAdditionalFlags() 
//    using the following machine instructions: 
// 
//    Version / platform   Opcode  Instruction 
//    ------------------   ------  ----------- 
//    Win 7 x86            E8      CALL    32-bit relative 
//    Win 7 x64            E9      JMP     32-bit relative 
//    Win 2008 R2          E9      JMP     32-bit relative 
//    Win 8 x86            E8      CALL    32-bit relative 
//    Win 8 x64            E9      JMP     32-bit relative 
//    Win 2012             EB      JMP     8-bit relative 
//    Win 8.1 x86          E8      CALL    32-bit relative 
//    Win 8.1 x64          E9      JMP     32-bit relative 
//    Win 2012 R2          E9      JMP     32-bit relative 
// 
// Our strategy is to hook the undocumented function 
// SHFileOperationWithAdditionalFlags() for all Windows version / platform 
// combinations. 
// 
// - On Win 7 to Win 8.1, and Win 2008 R2 to Win 2012 R2, we scan the machine 
//   code of the public function SHFileOperationW() to find the address of the 
//   internal function SHFileOperationWithAdditionalFlags().  Then we hook it 
//   using its address. 
// - On Win 10 and Win 2016, we hook SHFileOperationWithAdditionalFlags() 
//   directly using its name since it is an exported function.
// 
// NOTE: On Win 7 32-bit, it would be cleaner to hook the public 
// SHFileOperationW().  However, in order to keep the overall code simpler, we 
// still hook the undocumented function SHFileOperationWithAdditionalFlags() 
// instead. 

typedef int(__stdcall  *PF_SHFileOperationWithAdditionalFlags_2param)          (_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1);
typedef int(__stdcall  *PF_SHFileOperationWithAdditionalFlags_3param)          (_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1, DWORD param2);
typedef int(__fastcall *PF_SHFileOperationWithAdditionalFlags_3paramFastCall)  (_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1, DWORD param2);
PF_SHFileOperationWithAdditionalFlags_2param            Hooked_SHFileOperationWithAdditionalFlags_2param_Next = NULL;
PF_SHFileOperationWithAdditionalFlags_3param            Hooked_SHFileOperationWithAdditionalFlags_3param_Next = NULL;
PF_SHFileOperationWithAdditionalFlags_3paramFastCall    Hooked_SHFileOperationWithAdditionalFlags_3paramFastCall_Next = NULL;

// Scan the machine code of the public function SHFileOperationW() to find the 
// address of the internal function SHFileOperationWithAdditionalFlags(). 
PVOID FindSHFileOperationWithAdditionalFlagsAddr(void)
{
	PBYTE p = (PBYTE)&SHFileOperationW;

#ifdef _WIN64 

	std::vector<BYTE> codeToMatchWithJmpShortRel, codeToMatchWithJmpNearRel;

	if (g_winBuildNum >= NX::win::WBN_WIN10)
	{
		// Win 10 base / Win 2016 or above.  Not supported. 
		return NULL;
	}
	else if (g_winBuildNum >= NX::win::WBN_WIN81)
	{
		// Win 8.1 / Win 2012 R2 
		// 
		// SHFileOperationWithAdditionalFlags() is a __fastcall function with three parameters. 
		// 
		// SHFileOperationW() starts with either of these instructions: 
		// 45 33 C0             XOR         R8D,R8D   
		// 33 D2                XOR         EDX,EDX   
		// E9 xx xx xx xx       JMP         SHFileOperationWithAdditionalFlags (signed 32-bit relative offset) 
		// --- OR --- 
		// 45 33 C0             XOR         R8D,R8D   
		// 33 D2                XOR         EDX,EDX   
		// EB xx                JMP         SHFileOperationWithAdditionalFlags (signed 8-bit relative offset) 
		codeToMatchWithJmpNearRel = { 0x45, 0x33, 0xC0, 0x33, 0xD2, 0xE9 };
		codeToMatchWithJmpShortRel = { 0x45, 0x33, 0xC0, 0x33, 0xD2, 0xEB };
	}
	else if (g_winBuildNum >= NX::win::WBN_WIN7)
	{
		// Win 8 / Win 2012 / Win 7 / Win 2008 R2 
		// 
		// SHFileOperationWithAdditionalFlags() is a __stdcall function with two parameters. 
		// 
		// SHFileOperationW() starts with either of these instructions: 
		// 33 D2                XOR         EDX,EDX 
		// E9 xx xx xx xx       JMP         SHFileOperationWithAdditionalFlags (signed 32-bit relative offset) 
		// --- OR --- 
		// 33 D2                XOR         EDX,EDX 
		// EB xx                JMP         SHFileOperationWithAdditionalFlags (signed 8-bit relative offset) 
		codeToMatchWithJmpNearRel = { 0x33, 0xD2, 0xE9 };
		codeToMatchWithJmpShortRel = { 0x33, 0xD2, 0xEB };
	}
	else
	{
		// Win Vista / Win 2008 or below.  Not supported. 
		return NULL;
	}

	if (memcmp(p, codeToMatchWithJmpNearRel.data(), codeToMatchWithJmpNearRel.size()) == 0)
	{
		return (p + codeToMatchWithJmpNearRel.size() + sizeof(INT32)) + *(INT32*)(p + codeToMatchWithJmpNearRel.size());
	}
	else if (memcmp(p, codeToMatchWithJmpShortRel.data(), codeToMatchWithJmpShortRel.size()) == 0)
	{
		return (p + codeToMatchWithJmpShortRel.size() + sizeof(INT8)) + *(INT8*)(p + codeToMatchWithJmpShortRel.size());
	}

#else // !__WIN64 

	std::vector<BYTE> codeToMatchWithCallNearRel;

	if (g_winBuildNum >= NX::win::WBN_WIN10)
	{
		// Win 10 base or above.  Not supported. 
		return NULL;
	}
	else if (g_winBuildNum >= NX::win::WBN_WIN81)
	{
		// Win 8.1 
		// 
		// SHFileOperationWithAdditionalFlags() is a __fastcall function with three parameters. 
		// 
		// SHFileOperationW() starts with these instructions: 
		// 8B FF                MOV         EDI,EDI 
		// 55                   PUSH        EBP 
		// 8B EC                MOV         EBP,ESP 
		// 8B 4D 08             MOV         ECX,DWORD PTR [EBP+8] 
		// 33 D2                XOR         EDX,EDX 
		// 6A 00                PUSH        0 
		// E8 xx xx xx xx       CALL        SHFileOperationWithAdditionalFlags (signed 32-bit relative offset) 
		codeToMatchWithCallNearRel = { 0x8B, 0xFF, 0x55, 0x8B, 0xEC, 0x8B, 0x4D, 0x08, 0x33, 0xD2, 0x6A, 0x00, 0xE8 };
	}
	else if (g_winBuildNum >= NX::win::WBN_WIN7)
	{
		// Win 8 / Win 7 
		// 
		// SHFileOperationWithAdditionalFlags() is a __stdcall function with two parameters. 
		// 
		// SHFileOperationW() starts with these instructions: 
		// 8B FF                MOV         EDI,EDI   
		// 55                   PUSH        EBP   
		// 8B EC                MOV         EBP,ESP   
		// 6A 00                PUSH        0   
		// FF 75 08             PUSH        DWORD PTR [EBP+8]   
		// E8 xx xx xx xx       CALL        SHFileOperationWithAdditionalFlags (signed 32-bit relative offset) 
		codeToMatchWithCallNearRel = { 0x8B, 0xFF, 0x55, 0x8B, 0xEC, 0x6A, 0x00, 0xFF, 0x75, 0x08, 0xE8 };
	}
	else
	{
		// Win Vista or below.  Not supported. 
		return NULL;
	}

	if (memcmp(p, codeToMatchWithCallNearRel.data(), codeToMatchWithCallNearRel.size()) == 0)
	{
		return (p + codeToMatchWithCallNearRel.size() + sizeof(INT32)) + *(INT32*)(p + codeToMatchWithCallNearRel.size());
	}

#endif // _WIN64 

	// Cannot find the expected machine code.  Return error. 
	return NULL;
}

int Call_HookedSHFOWAFNext_Common(_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1, DWORD param2)
{
	if (g_winBuildNum >= NX::win::WBN_WIN10)
	{
		return Hooked_SHFileOperationWithAdditionalFlags_3param_Next(lpFileOp, param1, param2);
	}
	else if (g_winBuildNum >= NX::win::WBN_WIN81)
	{
		return Hooked_SHFileOperationWithAdditionalFlags_3paramFastCall_Next(lpFileOp, param1, param2);
	}
	else if (g_winBuildNum >= NX::win::WBN_WIN7)
	{
		return Hooked_SHFileOperationWithAdditionalFlags_2param_Next(lpFileOp, param1);
	}
	else
	{
		// Not supported. 
		assert(false);
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
}

int Hooked_SHFileOperationWithAdditionalFlags_Common(_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1, DWORD param2 = 0)
{
	CELOG_ENTER;

	if (hook_control.is_disabled())
	{
		CELOG_RETURN_VAL(Call_HookedSHFOWAFNext_Common(lpFileOp, param1, param2));
	}

	recursion_control_auto auto_disable(hook_control);

	if ((lpFileOp->wFunc == FO_DELETE) && (lpFileOp->fFlags & FOF_ALLOWUNDO))
	{
		SDWLResult res;

		bool bPermanentDelete = false;
		bool bChanged = false;
		std::vector<std::wstring> vecFileOp;

		for (PCZZWSTR fromPath = lpFileOp->pFrom; fromPath[0] != L'\0'; fromPath += wcslen(fromPath) + 1)
		{
			if (PathIsDirectoryW(fromPath))
			{
				RPMFolderRelation folderRelation = GetFolderRelation(fromPath);
#ifdef _DEBUG 
				AssertRPMFolderRelation(folderRelation);
#endif 
				if (folderRelation.bUnknownRelation)
				{
					// Error occurred while trying to determine the relationship. 
					CELOG_RETURN_VAL(ERROR_ACCESS_DENIED);
				}

				if (false)
				{
					// 
					// This slot is for adding support for RPM dirs in the future. 
					// 
				}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
				else if (folderRelation.bSanctuaryInheritedFolder &&
					!folderRelation.bSanctuaryAncestralFolder &&
					!folderRelation.bSanctuaryFolder)
				{
					// Change the delete to permanent-delete. 
					bPermanentDelete = true;
					bChanged = true;
				}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

				vecFileOp.push_back(fromPath);
			}
			else
			{
				RPMFileRelation fileRelation = GetFileRelation(fromPath, FALSE);

				if (fileRelation == UnknownRelation)
				{
					// Error occurred while trying to determine the relationship. 
					CELOG_RETURN_VAL(ERROR_ACCESS_DENIED);
				}

				if (false)
				{
					// 
					// This slot is for adding support for files in RPM dirs in the future. 
					// 
				}
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
				else if (fileRelation == NonNXLFileInSanctuaryDir || fileRelation == NXLFileInSanctuaryDir)
				{
					// Change the delete to permanent-delete. 
					vecFileOp.push_back(fromPath);
					bPermanentDelete = true;
					bChanged = true;
				}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
				else
				{
					vecFileOp.push_back(fromPath);
				}
			}
		}

		if (bChanged)
		{
			const PCZZWSTR oldPFrom = lpFileOp->pFrom;
			const FILEOP_FLAGS oldFFlags = lpFileOp->fFlags;
			const std::vector<wchar_t> newPFromStr = NX::conversion::strings_to_buffer(vecFileOp);
			lpFileOp->pFrom = newPFromStr.data();
			if (bPermanentDelete)
			{
				lpFileOp->fFlags &= ~(FOF_ALLOWUNDO | FOF_NOCONFIRMATION);
			}
			else
			{
				lpFileOp->fFlags |= FOF_ALLOWUNDO;
			}
			int iRet = Call_HookedSHFOWAFNext_Common(lpFileOp, param1, param2);
			lpFileOp->pFrom = oldPFrom;
			lpFileOp->fFlags = oldFFlags;

			CELOG_RETURN_VAL(iRet);
		}
	}

	CELOG_RETURN_VAL(Call_HookedSHFOWAFNext_Common(lpFileOp, param1, param2));
}

int __stdcall Hooked_SHFileOperationWithAdditionalFlags_2param(_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1)
{
	return Hooked_SHFileOperationWithAdditionalFlags_Common(lpFileOp, param1);
}

int __stdcall Hooked_SHFileOperationWithAdditionalFlags_3param(_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1, DWORD param2)
{
	return Hooked_SHFileOperationWithAdditionalFlags_Common(lpFileOp, param1, param2);
}

int __fastcall Hooked_SHFileOperationWithAdditionalFlags_3paramFastCall(_Inout_ LPSHFILEOPSTRUCTW lpFileOp, DWORD param1, DWORD param2)
{
	return Hooked_SHFileOperationWithAdditionalFlags_Common(lpFileOp, param1, param2);
}

// DLL Entry Point 
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	g_winBuildNum = NX::win::windows_info().build_number();
	g_hInstance = hInstance;

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		SDWLibInit();

		WCHAR name[MAX_PATH] = { 0 };
		GetModuleFileNameW(0, name, MAX_PATH);
		if (boost::algorithm::iends_with(name, L"\\explorer.exe"))
		{
			InitializeCriticalSection(&g_csHookFileOperation);
			InitializeCriticalSection(&g_csHookAcrobatContextMenu);

			SDWLibCreateInstance(&pInstance);

			OSVERSIONINFOW osver = { 0 };
			osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
			GetVersionExW(&osver);
			if (osver.dwMajorVersion > 6)
			{
				g_isWin7Above = true;
			}

			InitializeMadCHook();
			HookAPI("ole32", "CoCreateInstance", (void*)Hooked_CoCreateInstance, (void**)&Hooked_CoCreateInstance_Next);
			HookAPI("kernelbase", "CreateFileW", (void*)Hooked_CreateFileW, (void**)&Hooked_CreateFileW_Next);
#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 
			HookAPI("MAPI32", "MAPISendMailW", (void*)Hooked_MAPISendMailW, (void**)&Hooked_MAPISendMailW_Next);
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR 

			if (g_isWin7Above)
			{
				HookAPI("windows.storage.dll", "SHCreateItemFromParsingName", (void*)Hooked_SHCreateItemFromParsingName, (void**)&Hooked_SHCreateItemFromParsingName_Next);
			}
			else
			{
				HookAPI("Shell32", "SHCreateItemFromParsingName", (void*)Hooked_SHCreateItemFromParsingName, (void**)&Hooked_SHCreateItemFromParsingName_Next);
			}

			if (g_winBuildNum >= NX::win::WBN_WIN10)
			{
				// Win10 base / Win 2016 or above 
				HookAPI("windows.storage.dll", "SHFileOperationWithAdditionalFlags",
					(void*)Hooked_SHFileOperationWithAdditionalFlags_3param,
					(void**)&Hooked_SHFileOperationWithAdditionalFlags_3param_Next);
			}
			else
			{
				PVOID shfowafAddr = FindSHFileOperationWithAdditionalFlagsAddr();

				if (shfowafAddr != NULL)
				{
					if (g_winBuildNum >= NX::win::WBN_WIN81)
					{
						// Win 8.1 / Win 2012 R2 
						HookCode(shfowafAddr,
							(void*)Hooked_SHFileOperationWithAdditionalFlags_3paramFastCall,
							(void**)&Hooked_SHFileOperationWithAdditionalFlags_3paramFastCall_Next);
					}
					else if (g_winBuildNum >= NX::win::WBN_WIN7)
					{
						// Win 8 / Win 2012 / Win 7 / Win 2008 R2 
						HookCode(shfowafAddr,
							(void*)Hooked_SHFileOperationWithAdditionalFlags_2param,
							(void**)&Hooked_SHFileOperationWithAdditionalFlags_2param_Next);
					}
				}
			}

			GetModuleFileNameW(hInstance, name, MAX_PATH);
			LoadLibraryW(name);
		}
		else if (boost::algorithm::iends_with(name, L"\\dllhost.exe"))
		{
			SDWLibCreateInstance(&pInstance);
			g_isDllhost = TRUE;
		}
		else
		{
			if (boost::algorithm::iends_with(name, L"\\rundll32.exe"))
			{
				break;
			}

			// Workaround fix bajaj adams issue that adams always hangs when select working directory from common dialog.
			if (boost::algorithm::iends_with(name, L"\\aview.exe") // Adams view
				|| boost::algorithm::iends_with(name, L"\\ppt.exe")) // Adams post processor issue
			{
				::OutputDebugString(L"This is aview.exe process.");
				break;
			}

			if (boost::algorithm::iends_with(name, L"\\AcroRd32.exe") || boost::algorithm::iends_with(name, L"\\acrobat.exe"))
			{
				g_isAdobe = true;
			}

			g_otherProcess = TRUE;

			IFileOperation* pv = NULL;
			HRESULT hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_IFileOperation, (void**)&pv);
			if (SUCCEEDED(hr))
			{
				InitializeCriticalSection(&g_csHookFileOperation);

				SDWLibCreateInstance(&pInstance);

				const IFileOperation_C* pv_c = reinterpret_cast<IFileOperation_C*>(pv);

				LPVOID pCopyItem = pv_c->lpVtbl->CopyItem;
				HookCode((LPVOID)pCopyItem, (PVOID)Hooked_COMCopyItem, (LPVOID*)&pnext_CopyItem);

				LPVOID pCopyItems = pv_c->lpVtbl->CopyItems;
				HookCode((LPVOID)pCopyItems, (PVOID)Hooked_COMCopyItems, (LPVOID*)&pnext_CopyItems);

				LPVOID pMoveItem = pv_c->lpVtbl->MoveItem;
				HookCode((LPVOID)pMoveItem, (PVOID)Hooked_COMMoveItem, (LPVOID*)&pnext_MoveItem);

				LPVOID pMoveItems = pv_c->lpVtbl->MoveItems;
				HookCode((LPVOID)pMoveItems, (PVOID)Hooked_COMMoveItems, (LPVOID*)&pnext_MoveItems);

				LPVOID pDeleteItems = pv_c->lpVtbl->DeleteItems;
				HookCode((LPVOID)pDeleteItems, (PVOID)Hooked_COMDeleteItems, (LPVOID*)&pnext_DeleteItems);

				LPVOID pRenameItem = pv_c->lpVtbl->RenameItem;
				HookCode((LPVOID)pRenameItem, (PVOID)Hooked_COMRenameItem, (LPVOID*)&pnext_RenameItem);

				pv->Release();
			}

			GetModuleFileNameW(hInstance, name, MAX_PATH);
			LoadLibraryW(name);
		}
	}
	break;

	case DLL_PROCESS_DETACH:
	{
		if (pInstance != NULL)
		{
			SDWLibDeleteRmcInstance(pInstance);
		}

		SDWLibCleanup();
	}
	break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return _AtlModule.DllMain(dwReason, lpReserved);
}