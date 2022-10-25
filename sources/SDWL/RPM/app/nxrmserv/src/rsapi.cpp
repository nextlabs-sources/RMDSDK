

#include <Winsock2.h>
#include <WS2tcpip.h>
#include <WSPiApi.h>
#include <Psapi.h>

#include <Windows.h>
#include <assert.h>

#include <nudf\eh.hpp>
#include <nudf\string.hpp>
#include <nudf\conversion.hpp>
#include <nudf\crypto.hpp>
#include <nudf\json.hpp>

#include "nxrmserv.hpp"
#include "serv.hpp"
#include "global.hpp"
#include "rsapi.hpp"

#include <fstream>
#include <sstream>
#include <vector>

using namespace NX;


extern rmserv* SERV;

bool ROUTER::query_tenant_info(const std::wstring& router_url, const std::wstring& tenant_id, std::wstring& rms_server_url)
{
    bool result = false;
    std::shared_ptr<NX::http::client> p;
    std::vector<LPCWSTR> call_accept_types;
    std::vector<std::pair<std::wstring, std::wstring>> call_headers;
    std::wstring    encoded_call_parameters;
    std::shared_ptr<NX::http::string_request> call_request;
    std::shared_ptr<NX::http::string_response> call_response;

    try {
		LOGDEBUG(NX::string_formater(L"query_tenant_info:  router_url (%s)", router_url.c_str()));
		// const std::wstring service_path(L"/router/rs/q/tokenGroupName/" + tenant_id);
		const std::wstring service_path(L"/router/rs/q/defaultTenant");

        p = std::shared_ptr<NX::http::client>(new NX::http::client(router_url, true, SERV->get_service_conf().get_network_timeout()));

        call_accept_types.push_back(NX::http::mime_types::application_json.c_str());
        call_accept_types.push_back(NX::http::mime_types::text.c_str());
        call_request = std::shared_ptr<NX::http::string_request>(new NX::http::string_request(NX::http::methods::GET, service_path, call_headers, call_accept_types, std::wstring()));
        call_response = std::shared_ptr<NX::http::string_response>(new NX::http::string_response());

        p->send_request(call_request.get(), call_response.get());		

        NX::json_value json_response = NX::json_value::parse(call_response->body());
        const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        if (200 == statusCode) {
            rms_server_url = json_response.as_object().at(L"results").as_object().at(L"server").as_string();
            result = true;
			LOGDEBUG(NX::string_formater(L"query_tenant_info:  rms_server_url: %s", rms_server_url.c_str()));
        }
        else {
            LOGERROR(NX::string_formater(L"Fail to get RMS information from router (%d, %s)", statusCode, statusMessage.c_str()));
        }
    }
    catch (const std::exception& e) {
        LOGERROR(NX::string_formater("Fail to initialize RESTful service executor (%s)", e.what()));
        UNREFERENCED_PARAMETER(e);
        p.reset();
        result = false;
    }

    return result;
}

RS::executor::executor() : _good_connection(false)
{
}

RS::executor::~executor()
{
}

bool RS::executor::initialize(const std::wstring& url)
{
    bool ret = false;

    try {
		LOGDEBUG(NX::string_formater(L"executor::initialize:  url: (%s)", url.c_str()));
        _client = std::shared_ptr<NX::http::client>(new NX::http::client(url, true, SERV->get_service_conf().get_network_timeout()));
        _server_url = url;
        ret = true;
    }
    catch (const std::exception& e) {
        LOGERROR(NX::string_formater("Fail to initialize RESTful service executor (%s)", e.what()));
        UNREFERENCED_PARAMETER(e);
        _client.reset();
    }

    return ret;
}

void RS::executor::clear()
{
	LOGDEBUG(NX::string_formater(L"RS::executor::clear: "));
    _client.reset();
	_client = nullptr;
}

