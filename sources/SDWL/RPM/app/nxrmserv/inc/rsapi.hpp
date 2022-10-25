

#ifndef __NXSERV_RESTFUL_API_HPP__
#define __NXSERV_RESTFUL_API_HPP__

#include <string>
#include <memory>
#include <vector>

#include <nudf\uri.hpp>
#include <nudf\http_client.hpp>

#include "key_manager.hpp"

namespace NX {

namespace ROUTER {

bool query_tenant_info(const std::wstring& router_url, const std::wstring& tenant_id, std::wstring& rms_server_url);

}   // NX::ROUTER

namespace RS {


class executor
{
public:
    executor();
    virtual ~executor();

    bool initialize(const std::wstring& url);
    void clear();

    inline bool empty() const { return (_client == nullptr); }
    inline bool is_connection_good() const { return _good_connection; }

    bool request_sign_membership(__int64 user_id,
        const std::wstring& member_id,
        const std::wstring& ticket,
        const std::wstring& encoded_public_key,
        std::vector<std::wstring>& certificates
        );

    bool request_generate_tokens(__int64 user_id,
        const std::wstring& member_id,
        const std::wstring& ticket,
        const std::vector<UCHAR>& agreement,
        int count,
        std::vector<encrypt_token>& tokens
        );

    bool request_retrieve_decrypt_token(unsigned long process_id, __int64 user_id,
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
        _Out_opt_ int* returned_code
        );
	
	bool request_heartbeat_v1(__int64 user_id,
		const std::wstring& tenant_id,
		const std::wstring& ticket,
		const std::vector<std::pair<std::wstring, std::wstring>>& objects_request,
		std::vector<std::pair<std::wstring, std::pair<std::wstring, std::wstring>>>& objects_returned);

    bool request_heartbeat(__int64 user_id,
        const std::wstring& tenant_id,
        const std::wstring& ticket,
		std::map<std::wstring, std::wstring> &policy_map,
		std::wstring &policyData, unsigned int* heartbeatFrequency,
		std::map<std::wstring, std::wstring> &tokengroupRT_map,
		std::wstring &tokengroupRT_data);

    bool request_post_sharing_trasition(__int64 user_id, const std::wstring& ticket, const std::wstring& checksum, const std::wstring& shared_doc_info);

    bool request_upgrade(const std::wstring& tenant_id, const std::wstring& current_version, _Out_ std::wstring& new_version, _Out_ std::wstring& download_url, _Out_ std::wstring& checksum);

    bool request_log_activities(const __int64 user_id, const std::wstring& ticket, const std::vector<unsigned char>& logdata);

	bool request_classify_file(const __int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, const std::wstring &duid, const std::wstring &parameters);

	bool request_reclassify_project_file(const __int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, const std::wstring &duid, const int projectid, const std::wstring &parameters);

	bool request_reclassify_file(const __int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, const std::wstring &duid, const std::wstring &parameters);

	bool get_tenant_preference(__int64 user_id, const std::wstring& ticket, const std::wstring& tenant_id, int* client_heartbeat, bool* adhoc, bool* dapenable, std::wstring& system_bucket_name, std::wstring &icenetUrl);
		
	bool request_query_nonce(const __int64 app_id, const std::wstring& app_key, std::wstring& nonce);

	bool request_login_apiuser(const __int64 app_id, const std::wstring& app_key, const std::wstring& email, const std::wstring& private_cert_path, NX::json_value &login_output);

protected:
    bool execute(const std::wstring& service_method, const std::wstring& service_path, const std::wstring& parameters, const std::wstring& parameters_type, std::wstring& response);
    bool execute(const std::wstring& service_method, const std::wstring& service_path, const std::vector<unsigned char>& parameters, const std::wstring& parameters_type, const __int64 user_id, const std::wstring& ticket, const __int32 platformid, std::wstring& response);

	bool execute(const std::wstring& service_method, const std::wstring& service_path, const std::wstring& parameters, const std::wstring& parameters_type, std::wstring& response, 
		const std::vector<std::pair<std::wstring, std::wstring>>& call_headers);

protected:
    void on_network_recover();
    void on_network_error(long error_code, const char* message);

private:
    std::shared_ptr<NX::http::client> _client;
    bool            _good_connection;
    std::wstring    _server_url;
};

}   // NX::RS
}   // NX

#endif