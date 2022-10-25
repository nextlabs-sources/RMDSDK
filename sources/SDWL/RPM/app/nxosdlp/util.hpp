#pragma once

class scope_guard {
public:
    scope_guard(std::function<void()> fun) { m_f = fun; }
    ~scope_guard() { m_f(); }
private:
    std::function<void()> m_f;
};

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


inline std::string str_toupper(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::toupper(c); } // correct
	);
	return s;
}

inline std::string str_tolower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::tolower(c); } // correct
	);
	return s;
}

inline std::wstring wstr_toupper(std::wstring s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](wchar_t c) { return std::toupper(c); } // correct
	);
	return s;
}

inline std::wstring wstr_tolower(std::wstring s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](wchar_t c) { return std::tolower(c); } // correct
	);
	return s;
}

inline std::wstring to_wstr(const std::string& s) {
	return std::wstring(s.begin(), s.end());
}

inline bool iconstain(const std::string& s, const std::string& sub) {
	auto s_ = str_tolower(s);
	auto sub_ = str_tolower(sub);

	return s_.find(sub_, 0) != decltype(s_)::npos;
}

inline bool ibegin_with(const std::wstring& m, const std::wstring& s) {
	using namespace std;
	if (m.size() < s.size()) {
		return false;
	}
	auto m_it = m.cbegin();
	auto s_it = s.cbegin();
	for (; m_it != m.cend() && s_it != s.cend(); ++m_it, ++s_it) {
		if (tolower(*m_it) == tolower(*s_it)) {
			continue;
		}
		else {
			return false;
		}
	}
	// s must be get through
	if (s_it != s.cend()) {
		return false;
	}
	return true;
}

inline bool ibegin_with(const std::wstring& m, const std::set<std::wstring>& ss) {
	for (auto& i : ss) {
		if (ibegin_with(m, i)) {
			return true;
		}
	}
	return false;
}

inline bool iend_with(const std::wstring& m, const std::wstring& s) {
	if (m.size() < s.size()) {
		return false;
	}
	return std::equal(s.rbegin(), s.rend(), m.rbegin(), [](wchar_t l, wchar_t r) {
		return (tolower(l) == tolower(r));
		}
	);
}

inline bool is_win8andabove(void)
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;
	int op1 = VER_GREATER_EQUAL;

	// Initialize the OSVERSIONINFOEX structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 2;
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op1);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op1);

	return 0 != VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
}

inline bool is_nxl_file(const std::wstring& path) {
	auto file_exist_in_disk = [](const std::wstring& path) {
		// make sure file exist on disk
		DWORD fattr = ::GetFileAttributesW(path.c_str());
		// not a valid file
		if (fattr == INVALID_FILE_ATTRIBUTES) {
			return false;
		}
		// can be a folder
		if (fattr & FILE_ATTRIBUTE_DIRECTORY) {
			return false;
		}
		return true;
	};

	auto is_suffix_nxl = [](const std::wstring& path) {
		if (path.size() < 4) {
			return false;
		}
		return 0 == _wcsicmp(path.substr(path.length() - 4, 4).c_str(), L".nxl");
	};

	// .nxl suffix
	if (is_suffix_nxl(path)) {
		return file_exist_in_disk(path);
	}

	// file may be in rmp which do not have nxl suffix		
	if (!file_exist_in_disk(path)) {
		return false;
	}
	// cat path with .nxl and find this new file, if found, is nxl
	std::wstring nxl_cat_path = path + L".nxl";
	WIN32_FIND_DATAW findFileData = { 0 };
	HANDLE hFind = FindFirstFileW(nxl_cat_path.c_str(), &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		return !is_suffix_nxl(findFileData.cFileName);
	}
	return false;
}


inline bool is_outlook_process() {
	static std::string proc{ "OUTLOOK.EXE" };
	Module m;
	std::string cur = m.GetNameA();
	if (proc.size() != cur.size()) {
		return false;
	}
	if (0 == _stricmp(proc.c_str(), cur.c_str())) {
		return true;
	}
	return false;

}

inline bool is_solidwords_related_process() {
	static std::vector<std::string> solidewords_related{
		"xtop.exe",
		"vugraf.exe",
		"SLDWORKS.EXE",
		"cnext.exe",
		"acad.exe",
		"ugraf.exe"
	};
	Module m;
	std::string target = m.GetNameA();

	for (const auto& i : solidewords_related) {
		if (i.size() != target.size()) {
			continue;
		}
		if (0 == _stricmp(i.c_str(), target.c_str())) {
			return true;
		}
	}
	return false;
}

inline bool is_browser_related_process() {
  static std::vector<std::string> browser_related{
      "chrome.exe",
      "msedge.exe"
  };
  Module m;
  std::string target = m.GetNameA();

  for (const auto& i : browser_related) {
    if (i.size() != target.size()) {
      continue;
    }
    if (0 == _stricmp(i.c_str(), target.c_str())) {
      return true;
    }
  }
  return false;
}


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



class CRwLock
{
public:
	CRwLock() throw() { InitializeSRWLock(&m_srwLock); }
	virtual ~CRwLock() throw() {}

	inline PSRWLOCK GetLock() { return &m_srwLock; }

private:
	SRWLOCK m_srwLock;
};

class CRwExclusiveLocker
{
public:
	CRwExclusiveLocker(_In_ CRwLock* lock) throw() : m_pLock(lock) {
		if (NULL != m_pLock) {
			AcquireSRWLockExclusive(m_pLock->GetLock());
		}
	}
	~CRwExclusiveLocker() throw() {
		if (NULL != m_pLock) {
			ReleaseSRWLockExclusive(m_pLock->GetLock());
		}
	}

private:
	CRwLock* m_pLock;
};

class CRwSharedLocker
{
public:
	CRwSharedLocker(_In_ CRwLock* lock) throw() : m_pLock(lock) {
		if (NULL != m_pLock) {
			AcquireSRWLockShared(m_pLock->GetLock());
		}
	}
	~CRwSharedLocker() throw() {
		if (NULL != m_pLock) {
			ReleaseSRWLockShared(m_pLock->GetLock());
		}
	}

private:
	CRwLock* m_pLock;
};



class Registry {
public:
	class param {
		friend class Registry;
	public:
		param(const std::wstring& path, HKEY which_root = HKEY_LOCAL_MACHINE)
			:sub_key(path), access_right(KEY_READ | KEY_WOW64_64KEY), // always access 64bit, shutdown Registry-Visualization
			root(which_root), open_key(NULL) {}
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
