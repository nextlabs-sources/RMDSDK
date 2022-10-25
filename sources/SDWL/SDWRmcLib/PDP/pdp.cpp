#include "../stdafx.h"
#include <psapi.h>
#include <ws2tcpip.h>
#include "common/celog2/celog.h"
#include "CEsdk_helper.hpp"
#include "CEPrivate.h"
#include "rmccore/format/nxlrights.h"
#include "pdp.h"
#include "time.h"
#include "nudf/winutil.hpp"

#define CELOG_CUR_MODULE "rmdsdk"
#define CELOG_CUR_FILE CELOG_FILEPATH_SOURCES_SDWL_SDWRMCLIB_PDP_PDP_CPP

#define PC_SERVICE_NAME L"ComplianceEnforcerService"

#define STATE_TRY_TIMES		60
#define STATE_TRY_INTERVAL	500

static const ULONGLONG PDP_STARTUP_TIME_MS = 20 * 1000;

const wchar_t OB_NAME_WATERMARK[]                   = L"OB_OVERLAY";
const wchar_t OB_OPTION_WATERMARK_TEXT[]            = L"Text";
const wchar_t OB_OPTION_WATERMARK_FONT_NAME[]       = L"FontName";
const wchar_t OB_OPTION_WATERMARK_FONT_COLOR[]      = L"TextColor";
const wchar_t OB_OPTION_WATERMARK_FONT_SIZE[]       = L"FontSize";
const wchar_t OB_OPTION_WATERMARK_TRANSPARENCY[]    = L"Transparency";
const wchar_t OB_OPTION_WATERMARK_ROTATION[]        = L"Rotation";

const wchar_t OB_OPTION_WATERMARK_ROTATION_CLOCKWISE[]      = L"Clockwise";
const wchar_t OB_OPTION_WATERMARK_ROTATION_ANTICLOCKWISE[]  = L"Anticlockwise";



using namespace NX;

SDWPDP::~SDWPDP(void)
{
    CELOG_ENTER;
    DisconnectPDPMan();

    CELOG_RETURN;
}

bool SDWPDP::IsPDPManRunning()
{
    CELOG_ENTER;
    CELOG_LOG(CELOG_DEBUG, L"m_pdpDir=%s\n", m_pdpDir.c_str());

    std::vector<DWORD> pidArray;
    DWORD pidArrayMax = 0;
    DWORD cb;
    DWORD cbNeeded;

    // Repeat until EnumProcess() returns fewer PIDs than what we ask for.
    do {
        pidArrayMax += 10;
        pidArray.resize(pidArrayMax);
        cb = static_cast<DWORD>(pidArray.size() * sizeof(DWORD));
        if (!EnumProcesses(pidArray.data(), cb, &cbNeeded)) {
            // Error.  Assume that it is not running.
            CELOG_LOG(CELOG_ERROR, L"EnumProcesses failed, cb=%lu, lastErr=%lu\n",
                      cb, GetLastError());
            CELOG_RETURN_VAL(false);
        }
    } while (cbNeeded == cb);
    pidArray.resize(cbNeeded / sizeof(DWORD));

    // Check all processes and see if any process is running the same
    // cepdpman.exe file that we want to run.
    //
    // We cannot use OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ)
    // and then GetModuleFileNameEx() here.  Otherwise there would be two
    // problems:
    // - If an existing cepdpman.exe process was started as Administrator, and
    //   the current client app process is started as ordinary user,
    //   OpenProcess() on the existing cepdpman.exe process would fail with
    //   ERROR_ACCESS_DENIED.
    // - On an 64-bit machine with an existing cepdpman.exe process (which is
    //   always 64-bit), if the current client app process is 32-bit,
    //   OpenProcess() on the existing cepdpman.exe process would succeed, but
    //   GetModuleFileNameEx() would fail with ERROR_INVALID_HANDLE.
    //
    // Hence, we need to use OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION)
    // and then QueryFullProcessImageName() instead.
    for (auto pid : pidArray) {
        CELOG_LOG(CELOG_DEBUG, L"pid=%lu\n", pid);
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
            pid);
        if (hProcess == NULL) {
            CELOG_LOG(CELOG_WARNING, L"OpenProcess failed, lastErr=%lu\n", GetLastError());
            continue;
        }

        WCHAR nameBuf[MAX_PATH];
        DWORD dwSize _countof(nameBuf);
        DWORD ret;

        ret = QueryFullProcessImageName(hProcess, 0, nameBuf, &dwSize);
        CloseHandle(hProcess);

        if (ret == 0) {
            CELOG_LOG(CELOG_WARNING, L"QueryFullProcessImageName failed, lastErr=%lu\n", GetLastError());
            continue;
        }
		
        CELOG_LOG(CELOG_DEBUG, L"QueryFullProcessImageName returns %s\n", nameBuf);
        if (icompare(std::wstring(nameBuf), m_pdpDir + L"\\bin\\cepdpman.exe") == 0) {
			if (m_pdpProcessHandle == NULL) {
				m_pdpProcessHandle = hProcess;
				m_pdpProcessId = pid;
			}
			else if (m_pdpProcessId != pid) {
				m_pdpProcessHandle = hProcess;
				m_pdpProcessId = pid;
				DisconnectPDPMan();
			}
            // Found.
            CELOG_RETURN_VAL(true);
        }
    }

    // Not found.
    CELOG_RETURN_VAL(false);
}

