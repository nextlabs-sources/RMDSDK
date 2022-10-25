/*
	History:
		03/05/2020,  
			change osrms Global to accommodate more obligation
			add nxosdlp to be loaded
*/
#include "stdafx.h"
#include <mutex>
#include <chrono>
#include <thread>
#include <Windows.h>
#include "registry_whitelist.h"
#include "registry_3rdrmx.h"

namespace hook {

	class recursion_control
	{
	public:
		recursion_control(void) :disabled_process(false), disabled_thread() {
			if (InitializeCriticalSectionAndSpinCount(&cs, 0x80004000) == FALSE) {
				InitializeCriticalSection(&cs);
			}
		}

		~recursion_control() {
			DeleteCriticalSection(&cs);
		}

		void thread_enable(void) {
			EnterCriticalSection(&cs);
			disabled_thread[GetCurrentThreadId()]--;
			LeaveCriticalSection(&cs);
		}

		void thread_disable(void) {
			DWORD tid = GetCurrentThreadId();
			EnterCriticalSection(&cs);
			if (disabled_thread.find(tid) == disabled_thread.end()) {
				disabled_thread[tid] = 0;
			}
			disabled_thread[tid]++;
			LeaveCriticalSection(&cs);
		}

		void process_enable(void) {
			EnterCriticalSection(&cs);
			disabled_process = false;
			LeaveCriticalSection(&cs);
		}

		void process_disable(void) {
			EnterCriticalSection(&cs);
			disabled_process = true;
			LeaveCriticalSection(&cs);
		}

		bool is_process_disabled(void) {
			bool result;
			EnterCriticalSection(&cs);
			result = disabled_process;
			LeaveCriticalSection(&cs);
			return result;
		}

		bool is_thread_disabled(void) {
			bool result = false;
			DWORD tid = GetCurrentThreadId();
			EnterCriticalSection(&cs);
			if (disabled_thread.find(tid) != disabled_thread.end()) {
				if (disabled_thread[tid] > 0) {
					result = true;
				}
			}
			LeaveCriticalSection(&cs);
			return result;
		}

		bool is_disabled(void) {
			bool result;
			EnterCriticalSection(&cs);
			result = is_thread_disabled() || is_process_disabled();
			LeaveCriticalSection(&cs);
			return result;
		}

	private:
		std::map<DWORD, int>  disabled_thread;   /* Thread state is disabled? */
		bool                 disabled_process;  /* Process state is disabled? */
		CRITICAL_SECTION     cs;                /* Protect state variables */
	};

	class recursion_control_auto
	{
	public:
		recursion_control_auto(recursion_control& in_ac) : ac(in_ac) {
			ac.thread_disable();
		}
		~recursion_control_auto(void) {
			ac.thread_enable();
		}
	private:
		recursion_control& ac;

	};

#include "madCHook.h"

	HMODULE(WINAPI* Hooked_LoadLibraryW_Next)(LPCWSTR path) = NULL;
	HMODULE WINAPI Hooked_LoadLibraryW(LPCWSTR path);

	// I found notepad.exe will not call LoadLibraryW,
	VOID(WINAPI* Hooked_GetStartupInfoW_Next)(LPSTARTUPINFOW lpStartupInfo) = NULL;
	VOID WINAPI Hooked_GetStartupInfoW(LPSTARTUPINFOW lpStartupInfo);


// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
	// I found cmd.exe will not call LoadLibraryW or GetStartupInfoW when the user runs the built-in commands "copy" or "move".
	HANDLE(WINAPI* Hooked_FindFirstFileExW_Next)(LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags) = NULL;
	HANDLE WINAPI Hooked_FindFirstFileExW(LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags);
#endif  // #if 0
}

namespace aux {
	class Module {
	public:
		Module(HMODULE hm = NULL) :m_hm(hm) {
			{
				std::vector<char> buf(255, 0);
				buf.resize(GetModuleFileNameA(m_hm, (char*)buf.data(), 255));
				m_path.assign(buf.begin(), buf.end());
			}
			{
				std::vector<wchar_t> wbuf(255, 0);
				wbuf.resize(GetModuleFileNameW(m_hm, (wchar_t*)wbuf.data(), 255));
				m_wpath.assign(wbuf.begin(), wbuf.end());
			}
		}
		inline const std::string GetPathA() { return m_path; }
		inline const std::wstring GetPathW() { return m_wpath; }
		inline std::string GetNameA() { return m_path.substr(m_path.find_last_of("\\/") + 1); }
		inline std::wstring GetNameW() { return m_wpath.substr(m_wpath.find_last_of(L"\\/") + 1); }