bool RS::executor::request_sign_membership(__int64 user_id,
    const std::wstring& member_id,
    const std::wstring& ticket,
    const std::wstring& encoded_public_key,
    std::vector<std::wstring>& certificates)
{
    bool result = false;
    std::wstring request_body;
    std::wstring response;

    static const std::wstring CERT_HEADER(L"-----BEGIN CERTIFICATE-----");
    static const std::wstring CERT_FOOTER(L"-----END CERTIFICATE-----");

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    try {

        NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
        }), false);
        NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

        json_parameters[L"userId"] = NX::json_value(NX::conversion::to_wstring(user_id));
        json_parameters[L"ticket"] = NX::json_value(ticket);
        json_parameters[L"membership"] = NX::json_value(member_id);
        json_parameters[L"publicKey"] = NX::json_value(encoded_public_key);
		json_parameters[L"platformId"] = NX::json_value(GLOBAL.get_windows_info().platform_id());

        const std::wstring& parameters = json_body.serialize();
		LOGDEBUG(NX::string_formater(L"request_sign_membership: parameters: %s", parameters.c_str()));

        if (!execute(NX::http::methods::PUT, L"/rms/rs/membership", parameters, NX::http::mime_types::application_json, response)) {
            throw std::exception("fail to sign membership key");
        }

        NX::json_value json_response = NX::json_value::parse(response);
        const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        if (200 != statusCode) {
            LOGERROR(NX::string_formater(L"RS::sign_member_key failed (%d, %s)", statusCode, statusMessage.c_str()));
            if (400 == statusCode) {
                LOGDEBUG(parameters);
            }
            throw std::exception("fail to sign membership key");
        }
        
        std::wstring str_certificates = json_response.as_object().at(L"results").as_object().at(L"certficates").as_string();
        std::vector<std::wstring> received_certs;

        while (!str_certificates.empty()) {
            auto pos = str_certificates.find(CERT_HEADER);
            if (pos == std::wstring::npos) {
                break;
            }
            if (pos != 0) {
                str_certificates = str_certificates.substr(pos);
            }
            pos = str_certificates.find(CERT_HEADER, CERT_HEADER.length());
            if (pos != std::wstring::npos) {
                received_certs.push_back(str_certificates.substr(0, pos));
                str_certificates = str_certificates.substr(pos);
            }
            else {
                received_certs.push_back(str_certificates);
                str_certificates.clear();
            }
        }

        if (received_certs.size() < 2) {
            LOGERROR(NX::string_formater(L"rs::sign_member_key only %d certificates are returned", received_certs.size()));
            throw std::exception("too few certificates");
        }
        else if (received_certs.size() == 2) {
            certificates.push_back(received_certs[1]);
            result = true;
        }
        else if (received_certs.size() == 3) {
            certificates.push_back(received_certs[2]);
            certificates.push_back(received_certs[1]);
            result = true;
        }
        else {
            LOGWARNING(NX::string_formater(L"rs::sign_member_key too many certificates (%d) are returned", received_certs.size()));
            certificates.push_back(received_certs[2]);
            certificates.push_back(received_certs[1]);
            result = true;
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool RS::executor::request_generate_tokens(__int64 user_id,
    const std::wstring& member_id,
    const std::wstring& ticket,
    const std::vector<UCHAR>& agreement,
    int count,
    std::vector<encrypt_token>& tokens)
{
    bool result = false;
    std::wstring request_body;
    std::wstring response;

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    try {

        NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
        }), false);
        NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

        json_parameters[L"userId"] = NX::json_value(NX::conversion::to_wstring(user_id));
        json_parameters[L"ticket"] = NX::json_value(ticket);
        json_parameters[L"membership"] = NX::json_value(member_id);
        json_parameters[L"agreement"] = NX::json_value(NX::conversion::to_wstring(agreement));
        json_parameters[L"count"] = NX::json_value(count);
		json_parameters[L"prefetch"] = NX::json_value(true);

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", member_id));
		if (SERV->is_server_mode() == false)
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		}
		else
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring((__int64)SERV->get_service_conf().get_api_app_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", SERV->get_service_conf().get_api_app_key()));
		}
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));

        const std::wstring& parameters = json_body.serialize();
		::OutputDebugString(parameters.c_str());
        if (!execute(NX::http::methods::PUT, L"/rms/rs/token", parameters, NX::http::mime_types::application_json, response, call_headers)) {
			::OutputDebugString(L"fail to generate tokens key");
			throw std::exception("fail to generate tokens key");
        }
		::OutputDebugString(response.c_str());

        NX::json_value json_response = NX::json_value::parse(response);
		const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        if (200 != statusCode) {
            LOGERROR(NX::string_formater(L"RS::generate_token failed (%d, %s)", statusCode, statusMessage.c_str()));
            if (400 == statusCode) {
                LOGDEBUG(parameters);
            }
            throw std::exception("fail to sign membership key");
        }

        const NX::json_object& results = json_response.as_object().at(L"results").as_object();
        const std::wstring& ws_token_level = results.at(L"ml").as_string();
        const unsigned long token_level = std::wcstol(ws_token_level.c_str(), NULL, 10);
        const NX::json_object& tokens_list = results.at(L"tokens").as_object();

        std::for_each(tokens_list.begin(), tokens_list.end(), [&](const std::pair<std::wstring, NX::json_value>& item) {
            const std::wstring& token_data_str = item.second.as_object().at(L"token").as_string();
			const std::wstring& token_otp_str = item.second.as_object().at(L"otp").as_string();
			encrypt_token token(token_level, NX::utility::hex_string_to_buffer(item.first), NX::utility::hex_string_to_buffer(token_otp_str), NX::utility::hex_string_to_buffer(token_data_str));
            if (token.empty()) {
                LOGWARNING(NX::string_formater(L"Empty encrypt token: [%s, %s]", item.first.c_str(), token_data_str.c_str()));
            }
            else {
                if (token.get_token_id().size() == 16 && token.get_token_value().size() == 32) {
                    tokens.push_back(token);
                }
                else {
                    LOGWARNING(NX::string_formater(L"Bad encrypt token size (%d, %d): [%s, %s]", token.get_token_id().size(), token.get_token_value().size(), item.first.c_str(), token_data_str.c_str()));
                }
            }
        });

        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

BOOL IsCurrentSessionRemoteable()
{
	BOOL fIsRemoteable = FALSE;

	if (GetSystemMetrics(SM_REMOTESESSION))
	{
		fIsRemoteable = TRUE;
	}
	else
	{
		HKEY hRegKey = NULL;
		LONG lResult;

		lResult = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\",
			0, // ulOptions
			KEY_READ,
			&hRegKey
		);

		if (lResult == ERROR_SUCCESS)
		{
			DWORD dwGlassSessionId;
			DWORD cbGlassSessionId = sizeof(dwGlassSessionId);
			DWORD dwType;

			lResult = RegQueryValueEx(
				hRegKey,
				L"GlassSessionId",
				NULL, // lpReserved
				&dwType,
				(BYTE*)&dwGlassSessionId,
				&cbGlassSessionId
			);

			if (lResult == ERROR_SUCCESS)
			{
				DWORD dwCurrentSessionId;

				if (ProcessIdToSessionId(GetCurrentProcessId(), &dwCurrentSessionId))
				{
					fIsRemoteable = (dwCurrentSessionId != dwGlassSessionId);
				}
			}
		}

		if (hRegKey)
		{
			RegCloseKey(hRegKey);
		}
	}

	return fIsRemoteable;
}

