#include "stdafx.h"
#include "Shlobj.h"
#include "common/celog2/celog.h"
#include "SDRmcInstance.h"
#include "SDRmUser.h"
#include "SDRmNXLFile.h"
#include "Network/http_client.h"
#include "Winutil/securefile.h"
#include "Shlwapi.h"
#include <nudf\winutil.hpp>

#include "rmccore\network\httpClient.h"
#include "rmccore/utils/time.h"
#include "rmccore/crypto/sha.h"
#include "SDRmNXLFile.h"
#include <ios>
#include <iostream>
#include <unordered_map>
#include <shellapi.h>
#include <experimental/filesystem>

#include <Winsock2.h>
#include <WS2tcpip.h>
#include <WSPiApi.h>
#include <Psapi.h>
#include "nlohmann/json.hpp"


#define CELOG_CUR_MODULE "rmdsdk"
#define CELOG_CUR_FILE CELOG_FILEPATH_SOURCES_SDWL_SDWRMCLIB_SDRMUSER_CPP

using namespace SkyDRM;
using namespace RMCCORE;
using namespace NX;
using namespace NX::REST::http;

#define USERIDP_ID_KEY_NAME         "idp_unique_id"
#define USERDATA_KEY_NAME         "UserData"
#define TOKENPOOL_KEY_NAME        "TokenPool"
#define TOKENPOOLS_KEY_NAME        "TokenPools"
#define USER_PASSCODE_NAME		  "Passcode"
#define FILE_INFO_SECTION         L"NXLFILE"
#define ADHOC_ENABLED			  "ADHOC_ENABLED"
#define WORKSPACE_ENABLED			  "WORKSPACE_ENABLED"
#define FILE_INFO_NAME            L"nxlfile.ini"
#define TOKE_EXPIRED_DAY_FILE     L"expire.token"
#define ACTIVITY_LOG_FILE         L"activity.log"
#define POLICY_FILE               L"policy.txt"
#define HEARTBEAT_FILE            L"heartbeat.txt"
#define TOKE_UNUSE_FILE           L"unuse.token"
#define ATTRIB_FILE               L"Attribute.txt"

typedef enum _RECLASSIFY_TYPE {
	CLASSIFY_NEW_FILE = 0,
	RECLASSIFY_TENANT_FILE,
	RECLASSIFY_PROJECT_FILE
} RECLASSIFY_TYPE;

typedef enum _RECLASSIFY_WHEN {
	RECLASSIFY_LATER = 0,
	RECLASSIFY_NOW
} RECLASSIFY_WHEN;

// Convert string of chars to its representative string of hex numbers
void stream2hex(const std::string str, std::string& hexstr)
{
	hexstr.resize(str.size() * 2);
	char* buf = &hexstr[0];

	int npos = 0;
	byte b;
	for (size_t i = 0; i < str.size(); i++)
	{
		byte bt = str[i];
		b = str[i] >> 4;
		b = b & 0xF;
		hexstr[i*2] = (char)(55 + b + (((b - 10) >> 31)&-7));
		b = str[i] & 0xF;
		hexstr[i * 2 + 1] = (char)(55 + b + (((b - 10) >> 31)&-7));
	}
}

// Convert string of hex numbers to its equivalent char-stream
void hex2stream(const std::string hexstr, std::string& str)
{
	str.resize((hexstr.size() + 1) / 2);

	for (size_t i = 0, j = 0; i < str.size(); i++, j++)
	{
		str[i] = (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) << 4, j++;
		str[i] |= (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) & 0xF;
	}
}

HRESULT create_key_with_default_value(
	const HKEY	root,
	const WCHAR *parent,
	const WCHAR *key,
	const WCHAR *default_value)
{
	HRESULT nRet = S_OK;

	HKEY hParent = NULL;
	HKEY hKey = NULL;

	do
	{
		LONG lRet;

		if (ERROR_SUCCESS != (lRet = RegOpenKeyExW(root,
			parent,
			0,
			KEY_WRITE,
			&hParent)))
		{
			nRet = (lRet == ERROR_ACCESS_DENIED ? E_ACCESSDENIED : E_UNEXPECTED);
			break;
		}

		if (ERROR_SUCCESS != (lRet = RegCreateKey(hParent,
			key,
			&hKey)))
		{
			nRet = (lRet == ERROR_ACCESS_DENIED ? E_ACCESSDENIED : E_UNEXPECTED);
			break;
		}

		if (!default_value)
		{
			break;
		}

		if (ERROR_SUCCESS != (lRet = RegSetValueExW(hKey,
			NULL,
			0,
			REG_SZ,
			(const BYTE*)default_value,
			(DWORD)(wcslen(default_value) + 1) * sizeof(WCHAR))))
		{
			nRet = (lRet == ERROR_ACCESS_DENIED ? E_ACCESSDENIED : E_UNEXPECTED);
			break;
		}

	} while (FALSE);

	if (hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	if (hParent)
	{
		RegCloseKey(hParent);
		hParent = NULL;
	}

	return nRet;
}

HRESULT set_value_content(
	const HKEY  root,
	const WCHAR *key,
	const WCHAR *valuename,
	const WCHAR *content)
{
	HRESULT nRet = S_OK;

	HKEY hKey = NULL;

	do
	{
		if (ERROR_SUCCESS != RegOpenKeyExW(root,
			key,
			0,
			KEY_SET_VALUE,
			&hKey))
		{
			nRet = E_UNEXPECTED;
			break;
		}

		if (ERROR_SUCCESS != RegSetValueExW(hKey,
			valuename,
			0,
			REG_SZ,
			(const BYTE*)content,
			(DWORD)(wcslen(content) + 1) * sizeof(WCHAR)))
		{
			nRet = E_UNEXPECTED;
			break;
		}

	} while (FALSE);

	if (hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}

HRESULT delete_key(const HKEY root, const WCHAR *parent, const WCHAR *key)
{
	HRESULT nRet = S_OK;
	HKEY hKey = NULL;

	do
	{
		LONG lRet;

		if (ERROR_SUCCESS != (lRet = RegOpenKeyExW(root,
			parent,
			0,
			DELETE,
			&hKey)))
		{
			nRet = (lRet == ERROR_FILE_NOT_FOUND || lRet == ERROR_ACCESS_DENIED ? HRESULT_FROM_WIN32(lRet) : E_UNEXPECTED);
			break;
		}

		if (ERROR_SUCCESS != (lRet = RegDeleteKeyW(hKey, key)))
		{
			nRet = (lRet == ERROR_FILE_NOT_FOUND || lRet == ERROR_ACCESS_DENIED ? HRESULT_FROM_WIN32(lRet) : E_UNEXPECTED);
			break;
		}

	} while (FALSE);

	if (hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}

HRESULT delete_value(const HKEY root, const WCHAR *parent, const WCHAR *key)
{
	HRESULT nRet = S_OK;
	HKEY hKey = NULL;

	do
	{
		LONG lRet;

		if (ERROR_SUCCESS != (lRet = RegOpenKeyExW(root,
			parent,
			0,
			KEY_ALL_ACCESS,
			&hKey)))
		{
			nRet = (lRet == ERROR_FILE_NOT_FOUND || lRet == ERROR_ACCESS_DENIED ? HRESULT_FROM_WIN32(lRet) : E_UNEXPECTED);
			break;
		}

		if (ERROR_SUCCESS != (lRet = RegDeleteValueW(hKey, key)))
		{
			nRet = (lRet == ERROR_FILE_NOT_FOUND || lRet == ERROR_ACCESS_DENIED ? HRESULT_FROM_WIN32(lRet) : E_UNEXPECTED);
			break;
		}

	} while (FALSE);

	if (hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}


CSDRmUser::CSDRmUser() : ISDRmUser(), RMUser(), m_passcode("")
{
	::InitializeCriticalSection(&_listproject_lock);
	::InitializeCriticalSection(&_listprojectfiles_lock);
	::InitializeCriticalSection(&_downloadprojectfile_lock);
	m_SDRmcInstance = NULL;
	m_adhoc = true;
	m_workspace = true;
	m_systemProjectId = 0;
	m_systemProjectTenantId = "";
	m_systemProjectTenant = "";
	m_heartbeatFrequency = 60;
}


CSDRmUser::~CSDRmUser()
{
	::DeleteCriticalSection(&_listproject_lock);
	::DeleteCriticalSection(&_listprojectfiles_lock);
	::DeleteCriticalSection(&_downloadprojectfile_lock);
}

SDWLResult CSDRmUser::ImportDataFromJson(const nlohmann::json& root)
{
    SDWLResult res = RESULT(0);
    try {
        const nlohmann::json& userData = root.at(USERDATA_KEY_NAME);
        ImportFromJson(userData);

        m_passcode = root.at(USER_PASSCODE_NAME).get<std::string>();
        m_adhoc = root.at(ADHOC_ENABLED).get<bool>();
		m_workspace= root.at(WORKSPACE_ENABLED).get<bool>();
        m_heartbeatFrequency = root.at("HEARTBEAT_FREQUENCY").get<int>();
        m_systemProjectId = root.at("SYSTEM_PROJECTID").get<int>();
        m_systemProjectTenantId = root.at("SYSTEM_DEFAULT_PROJECT_TENANTID").get<std::string>();
        m_systemProjectTenant = root.at("SYSTEM_DEFAULT_PROJECT_TENANT").get<std::string>();

        m_TenantAdminList.clear();
        if (root.end() != root.find("TENANT_ADMINS"))
        {
            for (auto it = root["TENANT_ADMINS"].begin(); root["TENANT_ADMINS"].end() != it; it++)
            {
                std::string email = (*it).at("EMAIL").get<std::string>();
                m_TenantAdminList.push_back(email);
            }
        }

        m_ProjectAdminList.clear();
        if (root.end() != root.find("PROJECTS"))
        {
            const nlohmann::json& projects = root["PROJECTS"];
            for (auto it = projects.begin(); projects.end() != it; it++)
            {
                std::vector<std::string> vecAdmins;
                const nlohmann::json& admins = (*it).at("PROJECT_ADMINS");
                for (auto j = admins.begin(); admins.end() != j; j++)
                {
                    std::string email = (*j).at("EMAIL").get<std::string>();
                    vecAdmins.push_back(email);
                }

                std::pair<std::string, std::vector<std::string>> tadmins;
                tadmins.first = (*it).at("PROJECT_TENANTID").get<std::string>();
                tadmins.second = vecAdmins;
                m_ProjectAdminList.push_back(tadmins);
            }
        }

        m_nxlmetadatalist.clear();
        if (root.end() != root.find("NXLFILE_METADATA"))
        {
            const nlohmann::json& metaData = root["NXLFILE_METADATA"];
            for (auto it = metaData.begin(); metaData.end() != it; it++)
            {
                SDR_FILE_METADATA metadata;

                metadata.m_duid = (*it).at("DUID").get<std::string>();
                metadata.m_otp = (*it).at("OTP").get<std::string>();
                metadata.m_policy = (*it).at("POLICY").get<std::string>();
                metadata.m_tags = (*it).at("TAGS").get<std::string>();

                m_nxlmetadatalist[metadata.m_duid] = metadata;
            }
        }
    }
    catch (std::exception &e) {
        std::string strError = "JSON User Data is not correct : " + std::string(e.what());
        res = RESULT2(SDWL_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(SDWL_INVALID_DATA, "JSON User Data is not correct");
    }

    return res;
}

nlohmann::json CSDRmUser::ExportDataToJson()
{
    nlohmann::json root = nlohmann::json::object();
    try {
        root[USERDATA_KEY_NAME] = ExportToJson();

        root[USER_PASSCODE_NAME] = m_passcode;
        root[ADHOC_ENABLED] = m_adhoc;
		root[WORKSPACE_ENABLED] = m_workspace;
        root["HEARTBEAT_FREQUENCY"] = m_heartbeatFrequency;
        root["SYSTEM_PROJECTID"] = m_systemProjectId;
        root["SYSTEM_DEFAULT_PROJECT_TENANTID"] = m_systemProjectTenantId;
        root["SYSTEM_DEFAULT_PROJECT_TENANT"] = m_systemProjectTenant;

        root["TENANT_ADMINS"] = nlohmann::json::array();
        for (size_t i = 0; i < m_TenantAdminList.size(); i++)
        {
            nlohmann::json item = nlohmann::json::object();
            item["EMAIL"] = m_TenantAdminList[i];
            root["TENANT_ADMINS"].push_back(item);
        }

        root["PROJECTS"] = nlohmann::json::array();
        for (size_t i = 0; i < m_ProjectAdminList.size(); i++)
        {
            nlohmann::json project = nlohmann::json::object();
            project["PROJECT_TENANTID"] = m_ProjectAdminList[i].first;

            project["PROJECT_ADMINS"] = nlohmann::json::array();
            for (size_t j = 0; j < m_ProjectAdminList[i].second.size(); j++)
            {
                nlohmann::json item = nlohmann::json::object();
                item["EMAIL"] = m_ProjectAdminList[i].second[j];
                project["PROJECT_ADMINS"].push_back(item);
            }

            root["PROJECTS"].push_back(project);
        }

        root["NXLFILE_METADATA"] = nlohmann::json::array();
        for (auto const &metaitem : m_nxlmetadatalist)
        {
            nlohmann::json item = nlohmann::json::object();
            item["DUID"] = metaitem.second.m_duid;
            item["OTP"] = metaitem.second.m_otp;
            item["POLICY"] = metaitem.second.m_policy;
            item["TAGS"] = metaitem.second.m_tags;

            root["NXLFILE_METADATA"].push_back(item);
        }
    }
    catch (std::exception &e) {
        std::string strError = "Export User Data to Json error : " + std::string(e.what());
        throw RETMESSAGE(RMCCORE_ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        throw RESULT2(SDWL_INVALID_DATA, "Export User Data to Json error");
    }

    return root;
}

SDWLResult CSDRmUser::UpdateUserInfo()
{
	std::string jsonreturn;
	return SyncUserAttributes(jsonreturn);
}

SDWLResult CSDRmUser::SyncUserAttributes(std::string& jsonreturn)
{
    CELOG_ENTER;
    SDWLResult res;

    try {
		Client restclient(NXRMC_CLIENT_NAME, true);
		restclient.Open();
		std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

		RMCCORE::HTTPRequest httpreq = GetProfileQuery();
		StringBodyRequest request(httpreq);
		StringResponse response;

		res = spConn->SendRequest(request, response);

		if (res)
		{
			RMCCORE::RetValue ret = ImportFromRMSResponse(jsonreturn);
			//
			// for API user, the query profile will fail in SDK level, this shall not block the following membership query
			//
			//if (!ret)
			//    CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
		}
		jsonreturn = response.GetBody();
		CELOG_LOGA(CELOG_DEBUG, "SyncUserAttributes: response= %s\n", jsonreturn.c_str());
		
		RMCertificate   certificate = GetDefaultMembership().m_certificate;
        std::map<std::string, RMCertificate> certs;
        for (RMCCORE::RMMembership& member : m_memberships)
            certs[member.GetID()] = member.m_certificate;

		if (certificate.GetAgreement0().size() == 0)
		{
			RMCCORE::RMMembership member = GetDefaultMembership();
			CSDRmTenant tenant(m_tenant);
			res = GetCertificate(tenant, &member);
			if (res)
				GetDefaultMembership().m_certificate = member.m_certificate;
		}

        // User Profile is udpated. We also need to update the membership info with agreements
        for (RMCCORE::RMMembership& member : m_memberships)
        {
            RMCertificate cert = certs[member.GetID()];
            //
            // bug #51356, when there is a new project created/invited, we get the project tenant, but there is no membership locally, we shall sync membership from RMS
            //
            if (cert.GetAgreement0().size() == 0)
            {
                CSDRmTenant tenant(m_tenant);
                res = GetCertificate(tenant, &member);
            }
            else
                member.m_certificate = cert;
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "Profile JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetMyDriveInfo(uint64_t& usage, uint64_t& totalquota, uint64_t& vaultusage, uint64_t& vaultquota)
{
	RMMyDrive* mydrive = GetMyDrive();
	if (!mydrive)
		return RESULT2(SDWL_INVALID_DATA, "Invalid mydrive, user not login ");

	usage = mydrive->GetUsage();
	totalquota = mydrive->GetQuota();
	vaultusage = mydrive->GetVaultUsage();
	vaultquota = mydrive->GetVaultQuota();

	return RESULT(0);
}

SDWLResult CSDRmUser::UpdateMyDriveInfo()
{
    CELOG_ENTER;
    RMMyDrive* mydrive = GetMyDrive();
    if (!mydrive)
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid mydrive, user not login "));

    HTTPRequest	httpreq = mydrive->GetStorageQuery();

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    SDWLResult res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "UpdateMyDriveInfo: response= %s\n", jsonreturn.c_str());

    RMCCORE::RetValue ret = mydrive->ImportFromRMSResponse(jsonreturn);
    if (!ret)
        CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

    CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult CSDRmUser::SetSystemParameters(const RMCCORE::RMSystemPara & param)
{
	RMCCORE::RetValue ret = UpdateSystemParameters(param);

	if (!ret) {
		return RESULT2(SDWL_INVALID_DATA, "Invalid system parameters.");
	}
	return RESULT(0);
}

SDWLResult CSDRmUser::SetSDKLibFolder(const std::wstring & path)
{
	CELOG_ENTER;

	// Do nothing for now.
	CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult CSDRmUser::IsInitFinished(bool& finished)
{
	CELOG_ENTER;

	CELOG_RETURN_VAL_T(m_PDP.IsReadyForEval(finished));
}

SDWLResult CSDRmUser::SetTenant(const RMCCORE::RMTenant & tenant)
{
	RMCCORE::RetValue ret = UpdateTenant(tenant);
	if (!ret) {
		return RESULT2(SDWL_INVALID_DATA, "Invalid tenant.");
	}
	return RESULT(0);
}

SDWLResult CSDRmUser::InitializeLogin(CSDRmTenant & info)
{
	CELOG_ENTER;

	SDWLResult res;
	m_tenant = info;

	res = GetCertificate(info, &GetDefaultMembership());
	if (!res)
		CELOG_RETURN_VAL_T(res);

	for (RMCCORE::RMMembership& member : m_memberships)
	{
		RMMembership* rm = FindMembershipFromID(member.GetID());
		res = GetCertificate(info, rm);
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetCertificate(CSDRmTenant& info, RMCCORE::RMMembership* rm)
{
    CELOG_ENTER;

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    // After login, the request shall be sent to RMS Server, instead of Router.
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(info.GetRMSURL()));

    RMCCORE::HTTPRequest httpreq = GetMembershipQuery(*rm);
    StringBodyRequest certrequest(httpreq);
    StringResponse certresponse;

    SDWLResult res = spConn->SendRequest(certrequest, certresponse);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsondata = certresponse.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetCertificate: response= =%s\n", jsondata.c_str());

    try {
        RMCCORE::RetValue ret = rm->m_certificate.ImportFromRMSResponse(jsondata);
        if (!ret)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "Certificate JSON response is not correct");
        CELOG_RETURN_VAL_T(res);
    }

    CELOG_RETURN_VAL_T(res);
}

bool CSDRmUser::IsUserLogin()
{ 
	bool ret = IsLogin();
	if (!ret)
		return ret;

	uint64_t time = GetExpiredTime();
	ret = (time > 0) ? true : false;
	return ret;
}

SDWLResult CSDRmUser::SetLoginResult(const std::string& strJson)
{
    RMCCORE::RetValue ret = ImportFromRMSResponse(strJson);
	bool bret = IsLogin();
	if (bret && ret.GetCode() == 0)
	{
		GeneratePassCode();  // login need regenerate passcode
		return RESULT(0);
	}
	return RESULT2(ret.GetCode(), ret.GetMessage());
}

void CSDRmUser::SetRWFile(SDRSecureFile& file, const std::wstring &workFolder)
{ 
	m_File = file; 
	m_localFile.SetWorkFolder(workFolder);
}

SDWLResult CSDRmUser::IsMembershipExist(const std::string& ID, RMCCORE::RMMembership& rm)
{
	SDWLResult res = RESULT(0);
	bool found = false;
	if (GetDefaultMembership().GetID().compare(ID) == 0)
	{
		rm = GetDefaultMembership();
		found = true;
	}

	if (!found && m_memberships.size())
	{
		for (auto member : m_memberships)
		{
			if (member.GetID().compare(ID) == 0)
			{
				rm = member;
				found = true;
				break;
			}
		}
	}

	if (!found)
		return RESULT2(SDWL_NOT_FOUND, "Invalid membership ID");

	return res;
}


RMToken CSDRmUser::QueryTokenFromRMS(const std::string membershipid, int protectionType, const std::string &fileTagsOrPolicy)
{
    std::string _membershipid = membershipid;
    if (_membershipid == "")
        _membershipid = (GetDefaultMembership()).GetID();

    SDWLResult res;
    RMMembership rm;

    RMToken token;
    res = IsMembershipExist(_membershipid, rm);
    if (res.GetCode() != 0)
        return token;

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    RMCCORE::HTTPRequest httpreq = GetMembershipTokenQuery(rm, protectionType, fileTagsOrPolicy);
    StringBodyRequest tokenrequest(httpreq);
    StringResponse tokenresponse;

    res = spConn->SendRequest(tokenrequest, tokenresponse);

    if (!res)
        return token;

    const std::string& jsonreturn = tokenresponse.GetBody();
    RMTokenPool	tokenPool;

    try {
        RMCCORE::RetValue ret = tokenPool.ImportFromRMSResponse(jsonreturn);
        if (!ret)
            return token;

        token = tokenPool.pop();
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "Token JSON response is not correct");
    }

    return token;
}

RMToken CSDRmUser::PopToken(const std::string membershipid)
{
	std::string _membershipid = membershipid;
	if (_membershipid == "")
		_membershipid = GetDefaultMembership().GetID();

	RMToken token;
	if (m_SDRmcInstance)
		m_SDRmcInstance->RPMPopupNewToken(NX::conv::utf8toutf16(_membershipid), token);

	return token;
}


SDWLResult CSDRmUser::Save()
{
	SDWLResult res;

	res = SaveMisc();

	return res;
}

SDWLResult CSDRmUser::SaveMisc()
{
	SDWLResult res;
	std::string str;

	try {
		res = m_File.FileWrite(POLICY_FILE, m_policyBundle);
	}
	catch (...) {
		res = RESULT2(ERROR_INVALID_DATA, "fail to export policy data to file");
	}

	try {
		//SaveAttribs();
		str = m_heartbeat.ExportToString();
		res = m_File.FileWrite(HEARTBEAT_FILE, str);
	}
	catch (...) {
		return RESULT2(ERROR_INVALID_DATA, "fail to export heartbeat data to file");
	}

	return res;
}

SDWLResult CSDRmUser::SaveAttribs()
{
    SDWLResult res;
    std::string attrs;

    try {
        for (size_t i = 0; i < m_attributes.size(); i++)
        {
            attrs += m_attributes[i].first + "," + m_attributes[i].second;
            if (i != m_attributes.size() - 1)
                attrs += ",";
        }

        res = m_File.FileWrite(ATTRIB_FILE, attrs);
    }
    catch (...) {
        return RESULT2(ERROR_INVALID_DATA, "fail to write attributes data to file");
    }

    return res;
}

SDWLResult CSDRmUser::FileRead(const std::wstring& file, std::string& s)
{
	return m_File.FileRead(file, s);
}

SDWLResult CSDRmUser::FileWrite(const std::wstring& file, std::string& s)
{
	return m_File.FileWrite(file, s);
}

std::wstring CSDRmUser::GetTargetFilePath(const std::wstring filename, bool usetimestamp)
{
	if (!usetimestamp)
	{
		if (NX::iend_with<wchar_t>(filename, L".nxl"))
		{
			return filename;
		}
		else
		{
			return filename + L".nxl";
		}
	}

	SYSTEMTIME st = { 0 };

	std::wstring sret = filename;

	GetSystemTime(&st);
	// -YYYY-MM-DD-HH-MM-SS
	const std::wstring sTimestamp(NX::FormatString(L"-%04d-%02d-%02d-%02d-%02d-%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond));

	//if (!NX::iend_with<wchar_t>(sret, L".nxl")) 
	{

		std::wstring::size_type posSuffix = sret.rfind(L'.');
		if (posSuffix == std::wstring::npos) {
			sret += sTimestamp;
		}
		else {
			const std::wstring suffix(sret.substr(posSuffix));
			sret = sret.substr(0, posSuffix);
			sret += sTimestamp;
			sret += suffix;
		}
		sret += L".nxl";
	}

	return sret;
}


SDWLResult CSDRmUser::ProtectFile(const std::wstring &filepath, std::wstring& newcreatedfilePath,
	const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags, const std::string& memberid, bool usetimestamp)
{
	CELOG_ENTER;

	if (tags.length() > NXL_PAGE_SIZE) {
		char pageSize[128];
		sprintf_s(pageSize, "The max length of tag string is %d", NXL_PAGE_SIZE);
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, pageSize));
	}

	std::wstring _filepath = NX::fs::dos_fullfilepath(filepath, false).global_dos_path();

	RMLocalFile path(_filepath);
	if (!path.GetLastError()) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, path.GetLastError().GetMessage()));
	}

    std::string ttags;
    if (!IsFileTagValid(tags, ttags))
    {
        std::string message = "Invalid Json format : " + tags;
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, message));
    }

	std::wstring  targetName = GetTargetFilePath(conv::to_wstring(path.GetFileName()), usetimestamp);
	path.Close();
	if (targetName.length() == 0) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "fail to generate nxl file"));
	}

	std::wstring target;
	if (newcreatedfilePath.empty()) {
		target = m_localFile.GetWorkFolder() + L"\\" + targetName;
	}
	else if (NX::fs::is_dos_drive_only_path(newcreatedfilePath)) { // e.g. C:
		DWORD attr = GetFileAttributesW(newcreatedfilePath.c_str());
		if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
			target = newcreatedfilePath + L"\\" + targetName;
		}
		else {
			CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "The dest folder is invalid."));
		}
	}
	else {
		NX::fs::dos_fullfilepath _new_filepath(newcreatedfilePath);
		std::wstring _newcreatedfilePath = _new_filepath.path();

		
		if (_newcreatedfilePath.size() > 0)
		{
			/*if (PathIsDirectoryW(_new_filepath.path().c_str())) {
				target = std::experimental::filesystem::path(_newcreatedfilePath) / std::experimental::filesystem::path(targetName);
			}
			else
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "The dest folder is invalid."));*/

			// fix Bug 65416 - [ProtectFileFrom-LongPath]protectFileFrom API fail to protect file to destFolder with long path (edit)
			// and Bug 65412 - [protect-LongPath] protect API faile to protect file to desk folder with long path (edit)
			DWORD attr = GetFileAttributesW(_new_filepath.global_dos_path().c_str());
			if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
				target = std::experimental::filesystem::path(_newcreatedfilePath) / std::experimental::filesystem::path(targetName);
			}
			else {
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, "The dest folder is invalid."));
			}
		}
		else
			target = m_localFile.GetWorkFolder() + L"\\" + targetName;
	}

	NX::fs::dos_fullfilepath _target_path(target, false);
	target = _target_path.path();
	if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(_target_path.global_dos_path().c_str()))
		CELOG_RETURN_VAL_T(RESULT2(ERROR_ACCESS_DENIED, "target file exists"));

	NXLFMT::Obligations obs;
	NXLFMT::Rights nxlrights = ToFileRight(rights);
	RMToken token;
	if (memberid == "")
		token = PopToken();
	else {
		token = PopToken(memberid);
	}
	if (token.GetKey().size() == 0) {
		// invalid token for some reason
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "fail to get token"));
	}
	NXLAttributes attr;
	RMActivityLog log;
	RMNXLFile nfile(_target_path.global_dos_path());
	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}

	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.obs = &obs;
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = ttags;

	RMCCORE::RetValue ret = ProtectLocalFile(_filepath, _target_path.global_dos_path(), attr, token, log, nfile, memberid);
	if (!ret) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, ret.GetMessage()));
	}

	CSDRmNXLFile pfile(nfile);
	pfile.SetToken(token);

	//Move token to used token list
	AddCachedToken(token);

	// The update will fail when session is expired, we'll see protect operation as failure,
	// and prompt user to login again.(Fix bug 56028).
	auto rt = UpdateNXLMetaData(&pfile);
	if (!rt)
	{
		nfile.Close();
		pfile.Close();
		DeleteFile(_target_path.global_dos_path().c_str());
		CELOG_RETURN_VAL_T(RESULT2(rt.GetCode(), ret.GetMessage()));
	}

	AddActivityLog(log);

	newcreatedfilePath = _target_path.path();

    CELOG_RETURN_VAL_T(RESULT2(0, "OK"));
}

SDWLResult CSDRmUser::ProtectFileFrom(const std::wstring &srcplainfile, const std::wstring& originalnxlfile, std::wstring& output)
{
	CSDRmNXLFile *file = NULL;
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> userRightsAndWatermarks;
	std::vector<SDRmFileRight> rights;
	SDR_WATERMARK_INFO waterMark;
	SDR_Expiration expiration;
	std::string tags;
	std::string creatorid;
	std::string infoext;
	DWORD attributes;
	DWORD isRPMFolder;
	DWORD isNXLFile;
	std::string tenant;
	std::string membershipid;
	std::string duid;

	std::wstring exactOriginalNXL = NX::fs::dos_fullfilepath(originalnxlfile).path();
	SDWLResult res = OpenFile(exactOriginalNXL, (ISDRmNXLFile**)&file);
	if (!res) {
		// can't open this NXL file
		res = RPMEdit_FindMap(exactOriginalNXL, exactOriginalNXL);
		if (res)
		{
			// find it in the map
			res = OpenFile(exactOriginalNXL, (ISDRmNXLFile**)&file);
			if (res)
			{
				tags = file->GetTags();
				waterMark = file->GetWaterMark();
				expiration = file->GetExpiration();
				rights = file->GetRights();
				tenant = file->GetTenantName();
				membershipid = GetMembershipID(tenant);
				CloseFile(file);
			}
		}
		else
		{
			// it might be the file is under RPM folder
			// try to ask service to get file attributes
			res = m_SDRmcInstance->RPMGetFileInfo(originalnxlfile, duid, userRightsAndWatermarks,
				rights, waterMark, expiration, tags,
				tenant, creatorid, infoext, attributes, isRPMFolder, isNXLFile, false);
			if (res)
				membershipid = GetMembershipID(tenant);
		}
	}
	else
	{
		tags = file->GetTags();
		waterMark = file->GetWaterMark();
		expiration = file->GetExpiration();
		rights = file->GetRights();
		tenant = file->GetTenantName();
		membershipid = GetMembershipID(tenant);
		CloseFile(file);
	}

	if (res)
		res = ProtectFile(srcplainfile, output, rights, waterMark, expiration, tags, membershipid);

	return res;
}

BOOL CSDRmUser::IsAPIUser()
{
	static BOOL b_apiuser = false;
	static BOOL b_run = false;
	if (b_run == false)
	{
		b_apiuser = m_SDRmcInstance->RPMIsAPIUser();
		b_run = true;
	}

	return b_apiuser;
}

SDWLResult CSDRmUser::ChangeRightsOfFile(const std::wstring &originalNXLfilePath,
	const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags)
{
	std::wstring exactOriginalNXL = NX::fs::dos_fullfilepath(originalNXLfilePath).global_dos_path();
	//if (exactOriginalNXL.find(m_localFile.GetWorkFolder(), 0) != std::wstring::npos)
	//	return RESULT2(SDWL_ACCESS_DENIED, "you can't change new protected file");

	// check user permission to see whether user has Admin Rights permission
	if (false == HasAdminRights(exactOriginalNXL) && IsAPIUser() == false)
		return RESULT2(SDWL_ACCESS_DENIED, "no permission to change file rights");

    std::string ttags;
    if (!IsFileTagValid(tags, ttags))
    {
        std::string message = "Invalid Json format : " + tags;
        return RESULT2(SDWL_INVALID_DATA, message);
    }
	//
	// Query Token from RMS via RPM Service
	//
	std::string f_tags;
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> f_userRightsAndWatermarks;
	std::vector<SDRmFileRight> f_rights;
	SDR_WATERMARK_INFO f_waterMark;
	SDR_Expiration f_expiration;
	std::string f_creatorid;
	std::string f_infoext;
	DWORD f_attributes;
	DWORD f_isRPMFolder;
	DWORD f_isNXLFile = 0;
	std::string f_tenant;
	std::string f_membershipid;
	std::string f_duid;
	SDWLResult res = m_SDRmcInstance->RPMGetFileInfo(exactOriginalNXL, f_duid, f_userRightsAndWatermarks,
		f_rights, f_waterMark, f_expiration, f_tags,
		f_tenant, f_creatorid, f_infoext, f_attributes, f_isRPMFolder, f_isNXLFile);

	if (!res)
		return res;

	// Now query the token
	RMToken f_token;
	res = FindCachedToken(f_duid, f_token);

	CSDRmNXLFile *file = new CSDRmNXLFile(exactOriginalNXL);
	BOOL bvalid = file->SetToken(f_token);
	if (bvalid == false)
	{
		return RESULT2(SDWL_INTERNAL_ERROR, "Fail to open file");
	}

	std::string orgtags = file->GetTags();
	std::string orgpolicy = file->GetPolicy();

	// get and check Token
	RMToken token = file->GetToken();
	if (token.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	// get a new token for the file
	f_membershipid = GetMembershipIDByTenantName(f_tenant);

	RMActivityLog log;
	NXLAttributes attr;
	NXLFMT::Obligations obs;
	NXLFMT::Rights nxlrights = ToFileRight(rights);

	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}
	attr.obs = &obs;
	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = ttags;

	std::string tenant = file->GetTenantName();
	std::string memberid = GetMembershipIDByTenantName(tenant);

	// close the original NXL file before we want to re-protect and overwrite
	CloseFile(file);

    unsigned char org_header16k[16 * 1024];
    RMLocalFile org_rawfile(exactOriginalNXL);
    org_rawfile.Open();
    uint32_t org_hsize = org_rawfile.read(0, org_header16k, 16 * 1024);
    org_rawfile.Close();
    if (org_hsize != 16 * 1024)
    {
        res = RESULT2(SDWL_INTERNAL_ERROR, "failed to read file header");
        return res;
    }

	RMNXLFile newnxlfile("");
	RetValue value = ReProtectLocalFileHeader(exactOriginalNXL, token, 0, attr, conv::to_wstring(memberid), L"", log, newnxlfile, true, false);
	if (!value)
	{
		return RESULT(value);
	}

    newnxlfile.Close();

    // After change rights, need to tell RMS
    unsigned char header16k[16 * 1024];
    //RMLocalFile rawfile(NX::fs::dos_fullfilepath(exactOriginalNXL).global_dos_path());
    RMLocalFile rawfile(exactOriginalNXL);
    rawfile.Open();
    uint32_t hsize = rawfile.read(0, header16k, 16 * 1024);
    rawfile.Close();
    if (hsize != 16 * 1024)
    {
        res = RESULT2(SDWL_INTERNAL_ERROR, "failed to read file header");
        return res;
    }

    std::string fileheader = NX::conv::Base64Encode((const unsigned char*)header16k, 16 * 1024);
    res = UpdateNXLMetaDataEx(exactOriginalNXL, ttags, orgtags, fileheader);
    if (!res)
    {
        std::fstream out(exactOriginalNXL, std::ios::in | std::ios::binary | std::ios::out);
        if (out.good())
        {
            out.seekg(0, std::ios_base::beg);
            out.write((const char*)org_header16k, 16 * 1024);

            CELOG_LOGA(CELOG_DEBUG, "UpdateNXLMetaDataEx failed and change file header back\n");
        }
        out.close();

        return res;
    }

	return RESULT(0);
}