	private:
		HMODULE m_hm;
		std::string m_path;
		std::wstring m_wpath;
	};

	std::wstring query_registry_value(const std::wstring &strSubKey, const std::wstring &strValueName, HKEY hRoot = HKEY_CURRENT_USER)
	{
		HKEY hKey = NULL;
		std::wstring strValue;
		LSTATUS lStatus = RegOpenKeyExW(hRoot, strSubKey.c_str(), 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &hKey);
		if (lStatus == ERROR_SUCCESS)
		{
			DWORD dwType = REG_SZ;
			std::vector<unsigned char> buf;
			unsigned long value_size = 1;

			lStatus = RegQueryValueExW(hKey, strValueName.c_str(), NULL, &dwType, (LPBYTE)buf.data(), &value_size);
			if (ERROR_SUCCESS == lStatus)
			{
				buf.resize(value_size, 0);
				lStatus = RegQueryValueExW(hKey, strValueName.c_str(), NULL, &dwType, (LPBYTE)buf.data(), &value_size);
				if (ERROR_SUCCESS == lStatus)
				{
					strValue = (const wchar_t*)buf.data();
				}
			}
		}

		if (hKey != NULL)
		{
			RegCloseKey(hKey);
			hKey = NULL;
		}

		return std::move(strValue);
	}	
};


namespace {

	class IDllLoader {
	public:
		virtual bool match_context() = 0; // if this dll need to be loadered
		virtual bool prepare_env() = 0;   // 
		virtual bool do_load() = 0;	  // load this dll
	};
	
	class OsRmx :public IDllLoader {

		std::wstring path_osrmx;
		std::wstring folder_fips_libeay32;

		virtual bool match_context() override
		{
			return nx::cregistry_whitelist::getInstance()->is_app_in_whitelist();
		}

		virtual bool prepare_env() override
		{
			if (!get_osrmx_path(path_osrmx)) {
				DEVLOG(L"can not find path of osrmx\n");
				return false;
			}

			if (!get_fips_libear32_folder(folder_fips_libeay32)) {
				DEVLOG(L"can not find folder of fips libeay32\n");
				return false;
			}
			return true;
		}

		virtual bool do_load() override
		{
			// add new feature, before load os_rmx, set fips libeay32	
			// since osrmx must depend on libeay32, before load it, load libeay32 first.
			::SetDllDirectoryW(folder_fips_libeay32.c_str());
			std::wstring fispeay32_path = folder_fips_libeay32 + L"\\libeay32.dll";
			HMODULE hLibEay32 = hook::Hooked_LoadLibraryW_Next(fispeay32_path.c_str());
			if (NULL == hLibEay32){
				DEVLOG(L"Failed to load libeay32.dll in rmcore_hooked_loadlibrary");
				return false;
			}

//#ifndef _DEBUG
//			if (nx::verify_nextlabs_signature(path_osrmx) == false)
//				return false;
//#endif

			HMODULE hm = hook::Hooked_LoadLibraryW_Next(path_osrmx.c_str());
			if (hm == NULL) {
				DEVLOG(L"Failed to load osrmx.dll in rmcore_hooked_loadlibrary\n");
				return false;

			}
			auto fn = ::GetProcAddress(hm, "Initialize");
			if (fn == NULL) {
				DEVLOG(L"Failed to call GetProcAddress for osrmx!Initialize in rmcore_hooked_loadlibrary\n");
				::FreeLibrary(hm);
				return false;

			}
			// call to init;
			if (!fn()) {
				DEVLOG(L"Failed to call osrmx:Initialize() in rmcore_hooked_loadlibrary\n");
				::FreeLibrary(hm);
				return false;
			}
			return true;
		}

		inline bool get_osrmx_path(std::wstring& outpath) {
			/*
				for debug:
					load at: HKEY_LOCAL_MACHINE\Software\NextLabs\SkyDRM\OsRmx
						path32  for 32bit
						path64	for 64bit
				for release:
					load at: HKEY_LOCAL_MACHINE\Software\NextLabs\SkyDRM
						InstallPath
						32bit:  [InstallPath]\bin32\nxosrmx32.dll
						64bit:  [InstallPath]\bin\nxosrmx64.dll
			*/
			outpath.clear();
			static const std::wstring sub_key(L"Software\\NextLabs\\SkyDRM\\OsRmx");
#ifdef _WIN64
			std::wstring value(L"path64");
#elif  _WIN32
			std::wstring value(L"path32");
#endif
			outpath = aux::query_registry_value(sub_key, value, HKEY_LOCAL_MACHINE);
			return true;
		}