bool RS::executor::request_retrieve_decrypt_token(unsigned long process_id, __int64 user_id,
    const std::wstring& tenant_id,
    const std::wstring& owner_id,   // It is member_id
    const std::wstring& ticket,
    const std::vector<UCHAR>& agreement,
    const std::vector<UCHAR>& duid, // It is token_id
    int protection_type,
    const std::wstring& policy,
    const std::wstring& tags,
    int token_level,
    std::vector<UCHAR>& token_value,
    _Out_opt_ int* returned_code)
{
    bool result = false;
    std::wstring request_body;
    std::wstring response;

    if (nullptr != returned_code) {
        *returned_code = 0;
    }

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    try {

        const std::wstring& ws_token_id = NX::conversion::to_wstring(duid);

        NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
        }), false);
        NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

        json_parameters[L"userId"] = NX::json_value(NX::conversion::to_wstring(user_id));
        json_parameters[L"ticket"] = NX::json_value(ticket);
        json_parameters[L"tenant"] = NX::json_value(tenant_id);
        json_parameters[L"owner"] = NX::json_value(owner_id);
        json_parameters[L"agreement"] = NX::json_value(NX::conversion::to_wstring(agreement));
        json_parameters[L"duid"] = NX::json_value(NX::conversion::to_wstring(duid));
        json_parameters[L"protectionType"] = NX::json_value(NX::conversion::to_wstring(protection_type));
        json_parameters[L"filePolicy"] = NX::json_value(policy);
        json_parameters[L"fileTags"] = NX::json_value(tags);
		json_parameters[L"ml"] = NX::json_value(NX::conversion::to_wstring(token_level));

		NX::json_value eval_request = NX::json_value::create_object();
		NX::json_value host = NX::json_value::create_object();
		NX::json_value application = NX::json_value::create_object();

		std::wstring image_path;
		std::wstring image_name;
		HANDLE h = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
		if (NULL != h) {
			GetModuleFileNameEx(h, NULL, NX::string_buffer<wchar_t>(image_path, MAX_PATH), MAX_PATH);
			GetModuleBaseNameW(h, 0, NX::string_buffer<wchar_t>(image_name, MAX_PATH), MAX_PATH);
			CloseHandle(h);
		}

		application[L"name"] = NX::json_value(image_name);
		application[L"path"] = NX::json_value(image_path);
		application[L"pid"] = NX::json_value(NX::conversion::to_wstring((int)process_id));

		// Get Host Name, and Host IP
		static std::wstring g_hostName;
		static std::wstring g_hostIP;
		static std::wstring g_hostFQDN;
		static bool g_bRemoteSession;


		if (g_hostName.size() <= 0)
		{
			char hostName[1024];
			struct addrinfo hints, *res, *p;
			struct in_addr hostIPAddr;
			int err;

			// Declare and initialize variables
			WSADATA wsaData = { 0 };
			int iResult = 0;
			// Initialize Winsock
			iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (iResult == 0)
			{
				if (gethostname(hostName, 1024) == 0) {
					memset(&hints, 0, sizeof(hints));
					hints.ai_socktype = SOCK_STREAM;
					hints.ai_family = AF_INET;

					if ((err = getaddrinfo(hostName, NULL, &hints, &res)) == 0) {
						hostIPAddr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
						int ip = (int)(hostIPAddr.s_addr);
						char ipstr[INET6_ADDRSTRLEN];
						for (p = res; p != NULL; p = p->ai_next)
						{
							void *addr;
							char *ipver;

							// Get the pointer to the address itself, different fields in IPv4 and IPv6
							if (p->ai_family == AF_INET)
							{
								// IPv4
								struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
								addr = &(ipv4->sin_addr);
								ipver = "IPv4";
								// Convert the IP to a string
								// InetNtop(address_family, IP_address_in_network_byte_to_convert_to_a_string,
								//         buffer_to_store_the_IP_address_string, the_IP_string_length_in_character);
								inet_ntop(p->ai_family, addr, (PSTR)ipstr, sizeof(ipstr));
								g_hostIP = NX::conversion::utf8_to_utf16(std::string(ipstr));
								g_hostName = NX::conversion::utf8_to_utf16(std::string(hostName));

								//-----------------------------------------
								// Call getnameinfo
								char servInfo[NI_MAXSERV];
								DWORD dwRetval = getnameinfo((struct sockaddr *) ipv4,
									sizeof(struct sockaddr),
									hostName,
									NI_MAXHOST, servInfo, NI_MAXSERV, NI_NUMERICSERV);
								if (dwRetval == 0) {
									g_hostFQDN = NX::conversion::utf8_to_utf16(std::string(hostName));
								}

								break; // only support the 1st IPv4 address
							}
							else
							{
								// IPv6
								struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
								addr = &(ipv6->sin6_addr);
								ipver = "IPv6";
							}
						}
						freeaddrinfo(res);
					}
				}
				WSACleanup();
			}

			// Environment
			g_bRemoteSession = IsCurrentSessionRemoteable();
		}

		host[L"ipAddress"] = NX::json_value(g_hostIP);
		NX::json_value hostnames = NX::json_value::create_array();
		NX::json_value hattribute = NX::json_value::create_object();
		hostnames.as_array().push_back(NX::json_value(g_hostFQDN));
		hattribute[L"name"] = hostnames;
		host[L"attributes"] = hattribute;

		NX::json_value environment = NX::json_value::create_object();
		environment[L"name"] = NX::json_value(L"environment");
		NX::json_value eattribute = NX::json_value::create_object();
		NX::json_value connection_type = NX::json_value::create_array();
		if (g_bRemoteSession)
			connection_type.as_array().push_back(NX::json_value(L"remote"));
		else
			connection_type.as_array().push_back(NX::json_value(L"console"));
		eattribute[L"connection_type"] = connection_type;
		environment[L"attributes"] = eattribute;

		eval_request[L"host"] = host;
		eval_request[L"application"] = application;
		NX::json_value environments = NX::json_value::create_array();
		environments.as_array().push_back(environment);
		eval_request[L"environments"] = environments;
		json_parameters[L"dynamicEvalRequest"] = eval_request;

		const std::wstring& parameters = json_body.serialize();
		LOGDEBUG(NX::string_formater(L"RS::request_retrieve_decrypt_token:  (%s)", parameters.c_str()));
		LOGDEBUG(NX::string_formater(L"request_retrieve_decrypt_token: clientId: %s, duid: %s", SERV->get_client_id().c_str(), (NX::conversion::to_wstring(duid)).c_str() ));

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		if (SERV->is_server_mode() == false)
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", tenant_id));
		if (SERV->is_server_mode() == false)
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		}
		else
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring((__int64)(SERV->get_service_conf().get_api_app_id()))));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", SERV->get_service_conf().get_api_app_key()));
		}
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));

        if (!execute(NX::http::methods::POST, L"/rms/rs/token", parameters, NX::http::mime_types::application_json, response, call_headers)) {
            throw std::exception("fail to retrieve decrypt token");
        }

        NX::json_value json_response = NX::json_value::parse(response);
        const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        if (nullptr != returned_code) {
            *returned_code = statusCode;
        }

        if (200 != statusCode) 
		{
            LOGERROR(NX::string_formater(L"RS::acquire_token failed (%d, %s)", statusCode, statusMessage.c_str()));
            throw std::exception("fail to sign membership key");
        }

        const NX::json_object& results = json_response.as_object().at(L"results").as_object();
        if (!results.has_field(L"token")) {
            LOGWARNING(NX::string_formater(L"Fail to retrieve token: [%s, %d]", ws_token_id.c_str(), token_level));
        }
        else {
            const std::wstring& ws_token = results.at(L"token").as_string();
            token_value = NX::utility::hex_string_to_buffer(ws_token);
            if (token_value.size() == 32) {
                // Finally get a valid token
                result = true;
            }
            else {
                LOGWARNING(NX::string_formater(L"Retrieved token has a wrong size (%d): [%s, %d]", token_value.size(), ws_token_id.c_str(), token_level));
            }
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool RS::executor::request_heartbeat_v1(__int64 user_id,
    const std::wstring& tenant_id,
    const std::wstring& ticket,
    const std::vector<std::pair<std::wstring, std::wstring>>& objects_request,
    std::vector<std::pair<std::wstring, std::pair<std::wstring, std::wstring>>>& objects_returned)
{
    bool result = false;
    std::wstring request_body;
    std::wstring response;

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    try {

        const std::wstring service_path(tenant_id.empty() ? L"/rms/rs/heartbeat" : (std::wstring(L"/rms/rs/heartbeat?tenant=") + tenant_id));

        NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
        }), false);
        NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

        json_parameters[L"userId"] = NX::json_value(NX::conversion::to_wstring(user_id));
        json_parameters[L"tenant"] = NX::json_value(tenant_id);
        json_parameters[L"ticket"] = NX::json_value(ticket);
        json_parameters[L"platformId"] = NX::json_value((int)GLOBAL.get_windows_info().platform_id());
        json_parameters[L"objects"] = NX::json_value::create_array();
        NX::json_array& objects_array = json_parameters.at(L"objects").as_array();
        std::for_each(objects_request.begin(), objects_request.end(), [&](const std::pair<std::wstring, std::wstring>& object) {
            objects_array.push_back(NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
                std::pair<std::wstring, NX::json_value>(L"name", NX::json_value(object.first)),
                std::pair<std::wstring, NX::json_value>(L"serialNumber", NX::json_value(object.second))
            }), false));
        });

        const std::wstring& parameters = json_body.serialize();

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", tenant_id));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));


        if (!execute(NX::http::methods::POST, service_path, parameters, NX::http::mime_types::application_json, response)) {
            throw std::exception("fail to retrieve decrypt token");
        }

        NX::json_value json_response = NX::json_value::parse(response);
        const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        if (200 != statusCode) {
            LOGERROR(NX::string_formater(L"RS::heartbeat failed (%d, %s)", statusCode, statusMessage.c_str()));
            if (400 == statusCode) {
                LOGDEBUG(parameters);
            }
            throw std::exception("fail to do heartbeat");
        }

        if (json_response.as_object().has_field(L"results")) {

            const NX::json_object& results = json_response.as_object().at(L"results").as_object();
            std::for_each(results.begin(), results.end(), [&](const std::pair<std::wstring, NX::json_value>& object) {
                try {
					if (object.second.is_object()) {
						if (object.second.as_object().has_field(L"serialNumber") && object.second.as_object().has_field(L"content")) {
							const std::wstring& serialNumber = object.second.as_object().at(L"serialNumber").as_string();
							const std::wstring& content = object.second.as_object().at(L"content").as_string();
							objects_returned.push_back(std::pair<std::wstring, std::pair<std::wstring, std::wstring>>(object.first, std::pair<std::wstring, std::wstring>(serialNumber, content)));
						}
					}
                }
                catch (const std::exception& e) {
                    UNREFERENCED_PARAMETER(e);
                }
            });
        }

        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool RS::executor::request_heartbeat(__int64 user_id,
	const std::wstring& tenant_id,
	const std::wstring& ticket,
	std::map<std::wstring, std::wstring> &policy_map,
	std::wstring &policyData, unsigned int* heartbeatFrequency, 
	std::map<std::wstring, std::wstring> &tokengroupRT_map,
	std::wstring &tokengroupRT_data)
{
	bool result = false;
	std::wstring request_body;
	std::wstring response;

	if (empty()) {
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	try {

		const std::wstring service_path(L"/rms/rs/v2/heartbeat");

		NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
			std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
		}), false);
		NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();
		json_parameters[L"platformId"] = NX::json_value((int)GLOBAL.get_windows_info().platform_id());
		NX::json_array objects_array = NX::json_value::create_array().as_array();
		json_parameters[L"clientData"] = objects_array;

		const std::wstring& parameters = json_body.serialize();

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", tenant_id));
		if (SERV->is_server_mode() == false)
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		}
		else
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring((__int64)SERV->get_service_conf().get_api_app_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", SERV->get_service_conf().get_api_app_key()));
		}
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));


		if (!execute(NX::http::methods::POST, service_path, parameters, NX::http::mime_types::application_json, response, call_headers)) {
			throw std::exception("fail to get heartbeat");
		}

		LOGDEBUG(NX::string_formater(L"request_heartbeat: response: %s", response.c_str()));

		NX::json_value json_response = NX::json_value::parse(response);
		const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
		std::wstring statusMessage;
		if (json_response.as_object().has_field(L"message")) {
			statusMessage = json_response.as_object().at(L"message").as_string();
		}

		if (200 != statusCode) {
			LOGERROR(NX::string_formater(L"RS::heartbeat failed (%d, %s)", statusCode, statusMessage.c_str()));
			if (400 == statusCode) {
				LOGDEBUG(parameters);
			}
			throw std::exception("fail to do heartbeat");
		}
		
		if (json_response.as_object().has_field(L"results")) {

			const NX::json_object& results = json_response.as_object().at(L"results").as_object();
			policyData = L"";
			policy_map.clear();
			tokengroupRT_data = L"";
			tokengroupRT_map.clear();
			LOGDEBUG(L"request_heartbeat response: ");

			if (results.has_field(L"heartbeatFrequency"))
			{
				*heartbeatFrequency = results.at(L"heartbeatFrequency").as_uint32();
				LOGDEBUG(NX::string_formater(L"   heartbeatFrequency: %u", *heartbeatFrequency));
			}

			if (results.has_field(L"policyConfigData")) 
			{
				NX::json_value policyConfigData = results.at(L"policyConfigData");
				if (policyConfigData.is_array()) {
					policyData = policyConfigData.serialize();
					const NX::json_array& policy_array = policyConfigData.as_array();
					for (NX::json_array::const_iterator it = policy_array.cbegin(); it != policy_array.cend(); ++it)
					{
						std::wstring tokenGroupName = it->as_object().at(L"tokenGroupName").as_string();
						std::wstring bundle = L"";
						if (it->as_object().has_field(L"policyBundle"))
						{
							bundle = it->as_object().at(L"policyBundle").as_string();
						}
						policy_map[tokenGroupName] = bundle;
						LOGDEBUG(NX::string_formater(L"request_heartbeat: tokenGroupName: %s", tokenGroupName.c_str()));
					}
				}
			}

			if (results.has_field(L"tokenGroupResourceTypeMapping"))
			{
				NX::json_value tokengroupResTyMap = results.at(L"tokenGroupResourceTypeMapping");
				if (tokengroupResTyMap.is_object()) {
					tokengroupRT_data = tokengroupResTyMap.serialize();
					for (auto it = policy_map.cbegin(); it != policy_map.cend(); ++it)
					{
						std::wstring tokengroupName = it->first;
						if (tokengroupResTyMap.as_object().has_field(tokengroupName))
						{
							std::wstring resourcetype = tokengroupResTyMap.as_object().at(tokengroupName).as_string();
							tokengroupRT_map[tokengroupName] = resourcetype;
							LOGDEBUG(NX::string_formater(L"request_heartbeat: tokenGroupResourceTypeMapping tokenGroupName: %s", tokengroupName.c_str()));
						}
					}
				}
			}
		}

		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		result = false;
	}

	return result;
}

