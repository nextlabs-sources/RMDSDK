

#include <Windows.h>
#include <assert.h>

#include <nudf\json.hpp>
#include <nudf\crypto.hpp>
#include <nudf\string.hpp>
#include <nudf\conversion.hpp>

#include "nxrmserv.hpp"
#include "serv.hpp"
#include "global.hpp"

#include "nxlfmt.h"
#include "profile.hpp"


using namespace NX::dbg;

extern rmserv* SERV;


user_membership::user_membership() : _type(0)
{
    ::InitializeCriticalSection(&_reserved_tokens_lock);
}

user_membership::user_membership(const std::wstring& s) : _type(0)
{
    ::InitializeCriticalSection(&_reserved_tokens_lock);
    deserialize(s);
}

user_membership::user_membership(int type, const std::wstring& id, const std::wstring& name, const std::wstring& token_group_name, const std::wstring& tenant_id, int project_id) : _type(type), _id(id), _name(name), _token_group_name(token_group_name), _tenant_id(tenant_id), _project_id(project_id)
{
    ::InitializeCriticalSection(&_reserved_tokens_lock);
    // std::transform(_id.begin(), _id.end(), _id.begin(), tolower);
    // std::transform(_token_group_name.begin(), _token_group_name.end(), _token_group_name.begin(), tolower);
    // std::transform(_tenant_id.begin(), _tenant_id.end(), _tenant_id.begin(), tolower);
}

user_membership::user_membership(const user_membership& other) : _type(other.get_type()), _id(other.get_id()), _name(other.get_name()), _token_group_name(other.get_token_group_name()), _tenant_id(other.get_tenant_id()), _project_id(other.get_project_id()), _reserved_tokens(other.get_reserved_tokens())
{
    ::InitializeCriticalSection(&_reserved_tokens_lock);
}

user_membership::user_membership(user_membership&& other) : _type(other.get_type()), _id(std::move(other._id)), _name(std::move(other._name)), _token_group_name(std::move(other._token_group_name)), _tenant_id(std::move(other._tenant_id)), _project_id(other.get_project_id()), _reserved_tokens(std::move(other._reserved_tokens))
{
    ::InitializeCriticalSection(&_reserved_tokens_lock);
}

user_membership::~user_membership()
{
    ::DeleteCriticalSection(&_reserved_tokens_lock);
}

user_membership& user_membership::operator = (const user_membership& other)
{
    if (this != &other) {
        _type = other.get_type();
        _id = other.get_id();
        _name = other.get_name();
        _token_group_name = other.get_token_group_name();
        _tenant_id = other.get_tenant_id();
        _project_id = other.get_project_id();
        _reserved_tokens = other.get_reserved_tokens();
    }
    return *this;
}

user_membership& user_membership::operator = (user_membership&& other)
{
    if (this != &other) {
        _type = other.get_type();
        _id = std::move(other._id);
        _name = std::move(other._name);
        _token_group_name = std::move(other._token_group_name);
        _tenant_id = std::move(other._tenant_id);
        _project_id = other.get_project_id();
        _reserved_tokens = std::move(other._reserved_tokens);
    }
    return *this;
}

void user_membership::clear()
{
    ::EnterCriticalSection(&_reserved_tokens_lock);
    _type = 0;
    _id.clear();
    _name.clear();
    _token_group_name.clear();
    _tenant_id.clear();
    _project_id = 0;
    while (!_reserved_tokens.empty()) {
        _reserved_tokens.pop_front();
    }
    ::LeaveCriticalSection(&_reserved_tokens_lock);
}


encrypt_token user_membership::pop_token()
{
    encrypt_token token;
    ::EnterCriticalSection(&_reserved_tokens_lock);
    if (!_reserved_tokens.empty()) {
        token = _reserved_tokens.front();
        _reserved_tokens.pop_front();
    }
    ::LeaveCriticalSection(&_reserved_tokens_lock);
    return std::move(token);
}

void user_membership::push_token(encrypt_token&& token)
{
    ::EnterCriticalSection(&_reserved_tokens_lock);
    if (token.valid()) {
        _reserved_tokens.push_back(std::move(token));
    }
    ::LeaveCriticalSection(&_reserved_tokens_lock);
}

