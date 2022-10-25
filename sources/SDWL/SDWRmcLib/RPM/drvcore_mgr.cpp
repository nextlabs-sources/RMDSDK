
#include <crtdbg.h>
#include <vector>
#include <windows.h>
#include <assert.h>
#include <winternl.h>
#include <winioctl.h>
#include "common/celog2/celog.h"
#include "..\Common\string.h"
#include "..\Common\stringex.h"
#include "rmccore/network/httpConst.h"

#include "rbtree.h"
#include "nxrmdrv.h"
#include "nxrmcorehlp.h"
#include "drvcore_mgr.h"
#include "..\..\RPM\nudf\v2\inc\nudf\asyncpipe.hpp"
#include "nlohmann/json.hpp"

#define CELOG_CUR_MODULE "rmdsdk"
#define CELOG_CUR_FILE CELOG_FILEPATH_SOURCES_SDWL_SDWRMCLIB_RPM_DRVCOREMGR_CPP

drvcore_mgr::drvcore_mgr() : _h(NULL), _session_id(-1), _t_option(0)
{
	bool result = false;
	ProcessIdToSessionId(GetCurrentProcessId(), &_session_id);
	InitializeCriticalSection(&_lock);

	::EnterCriticalSection(&_lock);
	_h = init_transporter_client();
	::LeaveCriticalSection(&_lock);
}

drvcore_mgr::~drvcore_mgr()
{
	if (NULL != _h) {
		close_transporter_client(_h);
		_h = NULL;
	}

	DeleteCriticalSection(&_lock);
}

bool drvcore_mgr::isDriverExist()
{
	if (_h == NULL)
		return false;
	else
		return true;
}