bool RS::executor::get_tenant_preference(__int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, 
	                                     int* client_heartbeat, bool* adhoc, bool* dapenable, std::wstring& system_bucket_name, std::wstring &icenetUrl)
{
	bool result = false;
	std::wstring request_body;
	std::wstring response;

	if (empty()) {
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	try {

		const std::wstring service_path(L"/rms/rs/tenant/v2/" + tenant_id);

		NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
			std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
		}), false);

		NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();
		json_parameters[L"userId"] = NX::json_value(NX::conversion::to_wstring(user_id));
		json_parameters[L"ticket"] = NX::json_value(ticket);
		json_parameters[L"tenant"] = NX::json_value(tenant_id);


		const std::wstring& parameters = json_body.serialize();

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", tenant_id));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));


		if (!execute(NX::http::methods::GET, service_path, parameters, NX::http::mime_types::application_json, response, call_headers)) {
			throw std::exception("fail to get tenant preference");
		}

		LOGDEBUG(NX::string_formater(L"get_tenant_preference: response: %s", response.c_str()));

		NX::json_value json_response = NX::json_value::parse(response);
		const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
		std::wstring statusMessage;
		if (json_response.as_object().has_field(L"message")) {
			statusMessage = json_response.as_object().at(L"message").as_string();
		}

		if (200 != statusCode) {
			LOGERROR(NX::string_formater(L"RS::get_tenant_preference failed (%d, %s)", statusCode, statusMessage.c_str()));
			if (400 == statusCode) {
				LOGDEBUG(parameters);
			}
			throw std::exception("fail to do get_tenant_preference");
		}

		if (json_response.as_object().has_field(L"extra")) {

			const NX::json_object& results = json_response.as_object().at(L"extra").as_object();

			if (results.has_field(L"ADHOC_ENABLED"))
			{
				*adhoc = results.at(L"ADHOC_ENABLED").as_boolean();
			}

			if (results.has_field(L"DAPSERVER_ENABLED"))
			{
				*dapenable = results.at(L"DAPSERVER_ENABLED").as_boolean();
			}

			if (results.has_field(L"CLIENT_HEARTBEAT_FREQUENCY"))
			{
				std::string hearbeat = NX::conversion::utf16_to_utf8(results.at(L"CLIENT_HEARTBEAT_FREQUENCY").as_string());
				*client_heartbeat = std::atoi(hearbeat.c_str());
				LOGDEBUG(NX::string_formater(L"   CLIENT_HEARTBEAT_FREQUENCY: %d", *client_heartbeat));
			}

			if (results.has_field(L"SYSTEM_BUCKET_NAME"))
			{
				system_bucket_name = results.at(L"SYSTEM_BUCKET_NAME").as_string();
				LOGDEBUG(NX::string_formater(L"   SYSTEM_BUCKET_NAME: %s", system_bucket_name.c_str()));
			}

			if (results.has_field(L"ICENET_URL"))
			{
				icenetUrl = results.at(L"ICENET_URL").as_string();
				LOGDEBUG(NX::string_formater(L"   ICENET_URL: %s", icenetUrl.c_str()));
			}

		}

		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		result = false;
	}

	return result;
}

