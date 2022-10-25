
#ifndef __APP_WHITELIST_CONFIG_H__
#define __APP_WHITELIST_CONFIG_H__

#include <Windows.h>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <shared_mutex>
#include "nudf\xml.hpp"


//HKEY_LOCAL_MACHINE\SOFTWARE\NextLabs\SkyDRM\nxrmhandler
//  EXT1 : PATH
//	EXT2 : PATH
//	...
//	EXTn : PATH
//

//HKEY_LOCAL_MACHINE\SOFTWARE\NextLabs\SkyDRM\OSRmx\whitelists
// appname:hash(apppath)
//		actions: VIEW | PRINT | SAVE | SAVEAS
//		inherit: 0 (false), 1 (true)
//		path: C:\Program Files(x86)\Notepad++\notepad++.exe

//<APPITEM>
//<PATH>C:\Program Files(x86)\Microsoft Office\Office16\WINWORD3.EXE< / PATH>
//<EXT>.doc | .docx< / EXT>
//<ACTIONS>VIEW | PRINT | SAVE | SAVEAS< / ACTIONS>
//<INHERIT>TRUE< / INHERIT>
//</APPITEM>

namespace nx {
    class cwhitelist_item {
    public:
        cwhitelist_item()
        {
            m_strAppPath = L"";
            m_strExt = L"";
            m_strAppActions = L"";
            m_strOverPass = L"";
			m_strCleanupCMD = L"";
			m_ulInherit = 0;
        }

        ~cwhitelist_item()
        {

        }

    public:
        std::wstring	m_strAppPath;
        std::wstring	m_strExt;
        std::wstring	m_strAppActions;
        std::wstring    m_strOverPass;
        unsigned long	m_ulInherit;
		std::wstring    m_strCleanupCMD;
	};


    class capp_whiltelist_config
    {
    public:
        ~capp_whiltelist_config();

        static capp_whiltelist_config* getInstance();

    protected:
        capp_whiltelist_config();

    public:

        void check_register_file();

        std::set<std::wstring> get_app_whitelists();

        bool is_app_in_whitelist(const std::wstring& strAppPath);

        bool is_app_can_inherit(const std::wstring& strAppPath);

        bool is_app_can_save(const std::wstring& strAppPath);

        void insert_osrmx_process(unsigned long ulProcessID);

        bool remove_osrmx_process(unsigned long ulProcessID);

        bool is_osrmx_active_process(unsigned long ulProcessID);

    protected:

        bool is_whitelist_modified();
        bool is_register_file_modified();
        bool parse_register_file();

        bool parse_osrmxapps(std::shared_ptr<NX::xml_node>& node);
        bool parse_trustedprinters(std::shared_ptr<NX::xml_node>& node);

        void update_register();

        void insert_whitelist_app(std::shared_ptr<cwhitelist_item>& whitelistItem);
        void insert_file_association(const std::wstring&strAppPath, const std::wstring& strExt);

        std::wstring get_whitelist_itemname(const std::wstring& strAppPath);

        std::wstring gen_md5_hex_string(const std::wstring& strAppPath);

        std::string to_string(const std::wstring& strAppPath);

    protected:

        //registry relevant function
        void register_osrmx_whitelist();
        void register_file_association();
        void register_trustedprinters();

        void unregister_osrmx_whitelist();
        void unregister_file_association();
        void unregister_trustedprinters();

        void reg_set_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItem, const std::wstring& strItemValue);
        void reg_set_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strItem, unsigned long ulItemValue);

        bool reg_delete_key(HKEY hRoot, const std::wstring& strKey);
        bool reg_delete_value(HKEY hRoot, const std::wstring& strKey, const std::wstring& strValue);

    protected:

        unsigned long inherit_to_ulong(const std::wstring& inherit);

        std::vector<std::wstring> reg_split(const std::wstring& in, const std::wstring& delim);

        unsigned long find_current_user_sessionid();

        static void funRegisterThread(capp_whiltelist_config* pConfig);

    protected:

        std::mutex		m_mtxActivePID;

        std::wstring	m_strRegisterFile;
        std::wstring	m_strWindowsUserId;

        std::wstring    m_strTrustedPrinters;

        mutable std::shared_mutex		m_mutexWhiteList; //read, write lock for m_mapWhiteList

        std::map<std::wstring, std::wstring>	m_mapExtPath;
        std::map<std::wstring, std::shared_ptr<cwhitelist_item>>	m_mapWhiteList;

        std::map<std::wstring, std::wstring>	m_mapNewExtPath;
        std::map<std::wstring, std::shared_ptr<cwhitelist_item>>	m_mapNewWhiteList;

        std::map<unsigned long, unsigned long long>		m_mapOSRmxActivePID;
    };
}

#endif