SDWLResult CSDRmUser::ChangeRightsOfProjectFile(const std::wstring &originalNXLfilePath, unsigned int projectid, const std::string &fileName, const std::string &parentPathId,
	const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags)
{
	if (originalNXLfilePath.find(m_localFile.GetWorkFolder(), 0) != std::wstring::npos)
		return RESULT2(SDWL_ACCESS_DENIED, "you can't change new protected file");

	// check user permission to see whether user has Admin Rights permission
	if (false == HasAdminRights(originalNXLfilePath))
		return RESULT2(SDWL_ACCESS_DENIED, "no permission to change file rights");

	std::string ttags;
	if (!IsFileTagValid(tags, ttags))
	{
		std::string message = "Invalid Json format : " + tags;
		return RESULT2(SDWL_INVALID_DATA, message);
	}

	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(originalNXLfilePath, (ISDRmNXLFile**)&file);
	if (!res) {
		return res;
	}

	std::string orgtags = file->GetTags();
	std::string orgpolicy = file->GetPolicy();

	// get and check Token
	RMToken token = file->GetToken();
	if (token.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	RMActivityLog log;
	NXLAttributes attr;
	NXLFMT::Obligations obs;
	NXLFMT::Rights nxlrights = ToFileRight(rights);

	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}
	attr.obs = &obs;
	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = ttags;

	std::string tenant = file->GetTenantName();
	std::string memberid = GetMembershipIDByTenantName(tenant);

	// close the original NXL file before we want to re-protect and overwrite
	CloseFile(file);

    unsigned char org_header16k[16 * 1024];
    RMLocalFile org_rawfile(originalNXLfilePath);
    org_rawfile.Open();
    uint32_t org_hsize = org_rawfile.read(0, org_header16k, 16 * 1024);
    org_rawfile.Close();
    if (org_hsize != 16 * 1024)
    {
        res = RESULT2(SDWL_INTERNAL_ERROR, "failed to read file header");
        return res;
    }

	RMNXLFile newnxlfile("");
	RetValue value = ReProtectLocalFileHeader(NX::fs::dos_fullfilepath(originalNXLfilePath).global_dos_path(), token, 0, attr, conv::to_wstring(memberid), L"", log, newnxlfile, true, false);
	if (!value)
	{
		return RESULT(value);
	}
	newnxlfile.Close();

	// After change rights, need to tell RMS
	res = ClassifyProjectFile(originalNXLfilePath, projectid, fileName, parentPathId, ttags);
	if (!res)
	{
        std::fstream out(originalNXLfilePath, std::ios::in | std::ios::binary | std::ios::out);
        if (out.good())
        {
            out.seekg(0, std::ios_base::beg);
            out.write((const char*)org_header16k, 16 * 1024);

            CELOG_LOGA(CELOG_DEBUG, "UpdateNXLMetaDataEx failed and change file header back\n");
        }
        out.close();
        
        return res;
	}

	return RESULT(0);
}

SDWLResult CSDRmUser::ReProtectSystemBucketFile(const std::wstring &originalNXLfilePath)
{
	SDWLResult res = RESULT(SDWL_SUCCESS);
	//if (originalNXLfilePath.find(m_localFile.GetWorkFolder(), 0) != std::wstring::npos)
	//	return RESULT2(SDWL_ACCESS_DENIED, "you can't change new protected file");

	CSDRmNXLFile *file = NULL;

	if (IsAPIUser())
	{
		std::string f_tags;
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> f_userRightsAndWatermarks;
		std::vector<SDRmFileRight> f_rights;
		SDR_WATERMARK_INFO f_waterMark;
		SDR_Expiration f_expiration;
		std::string f_creatorid;
		std::string f_infoext;
		DWORD f_attributes;
		DWORD f_isRPMFolder;
		DWORD f_isNXLFile = 0;
		std::string f_tenant;
		std::string f_membershipid;
		std::string f_duid;
		res = m_SDRmcInstance->RPMGetFileInfo(originalNXLfilePath, f_duid, f_userRightsAndWatermarks,
			f_rights, f_waterMark, f_expiration, f_tags,
			f_tenant, f_creatorid, f_infoext, f_attributes, f_isRPMFolder, f_isNXLFile);

		if (!res)
			return res;

		// Now query the token
		RMToken f_token;
		res = FindCachedToken(f_duid, f_token);

		file = new CSDRmNXLFile(originalNXLfilePath);
		BOOL bvalid = file->SetToken(f_token);
		if (bvalid == false)
		{
			CloseFile(file);
			return RESULT2(SDWL_INTERNAL_ERROR, "Fail to open file");
		}
	}
	else
	{
		SDR_NXL_FILE_FINGER_PRINT fp;
		res = GetFingerPrint(originalNXLfilePath, fp);
		if (!res)
			return res;

		if (!(fp.IsByCentrolPolicy && fp.isFromSystemBucket))
		{
			return RESULT2(SDWL_ACCESS_DENIED, "The file must be CentrolPolicy and FromSystemBucket");
		}

		res = OpenFile(originalNXLfilePath, (ISDRmNXLFile**)&file);
		if (!res)
			return res;
	}

	std::string tags = file->GetTags();
	std::string tenant = file->GetTenantName();
	std::vector<SDRmFileRight> rights = file->GetRights();
	SDR_WATERMARK_INFO watermarkinfo = file->GetWaterMark();
	SDR_Expiration expire = file->GetExpiration();
	std::string memberid = GetMembershipIDByTenantName(tenant);

	//get and check Token
	RMToken token;
	if (memberid == "")
		token = PopToken();
	else {
		token = PopToken(memberid);
	}
	if (token.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	RMActivityLog log;
	NXLAttributes attr;
	NXLFMT::Obligations obs;
	NXLFMT::Rights nxlrights = ToFileRight(rights);

	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}

	attr.obs = &obs;
	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = tags;

	// get and check Token
	RMToken oldToken = file->GetToken();
	if (oldToken.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	// close the original NXL file before we want to re-protect and overwrite
	CloseFile(file);

	RMNXLFile newnxlfile("");

	RetValue value = ReProtectLocalFileHeader(NX::fs::dos_fullfilepath(originalNXLfilePath).global_dos_path(), oldToken, &token, attr, conv::to_wstring(memberid), L"", log, newnxlfile, true);
	AddCachedToken(token);

	if (!value)
	{
		return RESULT(value);
	}

	CSDRmNXLFile f(newnxlfile);
	f.SetToken(token);
	AddCachedToken(token);

	auto rt = UpdateNXLMetaData(&f);
	if (!rt)
	{
		newnxlfile.Close();
		f.Close();
		return RESULT2(rt.GetCode(), rt.GetMsg());
	}

	newnxlfile.Close();
	f.Close();

	return RESULT(0);
}

SDWLResult CSDRmUser::ChangeFileToMember(const std::wstring &filepath, const std::string &memberid) {
	SDWLResult res = RESULT(SDWL_SUCCESS);

	if (filepath.find(m_localFile.GetWorkFolder(), 0) != std::wstring::npos)
		return RESULT2(SDWL_ACCESS_DENIED, "you can't change new protected file");

	CSDRmNXLFile *file = NULL;
	res = OpenFile(filepath, (ISDRmNXLFile**)&file);
	if (!res) {
		return res;
	}

	std::string tags = file->GetTags();
	std::vector<SDRmFileRight> rights = file->GetRights();
	SDR_WATERMARK_INFO watermarkinfo = file->GetWaterMark();
	SDR_Expiration expire = file->GetExpiration();

	// check user permission to see whether user has Admin Rights permission or file has extract right.
	if (false == HasAdminRights(filepath)) {

		bool hasExtract = false;
		if (rights.size() > 0) {
			for (auto right : rights) {
				if (right == RIGHT_DECRYPT) {
					hasExtract = true;
					break;
				}
			}
		} else {
			std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsAndWatermarks;
			GetFileRightsFromCentralPolicies(filepath, rightsAndWatermarks, false);
			for (auto right : rightsAndWatermarks) {
				if (right.first == RIGHT_DECRYPT) {
					hasExtract = true;
					break;
				}
			}
		}
		
		if (hasExtract == false) {
			CloseFile(file);
			return RESULT2(SDWL_ACCESS_DENIED, "no permission to change file rights");
		}
	}

	// If membership is the same, no need to continue.
	std::string ownerId = file->GetOwnerID();
	if (ownerId == memberid) {
		CloseFile(file);
		return RESULT2(SDWL_INVALID_DATA, "membership has not change");
	}

	//get and check Token
	RMToken token;
	if (memberid == "")
		token = PopToken();
	else {
		token = PopToken(memberid);
	}
	if (token.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	RMActivityLog log;
	NXLAttributes attr;
	NXLFMT::Obligations obs;
	NXLFMT::Rights nxlrights = ToFileRight(rights);

	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}

	attr.obs = &obs;
	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = tags;

	// get and check Token
	RMToken oldToken = file->GetToken();
	if (oldToken.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	// close the original NXL file before we want to re-protect and overwrite
	CloseFile(file);

	RMNXLFile newnxlfile("");
	RetValue value = ReProtectLocalFileHeader(NX::fs::dos_fullfilepath(filepath).global_dos_path(), oldToken, &token, attr, conv::to_wstring(memberid), L"", log, newnxlfile, true);
	if (!value)
	{
		return RESULT(value);
	}


    CSDRmNXLFile pfile(newnxlfile);
    res = UpdateNXLMetaData(&pfile, true);
    if (!res)
    {
        pfile.Close();
        return res;
    }
    pfile.SetToken(token);
	AddCachedToken(token);
	AddActivityLog(log);
	pfile.Close();

	newnxlfile.Close();

	return RESULT(0);
}

SDWLResult CSDRmUser::RenameFile(const std::wstring &filepath, const std::string &name) {
	SDWLResult res = RESULT(SDWL_SUCCESS);
	
	// file name cannot empty or too long.
	if (name.empty()) {
		return RESULT2(SDWL_INVALID_DATA, "file name should not empty");
	}
	else if (name.size() > 255) {
		return RESULT2(SDWL_INVALID_DATA, "file name is too long");
	}

	CSDRmNXLFile *file = NULL;
	res = OpenFile(filepath, (ISDRmNXLFile**)&file);
	if (!res) {
		return res;
	}

	RMActivityLog log;

	// get and check Token
	RMToken oldToken = file->GetToken();
	if (oldToken.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	std::string tenant = file->GetTenantName();
	std::string memberid = GetMembershipIDByTenantName(tenant);

	// Get attr.
	NXLAttributes attr;
	std::string tags = file->GetTags();
	std::vector<SDRmFileRight> rights = file->GetRights();
	SDR_WATERMARK_INFO watermarkinfo = file->GetWaterMark();
	SDR_Expiration expire = file->GetExpiration();
	NXLFMT::Obligations obs;
	NXLFMT::Rights nxlrights = ToFileRight(rights);
	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}
	attr.obs = &obs;
	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = tags;

	// close the original NXL file before we want to re-protect and overwrite
	CloseFile(file);

	// Get origin file name and extension from name.
	std::string originFileName;
	std::string originFileExtension;

	int i;
	int length = static_cast<int>(name.length());
	int last = -1;
	
	for (i = length - 1; i >= 0; i--) {
		if (name[i] == '.') {
			last = i;
			break;
		}
	}
	if (last == -1) {
		originFileName = name;
		originFileExtension = "";
	}
	else {
		std::string fileExtension = name.substr(last, length - last);
		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
		if (fileExtension.compare(".nxl") == 0) {
			length = last;
			for (i = length - 1; i >= 0; i--) {
				if (name[i] == '.') {
					last = i;
					break;
				}
			}
		}

		originFileName = name.substr(0, length);
		originFileExtension = name.substr(last+1, length-last-1);
	}
	

	RMNXLFile newnxlfile("");
	nlohmann::json json;
	json["filename"] = originFileName;
	json["fileExtension"] = originFileExtension;
	std::string extend = json.dump();
	RetValue value = ReProtectLocalFileHeader(NX::fs::dos_fullfilepath(filepath).global_dos_path(), oldToken, 0, attr, conv::to_wstring(memberid), conv::to_wstring(extend), log, newnxlfile, true);
	if (!value)
	{
		return RESULT(value);
	}
	newnxlfile.Close();

	return RESULT(0);
}

SDWLResult CSDRmUser::ReProtectFile(const std::wstring& filepath, const std::wstring& originalNXLfilePath, const std::wstring& newtags)
{
	NX::fs::dos_fullfilepath dos_fullpath_original(originalNXLfilePath);
	NX::fs::dos_fullfilepath dos_fullpath_inputfile(filepath);

	RMLocalFile path(dos_fullpath_inputfile.global_dos_path());
	if (!path.GetLastError()) {
		return RESULT2(SDWL_PATH_NOT_FOUND, path.GetLastError().GetMessage());
	}
	path.Close();

	// originalNXLfilePath
	//	1. if empty, we will search from edit-file-mapping-cache
	//	2. if we can't find from mapping, check current folder to see whether the NXL file exists or not.
	//		if yes, set originalNXLfilePath as current working folder

	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(dos_fullpath_original.path(), (ISDRmNXLFile**)&file);
	if (!res) {
		return res;
	}

	// check user permission to see whether user has EDIT permission
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rightsAndWatermarks;
	res = GetRights(file, rightsAndWatermarks);
	if (!res) {
		CloseFile(file);
		return res;
	}
	bool haveEdit = false;
	for (size_t i = 0; i < rightsAndWatermarks.size(); i++)
	{
		if (rightsAndWatermarks[i].first == RIGHT_EDIT)
		{
			haveEdit = true;
			break;
		}
	}
	if (false == haveEdit)
	{
		CloseFile(file);
		return RESULT2(SDWL_ACCESS_DENIED, "no permission to edit file");
	}

	RMActivityLog log;
	NXLAttributes attr;
	NXLFMT::Obligations obs;

	// Extract all existing file info from original NXL file
	//	including: ad-hoc rights, watermark, expiry, tags, and TOKEN
	NXLFMT::Rights nxlrights = file->GetNXLRights();
	RMToken token = file->GetToken();
	
	if (token.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

  std::string ttags = "";
  if (!IsFileTagValid(conv::to_string(newtags), ttags))
  {
  }


	std::string tags = file->GetTags();
	SDR_Expiration expire = file->GetExpiration();
	SDR_WATERMARK_INFO watermarkinfo = file->GetWaterMark();
	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}
	attr.obs = &obs;
	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = tags;
  if (ttags == "")
  {
    attr.tags = tags;
  }
  else
  {
    attr.tags = ttags;
  }


	std::string comments = file->GetComments();
	RMCCORE::RMRecipients recipients = file->GetFileRecipients();
	RMRecipientList recipientList = recipients.GetRecipients();
	std::string recipientsemail = "";
	for (size_t i = 0; i < recipientList.size(); i++)
	{
		if (recipientsemail == "")
			recipientsemail = recipientList[i];
		else
			recipientsemail += "," + recipientList[i];
	}

	std::string tenant = file->GetTenantName();
	std::string memberid = GetMembershipIDByTenantName(tenant);

	// generate a temp file as the new NXL file
	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile((conv::to_wstring(NX::itos<char>(0))), conv::to_wstring(dos_fullpath_original.path()), tmpFilePath);

	RMNXLFile newnxlfile(dos_fullpath_original.global_dos_path());
	newnxlfile.Open(token);
	// issue here: we are reusing original ProtectLocalFile which originally is to protect a new file
	// then the file owner will be overwritten by current user
	RetValue value = ReProtectLocalFile(dos_fullpath_inputfile.global_dos_path(), tmpFilePath, attr, token, log, newnxlfile, memberid, false, true);
	if (!value)
	{
		CloseFile(file);
		return RESULT(value);
	}

	newnxlfile.Close();

	// close the original NXL file before we want to re-protect and overwrite
	CloseFile(file);

	// In case the orignal NXL file is under same folder as source file (user open file in RPM folder directly)
	// we should delete the original file instead of copy(overwrite) as RPM driver will block copy.
	if (dos_fullpath_original.path().find(dos_fullpath_inputfile.path(), 0) != std::wstring::npos)
	{
		if (m_SDRmcInstance)
		{
			std::wstring nonnxlfile = NX::conv::remove_extension(dos_fullpath_original.path());
			m_SDRmcInstance->RPMDeleteFile(nonnxlfile);
		}
	}
	else
	{
		DWORD attr = GetFileAttributes(dos_fullpath_original.global_dos_path().c_str());
		if ((INVALID_FILE_ATTRIBUTES != attr) && (attr & FILE_ATTRIBUTE_READONLY))
		{
			attr &= ~FILE_ATTRIBUTE_READONLY;
			SetFileAttributes(dos_fullpath_original.global_dos_path().c_str(), attr);
		}
		DeleteFile(dos_fullpath_original.global_dos_path().c_str());
	}

	if (CopyFile(tmpFilePath.c_str(), dos_fullpath_original.global_dos_path().c_str(), false))
	{
		DeleteFile(tmpFilePath.c_str());
	}
	else
	{
		DeleteFile(tmpFilePath.c_str());
		res = RESULT2(GetLastError(), "CopyFile failed");
		return res;
	}

	return RESULT(0);
}

SDWLResult CSDRmUser::GetRecipients(ISDRmNXLFile * nxlfile, std::vector<std::string> &recipientsemail, std::vector<std::string> &addrecipientsemail, std::vector<std::string> &removerecipientsemail)
{
	CELOG_ENTER;
	if (NULL == nxlfile)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid file handle."));
	CSDRmNXLFile * pfile = (CSDRmNXLFile *)nxlfile;
	if (!nxlfile->IsValidNXL())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid nxl file."));

	RMRecipients recipients = pfile->GetFileRecipients();
	{
		recipientsemail.clear();
		RMCCORE::RMRecipientList recipientslist = recipients.GetRecipients();
		for (size_t i = 0; i < recipientslist.size(); i++) {
			recipientsemail.push_back(recipientslist.at(i));
		}
	}

	{
		addrecipientsemail.clear();
		RMCCORE::RMRecipientList recipientslist = recipients.GetAddRecipients();
		for (size_t i = 0; i < recipientslist.size(); i++) {
			addrecipientsemail.push_back(recipientslist.at(i));
		}
	}

	{
		removerecipientsemail.clear();
		RMCCORE::RMRecipientList recipientslist = recipients.GetRemoveRecipients();
		for (size_t i = 0; i < recipientslist.size(); i++) {
			removerecipientsemail.push_back(recipientslist.at(i));
		}
	}

	CELOG_RETURN_VAL_T(RESULT(0));
}


SDWLResult CSDRmUser::UpdateRecipients(ISDRmNXLFile * nxlfile, const std::vector<std::string> &addrecipientsemail, const std::wstring &comment)
{
	CELOG_ENTER;

    std::vector<std::string> vecRecipients = addrecipientsemail;
    std::sort(vecRecipients.begin(), vecRecipients.end());

	CELOG_LOG(CELOG_INFO, L"nxlfile->GetFileName()=%s, addrecipientsemail={%hs}\n",
			  nxlfile->GetFileName().c_str(),
			  merge<char,','>(vecRecipients).c_str());
	if (NULL == nxlfile)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid file handle."));
	CSDRmNXLFile * pfile = (CSDRmNXLFile *)nxlfile;
	if (!nxlfile->IsValidNXL())
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid nxl file."));

	RMRecipients recipients;
	recipients.AddRecipients(vecRecipients, true);

	pfile->SetFileRecipients(recipients);

	//if (pfile->IsUploadToRMS()) 
	{
		//try update the recipient to RMS
		Client restclient(NXRMC_CLIENT_NAME, true);

		restclient.Open();
		std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

		RMCCORE::HTTPRequest httpreq = GetUpdateRecipientsQuery(*pfile, recipients, comment);
		StringBodyRequest certrequest(httpreq);
		StringResponse certresponse;

		SDWLResult res = spConn->SendRequest(certrequest, certresponse);

		if (res) {
			const std::string& jsondata = certresponse.GetBody();
			CELOG_LOGA(CELOG_DEBUG, "UpdateRecipients: response= %s\n", jsondata.c_str());
			RMCCORE::RetValue ret = recipients.ImportFromRMSResponse(jsondata);
			if (!ret)
				CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
		}
	}

	CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult CSDRmUser::ShareFileFromMyVault(const std::wstring &filepath, const std::vector<std::string> &recipients, const std::string &repositoryId, const std::string &fileName, const std::string &filePathId, const std::string &filePath, const std::string &comment)
{
    CELOG_ENTER;
    CSDRmNXLFile *nxlfile = NULL;
    SDWLResult res = OpenFile(filepath, (ISDRmNXLFile**)&nxlfile);

    if (res)
    {
        //try update the recipient to RMS
        Client restclient(NXRMC_CLIENT_NAME, true);

        restclient.Open();
        std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

        RMCCORE::HTTPRequest httpreq = GetShareFromMyVaultQuery(*nxlfile, recipients, repositoryId, fileName, filePathId, filePath, comment, GetDefaultMembership().GetID(), nxlfile->GetNXLRights());
        StringBodyRequest request(httpreq);
        StringResponse response;

        SDWLResult res = spConn->SendRequest(request, response);

        CloseFile(nxlfile);

        if (res)
        {
            const std::string& jsondata = response.GetBody();
            CELOG_LOGA(CELOG_DEBUG, "GetShareFromMyVaultQuery: response= %s\n", jsondata.c_str());

            unsigned short status = response.GetStatus();
            if (status != http::status_codes::OK.id) {
                CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(response.GetPhrase())));
            }

            nlohmann::json root = nlohmann::json::parse(jsondata);
            if (!root.is_object())
            {
                CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Response JSON error!"));
            }

            std::string message = root.at("message").get<std::string>();
            status = root.at("statusCode").get<uint32_t>();
            if (status != http::status_codes::OK.id) {
                CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
            }

            res = RESULT2(0, message);
        }
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyDriveCreateFolder(const std::wstring&name, const std::wstring&parentfolder)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	RMMyDrive* mydrive = GetMyDrive();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = mydrive->GetMyDriveCreateFolderQuery(conv::to_string(name), conv::to_string(parentfolder));
	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "create folder failed");
		CELOG_RETURN_VAL_T(res);
	}

    try
    {
        std::string strJsonResponse = response.GetBody();
        nlohmann::json root = nlohmann::json::parse(strJsonResponse);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetMyDriveCreateFolderQuery query JSON response is not correct");
    }

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyDriveDeleteItem(const std::wstring&pathid)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	RMMyDrive* mydrive = GetMyDrive();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = mydrive->GetMyDriveDeleteItemQuery(conv::to_string(pathid));
	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "delete item failed");
		CELOG_RETURN_VAL_T(res);
	}

    try
    {
        std::string strJsonResponse = response.GetBody();
        nlohmann::json root = nlohmann::json::parse(strJsonResponse);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetMyDriveDeleteItemQuery query JSON response is not correct");
    }

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyDriveCopyItem(const std::wstring&src, const std::wstring&dest)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	RMMyDrive* mydrive = GetMyDrive();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = mydrive->GetMyDriveCopyItemQuery(conv::to_string(src), conv::to_string(dest));
	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "copy item failed");
		CELOG_RETURN_VAL_T(res);
	}

    try
    {
        std::string strJsonResponse = response.GetBody();
        nlohmann::json root = nlohmann::json::parse(strJsonResponse);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetMyDriveCopyItemQuery query JSON response is not correct");
    }

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyDriveMoveItem(const std::wstring&src, const std::wstring&dest)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	RMMyDrive* mydrive = GetMyDrive();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = mydrive->GetMyDriveMoveItemQuery(conv::to_string(src), conv::to_string(dest));
	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "move item failed");
		CELOG_RETURN_VAL_T(res);
	}

    try
    {
        std::string strJsonResponse = response.GetBody();
        nlohmann::json root = nlohmann::json::parse(strJsonResponse);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetMyDriveMoveItemQuery query JSON response is not correct");
    }

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyDriveCreateShareURL(const std::wstring&pathId, std::wstring&sharedURL)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	RMMyDrive* mydrive = GetMyDrive();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = mydrive->GetMyDriveCreateShareURLQuery(conv::to_string(pathId));
	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "create shared URL failed");
		CELOG_RETURN_VAL_T(res);
	}

    try
    {
        std::string strJsonResponse = response.GetBody();
        nlohmann::json root = nlohmann::json::parse(strJsonResponse);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& results = root.at("results");
        if (results.end() != results.find("url"))
        {
            sharedURL = conv::to_wstring(results["url"].get<std::string>());
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetMyDriveCreateShareURLQuery query JSON response is not correct");
    }

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyDriveDownloadFile(const std::wstring& pathid, std::wstring& targetFolder)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile((conv::to_wstring("myvault")), conv::to_wstring(pathid), tmpFilePath);
	RMMyDrive* mydrive = GetMyDrive();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = mydrive->GetMyDriveDownloadFileQuery(conv::to_string(pathid));
	StringBodyRequest request(httpreq);
	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::wstring outFilePath;
	std::wstring preferredFileName;
	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
		CELOG_RETURN_VAL_T(res);
	}

	response.Finish();
	const HttpHeaders& headers = response.GetHeaders();
	auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
		return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
	});
	if (it != headers.end())
	{
		auto pos = (*it).second.find(L"UTF-8''");
		if (pos != std::wstring::npos) {
			preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
		}
	}
	outFilePath = std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + preferredFileName);


	if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(outFilePath.c_str())) {
		CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
	}

	std::wstring unlimitedPath = NX::fs::dos_fullfilepath(outFilePath).global_dos_path();
	if (!CopyFile(tmpFilePath.c_str(), unlimitedPath.c_str(), false))
	{
		res = RESULT2(GetLastError(), "CopyFile failed");
	}

	DeleteFile(tmpFilePath.c_str());
	targetFolder = outFilePath;

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyVaultDownloadFile(const std::string& pathid, std::wstring& targetFolder, uint32_t downloadtype)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile((conv::to_wstring("myvault")), conv::to_wstring(pathid), tmpFilePath);
	RMMyVault* myvault = GetMyVault();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = myvault->GetMyVaultDownloadFileQuery(pathid, downloadtype);
	StringBodyRequest request(httpreq);
	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::wstring outFilePath;
	std::wstring preferredFileName;
	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
		CELOG_RETURN_VAL_T(res);
	}

	response.Finish();
	const HttpHeaders& headers = response.GetHeaders();
	auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
		return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
	});
	if (it != headers.end())
	{
		auto pos = (*it).second.find(L"UTF-8''");
		if (pos != std::wstring::npos) {
			preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
		}
	}
	outFilePath = std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + preferredFileName);

	
	if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(outFilePath.c_str())) {
		CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
	}

	std::wstring unlimitedPath = NX::fs::dos_fullfilepath(outFilePath).global_dos_path();
	if (!CopyFile(tmpFilePath.c_str(), unlimitedPath.c_str(), false))
	{
		res = RESULT2(GetLastError(), "CopyFile failed");
	}

	// Everything is done, call openFile to let Container known this file, 
	// at least preare token for offline mode
	ISDRmNXLFile * pIgnored = NULL;
	OpenFile(outFilePath, &pIgnored);
	if (pIgnored != NULL)
		CloseFile(pIgnored);
	DeleteFile(tmpFilePath.c_str());
	targetFolder = outFilePath;

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::SharedWithMeDownloadFile(const std::wstring& transactionCode, const std::wstring& transactionId, std::wstring& targetFolder, bool forViewer)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile((conv::to_wstring("sharedwithme")), transactionId, tmpFilePath);
	RMSharedWithMe* sharedwithme = GetSharedWithMe();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = sharedwithme->GetSharedWithMeDownloadFileQuery(conv::to_string(transactionCode), conv::to_string(transactionId), forViewer);
	StringBodyRequest request(httpreq);
	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::wstring outFilePath;
	std::wstring preferredFileName;
	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
		CELOG_RETURN_VAL_T(res);
	}

	response.Finish();
	const HttpHeaders& headers = response.GetHeaders();
	auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
		return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
	});
	if (it != headers.end())
	{
		auto pos = (*it).second.find(L"UTF-8''");
		if (pos != std::wstring::npos) {
			preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
		}
	}

	outFilePath = std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + preferredFileName);
	if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(outFilePath.c_str())) {
		CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
	}

	std::wstring fixedOutFilePath = NX::fs::dos_fullfilepath(outFilePath).global_dos_path();
	if (!CopyFile(tmpFilePath.c_str(), fixedOutFilePath.c_str(), false))
	{
		res = RESULT2(GetLastError(), "CopyFile failed");
	}

	// Everything is done, call openFile to let Container known this file, 
	// at least preare token for offline mode
	ISDRmNXLFile * pIgnored = NULL;
	OpenFile(outFilePath, &pIgnored);
	if (pIgnored != NULL)
		CloseFile(pIgnored);
	DeleteFile(tmpFilePath.c_str());
	targetFolder = outFilePath;

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::SharedWithMeReShareFile(const std::string& transactionId, const std::string& transactionCode, const std::string emaillist)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);
    RMSharedWithMe* sharedwithme = GetSharedWithMe();
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    HTTPRequest httpreq;
    httpreq = sharedwithme->GetSharedWithMeReShareQuery(transactionId, transactionCode, emaillist);
    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    unsigned short status = response.GetStatus();
    if (status != http::status_codes::OK.id) {
        CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(response.GetPhrase())));
    }

    const std::string& jsondata = response.GetBody();
    try
    {
        nlohmann::json root = nlohmann::json::parse(jsondata);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json data is not correct!"));
        }

        int status = root.at("statusCode").get<int>();
        std::string message = root.at("message").get<std::string>();
        if (status != http::status_codes::OK.id) {
            // error from RMS, discard this one and continue
            res = RESULT2(SDWL_RMS_ERRORCODE_BASE + status, message);
            CELOG_RETURN_VAL_T(res);
        }

        res = RESULT2(0, message);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "Log query JSON response is not correct");
        CELOG_RETURN_VAL_T(res);
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectDownloadFile(const unsigned int projectid, const std::string& pathid, std::wstring& targetFolder, RM_ProjectFileDownloadType type)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile((conv::to_wstring(NX::itos<char>(projectid))), conv::to_wstring(pathid), tmpFilePath);
	RMMyProjects* projects = GetMyProjects();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	::EnterCriticalSection(&_downloadprojectfile_lock);
	HTTPRequest httpreq;
	httpreq = projects->GetDownloadFileQuery(projectid, pathid, ProjectFileDownloadType(type));
	StringBodyRequest request(httpreq);
	::LeaveCriticalSection(&_downloadprojectfile_lock);

	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::wstring outFilePath;
	std::wstring preferredFileName;
	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
		CELOG_RETURN_VAL_T(res);
	}

	response.Finish();
	const HttpHeaders& headers = response.GetHeaders();
	auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
		return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
	});
	if (it != headers.end()) 
	{
		auto pos = (*it).second.find(L"UTF-8''");
		if (pos != std::wstring::npos) {
			preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
		}
	}

	NX::fs::dos_fullfilepath _output_filepath(std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + preferredFileName), false);
	if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(_output_filepath.global_dos_path().c_str())) {
		CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
	}

	if (!CopyFile(tmpFilePath.c_str(), _output_filepath.global_dos_path().c_str(), false))
	{
		res = RESULT2(GetLastError(), "CopyFile failed");
	}

	// Everything is done, call openFile to let Container known this file, 
	// at least preare token for offline mode
	ISDRmNXLFile * pIgnored = NULL;
	OpenFile(_output_filepath.path(), &pIgnored);
	if (pIgnored != NULL)
		CloseFile(pIgnored);
	DeleteFile(tmpFilePath.c_str());
	targetFolder = _output_filepath.path();

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::SharedWithMeDownloadPartialFile(const std::wstring& transactionCode, const std::wstring& transactionId, std::wstring& targetFolder, bool forViewer)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile((conv::to_wstring("sharedwithme")), transactionId, tmpFilePath);
	RMSharedWithMe* sharedwithme = GetSharedWithMe();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = sharedwithme->GetSharedWithMeDownloadFileQuery(conv::to_string(transactionCode), conv::to_string(transactionId), forViewer);
	StringBodyRequest request(httpreq);
	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response, INFINITE, 0x4000);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::wstring outFilePath;
	std::wstring preferredFileName;
	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
		CELOG_RETURN_VAL_T(res);
	}

	response.Finish();
	const HttpHeaders& headers = response.GetHeaders();
	auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
		return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
	});
	if (it != headers.end())
	{
		auto pos = (*it).second.find(L"UTF-8''");
		if (pos != std::wstring::npos) {
			preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
		}
	}

	NX::fs::dos_fullfilepath _output_filepath(std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + L"partial_" + preferredFileName));
	if (!CopyFile(tmpFilePath.c_str(), _output_filepath.global_dos_path().c_str(), false))
	{
		res = RESULT2(GetLastError(), "CopyFile failed");
	}

	// Everything is done, call openFile to let Container known this file, 
	// at least preare token for offline mode
	ISDRmNXLFile * pIgnored = NULL;
	OpenFile(_output_filepath.path(), &pIgnored);
	if (pIgnored != NULL)
		CloseFile(pIgnored);
	DeleteFile(tmpFilePath.c_str());
	targetFolder = _output_filepath.path();

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::UploadFile(
    const std::wstring& nxlFilePath, 
    const std::wstring& sourcePath, 
    const std::wstring& recipientEmails, 
    const std::wstring& comments,
    bool bOverwrite)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);
	CSDRmNXLFile* nxlFile = NULL;
	res = OpenFile(nxlFilePath, (ISDRmNXLFile**)&nxlFile);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}

	UpdateNXLMetaData(nxlFile, false);

	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	// Parse recipients.
	std::vector<std::string> recipientsVec;
	NX::conv::split_string(toUtf8(recipientEmails), recipientsVec, ",");

	RMRecipients recipients(recipientsVec);
	nxlFile->SetFileRecipients(recipients);
	nxlFile->SetComments(toUtf8(comments));

	if (sourcePath.empty()) {
		nxlFile->SetSourceFilePath(toUtf8(nxlFile->GetFileName()));
	}
	else {
		nxlFile->SetSourceFilePath(toUtf8(sourcePath));
	}
		
	HTTPRequest httpreq;
    if (recipients.GetRecipients().size() != 0) {
        httpreq = GetShareLocalFileQuery(GetDefaultMembership(), *nxlFile, bOverwrite);
    }
    else {
        httpreq = GetProtectLocalFileQuery(*nxlFile, bOverwrite);
    }

	NXLUploadRequest filerequest(httpreq, conv::to_wstring(nxlFile->GetFilePath()), RMS::boundary::End);
	StringResponse response;
	res = spConn->SendRequest(filerequest, response);

	if (!res)
		return res;

	RMCCORE::RetValue ret = nxlFile->ImportFromRMSResponse(response.GetBody());
	if (!ret)
		res = RESULT2(ret.GetCode(), ret.GetMessage());

	// release
	CloseFile(nxlFile);
	nxlFile = NULL;

	return res;
}

