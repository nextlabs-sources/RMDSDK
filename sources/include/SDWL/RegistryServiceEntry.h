#pragma once

#include <string>
#include "SDLInstance.h"


namespace SkyDRM {

	class CRegistryServiceEntry
	{
	public:
		CRegistryServiceEntry(ISDRmcInstance* pInstance, std::string strSecurity);

		~CRegistryServiceEntry();
	public:

		SDWLResult get_value(HKEY hRoot, const std::string& strKey, const std::string& strItemName, uint32_t& u32ItemValue);
		SDWLResult get_value(HKEY hRoot, const std::string& strKey, const std::string& strItemName, uint64_t& u64ItemValue);
		SDWLResult get_value(HKEY hRoot, const std::string& strKey, const std::string& strItemName, std::string& strItemValue);
		SDWLResult get_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItemName, uint32_t& lItemValue);
		SDWLResult get_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItemName, uint64_t& u64ItemValue);
		SDWLResult get_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItemName, std::wstring& strItemValue);

		SDWLResult set_value(HKEY hRoot, const std::string& strKey, const std::string& strItemName, uint32_t u32ItemValue);
		SDWLResult set_value(HKEY hRoot, const std::string& strKey, const std::string& strItemName, uint64_t u64ItemValue);
		SDWLResult set_value(HKEY hRoot, const std::string& strKey, const std::string& strItemName, const std::string& strItemValue, bool expandable = false);
		SDWLResult set_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItemName, uint32_t u32ItemValue);
		SDWLResult set_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItemName, uint64_t u64ItemValue);
		SDWLResult set_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItemName, const std::wstring& strItemValue, bool expandable = false);

		SDWLResult delete_key(HKEY hRoot, const std::string& strKey);
		SDWLResult delete_key(HKEY hRoot, const std::wstring& strKey);

	protected:
		ISDRmcInstance*		m_pInstance;
		std::string			m_strSecurityCode;
	};
}