SDWLResult drvcore_mgr::get_response_status(std::string& result)
{
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

		uint32_t status = 0;
		if (root.find("code") !=root.end())
			status = root.at("code").get<uint32_t>();
		if (root.find("statusCode") != root.end())
			status = root.at("statusCode").get<uint32_t>();

        std::string msg = root.at("message").get<std::string>();

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult drvcore_mgr::get_registry_response_status(const std::string& response)
{
    try
    {
        nlohmann::json root = nlohmann::json::parse(response);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + response);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();
        if (root.end() != root.find("return"))
        {
            msg = root["return"].dump();
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult handle_get_protected_profiles_dir_response(
    const std::string& result,
    std::wstring& strPath)
{
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

        uint32_t status = root.at("code").get<uint32_t>();

        std::string msg;
        if (root.end() != root.find("message"))
        {
            msg = root["message"].get<std::string>();
        }

        if (root.end() != root.find("path"))
        {
            strPath = NX::conv::to_wstring(root["path"].get<std::string>());
        }

        if (0 == status)
        {
            return RESULT(0);
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult handle_get_user_info_response(
    const std::string& result,
    std::wstring& router,
    std::wstring& tenant,
    std::wstring& workingfolder,
    std::wstring& tempfolder,
    std::wstring& sdklibfolder,
    bool &blogin)
{
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        if (ERROR_SUCCESS == status)
        {
            router = NX::conv::to_wstring(root.at("router").get<std::string>());
            tenant = NX::conv::to_wstring(root.at("tenant").get<std::string>());
            workingfolder = NX::conv::to_wstring(root.at("workingfolder").get<std::string>());
            tempfolder = NX::conv::to_wstring(root.at("tempfolder").get<std::string>());
            sdklibfolder = NX::conv::to_wstring(root.at("sdklibfolder").get<std::string>());
            blogin = root.at("logined").get<bool>();
        }

        std::string msg;
        if (root.end() != root.find("message"))
        {
            msg = root.at("message").get<std::string>();
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult handle_read_file_tags_response(
    const std::string& result,
    std::wstring& strTags)
{
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg;
        if (root.end() != root.find("message"))
        {
            msg = root["message"].get<std::string>();
        }

        strTags = NX::conv::utf8toutf16(root.at("tags").get<std::string>());
        if (0 == status)
        {
            return RESULT(0);
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult handle_get_file_status_response(
    const std::string& result,
    uint32_t& dir_status,
    bool& nxl_file)
{
    nxl_file = false;
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();
        dir_status = root.at("dirstatus").get<uint32_t>();

        uint32_t u32NxlState = root.at("nxlstate").get<uint32_t>();
        if (u32NxlState > 0)
        {
            nxl_file = true;
        }

        if (0 == status)
        {
            return RESULT(0);
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult handle_popup_new_token_response(
    const std::string& result,
    std::string &token_id,
    std::string &token_otp,
    std::string &token_value)
{
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();

        if (root.end() != root.find("token"))
        {
            nlohmann::json& token = root["token"];
            token_id = token.at("id").get<std::string>();
            token_otp = token.at("otp").get<std::string>();
            token_value = token.at("value").get<std::string>();
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!");
    }
}

SDWLResult handle_find_cached_token_response(
    const std::string& result,
    std::string &token_id,
    std::string &token_otp,
    std::string &token_value,
    time_t &token_ttl)
{
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();

        if (root.end() != root.find("token"))
        {
            nlohmann::json& token = root.at("token");
            token_id = token.at("id").get<std::string>();
            token_otp = token.at("otp").get<std::string>();
            token_value = token.at("value").get<std::string>();
            token_ttl = token.at("ttl").get<uint64_t>();
        }
        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult handle_is_app_registered_response(std::string& result, bool& bregisted)
{
    bregisted = false;
    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();
        bregisted = root.at("register").get<bool>();

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

SDWLResult handle_is_dir_response_common(const std::string& result, uint32_t& dir_status, NXSERV_REQUEST req, SDRmRPMFolderOption* option, std::wstring& file_tags)
{
    CELOG_ENTER;

    try
    {
        nlohmann::json root = nlohmann::json::parse(result);
        if (!root.is_object())
        {
            CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result));
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();
        dir_status = root.at("dirstatus").get<uint32_t>();

        if (req == CTL_RPM_FOLDER && option != NULL && root.end() != root.find("option"))
        {
            *option = root.at("option").get<SDRmRPMFolderOption>();
        }

        if (root.end() != root.find("filetags"))
        {
            file_tags = NX::conv::utf8toutf16(root["filetags"].get<std::string>());
        }
        else
        {
            file_tags.clear();
        }

        CELOG_RETURN_VAL_T(RESULT2(status, msg));
    }
    catch (std::exception &e)
    {
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_JSON_FORMAT, e.what()));
    }
    catch (...)
    {
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct"));
    }
}

bool to_vec_right_watermark(
    const nlohmann::json& arrRightsWatermark,
    std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& vecRightsWatermark, uint64_t u64Rights)
{
    bool bRet = false;
    for (auto& item : arrRightsWatermark)
    {
        std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> pairRightWatermarks;
        pairRightWatermarks.first = item.at("right").get<SDRmFileRight>();

        const nlohmann::json& arrWatermark = item.at("watermark");
        for (auto& watermark : arrWatermark)
        {
            SDR_WATERMARK_INFO watermarkItem;
            watermarkItem.text = watermark.at("text").get<std::string>();
            watermarkItem.fontName = watermark.at("fontname").get<std::string>();
            watermarkItem.fontColor = watermark.at("fontcolor").get<std::string>();

            watermarkItem.fontSize = watermark.at("fontsize").get<int>();
            watermarkItem.transparency = watermark.at("transparency").get<int>();
            watermarkItem.rotation = watermark.at("rotation").get<WATERMARK_ROTATION>();
            watermarkItem.repeat = watermark.at("repeat").get<bool>();

            pairRightWatermarks.second.push_back(watermarkItem);
        }

        vecRightsWatermark.push_back(pairRightWatermarks);
        bRet = true;
    }

    //
    if (vecRightsWatermark.empty())
    {
        std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> pairRightWatermarks;
        pairRightWatermarks.first = (SDRmFileRight)u64Rights;
        vecRightsWatermark.push_back(pairRightWatermarks);

        bRet = true;
    }

    return bRet;
}

SDWLResult handle_opened_file_rights_response(
    const std::string& response,
    std::map<std::wstring,
    std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>>&mapOpenedFileRights,
    std::wstring &ex_useremail)
{
    mapOpenedFileRights.clear();

    try
    {
        nlohmann::json root = nlohmann::json::parse(response);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + response);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();

        if (root.end() != root.find("user"))
        {
            ex_useremail = NX::conv::utf8toutf16(root["user"].get<std::string>());
        }

        if (root.end() != root.find("filerights"))
        {
            const nlohmann::json& arrFileRights = root["filerights"];
            for (auto& item : arrFileRights)
            {
                std::wstring strPath = NX::conv::to_wstring(item.at("path").get<std::string>());
                uint64_t fileRights = item.at("rights").get<uint64_t>();

                const nlohmann::json& arrWatermark = item.at("rightswatermark");
                std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> vecRightsWatermark;
                if (to_vec_right_watermark(arrWatermark, vecRightsWatermark, fileRights))
                {
                    mapOpenedFileRights.insert(std::make_pair(strPath, vecRightsWatermark));
                }
            }
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

bool to_vec_right_watermark(
    const nlohmann::json& arrRightsWatermark,
    std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& vecRightsWatermark)
{
    bool bRet = false;
    for (auto& item : arrRightsWatermark)
    {
        std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> pairRightWatermarks;
        pairRightWatermarks.first = item.at("right").get<SDRmFileRight>();

        const nlohmann::json& arrWatermark = item.at("watermark");
        for (auto& watermark : arrWatermark)
        {
            SDR_WATERMARK_INFO watermarkItem;
            watermarkItem.text = watermark.at("text").get<std::string>();
            watermarkItem.fontName = watermark.at("fontname").get<std::string>();
            watermarkItem.fontSize = watermark.at("fontsize").get<int>();
            watermarkItem.transparency = watermark.at("transparency").get<int>();
            watermarkItem.rotation = watermark.at("rotation").get<WATERMARK_ROTATION>();
            watermarkItem.repeat = watermark.at("repeat").get<bool>();

            pairRightWatermarks.second.push_back(watermarkItem);
        }

        vecRightsWatermark.push_back(pairRightWatermarks);
        bRet = true;
    }

    return bRet;
}

SDWLResult handle_get_rights_response(
    const std::string& response,
    std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks,
    uint64_t& u64Rights)
{
    u64Rights = 0;
    rightsAndWatermarks.clear();
    try
    {
        nlohmann::json root = nlohmann::json::parse(response);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + response);
        }

        uint32_t status = root.at("code").get<uint32_t>();
        std::string msg = root.at("message").get<std::string>();

        if (root.end() != root.find("rights"))
        {
            u64Rights = root["rights"].get<uint64_t>();
        }

        if (root.end() != root.find("rightswatermark"))
        {
            const nlohmann::json& arrWatermark = root["rightswatermark"];
            to_vec_right_watermark(arrWatermark, rightsAndWatermarks);
        }

        return RESULT2(status, msg);
    }
    catch (std::exception &e)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, e.what());
    }
    catch (...)
    {
        return RESULT2(SDWL_INVALID_JSON_FORMAT, "JSON data is not correct");
    }
}

std::vector<SDRmFileRight> drvcore_mgr::to_sdrm_fileright(uint64_t u64Rights)
{
	std::vector<SDRmFileRight> setFileRights;
	if (RIGHT_VIEW & u64Rights)
		setFileRights.push_back(RIGHT_VIEW);

	if (RIGHT_EDIT & u64Rights)
        setFileRights.push_back(RIGHT_EDIT);

	if (RIGHT_PRINT & u64Rights)
        setFileRights.push_back(RIGHT_PRINT);

	if (RIGHT_CLIPBOARD & u64Rights)
        setFileRights.push_back(RIGHT_CLIPBOARD);

	if (RIGHT_SAVEAS & u64Rights)
		setFileRights.push_back(RIGHT_SAVEAS);

	if (RIGHT_DECRYPT & u64Rights)
		setFileRights.push_back(RIGHT_DECRYPT);

	if (RIGHT_SCREENCAPTURE & u64Rights)
		setFileRights.push_back(RIGHT_SCREENCAPTURE);

	if (RIGHT_SEND & u64Rights)
		setFileRights.push_back(RIGHT_SEND);

	if (RIGHT_CLASSIFY & u64Rights)
		setFileRights.push_back(RIGHT_CLASSIFY);

	if (RIGHT_SHARE & u64Rights)
		setFileRights.push_back(RIGHT_SHARE);

	if (RIGHT_DOWNLOAD & u64Rights)
		setFileRights.push_back(RIGHT_DOWNLOAD);

	return std::move(setFileRights);
}

SDWLResult drvcore_mgr::insert_dir(const std::wstring& path, uint32_t option, const std::wstring& filetags)
{
    return insert_dir_common(CTL_SERV_INSERT_DIR, path, option, filetags);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
SDWLResult drvcore_mgr::insert_sanctuary_dir(const std::wstring& path, const std::wstring& filetags)
{
    return insert_dir_common(CTL_SERV_INSERT_SANCTUARY_DIR, path, 0, filetags);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

SDWLResult drvcore_mgr::insert_dir_common(NXSERV_REQUEST req, const std::wstring& path, uint32_t option, const std::wstring& filetags)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = req;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(path);
        if (req == CTL_SERV_INSERT_DIR) {
            param["option"] = option;
        }
        param["fileTags"] = NX::conv::utf16toutf8(filetags);

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            if (result.find("succeed") != std::string::npos)
                return res;

            return get_response_status(result);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }
    return res;
}

SDWLResult drvcore_mgr::remove_dir(const std::wstring& path, bool bForce)
{
    return remove_dir_common(CTL_SERV_REMOVE_DIR, path, bForce);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
SDWLResult drvcore_mgr::remove_sanctuary_dir(const std::wstring& path)
{
    return remove_dir_common(CTL_SERV_REMOVE_SANCTUARY_DIR, path);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

SDWLResult drvcore_mgr::remove_dir_common(NXSERV_REQUEST req, const std::wstring& path, bool bForce)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = req;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(path);
        if (req == CTL_SERV_REMOVE_DIR) {
            param["bforce"] = bForce;
        }

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }
    return res;
}

SDWLResult drvcore_mgr::set_clientid(const std::wstring& clientid)
{
    SDWLResult res = RESULT(0);

    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_CLIENT_ID;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_CLIENTID] = NX::conv::utf16toutf8(clientid);
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }
    return res;
}

SDWLResult drvcore_mgr::set_login_result(const std::string& strJson, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json extra = nlohmann::json::parse(strJson);
        if (!extra.is_object())
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid login data");
        }

        if (extra.end() == extra.find("extra"))
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid login JSON data");
        }

        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_FINALIZE_LOGIN;
        root["parameters"] = extra["extra"];

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        if (str.size() > NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE)
            return RESULT2(SDWL_INVALID_DATA, "data is bigger than 258k");

        // login data can be big, use QUERY_SERVICE_REQUEST_BIG 
        std::shared_ptr<QUERY_SERVICE_REQUEST_BIG> request_blob = std::make_shared<QUERY_SERVICE_REQUEST_BIG>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST_BIG), 30000);
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::add_cached_token(const std::string &token_id, const std::string &token_otp, const std::string &token_value)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_SET_CACHED_TOKEN;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param["id"] = token_id;
        param["otp"] = token_otp;
        param["value"] = token_value;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), NXSERV_REQUEST_SERVICE_DATA_SIZE));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::remove_cached_token(const std::string& duid)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_REMOVE_CACHED_TOKEN;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param["duid"] = duid;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }
        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return get_response_status(result);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::delete_file_token(const std::wstring& filename)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_DELETE_FILE_TOKEN;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(filename);
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return get_response_status(result);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::add_activity_log(const std::string& strJson)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        if (0 >= strJson.size())
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid Json data");
        }

        nlohmann::json param = nlohmann::json::parse(strJson);
        if (!param.is_object())
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid Json data");
        }
        param[NXSERV_REQUEST_PARAM_SECURITY] = "";

        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_ADD_ACTIVITY_LOG;
        root["parameters"] = param;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), NXSERV_REQUEST_SERVICE_DATA_SIZE));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }
    return res;
}