		inline bool get_fips_libear32_folder(std::wstring& folder) {
			folder.clear();
#ifdef _WIN64
			std::wstring value = L"path64";
#elif  _WIN32
			std::wstring value = L"path32";
#endif

			std::wstring strPath = aux::query_registry_value(L"Software\\NextLabs\\SkyDRM\\OSRmx", value, HKEY_LOCAL_MACHINE);
			auto pos = strPath.rfind(L"\\");
			if (std::wstring::npos == pos)
			{
				std::wstring strErrMsg = L"not find \\ and the path : " + strPath + L"\n";
				::OutputDebugStringW(strErrMsg.c_str());
				return false;
			}

			strPath = strPath.substr(0, pos);
			if (strPath.empty() || strPath.size() < 3)
			{
				std::wstring strErrMsg = L"the path is empty or size < 3, the path : " + strPath + L"\n";
				::OutputDebugStringW(strErrMsg.c_str());
				return false;
			}

			folder = strPath;
			return true;
		}

	};

	class OsDlp : public IDllLoader {
		std::wstring path_osdlp;
		virtual bool match_context() override
		{
			// ignore process list;
			static const std::vector<std::string> ignore{
				"explorer.exe","rundll32.exe","svchost.exe","RuntimeBroker.exe",
				"dllhost.exe","SettingSyncHost.exe","browser_broker.exe","svchost.exe",
				"StartMenuExperienceHost.exe","StartMenuExperienceHost.exe",
				"backgroundTaskHost.exe","SearchProtocolHost.exe"

				"dwm.exe","conhost.exe",
				// nextlabs self
				"nxrmserv.exe","nxrmtray.exe","nxrmdapp.exe"
			};
			std::string proc_name = aux::Module(NULL).GetNameA();
			for (auto& i : ignore) {
				if (i.size() != proc_name.size()) {
					continue;
				}
				if (0 == _stricmp(i.c_str(), proc_name.c_str())) {
					return false;
				}
			}

			//
			// todo how to judge cur_proc is Win_console mode, and ignore it too.
			//

			return true;
		}
		virtual bool prepare_env() override
		{
			if (!get_osdlp_path(path_osdlp)) {
				DEVLOG(L"can not find path of osrmx\n");
				return false;
			}
			return true;
		}
		virtual bool do_load() override
		{
//#ifndef _DEBUG
//			if (nx::verify_nextlabs_signature(path_osdlp) == false)
//				return false;
//#endif

			HMODULE hm = hook::Hooked_LoadLibraryW_Next(path_osdlp.c_str());
			if (hm == NULL) {
				DEVLOG(L"Failed to load osdlp.dll in rmcore_hooked_loadlibrary\n");
				return false;
			}
			return true;
		}
		inline bool get_osdlp_path(std::wstring& outpath) {
			/*
				for debug:
					load at: HKEY_LOCAL_MACHINE\Software\NextLabs\SkyDRM\OsDLP
						path32  for 32bit
						path64	for 64bit
				for release:
					load at: HKEY_LOCAL_MACHINE\Software\NextLabs\SkyDRM
						InstallPath
						32bit:  [InstallPath]\bin32\nxosrmx32.dll
						64bit:  [InstallPath]\bin\nxosrmx64.dll
			*/
			outpath.clear();
			static const std::wstring sub_key(L"Software\\NextLabs\\SkyDRM\\OsDLP");
#ifdef _WIN64
			std::wstring value(L"path64");
#elif _WIN32
			std::wstring value(L"path32");
#endif
			outpath = aux::query_registry_value(sub_key, value, HKEY_LOCAL_MACHINE);
			return true;
		}

	};

	class ThirdPartyRmx :public IDllLoader {

		std::wstring path_3rdrmx;
		std::wstring folder_fips_libeay32;

		virtual bool match_context() override
		{
			return nx::cregistry_3rdrmx::getInstance()->is_app_has_rmx();
		}

