
#include "app_whitelist_config.h"
#include <tlhelp32.h>
#include <Shlwapi.h>
#include <regex>
#include "nudf\winutil.hpp"
#include "nudf\eh.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include "global.hpp"
#include <experimental/filesystem>
#include <Shlwapi.h>
#include <nudf\crypto.hpp>
#pragma comment(lib, "Shlwapi.lib")

std::atomic_flag whitelist_thread_running = ATOMIC_FLAG_INIT;

namespace nx {

    std::wstring trim_and_tolower(const std::wstring& str)
    {
        std::wstring strData(str);
        strData.erase(0, strData.find_first_not_of(L" "));
        strData.erase(strData.find_last_not_of(L" ") + 1);

        if (strData.empty())
        {
            return strData;
        }

        std::transform(strData.begin(), strData.end(), strData.begin(), ::towlower);
        return strData;
    }

    capp_whiltelist_config::capp_whiltelist_config()
    {
        m_strRegisterFile = GLOBAL.get_config_dir() + L"\\register.xml";
    }

    capp_whiltelist_config::~capp_whiltelist_config()
    {

    }

    capp_whiltelist_config* capp_whiltelist_config::getInstance()
    {
        static capp_whiltelist_config _instance;
        return &_instance;
    }


    void capp_whiltelist_config::check_register_file()
    {
        if (!is_whitelist_modified())
            return;

        //unsigned long session_id = find_current_user_sessionid();
        //NX::win::session_token st(session_id);
        //m_strWindowsUserId = st.get_user().id();
        if (whitelist_thread_running.test_and_set())
        {
            return;
        }

        std::thread fileThread(funRegisterThread, this);
        fileThread.detach();
    }

    std::set<std::wstring> capp_whiltelist_config::get_app_whitelists()
    {
        std::set<std::wstring>	setWhitelist;

        {
            std::shared_lock<std::shared_mutex> lock(m_mutexWhiteList);
            for (auto item : m_mapWhiteList)
            {
                setWhitelist.insert(item.second->m_strAppPath);
            }
        }

        return setWhitelist;
    }

    bool capp_whiltelist_config::is_app_in_whitelist(const std::wstring& strAppPath)
    {
        std::wstring strWhiteListItem = get_whitelist_itemname(strAppPath);

        {
            std::shared_lock<std::shared_mutex> lock(m_mutexWhiteList);
            auto itFind = m_mapWhiteList.find(strWhiteListItem);
            if (m_mapWhiteList.end() != itFind)
            {
                return true;
            }
        }

        return false;
    }

    bool capp_whiltelist_config::is_app_can_inherit(const std::wstring& strAppPath)
    {
        std::wstring strPath = trim_and_tolower(strAppPath);

        std::wstring strWhiteListItem = get_whitelist_itemname(strAppPath);

        {
            std::shared_lock<std::shared_mutex> lock(m_mutexWhiteList);
            auto itFind = m_mapWhiteList.find(strWhiteListItem);
            if (m_mapWhiteList.end() != itFind)
            {
                if ((1 == itFind->second->m_ulInherit) && (0 == strPath.compare(itFind->second->m_strAppPath)))
                    return true;
            }
        }

        return false;
    }