SDWLResult drvcore_mgr::add_nxl_metadata(const std::string& strJson)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json param = nlohmann::json::parse(strJson);
        if (!param.is_object())
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid parameter");
        }

        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_ADD_NXL_METADATA;

        param[NXSERV_REQUEST_PARAM_SECURITY] = "";
        root["parameters"] = param;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }
    return res;
}

SDWLResult drvcore_mgr::lock_file_forsync(const std::string& strJson)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json param = nlohmann::json::parse(strJson);
        if (!param.is_object())
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid parameter");
        }

        param[NXSERV_REQUEST_PARAM_SECURITY] = "";
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_LOCK_NXL_FORSYNC;
        root["parameters"] = param;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), NXSERV_REQUEST_SERVICE_DATA_SIZE));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }
    return res;
}

SDWLResult drvcore_mgr::request_run_registry_command(const std::string& reg_cmd, const std::string &security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json registry_call = nlohmann::json::parse(reg_cmd);
        if (!registry_call.is_object())
        {
            return RESULT2(SDWL_INVALID_DATA, "Input JSON data error");
        }

        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_RUN_REGISTRY_CMD;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];

        param["security"] = security;
        param["registry_call"] = registry_call;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_registry_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}


SDWLResult drvcore_mgr::request_apiuser_logindata(std::wstring& apiuser_logindata)
{
	SDWLResult res = RESULT(0);
	if (NULL == _h)
		return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

	BOOL bval = is_transporter_enabled(_h);
	try {
		nlohmann::json root = nlohmann::json::object();
		root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_QUERY_APIUSER;
		root["parameters"] = nlohmann::json::object();
		nlohmann::json& param = root["parameters"];

		std::string str = root.dump();

		std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
		request_blob->SessionId = _session_id;
		request_blob->ProcessId = GetCurrentProcessId();
		request_blob->ThreadId = GetCurrentThreadId();
		if (0 != str.length()) {
			memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
		}

		std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
		if (!response.empty()) {
			apiuser_logindata = std::wstring(response.begin(), response.end());
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
	}

	return res;
}

SDWLResult drvcore_mgr::request_is_apiuser()
{
	SDWLResult res = RESULT(SDWL_NOT_FOUND);
	if (NULL == _h)
		return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

	BOOL bval = is_transporter_enabled(_h);
	try {
		nlohmann::json root = nlohmann::json::object();
		root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_QUERY_APIUSER;
		root["parameters"] = nlohmann::json::object();
		nlohmann::json& param = root["parameters"];

		std::string str = root.dump();

		std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
		request_blob->SessionId = _session_id;
		request_blob->ProcessId = GetCurrentProcessId();
		request_blob->ThreadId = GetCurrentThreadId();
		if (0 != str.length()) {
			memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
		}

		std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
		if (!response.empty()) {
			std::string result = std::string(response.begin(), response.end());
			SDWLResult res2 = get_response_status(result);
			if (res2.GetCode() == 200)
				return RESULT(SDWL_SUCCESS);
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
	}

	return res;
}

SDWLResult drvcore_mgr::request_opened_file_rights(std::map<std::wstring, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>>& mapOpenedFileRights, const std::string &security, std::wstring &ex_useremail, unsigned long ulProcessId)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        uint32_t u32ProcessId = ulProcessId;
        if (0 == ulProcessId)
        {
            u32ProcessId = GetCurrentProcessId();
        }

        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_QUERY_OPENED_FILE_RIGHTS;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param["security"] = security;
        param["process"] = u32ProcessId;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }
        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = handle_opened_file_rights_response(result, mapOpenedFileRights, ex_useremail);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::set_user_attr(const std::string& strJson)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json extra = nlohmann::json::parse(strJson);
        if (!extra.is_object())
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid parameter");
        }

        if (extra.end() == extra.find("extra"))
        {
            return RESULT2(SDWL_INVALID_DATA, "Invalid login JSON data");
        }

        nlohmann::json param = extra.at("extra");
        param[NXSERV_REQUEST_PARAM_SECURITY] = "";
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_SYNC_USER_ATTR;
        root["parameters"] = param;
        std::string str = root.dump();

        if (str.size() > NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE)
            return RESULT2(SDWL_INVALID_DATA, "data is bigger than 258k");

        std::shared_ptr<QUERY_SERVICE_REQUEST_BIG> request_blob = std::make_shared<QUERY_SERVICE_REQUEST_BIG>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST_BIG));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }
    return res;
}