SDWLResult CSDRmUser::MyDriveUploadFile(
    const std::wstring& pathId, 
    const std::wstring& parentPathId, 
    bool overwrite)
{
    CELOG_ENTER;
	SDWLResult res = RESULT(0);
	NX::fs::dos_fullfilepath input_filepath(pathId);
	std::wstring unlimitedPath = input_filepath.global_dos_path();
	{
		GetFileAttributes(unlimitedPath.c_str()); // from winbase.h
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(unlimitedPath.c_str()) && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
			return RESULT2(ERROR_PATH_NOT_FOUND, "File not found");
	}

	// extract file name from source full path
	std::wstring filename = L"";
	std::experimental::filesystem::path _filename(unlimitedPath);
	filename = _filename.filename();


	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq = GetUploadMyDriveFileQuery(conv::to_string(filename), conv::to_string(parentPathId), overwrite);

	NXLUploadRequest filerequest(httpreq, conv::to_wstring(pathId), RMS::boundary::End);
	StringResponse response;
	res = spConn->SendRequest(filerequest, response);

	if (!res)
		return res;

	if (response.GetStatus() != 200)
	    res = RESULT2(response.GetStatus() + SDWL_RMS_ERRORCODE_BASE, "");

    try
    {
        std::string strJsonResponse = response.GetBody();
        nlohmann::json root = nlohmann::json::parse(strJsonResponse);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetUploadMyDriveFileQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::UploadProjectFile(uint32_t projectid, const std::wstring&destfolder, ISDRmNXLFile * file, int uploadType, bool userConfirmedFileOverwrite)
{
	SDWLResult res = RESULT(0);
	CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

	UpdateNXLMetaData(file, false);

	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = GetProtectProjectFileQuery(NX::itos<char>((int)projectid), conv::to_string(destfolder), *nxlFile, uploadType, userConfirmedFileOverwrite);

	NXLUploadRequest filerequest(httpreq, conv::to_wstring(nxlFile->GetFilePath()), RMS::boundary::End);
	StringResponse response;
	res = spConn->SendRequest(filerequest, response);

	if (!res)
		return res;

	//if (response.GetStatus() != 200)
	//    res = RESULT2(response.GetStatus(), "");
	RMCCORE::RetValue ret = nxlFile->ImportFromRMSResponse(response.GetBody());
	if (!ret)
		res = RESULT2(ret.GetCode(), ret.GetMessage());

	return res;
}

bool CSDRmUser::IsFileProtected(const std::wstring &filepath)
{
	NX::fs::dos_fullfilepath input_filepath(filepath);

	unsigned int dirstatus = 3;
	bool filestatus = 0;
	SDWLResult res = m_SDRmcInstance->RPMGetFileStatus(input_filepath.path(), &dirstatus, &filestatus);
	if (dirstatus & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR))
	{
		// file is under RPM folder
		return filestatus;
	}
	else
	{
		uint8_t buf[128];
		uint32_t bytesToRead = 64;
		uint32_t min = 64;

		RMLocalFile sourcefile(input_filepath.global_dos_path());
		if (!sourcefile.Open())
			return false;
		bytesToRead = sourcefile.read(0, buf, bytesToRead);
		if (bytesToRead < min)
			return false;
		uint8_t data[] = "NXLFMT@";
		if (memcmp(buf, data, sizeof(data)))
			return false;

		return true;
	}
}

//
// CloseFile
//		purpose:	remove the opened file, but still keep the token in local token cache
//					delete the file point to release memory [else there is a memory leak]
//
SDWLResult CSDRmUser::CloseFile(ISDRmNXLFile * file) 
{
	SDWLResult res = RESULT(0);

	if (file == NULL)
		return res;

	CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;
	nxlFile->Close();

	delete nxlFile;

	return res;
}

SDWLResult CSDRmUser::FindCachedToken(const std::string &duid, RMToken & token)
{
	SDWLResult res = RESULT(0);

	if (!m_tokenslist.FindToken(duid, token))
	{
		if (m_SDRmcInstance)
		{
			time_t ttl = 0;
			res = m_SDRmcInstance->RPMFindCachedToken(NX::conv::utf8toutf16(duid), token, ttl);
			if (res)
				m_tokenslist.AddToken(token, ttl);
		}
		else
			return RESULT2(ERROR_NOT_READY, "System is not initialized");
	}

	return res;
}

SDWLResult CSDRmUser::AddCachedToken(const RMToken & token)
{
	RMToken _token = token;
//	m_tokenslist.AddToken(_token);

	if (m_SDRmcInstance)
		return m_SDRmcInstance->RPMAddCachedToken(token);
	else
		return RESULT2(ERROR_NOT_READY, "System is not initialized");
}

SDWLResult CSDRmUser::RemoveCachedToken(const std::string & duid)
{
	m_tokenslist.RemoveToken(duid);
	if (m_SDRmcInstance)
		return m_SDRmcInstance->RPMRemoveCachedToken(duid);
	else
		return RESULT2(ERROR_NOT_READY, "System is not initialized");
}


SDWLResult CSDRmUser::OpenFile(const std::wstring &nxlfilepath, ISDRmNXLFile ** file) 
{
	RMToken filetoken;
	return OpenFileWithToken(nxlfilepath, file, false, filetoken);
}

SDWLResult CSDRmUser::OpenFileForMetaData(const std::wstring &nxlfilepath, ISDRmNXLFile ** file)
{
	RMToken filetoken;
	return OpenFileWithToken(nxlfilepath, file, true, filetoken);
}

SDWLResult CSDRmUser::OpenFileWithToken(const std::wstring &nxlfilepath, ISDRmNXLFile ** file, bool bIgnoreRight, RMToken &filetoken)
{
	SDWLResult res = RESULT(0);
	if (NULL == file)
		return RESULT2(SDWL_INVALID_DATA, "Invalid parameter");


	// find from m_localFile first
	CSDRmNXLFile *nxlfile = NULL;
	{
		// the file is not cached

		std::wstring unlimitedPath = NX::fs::dos_fullfilepath(nxlfilepath, false).global_dos_path();
		nxlfile = new CSDRmNXLFile(unlimitedPath);
		if (NULL == nxlfile)
			return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "fail to allocate memory");

		if (!nxlfile->IsNXL())
		{
			delete nxlfile;
			GetFileAttributes(unlimitedPath.c_str()); // from winbase.h
			if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(unlimitedPath.c_str()) && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
				return RESULT2(ERROR_PATH_NOT_FOUND, "File not found");
			else
				return RESULT2(SDWL_INVALID_DATA, "Invalid nxl file");
		}

		std::string duid = nxlfile->GetDuid();
		if (GetMembershipID(nxlfile->GetTenantName()) == "")
		{
			delete nxlfile;
			// current user does not belong to the tokengroup of the file
			return RESULT2(SDWL_NXL_NOT_AUTHORIZE, "Not authorized to access the file");
		}

		if (!FindCachedToken(duid, filetoken))
		{
			Client restclient(NXRMC_CLIENT_NAME, true);
			restclient.Open();
			std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

            std::string dynamicEvalRequest = BuildDynamicEvalRequest();
			HTTPRequest httpreq = GetFileTokenQuery(*nxlfile, 0, "", "", dynamicEvalRequest);

			StringBodyRequest request(httpreq);
			StringResponse response;
			res = spConn->SendRequest(request, response);

			if (!res) {
				delete nxlfile;
				CELOG_LOGA(CELOG_DEBUG, "OpenFile: get token failed. %s\n", response.GetBody().c_str());
				return res;
			}

			RMCCORE::RetValue ret = filetoken.ImportFromRMSResponse(response.GetBody());
			if (!ret) {
				do
				{
					if (ret.GetCode() == 61841)
					{
						// user is not authorized, check whehter current user is API user or not
						if (IsAPIUser() == TRUE)
						{
							// ask RPM service to query token via RPMGetFileInfo API
							std::string f_tags;
							std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> f_userRightsAndWatermarks;
							std::vector<SDRmFileRight> f_rights;
							SDR_WATERMARK_INFO f_waterMark;
							SDR_Expiration f_expiration;
							std::string f_creatorid;
							std::string f_infoext;
							DWORD f_attributes;
							DWORD f_isRPMFolder;
							DWORD f_isNXLFile = 0;
							std::string f_tenant;
							std::string f_membershipid;
							std::string f_duid;

							res = m_SDRmcInstance->RPMGetFileInfo(nxlfilepath, f_duid, f_userRightsAndWatermarks,
								f_rights, f_waterMark, f_expiration, f_tags,
								f_tenant, f_creatorid, f_infoext, f_attributes, f_isRPMFolder, f_isNXLFile);
							if (res)
							{
								if (FindCachedToken(duid, filetoken))
								{
									// now token is cached by RPM service
									break;
								}
							}
						}
					}

					delete nxlfile;
					return RESULT2(ret.GetCode(), ret.GetMessage());
				} while (false);
			}
			//filetoken set duid
			RMToken token(duid, filetoken.GetKey(), 0);
			filetoken = token;
			AddCachedToken(RMToken(duid, filetoken.GetKey(), 0));
		}

		if (!nxlfile->SetToken(filetoken)) // Open
		{
			RemoveCachedToken(duid);
			delete nxlfile;
			return RESULT2(SDWL_INTERNAL_ERROR, "Fail to open file");
		}

		std::string tenant = nxlfile->GetTenantName();
		std::string memberid = GetMembershipIDByTenantName(tenant);
		if (nxlfile->CheckExpired(memberid)) // expired
		{
			// exipred, check whether nxlfile is from myvaule and user is the file owner
			if (nxlfile->GetOwnerID() != m_defaultmembership.GetID()) // for creator, he might not be the owner with full rights in case file is in project/workspace; -- Raymond
			{
				RemoveCachedToken(duid);
				nxlfile->ResetToken();
				delete nxlfile;
				return RESULT2(SDWL_NXL_INSUFFICIENT_RIGHTS, "Expired");
			}
		}
		if (!bIgnoreRight) 
		{
			// now we have the token, shall we check the file permission (at least with VIEW right)
			std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rights;
			SDWLResult res = GetRights(nxlfile, rights);
			if (rights.size() == 0)
			{
				if (IsAPIUser() == FALSE) // if API user, shall not delete token
				{
					// no any right, remove the token, tell user open failure
					RemoveCachedToken(duid);
					nxlfile->ResetToken();
					delete nxlfile;
					return RESULT2(SDWL_NXL_INSUFFICIENT_RIGHTS, "Unauthorized to access file");
				}
			}
		}

		*file = (ISDRmNXLFile*)nxlfile;
		return res;
	}
}

SDWLResult CSDRmUser::GetFileDuid(const std::wstring &nxlfilepath, std::string& duid)
{
	SDWLResult res = RESULT(0);
	// find from m_localFile first
	NX::fs::dos_fullfilepath input_filepath(nxlfilepath);
	CSDRmNXLFile *nxlfile = NULL;
	nxlfile = new CSDRmNXLFile(input_filepath.global_dos_path());
	if (NULL == nxlfile)
		return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "fail to allocate memory");

	if (!nxlfile->IsNXL())
	{
		delete nxlfile;
		GetFileAttributes(nxlfilepath.c_str()); // from winbase.h
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(nxlfilepath.c_str()) && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
			return RESULT2(ERROR_PATH_NOT_FOUND, "File not found");
		else
			return RESULT2(SDWL_INVALID_DATA, "Invalid nxl file");
	}

	duid = nxlfile->GetDuid();

	return res;
}

SDWLResult CSDRmUser::GetFileToken(CSDRmNXLFile* nxlfile, RMToken& token)
{
	SDWLResult res = RESULT(0);
	std::string duid = nxlfile->GetDuid();
	RMToken filetoken;

	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    std::string dynamicEvalRequest = BuildDynamicEvalRequest();
	HTTPRequest httpreq = GetFileTokenQuery(*nxlfile, 0, "", "", dynamicEvalRequest);

	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res) {
		CELOG_LOGA(CELOG_DEBUG, "OpenFile: get token failed. %s\n", response.GetBody().c_str());
		// delete nxlfile;
		return res;
	}

	RMCCORE::RetValue ret = filetoken.ImportFromRMSResponse(response.GetBody());
	if (!ret) {
		// delete nxlfile;
		res = RESULT2(ret.GetCode(), ret.GetMessage());
		return res; // get token failed, return directly
	}
	//filetoken set duid
	RMToken ntoken(duid, filetoken.GetKey(), 0);
	token = ntoken;
	AddCachedToken(RMToken(duid, filetoken.GetKey(), 0));

	return res;
}

SDWLResult CSDRmUser::GetFileToken(const std::string& file_ownerid, const std::string &agreement, const std::string& duid, uint32_t ml, const std::string& policy, const std::string& tags, RMToken &filetoken)
{
	SDWLResult res;
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	const uint32_t protectionType = (policy.empty() || policy == "{}" ? 1 : 0);

    std::string dynamicEvalRequest = BuildDynamicEvalRequest();
	HTTPRequest httpreq = GetFileTokenQuery(file_ownerid, agreement, duid, ml, protectionType, policy, tags, dynamicEvalRequest);

	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res) {
		CELOG_LOGA(CELOG_ERROR, "OpenFile: get token failed. %s\n", response.GetBody().c_str());
		return res;
	}

	RMCCORE::RetValue ret = filetoken.ImportFromRMSResponse(response.GetBody());
	if (!ret) {
		res = RESULT2(ret.GetCode(), ret.GetMessage());
		CELOG_LOG(CELOG_ERROR, L"filetoken.ImportFromRMSResponse failed: \n");
		return res;
	}

	//filetoken set duid
	RMToken token(duid, filetoken.GetKey(), 0);
	filetoken = token;
	return res;
}

SDWLResult CSDRmUser::DecryptNXLFile(ISDRmNXLFile * file, const std::wstring &targetfilepath)
{
	SDWLResult res = RESULT(0);

	CSDRmNXLFile* rmnxlFile = (CSDRmNXLFile*)file;
	const std::wstring nxlFile = file->GetFileName();
	RMToken token;

	token = rmnxlFile->GetToken();
	if (!rmnxlFile->Open())
	{
		token = rmnxlFile->GetToken();
		if (token.GetKey().size() == 0)
		{
			// need get token from server
			res = GetFileToken(rmnxlFile, token);
			if (!res)
			{
				return res;
			}
		}
		if (!rmnxlFile->Open(token))
		{
			return RESULT2(SDWL_INVALID_DATA, "Token doesn't match with file");
		}
	}

	uint64_t contentLength = rmnxlFile->GetFileSize();
	uint64_t offset = 0;

	std::ofstream ifs;
	ifs.open(NX::fs::dos_fullfilepath(targetfilepath).global_dos_path(), std::ios_base::binary | std::ios_base::trunc | std::ios_base::out);

	while (contentLength)
	{
		uint8_t buf[513];
		uint32_t bytesToRead = 512;
		memset(buf, 0, sizeof(buf));
		bytesToRead = rmnxlFile->read(offset, buf, bytesToRead);
		if (bytesToRead > contentLength)
			bytesToRead = (uint32_t)contentLength;
		if (bytesToRead == 0)
			break;
		ifs.write((char *)buf, bytesToRead);		
		offset += bytesToRead;
		contentLength -= bytesToRead;
	}
	rmnxlFile->Close();
	rmnxlFile->Open();

	ifs.close();

	return res;
}

void CSDRmUser::GeneratePassCode()
{
	m_passcode.clear();
	std::srand((unsigned int)RMCCORE::NXTime::nowTime());
	for (int i = 0; i < 16; i++)
	{
		unsigned char r = (unsigned char)std::rand();
		std::string tmp = RMCCORE::utohs<char>(r);
		m_passcode += tmp;
	}
}

SDWLResult CSDRmUser::LogoutUser()
{
    CELOG_ENTER;
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    RMCCORE::HTTPRequest httpreq = GetUserLogoutQuery();
    StringBodyRequest request(httpreq);
    StringResponse response;

    SDWLResult res = spConn->SendRequest(request, response);

    if (!res) {
        CELOG_RETURN_VAL_T(res);
    }

    const std::string& jsondata = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "LogoutUser: response= %s\n", jsondata.c_str());

    try {
        Logout(jsondata);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "RMS returned invalid logout result");
        CELOG_RETURN_VAL_T(res);
    }

    RMHeartbeat nullHb;
    m_heartbeat = nullHb;

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetSavedFiles()
{
	SDWLResult res;

	try {
		res = GetMiscFile();
	}
	catch (...) {
		// The JSON data is NOT correct
		res = RESULT2(ERROR_INVALID_DATA, "Get saved files failed");
	}
	return res;
}

SDWLResult CSDRmUser::GetMiscFile()
{
	SDWLResult res;
	std::string s;

	//GetAttribsFromFile();

	try {
		res = m_File.FileRead(POLICY_FILE, m_policyBundle);
	}
	catch (...) {
		res = RESULT2(SDWL_INVALID_DATA, "Cannot read policy file");
	}

	res = m_File.FileRead(HEARTBEAT_FILE, s);
	if (!res)
		return res;
	try {
		RetValue ret = m_heartbeat.ImportFromString(s);
		if (!ret)
			return RESULT2(SDWL_INVALID_DATA, res.GetMsg());
	}
	catch (...) {
		// The JSON data is NOT correct
		res = RESULT2(SDWL_INVALID_DATA, "Heartbeat JSON file is incorrect");
	}

	return res;
}

void CSDRmUser::get_all_dirs(const std::string& str, std::vector<std::string>& vec)
{
    std::stringstream f(str);
    std::string s = str;
    while (std::getline(f, s, ','))
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        vec.push_back(s);
    }
}

SDWLResult CSDRmUser::GetAttribsFromFile()
{
    SDWLResult res;
    std::string s;
    std::pair<std::wstring, std::wstring> data;

    res = m_File.FileRead(ATTRIB_FILE, s);
    if (!res)
        return res;

    try {
        std::string attrs = s;
        if (attrs.size() < 3)
            return RESULT2(SDWL_INVALID_DATA, "GetAttribsFromFile not enough data");

        std::vector<std::string> vec;
        get_all_dirs(attrs, vec);
        m_attributes.clear();
        for (size_t i = 0; i < vec.size(); i += 2)
        {
            m_attributes.push_back(std::make_pair(vec[i], vec[i + 1]));
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(SDWL_INVALID_DATA, "GetAttribsFromFile failed");
    }

    return res;
}

SDWLResult CSDRmUser::AddActivityLog(const std::wstring& nxlFilePath, RM_ActivityLogOperation op, RM_ActivityLogResult result)
{
	CELOG_ENTER;
	SDWLResult res;
	NX::fs::dos_fullfilepath input_filepath(nxlFilePath);
	CSDRmNXLFile nxlfile(input_filepath.global_dos_path());

	std::string duid;
	std::string ownerid;
	std::string file_path;

	if (!nxlfile.IsNXL())
	{
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(input_filepath.global_dos_path().c_str()) && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
			CELOG_RETURN_VAL_T(RESULT2(ERROR_PATH_NOT_FOUND, "File not found"));

		std::wstring originalNXLFile = L"";
		RPMEdit_FindMap(input_filepath.path(), originalNXLFile);
		if (originalNXLFile.size() > 0)
		{
			// get the DUID, ownerid from original NXL
			CSDRmNXLFile orignxlfile(originalNXLFile);
			duid = orignxlfile.GetDuid();
			ownerid = orignxlfile.GetTenant();
			file_path = orignxlfile.GetFilePath();
		}
		else
		{
			// file is not in the edit-mapping
			// try current folder to see whether the folder is RPM folder
			unsigned int dirstatus = 3;
			bool bNXL = true;
			m_SDRmcInstance->RPMGetFileStatus(input_filepath.path(), &dirstatus, &bNXL);
			if ((dirstatus & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)) && bNXL)
			{
				// under RPM folder
				std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> userRightsAndWatermarks;
				std::vector<SDRmFileRight> rights;
				SDR_WATERMARK_INFO waterMark;
				SDR_Expiration expiration;
				std::string tags;
				std::string creatorid;
				std::string infoext;
				DWORD attributes;
				DWORD isRPMFolder;
				DWORD isNXLFile;
				std::string tenant;
				std::string membershipid;
				std::string _duid;


				res = m_SDRmcInstance->RPMGetFileInfo(input_filepath.path(), _duid, userRightsAndWatermarks,
							rights, waterMark, expiration, tags,
							tenant, creatorid, infoext, attributes, isRPMFolder, isNXLFile);
				if (!res)
					CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid nxl file"));

				duid = _duid;
				ownerid = creatorid;
				file_path = NX::conv::utf16toutf8(input_filepath.path().c_str());
			}
			else
			{
				CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid nxl file"));
			}
		}
	}
	else
	{
		duid = nxlfile.GetDuid();
		ownerid = nxlfile.GetTenant();
		file_path = nxlfile.GetFilePath();
	}

    nlohmann::json root = nlohmann::json::object();
    root["duid"] = duid;
    root["ownerid"] = ownerid;
    root["file_path"] = file_path;
    root["app_path"] = NX::conv::to_string(NX::win::get_current_process_path()); // m_params.GetProduct().GetName();
    root["company"] = m_params.GetProduct().GetPublisherName();
    root["operation"] = op;
    root["result"] = result;

    res = m_SDRmcInstance->RPMAddActivityLog(root.dump());
    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::AddActivityLog(ISDRmNXLFile * file, RM_ActivityLogOperation op, RM_ActivityLogResult result)
{
	CELOG_ENTER;
	SDWLResult res;
	CSDRmNXLFile* nxlfile = (CSDRmNXLFile*)file;

	if (!nxlfile) CELOG_RETURN_VAL_T(RESULT(ERROR_INVALID_DATA));

    nlohmann::json root = nlohmann::json::object();
    root["duid"] = nxlfile->GetDuid();
    root["ownerid"] = nxlfile->GetTenant();
    root["file_path"] = nxlfile->GetFilePath();
    root["app_path"] = NX::conv::to_string(NX::win::get_current_process_path()); // m_params.GetProduct().GetName();
    root["company"] = m_params.GetProduct().GetPublisherName();
    root["operation"] = op;
    root["result"] = result;

    res = m_SDRmcInstance->RPMAddActivityLog(root.dump());
    CELOG_RETURN_VAL_T(res);
}

void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = ",")
{
	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);

	// Find first non-delimiter.
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		std::string data = str.substr(lastPos, pos - lastPos);
		if (data.size() >= 1 && data[data.size() - 1] == '"')
		{
			if (data.size() >= 2 && data[0] == '"')
				data = data.substr(1, data.size() - 1);
			if (data.size() >= 1 && data[data.size() - 1] == '"')
				data = data.substr(0, data.size() - 1);
			tokens.push_back(data);
			// Skip delimiters.
			lastPos = str.find_first_not_of(delimiters, pos);
			// Find next non-delimiter.
			pos = str.find_first_of(delimiters, lastPos);
		}
		else
		{
			std::string::size_type _lastPos = str.find_first_not_of(delimiters, pos);
			pos = str.find_first_of(delimiters, _lastPos);
		}
	}
}

