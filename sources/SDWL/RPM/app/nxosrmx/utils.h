#pragma once
/*
	Global utilities defined here, this is the top level header, that any low level may incldue it.
		- make it as independent as possible
		- required each function and class can be tested separately
*/
#include "overlay.h"

#define NXUTILS_NAMESPACE namespace nx {namespace utils {
#define END_NXUTILS_NAMESPACE }}

//
NXUTILS_NAMESPACE
//

BOOL GetFileNameFromHandle(HANDLE hFile, std::wstring& strFilePath);

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
		if (ac.is_process_disabled()) {
			ac.process_enable();
		}
	}
private:
	recursion_control& ac;

};

inline void dev_log(const wchar_t* str) {
#ifdef _DEBUG
	::OutputDebugStringW(str);
#endif // _DEBUG
}
#ifdef DEVLOG
#error "can't be this, someone has defined DEVLOG"
#else
#ifdef _DEBUG
#define DEVLOG(str)   nx::utils::dev_log((str))
#else
#define DEVLOG(str) (0)
#endif // _DEBUG
#endif // DEVLOG

class Module {
public:
	Module(HMODULE hm = NULL) :m_hm(hm) {
		std::vector<char> buf(255, 0);
		buf.resize(GetModuleFileNameA(m_hm, (char*)buf.data(), 255));
		m_path.assign(buf.begin(), buf.end());

		std::vector<wchar_t> wbuf(255, 0);
		wbuf.resize(GetModuleFileNameW(m_hm, (wchar_t*)wbuf.data(), 255));
		m_wpath.assign(wbuf.begin(), wbuf.end());
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


inline HWND get_toplevel_window(HWND hw) {
	HWND parent = hw;
	HWND tmp;
	while ((tmp = ::GetParent(parent)) != NULL)
		parent = tmp;

	return parent;
}

inline bool is_toplevel_window(HWND hw) {
	return ::GetParent(hw) != NULL;
}

inline bool has_child_style(HWND wnd) {
	auto ws = ::GetWindowLong(wnd, GWL_STYLE);
	return ws & WS_CHILD;
}

inline std::string str_toupper(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::toupper(c); } // correct
	);
	return s;
}

inline std::wstring wstr_toupper(std::wstring s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::toupper(c); } // correct
	);
	return s;
}

inline std::wstring wstr_tolower(std::wstring s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::tolower(c); } // correct
	);
	return s;
}

inline std::wstring to_wstr(const std::string& s) {
	return std::wstring(s.begin(), s.end());
}


inline bool Is_Outlook_clsid(const IID& iid) {
	// {CLSID_Microsoft Outlook}
	return iid.Data1 == 454714
		&& iid.Data2 == 0
		&& iid.Data3 == 0
		&& strlen((const char*)iid.Data4) == 1
		&& iid.Data4[0] == 192;
}

#ifdef NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool is_sanctuary_file(const std::wstring &wstrPath);
#endif // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

inline void parse_dirs(std::vector<std::wstring>& markDir, const WCHAR* str)
{
	std::wstringstream f(str);
	std::wstring s;
	while (std::getline(f, s, L'<'))
	{
		markDir.push_back(s + L"\\");
	}
}

std::wstring utf82utf16(const std::string& str);

class Registry {
public:
	class param {
		friend class Registry;
	public:
		param(std::wstring path, HKEY which_root = HKEY_CURRENT_USER, REGSAM samDesired = KEY_READ) :sub_key(path), access_right(samDesired), root(which_root), open_key(NULL) {}
	private:
		HKEY root; //HKEY_CLASSES_ROOT,HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE
		std::wstring sub_key;
		REGSAM access_right;
		HKEY open_key;
		class close_guard {
			friend class Registry;
			HKEY _open_key;
			close_guard(HKEY open_key) :_open_key(open_key) {}
			~close_guard() { ::RegCloseKey(_open_key); }
		};
	};

	bool get(param& p, const std::wstring& name, std::wstring& out_value) {
		if (!_open(p)) {
			return false;
		}
		param::close_guard gurad(p.open_key);
		std::uint32_t length = _buflen(p, name);
		if (0 > length) {
			return false;
		}
		else if (0 == length) {
			out_value.clear();
			return true;
		}
		else {
			out_value.resize(length / 2);
		}

		if (!_get(p, name, (std::uint8_t*)out_value.data(), length)) {
			return false;
		}
		// str trim
		if (out_value.back() == '\0') {
			out_value.pop_back();
		}
		return true;

	}

	bool get(param& p, const std::wstring& name, std::uint32_t& out_value) {
		if (!_open(p)) {
			return false;
		}
		param::close_guard gurad(p.open_key);
		std::uint32_t length = _buflen(p, name);
		if (-1 == length || length != sizeof(out_value)) {
			return false;
		}
		return _get(p, name, (std::uint8_t*) & out_value, sizeof(out_value));
	}
private:
	inline bool _open(param& p) {
		return ERROR_SUCCESS == ::RegOpenKeyExW(p.root, p.sub_key.c_str(), NULL, p.access_right, &p.open_key);
	}
	inline std::uint32_t _buflen(param& p, const std::wstring& name) {
		DWORD length = -1;
		DWORD type;
		::RegQueryValueExW(p.open_key, name.c_str(), NULL, &type, NULL, &length);
		return length;

	}
	inline bool _get(param& p, const std::wstring& name, std::uint8_t* buf, std::uint32_t buf_len) {
		return ERROR_SUCCESS == ::RegQueryValueExW(p.open_key, name.c_str(), NULL, NULL, buf, (DWORD*)&buf_len);
	}

};

//
END_NXUTILS_NAMESPACE
//

