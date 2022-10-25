
#ifndef __REGISTRY_WHITELIST_H__
#define __REGISTRY_WHITELIST_H__

#include <set>

namespace nx {
	class cregistry_whitelist
	{
	public:
		~cregistry_whitelist();

		static cregistry_whitelist* getInstance();

	protected:
		cregistry_whitelist();

	public:

		bool is_app_in_whitelist();

		bool is_app_can_view();

		bool is_app_can_print();

		bool is_app_can_save();

		bool is_app_can_saveas();

		bool is_app_can_copycontent();

		bool is_app_can_printscreen();

	protected:

		void load_whitelist_from_registry();

		void parse_actions(const std::wstring& strActions);

	protected:

		bool has_action(const std::wstring& strAction);

		std::wstring get_whitelist_itemname(const std::wstring& strAppPath);

        std::wstring gen_md5_hex_string(const std::wstring& strAppPath);

        std::string to_string(const std::wstring& strAppPath);

		std::vector<std::wstring> regex_split(const std::wstring& in, const std::wstring& delim);

		std::wstring get_current_process_path();

		std::wstring get_process_path(DWORD ProcessId);

		std::wstring reg_get_value(const std::wstring &strSubKey, const std::wstring &strValueName, HKEY hRoot = HKEY_CURRENT_USER);

	protected:
		bool			m_bAppInWhiteLists;
		std::wstring	m_strAppProcessPath;
		std::wstring	m_strAppWhiteListName;
		std::set<std::wstring>	m_setActions;
	};
}

#endif