SDWLResult CSDRmUser::AddActivityLog(const RMActivityLog &act_log)
{
	CELOG_ENTER;
	SDWLResult res;

	std::string str_log = act_log.Serialize();

	//// Reference: https://bitbucket.org/nxtlbs-devops/rightsmanagement-wiki/wiki/RMS/RESTful%20API/Log%20REST%20API

	//m_logstring = "\"";
	//// Field 0: DUID
	//m_logstring += nxlduid;
	//// Field 1:  Owner Id
	//m_logstring += "\",\""; m_logstring += fileownerid;
	//// Field 2:  User Id
	//m_logstring += "\",\""; m_logstring += userid;
	//// Field 3:  Operation
	//m_logstring += "\",\""; m_logstring += itos<char>(operation);
	//// Field 4:  Device Id
	//m_logstring += "\",\""; m_logstring += param.GetDeviceID();
	//// Field 5:  Device Type
	//m_logstring += "\",\""; m_logstring += itos<char>(param.GetPlatformID());
	//// Field 6:  Repository Id
	//m_logstring += "\",\""; m_logstring += repoId;
	//// Field 7:  File pathId
	//m_logstring += "\",\""; m_logstring += fileId;
	//// Field 8:  File Name
	//m_logstring += "\",\""; m_logstring += fileName;
	//// Field 9:  File Display Path
	//m_logstring += "\",\""; m_logstring += filePath;
	//// Field 10: Application Name
	//m_logstring += "\",\""; m_logstring += param.GetProduct().GetName();
	//// Field 11: Application Path
	//m_logstring += "\",\""; m_logstring += param.GetProduct().GetPath();
	//// Field 12: Application Publisher
	//m_logstring += "\",\""; m_logstring += param.GetProduct().GetPublisherName();
	//// Field 13: Access Result
	//m_logstring += "\",\""; m_logstring += itos<char>(accessResult);
	//// Field 14: Access Time
	//m_logstring += "\",\""; m_logstring += i64tos<char>(accessTime);
	//// Field 15: Activity Data
	//m_logstring += "\",\""; m_logstring += extraData;
	//// Field 16: Account Type
	//m_logstring += "\",\""; m_logstring += itos<char>(accountType);
	//m_logstring += "\"";
	std::vector<std::string> log_data;
	tokenize(str_log, log_data, ",");

    nlohmann::json root = nlohmann::json::object();
    root["duid"] = log_data[0];
    root["ownerid"] = log_data[1];
    root["file_path"] = log_data[8];
    root["app_path"] = m_params.GetProduct().GetName();
    root["company"] = m_params.GetProduct().GetPublisherName();
    root["operation"] = std::stoi(log_data[3]);
    root["result"] = std::stoi(log_data[13]);

    res = m_SDRmcInstance->RPMAddActivityLog(root.dump());
    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetNXLFileActivityLog(ISDRmNXLFile* file, uint64_t startPos, uint64_t count, RM_ActivityLogSearchField searchField, const std::string &searchText, RM_ActivityLogOrderBy orderByField, bool orderByReverse)
{
    CELOG_ENTER;
    SDWLResult res;
    CSDRmNXLFile* sdrfile = (CSDRmNXLFile*)file;

    HTTPRequest httpreq = GetNXLFileActivitylogQuery(*sdrfile, startPos, count, (RMCCORE::RMLogSearchField) searchField, searchText, (RMCCORE::RMLogOrderBy) orderByField, orderByReverse);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetNXLFileActivitylog: response=%s\n", jsonreturn.c_str());

    RMNXLFileLogs logs;

    try
    {
        RMCCORE::RetValue ret = logs.ImportFromRMSResponse(jsonreturn);
        if (!ret)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "Log query JSON response is not correct");
    }

    std::vector<SDR_FILE_ACTIVITY_INFO> actVec;
    size_t lognum = logs.size();
    std::wstring name = sdrfile->GetFileName();
    if (m_fileActivityMap.find(name) != m_fileActivityMap.end())
        actVec = m_fileActivityMap[name];

    for (size_t i = 0; i < lognum; i++)
    {
        RMCCORE::RMNXLFileActivity* act = (RMCCORE::RMNXLFileActivity*)logs.GetAt(i);
        SDR_FILE_ACTIVITY_INFO info;
        info.email = act->GetEmail();
        info.operation = act->GetOperation();
        info.deviceType = act->GetDeviceType();
        info.deviceId = act->GetDeviceID();
        info.accessResult = act->GetAccessResult();
        info.accessTime = act->GetAccessTime();
        info.duid = logs.GetDUID(); // act->GetDuid(); activity->duid can't be loaded from Json; let's use the duid for the file (whole logs)
        actVec.push_back(info);
        m_fileActivityMap[name] = actVec;
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetActivityInfo(const std::wstring &fileName, std::vector<SDR_FILE_ACTIVITY_INFO>& infoVec)
{
	NX::fs::dos_fullfilepath input_filepath(fileName);
    if (m_fileActivityMap.find(input_filepath.path()) != m_fileActivityMap.end())
    {
        infoVec = m_fileActivityMap[input_filepath.path()];
        return RESULT(0);
    }

    return RESULT2(ERROR_FILE_NOT_FOUND, "File not found");
}

SDWLResult CSDRmUser::GetHeartBeatInfo()
{
    CELOG_ENTER;
    SDWLResult res;

    HTTPRequest httpreq = GetHeartBeatQuery(m_heartbeat);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetHeartBeatInfo: response=%s\n", jsonreturn.c_str());

    try
    {
        RMCCORE::RetValue ret = m_heartbeat.ImportFromRMSResponse(jsonreturn);
        if (!ret)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
        m_watermark = m_heartbeat.GetDefaultWatermarkSetting();
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "Heartbeat query JSON response is not correct");
        CELOG_RETURN_VAL_T(res);
    }

    SyncTenantPreference();
    CELOG_RETURN_VAL_T(res);
}

//SDWLResult CSDRmUser::GetAllPolicyBundle(std::unordered_map<std::string, std::string>& policymp)
//{
//	SDWLResult res;
//
//	policymap policyconfigs = m_heartbeat.GetAllPolicyConfigs();
//	std::for_each(policyconfigs.cbegin(), policyconfigs.cend(), [&](const auto & item) {
//		std::string tenantid = item.first;
//		RMPolicyConfig config = item.second;
//		std::string policyBundle = config.GetPolicyBundle();
//		policymp[tenantid] = policyBundle;
//	});
//	return res;
//}

//bool CSDRmUser::GetPolicyBundle(const std::wstring& tenantName, std::string& policyBundle)
//{
//	bool ret = true;
//	
//	RMPolicyConfig config;
//	if (!m_heartbeat.GetPolicyConfig(NX::conv::to_string(tenantName), config))
//	{
//		CELOG_LOG(CELOG_ERROR, L"GetPolicyConfig failed, tenantName=%s\n", tenantName.c_str());
//		// return false;
//	}
//	policyBundle = config.GetPolicyBundle();
//
//	std::wstring defaulttenant = conv::to_wstring(GetTenant().GetTenant());
//	if (tenantName != defaulttenant)
//	{
//		RMPolicyConfig masterconfig;
//		if (!m_heartbeat.GetPolicyConfig(NX::conv::to_string(defaulttenant), masterconfig))
//		{
//			CELOG_LOG(CELOG_ERROR, L"GetPolicyConfig failed, tenantName=%s\n", defaulttenant.c_str());
//		}
//		else
//		{
//			policyBundle += masterconfig.GetPolicyBundle();
//		}
//	}
//
//	return ret;
//}

const SDR_WATERMARK_INFO CSDRmUser::GetWaterMarkInfo()
{
	SDR_WATERMARK_INFO info;

	info.text = m_watermark.getText();
	info.fontColor = m_watermark.GetFontColor();
	info.fontName = m_watermark.getFontName();
	info.fontSize = m_watermark.getFontSize();
	info.repeat = m_watermark.getRepeat();
	info.rotation = (WATERMARK_ROTATION)m_watermark.getRotation();
	info.transparency = m_watermark.getTransparency();

	return info;
}

SDWLResult CSDRmUser::GetListProjects(uint32_t pageId, uint32_t pageSize, const std::string& orderBy, RM_ProjectFilter filter)
{
    CELOG_ENTER;
    SDWLResult res;

    RMMyProjects * myProjects = GetMyProjects();
    HTTPRequest httpreq = myProjects->GetListProjectsQuery(pageId, pageSize, orderBy, ProjectFilter(filter));
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetListProjects: response=%s\n", jsonreturn.c_str());

    try {
        RMCCORE::RetValue ret = myProjects->ImportFromRMSResponse(jsonreturn);
        if (!ret)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "GetListProjectsQuery JSON response is not correct");
    }

    m_projectsInfo.clear();
    for (size_t i = 0; i < myProjects->GetProjectNumber(); i++)
    {
        const RMProject* project = myProjects->GetProject((uint32_t)i);
        SDR_PROJECT_INFO info;
        info.projid = project->GetProjectID();
        info.parenttenantid = project->GetParentTenantId();
        info.parenttenantname = project->GetParentTenantName();
        info.tokengroupname = project->GetTokenGroupName();
        info.name = project->GetProjectName();
        info.displayname = project->GetDisplayName();
        info.description = project->GetDescription();
        info.totalfiles = project->GetTotalFiles();
        info.bowner = project->IsOwnbyMe();
        m_projectsInfo.push_back(info);
    }

    GetListProjectAdmins();

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetListProjectAdmins()
{
    // we assume GetListProjects is called before this call
    CELOG_ENTER;
    SDWLResult res;

    RMMyProjects * myProjects = GetMyProjects();
    m_ProjectAdminList.clear();
    for (size_t i = 0; i < myProjects->GetProjectNumber(); i++)
    {
        const RMProject* project = myProjects->GetProject((uint32_t)i);
        HTTPRequest httpreq = myProjects->GetProjectAdminQuery(GetDefaultTenantId());
        StringBodyRequest request(httpreq);

        Client restclient(NXRMC_CLIENT_NAME, true);
        restclient.Open();

        std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

        StringResponse response;
        res = spConn->SendRequest(request, response);

        if (!res)
            CELOG_RETURN_VAL_T(res);

        const std::string& jsonreturn = response.GetBody();
        CELOG_LOGA(CELOG_DEBUG, "GetProjectAdmins: response=%s\n", jsonreturn.c_str());

        try {
            auto root = nlohmann::json::parse(jsonreturn);
            if (!root.is_object())
            {
                continue;
            }

            int status = root.at("statusCode");
            std::string message = root.at("message");
            if (status != http::status_codes::OK.id) {
                continue;
            }

            const nlohmann::json& response = root.at("results");
            const nlohmann::json& projectAdmin = response.at("projectAdmin");
            if (projectAdmin.is_array())
            {
                std::vector<std::string> admins;
                for (auto it = projectAdmin.begin(); it != projectAdmin.end(); it++)
                {
                    //bool btenantAdmin = false;
                    //if ((*it).end() == (*it).find("tenantAdmin"))
                        //continue;

                    //btenantAdmin = (*it)["tenantAdmin"].get<bool>();
                    if ((*it).end() != (*it).find("email")) // if (btenantAdmin == false && ((*it).end() != (*it).find("email")))
                    {
                        admins.push_back((*it)["email"].get<std::string>());
                    }
                }

                std::pair<std::string, std::vector<std::string>> tadmins;
                tadmins.first = project->GetTokenGroupName();
                tadmins.second = admins;
                m_ProjectAdminList.push_back(tadmins);
            }
        }
        catch (std::exception &e) {
            std::string strError = "JSON response is not correct, error : " + std::string(e.what());
            res = RESULT2(ERROR_INVALID_DATA, strError);
        }
        catch (...) {
            res = RESULT2(ERROR_INVALID_DATA, "Update recipient query JSON response is not correct");
        }
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetTenantAdmins()
{
    CELOG_ENTER;
    SDWLResult res;

    if (GetDefaultTenantId().size() <= 0)
        CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "default tenant is not initialized!"));

    RMMyProjects* projects = GetMyProjects();
    if (projects)
    {
        HTTPRequest httpreq = projects->GetProjectAdminQuery(GetDefaultTenantId());
        StringBodyRequest request(httpreq);

        Client restclient(NXRMC_CLIENT_NAME, true);
        restclient.Open();

        std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

        StringResponse response;
        res = spConn->SendRequest(request, response);

        if (!res)
            CELOG_RETURN_VAL_T(res);

        const std::string& jsonreturn = response.GetBody();
        CELOG_LOGA(CELOG_DEBUG, "GetProjectAdmins: response=%s\n", jsonreturn.c_str());

        try {
            auto root = nlohmann::json::parse(jsonreturn);
            if (!root.is_object())
            {
                CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
            }

            int status = root.at("statusCode");
            std::string message = root.at("message");
            if (status != http::status_codes::OK.id) {
                CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
            }

            const nlohmann::json& response = root.at("results");
            const nlohmann::json& projectAdmin = response.at("projectAdmin");
            if (projectAdmin.is_array())
            {
                std::vector<std::string> admins;
                for (auto it = projectAdmin.begin(); it != projectAdmin.end(); it++)
                {
                    bool btenantAdmin = false;
                    if ((*it).end() == (*it).find("tenantAdmin"))
                        continue;

                    btenantAdmin = (*it)["tenantAdmin"].get<bool>();
                    if (btenantAdmin && ((*it).end() != (*it).find("email")))
                    {
                        admins.push_back((*it)["email"].get<std::string>());
                    }
                }

                m_TenantAdminList = admins;
            }

            CELOG_RETURN_VAL_T(RESULT2(0, NX::conv::to_string(message)));
        }
        catch (std::exception &e) {
            std::string strError = "JSON response is not correct, error : " + std::string(e.what());
            res = RESULT2(ERROR_INVALID_DATA, strError);
        }
        catch (...) {
            res = RESULT2(ERROR_INVALID_DATA, "Update recipient query JSON response is not correct");
        }
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetProjectListFiles(const uint32_t projectid, uint32_t pageId, uint32_t pageSize, const std::string& orderBy,
	                                      const std::string& pathId, const std::string& searchString, std::vector<SDR_PROJECT_FILE_INFO>& listfiles)
{
    CELOG_ENTER;
    SDWLResult res;

    RMMyProjects * myProjects = GetMyProjects();
    size_t	no = myProjects->GetProjectNumber();
    RMProject* project = NULL;
    for (uint32_t i = 0; i < no; i++)
    {
        project = (RMProject*)myProjects->GetProject(i);
        if (project->GetProjectID() == projectid)
            break;
    }
    if (project == NULL)
        CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "project id is incorrect"));

    HTTPRequest httpreq = project->GetListFilesQuery(projectid, pageId, pageSize, orderBy, pathId, searchString);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetProjectListFiles: response=%s\n", jsonreturn.c_str());

    try
    {
        RMProjectFiles projectfiles;
        RMCCORE::RetValue ret = projectfiles.ImportFromRMSResponse(jsonreturn);
        if (!ret)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

        std::vector<PROJECT_FILE_INFO>* infovec = projectfiles.GetProjectFileInfo();
        SDR_PROJECT_FILE_INFO sinfo;
        for (PROJECT_FILE_INFO info : *infovec)
        {
            sinfo.m_fileid = info.m_fileid;
            sinfo.m_duid = info.m_duid;
            sinfo.m_pathdisplay = info.m_pathdisplay;
            sinfo.m_pathid = info.m_pathid;
            sinfo.m_nxlname = info.m_nxlname;
            sinfo.m_lastmodified = info.m_lastmodified;
            sinfo.m_created = info.m_creationtime;
            sinfo.m_size = info.m_size;
            sinfo.m_bfolder = info.m_folder;
            sinfo.m_ownerdisplayname = info.m_displayname;
            sinfo.m_owneremail = info.m_owneremail;
            sinfo.m_ownerid = info.m_userid;
            listfiles.push_back(sinfo);
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "List files query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

std::string FilterTypeToString(RM_FILTER_TYPE filter)
{
    static const std::map<RM_FILTER_TYPE, std::string>  mapFilter{
        {RM_FILTER_TYPE::E_ALLFILES, "allFiles"},
        {RM_FILTER_TYPE::E_ALLSHARED, "allShared"},
        {RM_FILTER_TYPE::E_REVOKED, "revoked"}
    };

    auto it = mapFilter.find(filter);
    if (mapFilter.end() != it)
        return it->second;

    return "";
}

SDWLResult CSDRmUser::ProjectListFile(
    const uint32_t projectId,
    uint32_t pageId,
    uint32_t pageSize,
    const std::string& orderBy,
    const std::string& pathId,
    const std::string& searchString,
    RM_FILTER_TYPE filter,
    std::vector<SDR_PROJECT_SHARED_FILE>& listfiles)
{
    CELOG_ENTER;
    SDWLResult res;

    std::string strFilter = FilterTypeToString(filter);
    if (strFilter.empty())
    {
        CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "strFilter is empty."));
    }

    RMMyProjects * myProjects = GetMyProjects();
    ::EnterCriticalSection(&_listprojectfiles_lock);

    HTTPRequest httpreq = myProjects->GetListFileExQuery(projectId,
        pageId, pageSize, orderBy, pathId, searchString, strFilter);

    StringBodyRequest request(httpreq);
    ::LeaveCriticalSection(&_listprojectfiles_lock);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "ProjectListFile: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        listfiles.clear();
        const nlohmann::json& response = root.at("results");
        const nlohmann::json& detail = response.at("detail");
        const nlohmann::json& files = detail.at("files");
        auto it = files.begin();
        while (files.end() != it)
        {
            SDR_PROJECT_SHARED_FILE item;
            item.m_id = (*it).at("id").get<std::string>();

            if ((*it).end() != (*it).find("duid"))
            {//folder no this item
                item.m_duid = (*it).at("duid").get<std::string>();
            }

            item.m_pathdisplay = (*it).at("pathDisplay").get<std::string>();
            item.m_pathid = (*it).at("pathId").get<std::string>();
            item.m_name = (*it).at("name").get<std::string>();

            if ((*it).end() != (*it).find("fileType"))
            {//folder no this item
                item.m_filetype = (*it).at("fileType").get<std::string>();
            }

            item.m_lastmodified = (*it).at("lastModified").get<uint64_t>();
            item.m_creationtime = (*it).at("creationTime").get<uint64_t>();
            item.m_size = (*it).at("size").get<uint64_t>();

            item.m_folder = (*it).at("folder").get<bool>();
            item.m_isshared = (*it).at("isShared").get<bool>();
            item.m_revoked = (*it).at("revoked").get<bool>();

            if ((*it).end() != (*it).find("owner"))
            {
                item.m_owner.m_userid = (*it)["owner"].at("userId").get<uint32_t>();
                item.m_owner.m_displayname = (*it)["owner"].at("displayName").get<std::string>();
                item.m_owner.m_email = (*it)["owner"].at("email").get<std::string>();
            }

            if ((*it).end() != (*it).find("lastModifiedUser"))
            {
                item.m_lastmodifieduser.m_userid = (*it)["lastModifiedUser"].at("userId").get<uint32_t>();
                item.m_lastmodifieduser.m_displayname = (*it)["lastModifiedUser"].at("displayName").get<std::string>();

                if ((*it)["lastModifiedUser"].end() != (*it)["lastModifiedUser"].find("email"))
                {//RMS server format issue, qa will check with server develop
                    item.m_lastmodifieduser.m_email = (*it)["lastModifiedUser"].at("email").get<std::string>();
                }
            }

            if ((*it).end() != (*it).find("shareWithProject"))
            {
                item.m_sharewithproject = (*it)["shareWithProject"].get<std::vector<uint32_t>>();
            }

            listfiles.push_back(item);
            it++;
        }

        return RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "GetListFileExQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

void To_FileRights(const std::vector<std::string>& vecRights, std::vector<SDRmFileRight>& outRights)
{
    static const std::map<std::string, SDRmFileRight>  mapRight{
        {RIGHT_ACTION_VIEW, RIGHT_VIEW},
        {RIGHT_ACTION_EDIT, RIGHT_EDIT},
        {RIGHT_ACTION_PRINT, RIGHT_PRINT},
        {RIGHT_ACTION_CLIPBOARD, RIGHT_CLIPBOARD},
        {RIGHT_ACTION_SAVEAS, RIGHT_SAVEAS},
        {RIGHT_ACTION_DECRYPT, RIGHT_DECRYPT},
        {RIGHT_ACTION_SCREENCAP, RIGHT_SCREENCAPTURE},
        {RIGHT_ACTION_SEND, RIGHT_SEND},
        {RIGHT_ACTION_CLASSIFY, RIGHT_CLASSIFY},
        {RIGHT_ACTION_SHARE, RIGHT_SHARE},
        {RIGHT_ACTION_DOWNLOAD, RIGHT_DOWNLOAD},
        {OBLIGATION_NAME_WATERMARK, RIGHT_WATERMARK}
    };

    outRights.clear();
    auto it = vecRights.begin();
    while (vecRights.end() != it)
    {
        auto itFind = mapRight.find(*it);
        if (mapRight.end() != itFind)
        {
            outRights.push_back(itFind->second);
        }
        it++;
    }
}

SDWLResult CSDRmUser::ProjectListSharedWithMeFiles(
    uint32_t projectId,
    uint32_t pageId,
    uint32_t pageSize,
    const std::string& orderBy,
    const std::string& searchString,
    std::vector<SDR_PROJECT_SHAREDWITHME_FILE>& listfiles)
{
    CELOG_ENTER;
    SDWLResult res;

    RMMyProjects * myProjects = GetMyProjects();
    HTTPRequest httpreq = myProjects->GetSharedWithMeFilesQuery(projectId,
        pageId, pageSize, orderBy, searchString);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "ProjectListSharedWithMeFiles: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        listfiles.clear();
        const nlohmann::json& response = root.at("results");
        const nlohmann::json& detail = response.at("detail");
        const nlohmann::json& files = detail.at("files");
        auto it = files.begin();
        while (files.end() != it)
        {
            SDR_PROJECT_SHAREDWITHME_FILE item;
            item.m_duid = (*it).at("duid").get<std::string>();
            item.m_name = (*it).at("name").get<std::string>();
            item.m_size = (*it).at("size").get<uint64_t>();
            item.m_filetype = (*it).at("fileType").get<std::string>();
            item.m_shareddate = (*it).at("sharedDate").get<uint64_t>();
            item.m_sharedby = (*it).at("sharedBy").get<std::string>();
            item.m_transactionid = (*it).at("transactionId").get<std::string>();
            item.m_transactioncode = (*it).at("transactionCode").get<std::string>();

            if ((*it).end() != (*it).find("comment"))
            {
                item.m_comment = (*it)["comment"].get<std::string>();
            }

            if ((*it).end() != (*it).find("rights"))
            {
                std::vector<std::string> vec;
                vec = (*it)["rights"].get<std::vector<std::string>>();
                To_FileRights(vec, item.m_rights);
            }

            item.m_isowner = (*it).at("isOwner").get<bool>();
            item.m_protectedtype = (*it).at("protectionType").get<uint32_t>();
            item.m_sharedbyproject = (*it).at("sharedByProject").get<std::string>();

            listfiles.push_back(item);
            it++;
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "GetListFileExQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectDownloadSharedWithMeFile(
    uint32_t projectId,
    const std::wstring& transactionCode,
    const std::wstring& transactionId,
    std::wstring& targetFolder,
    bool forViewer)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);

    std::wstring tmpFilePath = L"c:\\";
    res = ProjectCreateTempFile((conv::to_wstring("sharedwithme")), transactionId, tmpFilePath);
    RMMyProjects * myProjects = GetMyProjects();
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    HTTPRequest httpreq;
    httpreq = myProjects->GetDownloadSharedWithMeFilesQuery(projectId,
        conv::to_string(transactionCode), conv::to_string(transactionId), forViewer);

    StringBodyRequest request(httpreq);
    FileResponse  response(tmpFilePath);
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    std::wstring outFilePath;
    std::wstring preferredFileName;
    unsigned short status = response.GetStatus();
    const std::wstring  phrase = response.GetPhrase();

    if (status != 200) {
        res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
        CELOG_RETURN_VAL_T(res);
    }

    response.Finish();
    const HttpHeaders& headers = response.GetHeaders();
    auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
        return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
    });
    if (it != headers.end())
    {
        auto pos = (*it).second.find(L"UTF-8''");
        if (pos != std::wstring::npos) {
            preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
        }
    }

	NX::fs::dos_fullfilepath output_filepath(std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + preferredFileName));
    if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(output_filepath.global_dos_path().c_str())) {
        CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
    }

    if (!CopyFile(tmpFilePath.c_str(), output_filepath.global_dos_path().c_str(), false))
    {
        res = RESULT2(GetLastError(), "CopyFile failed");
    }

    // Everything is done, call openFile to let Container known this file, 
    // at least preare token for offline mode
    ISDRmNXLFile * pIgnored = NULL;
    OpenFile(output_filepath.path(), &pIgnored);
    if (pIgnored != NULL)
        CloseFile(pIgnored);
    DeleteFile(tmpFilePath.c_str());
    targetFolder = output_filepath.path();

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectPartialDownloadSharedWithMeFile(
    uint32_t projectId,
    const std::wstring& transactionCode,
    const std::wstring& transactionId,
    std::wstring& targetFolder,
    bool forViewer)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);

    std::wstring tmpFilePath = L"c:\\";
    res = ProjectCreateTempFile((conv::to_wstring("sharedwithme")), transactionId, tmpFilePath);
    RMMyProjects * myProjects = GetMyProjects();
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    HTTPRequest httpreq;
    httpreq = myProjects->GetDownloadSharedWithMeFilesQuery(projectId,
        conv::to_string(transactionCode), conv::to_string(transactionId), forViewer);

    StringBodyRequest request(httpreq);
    FileResponse  response(tmpFilePath);
    res = spConn->SendRequest(request, response, INFINITE, 0x4000);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    std::wstring outFilePath;
    std::wstring preferredFileName;
    unsigned short status = response.GetStatus();
    const std::wstring  phrase = response.GetPhrase();

    if (status != 200) {
        res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
        CELOG_RETURN_VAL_T(res);
    }

    response.Finish();
    const HttpHeaders& headers = response.GetHeaders();
    auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
        return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
    });
    if (it != headers.end())
    {
        auto pos = (*it).second.find(L"UTF-8''");
        if (pos != std::wstring::npos) {
            preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
        }
    }

	NX::fs::dos_fullfilepath output_filepath(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + L"partial_" + preferredFileName);
    if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(output_filepath.global_dos_path().c_str())) {
        CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
    }

    if (!CopyFile(tmpFilePath.c_str(), output_filepath.global_dos_path().c_str(), false))
    {
        res = RESULT2(GetLastError(), "CopyFile failed");
    }

    // Everything is done, call openFile to let Container known this file,
    // at least preare token for offline mode
    ISDRmNXLFile * pIgnored = NULL;
    OpenFile(output_filepath.path(), &pIgnored);
    if (pIgnored != NULL)
        CloseFile(pIgnored);
    DeleteFile(tmpFilePath.c_str());
    targetFolder = output_filepath.path();

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectReshareSharedWithMeFile(
    uint32_t projectId,
    const std::string& transactionId,
    const std::string& transactionCode,
    const std::string emaillist,
    const std::vector<uint32_t>& recipients,
    SDR_PROJECT_RESHARE_FILE_RESULT& result)
{
    CELOG_ENTER;
    SDWLResult res;

    RMMyProjects * myProjects = GetMyProjects();
    HTTPRequest httpreq = myProjects->GetSharedWithMeReShareQuery(transactionId,
        transactionCode, emaillist, projectId, recipients);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "ProjectReshareSharedWithMeFile: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& response = root.at("results");
        result.m_protectiontype = response.at("protectionType").get<uint32_t>();

        if (response.end() != response.find("newTransactionId"))
        {
            result.m_newtransactionid = response["newTransactionId"].get<std::string>();
        }

        if (response.end() != response.find("newSharedList"))
        {
            result.m_newsharedlist = response["newSharedList"].get<std::vector<uint32_t>>();
        }

        if (response.end() != response.find("alreadySharedList"))
        {
            result.m_alreadysharedlist = response["alreadySharedList"].get<std::vector<uint32_t>>();
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectUpdateSharedFileRecipients(
    const std::string& duid,
    const std::map<std::string, std::vector<uint32_t>>& recipients,
    const std::string& comment,
    std::map<std::string, std::vector<uint32_t>>& results)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);

    RMMyProjects * myProjects = GetMyProjects();
    HTTPRequest httpreq = myProjects->GetUpdateRecipientsQuery(duid, recipients, comment);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "ProjectUpdateSharedFileRecipients: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        results.clear();
        const nlohmann::json& response = root.at("results");
        if (response.end() != response.find("alreadySharedList"))
        {
            results["alreadySharedList"] = response["alreadySharedList"].get<std::vector<uint32_t>>();
        }

        if (response.end() != response.find("newRecipients"))
        {
            results["newRecipients"] = response["newRecipients"].get<std::vector<uint32_t>>();
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "Update recipient query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectShareFile(
    uint32_t projectId,
    const std::vector<uint32_t> &recipients,
    const std::string &fileName,
    const std::string &filePathId,
    const std::string &filePath,
    const std::string &comment,
    SDR_PROJECT_SHARE_FILE_RESULT& result)
{
    CELOG_ENTER;
    SDWLResult res;

    std::string memberShipId = GetMembershipID(projectId);
    uint32_t spaceId = 1;
    RMMyProjects * myProjects = GetMyProjects();
    HTTPRequest httpreq = myProjects->GetShareFileQuery(recipients, fileName,
        filePathId, filePath, comment, memberShipId, spaceId, projectId);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetProjectListFiles: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& response = root.at("results");
        result.m_filename = response.at("fileName").get<std::string>();
        result.m_duid = response.at("duid").get<std::string>();
        result.m_filepathid = response.at("filePathId").get<std::string>();

        if (response.end() != response.find("transactionId"))
        {//this item just in the first time call
            result.m_transactionid = response["transactionId"].get<std::string>();
        }

        if (response.end() != response.find("newSharedList"))
        {
            result.m_newsharedlist = response["newSharedList"].get<std::vector<uint32_t>>();
        }

        if (response.end() != response.find("alreadySharedList"))
        {
            result.m_alreadysharedlist = response["alreadySharedList"].get<std::vector<uint32_t>>();
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "Update recipient query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectRevokeSharedFile(const std::string& duid)
{
    CELOG_ENTER;
    SDWLResult res;

    RMMyProjects * myProjects = GetMyProjects();
    HTTPRequest httpreq = myProjects->GetSharedFileRevokeQuery(duid);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "ProjectRevokeSharedFile: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "GetSharedFileRevokeQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectFileIsExist(
    uint32_t projectId,
    const std::string& pathId,
    bool& bExist)
{
    CELOG_ENTER;
    bExist = false;
    SDWLResult res;

    RMMyProjects * myProjects = GetMyProjects();
    HTTPRequest httpreq = myProjects->GetProjectFileIsExistQuery(projectId, pathId);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetProjectFileIsExistQuery: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& results = root.at("results");
        if (results.end() != results.find("fileExists"))
        {
            bExist = results["fileExists"].get<bool>();
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "GetProjectFileIsExistQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectGetNxlFileHeader(
    uint32_t projectId,
    const std::string& pathId,
    std::string& targetFolder)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);

    std::wstring tmpFilePath = L"c:\\";
    res = ProjectCreateTempFile(std::to_wstring(projectId), conv::to_wstring(pathId), tmpFilePath);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    unsigned short status = 0;
    {
        RMMyProjects* project = GetMyProjects();
        HTTPRequest httpreq = project->GetProjectNxlFileHeaderQuery(projectId, pathId);
        StringBodyRequest request(httpreq);
        FileResponse  response(tmpFilePath);
        res = spConn->SendRequest(request, response, INFINITE);

        if (!res)
            CELOG_RETURN_VAL_T(res);

        status = response.GetStatus();
        response.Finish();
        if (200 == status)
        {
            res = SaveFileToTargetFolder(pathId, tmpFilePath, targetFolder, response.GetHeaders());
        }
    }

    if (404 == status)
    {
        RMMyProjects* projects = GetMyProjects();
        HTTPRequest httpreq = projects->GetDownloadFileQuery(projectId, pathId, ProjectFileDownloadType::PFDTForViewer);
        StringBodyRequest request(httpreq);
        FileResponse  response(tmpFilePath);
        res = spConn->SendRequest(request, response, INFINITE, 0x4000);

        status = response.GetStatus();
        if (!res)
            CELOG_RETURN_VAL_T(res);

        response.Finish();
        if (200 == status)
        {
            res = SaveFileToTargetFolder(pathId, tmpFilePath, targetFolder, response.GetHeaders());
        }
    }

    if (status != 200) {
        res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
        CELOG_RETURN_VAL_T(res);
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ProjectFileOverwrite(
    uint32_t projectid,
    const std::wstring &parentPathId,
    ISDRmNXLFile* file,
    bool overwrite)
{
    CELOG_ENTER;

    SDWLResult res = RESULT(0);
    CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

    UpdateNXLMetaData(file, false);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    RMMyProjects* project = GetMyProjects();
    HTTPRequest httpreq = project->GetProjectFileOverwriteQuery(NX::itos<char>((int)projectid), conv::to_string(parentPathId), *nxlFile, overwrite);

    NXLUploadRequest filerequest(httpreq, conv::to_wstring(nxlFile->GetFilePath()), RMS::boundary::End);
    StringResponse response;
    res = spConn->SendRequest(filerequest, response);

    if (!res)
    {
        CELOG_RETURN_VAL_T(res);
    }

    RMCCORE::RetValue ret = nxlFile->ImportFromRMSResponse(response.GetBody());

    CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
}

SDWLResult CSDRmUser::GetRepositories(std::vector<SDR_REPOSITORY>& vecRepository)
{
    CELOG_ENTER;

    SDWLResult res;
    vecRepository.clear();

    RMRepositories& repositories = GetMyRepositories();
    HTTPRequest httpreq = repositories.GetRepositoriesQuery();
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetRepositoriesQuery : response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        nlohmann::json& results = root.at("results");
        nlohmann::json& repoItems = results.at("repoItems");
        for (auto& obj : repoItems)
        {
            SDR_REPOSITORY item;
            item.m_repoId = NX::conv::utf8toutf16(obj.at("repoId").get<std::string>());
            item.m_name = NX::conv::utf8toutf16(obj.at("name").get<std::string>());
            item.m_type = NX::conv::utf8toutf16(obj.at("type").get<std::string>());

            item.m_isShared = obj.at("isShared").get<bool>();
            item.m_isDefault = obj.at("isDefault").get<bool>();

            if (obj.end() != obj.find("accountName"))
            {// Local Driver without this item
                item.m_accountName = NX::conv::utf8toutf16(obj["accountName"].get<std::string>());
            }

            if (obj.end() != obj.find("accountId"))
            {// Local Driver sometimes without this item
                item.m_accountId = NX::conv::utf8toutf16(obj["accountId"].get<std::string>());
            }

            if (obj.end() != obj.find("token"))
            {// Local Driver sometimes without this item
                item.m_token = NX::conv::utf8toutf16(obj["token"].get<std::string>());
            }

            if (obj.end() != obj.find("preference"))
            {// Local Driver has this item
                item.m_preference = NX::conv::utf8toutf16(obj["preference"].get<std::string>());
            }

            item.m_createTime = obj.at("creationTime").get<uint64_t>();
            if (obj.end() != obj.find("updatedTime"))
            {
                item.m_updatedTime = obj["updatedTime"].get<uint64_t>();
            }

			if (obj.end() != obj.find("providerClass"))
			{
				item.m_providerClass = NX::conv::utf8toutf16(obj["providerClass"].get<std::string>());
			}
			else {
				item.m_providerClass = L"PERSONAL";
			}

            vecRepository.push_back(item);
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetRepositoriesQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetRepositoryAccessToken(
    const std::wstring& repoId,
    std::wstring& accessToken)
{
    CELOG_ENTER;

	if (repoId == L"") {
		SDWLResult res = RESULT2(ERROR_INVALID_DATA, "repoId is empty");
		return res;
	}

    accessToken.clear();

    SDWLResult res;
    std::string strRepoId = NX::conv::utf16toutf8(repoId);

    RMRepositories& repositories = GetMyRepositories();
    HTTPRequest httpreq = repositories.GetAccessTokenQuery(strRepoId);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetAccessTokenQuery : response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        nlohmann::json& results = root.at("results");
        accessToken = NX::conv::utf8toutf16(results.at("accessToken").get<std::string>());

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetAccessTokenQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetRepositoryAuthorizationUrl(
    const std::wstring& type,
    const std::wstring& name,
    std::wstring& authURL)
{
    CELOG_ENTER;
    SDWLResult res;
    std::string strRepositoryType = NX::conv::utf16toutf8(type);
    std::string strName = NX::conv::utf16toutf8(name);
    std::string strAuthURL = NX::conv::utf16toutf8(authURL);

    RMRepositories& repositories = GetMyRepositories();
    HTTPRequest httpreq = repositories.GetAuthorizationUrlQuery(strRepositoryType, strName, strAuthURL);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetAuthorizationUrlQuery: response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        nlohmann::json& results = root.at("results");
        authURL = NX::conv::utf8toutf16(results.at("authURL").get<std::string>());

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetAuthorizationUrlQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::UpdateRepository(
    const std::wstring& repoId,
    const std::wstring& token,
    const std::wstring& name)
{
    CELOG_ENTER;
    SDWLResult res;

    std::string strRepoId = NX::conv::utf16toutf8(repoId);
    std::string strToken = NX::conv::utf16toutf8(token);
    std::string strName = NX::conv::utf16toutf8(name);

    RMRepositories& repositories = GetMyRepositories();
    HTTPRequest httpreq = repositories.GetUpdateRepositoryQuery(strRepoId, strToken, strName);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetUpdateRepositoryQuery: response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetUpdateRepositoryQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::RemoveRepository(const std::wstring& repoId)
{
    CELOG_ENTER;
    SDWLResult res;

    std::string strRepoId = NX::conv::utf16toutf8(repoId);
    RMRepositories& repositories = GetMyRepositories();
    HTTPRequest httpreq = repositories.RemoveRepositoryQuery(strRepoId);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "RemoveRepositoryQuery: response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::NoContent.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...)
    {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "RemoveRepositoryQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::AddRepository(
    const std::wstring& name,
    const std::wstring& type,
    const std::wstring& accountName,
    const std::wstring& accountId,
    const std::wstring& token,
    const std::wstring& perference,
    uint64_t createTime,
    bool isShared,
    std::wstring& repoId)
{
    CELOG_ENTER;
    SDWLResult res;

    nlohmann::json root = nlohmann::json::object();
    root["name"] = NX::conv::utf16toutf8(name);
    root["type"] = NX::conv::utf16toutf8(type);
    root["accountName"] = NX::conv::utf16toutf8(accountName);
    root["accountId"] = NX::conv::utf16toutf8(accountId);
    root["token"] = NX::conv::utf16toutf8(token);
    root["preference"] = NX::conv::utf16toutf8(perference);
    root["creationTime"] = createTime;
    root["isShared"] = isShared;

    std::string str = root.dump();
    RMRepositories& repositories = GetMyRepositories();
    HTTPRequest httpreq = repositories.AddRepositoryQuery(str);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "AddRepositoryQuery: response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& results = root.at("results");
        if (results.end() != results.find("repoId"))
        {
            repoId = NX::conv::to_wstring(results["repoId"].get<std::string>());
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...)
    {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "AddRepositoryQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetRepositoryServiceProvider(std::vector<std::wstring>& serviceProvider)
{
    CELOG_ENTER;
    serviceProvider.clear();

    SDWLResult res;
    RMRepositories& repositories = GetMyRepositories();
    HTTPRequest httpreq = repositories.GetServiceProvider();
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetRepositoryServiceProvider: response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS Json Error!"));
        }

        int status = root.at("statusCode").get<int>();
        std::string message = root.at("message").get<std::string>();
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        nlohmann::json& results = root.at("results");
        nlohmann::json& providerList = results.at("configuredServiceProviderSettingList");

        std::vector<std::string> vecProvider = providerList.get<std::vector<std::string>>();
        for (auto& item : vecProvider)
        {
            serviceProvider.push_back(NX::conv::to_wstring(item));
        }

        res = RESULT2(0, message);
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...)
    {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetRepositoryServiceProvider query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyVaultFileIsExist(
    const std::string& pathId,
    bool& bExist)
{
    CELOG_ENTER;
    bExist = false;
    SDWLResult res;

    HTTPRequest httpreq = GetMyVaultFileIsExistQuery(pathId);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetMyVaultFileIsExistQuery: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& results = root.at("results");
        if (results.end() != results.find("fileExists"))
        {
            bExist = results["fileExists"].get<bool>();
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "GetMyVaultFileIsExistQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyVaultGetNxlFileHeader(
    const std::string& pathId,
    std::string& targetFolder)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);

    std::wstring tmpFilePath = L"c:\\";
    res = ProjectCreateTempFile((conv::to_wstring("myvault")), conv::to_wstring(pathId), tmpFilePath);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    unsigned short status = 0;
    {
        FileResponse  response(tmpFilePath);
        HTTPRequest httpreq = GetMyVaultGetNxlFileHeaderQuery(pathId);
        StringBodyRequest request(httpreq);
        res = spConn->SendRequest(request, response, INFINITE);

        if (!res)
            CELOG_RETURN_VAL_T(res);

        status = response.GetStatus();
        response.Finish();
        if (200 == status)
        {
            res = SaveFileToTargetFolder(pathId, tmpFilePath, targetFolder, response.GetHeaders());
        }
    }

    if (404 == status)
    {
        RMMyVault* myvault = GetMyVault();
        HTTPRequest httpreq = myvault->GetMyVaultDownloadFileQuery(pathId, 1);
        FileResponse  response(tmpFilePath);
        StringBodyRequest request(httpreq);
        res = spConn->SendRequest(request, response, INFINITE, 0x4000);

        status = response.GetStatus();
        if (!res)
            CELOG_RETURN_VAL_T(res);

        response.Finish();
        if (200 == status)
        {
            res = SaveFileToTargetFolder(pathId, tmpFilePath, targetFolder, response.GetHeaders());
        }
    }

    if (status != 200) {
        res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
        CELOG_RETURN_VAL_T(res);
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::MyDriveListFiles(const std::wstring& pathId, std::vector<SDR_MYDRIVE_FILE_INFO>& listfiles)
{
	CELOG_ENTER;
	SDWLResult res;

	RMMyDrive * mydrive = GetMyDrive();

	HTTPRequest httpreq = mydrive->GetMyDriveFilesQuery(conv::to_string(pathId));
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();

	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	StringBodyRequest request(httpreq);
	StringResponse response;
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	const std::string& jsonreturn = response.GetBody();
	CELOG_LOGA(CELOG_DEBUG, "GetMyDriveFiles: response=%s\n", jsonreturn.c_str());

	try
	{
		RMCCORE::RetValue ret = mydrive->ImportFromRMSListFilesResponse(jsonreturn);
		if (!ret)
			CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

		std::vector<MYDRIVE_FILE_INFO>* infovec = mydrive->GetMyDriveFileInfos();
		SDR_MYDRIVE_FILE_INFO sinfo;
		for (MYDRIVE_FILE_INFO info : *infovec)
		{
			sinfo.m_pathid = info.m_pathid;
			sinfo.m_pathdisplay = info.m_pathdisplay;
			sinfo.m_name = info.m_name;
			sinfo.m_lastmodified = info.m_lastmodified;
			sinfo.m_size = info.m_size;
			sinfo.m_bfolder = info.m_bfolder;
			listfiles.push_back(sinfo);
		}
	}
	catch (...) {
		// The JSON data is NOT correct
		res = RESULT2(ERROR_INVALID_DATA, "GetMyDriveFilesQuery query JSON response is not correct");
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetMyVaultFiles(uint32_t pageId, uint32_t pageSize, const std::string& orderBy,
	const std::string& searchString, std::vector<SDR_MYVAULT_FILE_INFO>& listfiles)
{
    CELOG_ENTER;
    SDWLResult res;

    RMMyVault * myvault = GetMyVault();

    HTTPRequest httpreq = myvault->GetMyVaultFilesQuery(pageId, pageSize, orderBy, searchString);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetMyVaultFiles: response=%s\n", jsonreturn.c_str());

    try
    {
        RMCCORE::RetValue ret = myvault->ImportFromRMSResponse(jsonreturn);
        if (!ret)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

        std::vector<MYVAULT_FILE_INFO>* infovec = myvault->GetMyVaultFileInfos();
        SDR_MYVAULT_FILE_INFO sinfo;
        for (MYVAULT_FILE_INFO info : *infovec)
        {
            sinfo.m_pathid = info.m_pathid;
            sinfo.m_pathdisplay = info.m_pathdisplay;
            sinfo.m_repoid = info.m_repoid;
            sinfo.m_duid = info.m_duid;
            sinfo.m_nxlname = info.m_nxlname;
            sinfo.m_lastmodified = info.m_lastmodified;
            sinfo.m_creationtime = info.m_creationtime;
            sinfo.m_sharedon = info.m_sharedon;
            sinfo.m_sharedwith = info.m_sharedwith;
            sinfo.m_size = info.m_size;
            sinfo.m_bdeleted = info.m_bdeleted;
            sinfo.m_brevoked = info.m_brevoked;
            sinfo.m_bshared = info.m_bshared;
            sinfo.m_sourcerepotype = info.m_sourcerepotype;
            sinfo.m_sourcefilepathdisplay = info.m_sourcefilepathdisplay;
            sinfo.m_sourcefilepathid = info.m_sourcefilepathid;
            sinfo.m_sourcereponame = info.m_sourcereponame;
            sinfo.m_sourcerepoid = info.m_sourcerepoid;

            listfiles.push_back(sinfo);
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "MyVault files query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetSharedWithMeFiles(uint32_t pageId, uint32_t pageSize, const std::string& orderBy,
	const std::string& searchString, std::vector<SDR_SHAREDWITHME_FILE_INFO>& listfiles)
{
    CELOG_ENTER;
    SDWLResult res;

    RMSharedWithMe * sharedwithme = GetSharedWithMe();

    HTTPRequest httpreq = sharedwithme->GetSharedWithMeFilesQuery(pageId, pageSize, orderBy, searchString);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetSharedWithMeFiles: response=%s\n", jsonreturn.c_str());

    try
    {
        RMCCORE::RetValue ret = sharedwithme->ImportFromRMSResponse(jsonreturn);
        if (!ret)
            CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

        std::vector<SHAREDWITHME_FILE_INFO>* infovec = sharedwithme->GetSharedWithMeFileInfos();
        SDR_SHAREDWITHME_FILE_INFO sinfo;
        for (SHAREDWITHME_FILE_INFO info : *infovec)
        {
            sinfo.m_duid = info.m_duid;
            sinfo.m_nxlname = info.m_nxlname;
            sinfo.m_filetype = info.m_filetype;
            sinfo.m_size = info.m_size;
            sinfo.m_shareddate = info.m_shareddate;
            sinfo.m_sharedby = info.m_sharedby;
            sinfo.m_transactionid = info.m_transactionid;
            sinfo.m_transactioncode = info.m_transactioncode;
            sinfo.m_sharedlink = info.m_sharedlink;
            sinfo.m_rights = info.m_rights;
            sinfo.m_comments = info.m_comments;
            sinfo.m_isowner = info.m_isowner;

            listfiles.push_back(sinfo);
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "MyVault files query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetWorkspaceFiles(
    uint32_t pageId, 
    uint32_t pageSize, 
    const std::string& path, 
    const std::string& orderBy,
	const std::string& searchString, 
    std::vector<SDR_WORKSPACE_FILE_INFO>& listfiles)
{
    CELOG_ENTER;
    SDWLResult res;

    listfiles.clear();

    std::string _searchString = NX::conv::UrlEncode(searchString);
    RMWorkspace * workspace = GetWorkspace();
    HTTPRequest httpreq = workspace->GetWorkspaceFilesQuery(pageId, pageSize, path, orderBy, _searchString);
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringBodyRequest request(httpreq);
    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetWorkspaceFiles: response=%s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(ERROR_BAD_FORMAT, "RMS Response JSON error!"));
        }

        int status = root.at("statusCode").get<int>();
        std::string message = root.at("message").get<std::string>();
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2((RMCCORE_RMS_ERRORCODE_BASE + status), message));
        }

        nlohmann::json& result = root.at("results");
        nlohmann::json& detail = result.at("detail");
        nlohmann::json& files = detail.at("files");
        for (auto it = files.begin(); files.end() != it; it++)
        {
            SDR_WORKSPACE_FILE_INFO info;

            info.m_fileid = (*it).at("id").get<std::string>();

            if ((*it).end() != (*it).find("duid"))
            {//folder without duid field
                info.m_duid = (*it).at("duid").get<std::string>();
            }

            info.m_pathdisplay = (*it).at("pathDisplay").get<std::string>();
            info.m_pathid = (*it).at("pathId").get<std::string>();
            info.m_nxlname = (*it).at("name").get<std::string>();

            if ((*it).end() != (*it).find("fileType"))
            {//folder without fileType field
                info.m_filetype = (*it).at("fileType").get<std::string>();
            }
            info.m_lastmodified = (*it).at("lastModified").get<uint64_t>();
            info.m_created = (*it).at("creationTime").get<uint64_t>();
            info.m_size = (*it).at("size").get<uint64_t>();
            info.m_bfolder = (*it).at("folder").get<bool>();

            if ((*it).end() != (*it).find("uploader"))
            {
                nlohmann::json& owner = (*it)["uploader"];
                info.m_ownerid = owner.at("userId").get<uint32_t>();
                info.m_ownerdisplayname = owner.at("displayName").get<std::string>();
                info.m_owneremail = owner.at("email").get<std::string>();
            }

            if ((*it).end() != (*it).find("lastModifiedUser"))
            {
                nlohmann::json& owner = (*it)["lastModifiedUser"];
                info.m_modifiedby = owner.at("userId").get<uint32_t>();
                info.m_modifiedbyname = owner.at("displayName").get<std::string>();

                if (owner.end() != owner.find("email"))
                {//folder without email field
                    info.m_modifiedbyemail = owner["email"].get<std::string>();
                }
            }

            listfiles.push_back(info);
        }
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "GetWorkspaceFilesQuery JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::WorkspaceFileIsExist(
    const std::string& pathId,
    bool& bExist)
{
    CELOG_ENTER;
    bExist = false;
    SDWLResult res;

    RMWorkspace* workspace = GetWorkspace();
    HTTPRequest httpreq = workspace->GetWorkspaceFileIsExistQuery(pathId);
    StringBodyRequest request(httpreq);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();

    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    StringResponse response;
    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetWorkspaceFileIsExistQuery: response=%s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& results = root.at("results");
        if (results.end() != results.find("fileExists"))
        {
            bExist = results["fileExists"].get<bool>();
        }

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "GetWorkspaceFileIsExistQuery query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::WorkspaceGetNxlFileHeader(
    const std::string& pathId,
    std::string& targetFolder)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);

    std::wstring tmpFilePath = L"c:\\";
    res = ProjectCreateTempFile((conv::to_wstring("workspace")), conv::to_wstring(pathId), tmpFilePath);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    unsigned short status = 0;
    {
        RMWorkspace* workspace = GetWorkspace();
        HTTPRequest httpreq = workspace->GetWorkspaceNxlFileHeaderQuery(pathId);
        StringBodyRequest request(httpreq);
        FileResponse  response(tmpFilePath);
        res = spConn->SendRequest(request, response, INFINITE);

        if (!res)
            CELOG_RETURN_VAL_T(res);

        status = response.GetStatus();
        response.Finish();
        if (200 == status)
        {
            res = SaveFileToTargetFolder(pathId, tmpFilePath, targetFolder, response.GetHeaders());
        }
    }

    if (404 == status)
    {
        RMWorkspace* workspace = GetWorkspace();
        HTTPRequest httpreq = workspace->GetWorkspaceDownloadFileQuery(pathId, 1);
        StringBodyRequest request(httpreq);
        FileResponse  response(tmpFilePath);
        res = spConn->SendRequest(request, response, INFINITE, 0x4000);

        status = response.GetStatus();
        if (!res)
            CELOG_RETURN_VAL_T(res);

        response.Finish();
        if (200 == status)
        {
            res = SaveFileToTargetFolder(pathId, tmpFilePath, targetFolder, response.GetHeaders());
        }
    }

    if (status != 200) {
        res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
        CELOG_RETURN_VAL_T(res);
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::WorkspaceFileOverwrite(
    const std::wstring& destfolder,
    ISDRmNXLFile* file,
    bool overwrite)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);
    CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

    UpdateNXLMetaData(file, false);

    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    std::string strParentPathId = conv::to_string(destfolder);
    std::transform(strParentPathId.begin(), strParentPathId.end(), strParentPathId.begin(), ::tolower);

    RMWorkspace* workspace = GetWorkspace();
    HTTPRequest httpreq = workspace->GetWorkspaceFileOverwriteQuery(strParentPathId, *nxlFile, overwrite);

    NXLUploadRequest filerequest(httpreq, conv::to_wstring(nxlFile->GetFilePath()), RMS::boundary::End);
    StringResponse response;
    res = spConn->SendRequest(filerequest, response);

    if (!res)
    {
        CELOG_RETURN_VAL_T(res);
    }

    RMCCORE::RetValue ret = nxlFile->ImportFromRMSResponse(response.GetBody());
    res = RESULT2(ret.GetCode(), ret.GetMessage());

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::WorkspaceDownloadFile(const std::string& pathid, std::wstring& targetFolder, uint32_t downloadtype)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile((conv::to_wstring("workspace")), conv::to_wstring(pathid), tmpFilePath);
	RMWorkspace * workspace = GetWorkspace();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = workspace->GetWorkspaceDownloadFileQuery(pathid, downloadtype);
	StringBodyRequest request(httpreq);
	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response);

	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::wstring outFilePath;
	std::wstring preferredFileName;
	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
		CELOG_RETURN_VAL_T(res);
	}

	response.Finish();
	const HttpHeaders& headers = response.GetHeaders();
	auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
		return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
	});
	if (it != headers.end())
	{
		auto pos = (*it).second.find(L"UTF-8''");
		if (pos != std::wstring::npos) {
			preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
		}
	}

	NX::fs::dos_fullfilepath output_filepath(std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + preferredFileName), false);
	if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(output_filepath.global_dos_path().c_str())) {
		CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
	}
	if (!CopyFile(tmpFilePath.c_str(), output_filepath.global_dos_path().c_str(), false))
	{
		res = RESULT2(GetLastError(), "CopyFile failed");
	}

	// Everything is done, call openFile to let Container known this file, 
	// at least preare token for offline mode
	ISDRmNXLFile * pIgnored = NULL;
	OpenFile(output_filepath.path(), &pIgnored);
	if (pIgnored != NULL)
		CloseFile(pIgnored);
	DeleteFile(tmpFilePath.c_str());
	targetFolder = output_filepath.path();

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ChangeRightsOfWorkspaceFile(const std::wstring &originalNXLfilePath, const std::string &fileName, const std::string &parentPathId,
	const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags)
{
	//if (originalNXLfilePath.find(m_localFile.GetWorkFolder(), 0) != std::wstring::npos)
	//	return RESULT2(SDWL_ACCESS_DENIED, "you can't change new protected file");

	// check user permission to see whether user has Admin Rights permission
	if (false == HasAdminRights(originalNXLfilePath))
		return RESULT2(SDWL_ACCESS_DENIED, "no permission to change file rights");

    std::string ttags;
    if (!IsFileTagValid(tags, ttags))
    {
        std::string message = "Invalid Json format : " + tags;
        return RESULT2(SDWL_INVALID_DATA, message);
    }

	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(originalNXLfilePath, (ISDRmNXLFile**)&file);
	if (!res) {
		return res;
	}

	std::string orgtags = file->GetTags();
	std::string orgpolicy = file->GetPolicy();

	// get and check Token
	RMToken token = file->GetToken();
	if (token.GetKey().size() == 0) {
		// invalid token for some reason
		CloseFile(file);
		return RESULT2(SDWL_INTERNAL_ERROR, "fail to get token");
	}

	RMActivityLog log;
	NXLAttributes attr;
	NXLFMT::Obligations obs;
	NXLFMT::Rights nxlrights = ToFileRight(rights);

	if (watermarkinfo.text.size() > 0) {
		OBLIGATION::Watermark waterm(watermarkinfo.text, watermarkinfo.fontName, watermarkinfo.fontColor, watermarkinfo.fontSize,
			watermarkinfo.transparency, OBLIGATION::Watermark::Rotation(watermarkinfo.rotation), watermarkinfo.repeat);
		obs.push_back(waterm);
	}
	attr.obs = &obs;
	NXLFMT::Expiration expiry((NXLFMT::Expiration::ExpirationType)expire.type, expire.start, expire.end);
	attr.rights = &nxlrights;
	attr.expiry = &expiry;
	attr.tags = ttags;

	std::string tenant = file->GetTenantName();
	std::string memberid = GetMembershipIDByTenantName(tenant);

	// close the original NXL file before we want to re-protect and overwrite
	CloseFile(file);

    unsigned char org_header16k[16 * 1024];
    RMLocalFile org_rawfile(originalNXLfilePath);
    org_rawfile.Open();
    uint32_t org_hsize = org_rawfile.read(0, org_header16k, 16 * 1024);
    org_rawfile.Close();
    if (org_hsize != 16 * 1024)
    {
        res = RESULT2(SDWL_INTERNAL_ERROR, "failed to read file header");
        return res;
    }

	RMNXLFile newnxlfile("");
	NX::fs::dos_fullfilepath output_filepath(originalNXLfilePath);
	RetValue value = ReProtectLocalFileHeader(output_filepath.global_dos_path(), token, 0, attr, conv::to_wstring(memberid), L"", log, newnxlfile, true, false);
	if (!value)
	{
		return RESULT(value);
	}
	newnxlfile.Close();

	// After change rights, need to tell RMS
	res = ClassifyWorkspaceFile(originalNXLfilePath, fileName, parentPathId, ttags);
	if (!res)
	{
        std::fstream out(originalNXLfilePath, std::ios::in | std::ios::binary | std::ios::out);
        if (out.good())
        {
            out.seekg(0, std::ios_base::beg);
            out.write((const char*)org_header16k, 16 * 1024);

            CELOG_LOGA(CELOG_DEBUG, "UpdateNXLMetaDataEx failed and change file header back\n");
        }
        out.close();
        return res;
	}

	return RESULT(0);
}

SDWLResult CSDRmUser::ClassifyWorkspaceFile(const std::wstring &nxlfilepath, const std::string &fileName, const std::string &parentPathId, const std::string &fileTags)
{
    CELOG_ENTER;
    CSDRmNXLFile *nxlFile = NULL;
    SDWLResult res = OpenFileForMetaData(nxlfilepath, (ISDRmNXLFile**)&nxlFile);
    if (res)
    {
        CloseFile(nxlFile);

        HTTPRequest httpreq = ClassifyWorkspaceFileQuery(fileName, parentPathId, fileTags);
        StringBodyRequest request(httpreq);
        Client restclient(NXRMC_CLIENT_NAME, true);
        restclient.Open();

        std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

        StringResponse response;
        res = spConn->SendRequest(request, response);
        if (!res)
            CELOG_RETURN_VAL_T(res);

        const std::string& jsonreturn = response.GetBody();
        CELOG_LOGA(CELOG_DEBUG, "ClassifyProjectFile: response=%s\n", jsonreturn.c_str());

        try
        {
            nlohmann::json root = nlohmann::json::parse(jsonreturn);
            if (!root.is_object())
            {
                CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json data is not correct!"));
            }

            int status = root.at("statusCode").get<int>();
            std::string message = root.at("message").get<std::string>();
            if (status != http::status_codes::OK.id) {
                // error from RMS, discard this one and continue
                res = RESULT2(SDWL_RMS_ERRORCODE_BASE + status, message);
                CELOG_RETURN_VAL_T(res);
            }

            res = RESULT2(0, message);
        }
        catch (...) {
            // The JSON data is NOT correct
            res = RESULT2(ERROR_INVALID_DATA, "Log query JSON response is not correct");
            CELOG_RETURN_VAL_T(res);
        }
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::UploadWorkspaceFile(const std::wstring&destfolder, ISDRmNXLFile * file, bool overwrite)
{
	SDWLResult res = RESULT(0);
	CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

	UpdateNXLMetaData(file, false);

	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = GetProtectWorkspaceFileQuery(conv::to_string(destfolder), *nxlFile, overwrite);

	NXLUploadRequest filerequest(httpreq, conv::to_wstring(nxlFile->GetFilePath()), RMS::boundary::End);
	StringResponse response;
	res = spConn->SendRequest(filerequest, response);

	if (!res)
		return res;

	//if (response.GetStatus() != 200)
	//    res = RESULT2(response.GetStatus(), "");
	RMCCORE::RetValue ret = nxlFile->ImportFromRMSResponse(response.GetBody());
	if (!ret)
		res = RESULT2(ret.GetCode(), ret.GetMessage());

	return res;
}

SDWLResult CSDRmUser::GetWorkspaceFileMetadata(const std::string& pathid, SDR_FILE_META_DATA& metadata)
{
    SDWLResult res;
    CELOG_ENTER;
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::string _pathid = pathid;
    NX::tolower(_pathid);
    // this is project file
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    RMCCORE::HTTPRequest httpreq = GetWorkspaceFileMetadataQuery(_pathid);
    StringBodyRequest request(httpreq);
    StringResponse response;

    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetWorkspaceFileMetadataQuery: response= %s\n", jsonreturn.c_str());

    try
    {
        nlohmann::json root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
            CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid JSON data"));

        int status = root.at("statusCode").get<int>();
        std::string message = root.at("message").get<std::string>();
        if (status != http::status_codes::OK.id)
            CELOG_RETURN_VAL_T(RESULT2(status, message));

        nlohmann::json& result = root.at("results");
        nlohmann::json& fileInfo = result.at("fileInfo");

        metadata.name = fileInfo.at("name").get<std::string>();
        metadata.pathDisplay = fileInfo.at("pathDisplay").get<std::string>();
        metadata.pathid = fileInfo.at("pathId").get<std::string>();

        if (fileInfo.end() != fileInfo.find("tags"))
        {
            metadata.tags = fileInfo["tags"].dump();
        }

        metadata.lastmodify = fileInfo.at("lastModified").get<uint64_t>();
        metadata.protectionType = fileInfo.at("protectionType").get<uint32_t>();

        if (fileInfo.end() != fileInfo.find("owner"))
        {
            metadata.owner = fileInfo["owner"].get<bool>();
        }

        metadata.nxl = fileInfo.at("nxl").get<bool>();

        SDR_Expiration expiration;
        if (fileInfo.end() != fileInfo.find("expiry"))
        {
            nlohmann::json& expiry = fileInfo["expiry"];
            if (expiry.end() != expiry.find("endDate"))
            {
                expiration.end = expiry["endDate"].get<uint64_t>();
            }

            expiration.type = ABSOLUTEEXPIRE;
            if (expiry.end() != expiry.find("startDate"))
            {
                expiration.start = expiry["startDate"].get<uint64_t>();
                expiration.type = RANGEEXPIRE;
            }
        }
        metadata.expiration = expiration;

        res = RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = res = RESULT2(ERROR_INVALID_DATA, "List files query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

const std::string CSDRmUser::GetMembershipID(uint32_t projectid)
{
	std::string membershipid = "";

	for (size_t i = 0; i < m_memberships.size(); i++)
	{
		RMMembership membership = m_memberships[i];
		if (membership.GetProjectID() == projectid)
		{
			membershipid = membership.GetID();
			break;
		}
	}

	if (membershipid.size() == 0 && m_defaultmembership.GetProjectID() == projectid)
		return m_defaultmembership.GetID();

	return membershipid;
}

const std::string CSDRmUser::GetMembershipIDByTenantName(const std::string tenant)
{
	std::string membershipid = "";

	if (m_defaultmembership.GetID().find(tenant) != std::string::npos)
		return m_defaultmembership.GetID();

	for (size_t i = 0; i < m_memberships.size(); i++)
	{
		RMMembership membership = m_memberships[i];
		if (membership.GetID().find(tenant) != std::string::npos)
		{
			membershipid = membership.GetID();
			break;
		}
	}

	return membershipid;
}

const std::string CSDRmUser::GetMembershipID(const std::string tokengroupname)
{
	std::string membershipid = "";

	if (m_defaultmembership.GetTokenGroupName() == tokengroupname)
		return m_defaultmembership.GetID();

	for (size_t i = 0; i < m_memberships.size(); i++)
	{
		RMMembership membership = m_memberships[i];
		if (membership.GetTokenGroupName() == tokengroupname)
		{
			membershipid = membership.GetID();
			break;
		}
	}

	return membershipid;
}

SDWLResult CSDRmUser::GetClassification(const std::string &tenantid, std::vector<SDR_CLASSIFICATION_CAT>& cats)
{
	CELOG_ENTER;

	// Network Error, use the cached one
	RMPolicyConfig config;
	if (!m_heartbeat.GetPolicyConfig(NX::conv::to_string(tenantid), config))
	{
		CELOG_LOG(CELOG_ERROR, L"GetPolicyConfig failed, tenantName=%s\n", tenantid.c_str());
		CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
	}
	std::vector<CLASSIFICATION_CAT> _cats = config.GetClassificationCategories();
	cats = CopyClassification(_cats);

	CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
}

std::vector<SDR_CLASSIFICATION_CAT> CSDRmUser::CopyClassification(std::vector<CLASSIFICATION_CAT>& classif)
{
	SDR_CLASSIFICATION_LABELS slabel;
	std::vector<SDR_CLASSIFICATION_LABELS> slabels;
	SDR_CLASSIFICATION_CAT scat;
	std::vector<SDR_CLASSIFICATION_CAT> sclass;
	for (CLASSIFICATION_CAT cate : classif)
	{
		slabels.clear();

		for (CLASSIFICATION_LABELS lab : cate.labels)
		{
			slabel.name = lab.name;
			slabel.allow = lab.allow;
			slabels.push_back(slabel);
		}
		scat.labels = slabels;
		scat.mandatory = cate.mandatory;
		scat.multiSelect = cate.multiSelect;
		scat.name = cate.name;

		sclass.push_back(scat);
	}
	return sclass;
}

bool CSDRmUser::CheckRights(const std::wstring& nxlfilepath, SDRmFileRight right)
{
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rights;
	SDWLResult res = GetRights(nxlfilepath, rights);
	if (!res)
		return false;

	for (size_t i = 0; i < rights.size(); i++)
	{
		SDRmFileRight cur = rights[i].first;
		if (cur == right) {
			return true;
		}
	}

	return false;
}

SDWLResult SkyDRM::CSDRmUser::GetFingerPrintWithoutToken(const std::wstring & nxlFilePath, SDR_NXL_FILE_FINGER_PRINT & fingerPrint)
{
	CELOG_ENTER;
	CELOG_LOGW(CELOG_INFO, L"nxlfilepath=%s\n", nxlFilePath.c_str());
	SDWLResult res;
	NX::fs::dos_fullfilepath input_filepath(nxlFilePath);
	RMNXLFile file(input_filepath.global_dos_path());

	file.Open();
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> tmp;
	std::wstring name;
	uint64_t size = 0;

	bool isOwner = false;
	bool isFromMyVault = false;
	bool isFromProject = false;
	bool isFromSystemBucket = false;
	uint32_t projectId = -1;
	std::string fileOwnerId = file.GetOwnerID();
	std::string fileTenantName;
	isFromSystemBucket = fileTenantName == GetSystemProjectTenant();

	// ugly code right? but it is the only way by now to judge whether file is issued by adhoc-policy
	bool isByAdhoc = false;
	bool isByCentrol = false;

	std::string watermark = "";
	std::vector <SDRmFileRight> rights;
	std::string tags = file.GetTags();
	SDR_Expiration expiration;
	std::string duid = file.GetDuid();

	file.Close();
	//
	// end prepare params
	//

	bool hasAdminRights = false;

	// Prepare outer param
	{
		fingerPrint.name = name;
		fingerPrint.fileSize = size;
		fingerPrint.localPath = nxlFilePath;
		fingerPrint.isOwner = isOwner;
		fingerPrint.isFromMyVault = isFromMyVault;
		fingerPrint.isFromProject = isFromProject;
		fingerPrint.isFromSystemBucket = isFromSystemBucket;
		fingerPrint.projectId = projectId;
		fingerPrint.isByAdHocPolicy = isByAdhoc;
		fingerPrint.IsByCentrolPolicy = isByCentrol;
		fingerPrint.rights = rights;// get the evaulated rights both for adhoc or centrl
		fingerPrint.adHocWatermar = conv::utf8toutf16(watermark);
		fingerPrint.tags = conv::utf8toutf16(tags); // by default
		fingerPrint.expiration = expiration;
		fingerPrint.hasAdminRights = hasAdminRights;
		fingerPrint.duid = conv::utf8toutf16(duid);
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult SkyDRM::CSDRmUser::GetFingerPrint(const std::wstring& nxlFilePath, SDR_NXL_FILE_FINGER_PRINT& fingerPrint, bool doOwnerCheck)
{
	CELOG_ENTER;
	CELOG_LOGW(CELOG_INFO, L"nxlfilepath=%s\n", nxlFilePath.c_str());
	SDWLResult res;
	CSDRmNXLFile *file = NULL;
	res = OpenFile(nxlFilePath, (ISDRmNXLFile**)&file);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}

	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> tmp;
	res = GetRights(nxlFilePath, tmp, doOwnerCheck);
	if (!res) {
		CloseFile(file);
		file = NULL;
		CELOG_RETURN_VAL_T(res);
	}
	//
	// prepare params
	//
	std::wstring name= file->GetFileName();
	uint64_t size = file->GetFileSize();

	bool isOwner = false; 
	bool isFromMyVault = false;
	bool isFromProject = false;
	bool isFromSystemBucket = false;
	uint32_t projectId = -1;
	std::string fileOwnerId = file->GetOwnerID();
	std::string fileTenantName = file->GetTenantName();
	{
		// make sure whether owner, file orginal source
		if (fileOwnerId == m_defaultmembership.GetID()) {
			isOwner = true;
			isFromMyVault = true;
		}
		else if (m_defaultmembership.GetID().find(fileTenantName) != std::string::npos) {
			isFromMyVault = true;
		}
		else {
			
			// maybe file from project
			std::vector<RMMembership> ms = m_memberships; // copy one, avoid multithread ops
			std::for_each(ms.begin(), ms.end(),
				[&fileOwnerId,&isFromProject,&isOwner,&projectId,&fileTenantName]
				(RMMembership& cur) {
					if (cur.GetID() == fileOwnerId) {
						isFromProject = true;
						projectId = cur.GetProjectID();
					}
					else if (cur.GetID().find(fileTenantName) != std::string::npos)
					{
						isFromProject = true;
						projectId = cur.GetProjectID();
					}
				}
			);			
		}
	}

	isFromSystemBucket = fileTenantName == GetSystemProjectTenant();

	// ugly code right? but it is the only way by now to judge whether file is issued by adhoc-policy
	bool isByAdhoc = file->GetRights().size() != 0 ? true : false;
	bool isByCentrol = !isByAdhoc;

	std::string watermark = file->GetAdhocWaterMarkString();
	if (isByAdhoc && doOwnerCheck)
	{
#pragma region MyRegion
		int begin = -1;
		int end = -1;
		std::string converted_wmtext;
		std::string wmtext = watermark;
		for (size_t i = 0; i < wmtext.size(); i++)
		{
			if (wmtext.data()[i] == '$')
			{
				if (begin >= 0)
				{
					std::string s2 = wmtext.substr(begin, (i - begin));
					converted_wmtext.append(s2);
				}
				begin = (int)i;
			}
			else if (wmtext.data()[i] == ')')
			{
				end = (int)i;
				if (begin >= 0)
				{
					std::string s1 = wmtext.substr(begin, end - begin + 1);
					std::string s2 = wmtext.substr(begin, end - begin + 1);
					std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
					bool has_premacro = false;

					if (s2 == "$(user)")
					{
						converted_wmtext.append(NX::conv::utf16toutf8(GetEmail())); // userDispName
						converted_wmtext.append(" ");
						has_premacro = true;
					}
					else if (s2 == "$(date)")
					{
						std::string strdate;
						time_t rawtime;
						struct tm timeinfo;
						char date_buffer[256] = { 0 };
						char time_buffer[256] = { 0 };
						time(&rawtime);
						localtime_s(&timeinfo, &rawtime);
						strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", &timeinfo);
						strdate = std::string(date_buffer);

						converted_wmtext.append(strdate);
						has_premacro = true;
					}
					else if (s2 == "$(time)")
					{
						std::string strtime;
						time_t rawtime;
						struct tm timeinfo;
						char time_buffer[256] = { 0 };
						time(&rawtime);
						localtime_s(&timeinfo, &rawtime);
						strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &timeinfo);
						strtime = std::string(time_buffer);

						converted_wmtext.append(strtime);
						has_premacro = true;
					}
					else if (s2 == "$(break)")
					{
						converted_wmtext.append("\n");
						has_premacro = true;
					}

					if (!has_premacro)
					{
						converted_wmtext.append(s1);
					}
				}
				else
				{
					// wrong format
					converted_wmtext.push_back(wmtext.data()[i]);
				}
				begin = -1;
				end = -1;
			}
			else
			{
				if (begin < 0)
				{
					converted_wmtext.push_back(wmtext.data()[i]);
				}
			}
		}

#pragma endregion

		watermark = converted_wmtext;
	}
	
	std::vector <SDRmFileRight> rights;
	for (size_t i = 0; i < tmp.size(); i++)
	{
		rights.push_back(tmp[i].first);
	}
	std::string tags = file->GetTags();
	SDR_Expiration expiration = file->GetExpiration();
	std::string duid = file->GetDuid();

	//
	// end prepare params
	//

	// release res
	CloseFile(file);
	file = NULL;

	bool hasAdminRights = HasAdminRights(nxlFilePath);

	// Prepare outer param
	{
		fingerPrint.name = name;
		fingerPrint.fileSize = size;
		fingerPrint.localPath = nxlFilePath;
		fingerPrint.isOwner = isOwner;
		fingerPrint.isFromMyVault = isFromMyVault;
		fingerPrint.isFromProject = isFromProject;
		fingerPrint.isFromSystemBucket = isFromSystemBucket;
		fingerPrint.projectId = projectId;
		fingerPrint.isByAdHocPolicy = isByAdhoc;
		fingerPrint.IsByCentrolPolicy = isByCentrol;
		fingerPrint.rights = rights;// get the evaulated rights both for adhoc or centrl
		fingerPrint.adHocWatermar = conv::utf8toutf16(watermark);
		fingerPrint.tags = conv::utf8toutf16(tags); // by default
		fingerPrint.expiration = expiration;
		fingerPrint.hasAdminRights = hasAdminRights;
		fingerPrint.duid = conv::utf8toutf16(duid);
	}

	CELOG_RETURN_VAL_T(res);
}


SDWLResult CSDRmUser::GetNxlFileOriginalExtention(const std::wstring &nxlFilePath, std::wstring &oriFileExtention) {
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"GetNxlFileOriginalExtention %s\n", nxlFilePath.c_str());
	SDWLResult res;
	CSDRmNXLFile *file = NULL;
	res = OpenFile(nxlFilePath, (ISDRmNXLFile**)&file);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}
	oriFileExtention = conv::utf8toutf16(file->GetOrgFileExtension());
	return res;
}


SDWLResult CSDRmUser::GetRights(const std::wstring& nxlfilepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"nxlfilepath=%s\n", nxlfilepath.c_str());

	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(nxlfilepath, (ISDRmNXLFile**)&file);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}

	// policy section is not empty, it is a Ad-hoc policy file
	std::vector<SDRmFileRight> rights;
	std::vector<SDR_WATERMARK_INFO> vwinfo;
	RMCCORE::OBLIGATION::Watermark watermark;

	rights = file->GetRights();
	if (rights.size() > 0) {
		
		bool isAdhocOwner = false;
		if (file->GetOwnerID() == this->m_defaultmembership.GetID())
			isAdhocOwner = true;

		// for ad-hoc file, the show file right is same with protected

		/*if (doOwnerCheck && isAdhocOwner) {
				RIGHT_VIEW = 1,
				RIGHT_EDIT = 2,
				RIGHT_PRINT = 4,
				RIGHT_CLIPBOARD = 8,
				RIGHT_SAVEAS = 0x10,
				RIGHT_DECRYPT = 0x20,
				RIGHT_SCREENCAPTURE = 0x40,
				RIGHT_SEND = 0x80,
				RIGHT_CLASSIFY = 0x100,
				RIGHT_SHARE = 0x200,
				RIGHT_DOWNLOAD = 0x400,
				RIGHT_WATERMARK = 0x40000000
			rights.clear();
			rights.push_back(SDRmFileRight::RIGHT_VIEW);
			rights.push_back(SDRmFileRight::RIGHT_EDIT);
			rights.push_back(SDRmFileRight::RIGHT_PRINT);
			rights.push_back(SDRmFileRight::RIGHT_CLIPBOARD);
			rights.push_back(SDRmFileRight::RIGHT_SAVEAS);
			rights.push_back(SDRmFileRight::RIGHT_DECRYPT);
			rights.push_back(SDRmFileRight::RIGHT_SCREENCAPTURE);
			rights.push_back(SDRmFileRight::RIGHT_SEND);
			rights.push_back(SDRmFileRight::RIGHT_CLASSIFY);
			rights.push_back(SDRmFileRight::RIGHT_SHARE);
			rights.push_back(SDRmFileRight::RIGHT_DOWNLOAD);
		}*/

		if (file->getWatermark(watermark)) {
			SDR_WATERMARK_INFO winfo;
			winfo.fontColor = watermark.GetFontColor();
			winfo.fontName = watermark.getFontName();
			winfo.fontSize = watermark.getFontSize();
			winfo.repeat = watermark.getRepeat();
			winfo.rotation = (WATERMARK_ROTATION)watermark.getRotation();
			winfo.text = watermark.getText();
			winfo.transparency = watermark.getTransparency();
			vwinfo.push_back(winfo);
		}

		for (size_t i = 0; i < rights.size(); i++) {
			std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> randw;
			randw.first = rights.at(i);
			randw.second = vwinfo;

			rightsAndWatermarks.push_back(randw);
		}

		// remove token if no rights in ad-hoc policy (revoked)
		if (rightsAndWatermarks.size() == 0)
		{
			if (IsAPIUser() == FALSE) // if API user, shall not delete token
			{
				RemoveCachedToken(file->GetDuid());
				file->ResetToken();
			}
		}

		CloseFile(file);
		CELOG_RETURN_VAL_T(res);
	}

	CloseFile(file);
	// the file is a central policy file
	CELOG_RETURN_VAL_T(GetFileRightsFromCentralPolicies(nxlfilepath, rightsAndWatermarks, doOwnerCheck));
}

SDWLResult CSDRmUser::GetFileRightsFromCentralPolicies(const std::wstring& nxlFilePath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"nxlFilePath=%s\n", nxlFilePath.c_str());
	SDWLResult res;
	NX::fs::dos_fullfilepath input_filepath(nxlFilePath);

	CSDRmNXLFile *file = NULL;
	res = OpenFile(nxlFilePath, (ISDRmNXLFile**) &file);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}

	// Check if it is an Ad-hoc policy file
	uint64_t nxlRights = file->GetNXLRights();
	if (nxlRights != 0) {
		// It is an Ad-hoc policy file
		CloseFile(file);
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "File is an Ad-hoc Policy file."));
	}

	res = PDPEvaluateRights(file->GetTenantName(), file->GetTags(), conv::utf8toutf16(file->GetFilePath()), rightsAndWatermarks, doOwnerCheck);

	// remove token if no rights in central policy
	if (rightsAndWatermarks.size() == 0)
	{
		if (IsAPIUser() == FALSE) // if API user, shall not delete token
		{
			RemoveCachedToken(file->GetDuid());
			file->ResetToken();
		}
	}

	CloseFile(file);
	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetResourceRightsFromCentralPolicies(const std::wstring& resourceName, const std::wstring& resourceType, const std::vector<std::pair<std::wstring, std::wstring>> &attrs, std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> &rightsAndObligations)
{
    CELOG_ENTER;
    CELOG_LOG(CELOG_INFO, L"resouceName=%s, resourceType=%s\n", resourceName.c_str(), resourceType.c_str());

    std::string bundle = "";
    //bool bRet = GetPolicyBundle(conv::to_wstring(GetTenant().GetTenant()), bundle);
    //if (!bRet) {
    //    CELOG_LOG(CELOG_ERROR, L"No policy bundle for current tenant %hs found.\n", GetTenant().GetTenant().c_str());
    //    CELOG_RETURN_VAL_T(RESULT2(SDWL_NOT_FOUND, "No policy bundle for current tenant found."));
    //}

	std::string userIdpid = "";
	std::vector<std::pair<std::wstring, std::wstring>> attributes;
	for (auto it = m_attributes.begin(); m_attributes.end() != it; it++)
	{
		if ((*it).first.compare(USERIDP_ID_KEY_NAME) == 0)
		{
			userIdpid = (*it).second;
		}
		std::wstring first = NX::conv::to_wstring((*it).first);
		std::wstring second = NX::conv::to_wstring((*it).second);
		attributes.push_back(std::pair<std::wstring, std::wstring>(first, second));
	}

	// find user id
	std::string userid = "";
	for (auto it = m_attributes.begin(); m_attributes.end() != it; it++)
	{
		if ((*it).first.compare(userIdpid) == 0)
		{
			userid = (*it).second;
			break;
		}
	}

	// if user id not exist, use email
	if (userid == "")
	{
		userid = conv::to_string(GetEmail());
	}

	std::wstring rtype = resourceType;
	// get default tenant resource type
	bool tenantbRet = false;
	std::string tenantResourcetype;
	if (rtype.empty())
	{
		tenantbRet = m_heartbeat.GetTokenGroupResourceType(conv::to_string(GetTenant().GetTenant()), tenantResourcetype);
		if (!tenantbRet) {
			CELOG_LOG(CELOG_ERROR, L"No resource type for default tenant %hs found.\n", GetTenant().GetTenant().c_str());
		}
		rtype = conv::to_wstring(tenantResourcetype);
	}

    CELOG_RETURN_VAL_T(m_PDP.EvalRights(GetName(), GetEmail(),
        conv::to_wstring(userid), NX::win::get_current_process_path(), resourceName, rtype, attrs, attributes,
        conv::to_wstring(bundle), nullptr, &rightsAndObligations));
}

SDWLResult CSDRmUser::ProjectCreateTempFile(const std::wstring& projectId, const std::wstring& pathId, std::wstring& tmpFilePath)
{
	std::wstring s(pathId + L"@" + projectId);
	NX::tolower<wchar_t>(s);
	UCHAR hash[20] = { 0 };
	RetValue ret = RMCCORE::CRYPT::CreateSha1((const unsigned char*)s.c_str(), (ULONG)(s.length() * 2), hash);
	if (!ret)
		return (RESULT2(ERROR_INVALID_DATA, "CreateSha1 failed"));

	tmpFilePath = GetTempDirectory();
	tmpFilePath.append(L"\\");
	tmpFilePath.append(NX::bintohs<wchar_t>(hash, 20));
	tmpFilePath.append(L".tmp");
	return RESULT(0);
}

std::wstring CSDRmUser::GetTempDirectory()
{
	PWSTR pszPath = NULL;
	std::wstring root;

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &pszPath);
	if (SUCCEEDED(hr)) {
		root = pszPath;
		root += L"\\NextLabs";
		::CreateDirectoryW(root.c_str(), NULL);
		root += L"\\SkyDRM";
		::CreateDirectoryW(root.c_str(), NULL);
	}
	else
	{
		WCHAR wc[256];
		if (!GetCurrentDirectory(256, wc))
			root = L"c:\\";
		else
			root = wc;
	}
	return root;
}

// compared with CheckRights with filepath, we will not call OpenFile
// these are used internally to check file permissions when retrieving tokens in OpenFile
bool CSDRmUser::CheckRights(CSDRmNXLFile *nxlfile, SDRmFileRight right)
{
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> rights;
	SDWLResult res = GetRights(nxlfile, rights);
	if (!res)
		return false;

	for (size_t i = 0; i < rights.size(); i++)
	{
		SDRmFileRight cur = rights[i].first;
		if (cur == right) {
			return true;
		}
	}

	return false;
}

SDWLResult CSDRmUser::GetRights(CSDRmNXLFile *nxlfile, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks)
{
	CELOG_ENTER;
	CELOG_LOGA(CELOG_INFO, "nxlfilepath=%s\n", nxlfile->GetFilePath().c_str());

	SDWLResult res;
	uint64_t frights = nxlfile->GetNXLRights();

	if (frights > 0) {
		// policy section is not empty, it is a Ad-hoc policy file
		std::vector<SDRmFileRight> rights;
		std::vector<SDR_WATERMARK_INFO> vwinfo;
		RMCCORE::OBLIGATION::Watermark watermark;

		// for ad-hoc file, it is in MyVault, file owner has full rights
		bool isAdhocOwner = false;
		if (nxlfile->GetOwnerID() == this->m_defaultmembership.GetID())
			isAdhocOwner = true;

		if (isAdhocOwner) {
			// current user is the file owner
			//	RIGHT_VIEW = 1,
			//	RIGHT_EDIT = 2,
			//	RIGHT_PRINT = 4,
			//	RIGHT_CLIPBOARD = 8,
			//	RIGHT_SAVEAS = 0x10,
			//	RIGHT_DECRYPT = 0x20,
			//	RIGHT_SCREENCAPTURE = 0x40,
			//	RIGHT_SEND = 0x80,
			//	RIGHT_CLASSIFY = 0x100,
			//	RIGHT_SHARE = 0x200,
			//	RIGHT_DOWNLOAD = 0x400,
			//	RIGHT_WATERMARK = 0x40000000
			rights.clear();
			rights.push_back(SDRmFileRight::RIGHT_VIEW);
			rights.push_back(SDRmFileRight::RIGHT_EDIT);
			rights.push_back(SDRmFileRight::RIGHT_PRINT);
			rights.push_back(SDRmFileRight::RIGHT_CLIPBOARD);
			rights.push_back(SDRmFileRight::RIGHT_SAVEAS);
			rights.push_back(SDRmFileRight::RIGHT_DECRYPT);
			rights.push_back(SDRmFileRight::RIGHT_SCREENCAPTURE);
			rights.push_back(SDRmFileRight::RIGHT_SEND);
			rights.push_back(SDRmFileRight::RIGHT_CLASSIFY);
			rights.push_back(SDRmFileRight::RIGHT_SHARE);
			rights.push_back(SDRmFileRight::RIGHT_DOWNLOAD);
			// rights.push_back(SDRmFileRight::RIGHT_WATERMARK);
		}
		else
		{
			rights = nxlfile->GetRights();
		}

		if (nxlfile->getWatermark(watermark)) {
			SDR_WATERMARK_INFO winfo;
			winfo.fontColor = watermark.GetFontColor();
			winfo.fontName = watermark.getFontName();
			winfo.fontSize = watermark.getFontSize();
			winfo.repeat = watermark.getRepeat();
			winfo.rotation = (WATERMARK_ROTATION)watermark.getRotation();
			winfo.text = watermark.getText();
			winfo.transparency = watermark.getTransparency();
			vwinfo.push_back(winfo);
		}

		for (size_t i = 0; i < rights.size(); i++) {
			std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> randw;
			randw.first = rights.at(i);
			randw.second = vwinfo;

			rightsAndWatermarks.push_back(randw);
		}
		CELOG_RETURN_VAL_T(res);
	}

	// the file is a central policy file
	CELOG_RETURN_VAL_T(GetFileRightsFromCentralPolicies(nxlfile, rightsAndWatermarks, true));
}

SDWLResult CSDRmUser::GetFileRightsFromCentralPolicies(CSDRmNXLFile *nxlfile, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_INFO, L"nxlFilePath=%hs\n", nxlfile->GetName().c_str());
	SDWLResult res;

	res = PDPEvaluateRights(nxlfile->GetTenantName(), nxlfile->GetTags(), conv::utf8toutf16(nxlfile->GetFilePath()), rightsAndWatermarks, doOwnerCheck);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetFileRightsFromCentralPolicies(const uint32_t projectid, const std::string &tags, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck)
{
	CELOG_ENTER;
	SDWLResult res;

	std::string tenantname = "";
	for (size_t i = 0; i< m_memberships.size(); i++) {
		if (m_memberships[i].GetProjectID() == projectid)
		{
			tenantname = m_memberships[i].GetID(); // uid@tenant-name
			size_t pos = tenantname.rfind('@');
			if (pos != std::string::npos)
				tenantname = tenantname.substr(pos + 1);
			break;
		}
	}

	CELOG_RETURN_VAL_T(GetFileRightsFromCentralPolicies(tenantname, tags, rightsAndWatermarks, doOwnerCheck));
}


SDWLResult CSDRmUser::GetFileRightsFromCentralPolicies(const std::string &tenantname, const std::string &tags, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck)
{
	CELOG_ENTER;
	SDWLResult res;

	res = PDPEvaluateRights(tenantname, tags, NX::win::get_current_process_path(), rightsAndWatermarks, doOwnerCheck);

	CELOG_RETURN_VAL_T(res);
}

// Note: when merge the rights, we should also consider the rights associated watermark.
void MergeRightsWatermark(const std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& fileTokenGroupRightsWatermark,
	const std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& tenantTokenGroupRightsWatermark,
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& result) {

	for (auto& outer : fileTokenGroupRightsWatermark) {

		bool bFind = false;
		std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> findInner;
		for (auto& inner : tenantTokenGroupRightsWatermark) {
			if (outer.first == inner.first) {
				bFind = true;
				findInner = inner;
				break;
			}
		}

		if (!bFind) {
			result.push_back(outer);
		}
		else {
			// If file token group rights no watermark but tenant token group have watermark, should use the latter.
			if (outer.second.size() == 0 && findInner.second.size() > 0) {
				result.push_back(findInner);
			}
			else {
				result.push_back(outer);
			}
		}

	}

	// Add the item that included in "tenantTokenGroupRightsWatermark" but excluded in "fileTokenGroupRightsWatermark"
	for (auto& one : tenantTokenGroupRightsWatermark) {
		bool bFind = false;
		for (auto& two : fileTokenGroupRightsWatermark) {
			if (one.first == two.first) {
				bFind = true;
				break;
			}
		}

		if (!bFind) {
			result.push_back(one);
		}
	}
}

SDWLResult CSDRmUser::PDPEvaluateRights(const std::string &filetokenGroup, const std::string &filetags, const std::wstring &resourcename, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck)
{
	CELOG_ENTER;
	SDWLResult fileRes;
	SDWLResult tenantRes;

	// Read tags
	const std::vector<std::pair<std::wstring, std::wstring>> attrs = NX::conv::ImportJsonTags(filetags);

	std::string bundle = "";

	std::string userIdpid = "";
	std::vector<std::pair<std::wstring, std::wstring>> attributes;
	for (auto it = m_attributes.begin(); m_attributes.end() != it; it++)
	{
		if ((*it).first.compare(USERIDP_ID_KEY_NAME) == 0)
		{
			userIdpid = (*it).second;
		}
		std::wstring first = NX::conv::to_wstring((*it).first);
		std::wstring second = NX::conv::to_wstring((*it).second);
		attributes.push_back(std::pair<std::wstring, std::wstring>(first, second));
	}

	// find user id
	std::string userid = "";
	for (auto it = m_attributes.begin(); m_attributes.end() != it; it++)
	{
		if ((*it).first.compare(userIdpid) == 0)
		{
			userid = (*it).second;
			break;
		}
	}

	// if user id not exist, use email
	if (userid == "")
	{
		userid = conv::to_string(GetEmail());
	}

	// get file tokengroup resource type
	bool filebRet = false;
	std::string tokenGroupResourcetype;
	filebRet = m_heartbeat.GetTokenGroupResourceType(filetokenGroup, tokenGroupResourcetype);
	if (!filebRet) {
		CELOG_LOG(CELOG_ERROR, L"No resource type for tenant name %hs found.\n", filetokenGroup.c_str());
	}

	// get default tenant resource type
	bool tenantbRet = false;
	std::string tenantResourcetype;
	if (filetokenGroup != GetTenant().GetTenant())
	{
		tenantbRet = m_heartbeat.GetTokenGroupResourceType(conv::to_string(GetTenant().GetTenant()), tenantResourcetype);
		if (!tenantbRet) {
			CELOG_LOG(CELOG_ERROR, L"No resource type for default tenant %hs found.\n", GetTenant().GetTenant().c_str());
		}
	}

	if (!filebRet && !tenantbRet)
	{
		CELOG_RETURN_VAL_T(RESULT2(SDWL_NOT_FOUND, "No resource type for tenantname and default tenant."));
	}

	// file token group resource type do evaluate right
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> fileRightsAndWatermarks;
	if (filebRet)
	{
		fileRes = m_PDP.EvalRights(GetName(), GetEmail(),
			conv::to_wstring(userid), NX::win::get_current_process_path(), resourcename, conv::to_wstring(tokenGroupResourcetype),
			attrs, attributes, conv::to_wstring(bundle), &fileRightsAndWatermarks, nullptr, doOwnerCheck);
		CELOG_LOG(CELOG_INFO, L"file tokengroup %hs do evaluate rights, resource type is %hs.\n", filetokenGroup.c_str(), tokenGroupResourcetype.c_str());
	}

	// default tenant resource type do evaluate right
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> tenantRightsAndWatermarks;
	if (tenantbRet)
	{
		tenantRes = m_PDP.EvalRights(GetName(), GetEmail(),
			conv::to_wstring(userid), NX::win::get_current_process_path(), resourcename, conv::to_wstring(tenantResourcetype),
			attrs, attributes, conv::to_wstring(bundle), &tenantRightsAndWatermarks, nullptr, doOwnerCheck);
		CELOG_LOG(CELOG_INFO, L"default tenant %hs do evaluate rights, resource type is %hs.\n", GetTenant().GetTenant().c_str(), tenantResourcetype.c_str());
	}

	if (!fileRes && !tenantRes)
	{
		CELOG_RETURN_VAL_T(RESULT2(SDWL_ACCESS_DENIED, "PDP EvalRights failed for file tokengroup and default tenant."));
	}

	MergeRightsWatermark(fileRightsAndWatermarks, tenantRightsAndWatermarks, rightsAndWatermarks);

	CELOG_RETURN_VAL_T(RESULT(0));
}


//SDWLResult CSDRmUser::GetFilePath(const std::wstring &filename, std::wstring &targetfilepath)
//{
//	SDWLResult res;
//
//	if (filename.size() == 0)
//		return RESULT2(SDWL_INVALID_DATA, "File name is empty");
//
//	std::wstring fpath;
//	RMLocalFile localfile(filename);
//	std::wstring fname = NX::conv::to_wstring(localfile.GetFileName());
//	auto pos = filename.rfind(L'\\');
//	fpath = (pos == std::wstring::npos) ? fpath : filename.substr(0, pos);
//	std::wstring folder = m_localFile.GetWorkFolder();
//
//	if (fpath.size())
//	{
//		if (_wcsicmp(folder.c_str(), fpath.c_str()) != 0)
//			return RESULT2(SDWL_INVALID_DATA, "File path is incorrect");
//	}
//
//	if ((CSDRmNXLFile*)m_localFile.GetFile(fname))
//	{
//		targetfilepath = folder + L"\\" + fname;
//		return res;
//	}
//	else
//		return RESULT2(SDWL_NOT_FOUND, "File is not in the working directory or not support");
//
//}

SDWLResult CSDRmUser::GetUserPreference(uint32_t &option, uint64_t &start, uint64_t &end, std::wstring &watermark)
{
    CELOG_ENTER;
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    RMCCORE::HTTPRequest httpreq = GetUserPreferenceQuery();
    StringBodyRequest request(httpreq);
    StringResponse response;

    SDWLResult res = spConn->SendRequest(request, response);

    if (!res) {
        CELOG_RETURN_VAL_T(res);
    }

    const std::string& jsondata = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "UpdateUserPreferenceQuery: response= %s\n", jsondata.c_str());
    try {
        auto root = nlohmann::json::parse(jsondata);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }


        const nlohmann::json& result = root.at("results");
        if (result.end() != result.find("watermark"))
        {
            watermark = NX::conv::to_wstring(result["watermark"].get<std::string>());
        }

        if (result.end() != result.find("expiry"))
        {
            const nlohmann::json& expiry = result["expiry"];
            option = expiry.at("option").get<int>();
            switch (option)
            {
            case 0:
            {
                start = 0;
                end = 0;
            }
            break;

            case 1:
            {
                const nlohmann::json& relativeDay = expiry.at("relativeDay");
                uint32_t year = relativeDay.at("year").get<uint32_t>();
                uint32_t month = relativeDay.at("month").get<uint32_t>();
                uint32_t week = relativeDay.at("week").get<uint32_t>();
                uint32_t day = relativeDay.at("day").get<uint32_t>();

                start = ((uint64_t)year) << 32 | month;
                end = ((uint64_t)week) << 32 | day;
            }
            break;

            case 2:
            {
                start = 0;
                end = expiry.at("endDate").get<int64_t>();
            }
            break;

            case 3:
            {
                start = expiry.at("startDate").get<int64_t>();
                end = expiry.at("endDate").get<int64_t>();
            }
            break;

            default:
                break;
            }
        }

        CELOG_RETURN_VAL_T(RESULT2(0, NX::conv::to_string(message)));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "Update recipient query JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::UpdateUserPreference(const uint32_t option, const uint64_t start, const uint64_t end, const std::wstring watermark)
{
    CELOG_ENTER;
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    RMCCORE::HTTPRequest httpreq = GetUpdateUserPreferenceQuery(option, start, end, watermark);
    StringBodyRequest request(httpreq);
    StringResponse response;

    SDWLResult res = spConn->SendRequest(request, response);

    if (!res) {
        CELOG_RETURN_VAL_T(res);
    }

    const std::string& jsondata = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "UpdateUserPreferenceQuery: response= %s\n", jsondata.c_str());

    unsigned short status = response.GetStatus();
    if (status != http::status_codes::OK.id) {
        CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(response.GetPhrase())));
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetTenantPreference(bool* adhoc, bool* workspace, int* heartbeat, int* sysprojectid, std::string &sysprojecttenantid, const std::string& tenantid)
{
	if (tenantid.size() == 0 || tenantid == this->m_defaultmembership.GetTenantID())
	{
		*adhoc = m_adhoc;
		*workspace = m_workspace;
		*heartbeat = m_heartbeatFrequency;
		*sysprojectid = m_systemProjectId;
		sysprojecttenantid = m_systemProjectTenantId;
	}

	return RESULT2(SDWL_SUCCESS, "");
}

SDWLResult CSDRmUser::SyncTenantPreference(const std::string& tenantid)
{
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));
    // consider tenantid=="", use defualt_tenantid
    RMCCORE::HTTPRequest httpreq = GetTenantPreferenceQuery(
        tenantid.length() == 0 ? this->m_defaultmembership.GetTenantID() : tenantid);
    StringBodyRequest request(httpreq);
    StringResponse response;

    SDWLResult res = spConn->SendRequest(request, response);

    if (!res) {
        return res;
    }

    const std::string& jsondata = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetTenantPreference: response= %s\n", jsondata.c_str());

    try {
        auto root = nlohmann::json::parse(jsondata);
        if (!root.is_object())
        {
            return RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!");
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            return RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message));
        }

        const nlohmann::json& extra = root.at("extra");
        m_adhoc = extra.at("ADHOC_ENABLED").get<bool>();

		if (extra.end() != extra.find("WORKSPACE_ENABLED"))
		{
			m_workspace = extra.at("WORKSPACE_ENABLED").get<bool>();
		}
		else
		{
			// fix Bug 67374, reset m_workspace value
			m_workspace = true;
		}

        if (extra.at("CLIENT_HEARTBEAT_FREQUENCY").is_string())
        {
            m_heartbeatFrequency = std::atoi(extra.at("CLIENT_HEARTBEAT_FREQUENCY").get<std::string>().c_str());
        }
        else if (extra.at("CLIENT_HEARTBEAT_FREQUENCY").is_number())
        {
            m_heartbeatFrequency = extra.at("CLIENT_HEARTBEAT_FREQUENCY").get<int>();
        }

        if (extra.end() != extra.find("SYSTEM_BUCKET_NAME"))
        {
            std::string sptenantid = "";
            if (extra["SYSTEM_BUCKET_NAME"].is_string())
            {
                sptenantid = extra["SYSTEM_BUCKET_NAME"].get<std::string>();
            }

            int projectid = 0;
            std::string sptenantname = "";
            if (sptenantid.size() > 0)
            {
                if (0 == m_defaultmembership.GetTokenGroupName().compare(sptenantid.c_str()))
                    projectid = m_defaultmembership.GetProjectID();
                else
                {
                    for (size_t i = 0; i < m_memberships.size(); i++) {
                        if (0 != m_memberships[i].GetTokenGroupName().compare(sptenantid.c_str()))
                            continue;

                        projectid = m_memberships[i].GetProjectID();
                        sptenantname = m_memberships[i].GetID();
                        size_t pos = sptenantname.rfind('@');
                        if (pos != std::string::npos)
                            sptenantname = sptenantname.substr(pos + 1);

                        break;
                    }
                }
            }

            m_systemProjectId = projectid;
            m_systemProjectTenantId = sptenantid;
            m_systemProjectTenant = sptenantname;
        }

		GetTenantAdmins();
		return RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "Update recipient query JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmUser::GetNXLFileMetadata(const std::wstring &nxlfilepath, const std::string& pathid, SDR_FILE_META_DATA& metadata)
{
	CELOG_ENTER;
	Client restclient(NXRMC_CLIENT_NAME, true);

	CSDRmNXLFile *nxlfile = NULL;

	// we only need duid without token. This code to be change later
	SDWLResult res = OpenFileForMetaData(nxlfilepath, (ISDRmNXLFile**)&nxlfile);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}

	std::string duid = nxlfile->GetDuid();
	restclient.Open();
		// this is myVault file
		std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

		RMCCORE::HTTPRequest httpreq = GetNXLFileMetadataQuery(duid, pathid);
		StringBodyRequest request(httpreq);
		StringResponse response;

		res = spConn->SendRequest(request, response);

		if (!res)
		{
			CloseFile(nxlfile);
			CELOG_RETURN_VAL_T(res);
		}

		const std::string& jsonreturn = response.GetBody();
		CELOG_LOGA(CELOG_DEBUG, "GetNXLFileMetadata: response= %s\n", jsonreturn.c_str());

		try
		{
			RMCCORE::RetValue ret = nxlfile->ImportMetadataQueryFromRMSResponse(jsonreturn);
			if (!ret)
			{
				CloseFile(nxlfile);
				CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
			}

			metadata.name = nxlfile->Meta_GetName();
			metadata.fileLink = nxlfile->Meta_GetFileLink();
			metadata.lastmodify = nxlfile->Meta_GetLastModify();
			// metadata.rights = nxlfile->GetRights();
			metadata.shared = nxlfile->Meta_IsShared();
			metadata.deleted = nxlfile->Meta_IsDeleted();
			metadata.revoked = nxlfile->Meta_IsRevoked();
			metadata.recipients = nxlfile->Meta_GetFileRecipients().GetRecipients();
			{
				SDR_Expiration expiration;
				RMCCORE::CONDITION::Expiry expiry = nxlfile->Meta_GetExpiration();
				expiration.type = (IExpiryType)expiry.getType();
				expiration.start = expiry.getStartDate();
				expiration.end = expiry.getEndDate();
				metadata.expiration = expiration;
			}
			metadata.protectionType = nxlfile->Meta_GetProtectionType();
			metadata.pathDisplay = nxlfile->Meta_GetPathDisplay();
			metadata.pathid = nxlfile->Meta_GetPathId();
			metadata.owner = nxlfile->Meta_IsOwner();
			metadata.nxl = nxlfile->Meta_IsNxlFile();
			metadata.protectedon = nxlfile->Meta_GetProtectedOn();
			metadata.sharedon = nxlfile->Meta_GetSharedOn();
		}
		catch (...) {
			// The JSON data is NOT correct
			res = RESULT2(ERROR_INVALID_DATA, "List files query JSON response is not correct");
		}

	CloseFile(nxlfile);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetProjectFileMetadata(const std::wstring &nxlfilepath, const std::string& projectid, const std::string& pathid, SDR_FILE_META_DATA& metadata)
{
	CELOG_ENTER;
	Client restclient(NXRMC_CLIENT_NAME, true);

	CSDRmNXLFile *nxlfile = NULL;
	SDWLResult res = OpenFileForMetaData(nxlfilepath, (ISDRmNXLFile**)&nxlfile);
	if (!res) {
		CELOG_RETURN_VAL_T(res);
	}

	std::string duid = nxlfile->GetDuid();
	restclient.Open();

		// this is project file
		std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

		RMCCORE::HTTPRequest httpreq = GetProjectFileMetadataQuery(projectid, pathid);
		StringBodyRequest request(httpreq);
		StringResponse response;

		res = spConn->SendRequest(request, response);

		if (!res)
		{
			CloseFile(nxlfile);
			CELOG_RETURN_VAL_T(res);
		}

		const std::string& jsonreturn = response.GetBody();
		CELOG_LOGA(CELOG_DEBUG, "GetNXLFileMetadata: response= %s\n", jsonreturn.c_str());

		try
		{
			RMCCORE::RetValue ret = nxlfile->ImportProjectFileMetadataQueryFromRMSResponse(jsonreturn);
			if (!ret)
			{
				CloseFile(nxlfile);
				CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));
			}

			metadata.name = nxlfile->Meta_GetName();
			metadata.fileLink = nxlfile->Meta_GetFileLink();
			metadata.lastmodify = nxlfile->Meta_GetLastModify();
			// metadata.rights = nxlfile->GetRights();
			metadata.shared = nxlfile->Meta_IsShared();
			metadata.deleted = nxlfile->Meta_IsDeleted();
			metadata.revoked = nxlfile->Meta_IsRevoked();
			metadata.recipients = nxlfile->Meta_GetFileRecipients().GetRecipients();
			{
				SDR_Expiration expiration;
				RMCCORE::CONDITION::Expiry expiry = nxlfile->Meta_GetExpiration();
				expiration.type = (IExpiryType)expiry.getType();
				expiration.start = expiry.getStartDate();
				expiration.end = expiry.getEndDate();
				metadata.expiration = expiration;
			}
			metadata.protectionType = nxlfile->Meta_GetProtectionType();
			metadata.pathDisplay = nxlfile->Meta_GetPathDisplay();
			metadata.pathid = nxlfile->Meta_GetPathId();
			metadata.owner = nxlfile->Meta_IsOwner();
			metadata.nxl = nxlfile->Meta_IsNxlFile();
		}
		catch (...) {
			// The JSON data is NOT correct
			res = RESULT2(ERROR_INVALID_DATA, "List files query JSON response is not correct");
		}


	CloseFile(nxlfile);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetProjectFileMetadata(const std::string& projectid, const std::string& pathid, SDR_FILE_META_DATA& metadata)
{
    SDWLResult res;
    CELOG_ENTER;
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();

    // this is project file
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

    RMCCORE::HTTPRequest httpreq = GetProjectFileMetadataQuery(projectid, pathid);
    StringBodyRequest request(httpreq);
    StringResponse response;

    res = spConn->SendRequest(request, response);

    if (!res)
        CELOG_RETURN_VAL_T(res);

    std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetNXLFileMetadata: response= %s\n", jsonreturn.c_str());

    try
    {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!"));
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
        }

        const nlohmann::json& results = root.at("results");
        const nlohmann::json& fileInfo = results.at("fileInfo");

        metadata.pathDisplay = fileInfo.at("pathDisplay").get<std::string>();
        metadata.pathid = fileInfo.at("pathId").get<std::string>();
        metadata.name = fileInfo.at("name").get<std::string>();

        metadata.lastmodify = fileInfo.at("lastModified").get<uint64_t>();
        metadata.owner = fileInfo.at("owner").get<bool>();
        metadata.nxl = fileInfo.at("nxl").get<bool>();
        metadata.protectionType = fileInfo.at("protectionType").get<uint32_t>();

        if (fileInfo.end() != fileInfo.find("tags"))
        {
            metadata.tags = fileInfo.at("tags").dump();
        }

        SDR_Expiration expiration;
        if (fileInfo.end() != fileInfo.find("expiry"))
        {
            const nlohmann::json& expiry = fileInfo["expiry"];
            if (expiry.end() != expiry.find("endDate"))
            {
                expiration.end = expiry["endDate"].get<uint64_t>();
            }

            if (expiry.end() != expiry.find("startDate"))
            {
                expiration.start = expiry["startDate"].get<uint64_t>();
                expiration.type = RANGEEXPIRE;
            }
            else
            {
                expiration.type = ABSOLUTEEXPIRE;
            }
        }

        metadata.expiration = expiration;
        CELOG_RETURN_VAL_T(RESULT2(0, NX::conv::to_string(message)));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetProjectFileRights(const std::string& projectid, const std::string& pathid, std::vector<SDRmFileRight>& rights)
{
    CELOG_LOGA(CELOG_DEBUG, "projectid: %s, pathid: %s\n", projectid.c_str(), pathid.c_str());
    SDWLResult res;
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));
    RMCCORE::HTTPRequest httpreq = GetProjectFileMetadataQuery(projectid, pathid);

    StringBodyRequest request(httpreq);
    StringResponse response;

    res = spConn->SendRequest(request, response);

    if (!res)
        return res;

    std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "GetNXLFileMetadata: response= %s\n", jsonreturn.c_str());

    try {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            return RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!");
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            return RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message));
        }

        const nlohmann::json& results = root.at("results");
        const nlohmann::json& fileInfo = results.at("fileInfo");

        if (fileInfo.end() != fileInfo.find("rights"))
        {
            std::vector<std::string> vec;
            vec = fileInfo["rights"].get<std::vector<std::string>>();
            To_FileRights(vec, rights);
        }

        return RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmUser::GetMyVaultFileRights(const std::string& duid, const std::string& pathid, std::vector<SDRmFileRight>& rights)
{
    CELOG_LOGA(CELOG_DEBUG, " duid: %s, pathid: %s\n", duid.c_str(), pathid.c_str());
    SDWLResult res;
    Client restclient(NXRMC_CLIENT_NAME, true);

    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));
    RMCCORE::HTTPRequest httpreq = GetNXLFileMetadataQuery(duid, pathid);

    StringBodyRequest request(httpreq);
    StringResponse response;

    res = spConn->SendRequest(request, response);

    if (!res)
        return res;

    const std::string& jsonreturn = response.GetBody();
    CELOG_LOGA(CELOG_DEBUG, "Get File Metadata: response= %s\n", jsonreturn.c_str());

    try {
        auto root = nlohmann::json::parse(jsonreturn);
        if (!root.is_object())
        {
            RESULT2(RMCCORE_INVALID_JSON_FORMAT, "RMS Json Error!");
        }

        int status = root.at("statusCode");
        std::string message = root.at("message");
        if (status != http::status_codes::OK.id) {
            RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message));
        }

        const nlohmann::json& results = root.at("results");
        const nlohmann::json& detail = results.at("detail");

        if (detail.end() != detail.find("rights"))
        {
            std::vector<std::string> vec;
            vec = detail["rights"].get<std::vector<std::string>>();
            To_FileRights(vec, rights);
        }

        return RESULT2(0, NX::conv::to_string(message));
    }
    catch (std::exception &e) {
        std::string strError = "JSON response is not correct, error : " + std::string(e.what());
        res = RESULT2(ERROR_INVALID_DATA, strError);
    }
    catch (...) {
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

SDWLResult CSDRmUser::RPMEditCopyFile(const std::wstring &filepath, std::wstring& destpath, const std::wstring hiddenrpmfolder)
{
	CELOG_ENTER;
	SDWLResult res;
	CELOG_LOG(CELOG_INFO, L" filepath = %s, destpath= %s, hiddenrpmfolder= %s, srclen= %d\n", filepath.c_str(), destpath.c_str(), hiddenrpmfolder.c_str(), filepath.length());

	NX::fs::dos_fullfilepath input_filepath(filepath);
	NX::fs::dos_fullfilepath output_filepath(destpath);
	NX::fs::dos_fullfilepath output_rpmpath(hiddenrpmfolder);

	// check source file exists or not
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(input_filepath.global_dos_path().c_str()) && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
		CELOG_RETURN_VAL_T(RESULT2(ERROR_PATH_NOT_FOUND, "File not found"));

	// extract file name from source full path
	std::wstring filename = L"";
	std::experimental::filesystem::path _filename(input_filepath.path());
	if (_filename.has_extension() == false || NX::icompare((std::wstring)(_filename.extension().c_str()), (std::wstring)(L".nxl")) != 0) //fix bug 56798
		filename = std::wstring(_filename.filename()) + L".nxl";
	else
		filename = _filename.filename();

	// make sure source file is NXL file and can be open
	CSDRmNXLFile *file = NULL;

#ifndef _Get_Token_
	// don't need get token
	res = OpenFile(input_filepath.path(), (ISDRmNXLFile**)&file);
	if (!res)
		CELOG_RETURN_VAL_T(res);
#else
	file = new CSDRmNXLFile(input_filepath.global_dos_path());
	if (NULL == file)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_NOT_ENOUGH_MEMORY, "fail to allocate memory"));
	if (!file->IsNXL())
	{
		delete file;
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Invalid nxl file"));
	}
