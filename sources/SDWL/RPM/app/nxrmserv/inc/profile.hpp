

#pragma once
#ifndef __NXRM_PROFILE_HPP__
#define __NXRM_PROFILE_HPP__


#include <string>
#include <vector>
#include <queue>
#include <map>

#include <nudf\time.hpp>
#include <nudf\json.hpp>

#include "key_manager.hpp"


class user_profile;
class user_login_session;
class rmsession;

class user_membership
{
public:
    user_membership();
    user_membership(const std::wstring& s);
    user_membership(int type, const std::wstring& id, const std::wstring& name, const std::wstring& token_group_name, const std::wstring& tenant_id, int project_id);
    user_membership(const user_membership& other);
    user_membership(user_membership&& other);
    virtual ~user_membership();

    user_membership& operator = (const user_membership& other);
    user_membership& operator = (user_membership&& other);
    void clear();

    inline bool empty() const { return _id.empty(); }
    inline int get_type() const { return _type; }
    inline const std::wstring& get_id() const { return _id; }
    inline const std::wstring& get_name() const { return _name; }
    inline const std::wstring& get_token_group_name() const { return _token_group_name; }
    inline const std::wstring& get_tenant_id() const { return _tenant_id; }
    inline int get_project_id() const { return _project_id; }
    inline const std::deque<encrypt_token>& get_reserved_tokens() const { return _reserved_tokens; }

    inline const std::vector<unsigned char>& get_agreement0() const { return _agreement0; }
    inline const std::vector<unsigned char>& get_agreement1() const { return _agreement1; }
    inline void set_agreement0(const std::vector<unsigned char>& agreement) { _agreement0 = agreement; }
    inline void set_agreement1(const std::vector<unsigned char>& agreement) { _agreement1 = agreement; }

    std::wstring serialize(bool with_agreement = false) const;
    bool deserialize(const std::wstring& s);

    encrypt_token pop_token();
    void push_token(encrypt_token&& token);
    void push_token(std::vector<encrypt_token>& tokens);

private:
    int             _type;
    std::wstring    _id;
    std::wstring    _name;
    std::wstring    _token_group_name;
    std::wstring    _tenant_id;         // valid if _type = 0
    int             _project_id;        // valid if _type = 1
    std::vector<unsigned char> _agreement0;
    std::vector<unsigned char> _agreement1;
    std::deque<encrypt_token>  _reserved_tokens;
    CRITICAL_SECTION           _reserved_tokens_lock;

    friend class user_profile;
};

class user_preferences
{
public:
    user_preferences();
    user_preferences(const user_preferences& other);
    user_preferences(user_preferences&& other);
    virtual ~user_preferences();

    user_preferences& operator = (const user_preferences& other);
    user_preferences& operator = (user_preferences&& other);
    void clear();

    inline bool empty() const { return _default_member.empty(); }
    inline int get_secure_mode() const { return _secure_mode; }
    inline const std::wstring& get_default_member() const { return _default_member; }
    inline const std::wstring& get_default_tenant() const { return _default_tenant; }

private:
    std::wstring    _default_tenant;
    std::wstring    _default_member;
    int             _secure_mode;

    friend class user_profile;
    friend class rmsession;
};

class user_token
{
public:
    user_token();
    user_token(const std::wstring& ticket, const NX::time::datetime& expire_time);
    user_token(const user_token& other);
    user_token(user_token&& other);
    virtual ~user_token();

    user_token& operator = (const user_token& other);
    user_token& operator = (user_token&& other);
    void clear();
    bool expired() const;

    inline bool empty() const { return _ticket.empty(); }
    inline const std::wstring& get_ticket() const { return _ticket; }
    inline const NX::time::datetime& get_expire_time() const { return _expire_time; }

private:
    std::wstring        _ticket;
    NX::time::datetime  _expire_time;
    friend class user_profile;
};

class user_profile
{
public:
    user_profile();
    user_profile(const user_profile& other);
    user_profile(user_profile&& other);
    virtual ~user_profile();

    user_profile& operator = (const user_profile& other);
    user_profile& operator = (user_profile&& other);
    void clear();

    inline bool empty() const { return (-1 == _id); }
    inline __int64 get_id() const { return _id; }
    inline const std::wstring& get_name() const { return _name; }
    inline const std::wstring& get_email() const { return _email; }
	const std::wstring& get_default_tenantId() const { return _tenantid; }
    inline const user_token& get_token() const { return _token; }
    inline const user_preferences& get_preferences() const { return _preferences; }
	std::vector<std::pair<std::wstring, std::wstring>> get_userAttributes();

    bool create(const std::wstring& rs_login_result);
    bool create(const NX::json_value& login_result);

    encrypt_token pop_token(const std::wstring& membership_id);
    void push_token(const std::wstring& membership_id, encrypt_token&& token);
    void push_token(const std::wstring& membership_id, std::vector<encrypt_token>& tokens);
	bool has_enough_token(const std::wstring& membership_id);
	std::vector<unsigned char> get_agreement0(const std::wstring& membership_id);
	std::vector<unsigned char> get_agreement1(const std::wstring& membership_id);
	std::deque<encrypt_token> get_reserved_tokens(const std::wstring& membership_id);
	bool is_me(const std::wstring& membership_id) const;

	bool save_member_reserved_tokens(const std::wstring& protected_profile_dir);
	bool save_member_reserved_tokens(const std::wstring& membership_id, const std::wstring& protected_profile_dir);

	void init_membership_info(const std::wstring& protected_profile_dir, rmsession* session);
	void init_membership_tokens(const std::wstring& protected_profile_dir, rmsession* session);
	bool load_member_info(const std::wstring& protected_profile_dir, const std::wstring& member_id, user_membership& member);
	bool generate_member_agreements(const std::wstring& member_id, std::vector<unsigned char>& agreement0, std::vector<unsigned char>& agreement1, rmsession* session);
	bool save_member_info(const std::wstring& protected_profile_dir, const user_membership& member);
	bool load_member_reserved_tokens(const std::wstring& protected_profile_dir, const std::wstring& member_id, std::vector<encrypt_token>& reserved_tokens);
	void ensure_sufficient_tokens(const std::wstring& membership_id, rmsession* session);

private:
	inline const std::map<std::wstring, user_membership>& get_memberships() const { return _memberships; }
	//inline std::map<std::wstring, user_membership>& get_memberships() { return _memberships; }
	
	__int64             _id;
    std::wstring        _name;
    std::wstring        _email;
	std::wstring        _tenantid;
    user_token          _token;
    user_preferences    _preferences;
    std::map<std::wstring, user_membership>   _memberships;
	std::vector<std::pair<std::wstring, std::wstring>> m_attributes;
	CRITICAL_SECTION           _user_memberships_lock;
	
	friend class rmsession;
};



#endif