bool RS::executor::request_post_sharing_trasition(__int64 user_id, const std::wstring& ticket, const std::wstring& checksum, const std::wstring& shared_doc_info)
{
    bool result = false;
    std::wstring request_body;
    std::wstring response;

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }
    
    try {

        NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
        }), false);
        NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();


        json_parameters[L"userId"] = NX::json_value(NX::conversion::to_wstring(user_id));
        json_parameters[L"deviceId"] = NX::json_value(SERV->GetDeviceID());
        json_parameters[L"deviceType"] = NX::json_value(NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id()));
        json_parameters[L"ticket"] = NX::json_value(ticket);
        json_parameters[L"checksum"] = NX::json_value(checksum);
        json_parameters[L"sharedDocument"] = NX::json_value(shared_doc_info);

        const std::wstring& parameters = json_body.serialize();
        if (!execute(NX::http::methods::POST, L"/rms/rs/share", parameters, NX::http::mime_types::application_json, response)) {
            throw std::exception("fail to retrieve decrypt token");
        }

        NX::json_value json_response = NX::json_value::parse(response);
        const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        if (200 != statusCode) {
            LOGERROR(NX::string_formater(L"RS::share failed (%d, %s)", statusCode, statusMessage.c_str()));
            if (400 == statusCode) {
                LOGDEBUG(parameters);
            }
            throw std::exception("fail to upload sharing information");
        }

        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

bool RS::executor::request_upgrade(const std::wstring& tenant_id, const std::wstring& current_version, _Out_ std::wstring& new_version, _Out_ std::wstring& download_url, _Out_ std::wstring& checksum)
{
    bool result = false;
    std::wstring request_body;
    std::wstring response;

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    try {

        //const bool is_default_tenant = (tenant_id.empty() || (0 == _wcsicmp(tenant_id.c_str(), L"nextlabs.com")));
        const std::wstring service_path(tenant_id.empty() ? L"/rms/rs/upgrade" : (std::wstring(L"/rms/rs/upgrade?tenant=") + tenant_id));

        NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
            std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
        }), false);
        NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

        json_parameters[L"platformId"] = NX::json_value(NX::conversion::to_wstring((int)(GLOBAL.get_windows_info().platform_id())));
#ifdef _WIN64
        json_parameters[L"processorArch"] = NX::json_value(L"x64");
#else
#ifdef _IA64_
        json_parameters[L"processorArch"] = NX::json_value(L"IA64");
#else
        json_parameters[L"processorArch"] = NX::json_value(L"x86");