#endif
	std::wstring duid = NX::conv::to_wstring(file->GetDuid());
	CloseFile(file);

	// get RPM folder
	std::wstring rpmfolder = L"";
	if (destpath.size() > 0)
		rpmfolder = output_filepath.path();
	else
	{
		// use RPM hidden folder for dest RPM folder
		rpmfolder = output_rpmpath.path();
	}

	// no RPM folder set, return
	if (rpmfolder.size() == 0)
	{
		CELOG_LOG(CELOG_WARNING, L"no RPM folder set");
		CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
	}

	CELOG_LOG(CELOG_DEBUG, L"source file = %s\n", input_filepath.path().c_str());
	// if file is already in a RPM folder, we will not copy
	if (input_filepath.path().find(NX::conv::to_wstring(rpmfolder), 0) != std::wstring::npos)
	{
		// file is in RPM folder (passed in RPM folder or hidden RPM folder)
		CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
	}

	// generate file under RPM folder
	// file path is like "c://RPM folder//DUID-DUID-DUID-DUID//filename.docx
	SYSTEMTIME st = { 0 };
	GetSystemTime(&st);
	// -YYYY-MM-DD-HH-MM-SS
	const std::wstring sTimestamp(NX::FormatString(L"%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond));
	// std::wstring dfullpath = NX::conv::to_unlimitedpath(std::experimental::filesystem::path(rpmfolder) / std::experimental::filesystem::path(duid + sTimestamp));
	NX::fs::dos_fullfilepath dfullpath(std::experimental::filesystem::path(rpmfolder) / std::experimental::filesystem::path(duid + sTimestamp));

	// Create duid directory under RPM folder
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(dfullpath.global_dos_path().c_str()) && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
	{
		if (::CreateDirectoryW(dfullpath.global_dos_path().c_str(), NULL) == false)
		{
			// failed, then just use RPM folder
			dfullpath = NX::fs::dos_fullfilepath(rpmfolder);
		}
	}

	dfullpath = NX::fs::dos_fullfilepath(dfullpath.path() / std::experimental::filesystem::path(filename));
	CELOG_LOGA(CELOG_DEBUG, "RPMEditCopyFile:  dfullpath = %s\n", NX::conv::to_string(dfullpath.path()).c_str());

	std::wstring nonnxlfile = NX::conv::remove_extension(dfullpath.path());

	CELOG_LOGA(CELOG_DEBUG, "RPMEditCopyFile: nonnxlfile = %s\n", NX::conv::to_string(nonnxlfile).c_str());
	// if both NXL file and decrypted file exists, we will not copy, but directly return the file
	BOOL bExistsNXL = true;
	BOOL bExistsNormal = true;
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(dfullpath.global_dos_path().c_str()))
		bExistsNXL = false;
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(NX::fs::dos_fullfilepath(nonnxlfile).global_dos_path().c_str()))
		bExistsNormal = false;

	if ((bExistsNormal && false == bExistsNXL) || (false == bExistsNormal))
	{
		// in RPM folder
		if (m_SDRmcInstance)
		{
			res = m_SDRmcInstance->RPMDeleteFile(nonnxlfile);
			if (!res)
			{
				CELOG_LOGA(CELOG_DEBUG, "RPMDeleteFile failed = %d, %s\n", res.GetCode(), NX::conv::to_string(nonnxlfile).c_str());
			}
		}

		DWORD FileAttributes  = GetFileAttributes(input_filepath.global_dos_path().c_str());
		if (FILE_ATTRIBUTE_ENCRYPTED & FileAttributes) {
			std::ifstream ifs(input_filepath.global_dos_path().c_str(), std::ios::binary);
			std::ofstream ofs(dfullpath.global_dos_path().c_str(), std::ios::binary);
			ofs << ifs.rdbuf();
			bool state = ifs.good();

			ifs.close();
			ofs.close();

			if (state) {
				// Call FindFirstFile to notify driver there is a new NXL file
				std::wstring filePath = dfullpath.global_dos_path();
				WIN32_FIND_DATA pNextInfo;
				HANDLE h = FindFirstFile(filePath.c_str(), &pNextInfo);
				if (h != INVALID_HANDLE_VALUE)
				{
					FindClose(h);
				}
			}
			else {
				DWORD error = GetLastError();
				CELOG_LOGA(CELOG_DEBUG, "CopyFile failed = %lu, %s\n", error, NX::conv::to_string(dfullpath.path()).c_str());
				res = RESULT2(error, "CopyFile failed");
				CELOG_RETURN_VAL_T(res);
			}
		}
		else {
			// copy the source file to RPM folder now
			if (CopyFile(input_filepath.global_dos_path().c_str(), dfullpath.global_dos_path().c_str(), false))
			{
				// Call FindFirstFile to notify driver there is a new NXL file
				//LPCWSTR lpNXLFileName = dfullpath.global_dos_path().c_str();
				std::wstring filePath = dfullpath.global_dos_path();
				WIN32_FIND_DATA pNextInfo;
				HANDLE h = FindFirstFile(filePath.c_str(), &pNextInfo);
				if (h != INVALID_HANDLE_VALUE)
				{
					FindClose(h);
				}
			}
			else
			{
				DWORD error = GetLastError();
				CELOG_LOGA(CELOG_DEBUG, "CopyFile failed = %lu, %s\n", error, NX::conv::to_string(dfullpath.path()).c_str());
				res = RESULT2(error, "CopyFile failed");
				CELOG_RETURN_VAL_T(res);
			}
		}
	}

	destpath = nonnxlfile;

	// need a lock for inter-process communication
	// add to editing-file-mapping
	RPMEdit_Map(destpath, input_filepath.path());

	CELOG_RETURN_VAL_T(RESULT(SDWL_SUCCESS));
}