void user_membership::push_token(std::vector<encrypt_token>& tokens)
{
    ::EnterCriticalSection(&_reserved_tokens_lock);
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        if (it->valid()) {
            _reserved_tokens.push_back(std::move(*it));
        }
    }
    ::LeaveCriticalSection(&_reserved_tokens_lock);
    tokens.clear();
}

std::wstring user_membership::serialize(bool with_agreement) const
{
    std::wstring s;
    try {
        NX::json_value v = NX::json_value::create_object();
        v.as_object()[L"id"] = NX::json_value(_id);
        v.as_object()[L"type"] = NX::json_value(_type);
        v.as_object()[L"name"] = NX::json_value(_name);
        v.as_object()[L"tokenGroupName"] = NX::json_value(_token_group_name);
        switch (_type) {
        case 0:
            v.as_object()[L"tenantId"] = NX::json_value(_tenant_id);
            break;
        case 1:
            v.as_object()[L"projectId"] = NX::json_value(_project_id);
            break;
        }
        if (with_agreement) {
            v.as_object()[L"agreement0"] = NX::json_value(NX::conversion::to_wstring(_agreement0));
            v.as_object()[L"agreement1"] = NX::json_value(NX::conversion::to_wstring(_agreement1));
        }
        s = v.serialize();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        s.clear();
    }
    return std::move(s);
}

bool user_membership::deserialize(const std::wstring& s)
{
    bool result = false;

    try {

        NX::json_value v = NX::json_value::parse(s);
        _id = v.as_object()[L"id"].as_string();
        _token_group_name = v.as_object()[L"tokenGroupName"].as_string();
        _type = v.as_object()[L"type"].as_int32();
        switch (_type) {
        case 0:
            if (v.as_object().has_field(L"tenantId")) {
                _tenant_id = v.as_object()[L"tenantId"].as_string();
            }
            break;
        case 1:
            if (v.as_object().has_field(L"projectId")) {
                _project_id = v.as_object()[L"projectId"].as_int32();
            }
            break;
        }
        //std::transform(_id.begin(), _id.end(), _id.begin(), tolower);
        //std::transform(_tenant_id.begin(), _tenant_id.end(), _tenant_id.begin(), tolower);
        if (v.as_object().has_field(L"name")) {
            _name = v.as_object()[L"name"].as_string();
        }
        if (v.as_object().has_field(L"agreement0")) {
            const std::wstring& ws_agreement0 = v.as_object()[L"agreement0"].as_string();
            _agreement0 = NX::utility::hex_string_to_buffer(ws_agreement0);
            if (_agreement0.size() != 128 && _agreement0.size() != 256) {
                _agreement0.clear();
            }
        }
        if (v.as_object().has_field(L"agreement1")) {
            const std::wstring& ws_agreement1 = v.as_object()[L"agreement1"].as_string();
            _agreement1 = NX::utility::hex_string_to_buffer(ws_agreement1);
            if (_agreement1.size() != 128 && _agreement1.size() != 256) {
                _agreement1.clear();
            }
        }

        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
    }

    return result;
}



user_preferences::user_preferences() : _secure_mode(KF_SERVER_SECURE_MODE)
{
}

user_preferences::user_preferences(const user_preferences& other) : _secure_mode(other.get_secure_mode()), _default_member(other.get_default_member()), _default_tenant(other.get_default_tenant())
{
}

user_preferences::user_preferences(user_preferences&& other) : _secure_mode(other.get_secure_mode()), _default_member(std::move(other._default_member)), _default_tenant(std::move(other._default_tenant))
{
}

user_preferences::~user_preferences()
{
}

user_preferences& user_preferences::operator = (const user_preferences& other)
{
    if (this != &other) {
        _secure_mode = other.get_secure_mode();
        _default_member = other.get_default_member();
        _default_tenant = other.get_default_tenant();
    }
    return *this;
}