SDWLResult drvcore_mgr::logout()
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_USER_LOGOUT;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::set_service_stop(bool enable, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_SET_SERVICE_STOP;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_ENABLE] = enable;
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        // Allocate this 258KB blob on the heap instead of putting it in a
        // local variable.  Otherwise, when this function is called by our
        // installer custom action DLL which is invoked on a thread that only
        // has a total of 256KB stack space (2KB less than this one single
        // variable!), stack overflow occurs.
        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}


//
// The following functions are backdoors for installer to stop the
// RPM service without storing the security string somewhere in the installer.
// They should only be compiled into the internal version of the Runtime Lib
// SDWRmcLib_forInstaller.lib.  They should not be compiled into the public
// version of the Runtime Lib SDWRmcLib.lib, so as to avoid reverse
// engineering.
//
// Malicious users can still attempt to reverse-engineer the installer to
// reveal the backdoors, but the chance is lower.  Plus it is still better
// than revealing the security string itself.
//
#ifdef SDWRMCLIB_FOR_INSTALLER

SDWLResult drvcore_mgr::set_service_stop_no_security(bool enable)
{
	SDWLResult res = RESULT(0);

	if (NULL == _h)
		return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

	BOOL bval = is_transporter_enabled(_h);
	try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_SET_SERVICE_STOP_NO_SECURITY;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_ENABLE] = enable;
        std::string str = root.dump();

		// Allocate this 258KB blob on the heap instead of putting it in a
		// local variable.  Otherwise, when this function is called by our
		// installer custom action DLL which is invoked on a thread that only
		// has a total of 256KB stack space (2KB less than this one single
		// variable!), stack overflow occurs.
		std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
		request_blob->SessionId = _session_id;
		request_blob->ProcessId = GetCurrentProcessId();
		request_blob->ThreadId = GetCurrentThreadId();
		if (0 != str.length()) {
			memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
		}

		std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
		if (!response.empty()) {
			std::string result = std::string(response.begin(), response.end());
			res = get_response_status(result);
			return res;
		}
	}
	catch (const std::bad_alloc& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
	}

	return res;
}