SDWLResult CSDRmUser::RPMEditSaveFile(const std::wstring& inputpath, const std::wstring& originalNXLfilePath, bool deletesource, uint32_t exitedit, const std::wstring& tags)
{
	CELOG_ENTER;
	//*exitedit				
	//	* 0, not exit and not save
	//	* 1, not exit, but save
	//	* 2, exit and not save
	//	* 3 (and others), exit and save

	if (exitedit == 0)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_SUCCESS, ""));

	CELOG_LOG(CELOG_INFO, L"filepath = %s, originalNXLfilePath=%s,  exitedit= %d\n", inputpath.c_str(), originalNXLfilePath.c_str(), exitedit);

	NX::fs::dos_fullfilepath filepath(inputpath);

	if (exitedit == 2)
	{
		// exit without save
		NX::fs::dos_fullfilepath exactOriginalNXL(originalNXLfilePath);
		BOOL bTmpOriginalNXL = false;
		if (originalNXLfilePath.size() == 0)
		{
			std::wstring _tmp;
			RPMEdit_FindMap(filepath.path(), _tmp);
			exactOriginalNXL = NX::fs::dos_fullfilepath(_tmp);
		}

		std::string duid = "";
		if (exactOriginalNXL.path().size() > 0)
		{
			CSDRmNXLFile *file = NULL;
			SDWLResult res = OpenFile(exactOriginalNXL.path(), (ISDRmNXLFile**)&file);
			if (!res)
				CELOG_RETURN_VAL_T(res);
			duid = file->GetDuid();
			CloseFile(file);
		}
		else
		{
			exactOriginalNXL = NX::fs::dos_fullfilepath(filepath.path() + L".nxl");
			RMLocalFile path(exactOriginalNXL.global_dos_path());
			if (!path.GetLastError()) {
				exactOriginalNXL.clear();
			}
			path.Close();
		}
		// need a lock
		// exit the edit mode, clear the editing-file from mapping
		RPMEdit_UnMap(filepath.path());

		if (deletesource)
		{
			// delete the source file when exit
			// we must add the appendix ".nxl"
			std::wstring nxlfilepath = filepath.path() + L".nxl";
			if (nxlfilepath != exactOriginalNXL.path()) // we shall not delete the file if plain file is under rpm folder
			{
				// try to call RPM service to delete the NXL file
				if (m_SDRmcInstance)
				{
					SDWLResult ret = m_SDRmcInstance->RPMDeleteFile(filepath.path());
				}

				if (duid.size() > 0)
				{
					// delete the parent folder if it is DUID folder
					if (nxlfilepath.find(NX::conv::to_wstring(duid), 0) != std::wstring::npos)
					{
						std::experimental::filesystem::path duidfolder(nxlfilepath);
						duidfolder.remove_filename();

						try {
							std::experimental::filesystem::remove_all(NX::fs::dos_fullfilepath(duidfolder.c_str()).global_dos_path());
						}
						catch (...) {}

						DWORD attr = GetFileAttributesW(NX::fs::dos_fullfilepath(duidfolder.c_str()).global_dos_path().c_str());
						DWORD dwErr = ::GetLastError();
						if (INVALID_FILE_ATTRIBUTES != attr)
						{
							// Ask RPM service to delete the folder
							if (m_SDRmcInstance)
								m_SDRmcInstance->RPMDeleteFolder(duidfolder);
						}

					}
				}
			}
		}

		// exit without save
		CELOG_RETURN_VAL_T(RESULT2(SDWL_SUCCESS, ""));
	}

	BOOL bExits = false;
	if (exitedit == 1) bExits = false;
	else bExits = true;

	// originalNXLfilePath
	//	1. if empty, we will search from edit-file-mapping-cache
	//	2. if we can't find from mapping, check current folder to see whether the NXL file exists or not.
	//		if yes, set originalNXLfilePath as current working folder
	NX::fs::dos_fullfilepath exactOriginalNXL(originalNXLfilePath);
	BOOL bTmpOriginalNXL = false;
	if (originalNXLfilePath.size() == 0)
	{
		std::wstring _tmp = L"";
		RPMEdit_FindMap(filepath.path(), _tmp);
		// not find the mapping
		exactOriginalNXL = NX::fs::dos_fullfilepath(_tmp);
		if (_tmp.size() == 0)
		{
			// Check current folder to see whether the "filepath".NXL exists or not
			_tmp = filepath.path() + L".nxl";
			exactOriginalNXL = NX::fs::dos_fullfilepath(_tmp);
			RMLocalFile path(exactOriginalNXL.global_dos_path());
			if (!path.GetLastError()) {
				CELOG_RETURN_VAL_T(RESULT2(SDWL_PATH_NOT_FOUND, path.GetLastError().GetMessage()));
			}
			path.Close();

			CELOG_LOG(CELOG_DEBUG, L"original NXL file = %s\n", exactOriginalNXL.path().c_str());
			// check to see if current folder is RPM folder
			unsigned int dirstatus = 3;
			bool filestatus = 0;
			SDWLResult res = m_SDRmcInstance->RPMGetFileStatus(exactOriginalNXL.path(), &dirstatus, &filestatus);
			if (dirstatus & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR))
			{
				// 
				// WARNING!!!
				//
				// We shall ask RPM Service to get NXL file header info
				//
				// now we just did a workaround to copy the file out to a temp folder
				//
				//

				// NXL file is under RPM folder, try to copy it to a temp file
				std::wstring tmpFilePath = L"c:\\";
				res = ProjectCreateTempFile((conv::to_wstring(NX::itos<char>(0))), conv::to_wstring(exactOriginalNXL.path()), tmpFilePath);
				tmpFilePath = tmpFilePath + L".nxl";

				res = m_SDRmcInstance->RPMCopyFile(exactOriginalNXL.path(), tmpFilePath);
				if (res)
				{
					// we now get the tmp nxl file
					bTmpOriginalNXL = true;
					exactOriginalNXL = tmpFilePath;
				}
				else
				{
					CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "system copy file failed."));
				}
			}
		}
	}

	// Open and close file to make sure the file is not locked.
	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(exactOriginalNXL.path(), (ISDRmNXLFile**)&file);
	if (!res)
		CELOG_RETURN_VAL_T(res);
	std::string duid = file->GetDuid();
	CloseFile(file);

	// Reprotect file
	CELOG_LOG(CELOG_DEBUG, L"ReProtectFile - source = %s\n", filepath.path().c_str());
	CELOG_LOG(CELOG_DEBUG, L"ReProtectFile - dest = %s\n", exactOriginalNXL.path().c_str());

	DWORD dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	m_SDRmcInstance->RPMGetFileAttributes(exactOriginalNXL.path(), dwFileAttributes);
	// exitedit = 1 or 3 (or others), save
	res = ReProtectFile(filepath.path(), exactOriginalNXL.path(),tags);
	if (!res)
		CELOG_RETURN_VAL_T(res);

	//fix Bug 69408 - Can't open nxl office file if file with "Encrypt contents to secure data" attribute 
	if (FILE_ATTRIBUTE_ENCRYPTED & dwFileAttributes) {
		m_SDRmcInstance->RPMWindowsEncryptFile(exactOriginalNXL.path());
	}

	if (bExits)
	{
		// need a lock for inter-process communication
		// exit the edit mode, clear the editing-file from mapping
		RPMEdit_UnMap(filepath.path());
	}

	if (bTmpOriginalNXL)
	{
		// must save the tmp file back to the original NXL

		// we assume the NXL file is under RPM folder
		std::wstring nxlfilepath = filepath.path() + L".nxl";
		if (m_SDRmcInstance)
		{
			DWORD dwFileAttributes = INVALID_FILE_ATTRIBUTES;
			m_SDRmcInstance->RPMGetFileAttributes(nxlfilepath, dwFileAttributes);
			SDWLResult ret = m_SDRmcInstance->RPMCopyFile(exactOriginalNXL.path(), nxlfilepath, true);
			if (ret)
			{
				CELOG_LOGA(CELOG_DEBUG, "RPMEditSaveFile: save back - source = %s\n", NX::conv::to_string(exactOriginalNXL.path()));
				CELOG_LOGA(CELOG_DEBUG, "RPMEditSaveFile: save back - dest = %s\n", NX::conv::to_string(nxlfilepath));
				// DeleteFile(exactOriginalNXL.c_str());
				exactOriginalNXL = nxlfilepath;
				//fix Bug 69408 - Can't open nxl office file if file with "Encrypt contents to secure data" attribute 
				if (FILE_ATTRIBUTE_ENCRYPTED & dwFileAttributes) {
					m_SDRmcInstance->RPMWindowsEncryptFile(exactOriginalNXL.path());
				}
			}
			else
			{
				DeleteFile(exactOriginalNXL.global_dos_path().c_str());
				CELOG_RETURN_VAL_T(ret);
			}
		}
		else
			CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "system is not correctly initialized."));
	}

	if (bExits && deletesource)
	{
		// delete the source file when exit
		// we must add the appendix ".nxl"
		std::wstring nxlfilepath = filepath.path() + L".nxl";
		if (nxlfilepath != exactOriginalNXL.path()) // we shall not delete the file if plain file is under rpm folder
		{
			// try to call RPM service to delete the NXL file
			if (m_SDRmcInstance)
			{
				CELOG_LOG(CELOG_DEBUG, L"delete - source = %s\n", filepath.path().c_str());
				SDWLResult ret = m_SDRmcInstance->RPMDeleteFile(filepath.path());
			}

			// delete the parent folder if it is DUID folder
			if (nxlfilepath.find(NX::conv::to_wstring(duid), 0) != std::wstring::npos)
			{
				std::experimental::filesystem::path duidfolder(nxlfilepath);
				duidfolder.remove_filename();

				try {
					std::experimental::filesystem::remove_all(NX::fs::dos_fullfilepath(duidfolder.c_str()).global_dos_path());
				}
				catch (...) {}

				DWORD attr = GetFileAttributesW(NX::fs::dos_fullfilepath(duidfolder.c_str()).global_dos_path().c_str());
				DWORD dwErr = ::GetLastError();
				if (INVALID_FILE_ATTRIBUTES != attr)
				{
					// Ask RPM service to delete the folder
					if (m_SDRmcInstance)
						m_SDRmcInstance->RPMDeleteFolder(duidfolder);
				}
			}
		}
	}

	CELOG_RETURN_VAL_T(RESULT2(SDWL_SUCCESS, ""));
}