		virtual bool prepare_env() override
		{
			if (!get_3rdrmx_path(path_3rdrmx)) {
				DEVLOG(L"can not find path of 3rdrmx\n");
				return false;
			}

			if (!get_fips_libear32_folder(folder_fips_libeay32)) {
				DEVLOG(L"can not find folder of fips libeay32\n");
				return false;
			}
			return true;
		}

		virtual bool do_load() override
		{
			// add new feature, before load os_rmx, set fips libeay32	
			// since osrmx must depend on libeay32, before load it, load libeay32 first.
			::SetDllDirectoryW(folder_fips_libeay32.c_str());
			std::wstring fispeay32_path = folder_fips_libeay32 + L"\\libeay32.dll";
			HMODULE hLibEay32 = hook::Hooked_LoadLibraryW_Next(fispeay32_path.c_str());
			if (NULL == hLibEay32) {
				DEVLOG(L"Failed to load libeay32.dll in rmcore_hooked_loadlibrary");
				return false;
			}

			if (nx::verify_nextlabs_signature(path_3rdrmx) == false)
				return false;

			::SetDllDirectoryW(path_3rdrmx.substr(0, path_3rdrmx.find_last_of(L"\\/")).c_str());
			HMODULE hm = hook::Hooked_LoadLibraryW_Next(path_3rdrmx.c_str());
			if (hm == NULL) {
				DEVLOG(L"Failed to load 3rdrmx.dll in rmcore_hooked_loadlibrary\n");
				return false;

			}
			auto fn = ::GetProcAddress(hm, "Initialize");
			if (fn == NULL) {
				DEVLOG(L"Failed to call GetProcAddress for 3rdrmx!Initialize in rmcore_hooked_loadlibrary\n");
				::FreeLibrary(hm);
				return false;

			}
			// call to init;
			if (!fn()) {
				DEVLOG(L"Failed to call 3rdrmx:Initialize() in rmcore_hooked_loadlibrary\n");
				::FreeLibrary(hm);
				return false;
			}
			return true;
		}

		inline bool get_3rdrmx_path(std::wstring& outpath) {
			outpath = nx::cregistry_3rdrmx::getInstance()->get_rmx_path();
			return nx::cregistry_3rdrmx::getInstance()->is_app_has_rmx();
		}

		inline bool get_fips_libear32_folder(std::wstring& folder) {
			folder.clear();
#ifdef _WIN64
			std::wstring value = L"path64";
#elif  _WIN32
			std::wstring value = L"path32";
#endif

			std::wstring strPath = aux::query_registry_value(L"Software\\NextLabs\\SkyDRM\\OSRmx", value, HKEY_LOCAL_MACHINE);
			auto pos = strPath.rfind(L"\\");
			if (std::wstring::npos == pos)
			{
				std::wstring strErrMsg = L"not find \\ and the path : " + strPath + L"\n";
				::OutputDebugStringW(strErrMsg.c_str());
				return false;
			}

			strPath = strPath.substr(0, pos);
			if (strPath.empty() || strPath.size() < 3)
			{
				std::wstring strErrMsg = L"the path is empty or size < 3, the path : " + strPath + L"\n";
				::OutputDebugStringW(strErrMsg.c_str());
				return false;
			}

			folder = strPath;
			return true;
		}

	};

	struct Global : public IDllLoader {
		HMODULE this_dll_moudle;
		bool is_match_target;
		bool is_hook_env_prepared;
		bool is_anti_reentrant;
		std::vector< IDllLoader*> loader;
		std::once_flag load_oxrmx_only_once;
		hook::recursion_control recur_cntl;

		Global() :this_dll_moudle(NULL),
			is_hook_env_prepared(false), is_anti_reentrant(false){}


		inline bool prepare_hook_env() {
			is_hook_env_prepared = false;
			hook::InitializeMadCHook();

			if (!hook::HookAPI("Kernel32", "LoadLibraryW",
				(PVOID)hook::Hooked_LoadLibraryW,
				(PVOID*)&hook::Hooked_LoadLibraryW_Next)) {
				DEVLOG(L"failed in hook LoadLibraryW\n");
				return false;
			}

			if (!hook::HookAPI("Kernel32", "GetStartupInfoW",
				(PVOID)hook::Hooked_GetStartupInfoW,
				(PVOID*)&hook::Hooked_GetStartupInfoW_Next)) {
				DEVLOG(L"failed in hook GetStartupInfoW\n");
				return false;
			}

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
			if (!hook::HookAPI("KernelBase", "FindFirstFileExW",
				(PVOID)hook::Hooked_FindFirstFileExW,
				(PVOID*)&hook::Hooked_FindFirstFileExW_Next)) {
				DEVLOG(L"failed in hook FindFirstFileExW\n");
				return false;
			}
#endif  // #if 0

			global.is_hook_env_prepared = true;
			return true;
		}