    bool capp_whiltelist_config::is_app_can_save(const std::wstring& strAppPath)
    {
        std::wstring strWhiteListItem = get_whitelist_itemname(strAppPath);

        {
            std::shared_lock<std::shared_mutex> lock(m_mutexWhiteList);
            auto itFind = m_mapWhiteList.find(strWhiteListItem);
            if (m_mapWhiteList.end() != itFind)
            {
                std::wstring strActions = itFind->second->m_strAppActions;
                std::transform(strActions.begin(), strActions.end(), strActions.begin(), ::towlower);
                auto vecActions = reg_split(strActions, L"[|]+");
                for (auto item : vecActions)
                {
                    std::wstring strAction = item;
                    strAction.erase(0, strAction.find_first_not_of(L" "));
                    strAction.erase(strAction.find_last_not_of(L" ") + 1);

                    if (0 == strAction.compare(L"save"))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void capp_whiltelist_config::insert_osrmx_process(unsigned long ulProcessID)
    {
        std::lock_guard<std::mutex>   guard(m_mtxActivePID);

        auto itFind = m_mapOSRmxActivePID.find(ulProcessID);
        if (m_mapOSRmxActivePID.end() != itFind)
            return;

        ULONGLONG ulTick64 = ::GetTickCount64();
        m_mapOSRmxActivePID.insert(std::make_pair(ulProcessID, ulTick64));
    }

    bool capp_whiltelist_config::remove_osrmx_process(unsigned long ulProcessID)
    {
        std::lock_guard<std::mutex>   guard(m_mtxActivePID);

        auto itFind = m_mapOSRmxActivePID.find(ulProcessID);
        if (m_mapOSRmxActivePID.end() == itFind)
            return false;

        m_mapOSRmxActivePID.erase(itFind);
        return true;
    }

    bool capp_whiltelist_config::is_osrmx_active_process(unsigned long ulProcessID)
    {
        std::lock_guard<std::mutex>   guard(m_mtxActivePID);

        auto itFind = m_mapOSRmxActivePID.find(ulProcessID);
        if (m_mapOSRmxActivePID.end() == itFind)
            return false;

        return true;
    }

    bool capp_whiltelist_config::is_whitelist_modified()
    {
        if (!is_register_file_modified())
            return false;

        return true;
    }

    bool capp_whiltelist_config::is_register_file_modified()
    {
        static struct _stat64i32 s_buf = { 0 };

        try {
            struct _stat64i32 buf;
            _wstat(m_strRegisterFile.c_str(), &buf);
            if ((s_buf.st_mtime != buf.st_mtime) || (s_buf.st_atime != buf.st_atime))
            {
                s_buf.st_atime = buf.st_atime;
                s_buf.st_mtime = buf.st_mtime;
                return true;
            }
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
            std::string strMsg = "is_register_file_modified exception : " + std::string(e.what()) + "\n";
            ::OutputDebugStringA(strMsg.c_str());
            return false;
        }

        return false;
    }



    //<APPITEM>
    //<PATH>C:\Program Files(x86)\Microsoft Office\Office16\WINWORD.EXE< / PATH>
    //<EXT>.doc | .docx< / EXT>
    //<ACTIONS>VIEW | PRINT | SAVE | SAVEAS< / ACTIONS>
    //</APPITEM>

    bool capp_whiltelist_config::parse_register_file()
    {
        try {
            NX::xml_document xmldoc;
            xmldoc.load_from_file(m_strRegisterFile);
            std::shared_ptr<NX::xml_node> spRoot = xmldoc.document_root();
            if (spRoot == nullptr)
                return false;

            parse_osrmxapps(spRoot);

            parse_trustedprinters(spRoot);
        }
        catch (const std::exception& e) {
            UNREFERENCED_PARAMETER(e);
            return false;
        }
        return true;
    }

    bool capp_whiltelist_config::parse_osrmxapps(std::shared_ptr<NX::xml_node>& node)
    {
        std::shared_ptr<NX::xml_node> spTrustedApps = node->find_child_element(L"OSRMXAPPS");
        if (nullptr == spTrustedApps)
            return false;

        for (auto child : spTrustedApps->get_children())
        {
            std::wstring childName = child->get_name();
            childName.erase(0, childName.find_first_not_of(L" "));
            childName.erase(childName.find_last_not_of(L" ") + 1);
            std::transform(childName.begin(), childName.end(), childName.begin(), ::towlower);
            if (0 != childName.compare(L"appitem"))
            {
                continue;
            }

            auto whitelistItem = std::make_shared<cwhitelist_item>();
            std::shared_ptr<NX::xml_node> spPath = child->find_child_element(L"PATH");
            if (!spPath)
            {
                continue;
            }
            whitelistItem->m_strAppPath = trim_and_tolower(spPath->get_text());

			// We ignore this check for UNC path, since this check will failed sometimes 
			// even though the path is actually exists in some machines (e.g. QA test machine),fix bug 70888
			if (!PathIsUNC(whitelistItem->m_strAppPath.c_str())) {
				if (!std::experimental::filesystem::exists(whitelistItem->m_strAppPath))
					continue;
			}

            std::shared_ptr<NX::xml_node> spExt = child->find_child_element(L"EXT");
            if (spExt)
            {
                whitelistItem->m_strExt = spExt->get_text();
            }

            std::shared_ptr<NX::xml_node> spActions = child->find_child_element(L"ACTIONS");
            if (spActions)
            {
                whitelistItem->m_strAppActions = spActions->get_text();
            }

            std::shared_ptr<NX::xml_node> spOverPass = child->find_child_element(L"OVERPASS");
            if (spOverPass)
            {
                whitelistItem->m_strOverPass = spOverPass->get_text();
            }

			std::shared_ptr<NX::xml_node> spCleanUP = child->find_child_element(L"CLEANUP");
			if (spCleanUP)
			{
				whitelistItem->m_strCleanupCMD = spCleanUP->get_text();
			}
			

			std::shared_ptr<NX::xml_node> spInherit = child->find_child_element(L"INHERIT");
            if (spInherit)
            {
                whitelistItem->m_ulInherit = inherit_to_ulong(spInherit->get_text());
            }

            insert_whitelist_app(whitelistItem);
            insert_file_association(whitelistItem->m_strAppPath, whitelistItem->m_strExt);
        }

        return true;
    }

    bool capp_whiltelist_config::parse_trustedprinters(std::shared_ptr<NX::xml_node>& node)
    {
        m_strTrustedPrinters.clear();
        std::shared_ptr<NX::xml_node> spTrustedPrinters = node->find_child_element(L"TRUSTEDPRINTERS");
        if (nullptr == spTrustedPrinters)
            return false;

        std::set<std::wstring> setPrinter;
        for (auto child : spTrustedPrinters->get_children())
        {
            std::wstring childName = child->get_name();
            childName.erase(0, childName.find_first_not_of(L" "));
            childName.erase(childName.find_last_not_of(L" ") + 1);
            std::transform(childName.begin(), childName.end(), childName.begin(), ::towlower);
            if (0 != childName.compare(L"printer"))
            {
                continue;
            }

            std::wstring strPrinter = child->get_text();
            strPrinter.erase(strPrinter.find_last_not_of(L" /t") + 1);
            strPrinter.erase(0, strPrinter.find_first_not_of(L" /t"));
            setPrinter.insert(strPrinter);
        }

        std::wstring strPrinters;
        for (auto item : setPrinter)
        {
            strPrinters += item + L"|";
        }

        std::size_t pos = strPrinters.rfind(L"|");
        if ((std::wstring::npos != pos) && (pos == strPrinters.length() - 1))
        {
            strPrinters = strPrinters.substr(0, pos);
        }

        m_strTrustedPrinters = strPrinters;
        return true;
    }

    void capp_whiltelist_config::update_register()
    {
        unregister_osrmx_whitelist();
        unregister_file_association();
        unregister_trustedprinters();

        register_osrmx_whitelist();
        register_file_association();
        register_trustedprinters();

        {
            std::unique_lock<std::shared_mutex> lock(m_mutexWhiteList);
            m_mapWhiteList = std::move(m_mapNewWhiteList);
        }

        m_mapExtPath = std::move(m_mapNewExtPath);
    }

    void capp_whiltelist_config::insert_whitelist_app(std::shared_ptr<cwhitelist_item>& whitelistItem)
    {
        std::wstring itemName = get_whitelist_itemname(whitelistItem->m_strAppPath);
        m_mapNewWhiteList.insert(std::make_pair(itemName, whitelistItem));
    }

    void capp_whiltelist_config::insert_file_association(const std::wstring&strAppPath, const std::wstring& strExt)
    {
        auto vecExt = reg_split(strExt, L"[|]+");
        for (auto ext : vecExt)
        {
            ext.erase(0, ext.find_first_not_of(L" "));
            ext.erase(ext.find_last_not_of(L" ") + 1);
            if (!ext.empty())
            {
                std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
                m_mapNewExtPath.insert(std::make_pair(ext, strAppPath));
            }
        }
    }

    std::wstring capp_whiltelist_config::get_whitelist_itemname(const std::wstring& strAppPath)
    {
        std::wstring appPath(strAppPath);
        appPath.erase(0, appPath.find_first_not_of(L" "));
        appPath.erase(appPath.find_last_not_of(L" ") + 1);
        std::transform(appPath.begin(), appPath.end(), appPath.begin(), ::towlower);

        auto pos = appPath.rfind(L"\\");
        if (std::wstring::npos == pos)
            return L"";

        //std::wstring appName = appPath.substr(pos + 1);
        //appName.erase(0, appName.find_first_not_of(L" "));
        //appName.erase(appName.find_last_not_of(L" ") + 1);

        //std::hash<std::wstring> hash_fn;
        //size_t str_hash = hash_fn(appPath);
        //std::wstring strAppPathHash = std::to_wstring(str_hash);

        std::wstring strMd5 = gen_md5_hex_string(appPath);

        std::wstring strItemName = strMd5;//appName;// +L":" + strAppPathHash;
        return strItemName;
    }

    std::wstring capp_whiltelist_config::gen_md5_hex_string(const std::wstring& strAppPath)
    {
        std::wstring strMd5;
        std::vector<unsigned char> md5_result;
        md5_result.resize(16, 0);

        std::string strData = to_string(strAppPath);

        if (NX::crypto::md5((const unsigned char*)strData.data(), (ULONG)strData.size(), md5_result.data()))
        {
            strMd5 = NX::conversion::to_wstring(md5_result);
        }

        return strMd5;
    }

    std::string capp_whiltelist_config::to_string(const std::wstring& wstr)
    {
        using convert_t = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_t, wchar_t> strconverter;
        return strconverter.to_bytes(wstr);
    }

    void capp_whiltelist_config::reg_set_value(
        HKEY hRoot,
        const std::wstring& strKey,
        const std::wstring& strItem,
        const std::wstring& strItemValue)
    {
        try {
            NX::win::reg_key key;
            std::wstring strItemName(L"");

            if (!key.exist(hRoot, strKey)) {
                key.create(hRoot, strKey, NX::win::reg_key::reg_position::reg_default);
                key.set_value(strItem, strItemValue, true);
            }
            else {
                key.open(hRoot, strKey, NX::win::reg_key::reg_position::reg_default, false);
                key.set_value(strItem, strItemValue, true);
            }
        }
        catch (const NX::exception &e) {
            UNREFERENCED_PARAMETER(e);
        }
    }

    void capp_whiltelist_config::reg_set_value(
        HKEY hRoot,
        const std::wstring& strKey,
        const std::wstring& strItem,
        unsigned long ulItemValue)
    {
        try {
            NX::win::reg_key key;
            std::wstring strItemName(L"");

            if (!key.exist(hRoot, strKey)) {
                key.create(hRoot, strKey, NX::win::reg_key::reg_position::reg_default);
                key.set_value(strItem, ulItemValue);
            }
            else {
                key.open(hRoot, strKey, NX::win::reg_key::reg_position::reg_default, false);
                key.set_value(strItem, ulItemValue);
            }
        }
        catch (const NX::exception &e) {
            UNREFERENCED_PARAMETER(e);
        }
    }

    bool capp_whiltelist_config::reg_delete_key(HKEY hRoot, const std::wstring& strKey)
    {
        long result = ::SHDeleteKeyW(hRoot, strKey.c_str());
        if ((ERROR_SUCCESS == result) || (ERROR_FILE_NOT_FOUND == result))
        {
            return true;
        }
        return false;
    }

    bool capp_whiltelist_config::reg_delete_value(
        HKEY hRoot,
        const std::wstring& strKey,
        const std::wstring& strValue)
    {
        long result = ::SHDeleteValueW(hRoot, strKey.c_str(), strValue.c_str());

        std::wstring strMsg = L"delete value : " + strKey + L"(" + strValue + L")" + L" result : " + std::to_wstring(result);
        ::OutputDebugStringW(strMsg.c_str());
        if ((ERROR_SUCCESS == result) || (ERROR_FILE_NOT_FOUND == result))
        {
            return true;
        }
        return false;
    }

    //HKEY_LOCAL_MACHINE\SOFTWARE\NextLabs\SkyDRM\OSRmx\whitelists
    // appname:hash(apppath)
    //		actions: VIEW | PRINT | SAVE | SAVEAS
    //		inherit: 0 (false), 1 (true)
    //		path: C:\Program Files(x86)\Notepad++\notepad++.exe
    void capp_whiltelist_config::register_osrmx_whitelist()
    {
        for (auto item : m_mapNewWhiteList)
        {
            std::wstring strKeyName = get_whitelist_itemname(item.second->m_strAppPath);

            std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM\\OSRmx\\whitelists\\" + strKeyName;

            reg_set_value(HKEY_LOCAL_MACHINE, strKey, L"path", item.second->m_strAppPath);
            reg_set_value(HKEY_LOCAL_MACHINE, strKey, L"actions", item.second->m_strAppActions);
            reg_set_value(HKEY_LOCAL_MACHINE, strKey, L"overpass", item.second->m_strOverPass);
			reg_set_value(HKEY_LOCAL_MACHINE, strKey, L"cleanup", item.second->m_strCleanupCMD);
			reg_set_value(HKEY_LOCAL_MACHINE, strKey, L"inherit", item.second->m_ulInherit);

            std::wstring strMsg = L"***whitelist register key (whitelists): " + strKey + L" ***\n";
            ::OutputDebugStringW(strMsg.c_str());
        }
    }


    //HKEY_LOCAL_MACHINE\SOFTWARE\NextLabs\SkyDRM\nxrmhandler
    //EXT1 : PATH
    //	EXT2 : PATH
    //	...
    //	EXTn : PATH
    void capp_whiltelist_config::register_file_association()
    {
        for (auto item : m_mapNewExtPath)
        {
            std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM\\nxrmhandler\\" + item.first;
            reg_set_value(HKEY_LOCAL_MACHINE, strKey, L"", item.second);

            std::wstring strMsg = L"***whitelist register key (nxrmhandler): " + strKey + L" ***\n";
            ::OutputDebugStringW(strMsg.c_str());
        }
    }

    void capp_whiltelist_config::register_trustedprinters()
    {
        if (m_strTrustedPrinters.empty())
            return;

        std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM";
        reg_set_value(HKEY_LOCAL_MACHINE, strKey, L"TrustedPrinters", m_strTrustedPrinters);
    }

    void capp_whiltelist_config::unregister_osrmx_whitelist()
    {
        std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM\\OSRmx\\whitelists";
        bool bRet = reg_delete_key(HKEY_LOCAL_MACHINE, strKey);
        std::wstring strMsg = L"***whitelist delete key (whitelists): " + strKey + L" result : " + std::to_wstring(bRet) + L" ***\n";
        ::OutputDebugStringW(strMsg.c_str());

        //std::shared_lock<std::shared_mutex> lock(m_mutexWhiteList);
        //for (auto item : m_mapWhiteList)
        //{
        //	std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM\\OSRmx\\whitelists\\" + item.first;
        //	bool bRet = reg_delete_key(HKEY_LOCAL_MACHINE, strKey);

        //	std::wstring strMsg = L"***whitelist delete key (whitelists): " + strKey + L" result : " + std::to_wstring(bRet) + L" ***\n";
        //	::OutputDebugStringW(strMsg.c_str());
        //}
    }

    void capp_whiltelist_config::unregister_file_association()
    {
        for (auto item : m_mapExtPath)
        {
            std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM\\nxrmhandler\\" + item.first;
            bool bRet = reg_delete_key(HKEY_LOCAL_MACHINE, strKey);

            std::wstring strMsg = L"***whitelist delete key (nxrmhandler): " + strKey + L" result : " + std::to_wstring(bRet) + L" ***\n";
            ::OutputDebugStringW(strMsg.c_str());
        }
    }

    void capp_whiltelist_config::unregister_trustedprinters()
    {
        std::wstring strKey = L"SOFTWARE\\NextLabs\\SkyDRM";
        reg_delete_value(HKEY_LOCAL_MACHINE, strKey, L"TrustedPrinters");
    }


    unsigned long capp_whiltelist_config::inherit_to_ulong(const std::wstring& inherit)
    {
        unsigned long ulInherit = 0;
        std::wstring appInherit = inherit;
        appInherit.erase(0, appInherit.find_first_not_of(L" "));
        appInherit.erase(appInherit.find_last_not_of(L" ") + 1);
        std::transform(appInherit.begin(), appInherit.end(), appInherit.begin(), ::towlower);
        if (0 == appInherit.compare(L"true"))
        {
            ulInherit = 1;
        }
        return ulInherit;
    }

    std::vector<std::wstring> capp_whiltelist_config::reg_split(const std::wstring& in, const std::wstring& delim)
    {
        std::wregex re{ delim };
        return std::vector<std::wstring> {
            std::wsregex_token_iterator(in.begin(), in.end(), re, -1), std::wsregex_token_iterator()
        };
    }

    unsigned long capp_whiltelist_config::find_current_user_sessionid()
    {
        const std::wstring process_name(L"explorer.exe");
        unsigned long cur_session_id = 0;
        PROCESSENTRY32W pe32;

        memset(&pe32, 0, sizeof(pe32));
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE == hSnapshot)
        {
            return 0;
        }

        if (!Process32FirstW(hSnapshot, &pe32))
        {
            CloseHandle(hSnapshot);
            hSnapshot = INVALID_HANDLE_VALUE;
            return 0;
        }

        do {
            const wchar_t* name = wcsrchr(pe32.szExeFile, L'\\');
            name = (NULL == name) ? pe32.szExeFile : (name + 1);

            if (0 == _wcsicmp(process_name.c_str(), name))
            {
                if (ProcessIdToSessionId(pe32.th32ProcessID, &cur_session_id))
                {
                    break;
                }
            }
        } while (Process32NextW(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        hSnapshot = INVALID_HANDLE_VALUE;
        return cur_session_id;
    }

    void capp_whiltelist_config::funRegisterThread(capp_whiltelist_config* pConfig)
    {
        ::Sleep(1000);

        ULONGLONG ul64Tick1 = ::GetTickCount64();
        if (pConfig)
        {
            pConfig->parse_register_file();
            pConfig->update_register();
        }

        ULONGLONG ul64Tick2 = ::GetTickCount64();

        std::wstring tick = std::to_wstring(ul64Tick2 - ul64Tick1);

        std::wstring strMessage = L"***********whitelist threadid : " + std::to_wstring(::GetCurrentThreadId());
        strMessage += L" tick: " + tick + L" ms **************\n";
        ::OutputDebugStringW(strMessage.c_str());

        whitelist_thread_running.clear();
    }

}