SDWLResult CSDRmUser::RPMGetRights(const std::wstring& filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks)
{
	// find the original NXL file from editing-map
	CELOG_LOG(CELOG_INFO, L" filepath = %s \n", filepath.c_str());
	NX::fs::dos_fullfilepath input_filepath(filepath);
	std::wstring originalNXLFile = L"";

	RPMEdit_FindMap(input_filepath.path(), originalNXLFile);
	if (originalNXLFile.size() > 0)
	{
		//fix bug 55764 - ppt file cannot mark offline and prompt has no permission
		//CSDRmNXLFile *file = NULL;
		//SDWLResult res = OpenFile(originalNXLFile, (ISDRmNXLFile**)&file);
		//if (!res)
		//	return res;

		return GetRights(originalNXLFile, rightsAndWatermarks);
	}
	else
	{
		// file is not in the edit-mapping
		// try current folder.
		NX::fs::filename fn(input_filepath.global_dos_path());
		auto ext = fn.extension();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		originalNXLFile = (ext != L".nxl") ? input_filepath.global_dos_path() + L".nxl" : input_filepath.global_dos_path();

		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(originalNXLFile.c_str()) && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
			return RESULT2(ERROR_PATH_NOT_FOUND, "File not found");

		SDWLResult res = m_SDRmcInstance->RPMGetFileRights(originalNXLFile, rightsAndWatermarks, 0); // option: 0, user rights on file; 1, file rights set on NXL header

		return res;
	}
}

SDWLResult CSDRmUser::RPMEdit_Map(const std::wstring &filepath, const std::wstring &nxlfilepath)
{
	CELOG_ENTER;
	NX::fs::dos_fullfilepath input_filepath(filepath);
	NX::fs::dos_fullfilepath nxl_filepath(nxlfilepath);
	HRESULT nRet = create_key_with_default_value(HKEY_CURRENT_USER,
		L"Software",
		L"NextLabs",
		NULL);

	nRet = create_key_with_default_value(HKEY_CURRENT_USER,
		L"Software\\NextLabs",
		L"SkyDRM",
		NULL);

	nRet = create_key_with_default_value(HKEY_CURRENT_USER,
		L"Software\\NextLabs\\SkyDRM",
		L"Session",
		NULL);

	nRet = set_value_content(HKEY_CURRENT_USER,
		L"Software\\NextLabs\\SkyDRM\\Session",
		input_filepath.path().c_str(),
		nxl_filepath.path().c_str());

	if (S_OK != nRet)
	{
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "set_value_content error"));
	}

	CELOG_RETURN_VAL_T(RESULT2(SDWL_SUCCESS, ""));
}

SDWLResult CSDRmUser::RPMEdit_FindMap(const std::wstring &filepath, std::wstring &nxlfilepath)
{
	CELOG_ENTER;

	NX::fs::dos_fullfilepath input_filepath(filepath);

	HKEY root = HKEY_CURRENT_USER;
	const wchar_t* parent = L"Software\\NextLabs\\SkyDRM\\Session";

	HKEY hParent;
	if (ERROR_SUCCESS != RegOpenKeyExW(root, parent, 0, KEY_READ, &hParent))
	{
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, ""));
	}

	DWORD value_type;
	BYTE* value_buffer;
	DWORD value_length;

	// get length first
	if (ERROR_SUCCESS != RegQueryValueExW(hParent, input_filepath.path().c_str(), NULL, &value_type, NULL, &value_length)) {
		RegCloseKey(hParent);
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegQueryValueExW failed to get value length"));
	}

	value_buffer = new BYTE[value_length + 2];
	// get value;
	if (ERROR_SUCCESS != RegQueryValueExW(hParent, input_filepath.path().c_str(), NULL, &value_type, value_buffer, &value_length)) {
		RegCloseKey(hParent);
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegQueryValueExW failed to get value"));
	}
	// close 
	if (ERROR_SUCCESS != RegCloseKey(hParent)) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "RegCloseKey failed"));
	}

	// set value to out param
	nxlfilepath.assign((wchar_t*)value_buffer);
	delete[] value_buffer;

	CELOG_RETURN_VAL_T(RESULT2(SDWL_SUCCESS, ""));
}


SDWLResult CSDRmUser::RPMEdit_UnMap(const std::wstring &filepath)
{
	CELOG_ENTER;

	NX::fs::dos_fullfilepath input_filepath(filepath);

	HKEY root = HKEY_CURRENT_USER;
	const wchar_t* parent = L"Software\\NextLabs\\SkyDRM\\Session";

	HRESULT nRet = delete_value(root, parent, input_filepath.path().c_str());

	CELOG_RETURN_VAL_T(RESULT2(nRet, ""));
}

SDWLResult CSDRmUser::PDSetupHeader(const unsigned char* header, long header_len, int64_t* contentLength, unsigned int* contentOffset)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_DEBUG, L"header_len=%ld\n", header_len);
	SDWLResult res;

	res = m_partialDownload.BuildNxlHeader(header, header_len);
	if (!res)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header data is incorrect"));

	RMCCORE::NXLFMT::Header nxlheader;
	m_partialDownload.GetHeader(nxlheader);
    static bool once = true;
    if (once)
    {
        once = false;
        std::string duid = nxlheader.getDuid();
        RMToken filetoken;
        if (RemoveCachedToken(duid))
        {
            CELOG_LOG(CELOG_DEBUG, L"Fresh token in cache\n");
            std::string policy, tags;
            res = m_partialDownload.GetPolicy(policy);
            if (!res)
                CELOG_RETURN_VAL_T(res);

            // Tags are needed only for Central Policy files.
            if (policy.empty() || policy == "{}")
            {
                res = m_partialDownload.GetTags(tags);
                if (!res)
                    CELOG_RETURN_VAL_T(res);
            }

            res = GetFileToken(nxlheader.getOwnerId(), bintohs<char>(nxlheader.getAgreement0()), duid, 0, policy, tags, filetoken);
            if (!res)
                CELOG_RETURN_VAL_T(res);
            AddCachedToken(filetoken);
        }
    }

	*contentLength = nxlheader.dynamic.contentLength;
	*contentOffset = nxlheader.fixed.fileInfo.contentOffset;
	CELOG_LOG(CELOG_DEBUG, L"*contentLength=%I64d, *contentOffset=%u\n", *contentLength, *contentOffset);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::PDSetupHeaderEx(const unsigned char* header, long header_len, int64_t* contentLength, unsigned int* contentOffset, void *&context)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_DEBUG, L"header_len=%ld\n", header_len);
	SDWLResult res;

	context = NULL;
	SDRPartialDownload *pPartialDownload = new SDRPartialDownload;
	if (NULL == pPartialDownload)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_NOT_ENOUGH_MEMORY, "fail to allocate memory"));
	res = pPartialDownload->BuildNxlHeader(header, header_len);
	if (!res)
	{
		delete pPartialDownload;
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header data is incorrect"));
	}

	RMCCORE::NXLFMT::Header nxlheader;
	pPartialDownload->GetHeader(nxlheader);
    static bool once = true;
    if (once)
    {
        once = false;
        std::string duid = nxlheader.getDuid();
        RMToken filetoken;
        if (RemoveCachedToken(duid))
        {
            CELOG_LOG(CELOG_DEBUG, L"Fresh token in cache\n");
            std::string policy, tags;
            res = pPartialDownload->GetPolicy(policy);
            if (!res)
            {
                delete pPartialDownload;
                CELOG_RETURN_VAL_T(res);
            }

            // Tags are needed only for Central Policy files.
            if (policy.empty() || policy == "{}")
            {
                res = pPartialDownload->GetTags(tags);
                if (!res)
                {
                    delete pPartialDownload;
                    CELOG_RETURN_VAL_T(res);
                }
            }

            res = GetFileToken(nxlheader.getOwnerId(), bintohs<char>(nxlheader.getAgreement0()), duid, 0, policy, tags, filetoken);
            if (!res)
            {
                delete pPartialDownload;
                CELOG_RETURN_VAL_T(res);
            }
            AddCachedToken(filetoken);
        }
    }

	*contentLength = nxlheader.dynamic.contentLength;
	*contentOffset = nxlheader.fixed.fileInfo.contentOffset;
	context = pPartialDownload;
	CELOG_LOG(CELOG_DEBUG, L"*contentLength=%I64d, *contentOffset=%u, context=%p\n", *contentLength, *contentOffset, context);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::PDDecryptPartial(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len, const unsigned char* header, long header_len)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_DEBUG, L"in_len=%ld, offset=%ld, header_len=%ld\n", in_len, offset, header_len);
	SDWLResult res;

	res = m_partialDownload.BuildNxlHeader(header, header_len);
	if (!res)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header data is incorrect"));

	RMCCORE::NXLFMT::Header nxlheader;
	RMToken filetoken;
	m_partialDownload.GetHeader(nxlheader);
	std::string duid = nxlheader.getDuid();
	if (!FindCachedToken(duid, filetoken))
	{
		CELOG_LOG(CELOG_DEBUG, L"File token not found in cache\n");
		std::string policy, tags;
		res = m_partialDownload.GetPolicy(policy);
		if (!res)
			CELOG_RETURN_VAL_T(res);

		// Tags are needed only for Central Policy files.
		if (policy.empty() || policy == "{}")
		{
			res = m_partialDownload.GetTags(tags);
			if (!res)
				CELOG_RETURN_VAL_T(res);
		}

		res = GetFileToken(nxlheader.getOwnerId(), bintohs<char>(nxlheader.getAgreement0()), duid, 0, policy, tags, filetoken);
		if (!res)
			CELOG_RETURN_VAL_T(res);
		AddCachedToken(filetoken);
	}

	m_partialDownload.SetToken(filetoken);
	res = m_partialDownload.DecryptData(in, in_len, offset, out, out_len);
	CELOG_LOG(CELOG_DEBUG, L"*out_len=%ld\n", *out_len);

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::PDDecryptPartialEx(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len, const unsigned char* header, long header_len, void *context)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_DEBUG, L"in_len=%ld, offset=%ld, header_len=%ld, context=%p\n", in_len, offset, header_len, context);
	SDWLResult res;

	SDRPartialDownload *pPartialDownload = static_cast<SDRPartialDownload *>(context);
	res = pPartialDownload->BuildNxlHeader(header, header_len);
	if (!res)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header data is incorrect"));

	RMCCORE::NXLFMT::Header nxlheader;
	RMToken filetoken;
	pPartialDownload->GetHeader(nxlheader);
	std::string duid = nxlheader.getDuid();
	if (!FindCachedToken(duid, filetoken))
	{
		CELOG_LOG(CELOG_DEBUG, L"File token not found in cache\n");
		std::string policy, tags;
		res = pPartialDownload->GetPolicy(policy);
		if (!res)
			CELOG_RETURN_VAL_T(res);

		// Tags are needed only for Central Policy files.
		if (policy.empty() || policy == "{}")
		{
			res = pPartialDownload->GetTags(tags);
			if (!res)
				CELOG_RETURN_VAL_T(res);
		}

		res = GetFileToken(nxlheader.getOwnerId(), bintohs<char>(nxlheader.getAgreement0()), duid, 0, policy, tags, filetoken);
		if (!res)
			CELOG_RETURN_VAL_T(res);
		AddCachedToken(filetoken);
	}

	pPartialDownload->SetToken(filetoken);
	res = pPartialDownload->DecryptData(in, in_len, offset, out, out_len);
	CELOG_LOG(CELOG_DEBUG, L"*out_len=%ld\n", *out_len);

	CELOG_RETURN_VAL_T(res);
}