		inline void release_res() {
			for (auto i : loader) {
				delete i;
			}

			if (is_hook_env_prepared) {
				hook::FinalizeMadCHook();
			}
		}


		// Inherited via IDllLoader
		virtual bool match_context() override
		{
			{
				IDllLoader* osrmx = new OsRmx();
				if (osrmx->match_context()) {
					loader.push_back(osrmx);
				}
			}
			{
				IDllLoader* osdlp = new OsDlp();
				if (osdlp->match_context()) {
					loader.push_back(osdlp);
				}
			}
			{
				IDllLoader* thirdrmx = new ThirdPartyRmx();
				if (thirdrmx->match_context()) {
					loader.push_back(thirdrmx);
				}
			}

			return !loader.empty();
		}

		virtual bool prepare_env() override
		{
			if (loader.empty()) {
				return true;
			}
			for (auto i : loader) {
				i->prepare_env();
			}
			if (!prepare_hook_env()) {
				return false;
			}
			return true;
		}

		virtual bool do_load() override
		{
			if (loader.empty()) {
				return false;
			}

			if (is_anti_reentrant) {
				return false;
			}

			std::call_once(load_oxrmx_only_once, [this]() {
				    // The call of this->impl_do_load() maybe trigger hooked LoadLibrary\GetStartUpInfo calling again, 
	                // then reenter std::call_once again, which will results in deadlock, so should do anti-reenter here.(fix bajaj Office 365 issue)
					is_anti_reentrant = true;
					this->impl_do_load(); 
					is_anti_reentrant = false;
				}
			);
			return true;
		}

		void impl_do_load() {

			for (auto i : loader) {
				i->do_load();
			}

			//std::thread t([]() {
		//	std::this_thread::yield();
		//	std::this_thread::sleep_for(std::chrono::seconds(10));
		//	// try i want to unhook this and free rmcore library
		//	hook::UnhookAPI((PVOID*)&hook::Hooked_LoadLibraryW_Next);
		//	hook::UnhookAPI((PVOID*)&hook::Hooked_GetStartupInfoW_Next);
		//	::FreeLibrary(global.this_dll_moudle);
		//});
		//t.detach();
		}

	}global;
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		DEVLOG(L"dll_attach nxrmcore\n");
		::DisableThreadLibraryCalls(hModule);
		global.this_dll_moudle = hModule;

		if (!global.match_context()) {
			DEVLOG(L"not match target in rmcore\n");
			return FALSE;
		}		
		if (!global.prepare_env()) {
			DEVLOG(L"can not prepare oxrmx_env in rmcore\n");
			return FALSE;
		}
		return TRUE;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
	{
		DEVLOG(L"dll_detach nxrmcore\n");
		global.release_res();
		break;
	}
	}
	return TRUE;
}

namespace hook {
	HMODULE WINAPI Hooked_LoadLibraryW(LPCWSTR path) {
		if (global.recur_cntl.is_disabled()) {
			return Hooked_LoadLibraryW_Next(path);
		}
		hook::recursion_control_auto auto_disable(global.recur_cntl);
		global.do_load();
		return Hooked_LoadLibraryW_Next(path);
	}

	VOID WINAPI Hooked_GetStartupInfoW(LPSTARTUPINFOW lpStartupInfo) {
		if (global.recur_cntl.is_disabled()) {
			return Hooked_GetStartupInfoW_Next(lpStartupInfo);
		}
		hook::recursion_control_auto auto_disable(global.recur_cntl);
		global.do_load();
		return Hooked_GetStartupInfoW_Next(lpStartupInfo);
	}

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
	HANDLE WINAPI Hooked_FindFirstFileExW(LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags) {
		if (global.recur_cntl.is_disabled()) {
			return Hooked_FindFirstFileExW_Next(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
		}
		hook::recursion_control_auto auto_disable(global.recur_cntl);
		global.do_load();
		return Hooked_FindFirstFileExW_Next(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
	}
#endif  // #if 0
}