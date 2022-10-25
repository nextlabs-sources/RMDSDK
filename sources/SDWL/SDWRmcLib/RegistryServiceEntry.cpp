#include "stdafx.h"
#include "RegistryServiceEntry.h"

#include "common/celog2/celog.h"
#include "nlohmann/json.hpp"


#pragma warning( push )
#pragma warning( disable : 4003 4311 4302 )

using namespace SkyDRM;

#define CELOG_CUR_MODULE "rmdsdk"
#define CELOG_CUR_FILE CELOG_FILEPATH_SOURCES_SDWL_SDWRMCLIB_REGISTRYSERVICEENTRY_CPP


template <class T>
std::string setvalue_to_json(
    uint32_t u32Root,
    const std::string& strKey,
    const std::string& strItemName,
    uint32_t u32ValueType,
    const T& itemValue)
{
    nlohmann::json root = nlohmann::json::object();
    root["function"] = "set_value";
    root["rootkey"] = u32Root;
    root["subkey"] = strKey;
    root["itemname"] = strItemName;
    root["valuetype"] = u32ValueType;
    root["itemvalue"] = itemValue;

    return std::move(root.dump());
}

std::string getvalue_to_json(
    uint32_t u32Root,
    const std::string& strKey,
    const std::string& strItemName,
    uint32_t u32ValueType)
{
    nlohmann::json root = nlohmann::json::object();
    root["function"] = "get_value";
    root["rootkey"] = u32Root;
    root["subkey"] = strKey;
    root["itemname"] = strItemName;
    root["valuetype"] = u32ValueType;

    return std::move(root.dump());
}

template <class T>
int parse_getvalue_response(
    const std::string& strJson,
    uint32_t u32ValueType,
    T& itemValue)
{
    nlohmann::json root = nlohmann::json::parse(strJson);
    if (!root.is_object())
    {
        return -1;
    }

    if (u32ValueType == root.at("valuetype").get<uint32_t>())
    {
        itemValue = root.at("itemvalue").get<T>();
        return 0;
    }

    return 1;
}

std::string deletekey_to_json(uint32_t u32Root, const std::string& strKey)
{
    nlohmann::json root = nlohmann::json::object();
    root["function"] = "delete_key";
    root["rootkey"] = u32Root;
    root["subkey"] = strKey;

    return std::move(root.dump());
}

CRegistryServiceEntry::CRegistryServiceEntry(ISDRmcInstance* pInstance, std::string strSecurity)
    : m_pInstance(pInstance)
    , m_strSecurityCode(strSecurity)
{

}

CRegistryServiceEntry::~CRegistryServiceEntry()
{

}