#endif
#endif
        json_parameters[L"tenant"] = NX::json_value(SERV->get_router_config().get_tenant_id());
        json_parameters[L"currentVersion"] = NX::json_value(GLOBAL.get_product_version_string());

        const std::wstring& parameters = json_body.serialize();
        if (!execute(NX::http::methods::POST, service_path, parameters, NX::http::mime_types::application_json, response)) {
            throw std::exception("fail to check for upgrade");
        }

        NX::json_value json_response = NX::json_value::parse(response);
        const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        if (200 != statusCode) {
            if (304 != statusCode) {
                LOGERROR(NX::string_formater(L"RS::upgrade failed (%d, %s)", statusCode, statusMessage.c_str()));
                if (400 == statusCode) {
                    LOGDEBUG(parameters);
                }
            }
            throw std::exception("fail to request upgrade information");
        }

        const std::wstring& newVersion = json_response.as_object().at(L"results").as_object().at(L"newVersion").as_string();
        const std::wstring& downloadURL = json_response.as_object().at(L"results").as_object().at(L"downloadURL").as_string();
        const std::wstring& sha1Checksum = json_response.as_object().at(L"results").as_object().at(L"sha1Checksum").as_string();
        
        if (!newVersion.empty() && !downloadURL.empty() && !sha1Checksum.empty()) {
            new_version = newVersion;
            download_url = downloadURL;
            checksum = sha1Checksum;
            result = true;
        }

    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

// upload activity log
bool RS::executor::request_log_activities(const __int64 user_id, const std::wstring& ticket, const std::vector<unsigned char>& logdata)
{
    bool result = false;

	//const std::wstring& service_path = NX::string_formater(L"/rms/rs/log/activity/%I64d/%s", user_id, ticket.c_str());
	const std::wstring& service_path = NX::string_formater(L"/rms/rs/log/v2/activity");
	std::wstring response;

	if (SERV->is_server_mode() == false)
	{
		if (!execute(NX::http::methods::PUT, service_path, logdata, http::mime_types::text_csv, user_id, ticket, GLOBAL.get_windows_info().platform_id(), response)) {
			return false;
		}
	}
	else
	{
		if (!execute(NX::http::methods::PUT, service_path, logdata, http::mime_types::text_csv, (__int64)SERV->get_service_conf().get_api_app_id(), SERV->get_service_conf().get_api_app_key(), -1, response)) {
			return false;
		}
	}

    try {
        
        NX::json_value json_response = NX::json_value::parse(response);
        const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
        std::wstring statusMessage;
        if (json_response.as_object().has_field(L"message")) {
            statusMessage = json_response.as_object().at(L"message").as_string();
        }

        // As long as the data has been delivered, the job is done
        // Although the data may be dropped by server
        result = true;

        if (200 != statusCode) {
            LOGERROR(NX::string_formater(L"RS::log_activities failed (%d, %s)", statusCode, statusMessage.c_str()));
            if (400 == statusCode) {
                // 400 - Malformed data, just abandon it
                LOGERROR(L"  -> Abandon invalid log data");
            }
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        result = false;
    }

    return result;
}

// upload new NXL metadata
bool RS::executor::request_classify_file(const __int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, const std::wstring &duid, const std::wstring &parameters)
{
	bool result = true;
	std::wstring response;

	{
		std::wstring service_path = L"/rms/rs/token/" + duid;

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", tenant_id));
		if (SERV->is_server_mode() == false)
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		}
		else
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring((__int64)SERV->get_service_conf().get_api_app_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", SERV->get_service_conf().get_api_app_key()));
		}
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));


        LOGDEBUG(NX::string_formater(L"RS::request_classify_file"));
        
        if (!execute(NX::http::methods::PUT, service_path, parameters, http::mime_types::application_json, response, call_headers)) {
			return false; // network issue, return to wait for another run
		}

		try {

			NX::json_value json_response = NX::json_value::parse(response);
			const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
			std::wstring statusMessage;
			if (json_response.as_object().has_field(L"message")) {
				statusMessage = json_response.as_object().at(L"message").as_string();
			}

            LOGDEBUG(NX::string_formater(L"RS::request_classify_file response (%s)", statusMessage.c_str()));

			if (200 != statusCode) {
				result = false;
				LOGERROR(NX::string_formater(L"RS::request_classify_file failed (%d, %s)", statusCode, statusMessage.c_str()));
				if (400 == statusCode) {
					// 400 - Malformed data, just abandon it
					LOGERROR(L"  -> Abandon invalid metadata");
				}
			}
		}
		catch (const std::exception& e) {
			result = false;
			UNREFERENCED_PARAMETER(e);
		}
	}

	return result;
}

bool RS::executor::request_reclassify_project_file(const __int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, const std::wstring &duid, const int projectid, const std::wstring &parameters)
{
	bool result = true;
	std::wstring response;

	{
		std::wstring service_path = L"/rms/rs/project/" + std::to_wstring(projectid) + L"/file/classification";

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", tenant_id));
		if (SERV->is_server_mode() == false)
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		}
		else
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring((__int64)SERV->get_service_conf().get_api_app_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", SERV->get_service_conf().get_api_app_key()));
		}
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));

        LOGDEBUG(NX::string_formater(L"RS::request_reclassify_project_file"));

		if (!execute(NX::http::methods::PUT, service_path, parameters, http::mime_types::application_json, response, call_headers)) {
			return false; // network issue, return to wait for another run
		}

		try {

			NX::json_value json_response = NX::json_value::parse(response);
			const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
			std::wstring statusMessage;
			if (json_response.as_object().has_field(L"message")) {
				statusMessage = json_response.as_object().at(L"message").as_string();
			}

			if (200 != statusCode) {
				result = false;
				LOGERROR(NX::string_formater(L"RS::request_reclassify_file failed (%d, %s)", statusCode, statusMessage.c_str()));
				if (400 == statusCode) {
					// 400 - Malformed data, just abandon it
					result = true;
					LOGERROR(L"  -> Abandon invalid re-classifications");
				}
			}

		}
		catch (const std::exception& e) {
			UNREFERENCED_PARAMETER(e);
		}
	}

	return result;
}

bool RS::executor::request_reclassify_file(const __int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, const std::wstring &duid, const std::wstring &parameters)
{
	bool result = true;
	std::wstring response;

	{
		std::wstring service_path = L"/rms/rs/token/" + duid;

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"tenant", tenant_id));
		if (SERV->is_server_mode() == false)
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
		}
		else
		{
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring((__int64)SERV->get_service_conf().get_api_app_id())));
			call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", SERV->get_service_conf().get_api_app_key()));
		}
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));

        LOGDEBUG(NX::string_formater(L"RS::request_reclassify_file"));

		if (!execute(NX::http::methods::PUT, service_path, parameters, http::mime_types::application_json, response, call_headers)) {
			return false; // network issue, return to wait for another run
		}

		try {

			NX::json_value json_response = NX::json_value::parse(response);
			const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
			std::wstring statusMessage;
			if (json_response.as_object().has_field(L"message")) {
				statusMessage = json_response.as_object().at(L"message").as_string();
			}

			LOGDEBUG(NX::string_formater(L"RS::request_reclassify_file response (%d, %s)", statusCode, statusMessage.c_str()));

			// As long as the data has been delivered, the job is done
			// Although the data may be dropped by server
			result = true;

			if (200 != statusCode) {
				LOGERROR(NX::string_formater(L"RS::request_reclassify_file failed (%d, %s)", statusCode, statusMessage.c_str()));
				if (400 == statusCode) {
					// 400 - Malformed data, just abandon it
					LOGERROR(L"  -> Abandon invalid re-classifications");
				}
			}
		}
		catch (const std::exception& e) {
			UNREFERENCED_PARAMETER(e);
		}
	}

	return result;
}