SDWLResult drvcore_mgr::stop_service_no_security()
{
    SDWLResult res = RESULT(0);

    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_STOP_SERVICE_NO_SECURITY;
        std::string str = root.dump();

        // Allocate this 258KB blob on the heap instead of putting it in a
        // local variable.  Otherwise, when this function is called by our
        // installer custom action DLL which is invoked on a thread that only
        // has a total of 256KB stack space (2KB less than this one single
        // variable!), stack overflow occurs.
        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

#endif // SDWRMCLIB_FOR_INSTALLER

SDWLResult drvcore_mgr::delete_cheched_token()
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_DELETE_CACHED_TOKEN;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::get_secret_dir(std::wstring& filepath)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_SECRET_DIR;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            filepath = NX::conv::to_wstring(res.GetMsg());
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::set_router_key(std::wstring router, std::wstring tenant, std::wstring workingfolder, std::wstring tempfolder, std::wstring sdklibfolder)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        std::string strRouter = NX::conv::utf16toutf8(router);
        std::string strTenant = NX::conv::utf16toutf8(tenant);
        std::string strWorkingFolder = NX::conv::utf16toutf8(workingfolder);
        std::string strTempFolder = NX::conv::utf16toutf8(tempfolder);
        std::string strSdkLibFolder = NX::conv::utf16toutf8(sdklibfolder);

        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_SET_ROUTER;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param["router"] = strRouter;
        param["tenant"] = strTenant;
        param["workingfolder"] = strWorkingFolder;
        param["tempfolder"] = strTempFolder;
        param["sdklibfolder"] = strSdkLibFolder;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::register_app(const std::wstring& appPath, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_REGISTER_APP;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_APPPATH] = NX::conv::utf16toutf8(appPath);
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::unregister_app(const std::wstring& appPath, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_UNREGISTER_APP;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_APPPATH] = NX::conv::utf16toutf8(appPath);
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::notify_rmx_status(bool running, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_NOTIFY_RMX_STATUS;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_RUNNING] = running;
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::add_trusted_process(unsigned long processId, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_ADD_TRUSTED_PROCESS;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_PROCESSID] = processId;
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::remove_trusted_process(unsigned long processId, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_REMOVE_TRUSTED_PROCESS;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_PROCESSID] = processId;
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::add_trusted_app(const std::wstring& appPath, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_ADD_TRUSTED_APP;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_APPPATH] = NX::conv::utf16toutf8(appPath);
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::remove_trusted_app(const std::wstring& appPath, const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_REMOVE_TRUSTED_APP;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_APPPATH] = NX::conv::utf16toutf8(appPath);
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::get_rpm_folder(std::vector<std::wstring> &folders, SDRmRPMFolderQuery option)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_RPM_FOLDER;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param["option"] = (unsigned int) option;
        std::string str = root.dump();
        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            nlohmann::json root = nlohmann::json::parse(result);
            if (!root.is_object())
            {
                return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
            }
            unsigned int ret = 1;
            std::string msg;

            if (root.end() != root.find("code"))
            {
                ret = root["code"].get<uint32_t>();
            }

            if (root.end() != root.find("message"))
            {
                msg = root["message"].get<std::string>();
            }

            if (root.end() != root.find("folders"))
            {
                const nlohmann::json& arrFolders = root["folders"];
                for (auto& item : arrFolders)
                {
                    std::wstring strPath = NX::conv::to_wstring(item.at("path").get<std::string>());
                    folders.push_back(strPath);
                }
            }


            return RESULT2(ret, msg);
        }

    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::is_rpm_folder(const std::wstring& path, uint32_t* dirstatus, SDRmRPMFolderOption* option, std::wstring& filetags)
{
    return is_dir_common(CTL_RPM_FOLDER, path, dirstatus, option, filetags);
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
SDWLResult drvcore_mgr::is_sanctuary_folder(const std::wstring& path, uint32_t* dirstatus, std::wstring& filetags)
{
    return is_dir_common(CTL_SERV_IS_SANCTUARY_DIR, path, dirstatus, NULL, filetags);
}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

SDWLResult drvcore_mgr::is_dir_common(NXSERV_REQUEST req, const std::wstring& path, uint32_t* dirstatus, SDRmRPMFolderOption* option, std::wstring& filetags)
{
    CELOG_ENTER;

    SDWLResult res = RESULT(0);
    *dirstatus = 0;

    if (NULL == _h)
        CELOG_RETURN_VAL_T(RESULT2(SDWL_NOT_READY, "RPM driver is not ready"));

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = req;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(path);

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            CELOG_RETURN_VAL_T(handle_is_dir_response_common(result, *dirstatus, req, option, filetags));
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service"));
    }

    CELOG_RETURN_VAL_T(res);
}