user_preferences& user_preferences::operator = (user_preferences&& other)
{
    if (this != &other) {
        _secure_mode = other.get_secure_mode();
        _default_member = std::move(other._default_member);
        _default_tenant = std::move(other._default_tenant);
    }
    return *this;
}

void user_preferences::clear()
{
    _secure_mode = KF_SERVER_SECURE_MODE;
    _default_member.clear();
    _default_tenant.clear();
}



user_token::user_token()
{
}

user_token::user_token(const std::wstring& ticket, const NX::time::datetime& expire_time) : _expire_time(expire_time), _ticket(ticket)
{
}

user_token::user_token(const user_token& other) : _expire_time(other.get_expire_time()), _ticket(other.get_ticket())
{
}

user_token::user_token(user_token&& other) : _expire_time(std::move(other._expire_time)), _ticket(std::move(other._ticket))
{
    other.clear();
}

user_token::~user_token()
{
}

user_token& user_token::operator = (const user_token& other)
{
    if (this != &other) {
        _expire_time = other.get_expire_time();
        _ticket = other.get_ticket();
    }
    return *this;
}

user_token& user_token::operator = (user_token&& other)
{
    if (this != &other) {
        _ticket = std::move(other._ticket);
        _expire_time = std::move(other._expire_time);
    }
    return *this;
}

void user_token::clear()
{
    _expire_time.clear();
    _ticket.clear();
}

bool user_token::expired() const
{
    FILETIME ft = { 0, 0 };
    GetSystemTimeAsFileTime(&ft);
    NX::time::datetime dt_now(&ft);
    return (dt_now >= _expire_time);
}



user_profile::user_profile() : _id(-1)
{
	::InitializeCriticalSection(&_user_memberships_lock);
}

user_profile::user_profile(const user_profile& other) : _id(other.get_id()), _name(other.get_name()), _email(other.get_email()), _preferences(other.get_preferences()), _memberships(other.get_memberships())
{
	::InitializeCriticalSection(&_user_memberships_lock);
}

user_profile::user_profile(user_profile&& other) : _id(other.get_id()), _name(std::move(other._name)), _email(std::move(other._email)), _preferences(std::move(other._preferences)), _memberships(std::move(other._memberships))
{
	::InitializeCriticalSection(&_user_memberships_lock);
	other.clear();
}

user_profile::~user_profile()
{
	::DeleteCriticalSection(&_user_memberships_lock);
}

user_profile& user_profile::operator = (const user_profile& other)
{
    if (this != &other) {
        _id = other.get_id();
        _name = other.get_name();
        _email = other.get_email();
        _preferences = other.get_preferences();
        _memberships = other.get_memberships();
    }
    return *this;
}

user_profile& user_profile::operator = (user_profile&& other)
{
    if (this != &other) {
        _id = other.get_id();
        _name = std::move(other._name);
        _email = std::move(other._email);
        _preferences = std::move(other._preferences);
        _memberships = std::move(other._memberships);
    }
    return *this;
}

void user_profile::clear()
{
    _id = -1;
    _name.clear();
    _email.clear();
    _preferences.clear();
    _memberships.clear();
}