bool RS::executor::execute(const std::wstring& service_method, const std::wstring& service_path, const std::wstring& parameters, const std::wstring& parameters_type, std::wstring& response,
	const std::vector<std::pair<std::wstring, std::wstring>>& call_headers)
{
	static bool network_good_condition = true;

	if (empty()) {
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	bool ret = false;
	std::vector<LPCWSTR> call_accept_types;
	//std::vector<std::pair<std::wstring, std::wstring>> call_headers;
	std::wstring    encoded_call_parameters;
	std::shared_ptr<NX::http::string_request> call_request;
	std::shared_ptr<NX::http::string_response> call_response;

	call_accept_types.push_back(NX::http::mime_types::application_json.c_str());
	call_accept_types.push_back(NX::http::mime_types::text.c_str());
	//call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
	//call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id())));

	if (!parameters_type.empty()) {
		//call_headers.push_back(std::pair<std::wstring, std::wstring>(NX::http::header_names::content_type, parameters_type));
	}
	if (parameters_type == NX::http::mime_types::application_x_www_form_urlencoded) {
		encoded_call_parameters = NX::conversion::x_www_form_urlencode(parameters);
	}
	else {
		encoded_call_parameters = parameters;
	}

	//LOGDEBUG(NX::string_formater(L"executor::execute: encoded_call_parameters:  %s", encoded_call_parameters.c_str()));
	call_request = std::shared_ptr<NX::http::string_request>(new NX::http::string_request(service_method, service_path, call_headers, call_accept_types, encoded_call_parameters));
	call_response = std::shared_ptr<NX::http::string_response>(new NX::http::string_response());

	try {

		try {

			_client->send_request(call_request.get(), call_response.get());
			if (!network_good_condition) {
				network_good_condition = true;
				on_network_recover();
			}
		}
		catch (const NX::exception& e) {
			if (network_good_condition) {
				network_good_condition = false;
				on_network_error(e.code(), e.what());
			}
			throw e;
		}

		response = NX::conversion::utf8_to_utf16(call_response->body());
		ret = true;
	}
	catch (const std::exception& e) {
		::OutputDebugStringA(e.what());
		UNREFERENCED_PARAMETER(e);
		ret = false;
	}

	return ret;
}

bool RS::executor::execute(const std::wstring& service_method, const std::wstring& service_path, const std::wstring& parameters, const std::wstring& parameters_type, std::wstring& response)
{
    static bool network_good_condition = true;

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    bool ret = false;
    std::vector<LPCWSTR> call_accept_types;
    std::vector<std::pair<std::wstring, std::wstring>> call_headers;
    std::wstring    encoded_call_parameters;
    std::shared_ptr<NX::http::string_request> call_request;
    std::shared_ptr<NX::http::string_response> call_response;

    call_accept_types.push_back(NX::http::mime_types::application_json.c_str());
    call_accept_types.push_back(NX::http::mime_types::text.c_str());
    call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
	call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id()) ));
	
	if (!parameters_type.empty()) {
        call_headers.push_back(std::pair<std::wstring, std::wstring>(NX::http::header_names::content_type, parameters_type));
    }
    if (parameters_type == NX::http::mime_types::application_x_www_form_urlencoded) {
        encoded_call_parameters = NX::conversion::x_www_form_urlencode(parameters);
    }
    else {
        encoded_call_parameters = parameters;
    }

	//LOGDEBUG(NX::string_formater(L"executor::execute: encoded_call_parameters:  %s", encoded_call_parameters.c_str()));
    call_request = std::shared_ptr<NX::http::string_request>(new NX::http::string_request(service_method, service_path, call_headers, call_accept_types, encoded_call_parameters));
    call_response = std::shared_ptr<NX::http::string_response>(new NX::http::string_response());

    try {

        try {

            _client->send_request(call_request.get(), call_response.get());
            if (!network_good_condition) {
                network_good_condition = true;
                on_network_recover();
            }
        }
        catch (const NX::exception& e) {
            if (network_good_condition) {
                network_good_condition = false;
                on_network_error(e.code(), e.what());
            }
            throw e;
        }

        response = NX::conversion::utf8_to_utf16(call_response->body());
        ret = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        ret = false;
    }

    return ret;
}

bool RS::executor::execute(const std::wstring& service_method, const std::wstring& service_path, const std::vector<unsigned char>& parameters, const std::wstring& parameters_type, const __int64 user_id, const std::wstring& ticket, const __int32 platformid, std::wstring& response)
{
    static bool network_good_condition = true;

    if (empty()) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    bool ret = false;
    std::vector<LPCWSTR> call_accept_types;
    std::vector<std::pair<std::wstring, std::wstring>> call_headers;
    std::shared_ptr<NX::http::raw_request> call_request;
    std::shared_ptr<NX::http::string_response> call_response;

    call_accept_types.push_back(NX::http::mime_types::application_json.c_str());
    call_accept_types.push_back(NX::http::mime_types::text.c_str());
	call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(user_id)));
	call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", ticket));
    call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
	if (platformid > 0)
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"platformId", NX::conversion::to_wstring((int)platformid)));

    if (!parameters_type.empty()) {
        call_headers.push_back(std::pair<std::wstring, std::wstring>(NX::http::header_names::content_type, parameters_type));
    }

    call_request = std::shared_ptr<NX::http::raw_request>(new NX::http::raw_request(service_method, service_path, call_headers, call_accept_types, parameters));
    call_response = std::shared_ptr<NX::http::string_response>(new NX::http::string_response());

    try {

        try {

            _client->send_request(call_request.get(), call_response.get());
            if (!network_good_condition) {
                network_good_condition = true;
                on_network_recover();
            }
        }
        catch (const NX::exception& e) {
            if (network_good_condition) {
                network_good_condition = false;
                on_network_error(e.code(), e.what());
            }
            throw e;
        }

        response = NX::conversion::utf8_to_utf16(call_response->body());
        ret = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        ret = false;
    }

    return ret;
}