SDWLResult drvcore_mgr::get_user_info(std::wstring& router, std::wstring& tenant, std::wstring& workingfolder, std::wstring& tempfolder, std::wstring& sdklibfolder, bool &blogin)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_USER_INFO;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());

            return handle_get_user_info_response(result, router, tenant, workingfolder, tempfolder, sdklibfolder, blogin);
        }
        else // RESPONSE is empty
        {
            CELOG_LOG(CELOG_ERROR, L"response = send_request is EMPTY!");
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::delete_file(const std::wstring& filepath)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_DELETE_FILE;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(filepath);
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            if (result.find("succeed") != std::string::npos)
                return res;

            return get_response_status(result);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::delete_folder(const std::wstring& folderpath)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_DELETE_FOLDER;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(folderpath);
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            if (result.find("succeed") != std::string::npos)
                return res;

            return get_response_status(result);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::copy_file(const std::wstring& srcpath, const std::wstring& destpath, bool deletesource)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_COPY_FILE;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];

        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(destpath);
        param[NXSERV_REQUEST_PARAM_SOURCEPATH] = NX::conv::utf16toutf8(srcpath);
        param[NXSERV_REQUEST_PARAM_DELSOURCE] = deletesource;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            if (result.find("succeed") != std::string::npos)
                return res;

            return get_response_status(result);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::get_rights(const std::wstring& filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, int option)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_RIGHTS;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param["option"] = option;
        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(filepath);

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            uint64_t rights = 0;
            SDWLResult ret = handle_get_rights_response(result, rightsAndWatermarks, rights);

            if ((0 == rightsAndWatermarks.size()) && (rights > 0))
            {
                //rights
                std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> pairRightWatermark;
                std::vector<SDR_WATERMARK_INFO> watermarkInfo;
                if (rights & RIGHT_VIEW)
                {
                    pairRightWatermark.first = RIGHT_VIEW;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_EDIT)
                {
                    pairRightWatermark.first = RIGHT_EDIT;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_PRINT)
                {
                    pairRightWatermark.first = RIGHT_PRINT;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_CLIPBOARD)
                {
                    pairRightWatermark.first = RIGHT_CLIPBOARD;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_SAVEAS)
                {
                    pairRightWatermark.first = RIGHT_SAVEAS;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_DECRYPT)
                {
                    pairRightWatermark.first = RIGHT_DECRYPT;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_SCREENCAPTURE)
                {
                    pairRightWatermark.first = RIGHT_SCREENCAPTURE;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_SEND)
                {
                    pairRightWatermark.first = RIGHT_SEND;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_CLASSIFY)
                {
                    pairRightWatermark.first = RIGHT_CLASSIFY;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_SHARE)
                {
                    pairRightWatermark.first = RIGHT_SHARE;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }

                if (rights & RIGHT_DOWNLOAD)
                {
                    pairRightWatermark.first = RIGHT_DOWNLOAD;
                    pairRightWatermark.second = watermarkInfo;
                    rightsAndWatermarks.push_back(pairRightWatermark);
                }
            }

            return ret;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::get_file_status(const std::wstring& filename, uint32_t* dirstatus, bool* filestatus)
{
    SDWLResult res = RESULT(0);
    *dirstatus = 0;
    *filestatus = false;

    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_FILE_STATUS;

        std::string strFileName = NX::conv::utf16toutf8(filename);
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_FILEPATH] = strFileName;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return handle_get_file_status_response(result, *dirstatus, *filestatus);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::get_file_info(const std::wstring &filepath, std::string &duid, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &userRightsAndWatermarks,
	std::vector<SDRmFileRight> &rights, SDR_WATERMARK_INFO &waterMark, SDR_Expiration &expiration, std::string &tags,
	std::string &tokengroup, std::string &creatorid, std::string &infoext, DWORD &attributes, DWORD &isRPMFolder, DWORD &isNXLFile, bool checkOwner)
{
	SDWLResult res = RESULT(0);
	isRPMFolder = 0;
	isNXLFile = false;

	if (NULL == _h)
		return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

	BOOL bval = is_transporter_enabled(_h);
	try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_FILE_STATUS;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];

        param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(filepath);
        param["FullQuery"] = 1; // 1: Query all information of the file
        param["CheckOwner"] = checkOwner;
        std::string str = root.dump();

		std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
		request_blob->SessionId = _session_id;
		request_blob->ProcessId = GetCurrentProcessId();
		request_blob->ThreadId = GetCurrentThreadId();
		if (0 != str.length()) {
			memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
		}

		std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
		
        if (!response.empty())
		{
			std::string result = std::string(response.begin(), response.end());

            unsigned int filestate = 0;
            nlohmann::json root = nlohmann::json::parse(result);
            if (!root.is_object())
            {
                return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
            }

            unsigned int ret = 1;
            std::string msg;

            if (root.end() != root.find("code"))
            {
                ret = root["code"].get<uint32_t>();
            }

            if (root.end() != root.find("message"))
            {
                msg = root["message"].get<std::string>();
            }

            if (root.end() != root.find("dirstatus"))
            {
                isRPMFolder = root["dirstatus"].get<uint32_t>();
            }

            if (root.end() != root.find("nxlstate"))
            {
                filestate = root["nxlstate"].get<uint32_t>();
                if (filestate > 0)
                    isNXLFile = true;
            }

			// tags in header
			if (root.end() != root.find("duid"))
			{
				duid = root["duid"].get<std::string>();
			}
			
			if (root.end() != root.find("attributes"))
            {
                attributes = root["attributes"].get<uint32_t>();
            }

			// following is the user rights
            if (root.end() != root.find("rightswatermark"))
            {
                const nlohmann::json& arrRightsWatermark = root["rightswatermark"];
                for (auto objRightWatermark : arrRightsWatermark)
                {
                    std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> pairRightWatermarks;
                    if(objRightWatermark.end() != objRightWatermark.find("right"))
                    { 
                        pairRightWatermarks.first = objRightWatermark["right"].get<SDRmFileRight>();
                    }

                    if (objRightWatermark.end() != objRightWatermark.find("watermark"))
                    {
                        const nlohmann::json& arrWatermark = objRightWatermark["watermark"];
                        for (auto item : arrWatermark)
                        {
                            SDR_WATERMARK_INFO watermarkItem;
                            watermarkItem.text = item["text"].get<std::string>();
                            watermarkItem.fontName = item["fontname"].get<std::string>();
                            watermarkItem.fontColor = item["fontcolor"].get<std::string>();
                            watermarkItem.fontSize = item["fontsize"].get<int>();
                            watermarkItem.transparency = item["transparency"].get<int>();
                            watermarkItem.rotation = item["rotation"].get<WATERMARK_ROTATION>();
                            watermarkItem.repeat = item["repeat"].get<bool>();

                            pairRightWatermarks.second.push_back(watermarkItem);
                        }

                        userRightsAndWatermarks.push_back(pairRightWatermarks);
                    }
                }
            }

			// following is the ad-hoc rights in NXL header
			uint64_t _rights = 0;
            if (root.end() != root.find("rights"))
            {
                _rights = root["rights"].get<uint64_t>();

                if (_rights > 0)
                {//rights
                    rights = to_sdrm_fileright(_rights);
                }
            }

			// watermark in header
            if (root.end() != root.find("watermark"))
            {
                const nlohmann::json& watermark = root["watermark"];
                waterMark.text = watermark["text"].get<std::string>();
                waterMark.fontName = watermark["fontname"].get<std::string>();
                waterMark.fontColor = watermark["fontcolor"].get<std::string>();
                waterMark.fontSize = watermark["fontsize"].get<int>();
                waterMark.transparency = watermark["transparency"].get<int>();
                waterMark.rotation = watermark["rotation"].get<WATERMARK_ROTATION>();
                waterMark.repeat = watermark["repeat"].get<bool>();
            }

			// expiration in header
            if (root.end() != root.find("expiry"))
            {
                SDR_Expiration _expiration;
                const nlohmann::json& validity = root["expiry"];
                if (validity.end() != validity.find("endDate"))
                {
                    _expiration.end = validity["endDate"].get<uint64_t>();
                }

                if (validity.end() != validity.find("startDate"))
                {
                    _expiration.start = validity["startDate"].get<uint64_t>();
                    _expiration.type = RANGEEXPIRE;
                }
                else
                {
                    _expiration.type = ABSOLUTEEXPIRE;
                }

                expiration = _expiration;
            }

			// tags in header
            if (root.end() != root.find("tags"))
            {
                tags = root["tags"].get<std::string>();
            }

			// fileinfo section in header
            if (root.end() != root.find("infoext"))
            {
                infoext = root["infoext"].get<std::string>();
            }

			// creator in header
            if (root.end() != root.find("creator"))
            {
                creatorid = root["creator"].get<std::string>();
            }

			// tokengroup in header
            if (root.end() != root.find("tokengroup"))
            {
                tokengroup = root["tokengroup"].get<std::string>();
            }

            return RESULT2(ret, msg);
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
	}

	return res;
}