SDWLResult SDWPDP::StartPDPMan(void)
{
    CELOG_ENTER;

    // Get a handle to the SCM database.
    SC_HANDLE schSCManager = OpenSCManager(
        NULL,                   // local computer
        NULL,                   // ServicesActive database
        SC_MANAGER_CONNECT);    // full access rights

    if (NULL == schSCManager)
    {
        CELOG_LOG(CELOG_ERROR, L"OpenSCManager failed error code %lu\n", GetLastError());
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "OpenSCManager failed"));
    }

    // Get a handle to the service.
    SC_HANDLE schService = OpenService(
        schSCManager,       // SCM database
        PC_SERVICE_NAME,    // name of service
        SERVICE_START);     // full access

    if (schService == NULL)
    {
        CELOG_LOG(CELOG_ERROR, L"OpenService failed error code %lu\n", GetLastError());
        CloseServiceHandle(schSCManager);
        CELOG_RETURN_VAL_T(RESULT2(SDWL_INTERNAL_ERROR, "OpenService failed"));
    }

    // Attempt to start the service.
    if (!StartService(
        schService,  // handle to service
        0,           // number of arguments
        NULL) )      // no arguments
    {
        DWORD lastErr = GetLastError();

        //We found a problem, sometime, the API "StopPDPMan" returns, but the "service" is still running.
        //So, we need to check the Error, try to start PC again if the error is ERROR_SERVICE_ALREADY_RUNNING
        //Try about 50 seconds.
        if (lastErr == ERROR_SERVICE_ALREADY_RUNNING)
        {
            CELOG_LOG(CELOG_WARNING, L"StartService failed error code is \"already running\" \n");

            for( int i = 0; i < 100; i++)
            {
                if (StartService(
                    schService,  // handle to service
                    0,           // number of arguments
                    NULL) )      // no arguments
                {
                    CloseServiceHandle(schService);
                    CloseServiceHandle(schSCManager);
                    CELOG_LOG(CELOG_DEBUG, L"StartService succeed\n");
                    CELOG_RETURN_VAL_T(RESULT(0));
                }
                else
                {
                    if(GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
                    {
                        CELOG_LOG(CELOG_ERROR, L"StartService failed error code is %lu\n", lastErr);
                        break;
                    }
                }
                Sleep(500);
            }
        }
        else
        {
            CELOG_LOG(CELOG_ERROR, L"StartService failed error code is %lu\n", lastErr);
        }

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);

        CELOG_RETURN_VAL_T(RESULT(SDWL_ACCESS_DENIED));
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    CELOG_LOG(CELOG_DEBUG, L"StartService succeed\n");
    CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult SDWPDP::ConnectPDPMan(void)
{
    CELOG_ENTER;

    CEApplication app;
    CEUser user;
    const CEint32 timeoutMs = 500;
    CEResult_t ceRes;

    app.appName = CEM_AllocateString(L"");
    app.appPath = CEM_AllocateString(L"");
    app.appURL = CEM_AllocateString(L"");
    user.userName = CEM_AllocateString(L"");
    user.userID = CEM_AllocateString(L"");

    std::unique_lock<std::shared_mutex> lck(m_connHandleMtx);

    ceRes = CECONN_Initialize(app, user, NULL, &m_connHandle, timeoutMs);

    CEM_FreeString(user.userID);
    CEM_FreeString(user.userName);
    CEM_FreeString(app.appURL);
    CEM_FreeString(app.appPath);
    CEM_FreeString(app.appName);

    if (ceRes != CE_RESULT_SUCCESS) {
        CELOG_LOG(CELOG_ERROR, L"Cannot connect to PDPMan, ceRes=%d\n", ceRes);
        m_connHandle = NULL;
        CELOG_RETURN_VAL_T(RESULT2(ceRes, "Cannot connect to PDPMan"));
    }

    CELOG_LOG(CELOG_DEBUG, L"m_connHandle=0x%p\n", m_connHandle);
    CELOG_RETURN_VAL_T(RESULT(CE_RESULT_SUCCESS));
}

SDWLResult SDWPDP::ConnectPDPManNoLock(void)
{
    CELOG_ENTER;

    CEApplication app;
    CEUser user;
    const CEint32 timeoutMs = 500;
    CEResult_t ceRes;

    app.appName = CEM_AllocateString(L"");
    app.appPath = CEM_AllocateString(L"");
    app.appURL = CEM_AllocateString(L"");
    user.userName = CEM_AllocateString(L"");
    user.userID = CEM_AllocateString(L"");

    ceRes = CECONN_Initialize(app, user, NULL, &m_connHandle, timeoutMs);

    CEM_FreeString(user.userID);
    CEM_FreeString(user.userName);
    CEM_FreeString(app.appURL);
    CEM_FreeString(app.appPath);
    CEM_FreeString(app.appName);

    if (ceRes != CE_RESULT_SUCCESS) {
        CELOG_LOG(CELOG_ERROR, L"Cannot connect to PDPMan, ceRes=%d\n", ceRes);
        m_connHandle = NULL;
        CELOG_RETURN_VAL_T(RESULT2(ceRes, "Cannot connect to PDPMan"));
    }

    CELOG_LOG(CELOG_DEBUG, L"m_connHandle=0x%p\n", m_connHandle);
    CELOG_RETURN_VAL_T(RESULT(CE_RESULT_SUCCESS));
}

SDWLResult SDWPDP::DisconnectPDPMan(void)
{
    CELOG_ENTER;

    std::unique_lock<std::shared_mutex> lck(m_connHandleMtx);

    if (m_connHandle == NULL) {
        CELOG_RETURN_VAL_T(RESULT(0));
    }

    const CEint32 timeoutMs = 500;
    CEResult_t ceRes;

    CELOG_LOG(CELOG_DEBUG, L"About to close handle 0x%p\n", m_connHandle);
    ceRes = CECONN_Close(m_connHandle, timeoutMs);
    if (ceRes != CE_RESULT_SUCCESS) {
        CELOG_LOG(CELOG_ERROR, L"Cannot close PDPMan connection, ceRes=%d\n", ceRes);
        CELOG_RETURN_VAL_T(RESULT2(ceRes, "Cannot close PDPMan connection"));
    }

    m_connHandle = NULL;
    CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult SDWPDP::DisconnectPDPManNoLock(void)
{
    CELOG_ENTER;

    if (m_connHandle == NULL) {
        CELOG_RETURN_VAL_T(RESULT(0));
    }

    const CEint32 timeoutMs = 500;
    CEResult_t ceRes;

    CELOG_LOG(CELOG_DEBUG, L"About to close handle 0x%p\n", m_connHandle);
    ceRes = CECONN_Close(m_connHandle, timeoutMs);
    if (ceRes != CE_RESULT_SUCCESS) {
        CELOG_LOG(CELOG_ERROR, L"Cannot close PDPMan connection, ceRes=%d\n", ceRes);
        CELOG_RETURN_VAL_T(RESULT2(ceRes, "Cannot close PDPMan connection"));
    }

    m_connHandle = NULL;
    CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult SDWPDP::StopPDPMan(void)
{
    CELOG_ENTER;

    // Return error if we cannot connect to PDP successfully.
    if (m_connHandle == NULL) {
        if (ConnectPDPMan() != CE_RESULT_SUCCESS) {
            CELOG_RETURN_VAL_T(RESULT2(SDWL_BUSY, "SDKLib cannot establish connection with PDP."));
        }
    }

    CEResult_t ceRes;
    const CEint32 timeoutMs = 10 * 1000;

    CEString password = CEM_AllocateString(L"x");
    ceRes = CEP_StopPDPWithoutPassword(m_connHandle, password, timeoutMs);
    CEM_FreeString(password);

    if (ceRes != CE_RESULT_SUCCESS) {
        CELOG_LOG(CELOG_ERROR, L"Cannot stop PDPMan, error=%d\n", ceRes);
        CELOG_RETURN_VAL_T(RESULT2(SDWL_NOT_READY, "Cannot stop PDPMan"));
    }

    m_connHandle = NULL;
    m_pdpProcessHandle = NULL;
    CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult SDWPDP::Initialize(const std::wstring &pdpDir)
{
    CELOG_ENTER;
    CELOG_LOG(CELOG_INFO, L"pdpDir=%s\n", pdpDir.c_str());

    SDWLResult res;

    // Convert pdpDir to long path, just in case it is a short path.  This
    // makes it easier when we need to compare it with process image paths
    // later, since those paths are long paths.
    WCHAR pdpDirLongPathBuf[MAX_PATH];
    if (GetLongPathName(pdpDir.c_str(), pdpDirLongPathBuf,
                        _countof(pdpDirLongPathBuf)) == 0) {
        // Error.  Just use the passed path.
        CELOG_LOG(CELOG_ERROR, L"GetLongPathName(\"%s\") failed, lastErr=%lu\n",
                  pdpDir.c_str(), GetLastError());
        m_pdpDir = pdpDir;
    } else {
        m_pdpDir = pdpDirLongPathBuf;
    }

    // Do not start PDPMan ourselves, since PDPMan now runs as a Windows
    // Service which starts and re-starts automatically.

    // Do not try to connect to PDPMan here, because PDPMan needs time to
    // finish starting up.

    CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult SDWPDP::ensurePDPConnectionReady(const std::wstring &pdpDir, bool &ready) {
	CELOG_ENTER;

	if (!IsPDPManRunning()) {
		DisconnectPDPMan();
		Initialize(pdpDir);
		for (int i = 0; i < STATE_TRY_TIMES; i++) {
			Sleep(STATE_TRY_INTERVAL);
			if (IsPDPManRunning()) {
				Sleep(STATE_TRY_INTERVAL);
				break;
			}
		}
	}

	{
		std::shared_lock<std::shared_mutex> lck(m_connHandleMtx);
		if (m_connHandle != NULL) {
			ready = true;
			CELOG_RETURN_VAL_T(RESULT(0));
		}
	}

	SDWLResult res;
	for (int i = 0; i < STATE_TRY_TIMES; i++) {
		res = ConnectPDPMan();
		if (res) {
			break;
		}
		Sleep(STATE_TRY_INTERVAL);
	}
	if (res) {
		ready = true;
		CELOG_RETURN_VAL_T(RESULT(0));
	}
	else if (res.GetCode() == CE_RESULT_CONN_FAILED) {
		ready = false;
		CELOG_RETURN_VAL_T(RESULT(0));
	}
	else {
		CELOG_RETURN_VAL_T(res);
	}
}

SDWLResult SDWPDP::IsReadyForEval(bool &ready)
{
    CELOG_ENTER;

    {
        std::shared_lock<std::shared_mutex> lck(m_connHandleMtx);
        if (m_connHandle != NULL) {
            ready = true;
            CELOG_RETURN_VAL_T(RESULT(0));
        }
    }

    SDWLResult res = ConnectPDPMan();
    if (res) {
        ready = true;
        CELOG_RETURN_VAL_T(RESULT(0));
    } else if (res.GetCode() == CE_RESULT_CONN_FAILED) {
        ready = false;
        CELOG_RETURN_VAL_T(RESULT(0));
    } else {
        CELOG_RETURN_VAL_T(res);
    }
}

// Adapted from CESDK/client/stub/CONN/src/conn.cpp.
//
//Get local host IP address in network byte order
bool GetHostIPAddress(int &ip)
{
    char hostName[1024];
    struct addrinfo hints, *res;
    struct in_addr hostIPAddr;
    int err;

    if (gethostname(hostName, 1024) != 0) {
        CELOG_LOG(CELOG_ERROR, L"Failed to get host name: error=%d\n", errno);
        return false;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    if ((err = getaddrinfo(hostName, NULL, &hints, &res)) != 0) {
        CELOG_LOG(CELOG_ERROR, L"Failed to get host ip: error=%d\n", err);
        return false;
    }

    hostIPAddr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
    ip = (int)(hostIPAddr.s_addr);
    CELOG_LOG(CELOG_DEBUG, L"host IP address (network byte order): 0x%08X\n", ip);

    freeaddrinfo(res);
    return true;  
}

SDRmFileRight RightStrToSDRmFileRight(const std::wstring &rightStr)
{
    static_assert(BUILTIN_RIGHT_VIEW        == RIGHT_VIEW           &&
                  BUILTIN_RIGHT_EDIT        == RIGHT_EDIT           &&
                  BUILTIN_RIGHT_PRINT       == RIGHT_PRINT          &&
                  BUILTIN_RIGHT_CLIPBOARD   == RIGHT_CLIPBOARD      &&
                  BUILTIN_RIGHT_SAVEAS      == RIGHT_SAVEAS         &&
                  BUILTIN_RIGHT_DECRYPT     == RIGHT_DECRYPT        &&
                  BUILTIN_RIGHT_SCREENCAP   == RIGHT_SCREENCAPTURE  &&
                  BUILTIN_RIGHT_SEND        == RIGHT_SEND           &&
                  BUILTIN_RIGHT_CLASSIFY    == RIGHT_CLASSIFY       &&
                  BUILTIN_RIGHT_SHARE       == RIGHT_SHARE          &&
                  BUILTIN_RIGHT_DOWNLOAD    == RIGHT_DOWNLOAD,
                  "Constants don't match.");
    return (SDRmFileRight) ActionToRights(NX::conv::to_string(rightStr).c_str());
}

SDRmFileRight CERightStrToSDRmFileRight(const std::wstring &ceRightStr)
{
    const std::wstring prefix(L"RIGHT_");

    //assert(ceRightStr.compare(0, prefix.size(), prefix) == 0);
	if (ceRightStr.compare(0, prefix.size(), prefix) != 0)
	{
		return (SDRmFileRight)0ULL;
	}
    return RightStrToSDRmFileRight(ceRightStr.substr(prefix.size()));
}

std::vector<SDR_WATERMARK_INFO> SDWPDP::GetWatermarkObsFromEvalResult(
    CEAttributes& obligations, const std::wstring& useremail,
    const std::wstring& userID, const std::vector<std::pair<std::wstring, std::wstring>>& rAttrs, const std::vector<std::pair<std::wstring, std::wstring>>& uAttrs, bool doOwnerCheck)
{
    std::vector<SDR_WATERMARK_INFO> v;

    CESDK::Obligations obs;
    obs.Assign(&CEM_GetString, obligations);

    for (const auto& ob : obs) {
        if (ob.name == OB_NAME_WATERMARK) {
            std::wstring opVal;
            SDR_WATERMARK_INFO wi;

            if (ob.GetObligationValue(OB_OPTION_WATERMARK_TEXT, opVal)) {

                if (!doOwnerCheck) // not tranfer
                {
                    wi.text = NX::conv::to_string(opVal);
                }
                else
                {
                    static std::string g_hostName;
                    static std::string g_hostIP;
                    static std::string g_hostFQDN;
                    std::string wmtext = NX::conv::to_string(opVal);

#pragma region convert the watermark text with pre-defined macro
                    int begin = -1;
                    int end = -1;
                    std::string converted_wmtext;
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
                                std::string s1 = wmtext.substr(begin, (end - begin + 1));
                                std::string s2 = wmtext.substr(begin, (end - begin + 1));
                                std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
                                bool has_attr = false;

                                //// $(fso.classification)
                                for (size_t i = 0; i < rAttrs.size(); i++) {
                                    std::string a_s1 = "$(" + NX::conv::utf16toutf8(rAttrs[i].first) + ")";
                                    std::transform(a_s1.begin(), a_s1.end(), a_s1.begin(), ::tolower);
                                    if (a_s1 == s2)
                                    {
                                       /* std::string v_s1 = NX::conv::utf16toutf8(rAttrs[i].second);
                                        converted_wmtext.append(v_s1);
                                        converted_wmtext.append(" ");
                                        has_attr = true;*/
                                        std::string v_s1 = NX::conv::utf16toutf8(rAttrs[i].second);
                                        converted_wmtext.append(v_s1);
                                        std::string first = NX::conv::utf16toutf8(rAttrs[i].first);
                                        if ((i + 1) != rAttrs.size())
                                        {
                                            std::string second = NX::conv::utf16toutf8(rAttrs[i + 1].first);
                                            if (strcmp(first.c_str(), second.c_str()) == 0)
                                            {
                                                converted_wmtext.append(",");
                                            }
                                        }
                                        converted_wmtext.append(" ");
                                        has_attr = true;
                                    }
                                }

                                //// $(user.attribute)
                                for (size_t i = 0; i < uAttrs.size(); i++) {
                                    std::string a_s1 = "$(" + NX::conv::utf16toutf8(uAttrs[i].first) + ")";
                                    std::transform(a_s1.begin(), a_s1.end(), a_s1.begin(), ::tolower);
                                    if (a_s1 == s2)
                                    {
                                        std::string v_s1 = NX::conv::utf16toutf8(uAttrs[i].second);
                                        converted_wmtext.append(v_s1);
                                        converted_wmtext.append(" ");
                                        has_attr = true;
                                    }
                                }

                                if (s2 == "$(user)")
                                {
                                    converted_wmtext.append(NX::conv::utf16toutf8(userID)); // userDispName
                                    converted_wmtext.append(" ");
                                    has_attr = true;
                                }
                                else if (s2 == "$(classification)")
                                {
                                    std::string classifications;
                                    for (size_t i = 0; i < rAttrs.size(); i++) {
                                        if (i > 0)
                                            classifications += ";";

                                        classifications += NX::conv::utf16toutf8(rAttrs[i].first) + "=" + NX::conv::utf16toutf8(rAttrs[i].second);
                                    }

                                    converted_wmtext.append(classifications);
                                    has_attr = true;
                                }
                                else if (s2 == "$(host)")
                                {
                                    if (g_hostName.size() <= 0)
                                    {
                                        NX::win::host hinfo;

                                        g_hostName = NX::conv::utf16toutf8(hinfo.dns_host_name());
                                        g_hostFQDN = NX::conv::utf16toutf8(hinfo.fqdn_name());
                                        g_hostIP = NX::conv::utf16toutf8(hinfo.ip_address());
                                    }

                                    converted_wmtext.append(g_hostFQDN);
                                    has_attr = true;
                                }
                                else if (s2 == "$(ip)")
                                {
                                    if (g_hostName.size() <= 0)
                                    {
                                        NX::win::host hinfo;

                                        g_hostName = NX::conv::utf16toutf8(hinfo.dns_host_name());
                                        g_hostFQDN = NX::conv::utf16toutf8(hinfo.fqdn_name());
                                        g_hostIP = NX::conv::utf16toutf8(hinfo.ip_address());
                                    }

                                    converted_wmtext.append(g_hostIP);
                                    has_attr = true;
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
                                    has_attr = true;
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
                                    has_attr = true;
                                }
                                else if (s2 == "$(break)")
                                {
                                    converted_wmtext.append("\n");
                                    has_attr = true;
                                }

                                if (has_attr == false)
                                {
                                    //
                                    // non-supported classification, append back
                                    //

                                    //
                                    // We commented following line, which means if the $(VARIABLE) is not found in our pre-defined set and not found in document/user attributes, they will be ignored
                                    // change above definition: we will let external caller to handle if it is not in our pre-defined macro words
                                    //
                                    if (s2.length() > 3 && s2[0] == '$' && s2[1] == '(')
                                    {
                                        converted_wmtext.append(" ");
                                    }
                                    else
                                    {
                                        converted_wmtext.append(s1);
                                    }
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
                            if (begin >= 0)
                            {
                                // in processing $xxxx
                            }
                            else
                            {
                                converted_wmtext.push_back(wmtext.data()[i]);
                            }

                        }
                    }

                    if (begin >= 0 && end == -1)
                    {
                        // never match $xxxx
                        //std::string s2 = wmtext.substr(begin);
                        //converted_wmtext.append(s2);
                    }
#pragma endregion


                    wi.text = NX::conv::to_string(converted_wmtext);
                }
              
            }
            if (ob.GetObligationValue(OB_OPTION_WATERMARK_FONT_NAME, opVal)) {
                wi.fontName = NX::conv::to_string(opVal);
            }
            if (ob.GetObligationValue(OB_OPTION_WATERMARK_FONT_COLOR, opVal)) {
                wi.fontColor = NX::conv::to_string(opVal);
            }

            if (ob.GetObligationValue(OB_OPTION_WATERMARK_FONT_SIZE, opVal)) {
                try {
                    wi.fontSize = std::stoi(opVal);
                } catch (...) {
                    wi.fontSize = 0;
                }
            } else {
                wi.fontSize = 0;
            }

            if (ob.GetObligationValue(OB_OPTION_WATERMARK_TRANSPARENCY, opVal)) {
                try {
                    wi.transparency = std::stoi(opVal);
                } catch (...) {
                    wi.transparency = 0;
                }
            } else {
                wi.transparency = 0;
            }

            wi.rotation = NOROTATION;
            if (ob.GetObligationValue(OB_OPTION_WATERMARK_ROTATION, opVal)) {
                if (opVal == OB_OPTION_WATERMARK_ROTATION_CLOCKWISE) {
                    wi.rotation = CLOCKWISE;
                } else if (opVal == OB_OPTION_WATERMARK_ROTATION_ANTICLOCKWISE) {
                    wi.rotation = ANTICLOCKWISE;
                }
            }

            wi.repeat = false;

            v.push_back(wi);
        }
    }

    return v;
}

std::vector<SDR_OBLIGATION_INFO> SDWPDP::GetGenericObsFromEvalResult(
    CEAttributes& obligations)
{
    std::vector<SDR_OBLIGATION_INFO> v;

    CESDK::Obligations obs;
    obs.Assign(&CEM_GetString, obligations);

    for (const auto& ob : obs) {
        SDR_OBLIGATION_INFO oi;
        oi.name = ob.name;

        for (const auto& option : ob.options) {
            oi.options.push_back(option);
        }

        v.push_back(oi);
    }

    return v;
}

SDWLResult SDWPDP::EvalRights(const std::wstring &userDispName,
	const std::wstring &useremail,
    const std::wstring &userID,
    const std::wstring &appPath,
    const std::wstring &_resourceName,
    const std::wstring &resourceType,
    const std::vector<std::pair<std::wstring, std::wstring>> &rAttrs,
    const std::vector<std::pair<std::wstring, std::wstring>> &uAttrs,
    const std::wstring &bundle,
    std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> *rightsAndWatermarks,
    std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> *rightsAndObligations, bool doOwnerCheck)
{
    CELOG_ENTER;
	NX::fs::dos_filepath _resourceName_path(_resourceName);
	std::wstring resourceName = _resourceName_path.file_name().full_name();

    // Return View right immediately, instead of requesting PDPMan to
    // evaluate, if the app is the PDPMan itself.  This is to avoid recursion.
    if (icompare(appPath, m_pdpDir + L"\\bin\\cepdpman.exe") == 0) {
        CELOG_LOG(CELOG_DEBUG, L"Returning RIGHT_VIEW for cepdpman.exe on resource %s\n", resourceName.c_str());

        if (rightsAndWatermarks != nullptr) {
            const std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> rw =
                std::make_pair(RIGHT_VIEW, std::vector<SDR_WATERMARK_INFO>());
            rightsAndWatermarks->push_back(rw);
        } else {
            const std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>> ro =
                std::make_pair(RIGHT_VIEW, std::vector<SDR_OBLIGATION_INFO>());
            rightsAndObligations->push_back(ro);
        }

        CELOG_RETURN_VAL_T(RESULT(0));
    }

    // Return error if we have not connected to PDP successfully yet.
    if (m_connHandle == NULL) {
		if (ConnectPDPMan() != CE_RESULT_SUCCESS)
		{
			CELOG_RETURN_VAL_T(RESULT2(SDWL_BUSY, "SDKLib instance has not established connection with PDP yet."));
		}
    }

    CEResult_t ceRes;
    CEResource *srcRes;

    // We need two extra slots for these additional attributes of our own:
    // - "ce::nocache" = "yes"
    // - "ce::filesystemcheck" = "no"
    CEAttribute *srcAttrArray = new CEAttribute[rAttrs.size() + 2];
    CEAttributes srcAttrs = {srcAttrArray, (CEint32) (rAttrs.size() + 2)};

    CEAttribute *userAttrArray = new CEAttribute[uAttrs.size()];
    CEAttributes userAttrs = { userAttrArray, (CEint32)uAttrs.size() };
    CEUser user;
    CEApplication app;
    CEPermissionsRequest request;
    CEString pql;
    CEint32 ipAddr;
    CEPermissionsEnforcement_t enforcement;
    const CEint32 timeoutMs = 10 * 1000;

    //
    // Allocate the arguments for evaluation.
    //
    srcRes = CEM_CreateResource(resourceName.c_str(), resourceType.c_str());

    for (size_t i = 0; i < rAttrs.size(); i++) {
        srcAttrArray[i].key = CEM_AllocateString(rAttrs[i].first.c_str());
		if (rAttrs[i].second == L"")
			srcAttrArray[i].value = CEM_AllocateString(L"[NULL]");
		else
			srcAttrArray[i].value = CEM_AllocateString(rAttrs[i].second.c_str());
    }

    // Currently there is the following bug in PDPMan: After evaluating with
    // Policy X, if the same file and same user are evaluated again with Policy
    // Y, and the only difference between Policy X and Policy Y is the "FOR
    // xxx" part, the result of the second evaluation will agree with Policy X
    // instead of Policy Y.  (Please see Bug 51160 for details.)
    //
    // To work around this problem, we always pass the attribute "ce::nocache"
    // = "yes" in the evaluation request.  This workaround can be removed once
    // the problem in PDPMan is fixed.
    srcAttrArray[rAttrs.size()].key = CEM_AllocateString(L"ce::nocache");
    srcAttrArray[rAttrs.size()].value = CEM_AllocateString(L"yes");

    // Since SkyDRM doesn't suppot policies specifying attributes that PDP
    // needs to fetch from the file (e.g. owner, last modification time), we
    // add "ce::filesystemcheck" = "no" to tell PDP not to fetch anything from
    // the file.  This way PDP won't access the file at all.  This is an
    // optimization.
    srcAttrArray[rAttrs.size() + 1].key = CEM_AllocateString(L"ce::filesystemcheck");
    srcAttrArray[rAttrs.size() + 1].value = CEM_AllocateString(L"no");

    for (size_t i = 0; i < uAttrs.size(); i++) {
        userAttrArray[i].key = CEM_AllocateString(uAttrs[i].first.c_str());
		if (uAttrs[i].second == L"")
			userAttrArray[i].value = CEM_AllocateString(L"[NULL]");
		else
			userAttrArray[i].value = CEM_AllocateString(uAttrs[i].second.c_str());
    }

    user.userName = CEM_AllocateString(useremail.c_str());
    user.userID = CEM_AllocateString(userID.c_str());
    app.appName = CEM_AllocateString(L"");
    app.appPath = CEM_AllocateString(appPath.c_str());
    app.appURL = CEM_AllocateString(L"");

    request.source = srcRes;
    request.sourceAttributes = &srcAttrs;
    request.target = NULL;
    request.targetAttributes = NULL;
    request.user = &user;
    request.userAttributes = &userAttrs;
    request.app = &app;
    request.appAttributes = NULL;
    request.recipients = NULL;
    request.numRecipients = 0;
    request.additionalAttributes = NULL;
    request.numAdditionalAttributes = 0;
    request.performObligation = CETrue;
    request.noiseLevel = CE_NOISE_LEVEL_USER_ACTION;

    pql = CEM_AllocateString(bundle.c_str());

    if (GetHostIPAddress(ipAddr)) {
        ipAddr = ntohl(ipAddr);
    } else {
        // Use loopback address
        const struct in_addr inAddr = {127, 0, 0, 1};
        ipAddr = ntohl(inAddr.S_un.S_addr);
    }

    //
    // Evaluate.
    //
    m_connHandleMtx.lock_shared();

    CELOG_LOG(CELOG_DEBUG, L"CEEVALUATE_CheckPermissions: m_connHandle: 0x%p\n", m_connHandle);
    ceRes = CEEVALUATE_CheckPermissions(m_connHandle, &request,
        1, pql, CEFalse, ipAddr, &enforcement, timeoutMs);
	CELOG_LOG(CELOG_DEBUG, L"CEEVALUATE_CheckPermissions: ceRes: %d\n", ceRes);

    if (ceRes != CE_RESULT_THREAD_NOT_INITIALIZED)
    {
        m_connHandleMtx.unlock_shared();
    }
    else
    {
        m_needToReconnect = true;
        m_connHandleMtx.unlock_shared();

        bool tryEvalAgain = true;

        m_connHandleMtx.lock();
        if (m_needToReconnect)
        {
            m_needToReconnect = false;
            DisconnectPDPManNoLock();   // make sure previous connection closed
            if (ConnectPDPManNoLock() != CE_RESULT_SUCCESS)
            {
                CELOG_LOG(CELOG_DEBUG, L"Reconnect failed\n");
                tryEvalAgain = false;
            }
        }
        m_connHandleMtx.unlock();

        if (tryEvalAgain)
        {// try again
            m_connHandleMtx.lock_shared();
            CELOG_LOG(CELOG_DEBUG, L"CEEVALUATE_CheckPermissions: reconnect try again, m_connHandle: 0x%p\n", m_connHandle);
            ceRes = CEEVALUATE_CheckPermissions(m_connHandle, &request,
                1, pql, CEFalse, ipAddr, &enforcement, timeoutMs);
            CELOG_LOG(CELOG_DEBUG, L"CEEVALUATE_CheckPermissions: reconnect try again, ceRes: %d\n", ceRes);
            m_connHandleMtx.unlock_shared();
        }
    }

    //
    // Free the arguments for evaluation.
    //
    CEM_FreeString(pql);

    CEM_FreeString(app.appURL);
    CEM_FreeString(app.appPath);
    CEM_FreeString(app.appName);
    CEM_FreeString(user.userID);
    CEM_FreeString(user.userName);

    for (size_t i = 0; i < uAttrs.size(); i++) {
        CEM_FreeString(userAttrArray[i].value);
        CEM_FreeString(userAttrArray[i].key);
    }

    // Free the "ce::filesystemcheck" = "no" attribute that is an optimization.
    CEM_FreeString(srcAttrArray[rAttrs.size() + 1].value);
    CEM_FreeString(srcAttrArray[rAttrs.size() + 1].key);

    // Free the "ce::nocache" = "yes" attribute that is a workaround for the
    // bug in PDPMan.
    CEM_FreeString(srcAttrArray[rAttrs.size()].value);
    CEM_FreeString(srcAttrArray[rAttrs.size()].key);

    for (size_t i = 0; i < rAttrs.size(); i++) {
        CEM_FreeString(srcAttrArray[i].value);
        CEM_FreeString(srcAttrArray[i].key);
    }

    CEM_FreeResource(srcRes);

    delete[] userAttrArray;
    delete[] srcAttrArray;

    if (ceRes != CE_RESULT_SUCCESS) {
        CELOG_LOG(CELOG_ERROR, L"CheckPermissions failed, ceRes=%d\n", ceRes);
        CELOG_RETURN_VAL_T(RESULT(ceRes));
    }

    //
    // Convert each allowed right to SDRmFileRight, and attach any associated
    // watermark(s) or generic obligation(s) with it.
    //
    // Only CEAllow means the right is granted, while both CEDeny and
    // CEDontCare mean the right is not granted.
    //
	BOOL bcheckWaterMark = false;
    if (rightsAndWatermarks != nullptr) {
        rightsAndWatermarks->clear();

        for (int i = 0; i < enforcement.count; i++) {
            if (enforcement.permissions[i].result == CEAllow) {
                std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> rw;
                rw.first = CERightStrToSDRmFileRight(CEM_GetString(enforcement.permissions[i].operation));
				if (rw.first == 0ULL) {
					continue;
				}
                if (enforcement.permissions[i].obligation != NULL) {
                    rw.second = GetWatermarkObsFromEvalResult(*enforcement.permissions[i].obligation, useremail,userID, rAttrs,uAttrs,doOwnerCheck);
					if (rw.second.size() > 0)
						bcheckWaterMark = true;
                }

                rightsAndWatermarks->push_back(rw);
            }
        }
    } else {
        rightsAndObligations->clear();

        for (int i = 0; i < enforcement.count; i++) {
            if (enforcement.permissions[i].result == CEAllow) {
                std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>> ro;
                ro.first = CERightStrToSDRmFileRight(CEM_GetString(enforcement.permissions[i].operation));
				if (ro.first == 0ULL) {
					continue;
				}
                if (enforcement.permissions[i].obligation != NULL) {
                    ro.second = GetGenericObsFromEvalResult(*enforcement.permissions[i].obligation);
                }

                rightsAndObligations->push_back(ro);
            }
        }
    }

    //
    // Free the results from evaluation.
    //
    CEEVALUATE_FreePermissionsEnforcement(enforcement);

	if (bcheckWaterMark && doOwnerCheck)
		ParsingWaterMark(userDispName, useremail, rAttrs, uAttrs, *rightsAndWatermarks);

    CELOG_RETURN_VAL_T(RESULT(0));
}

SDWLResult SDWPDP::ParsingWaterMark(const std::wstring &userDispName, const std::wstring &userID,
	const std::vector<std::pair<std::wstring, std::wstring>> &rAttrs, const std::vector<std::pair<std::wstring, std::wstring>> &uAttrs,
	std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks)
{
	// originally, the following parms are static.
	// however, considering that user might change his IP (example, swith nextwork), we make it as local variable, which can be changed for each call
	static std::string g_hostName;
	static std::string g_hostIP;
	static std::string g_hostFQDN;

	for (size_t i = 0; i < rightsAndWatermarks.size(); i++) {
		std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>> &rw = rightsAndWatermarks[i];
		for (auto& ob : rw.second) {

			std::string wmtext = ob.text;
			if (wmtext.size() <= 0)
				continue;

            #pragma region convert the watermark text with pre-defined macro
            int begin = -1;
            int end = -1;
            std::string converted_wmtext;
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
                        std::string s1 = wmtext.substr(begin, (end - begin + 1));
                        std::string s2 = wmtext.substr(begin, (end - begin + 1));
                        std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
                        bool has_attr = false;

                        //// $(fso.classification)
                        for (size_t i = 0; i < rAttrs.size(); i++) {
                            std::string a_s1 = "$(" + NX::conv::utf16toutf8(rAttrs[i].first) + ")";
                            std::transform(a_s1.begin(), a_s1.end(), a_s1.begin(), ::tolower);
                            if (a_s1 == s2)
                            {
                               /* std::string v_s1 = NX::conv::utf16toutf8(rAttrs[i].second);
                                converted_wmtext.append(v_s1);
                                converted_wmtext.append(" ");
                                has_attr = true;*/

                                std::string v_s1 = NX::conv::utf16toutf8(rAttrs[i].second);
                                converted_wmtext.append(v_s1);
                                std::string first = NX::conv::utf16toutf8(rAttrs[i].first);
                                if ((i + 1) != rAttrs.size())
                                {
                                    std::string second = NX::conv::utf16toutf8(rAttrs[i + 1].first);
                                    if (strcmp(first.c_str(), second.c_str()) == 0)
                                    {
                                        converted_wmtext.append(",");
                                    }
                                }
                                converted_wmtext.append(" ");
                                has_attr = true;
                            }
                        }

                        //// $(user.attribute)
                        for (size_t i = 0; i < uAttrs.size(); i++) {
                            std::string a_s1 = "$(" + NX::conv::utf16toutf8(uAttrs[i].first) + ")";
                            std::transform(a_s1.begin(), a_s1.end(), a_s1.begin(), ::tolower);
                            if (a_s1 == s2)
                            {
                                std::string v_s1 = NX::conv::utf16toutf8(uAttrs[i].second);
                                converted_wmtext.append(v_s1);
                                converted_wmtext.append(" ");
                                has_attr = true;
                            }
                        }

                        if (s2 == "$(user)")
                        {
                            converted_wmtext.append(NX::conv::utf16toutf8(userID)); // userDispName
                            converted_wmtext.append(" ");
                            has_attr = true;
                        }
                        else if (s2 == "$(classification)")
                        {
                            std::string classifications;
                            for (size_t i = 0; i < rAttrs.size(); i++) {
                                if (i > 0)
                                    classifications += ";";

                                classifications += NX::conv::utf16toutf8(rAttrs[i].first) + "=" + NX::conv::utf16toutf8(rAttrs[i].second);
                            }

                            converted_wmtext.append(classifications);
                            has_attr = true;
                        }
                        else if (s2 == "$(host)")
                        {
                            if (g_hostName.size() <= 0)
                            {
                                NX::win::host hinfo;

                                g_hostName = NX::conv::utf16toutf8(hinfo.dns_host_name());
                                g_hostFQDN = NX::conv::utf16toutf8(hinfo.fqdn_name());
                                g_hostIP = NX::conv::utf16toutf8(hinfo.ip_address());
                            }

                            converted_wmtext.append(g_hostFQDN);
                            has_attr = true;
                        }
                        else if (s2 == "$(ip)")
                        {
                            if (g_hostName.size() <= 0)
                            {
                                NX::win::host hinfo;

                                g_hostName = NX::conv::utf16toutf8(hinfo.dns_host_name());
                                g_hostFQDN = NX::conv::utf16toutf8(hinfo.fqdn_name());
                                g_hostIP = NX::conv::utf16toutf8(hinfo.ip_address());
                            }

                            converted_wmtext.append(g_hostIP);
                            has_attr = true;
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
                            has_attr = true;
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
                            has_attr = true;
                        }
                        else if (s2 == "$(break)")
                        {
                            converted_wmtext.append("\n");
                            has_attr = true;
                        }

                        if (has_attr == false)
                        {
                            //
                            // non-supported classification, append back
                            //

                            //
                            // We commented following line, which means if the $(VARIABLE) is not found in our pre-defined set and not found in document/user attributes, they will be ignored
                            // change above definition: we will let external caller to handle if it is not in our pre-defined macro words
                            //
                            if (s2.length() > 3 && s2[0] == '$' && s2[1] == '(')
                            {
                                converted_wmtext.append(" ");
                            }
                            else
                            {
                                converted_wmtext.append(s1);
                            }
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
                    if (begin >= 0)
                    {
                        // in processing $xxxx
                    }
                    else
                    {
                        converted_wmtext.push_back(wmtext.data()[i]);
                    }

                }
            }

            if (begin >= 0 && end == -1)
            {
                // never match $xxxx
                //std::string s2 = wmtext.substr(begin);
                //converted_wmtext.append(s2);
            }
#pragma endregion

			ob.text = converted_wmtext;
		}
	}

	return RESULT(0);
}