void CSDRmUser::PDTearDownHeaderEx(void *context)
{
	CELOG_ENTER;
	CELOG_LOG(CELOG_DEBUG, L"context=%p\n", context);

	SDRPartialDownload *pPartialDownload = static_cast<SDRPartialDownload *>(context);
	delete pPartialDownload;

	CELOG_RETURN;
}

bool CSDRmUser::HasAdminRights(const std::wstring &nxlfilepath)
{
	bool isAdmin = false;
	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(nxlfilepath, (ISDRmNXLFile**)&file);
	if (!res)
		return false;

	std::vector<SDRmFileRight> adhocrights = file->GetRights();
	std::string tags = file->GetTags();
	std::string tenantName = file->GetTenant();

	if (adhocrights.size() > 0)
	{
		if (m_defaultmembership.GetID().find(tenantName) != std::string::npos)
		{
			// ad-hoc file in MyVault
			if (file->IsOwner(GetDefaultMembership().GetID()))
				isAdmin = true;
		}
		else if (tenantName == GetSystemProjectTenant())
		{
			// ad-hoc file in System Bucket
			if (IsTenantAdmin(NX::conv::to_string(GetEmail())))
			{
				isAdmin = true;
			}
		}
		else
		{
			for (size_t i = 0; i < m_memberships.size(); i++)
			{
				if (m_memberships[i].GetID().find(tenantName) != std::string::npos)
				{
					if (IsProjectAdmin(m_memberships[i].GetTokenGroupName(), NX::conv::to_string(GetEmail())))
						isAdmin = true;

					break;
				}
			}
		}
	}
	else
	{
		if (tenantName == GetSystemProjectTenant())
		{
			// central policy file in System Bucket
			if (IsTenantAdmin(NX::conv::to_string(GetEmail())))
			{
				isAdmin = true;
			}
		}
		else
		{
			for (size_t i = 0; i < m_memberships.size(); i++)
			{
				if (m_memberships[i].GetID().find(tenantName) != std::string::npos)
				{
					// am I project administrator for this project
					if (IsProjectAdmin(m_memberships[i].GetTokenGroupName(), NX::conv::to_string(GetEmail())))
						isAdmin = true;

					break;
				}
			}
		}
	}

	CloseFile(file);
	return isAdmin;
}

bool CSDRmUser::IsTenantAdmin(const std::string &email)
{
	for (size_t i = 0; i < m_TenantAdminList.size(); i++)
	{
		if (NX::conv::string_icompare(email,m_TenantAdminList[i]))
			return true;
	}

	return false;
}

bool CSDRmUser::IsProjectAdmin(const std::string &project_tenantid, const std::string &email)
{
	for (size_t i = 0; i < m_ProjectAdminList.size(); i++)
	{
		if (project_tenantid == m_ProjectAdminList[i].first)
		{
			for (size_t j = 0; j < m_ProjectAdminList[i].second.size(); j++)
			{
				if (NX::conv::string_icompare(email, m_ProjectAdminList[i].second[j]))
					return true;
			}
		}
	}

	return false;
}

SDWLResult CSDRmUser::LockFileSync(const std::wstring &nxlfilepath)
{
	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(nxlfilepath, (ISDRmNXLFile**)&file);
	if (!res)
		return res;

	std::string duid = file->GetDuid();

	CloseFile(file);

    nlohmann::json root = nlohmann::json::object();
    root["duid"] = duid;
    root["path"] = NX::conv::utf16toutf8(NX::fs::dos_fullfilepath(nxlfilepath).path());
    root["lock"] = true;

    std::string str = root.dump();
    return m_SDRmcInstance->RPMLockFileForSync(str);
}

SDWLResult CSDRmUser::ResumeFileSync(const std::wstring &nxlfilepath)
{
	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(nxlfilepath, (ISDRmNXLFile**)&file);
	if (!res)
		return res;

	std::string duid = file->GetDuid();

	CloseFile(file);

    nlohmann::json root = nlohmann::json::object();
    root["duid"] = duid;
    root["path"] = NX::conv::utf16toutf8(NX::fs::dos_fullfilepath(nxlfilepath).path());
    root["lock"] = false;

    std::string str = root.dump();
    return m_SDRmcInstance->RPMLockFileForSync(str);
}


SDWLResult CSDRmUser::UpdateNXLMetaData(const std::wstring &nxlfilepath, bool bRetry)
{
	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(nxlfilepath, (ISDRmNXLFile**)&file);
	if (!res)
		return res;

	res = UpdateNXLMetaData(file, bRetry);
	CloseFile(file);
	return res;
}


SDWLResult CSDRmUser::UpdateNXLMetaData(ISDRmNXLFile* file, bool bRetry)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

	std::string duid = nxlFile->GetDuid();
	std::string adhoc = nxlFile->GetPolicy();
	std::string tags = nxlFile->GetTags();
	std::string otp = nxlFile->GetToken().GetOtp();

	{
		HTTPRequest httpreq = UpdateNXLMetadataQuery(duid, otp, adhoc, tags, 0);
		StringBodyRequest request(httpreq);

		Client restclient(NXRMC_CLIENT_NAME, true);
		restclient.Open();

		std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

		StringResponse response;
		res = spConn->SendRequest(request, response);
		if (!res)
		{
			// Network Error, cache this request to RPM service
            if (bRetry)
            {
                nlohmann::json root = nlohmann::json::object();
                root["duid"] = duid;
                root["params"] = httpreq.GetBody();
                root["projectid"] = 0;
                root["type"] = RECLASSIFY_TYPE::CLASSIFY_NEW_FILE;
                root["option"] = RECLASSIFY_WHEN::RECLASSIFY_NOW;

                std::string str = root.dump();
                res = m_SDRmcInstance->RPMAddNXLMetadata(str);
            }

			CELOG_RETURN_VAL_T(res);
		}

		const std::string& jsonreturn = response.GetBody();
		CELOG_LOGA(CELOG_DEBUG, "UpdateNXLMetadataQuery: response=%s\n", jsonreturn.c_str());

		try
		{
            nlohmann::json root = nlohmann::json::parse(jsonreturn);
            if (!root.is_object())
            {
                CELOG_RETURN_VAL_T(RESULT2(RMCCORE_INVALID_JSON_FORMAT, "fail to load update NXL metadata response from RMS returned string!"));
            }

            int status = root.at("statusCode").get<int>();
            std::string message = root.at("message").get<std::string>();

            if (status != http::status_codes::OK.id) {
                res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, message);
				if (bRetry)
				{
					nlohmann::json root = nlohmann::json::object();
					root["duid"] = duid;
					root["params"] = httpreq.GetBody();
					root["projectid"] = 0;
					root["type"] = RECLASSIFY_TYPE::CLASSIFY_NEW_FILE;
					root["option"] = RECLASSIFY_WHEN::RECLASSIFY_NOW;

					std::string str = root.dump();
					res = m_SDRmcInstance->RPMAddNXLMetadata(str);
				}
				
				CELOG_RETURN_VAL_T(res);
            }
		}
		catch (...) {
			// The JSON data is NOT correct
			res = RESULT2(ERROR_INVALID_DATA, "Log query JSON response is not correct");
			CELOG_RETURN_VAL_T(res);
		}
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::UpdateNXLMetaDataEx(const std::wstring &nxlfilepath, const std::string &fileTags, const std::string &existingFileTags, const std::string &fileHeader)
{
	CELOG_ENTER;
	CSDRmNXLFile *nxlFile = NULL;
	SDWLResult res = OpenFileForMetaData(nxlfilepath, (ISDRmNXLFile**)&nxlFile);
	if (res)
	{
		std::string _duid = nxlFile->GetDuid();
		CloseFile(nxlFile);

		HTTPRequest httpreq = UpdateNXLMetadataExQuery(_duid, fileTags, existingFileTags, fileHeader, 0);

        nlohmann::json root = nlohmann::json::object();
        root["duid"] = _duid;
        root["params"] = httpreq.GetBody();
        root["projectid"] = 0;
        root["type"] = RECLASSIFY_TYPE::RECLASSIFY_TENANT_FILE;
        root["option"] = RECLASSIFY_WHEN::RECLASSIFY_NOW;

        std::string str = root.dump();
        res = m_SDRmcInstance->RPMAddNXLMetadata(str);
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ClassifyProjectFile(const std::wstring &nxlfilepath, unsigned int projectid, const std::string &fileName, const std::string &parentPathId, const std::string &fileTags)
{
    CELOG_ENTER;
    CSDRmNXLFile *nxlFile = NULL;
    SDWLResult res = OpenFileForMetaData(nxlfilepath, (ISDRmNXLFile**)&nxlFile);
    if (res)
    {
        std::string _duid = nxlFile->GetDuid();
        CloseFile(nxlFile);

        HTTPRequest httpreq = ClassifyProjectFileQuery(projectid, fileName, parentPathId, fileTags);

        nlohmann::json root = nlohmann::json::object();
        root["duid"] = _duid;
        root["params"] = httpreq.GetBody();
        root["projectid"] = projectid;
        root["type"] = RECLASSIFY_TYPE::RECLASSIFY_PROJECT_FILE;
        root["option"] = RECLASSIFY_WHEN::RECLASSIFY_NOW;

        std::string str = root.dump();
        res = m_SDRmcInstance->RPMAddNXLMetadata(str);
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::ResetSourcePath(const std::wstring & nxlfilepath, const std::wstring & sourcepath)
{
	CSDRmNXLFile *file = NULL;
	SDWLResult res = OpenFile(nxlfilepath, (ISDRmNXLFile**)&file);
	if (!res)
		return res;

	CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

	if (nxlFile)
	{
		nxlFile->SetSourceFilePath(conv::to_string(sourcepath));
	}

	CloseFile(file);
	return res;
}

SDWLResult CSDRmUser::ResetSourcePath(ISDRmNXLFile* file, const std::wstring & sourcepath)
{
	SDWLResult res = RESULT(0);
	CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

	if (nxlFile)
	{
		nxlFile->SetSourceFilePath(conv::to_string(sourcepath));
	}

	return res;
}


std::string CSDRmUser::BuildDynamicEvalRequest()
{
    nlohmann::json root = nlohmann::json::object();

    root["host"] = nlohmann::json::object();
    nlohmann::json& host = root["host"];

    root["application"] = nlohmann::json::object();
    nlohmann::json& application = root["application"];

    root["environments"] = nlohmann::json::array();
    root["environments"].push_back(nlohmann::json::object());
    nlohmann::json& environment = root["environments"][0];

    // Get Host Name, and Host IP
    static std::string g_hostName;
    static std::string g_hostIP;
    static std::string g_hostFQDN;

    if (g_hostName.size() <= 0)
    {
		NX::win::host hinfo;
		if (hinfo.empty())
			return "";

		g_hostName = NX::conv::utf16toutf8(hinfo.dns_host_name());
		g_hostFQDN = NX::conv::utf16toutf8(hinfo.fqdn_name());
		g_hostIP = NX::conv::utf16toutf8(hinfo.ip_address());
    }

    // host
    /* ================
        "host": {
        "ipAddress": "10.23.56.111",
        "attributes" : {
            "name": ["Lifixvs.qapf1.qalab01.nextlabs.com"]
        }
    ================ */

    host["ipAddress"] = g_hostIP;
    host["attributes"] = nlohmann::json::object();
    nlohmann::json& attribute = host["attributes"];
    attribute["name"] = nlohmann::json::array();
    attribute["name"].push_back(g_hostFQDN);

    // Get Application
    static std::wstring g_basename;
    static DWORD g_processid;
    static std::wstring g_path;
    static BOOL g_bRemoteSession;

    if (g_basename.size() <= 0)
    {
		NX::win::get_current_process_info(g_basename, g_path, g_processid, g_bRemoteSession);
    }

    application["name"] = NX::conv::utf16toutf8(g_basename);
    application["path"] = NX::conv::utf16toutf8(g_path);
    application["pid"] = g_processid;

    /*----------------
        Base on RMS wiki, the environment section should be :
    "environments" : [
    {
        "name":"environment",
            "attributes" : {
            "connection_type":[
                "console"
            ]
        }
    }
    ]
        ----------------
        Note: don't missing [ ] \in below:
        (1) "environments" : [{...}]
        (2)"connection_type" : ["..."]
    */

    // Environment
    environment["name"] = "environment";
    environment["attributes"] = nlohmann::json::object();
    nlohmann::json& attri = environment["attributes"];
    attri["connection_type"] = nlohmann::json::array();

    nlohmann::json& connection_type = attri["connection_type"];
    connection_type.push_back((g_bRemoteSession ? "remote" : "console"));

    std::string str = root.dump();
    return str;
}

SDWLResult CSDRmUser::SaveFileToTargetFolder(
    const std::string& pathId,
    const std::wstring& tmpFilePath,
    std::string& targetFolder,
    const  NX::REST::http::HttpHeaders& headers)
{
    SDWLResult res = RESULT(0);

    std::wstring strPostFileName;
    auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
        return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
    });
    if (it != headers.end())
    {
        auto pos = (*it).second.find(L"UTF-8''");
        if (pos != std::wstring::npos) {
            strPostFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
        }
    }

    if (strPostFileName.empty())
    {
        size_t pos = pathId.rfind("/");
        if (std::string::npos != pos)
        {
            strPostFileName = NX::conv::to_wstring(pathId.substr(pos + 1));
        }
    }

    std::wstring strFolder = NX::conv::to_wstring(targetFolder);
    std::wstring strOutFilePath = std::wstring(strFolder + (NX::iend_with<wchar_t>(strFolder, L"\\") ? L"" : L"\\") + L"partial_" + strPostFileName);

    // The outpath length may exceed 260
    std::wstring unlimitedOutPath = NX::fs::dos_fullfilepath(strOutFilePath).global_dos_path();

    if (!CopyFile(tmpFilePath.c_str(), unlimitedOutPath.c_str(), false))
    {
        res = RESULT2(GetLastError(), "CopyFile failed");
    }

    // Everything is done, call openFile to let Container known this file,
    // at least preare token for offline mode
    ISDRmNXLFile * pIgnored = NULL;
    OpenFile(strOutFilePath, &pIgnored);
    if (pIgnored != NULL)
        CloseFile(pIgnored);

    targetFolder = NX::conv::to_string(NX::fs::dos_fullfilepath(strOutFilePath).path());
    return res;
}

bool CSDRmUser::IsFileTagValid(
    const std::string& tag,
    std::string& outTag)
{
    if (tag.empty())
        return true;

    bool bRet = false;
    try
    {
        nlohmann::json root = nlohmann::json::parse(tag);
        if (root.is_object())
        {
            bRet = true;
            outTag = root.dump();
        }
    }
    catch (...)
    {
    }

    return bRet;
}

SDWLResult CSDRmUser::GetSharedWorkspaceListFiles(const std::string& repoId, uint32_t pageId, uint32_t pageSize, const std::string& orderBy, const std::string& pathId, const std::string& searchString, std::vector<SDR_SHARED_WORKSPACE_FILE_INFO>& listfiles)
{
	CELOG_ENTER;
	SDWLResult res;
	
	if (repoId == "") {
		res = RESULT2(ERROR_INVALID_DATA, "repoId is empty");
		return res;
	}

	// Send request.
	RMSharedWorkspace* workspace = GetSharedWorkspace();
	/*HTTPRequest httpreq = workspace->GetSharedWorkspaceFilesQuery(repoId, pageId, pageSize, searchString, orderBy, NX::conv::UrlEncode(pathId));*/
	// NX::conv::UrlEncode method has problems with the conversion of special characters, the path id is Encode by the upper caller
	HTTPRequest httpreq = workspace->GetSharedWorkspaceFilesQuery(repoId, pageId, pageSize, searchString, orderBy, pathId);
	StringBodyRequest request(httpreq);
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();

	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	StringResponse response;
	res = spConn->SendRequest(request, response);

	// Analyse reponse.
	if (!res)
		CELOG_RETURN_VAL_T(res);

	const std::string& jsonreturn = response.GetBody();
	CELOG_LOGA(CELOG_DEBUG, "GetSharedWorkspaceListFiles: response=%s\n", jsonreturn.c_str());

	try
	{
		std::vector<SHAREDWORKSPACE_FILE_INFO> files;
		RMCCORE::RetValue ret = workspace->AnalyseSharedWorkspaceListFilesResponse(jsonreturn, files);
		if (!ret)
			CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

		SDR_SHARED_WORKSPACE_FILE_INFO sinfo;
		for (SHAREDWORKSPACE_FILE_INFO info : files)
		{
			sinfo.fileId = info.fileId;
			sinfo.path = info.path;
			sinfo.pathId = info.pathId;
			sinfo.fileName = info.fileName;
			sinfo.fileType = info.fileType;
			sinfo.isProtectedFile = info.isProtectedFile;
			sinfo.lastModified = info.lastModified;
			sinfo.creationTime = info.creationTime;
			sinfo.fileSize = info.fileSize;
			sinfo.isFolder = info.isFolder;
			listfiles.push_back(sinfo);
		}
	}
	catch (...) {
		// The JSON data is NOT correct
		res = RESULT2(ERROR_INVALID_DATA, "List files query JSON response is not correct");
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetSharedWorkspaceFileMetadata(const std::string& repoId, const std::string& pathId, SDR_SHARED_WORKSPACE_FILE_METADATA& metadata)
{
	CELOG_ENTER;

	SDWLResult res;

	if (repoId == "") {
		res = RESULT2(ERROR_INVALID_DATA, "repoId is empty");
		return res;
	}

	// Send request.
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();

	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	RMSharedWorkspace *workspace = GetSharedWorkspace();
	HTTPRequest httpreq = workspace->GetSharedWorkspaceFileMetadataQuery(repoId, pathId);
	StringBodyRequest request(httpreq);
	StringResponse response;

	res = spConn->SendRequest(request, response);

	// Analyse response.
	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::string& jsonreturn = response.GetBody();
	CELOG_LOGA(CELOG_DEBUG, "GetSharedWorkspaceFileMetadata: response= %s\n", jsonreturn.c_str());

	try
	{
		SHAREDWORKSPACE_FILE_METADATA info;
		RMCCORE::RetValue ret = workspace->AnalyseSharedWorkspaceFileMetaDataResponse(jsonreturn, info);
		if (!ret)
			CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));


		SDR_SHARED_WORKSPACE_FILE_METADATA sinfo;
		sinfo.createByUser.userId = info.createByUser.userId;
		sinfo.createByUser.displayName = info.createByUser.displayName;
		sinfo.createByUser.email = info.createByUser.email;
		sinfo.lastModifiedByUser.userId = info.lastModifiedByUser.userId;
		sinfo.lastModifiedByUser.displayName = info.lastModifiedByUser.displayName;
		sinfo.lastModifiedByUser.email = info.lastModifiedByUser.email;
		sinfo.fileRights = info.fileRights;
		sinfo.fileTags = info.fileTags;
		sinfo.protectionType = info.protectionType;

		sinfo.fileId = info.fileId;
		sinfo.path = info.path;
		sinfo.pathId = info.pathId;
		sinfo.fileName = info.fileName;
		sinfo.fileType = info.fileType;
		sinfo.isProtectedFile = info.isProtectedFile;
		sinfo.lastModified = info.lastModified;
		sinfo.creationTime = info.creationTime;
		sinfo.fileSize = info.fileSize;
		sinfo.isFolder = info.isFolder;

		metadata = sinfo;
		
	}
	catch (...) {
		res = res = RESULT2(ERROR_INVALID_DATA, "Get file metadata query JSON response is not correct");
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::UploadSharedWorkspaceFile(const std::string& repoId, const std::wstring &destfolder, ISDRmNXLFile* file, int uploadType, bool userConfirmedFileOverwrite)
{
	CELOG_ENTER;

	SDWLResult res = RESULT(0);

	if (repoId == "") {
		res = RESULT2(ERROR_INVALID_DATA, "repoId is empty");
		return res;
	}

	CSDRmNXLFile* nxlFile = (CSDRmNXLFile*)file;

	UpdateNXLMetaData(file, false);

	// Send request.
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	RMSharedWorkspace *workspace = GetSharedWorkspace();
	HTTPRequest httpreq;
	httpreq = workspace->GetSharedWorkspaceUpoadFileQuery(repoId, conv::to_string(destfolder), *nxlFile, userConfirmedFileOverwrite, uploadType);

	NXLUploadRequest filerequest(httpreq, conv::to_wstring(nxlFile->GetFilePath()), RMS::boundary::End);
	StringResponse response;
	res = spConn->SendRequest(filerequest, response);

	// Analyse response.
	if (!res)
		return res;

	RMCCORE::RetValue ret = nxlFile->ImportFromRMSResponse(response.GetBody());
	if (!ret)
		res = RESULT2(ret.GetCode(), ret.GetMessage());

	return res;
}

SDWLResult CSDRmUser::DownloadSharedWorkspaceFile(const std::string& repoId, const std::string& pathId, std::wstring& targetFolder, uint32_t downloadtype, bool isNXL)
{
	CELOG_ENTER;

	SDWLResult res = RESULT(0);

	if (repoId == "") {
		res = RESULT2(ERROR_INVALID_DATA, "repoId is empty");
		return res;
	}

	// Send request.
	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile(conv::to_wstring(repoId), conv::to_wstring(pathId), tmpFilePath);
	RMSharedWorkspace * workspace = GetSharedWorkspace();
	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	HTTPRequest httpreq;
	httpreq = workspace->GetSharedWorkspaceDownloadFileQuery(repoId, pathId, 0, 0, downloadtype, isNXL);
	StringBodyRequest request(httpreq);
	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response);

	// Analyse response.
	if (!res)
		CELOG_RETURN_VAL_T(res);

	std::wstring outFilePath;
	std::wstring preferredFileName;
	unsigned short status = response.GetStatus();
	const std::wstring  phrase = response.GetPhrase();

	if (status != 200) {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
		CELOG_RETURN_VAL_T(res);
	}

	response.Finish();
	const HttpHeaders& headers = response.GetHeaders();
	auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
		return (0 == _wcsicmp(item.first.c_str(), L"Content-Disposition"));
	});
	if (it != headers.end())
	{
		auto pos = (*it).second.find(L"UTF-8''");
		if (pos != std::wstring::npos) {
			preferredFileName = NX::conv::utf8toutf16(NX::conv::UrlDecode(NX::conv::utf16toutf8((*it).second.substr(pos + 7))));
		}
	}
	outFilePath = std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + preferredFileName);


	if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(outFilePath.c_str())) {
		CELOG_RETURN_VAL_T(RESULT2(ERROR_FILE_EXISTS, "file already exists locally"));
	}

	NX::fs::dos_fullfilepath unlimitedPath(outFilePath, false);
	if (!CopyFile(tmpFilePath.c_str(), unlimitedPath.global_dos_path().c_str(), false))
	{
		res = RESULT2(GetLastError(), "CopyFile failed");
	}

	// Everything is done, call openFile to let Container known this file, 
	// at least preare token for offline mode
	ISDRmNXLFile * pIgnored = NULL;
	OpenFile(outFilePath, &pIgnored);
	if (pIgnored != NULL)
		CloseFile(pIgnored);
	DeleteFile(tmpFilePath.c_str());
	targetFolder = unlimitedPath.path();

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::IsSharedWorkspaceFileExist(const std::string& repoId, const std::string& pathId, bool& bExist)
{
	CELOG_ENTER;
	bExist = false;
	SDWLResult res;

	if (repoId == "") {
		res = RESULT2(ERROR_INVALID_DATA, "repoId is empty");
		return res;
	}

	// Send request.
	RMSharedWorkspace * workspace = GetSharedWorkspace();
	HTTPRequest httpreq = workspace->GetSharedWorkspaceCheckFileExistsQuery(repoId, pathId);
	StringBodyRequest request(httpreq);

	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();

	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	StringResponse response;
	res = spConn->SendRequest(request, response);

	// Analyse response.
	if (!res)
		CELOG_RETURN_VAL_T(res);

	const std::string& jsonreturn = response.GetBody();
	CELOG_LOGA(CELOG_DEBUG, "IsSharedWorkspaceFileExist: response=%s\n", jsonreturn.c_str());

	try
	{
		SHAREDWORKSPACE_FILE_METADATA info;
		RMCCORE::RetValue ret = workspace->AnalyseSharedWorkspaceCheckFileExistResponse(jsonreturn, bExist);
		if (!ret)
			CELOG_RETURN_VAL_T(RESULT2(ret.GetCode(), ret.GetMessage()));

	}
	catch (...) {
		res = RESULT2(ERROR_INVALID_DATA, "IsSharedWorkspaceFileExist query JSON response is not correct");
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult CSDRmUser::GetWorkspaceNxlFileHeader(const std::string& repoId, const std::string& pathId, std::string& targetFolder)
{
	CELOG_ENTER;
	SDWLResult res = RESULT(0);

	if (repoId == "") {
		res = RESULT2(ERROR_INVALID_DATA, "repoId is empty");
		return res;
	}

	// Send request.
	std::wstring tmpFilePath = L"c:\\";
	res = ProjectCreateTempFile(conv::to_wstring(repoId), conv::to_wstring(pathId), tmpFilePath);

	Client restclient(NXRMC_CLIENT_NAME, true);
	restclient.Open();
	std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));

	unsigned short status = 0;
	RMSharedWorkspace* workspace = GetSharedWorkspace();
	HTTPRequest httpreq = workspace->GetSharedWorkspaceNXLFileHeaderQuery(repoId, pathId);
	StringBodyRequest request(httpreq);
	FileResponse  response(tmpFilePath);
	res = spConn->SendRequest(request, response, INFINITE);

	// Analyse response.
	if (!res)
		CELOG_RETURN_VAL_T(res);

	status = response.GetStatus();
	response.Finish();
	if (200 == status) {
		res = SaveFileToTargetFolder(pathId, tmpFilePath, targetFolder, response.GetHeaders());
	} else {
		res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "download failed");
	}

	CELOG_RETURN_VAL_T(res);
}

std::string GetSpaceTypeString(RM_NxlFileSpaceType type)
{
	static  std::string typeStrings[] = 
	{ 
		"LOCAL_DRIVE", 
		"MY_VAULT", 
		"SHARED_WITH_ME", 
		"ENTERPRISE_WORKSPACE",
	    "PROJECT", 
		"SHAREPOINT_ONLINE", 
		"BOX", 
		"DROPBOX", 
		"GOOGLE_DRIVE", 
		"ONE_DRIVE" 
	};
	return typeStrings[type];
}

SDWLResult CSDRmUser::CopyNxlFile(const std::string& srcFileName, const std::string& srcFilePath, RM_NxlFileSpaceType srcSpaceType, const std::string& srcSpaceId,
	const std::string& destFileName, const std::string& destFolderPath, RM_NxlFileSpaceType destSpaceType, const std::string& destSpaceId, 
	bool bOverwrite, const std::string& transactionCode, const std::string& transactionId)
{
    CELOG_ENTER;
    SDWLResult res = RESULT(0);

    // Sanity check
    if (srcFileName.empty() || srcFilePath.empty() || destFolderPath.empty()) {
        CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_PARAMETER, "Missing required parameters"));
    }

    // Send request.
    Client restclient(NXRMC_CLIENT_NAME, true);
    restclient.Open();
    std::shared_ptr<Connection> spConn(restclient.CreateConnection(conv::to_wstring(GetTenant().GetRMSURL())));
    HTTPRequest  httpreq = GetCopyNxlFileQuery(srcFileName, srcFilePath, GetSpaceTypeString(srcSpaceType), srcSpaceId,
        destFileName, destFolderPath, GetSpaceTypeString(destSpaceType), destSpaceId, bOverwrite, transactionCode, transactionId);

    // Add nxl file from local drive
    if (srcSpaceType == RM_NxlFileSpaceType::LOCAL_DRIVE) {
        NXLUploadRequest copyNxlRequest(httpreq, conv::to_wstring(srcFilePath), RMS::boundary::End);
        StringResponse response;
        res = spConn->SendRequest(copyNxlRequest, response);
        if (!res) {
            CELOG_RETURN_VAL_T(res);
        }

        try
        {
            std::string strJsonResponse = response.GetBody();
            nlohmann::json root = nlohmann::json::parse(strJsonResponse);
            if (!root.is_object()) {
                CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS json error!"));
            }
            int status = root.at("statusCode");
            std::string message = root.at("message");
            if (status != http::status_codes::OK.id) {
                CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
            }
            res = RESULT2(0, message);
        }
        catch (const std::exception& e)
        {
            std::string strError("Json response error: " + std::string(e.what()));
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, strError));
        }
    }
    // Download
    else if (destSpaceType == RM_NxlFileSpaceType::LOCAL_DRIVE) {
        StringBodyRequest request(httpreq);

        std::wstring tmpFilePath(L"c:\\");
        res = ProjectCreateTempFile(conv::to_wstring(srcSpaceId), conv::to_wstring(srcFilePath), tmpFilePath);
        FileResponse  response(tmpFilePath);
        res = spConn->SendRequest(request, response);
        if (!res) {
            CELOG_RETURN_VAL_T(res);
        }

        int status = response.GetStatus();
        response.Finish();

        std::wstring targetFolder = NX::conv::utf8toutf16(destFolderPath);

        if (status == http::status_codes::OK.id)
        {
            std::wstring outFilePath = std::wstring(targetFolder + (NX::iend_with<wchar_t>(targetFolder, L"\\") ? L"" : L"\\") + NX::conv::utf8toutf16(destFileName));

            if (!PathIsDirectory(targetFolder.c_str())) {
                CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_PARAMETER, "Invalid parameter."));
            }

            const HttpHeaders& headers = response.GetHeaders();
            auto it = std::find_if(headers.begin(), headers.end(), [](const std::pair<std::wstring, std::wstring>& item)->bool {
                return (0 == _wcsicmp(item.first.c_str(), L"Content-Type"));
            });

            if (it != headers.end())
            {
                std::wstring contentType = it->second.c_str();
                contentType.erase(0, contentType.find_first_not_of(L" "));
                contentType.erase(contentType.find_last_not_of(L" ") + 1);
                if (0 == _wcsicmp(contentType.c_str(), L"application/json"))
                {
                    try
                    {
                        std::ifstream tmpFile(tmpFilePath.c_str());
                        nlohmann::json root;
                        tmpFile >> root;

                        if (!root.is_object())
                        {
                            res = RESULT2(ERROR_INVALID_DATA, "RMS json error!");
                        }
                        else
                        {
                            int status = root.at("statusCode");
                            std::string message = root.at("message");
                            res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message));
                        }
                    }
                    catch (...)
                    {
                        res = RESULT2(ERROR_INVALID_DATA, "CopyNxlFile query JSON response is not correct");
                    }
                }
                else
                {
                    std::wstring fixedOutFilePath = NX::fs::dos_fullfilepath(outFilePath).global_dos_path();
                    if (!CopyFile(tmpFilePath.c_str(), fixedOutFilePath.c_str(), !bOverwrite))
                    {
                        res = RESULT2(GetLastError(), "CopyFile failed");
                    }

                    // Everything is done, call openFile to let Container known this file,
                    // at least preare token for offline mode
                    ISDRmNXLFile * pIgnored = NULL;
                    OpenFile(outFilePath, &pIgnored);
                    if (pIgnored != NULL)
                    {
                        CloseFile(pIgnored);
                    }
                }
            }

            DeleteFile(tmpFilePath.c_str());
        }
        else {
            // For myVault & SharedWithMe, should be compatible with original download file api,
            // so when this call failed, will try download api again.
            if (srcSpaceType == MY_VAULT) {
                res = MyVaultDownloadFile(srcFilePath, targetFolder, 0);
            }
            else if (srcSpaceType == SHARED_WITH_ME) {
                res = SharedWithMeDownloadFile(NX::conv::utf8toutf16(transactionCode), NX::conv::utf8toutf16(transactionId), targetFolder, false);
            }
            else {
                res = RESULT2(status + SDWL_RMS_ERRORCODE_BASE, "Copy nxl file failed");
                CELOG_RETURN_VAL_T(res);
            }

			// fix bug 66588 -- handle the old api compatibility issue when rename
			size_t pos = targetFolder.rfind(L"\\");
			std::wstring fname = targetFolder.substr(pos + 1);
			if (res && NX::conv::utf8toutf16(destFileName) != fname) {
				// rename
				if (pos != std::wstring::npos) {
					std::wstring newpath = targetFolder.substr(0, pos) + L"\\" + NX::conv::utf8toutf16(destFileName);
					if (!MoveFile(targetFolder.c_str(), newpath.c_str())) {
						CELOG_RETURN_VAL_T(RESULT2(::GetLastError(), "Copy nxl file failed"));
					}
				}
			}
        }
    }
    else {
        StringBodyRequest request(httpreq);
        StringResponse response;
        res = spConn->SendRequest(request, response);
        if (!res) {
            CELOG_RETURN_VAL_T(res);
        }

        try
        {
            std::string strJsonResponse = response.GetBody();
            nlohmann::json root = nlohmann::json::parse(strJsonResponse);
            if (!root.is_object()) {
                CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, "RMS json error!"));
            }
            int status = root.at("statusCode");
            std::string message = root.at("message");
            if (status != http::status_codes::OK.id) {
                CELOG_RETURN_VAL_T(RESULT2(status + SDWL_RMS_ERRORCODE_BASE, NX::conv::to_string(message)));
            }
            res = RESULT2(0, message);
        }
        catch (const std::exception& e)
        {
            std::string strError("Json response error: " + std::string(e.what()));
            CELOG_RETURN_VAL_T(RESULT2(ERROR_INVALID_DATA, strError));
        }
    }

    return res;
}
