
#ifndef __REGISTRY_3RDRMX_H__
#define __REGISTRY_3RDRMX_H__

#include <set>

namespace nx {
	bool verify_nextlabs_signature(const std::wstring& module);

	class cregistry_3rdrmx
	{
	public:
		~cregistry_3rdrmx();

		static cregistry_3rdrmx* getInstance();

	protected:
		cregistry_3rdrmx();

	public:

		bool is_app_has_rmx();

		std::wstring get_rmx_path() { return m_strAppRMXPath; }


	protected:

		std::wstring get_app_name(const std::wstring& strAppPath);

		std::vector<std::wstring> regex_split(const std::wstring& in, const std::wstring& delim);

		void load_rmx_from_registry();

	protected:

		std::wstring get_current_process_path();

		std::wstring get_process_path(DWORD ProcessId);

		std::wstring reg_get_value(const std::wstring &strSubKey, const std::wstring &strValueName, HKEY hRoot = HKEY_CURRENT_USER);

	protected:
		bool			m_bAppHasRMX;
		std::wstring	m_strAppProcessPath;
		std::wstring	m_strAppRMXPath;
		std::wstring	m_strAppName;
	};
}

#endif