SDWLResult CRegistryServiceEntry::get_value(
    HKEY hRoot,
    const std::string& strKey,
    const std::string& strItemName,
    uint32_t& u32ItemValue)
{
    u32ItemValue = 0;
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_DWORD;

    std::string str = getvalue_to_json(u32Root, strKey, strItemName, u32ValueType);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 != res.GetCode())
    {
        return res;
    }

    try
    {
        std::string strJson = res.GetMsg();
        int nRet = parse_getvalue_response<uint32_t>(strJson, u32ValueType, u32ItemValue);
        if (0 == nRet)
        {
            return res;
        }
        else if (-1 == nRet)
        {
            std::wstring strException = NX::conv::utf8toutf16(strJson);
            CELOG_LOG(CELOG_ERROR, L"json parse error , return json=%s\n", strException.c_str());
            return RESULT2(SDWL_INVALID_JSON_FORMAT, strJson.c_str());
        }
    }
    catch (const std::exception& e)
    {
        std::wstring strException = NX::conv::utf8toutf16(e.what());
        CELOG_LOG(CELOG_ERROR, L"exception=%s\n", strException.c_str());
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::get_value(
    HKEY hRoot,
    const std::string& strKey,
    const std::string& strItemName,
    uint64_t& u64ItemValue)
{
    u64ItemValue = 0;
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_QWORD;

    std::string str = getvalue_to_json(u32Root, strKey, strItemName, u32ValueType);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 != res.GetCode())
    {
        return res;
    }

    try
    {
        std::string strJson = res.GetMsg();
        int nRet = parse_getvalue_response<uint64_t>(strJson, u32ValueType, u64ItemValue);
        if (0 == nRet)
        {
            return res;
        }
        else if (-1 == nRet)
        {
            std::wstring strException = NX::conv::utf8toutf16(strJson);
            CELOG_LOG(CELOG_ERROR, L"json parse error , return json=%s\n", strException.c_str());
            return RESULT2(SDWL_INVALID_JSON_FORMAT, strJson.c_str());
        }
    }
    catch (const std::exception& e)
    {
        std::wstring strException = NX::conv::utf8toutf16(e.what());
        CELOG_LOG(CELOG_ERROR, L"exception=%s\n", strException.c_str());
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::get_value(
    HKEY hRoot,
    const std::string& strKey,
    const std::string& strItemName,
    std::string& strItemValue)
{
    strItemValue = "";
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_SZ;

    std::string str = getvalue_to_json(u32Root, strKey, strItemName, u32ValueType);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 != res.GetCode())
    {
        return res;
    }

    try
    {
        std::string strJson = res.GetMsg();
        int nRet = parse_getvalue_response<std::string>(strJson, u32ValueType, strItemValue);
        if (0 == nRet)
        {
            return res;
        }
        else if (-1 == nRet)
        {
            std::wstring strException = NX::conv::utf8toutf16(strJson);
            CELOG_LOG(CELOG_ERROR, L"json parse error , return json=%s\n", strException.c_str());
            return RESULT2(SDWL_INVALID_JSON_FORMAT, strJson.c_str());
        }
    }
    catch (const std::exception& e)
    {
        std::wstring strException = NX::conv::utf8toutf16(e.what());
        CELOG_LOG(CELOG_ERROR, L"exception=%s\n", strException.c_str());
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::get_value(
    HKEY hRoot,
    const std::wstring& strKey,
    const std::wstring& strItemName,
    uint32_t& u32ItemValue)
{
    u32ItemValue = 0;
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_DWORD;
    std::string utf8Key = NX::conv::utf16toutf8(strKey);
    std::string utf8Name = NX::conv::utf16toutf8(strItemName);

    std::string str = getvalue_to_json(u32Root, utf8Key, utf8Name, u32ValueType);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 != res.GetCode())
    {
        return res;
    }

    try
    {
        std::string strJson = res.GetMsg();
        int nRet = parse_getvalue_response<uint32_t>(strJson, u32ValueType, u32ItemValue);
        if (0 == nRet)
        {
            return res;
        }
        else if (-1 == nRet)
        {
            std::wstring strException = NX::conv::utf8toutf16(strJson);
            CELOG_LOG(CELOG_ERROR, L"json parse error , return json=%s\n", strException.c_str());
            return RESULT2(SDWL_INVALID_JSON_FORMAT, strJson.c_str());
        }
    }
    catch (const std::exception& e)
    {
        std::wstring strException = NX::conv::utf8toutf16(e.what());
        CELOG_LOG(CELOG_ERROR, L"exception=%s\n", strException.c_str());
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::get_value(
    HKEY hRoot,
    const std::wstring& strKey,
    const std::wstring& strItemName,
    uint64_t& u64ItemValue)
{
    u64ItemValue = 0;
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_QWORD;
    std::string utf8Key = NX::conv::utf16toutf8(strKey);
    std::string utf8Name = NX::conv::utf16toutf8(strItemName);

    std::string str = getvalue_to_json(u32Root, utf8Key, utf8Name, u32ValueType);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 != res.GetCode())
    {
        return res;
    }

    try
    {
        std::string strJson = res.GetMsg();
        int nRet = parse_getvalue_response<uint64_t>(strJson, u32ValueType, u64ItemValue);
        if (0 == nRet)
        {
            return res;
        }
        else if (-1 == nRet)
        {
            std::wstring strException = NX::conv::utf8toutf16(strJson);
            CELOG_LOG(CELOG_ERROR, L"json parse error , return json=%s\n", strException.c_str());
            return RESULT2(SDWL_INVALID_JSON_FORMAT, strJson.c_str());
        }
    }
    catch (const std::exception& e)
    {
        std::wstring strException = NX::conv::utf8toutf16(e.what());
        CELOG_LOG(CELOG_ERROR, L"exception=%s\n", strException.c_str());
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::get_value(
    HKEY hRoot,
    const std::wstring& strKey,
    const std::wstring& strItemName,
    std::wstring& strItemValue)
{
    strItemValue = L"";
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_SZ;
    std::string utf8Key = NX::conv::utf16toutf8(strKey);
    std::string utf8Name = NX::conv::utf16toutf8(strItemName);

    std::string str = getvalue_to_json(u32Root, utf8Key, utf8Name, u32ValueType);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 != res.GetCode())
    {
        return res;
    }

    try
    {
        std::string strJson = res.GetMsg();
        std::string strValue;
        int nRet = parse_getvalue_response<std::string>(strJson, u32ValueType, strValue);
        if (0 == nRet)
        {
            strItemValue = NX::conv::utf8toutf16(strValue);
            return res;
        }
        else if (-1 == nRet)
        {
            std::wstring strException = NX::conv::utf8toutf16(strJson);
            CELOG_LOG(CELOG_ERROR, L"json parse error , return json=%s\n", strException.c_str());
            return RESULT2(SDWL_INVALID_JSON_FORMAT, strJson.c_str());
        }
    }
    catch (const std::exception& e)
    {
        std::wstring strException = NX::conv::utf8toutf16(e.what());
        CELOG_LOG(CELOG_ERROR, L"exception=%s\n", strException.c_str());
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::set_value(
    HKEY hRoot,
    const std::string& strKey,
    const std::string& strItemName,
    uint32_t u32ItemValue)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_DWORD;

    std::string str = setvalue_to_json<uint32_t>(u32Root, strKey, strItemName, u32ValueType, u32ValueType);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::set_value(
    HKEY hRoot,
    const std::string& strKey,
    const std::string& strItemName,
    uint64_t u64ItemValue)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_QWORD;

    std::string str = setvalue_to_json<uint64_t>(u32Root, strKey, strItemName, u32ValueType, u64ItemValue);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::set_value(
    HKEY hRoot,
    const std::string& strKey,
    const std::string& strItemName,
    const std::string& strItemValue,
    bool expandable)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_SZ;
    if (expandable)
    {
        u32ValueType = REG_EXPAND_SZ;
    }

    std::string str = setvalue_to_json<std::string>(u32Root, strKey, strItemName, u32ValueType, strItemValue);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::set_value(
    HKEY hRoot,
    const std::wstring& strKey,
    const std::wstring& strItemName,
    uint32_t u32ItemValue)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_DWORD;
    std::string utf8Key = NX::conv::utf16toutf8(strKey);
    std::string utf8Name = NX::conv::utf16toutf8(strItemName);
    std::string str = setvalue_to_json<uint32_t>(u32Root, utf8Key, utf8Name, u32ValueType, u32ItemValue);

    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::set_value(
    HKEY hRoot,
    const std::wstring& strKey,
    const std::wstring& strItemName,
    uint64_t u64ItemValue)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_QWORD;
    std::string utf8Key = NX::conv::utf16toutf8(strKey);
    std::string utf8Name = NX::conv::utf16toutf8(strItemName);
    std::string str = setvalue_to_json<uint64_t>(u32Root, utf8Key, utf8Name, u32ValueType, u64ItemValue);

    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::set_value(
    HKEY hRoot,
    const std::wstring& strKey,
    const std::wstring& strItemName,
    const std::wstring& strItemValue,
    bool expandable)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    uint32_t u32Root = (uint32_t)hRoot;
    uint32_t u32ValueType = REG_SZ;
    if (expandable)
    {
        u32ValueType = REG_EXPAND_SZ;
    }

    std::string utf8Key = NX::conv::utf16toutf8(strKey);
    std::string utf8Name = NX::conv::utf16toutf8(strItemName);
    std::string utf8Value = NX::conv::utf16toutf8(strItemValue);
    std::string str = setvalue_to_json<std::string>(u32Root, utf8Key, utf8Name, u32ValueType, utf8Value);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::delete_key(HKEY hRoot, const std::string& strKey)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    std::string str = deletekey_to_json((uint32_t)hRoot, strKey);
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

SDWLResult CRegistryServiceEntry::delete_key(HKEY hRoot, const std::wstring& strKey)
{
    if (!m_pInstance)
    {
        CELOG_LOG(CELOG_INFO, L"m_pInstance is NULL");
        return RESULT2(SDWL_INVALID_DATA, "m_pInstance is NULL");
    }

    std::string str = deletekey_to_json((uint32_t)hRoot, NX::conv::utf16toutf8(strKey));
    SDWLResult res = m_pInstance->RPMRequestRunRegCmd(str, m_strSecurityCode);
    if (0 == res.GetCode())
    {
        return res;
    }

    CELOG_LOG(CELOG_INFO, L"code=%d msg:%s", res.GetCode(), res.GetMsg().c_str());
    return res;
}

#pragma warning( pop )