bool user_profile::create(const std::wstring& rs_login_result)
{
    bool ret = false;

    try {
        NX::json_value result = NX::json_value::parse(rs_login_result);
        if (200 == result[L"statusCode"].as_int32()) {
            const NX::json_value& login_result = result[L"extra"];
            ret = create(login_result);
        }
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    return ret;
}

bool user_profile::create(const NX::json_value& login_result)
{
	::EnterCriticalSection(&_user_memberships_lock);
	bool ret = false;
    clear();

    try {

        const __int64 token_ttl = login_result.as_object().at(L"ttl").as_int64();
        _token._ticket = login_result.as_object().at(L"ticket").as_string();
        _token._expire_time = NX::time::datetime::from_java_time(token_ttl);
        _id = login_result.as_object().at(L"userId").as_int64();
        _name = login_result.as_object().at(L"name").as_string();
        _email = login_result.as_object().at(L"email").as_string();
		_tenantid = login_result.as_object().at(L"tenantId").as_string();
		LOGDEBUG(NX::string_formater(L"user_profile::create: _tenantid=%s", _tenantid.c_str()));

#ifdef _DEBUG
        SYSTEMTIME current_st = { 0 };
        GetSystemTime(&current_st);
        NX::time::datetime current_time(&current_st);
        const std::wstring& str_current_time = current_time.serialize(true, false);
        const __int64 current_java_time = current_time.to_java_time();
        LOGDEBUG(NX::string_formater(L"Login Token TTL = %I64d", token_ttl));
        LOGDEBUG(NX::string_formater(L"    -> Current Time: %I64d (%s)", current_java_time, str_current_time.c_str()));
#endif
		m_attributes.clear();
		if (login_result.as_object().has_field(L"attributes")) {
			const NX::json_value &attr = login_result.as_object().at(L"attributes");
			const NX::json_object &attrObj = attr.as_object();
			for (NX::json_object::const_iterator it = attrObj.cbegin(); it != attrObj.cend(); ++it) {
				const NX::json_array &arr = it->second.as_array();
				if (!arr.empty()) {
					for (auto element : arr) {
						m_attributes.push_back(std::pair<std::wstring, std::wstring>(it->first, element.as_string()));
					}
				}
			}
		}

        const NX::json_value& memberships_result = login_result.as_object().at(L"memberships");
        std::for_each(memberships_result.as_array().begin(), memberships_result.as_array().end(), [&](const NX::json_value& item) {
            const std::wstring& member_id = item.as_object().at(L"id").as_string();
            std::wstring member_name;
            if (item.as_object().has_field(L"name")) {
                member_name = item.as_object().at(L"name").as_string();
            }

            const std::wstring& member_token_group_name = item.as_object().at(L"tokenGroupName").as_string();
            std::wstring member_tenant_id;
            if (item.as_object().has_field(L"tenantId")) {
                member_tenant_id = item.as_object().at(L"tenantId").as_string();
            }
            int member_project_id = -1;
            if (item.as_object().has_field(L"projectId")) {
                member_project_id = item.as_object().at(L"projectId").as_int32();
            }

            int member_type = item.as_object().at(L"type").as_int32();
            _memberships[member_id] = user_membership(member_type, member_id, member_name, member_token_group_name, member_tenant_id, member_project_id);
        });
        assert(!_memberships.empty());

        if (login_result.as_object().has_field(L"preferences")) {
            const NX::json_value& preferences_result = login_result.as_object().at(L"preferences");
            if (preferences_result.as_object().has_field(L"securityMode")) {
                _preferences._secure_mode = preferences_result.as_object().at(L"securityMode").as_int32();
            }
            if (preferences_result.as_object().has_field(L"defaultTenant")) {
                _preferences._default_tenant = preferences_result.as_object().at(L"defaultTenant").as_string();
            }
            if (preferences_result.as_object().has_field(L"defaultMember")) {
                _preferences._default_member = preferences_result.as_object().at(L"defaultMember").as_string();
            }
        }

        if (_preferences._default_member.empty()) {

            /*
                If no preferences (membership) is defined, use user itself membership as default
            */

            auto it = std::find_if(_memberships.begin(), _memberships.end(), [](const std::pair<std::wstring, user_membership>& member) {
                return member.second.get_name().empty();
            });

            if (it == _memberships.end()) {
                LOGWARNING(NX::string_formater(L"Profile without membership stands for user itself (user: %I64d, %s, %s)", _id, _name.c_str(), _email.c_str()));
                it = _memberships.begin();
            }
            _preferences._default_member = it->second.get_id();
            _preferences._default_tenant = it->second.get_tenant_id();
        }

        ret = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
        ret = false;
    }

	::LeaveCriticalSection(&_user_memberships_lock);
	return ret;
}

encrypt_token user_profile::pop_token(const std::wstring& membership_id)
{
	encrypt_token token;
	::EnterCriticalSection(&_user_memberships_lock);
	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
    if (pos != _memberships.end()) {
		token = (*pos).second.pop_token();
    }
	::LeaveCriticalSection(&_user_memberships_lock);

    return token;
}

void user_profile::push_token(const std::wstring& membership_id, encrypt_token&& token)
{
	::EnterCriticalSection(&_user_memberships_lock);
	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
    if (pos != _memberships.end()) {
        (*pos).second.push_token(std::move(token));
    }
	::LeaveCriticalSection(&_user_memberships_lock);
}

void user_profile::push_token(const std::wstring& membership_id, std::vector<encrypt_token>& tokens)
{
	::EnterCriticalSection(&_user_memberships_lock);
	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
    if (pos != _memberships.end()) {
        (*pos).second.push_token(tokens);
    }
	::LeaveCriticalSection(&_user_memberships_lock);
}

bool user_profile::is_me(const std::wstring& membership_id) const
{
	::EnterCriticalSection((LPCRITICAL_SECTION)&_user_memberships_lock);
	bool is_me = (_memberships.end() != std::find_if(_memberships.begin(), _memberships.end(), [&](const std::pair<std::wstring, user_membership>& item)->bool {return (0 == _wcsicmp(membership_id.c_str(), item.first.c_str())); }));
	::LeaveCriticalSection((LPCRITICAL_SECTION)&_user_memberships_lock);
	return is_me;
}

bool user_profile::has_enough_token(const std::wstring& membership_id)
{
	size_t size = 0;
	::EnterCriticalSection(&_user_memberships_lock);
	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
	if (pos != _memberships.end()) {
		size = (*pos).second.get_reserved_tokens().size();
	}
	::LeaveCriticalSection(&_user_memberships_lock);

	if (size > 32) return true;
	else return false;
}

std::vector<unsigned char> user_profile::get_agreement0(const std::wstring& membership_id)
{
	std::vector<unsigned char> agreement;
	::EnterCriticalSection(&_user_memberships_lock);
	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
	if (pos != _memberships.end()) {
		agreement = (*pos).second.get_agreement0();
	}
	::LeaveCriticalSection(&_user_memberships_lock);
	
	return agreement;
}

std::vector<unsigned char> user_profile::get_agreement1(const std::wstring& membership_id)
{
	std::vector<unsigned char> agreement;
	::EnterCriticalSection(&_user_memberships_lock);
	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
	if (pos != _memberships.end()) {
		agreement = (*pos).second.get_agreement1();
	}
	::LeaveCriticalSection(&_user_memberships_lock);

	return agreement;
}

std::deque<encrypt_token> user_profile::get_reserved_tokens(const std::wstring& membership_id)
{
	std::deque<encrypt_token> tokens;
	::EnterCriticalSection(&_user_memberships_lock);
	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
	if (pos != _memberships.end()) {
		tokens = (*pos).second.get_reserved_tokens();
	}
	::LeaveCriticalSection(&_user_memberships_lock);

	return tokens;
}

bool user_profile::save_member_reserved_tokens(const std::wstring& protected_profile_dir)
{
	::EnterCriticalSection(&_user_memberships_lock);

	for (auto it = _memberships.begin(); it != _memberships.end(); ++it) {

		const std::deque<encrypt_token>& reserved_tokens = (*it).second.get_reserved_tokens();

		if (!reserved_tokens.empty()) {
			// prepare files path
			std::wstring member_token_file(protected_profile_dir);
			member_token_file += L"\\";
			member_token_file += (*it).first;
			member_token_file += L".tokens.json";

			try {

				NX::json_value v = NX::json_value::create_object();
				v[L"tokens"] = NX::json_value::create_array();
				NX::json_array& tokens_array = v.as_object().at(L"tokens").as_array();
				for(auto token: reserved_tokens) {
					NX::json_value token_object = NX::json_value::create_object();
					token_object[L"ml"] = NX::json_value((int)token.get_token_level());
					token_object[L"id"] = NX::json_value(NX::conversion::to_wstring(token.get_token_id()));
					token_object[L"otp"] = NX::json_value(NX::conversion::to_wstring(token.get_token_otp()));
					token_object[L"value"] = NX::json_value(NX::conversion::to_wstring(token.get_token_value()));
					tokens_array.push_back(token_object);
				}

				std::wstring ws = v.serialize();
				GLOBAL.nt_generate_file(member_token_file, std::string(ws.begin(), ws.end()), true);

				//LOGDEBUG(NX::string_formater(L"%d reserved tokens for %s %s saved to config file %s", reserved_tokens.size(), member_id.c_str(), result ? L"has been" : L"failed to be", member_token_file.c_str()));
			}
			catch (const std::exception& e) {
				UNREFERENCED_PARAMETER(e);
			}
		}
	}

	::LeaveCriticalSection(&_user_memberships_lock);

	return true;
}

bool user_profile::save_member_reserved_tokens(const std::wstring& membership_id, const std::wstring& protected_profile_dir)
{
	::EnterCriticalSection(&_user_memberships_lock);

	std::map<std::wstring, user_membership>::iterator pos = _memberships.find(membership_id);
	if (pos != _memberships.end()) {
		const std::deque<encrypt_token>& reserved_tokens = (*pos).second.get_reserved_tokens();
		if (!reserved_tokens.empty()) {
			// prepare files path
			std::wstring member_token_file(protected_profile_dir);
			member_token_file += L"\\";
			member_token_file += membership_id;
			member_token_file += L".tokens.json";

			try {

				NX::json_value v = NX::json_value::create_object();
				v[L"tokens"] = NX::json_value::create_array();
				NX::json_array& tokens_array = v.as_object().at(L"tokens").as_array();
				for (auto token : reserved_tokens) {
					NX::json_value token_object = NX::json_value::create_object();
					token_object[L"ml"] = NX::json_value((int)token.get_token_level());
					token_object[L"id"] = NX::json_value(NX::conversion::to_wstring(token.get_token_id()));
					token_object[L"otp"] = NX::json_value(NX::conversion::to_wstring(token.get_token_otp()));
					token_object[L"value"] = NX::json_value(NX::conversion::to_wstring(token.get_token_value()));
					tokens_array.push_back(token_object);
				}

				std::wstring ws = v.serialize();
				GLOBAL.nt_generate_file(member_token_file, std::string(ws.begin(), ws.end()), true);

				//LOGDEBUG(NX::string_formater(L"%d reserved tokens for %s %s saved to config file %s", reserved_tokens.size(), member_id.c_str(), result ? L"has been" : L"failed to be", member_token_file.c_str()));
			}
			catch (const std::exception& e) {
				UNREFERENCED_PARAMETER(e);
			}
		}
	}

	::LeaveCriticalSection(&_user_memberships_lock);

	return true;
}

void user_profile::init_membership_info(const std::wstring& protected_profile_dir, rmsession* session)
{
	::EnterCriticalSection(&_user_memberships_lock);
	for (std::map<std::wstring, user_membership>::iterator it = _memberships.begin(); it != _memberships.end(); ++it) {
		user_membership existing_member_info;

		LOGDEBUG(NX::string_formater(L"Member present: %s", (*it).second.get_id().c_str()));

		if (load_member_info(protected_profile_dir, (*it).second.get_id(), existing_member_info) && !existing_member_info.empty() && !existing_member_info.get_agreement0().empty()) {
			// we already have agreement
			(*it).second.set_agreement0(existing_member_info.get_agreement0());
			(*it).second.set_agreement1(existing_member_info.get_agreement1());
			// finished
			continue;
		}

		std::vector<unsigned char> agreement0;
		std::vector<unsigned char> agreement1;
		if (!generate_member_agreements((*it).second.get_id().c_str(), agreement0, agreement1, session)) {
			LOGERROR(NX::string_formater(L"Fail to get agreement for member %s", (*it).second.get_id().c_str()));
			continue;
		}
		else {
			LOGDEBUG(NX::string_formater(L"  -> Retrieved agreement for member (%s) from server", (*it).second.get_id().c_str()));
#ifdef _DEBUG
			const std::wstring& str_agreement0 = NX::conversion::to_wstring(agreement0);
			//LOGDEBUG(NX::string_formater(L"     Agreement 0: %s", str_agreement0.c_str()));
			const std::wstring& str_agreement1 = NX::conversion::to_wstring(agreement1);
			//LOGDEBUG(NX::string_formater(L"     Agreement 1: %s", str_agreement1.c_str()));
#endif
		}

		(*it).second.set_agreement0(agreement0);
		(*it).second.set_agreement1(agreement1);
		if (!save_member_info(protected_profile_dir, (*it).second)) {
			LOGWARNING(NX::string_formater(L"Fail to save member profile (%s.json)", (*it).second.get_id().c_str()));
		}
		else {
			LOGDEBUG(NX::string_formater(L"  -> Saved new agreement for member (%s) to  file (%s.json)", (*it).second.get_id().c_str(), (*it).second.get_id().c_str()));
		}
	}
	::LeaveCriticalSection(&_user_memberships_lock);
}

void user_profile::init_membership_tokens(const std::wstring& protected_profile_dir, rmsession* session)
{
	::EnterCriticalSection(&_user_memberships_lock);
	for (std::map<std::wstring, user_membership>::iterator it = _memberships.begin(); it != _memberships.end(); ++it) {
		std::vector<encrypt_token> tokens;
		if (load_member_reserved_tokens(protected_profile_dir, (*it).second.get_id(), tokens) && !tokens.empty()) {
			(*it).second.push_token(tokens);
		}
		ensure_sufficient_tokens((*it).second.get_id(), session);

		// save tokens
		save_member_reserved_tokens((*it).second.get_id(), protected_profile_dir);
	}
	::LeaveCriticalSection(&_user_memberships_lock);
}

bool user_profile::load_member_info(const std::wstring& protected_profile_dir, const std::wstring& member_id, user_membership& member)
{
	// prepare files path
	std::wstring member_info_file(protected_profile_dir);
	member_info_file += L"\\";
	member_info_file += member_id;
	member_info_file += L".json";

	std::string content;
	if (!GLOBAL.nt_load_file(member_info_file, content)) {
		return false;
	}

	if (!member.deserialize(NX::conversion::utf8_to_utf16(content))) {
		return false;
	}

	return true;
}

bool user_profile::generate_member_agreements(const std::wstring& member_id, std::vector<unsigned char>& agreement0, std::vector<unsigned char>& agreement1, rmsession* session)
{
	bool result = false;
	std::wstring x509_encoded_pubkey;
	std::vector<std::wstring> certificates;

	if (!session->ensure_rs_executor()) {
		return false;
	}

	// For some unknown reason, the key generated here is not always
	// compatible with the RMS side when RMS is in FIPS mode.  When that
	// happens, the REST call inside request_sign_membership() would fail with
	// HTTP 400 - "Malformed request".  The workaround here is to retry
	// several times until the error disappears.
	//
	// This problem does not happen when RMS is in non-FIPS mode.
	const int sign_membership_max_failures = 50;
	int sign_membership_num_failures = 0;

	while (true) {
		NX::crypto::diffie_hellman_key_blob& keyblob = generate_dh_keyblob();
		if (keyblob.empty()) {
			LOGERROR(NX::string_formater(L"fail to generate Diffie-hellman key pair (%08X)", GetLastError()));
			return false;
		}

		if (!keyblob.encode_public_key(x509_encoded_pubkey)) {
			LOGERROR(NX::string_formater(L"fail to encode Diffie-hellman public key (%08X)", GetLastError()));
			return false;
		}

		if (session->get_rs_executor()->request_sign_membership(get_id(), member_id, get_token().get_ticket(), x509_encoded_pubkey, certificates)) {
			break;
		}

		LOGWARNING(NX::string_formater(L"fail to sign Diffie-hellman public key (%08X)", GetLastError()));
		if (++sign_membership_num_failures >= sign_membership_max_failures) {
			return false;
		}
		LOGWARNING(NX::string_formater(L"retrying to sign."));
	}

	if (certificates.empty()) {
		LOGWARNING(NX::string_formater(L"fail to get returned agreements from server"));
		return false;
	}

	if (!NX::crypto::diffie_hellman_key::get_y_from_cert(certificates[0], agreement0)) {
		agreement0.clear();
		LOGWARNING(NX::string_formater(L"fail to extract agreement 0 (%08X)", GetLastError()));
		return false;
	}

	if (certificates.size() > 1) {
		if (!NX::crypto::diffie_hellman_key::get_y_from_cert(certificates[1], agreement1)) {
			agreement1.clear();
			LOGWARNING(NX::string_formater(L"fail to extract agreement 1 (%08X)", GetLastError()));
		}
	}

	return true;
}

bool user_profile::save_member_info(const std::wstring& protected_profile_dir, const user_membership& member)
{
	const std::wstring& info = member.serialize(true);
	bool result = false;

	// prepare files path
	std::wstring member_info_file(protected_profile_dir);
	member_info_file += L"\\";
	member_info_file += member.get_id();
	member_info_file += L".json";

	std::string content = NX::conversion::utf16_to_utf8(info);
	if (!GLOBAL.nt_generate_file(member_info_file, content, true)) {
		return false;
	}

	return true;
}

bool user_profile::load_member_reserved_tokens(const std::wstring& protected_profile_dir, const std::wstring& member_id, std::vector<encrypt_token>& reserved_tokens)
{
	// prepare files path
	std::wstring member_token_file(protected_profile_dir);
	member_token_file += L"\\";
	member_token_file += member_id;
	member_token_file += L".tokens.json";

	std::string content;
	if (!GLOBAL.nt_load_file(member_token_file, content)) {
		LOGDETAIL(NX::string_formater(L"Tokens config file doesn't exist (%s)", member_token_file.c_str()));
		return false;
	}

	try {

		NX::json_value v = NX::json_value::parse(content);
		const NX::json_array& tokens_array = v.as_object().at(L"tokens").as_array();
		const size_t count = tokens_array.size();

		//LOGDEBUG(NX::string_formater(L"Load %d reserved tokens for %s from config file %s", count, member_id.c_str(), member_token_file.c_str()));

		for (size_t i = 0; i < count; i++) {
			const unsigned long token_level = tokens_array[i].as_object().at(L"ml").as_int32();
			const std::wstring& token_id = tokens_array[i].as_object().at(L"id").as_string();
			std::wstring token_otp = L"";
			if (tokens_array[i].as_object().has_field(L"otp"))
				token_otp = tokens_array[i].as_object().at(L"otp").as_string();
			const std::wstring& token_value = tokens_array[i].as_object().at(L"value").as_string();
			encrypt_token   token(token_level, NX::utility::hex_string_to_buffer(token_id), NX::utility::hex_string_to_buffer(token_otp), NX::utility::hex_string_to_buffer(token_value));
			if (token.valid()) {
				reserved_tokens.push_back(token);
			}
		}
	}
	catch (const std::exception& e) {
		UNREFERENCED_PARAMETER(e);
	}

	return true;
}

void user_profile::ensure_sufficient_tokens(const std::wstring& membership_id, rmsession* session)
{
	if (has_enough_token(membership_id)) {
		return;
	}

	LOGDETAIL(NX::string_formater(L"Not enough tokens available for member %s, try to get more ...", membership_id));

	std::vector<encrypt_token> tokens;

	if (!session->ensure_rs_executor()) {
		return;
	}

	if (!session->get_rs_executor()->request_generate_tokens(get_id(), membership_id, get_token().get_ticket(), get_agreement0(membership_id), 50, tokens)) {
		LOGDETAIL(NX::string_formater(L"  -> Fail to get token"));
		return;
	}

	LOGDETAIL(NX::string_formater(L"  -> Get %d tokens", tokens.size()));

	if (!tokens.empty()) {
		push_token(membership_id, tokens);
	}
}

std::vector<std::pair<std::wstring, std::wstring>> user_profile::get_userAttributes() 
{
	::EnterCriticalSection(&_user_memberships_lock);
	std::vector<std::pair<std::wstring, std::wstring>> attrs = m_attributes;
	::LeaveCriticalSection(&_user_memberships_lock);

	return attrs;
}