void RS::executor::on_network_recover()
{
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        LOGINFO(NX::string_formater(L"Network setup (%s)", _server_url.c_str()));
    }
    else {
        LOGINFO(L"Network recovered");
    }
    _good_connection = true;
}

void RS::executor::on_network_error(long error_code, const char* message)
{
    LOGINFO(NX::string_formater("Network error (%d, %s)", error_code, message));
    _good_connection = false;
}


bool RS::executor::request_query_nonce(const __int64 app_id, const std::wstring& app_key, std::wstring& nonce)
{
	bool result = false;
	std::wstring request_body;
	std::wstring response;

	if (empty()) {
		SetLastError(ERROR_INVALID_HANDLE);
    LOGERROR(NX::string_formater("error (%d) to call request_query_nonce)", GetLastError()));
    return false;
	}

	try {

		const std::wstring service_path(L"/rms/rs/login/trustedapp/nonce");

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(app_id)));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", app_key));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));

		if (!execute(NX::http::methods::GET, service_path, L"", NX::http::mime_types::application_json, response, call_headers)) {
      LOGERROR(NX::string_formater(L"error (%d) to call %s)", GetLastError(), service_path.c_str()));
      throw std::exception("fail to get nonce");
		}

		LOGDEBUG(NX::string_formater(L"get_nonce: response: %s", response.c_str()));

		NX::json_value json_response = NX::json_value::parse(response);
		const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
		std::wstring statusMessage;
		if (json_response.as_object().has_field(L"message")) {
			statusMessage = json_response.as_object().at(L"message").as_string();
		}

		if (200 != statusCode) {
			LOGERROR(NX::string_formater(L"RS::get_nonce failed (%d, %s)", statusCode, statusMessage.c_str()));
			throw std::exception("fail to do get_nonce");
		}

		if (json_response.as_object().has_field(L"results")) {

			const NX::json_object& results = json_response.as_object().at(L"results").as_object();

			if (results.has_field(L"nonce"))
			{
				nonce = results.at(L"nonce").as_string();
				LOGDEBUG(NX::string_formater(L"   nonce: %s", nonce.c_str()));
			}

		}

		result = true;
	}
	catch (const std::exception& e) {
    LOGERROR(NX::string_formater("request_query_nonce exception"));
    UNREFERENCED_PARAMETER(e);
		result = false;
	}

	return result;
}

bool RS::executor::request_login_apiuser(const __int64 app_id,
	const std::wstring& app_key,
	const std::wstring& email,
	const std::wstring& private_cert_path,
	NX::json_value &login_output)
{
	bool result = false;
	std::wstring request_body;
	std::wstring response;

	if (empty()) {
		SetLastError(ERROR_INVALID_HANDLE);
    LOGERROR(NX::string_formater("request_login_apiuser error"));
    return false;
	}

	try {
		std::wstring nonce;
		if (request_query_nonce(app_id, app_key, nonce) == false)
		{
      LOGERROR(NX::string_formater("request_login_apiuser error to call request_query_nonce"));
      throw std::exception("fail to get nonce for signin.");
    }

		std::string sha_value = NX::conversion::to_string(app_id) + "." 
			+ NX::conversion::utf16_to_utf8(app_key) + "." 
			+ NX::conversion::utf16_to_utf8(email) + "." 
			+ "21600000000" + "." 
			+ NX::conversion::utf16_to_utf8(nonce) + "." 
			+ "{}";

		std::vector<unsigned char> rsa_result;
		rsa_result.resize(1024, 0);
		ULONG rsa_result_size = 1024;
		std::vector<unsigned char> private_cert; //  NX::conversion::from_base64(private_cert_base64);
		std::ifstream       file(private_cert_path);
		if (file)
		{
			file.seekg(0, std::ios::end);
			std::streampos          length = file.tellg();
			file.seekg(0, std::ios::beg);

			std::vector<unsigned char>       buffer(length);
			file.read((char*)&buffer[0], length);

			private_cert = buffer;
			file.close();
		}
		else
		{
      LOGERROR(NX::string_formater("request_login_apiuser error to read certificate file"));
      throw std::exception("fail to read certificate file for login");
		}
		NX::crypto::rsasha256_sign(private_cert, (unsigned char *)sha_value.data(), sha_value.size(), (unsigned char *)rsa_result.data(), &rsa_result_size);

		std::vector<unsigned char> final_rsa_result(rsa_result.begin(), rsa_result.begin() + rsa_result_size);
		std::wstring rsa_output = NX::conversion::to_base64(final_rsa_result);

		NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
			std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
			}), false);
		NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

		json_parameters[L"appId"] = NX::json_value(app_id);
		json_parameters[L"ttl"] = NX::json_value(21600000000);
		json_parameters[L"nonce"] = NX::json_value(nonce);
		json_parameters[L"email"] = NX::json_value(email);
		json_parameters[L"userAttributes"] = NX::json_value(L"{}");
		json_parameters[L"signature"] = NX::json_value(rsa_output);

		const std::wstring& parameters = json_body.serialize();
		LOGDEBUG(NX::string_formater(L"request_login_apiuser: parameters: %s", parameters.c_str()));

		std::vector<std::pair<std::wstring, std::wstring>> call_headers;
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"userId", NX::conversion::to_wstring(app_id)));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"ticket", app_key));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"clientId", SERV->get_client_id()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"deviceId", SERV->GetDeviceID()));
		call_headers.push_back(std::pair<std::wstring, std::wstring>(L"Content-Type", NX::http::mime_types::application_json));

		if (!execute(NX::http::methods::POST, L"/rms/rs/login/trustedapp", parameters, NX::http::mime_types::application_json, response, call_headers)) {
      LOGERROR(NX::string_formater("request_login_apiuser error to call /rms/rs/login/trustedapp"));
      throw std::exception("fail to login api user");
		}

		NX::json_value json_response = NX::json_value::parse(response);
		const int statusCode = json_response.as_object().at(L"statusCode").as_int32();
		std::wstring statusMessage;
		if (json_response.as_object().has_field(L"message")) {
			statusMessage = json_response.as_object().at(L"message").as_string();
		}

		if (200 != statusCode) {
			LOGERROR(NX::string_formater(L"RS::request_login_apiuser failed (%d, %s)", statusCode, statusMessage.c_str()));
			if (400 == statusCode) {
				LOGDEBUG(parameters);
			}
			throw std::exception("fail to login api user");
		}

		login_output = json_response.as_object().at(L"extra");
		result = true;
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
		result = false;
	}

	return result;
}