SDWLResult drvcore_mgr::windows_encrypt_file(const std::wstring &filePath) {
	SDWLResult res = RESULT(0);
	try {
		nlohmann::json root = nlohmann::json::object();
		root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_WINDOWS_ENCRYPT_FILE;
		root["parameters"] = nlohmann::json::object();
		nlohmann::json& param = root["parameters"];

		param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(filePath);
		std::string str = root.dump();

		std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
		request_blob->SessionId = _session_id;
		request_blob->ProcessId = GetCurrentProcessId();
		request_blob->ThreadId = GetCurrentThreadId();

		if (0 != str.length()) {
			memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
		}

		std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
		if (!response.empty()) {
			std::string result = std::string(response.begin(), response.end());
			nlohmann::json root = nlohmann::json::parse(result);
			if (!root.is_object())
			{
				return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
			}
			unsigned int ret = 1;
			std::string msg;

			if (root.end() != root.find("code"))
			{
				ret = root["code"].get<uint32_t>();
			}

			if (root.end() != root.find("message"))
			{
				msg = root["message"].get<std::string>();
			}

			return RESULT2(ret, msg);
		}

	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
	}
	return res;
}

SDWLResult drvcore_mgr::get_file_attributes(const std::wstring &filePath, DWORD &fileAttributes) {
	SDWLResult res = RESULT(0);
	try {
		nlohmann::json root = nlohmann::json::object();
		root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_FILE_ATTRIBUTES;
		root["parameters"] = nlohmann::json::object();
		nlohmann::json& param = root["parameters"];

		param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(filePath);
		std::string str = root.dump();

		std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
		request_blob->SessionId = _session_id;
		request_blob->ProcessId = GetCurrentProcessId();
		request_blob->ThreadId = GetCurrentThreadId();

		if (0 != str.length()) {
			memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
		}

		std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
		if (!response.empty()) {
			std::string result = std::string(response.begin(), response.end());
			nlohmann::json root = nlohmann::json::parse(result);
			if (!root.is_object())
			{
				return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
			}
			unsigned int ret = 1;
			std::string msg;

			if (root.end() != root.find("code"))
			{
				ret = root["code"].get<uint32_t>();
			}

			if (root.end() != root.find("message"))
			{
				msg = root["message"].get<std::string>();
			}

			if (root.end() != root.find("fileAttributes"))
			{
				fileAttributes = root["fileAttributes"].get<uint32_t>();
			}

			return RESULT2(ret, msg);
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
	}
	return res;
}


SDWLResult drvcore_mgr::set_file_attributes(const std::wstring &filePath, DWORD fileAttributes)
{
	SDWLResult res = RESULT(0);
	try {
		nlohmann::json root = nlohmann::json::object();
		root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_SET_FILE_ATTRIBUTES;
		root["parameters"] = nlohmann::json::object();
		nlohmann::json& param = root["parameters"];

		param[NXSERV_REQUEST_PARAM_FILEPATH] = NX::conv::utf16toutf8(filePath);
		param[NXSERV_REQUEST_PARAM_FILE_ATTRIBUTES] = fileAttributes; // 1: Query all information of the file
		std::string str = root.dump();

		std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
		request_blob->SessionId = _session_id;
		request_blob->ProcessId = GetCurrentProcessId();
		request_blob->ThreadId = GetCurrentThreadId();
		if (0 != str.length()) {
			memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
		}

		std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
		if (!response.empty())
		{
			std::string result = std::string(response.begin(), response.end());
			nlohmann::json root = nlohmann::json::parse(result);
			if (!root.is_object())
			{
				return RESULT2(SDWL_INVALID_JSON_FORMAT, "RPM service return wrong JSON!" + result);
			}
			unsigned int ret = 1;
			std::string msg;

			if (root.end() != root.find("code"))
			{
				ret = root["code"].get<uint32_t>();
			}

			if (root.end() != root.find("message"))
			{
				msg = root["message"].get<std::string>();
			}
			return RESULT2(ret, msg);
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
	}
	return res;
}

SDWLResult drvcore_mgr::set_app_key(const std::wstring& subkey, const std::wstring& name, const std::wstring& data, uint32_t op, const std::string &security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_SET_APP_REGISTRY;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];

        param["security"] = security;
        param["subkey"] = NX::conv::utf16toutf8(subkey);
        param["name"] = NX::conv::utf16toutf8(name);
        param["data"] = NX::conv::utf16toutf8(data);
        param["operation"] = op;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::launch_pdp_process()
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_LAUNCH_PDP;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::is_app_registered(const std::wstring& appPath, bool& registered)
{
    SDWLResult res = RESULT(0);
    registered = false;
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_IS_APP_REGISTERED;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_APPPATH] = NX::conv::utf16toutf8(appPath);
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return handle_is_app_registered_response(result, registered);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::get_protected_profiles_dir(std::wstring& path)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_GET_PROTECTED_PROFILES_DIR;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return handle_get_protected_profiles_dir_response(result, path);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::popup_new_token(const std::wstring& membershipid, std::string &token_id, std::string &token_otp, std::string &token_value)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_POPUP_NEW_TOKEN;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param["membershipid"] = NX::conv::utf16toutf8(membershipid);
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return	handle_popup_new_token_response(result, token_id, token_otp, token_value);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::find_cached_token(const std::wstring& duid, std::string &token_id, std::string &token_otp, std::string &token_value, time_t &token_ttl)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_FIND_CACHED_TOKEN;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param["duid"] = NX::conv::utf16toutf8(duid);

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return handle_find_cached_token_response(result, token_id, token_otp, token_value, token_ttl);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::request_login(const std::wstring &callback_cmd, const std::wstring &callback_cmd_param)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_REQUEST_LOGIN;
        root["parameters"] = nlohmann::json::object();

        nlohmann::json& param = root["parameters"];
        param["command"] = NX::conv::utf16toutf8(callback_cmd);
        param["param"] = NX::conv::utf16toutf8(callback_cmd_param);

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return get_response_status(result);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::request_logout(uint32_t option)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_REQUEST_LOGOUT;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param["option"] = option;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return get_response_status(result);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::notify_message(const std::wstring &app, const std::wstring &target, const std::wstring &message,
	uint32_t msgtype, const std::wstring &operation, uint32_t result, uint32_t fileStatus)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_NOTIFY_MESSAGE;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];

        param["application"] = NX::conv::utf16toutf8(app);
        param["target"] = NX::conv::utf16toutf8(target);
        param["message"] = NX::conv::utf16toutf8(message);
        param["msgtype"] = msgtype;
        param["operation"] = NX::conv::utf16toutf8(operation);
        param["result"] = result;
        param["fileStatus"] = fileStatus;

        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return get_response_status(result);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::read_file_tags(const std::wstring& filepath, std::wstring &tags)
{
    SDWLResult res = RESULT(0);

    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_READ_FILE_TAGS;
        root["parameters"] = nlohmann::json::object();

        std::string strPath = NX::conv::utf16toutf8(filepath);
        nlohmann::json& param = root["parameters"];
        param["path"] = strPath;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty())
        {
            std::string result = std::string(response.begin(), response.end());
            return handle_read_file_tags_response(result, tags);
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

SDWLResult drvcore_mgr::verify_security(const std::string& security)
{
    SDWLResult res = RESULT(0);
    if (NULL == _h)
        return RESULT2(SDWL_NOT_READY, "RPM driver is not ready");

    BOOL bval = is_transporter_enabled(_h);
    try {
        nlohmann::json root = nlohmann::json::object();
        root[NXSERV_REQUEST_PARAM_CODE] = CTL_SERV_VERIFY_SECURITY;
        root["parameters"] = nlohmann::json::object();
        nlohmann::json& param = root["parameters"];
        param[NXSERV_REQUEST_PARAM_SECURITY] = security;
        std::string str = root.dump();

        std::shared_ptr<QUERY_SERVICE_REQUEST> request_blob = std::make_shared<QUERY_SERVICE_REQUEST>();
        request_blob->SessionId = _session_id;
        request_blob->ProcessId = GetCurrentProcessId();
        request_blob->ThreadId = GetCurrentThreadId();
        if (0 != str.length()) {
            memcpy(request_blob->Data, str.c_str(), min(str.length(), 2048));
        }

        std::vector<unsigned char> response = send_request(NXRMDRV_MSG_TYPE_QUERY_SERVICE, request_blob.get(), (unsigned long)sizeof(QUERY_SERVICE_REQUEST));
        if (!response.empty()) {
            std::string result = std::string(response.begin(), response.end());
            res = get_response_status(result);
            return res;
        }
    }
    catch (const std::bad_alloc& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_NOT_ENOUGH_MEMORY, "Exception: not enough memory");
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        return RESULT2(SDWL_INVALID_DATA, "Exception: send data to RPM service");
    }

    return res;
}

std::vector<unsigned char> drvcore_mgr::send_request(unsigned long type, const void* request_data, unsigned long data_size, unsigned long timeout)
{
	CELOG_ENTER;

	if (_t_option == 0)
	{
		// use pipeline
		::EnterCriticalSection(&_lock);
		std::vector<unsigned char> response_data(sizeof(QUERY_SERVICE_REQUEST), 0);
		unsigned long bytes_returned = 0;

		NX::async_pipe::client pipe_client(sizeof(QUERY_SERVICE_REQUEST));

		if (!pipe_client.connect(L"nxrmservice", timeout)) {
			CELOG_LOGA(CELOG_ERROR, "Failed to connect to pipe server\n");
			::LeaveCriticalSection(&_lock);
			CELOG_RETURN_VAL_NOPRINT(response_data);
		}

		PQUERY_SERVICE_REQUEST request_blob = (PQUERY_SERVICE_REQUEST)request_data;
		std::string request((char*)request_blob->Data, ((char*)request_blob->Data + sizeof(QUERY_SERVICE_REQUEST::Data)));

        nlohmann::json root = nlohmann::json::object();
        root["process"] = request_blob->ProcessId;
        root["thread"] = request_blob->ThreadId;
        root["session"] = request_blob->SessionId;
        root["request"] = request.c_str();

        std::string str = root.dump();

		std::vector<unsigned char> data(str.begin(), str.end());
		bool result = pipe_client.write(data, timeout);
		if (!result) {
			CELOG_LOGA(CELOG_ERROR, "Failed to write data to pipe server\n");
			pipe_client.disconnect();
			::LeaveCriticalSection(&_lock);
			CELOG_RETURN_VAL_NOPRINT(response_data);
		}

		result = pipe_client.read(response_data, timeout);
		if (!result) {
			CELOG_LOGA(CELOG_ERROR, "Failed to read data from pipe server\n");
			pipe_client.disconnect();
			::LeaveCriticalSection(&_lock);
			CELOG_RETURN_VAL_NOPRINT(response_data);
		}
		pipe_client.disconnect();
		::LeaveCriticalSection(&_lock);
		CELOG_RETURN_VAL_NOPRINT(response_data);
	}
	else if (_t_option == 1)
	{
		// use the driver shared memory
		std::vector<unsigned char> response_data(4096, 0);
		unsigned long bytes_returned = 0;

		HANDLE request = submit_request(_h, type, (PVOID)request_data, data_size);
		if (NULL == request)
			CELOG_RETURN_VAL_NOPRINT(response_data);

		if (!wait_for_response(request, _h, response_data.data(), (unsigned long)response_data.size(), &bytes_returned))
			CELOG_RETURN_VAL_NOPRINT(response_data);

		if (bytes_returned != 4096)
			response_data.resize(bytes_returned);

		CELOG_RETURN_VAL_NOPRINT(response_data);
	}

	std::vector<unsigned char> response_data(4096, 0);
	CELOG_RETURN_VAL_NOPRINT(response_data);
}

void drvcore_mgr::setTransportOption(unsigned int option)
{
	if (option)
		_t_option = 1; // shared memory
	else
		_t_option = 0; // pipe
}
