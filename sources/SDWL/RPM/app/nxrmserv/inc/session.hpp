

#pragma once
#ifndef __NXRM_SESSION_HPP__
#define __NXRM_SESSION_HPP__

#include <map>
#include <memory>
#include <thread>
#include <boost/noncopyable.hpp>

#include <nudf\json.hpp>
#include <nudf\timer.hpp>
#include <nudf\rwlock.hpp>
#include <nudf\winutil.hpp>

#include "nxlogdb.hpp"

#include "nxlfmthlp.hpp"
#include "profile.hpp"
#include "rsapi.hpp"
#include "uploadserv.hpp"
#include "logactivity.hpp"
#include "config.hpp"
#include "policy.hpp"

//For Bug 58592 - RPMGetRights(): cannot get file watermark info
#include "..\..\..\..\..\..\sources\SDWL\SDWRmcLib\PDP\pdp.h"

class winsession;

typedef enum _LOGIN_INFO {
	RPMFolder = 0,
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	sanctuaryFolder,
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	sdklibFolder,
	routerAddr,	
	tenantId,
	workingFolder,
	tempFolder,
    myFolder
} LOGIN_INFO;

typedef enum _RECLASSIFY_TYPE {
	CLASSIFY_NEW_FILE = 0,
	RECLASSIFY_TENANT_FILE,
	RECLASSIFY_PROJECT_FILE
} RECLASSIFY_TYPE;

class rmappinstance : boost::noncopyable
{
public:
    rmappinstance();
    rmappinstance(const std::wstring& app_path);
    ~rmappinstance();

    virtual void run(unsigned long session_id);
    virtual void quit(unsigned long wait_time = 3000);  // by default, wait 3 seconds
    void kill();

    void attach(unsigned long process_id);
    void detach();


    inline bool is_running() const { return (NULL != _process_handle); }
    inline HANDLE get_process_handle() const { return _process_handle; }
    inline unsigned long get_process_id() const { return _process_id; }
    inline const std::wstring& get_port_name() const { return _port_name; }


    
protected:
    HWND find_target_hwnd(bool wait, unsigned long timeout);

private:
    std::wstring    _image_path;
    std::wstring    _port_name;
    HANDLE          _process_handle;
    unsigned long   _process_id;
};

class rmappmanager
{
public:
    rmappmanager();
    ~rmappmanager();

    void start(unsigned long session_id, const std::wstring& app_path);
    void shutdown();

    bool send_pilicy_changed_notification(const std::wstring& policyId);
    bool send_popup_notification(const std::wstring& message, bool isJsonMessage = false);
    bool send_logon_notification();
    bool send_status_changed_notification();
    bool send_quit_notification();

    bool send_nxlfile_rights_notification(const std::wstring& strJsonMessage);
    bool send_process_exit_notificatoin(const std::wstring& strJsonMessage);

    inline const std::shared_ptr<rmappinstance>& get_instance() const { return _instance; }
    inline std::shared_ptr<rmappinstance>& get_instance() { return _instance; }
    inline unsigned long get_session_id() const { return _session_id; }
    inline HANDLE get_shutdown_event() const { return _shutdown_event; }

protected:
    static void daemon(rmappmanager* manager);
    void close_existing_app(unsigned long wait_time, bool force);
    bool notify(const std::string& message);
    
private:
    std::shared_ptr<rmappinstance> _instance;
    HANDLE          _shutdown_event;
    std::thread     _daemon_thread;
    unsigned long   _session_id;
};

class rmsession_timer : public NX::timer
{
public:
    rmsession_timer();
    virtual ~rmsession_timer();

    void force_heartbeat();
    void force_uploadlog();
    void force_checkupdate();
    void force_all();

protected:
    virtual void on_timer();
    virtual void on_stop();
    void on_heartbeat();
    void on_log();
    void on_check_upgrade();

private:
    rmsession*      _rmsession;
    unsigned long   _heartbeat_count;
    unsigned long   _log_count;
    unsigned long   _checkupdate_count;
    unsigned long   _force_flags;

    friend class rmsession;
};


#define EVAL_CACHE_SIZE     64

class eval_cache : public boost::noncopyable
{
public:
    eval_cache();
    ~eval_cache();

    void push(std::shared_ptr<eval_result>& sp);
    void clear();
    std::shared_ptr<eval_result> get(unsigned __int64 id) const;

private:
    std::list<std::shared_ptr<eval_result>> _list;
    mutable NX::utility::CRwLock _lock;
};


class decrypt_token_cache
{
public:
    decrypt_token_cache();
    ~decrypt_token_cache();

	std::vector<unsigned char> find(const std::vector<unsigned char>& id, unsigned long level) const;
	std::vector<unsigned char> find(const std::vector<unsigned char>& id, unsigned long level, time_t &ttl, std::vector<unsigned char>& otp) const;
    void insert(const std::vector<unsigned char>& id, unsigned long level, const std::vector<unsigned char>& token, time_t ttl, const std::vector<unsigned char>& otp);
	void insert(const std::vector<unsigned char>& id, unsigned long level, const std::vector<unsigned char>& token);
	void clear();
	void delete_token(const std::vector<unsigned char>& id, unsigned long level);

public:
    class token_key
    {
    public:
        token_key(const std::vector<unsigned char>& id, unsigned long level) : _level(level), _id(id), _ttl(0), _otp() {}
		token_key(const std::vector<unsigned char>& id, unsigned long level, time_t ttl) : _level(level), _id(id), _ttl(ttl), _otp() {}
		token_key(const std::vector<unsigned char>& id, unsigned long level, time_t ttl, const std::vector<unsigned char>& otp) : _level(level), _id(id), _ttl(ttl), _otp(otp) {}
		token_key(const token_key& other) : _level(other._level), _ttl(other._ttl), _id(other._id), _otp(other._otp) {}
        token_key(token_key&& other) : _level(std::move(other._level)), _ttl(std::move(other._ttl)), _id(std::move(other._id)), _otp(std::move(other._otp)) {}
        ~token_key() {}
        inline const std::vector<unsigned char>& get_id() const { return _id; }
		inline const std::vector<unsigned char>& get_otp() const { return _otp; }
		inline unsigned long get_level() const { return _level; }
		inline time_t const get_ttl() const { return _ttl; }
		token_key& operator = (const token_key& other)
        {
            if (this != &other) {
                _level = other._level;
                _id = other._id;
				_ttl = other._ttl;
				_otp = other._otp;
			}
            return *this;
        }
        token_key& operator = (token_key&& other)
        {
            if (this != &other) {
                _level = std::move(other._level);
				_ttl = std::move(other._ttl);
				_id = std::move(other._id);
				_otp = std::move(other._otp);
			}
            return *this;
        }
        bool operator == (const token_key& other) const
        {
            if (this == &other) {
                return true;
            }
            return ((_level == other._level) && (_id == other._id));
        }
        bool operator < (const token_key& other) const
        {
            if (this == &other) {
                return false;
            }
            if (_id.size() < other._id.size()) {
                return true;
            }
            else if (_id.size() > other._id.size()) {
                return false;
            }
            else {
                int cmpresult = memcmp(_id.data(), other._id.data(), _id.size());
                if (cmpresult < 0) {
                    return true;
                }
                else if (cmpresult > 0) {
                    return false;
                }
                else {
                    return (_level < other._level) ? true : false;
                }
            }
        }

    private:
        unsigned long _level;
		time_t _ttl;
		std::vector<unsigned char> _id;
		std::vector<unsigned char> _otp;
	};

private:
    std::map<token_key, std::vector<unsigned char>> _map;
	std::map<token_key, std::vector<unsigned char>> _map_otp;
	mutable NX::utility::CRwLock _lock;

public:
	inline const std::map<token_key, std::vector<unsigned char>> & getTokenMap() const { return _map; }
};

#define CACHE_LOGIN_STATUS_FILE     L"loginStatus.json"
#define POLICY_BUNDLE_FILE          L"policy_bundle.json"
#define CLIENT_CONFIG_FILE          L"client_config.json"
#define CLIENT_LOCAL_CONFIG_FILE    L"client_local_config.json"
#define CLASSIFY_CONFIG_FILE        L"classify_config.json"
#define WATERMARK_CONFIG_FILE       L"watermark_config.json"
#define POLICY_BUNDLE_FILE_2        L"policy_bundle_2.json"
#define TOKENGROUP_RESOURCETYPE_FILE        L"tokengroup_resourcetype.json"
#define LOCKED_DUIDS_FILE           L"locked_duids.json"
#define CLIENT_DIR_CONFIG_FILE      L"client_dir_config.json"
#define RPM_DIR_CONFIG_FILE         L"rpm_dir_config.json"
#define SANC_DIR_CONFIG_FILE         L"sanc_dir_config.json"
#define APP_WHITELIST_CONFIG_FILE   L"app_whitelist_config.json"

#define TOKENLIST_ELEMENT_NAME					L"TokenList"
#define TOKENID_ELEMENT_NAME                    L"Duid"
#define TOKENKEY_ELEMENT_NAME                   L"Key"
#define TOKENOTP_ELEMENT_NAME                   L"Otp"
#define TOKENTTL_ELEMENT_NAME                   L"Ttl"
#define TOKENMAINTAINLEVEL_ELEMENT_NAME         L"Ml"

#define LOCKEDDUID_ELEMENT_NAME					L"LockedDUIDs"

class rmsession
{
public:
    rmsession();
    rmsession(winsession* p);
    virtual ~rmsession();

    inline bool is_logged_on() const { return (!_profile.empty() && !_profile.get_token().empty()); }
    inline const user_profile& get_profile() const { return _profile; }
    inline user_profile& get_profile() { return _profile; }
    inline rmappmanager& get_app_manager() { return _appmanager; }
    inline void set_notify_window(HWND h) { _notify_hwnd = h; }
    inline rmsession_timer& get_timer() { return _timer; }
    inline const std::wstring& get_tenant_id() const { return _tenant_id; }
	inline const std::wstring& get_rms_url() { ensure_rs_executor(); return _rms_url; }
    inline const std::wstring& get_temp_profile_dir() const { return _temp_profile_dir; }
    inline const std::wstring& get_protected_profile_dir() const { return _protected_profile_dir; }

    // cache
    inline const eval_cache& get_eval_cache() const { return _eval_cache; }
    inline decrypt_token_cache& get_token_cache() { return _token_cache; }
	inline const std::vector<std::pair<std::wstring, std::wstring>>& get_callback_cmds() const { return _logon_callbacks; }
	inline void clear_callback_cmds() { _logon_callbacks.clear(); }
	inline void add_callback_cmds(std::pair<std::wstring, std::wstring> cmd) { _logon_callbacks.insert(_logon_callbacks.end(), cmd); }
	encrypt_token popup_new_token(const std::wstring & membershipid);
	encrypt_token find_file_token(const std::wstring & duid, time_t &ttl);
	bool insert_cached_token(const NX::json_value & cached_token);
	bool delete_user_cached_tokens();
	bool delete_file_token(const NX::json_value & cached_token);
	bool delete_file_token(const std::wstring duid, unsigned long level);

    inline const client_config& get_client_config() const { return _client_config; }
	inline client_config& get_client_config() { return _client_config; }
    inline const client_local_config& get_local_config() const { return _local_config; }
    inline const classify_config& get_classify_config() const { return _classify_config; }
    inline const watermark_config& get_watermark_config() const { return _watermark_config; }

    inline bool get_dwm_status() const { return _dwm_enabled; }
    inline void set_dwm_status(bool enabled) { _dwm_enabled = enabled; }

    void initialize();
    void clear();

    std::shared_ptr<policy_bundle> get_policy_bundle() const;
    void reset_policy_bundle(std::shared_ptr<policy_bundle> sp = std::shared_ptr<policy_bundle>(nullptr));

    // user management
    bool query_rms_url();
    bool query_token(_In_ unsigned long process_id, _In_ const std::wstring& owner_id, _In_ const std::vector<unsigned char>& agreement, _In_ const std::vector<unsigned char>& token_id, _In_ int protection_type, _In_ const std::wstring& policy, _In_ const std::wstring& tags, _In_ const unsigned long token_level, _Out_ std::vector<unsigned char>& token_value, _Out_opt_ bool* cached = nullptr, _Out_opt_ bool* access_denied = nullptr);
    bool logon(const NX::json_value& logon_data);
    void logout(bool keep_logon_data = false);
	bool update_client_config(const std::wstring& clientid);
	bool update_client_dir_config(const std::wstring& filepath, int no=0);
	bool update_app_whitelist_config(const std::set<std::wstring>& registeredApps);
	bool update_app_whitelist_config(const std::map<std::wstring, std::vector<unsigned char>>& trustedApps);
	int get_client_dir_info(std::wstring& router, std::wstring& tenant, std::wstring& workingfolder, std::wstring& tempfolder, std::wstring& sdklibfolder);

    // rights management
    bool rights_evaluate(const std::wstring& file, unsigned long process_id, unsigned long session_id, _Out_ unsigned __int64* evaluate_id, _Out_ unsigned __int64* rights,  _Out_opt_ unsigned __int64* custom_rights, bool bcheckowner = true);
	
	bool rights_evaluate_with_watermark(const std::wstring& file, unsigned long process_id, unsigned long session_id, 
		_Out_ unsigned __int64* evaluate_id, _Out_ unsigned __int64* rights, _Out_opt_ unsigned __int64* custom_rights, 
		_Out_ std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &vecRightsWatermarks, bool bcheckowner = true);

	std::wstring file_rights_watermark_to_json(const std::wstring& strPath, uint64_t u64Rights,
		const std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& vecRightsWatermarks);
	
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	bool trust_evaluate(unsigned long process_id, _Out_ bool* trusted);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	bool read_file_tags(const std::wstring& file, unsigned long process_id, unsigned long session_id, std::wstring &tags);

    // Log activity
    void log_activity(const activity_record& record);
	void log_activity2(const activity_record& record);
	void audit_activity(int user_operation,
        int result,
        const std::wstring& duid,
        const std::wstring& owner,
        const std::wstring& app_name,
        const std::wstring& file_path);
    bool export_audit_log();
    bool export_audit_query_result(std::wstring& outfile);
	void request_add_activity_log(const NX::json_value& log);

	bool log_metadata(const metadata_record& record, const int option);
	bool request_add_metadata(const NX::json_value& log);
	void request_lock_localfile(const NX::json_value& params);

	bool on_set_user_attributes(const NX::json_value& json);

    // upload data
    bool upload_data(const upload_object& object);
    bool upload_sharing_transition(unsigned long process_id, int shareType, const std::wstring& file, const std::wstring& sharing_info);
    bool upload_activity_log(const std::vector<unsigned char>& logs);
	bool upload_metadata_log(const std::vector<std::string>& logs, std::vector<std::string> &retrylogs);
	//bool update_policy(const std::wstring& serial_number, const std::wstring& content);
	//bool get_policybundle(const std::wstring& tokenGroupName, std::string& policyBundle);
	bool update_policy(std::map<std::wstring, std::wstring> &policy_map, std::wstring &policyData);
	bool get_tokengroup_resourcetype(const std::wstring& tokenGroupName, std::string& resourcetype);
	bool update_tokengroup_resourcetype(std::map<std::wstring, std::wstring> &tokengroup_resoucetype_map, std::wstring &tokengroup_resoucetype_data);
	bool update_pdp_url(std::wstring &icenetUrl);
	bool update_dap_configfile(std::wstring &rms_url);
	bool update_dap_configfile2(std::wstring &rms_url);
	bool change_dap_filename(bool dap_enable);

	// update dirs
	bool insert_directory_action(const std::wstring& filepath);
	bool remove_directory_action(const std::wstring& filepath);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	bool insert_sanctuary_directory_action(const std::wstring& filepath);
	bool remove_sanctuary_directory_action(const std::wstring& filepath);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	bool delete_nxl_file(const std::wstring& filename);
	unsigned long request_delete_file_token(const std::wstring& filename);
	unsigned int is_nxl_file(const std::wstring& filename);

	bool ensure_rs_executor(bool enable=false);
	inline NX::RS::executor* get_rs_executor() { return &_rs_executor; }

	bool save_user_cached_tokens();
	bool load_user_cached_tokens();

	bool save_locked_duids();
	bool load_locked_duids();

	// on_timer
	void on_heartbeat_v1();
    void on_heartbeat();
    void on_uploadlog();
    void on_checkupgrade();
	bool is_ad_hoc() { return _ad_hoc; };

	void block_notify(const std::wstring& file, const std::wstring& appname);

	bool pre_rights_evaluate(const std::wstring& file, unsigned long process_id, unsigned long session_id);
	bool get_document_eval_info(_In_ const unsigned long process_id, _In_ const std::wstring file, _Out_ std::shared_ptr<policy_bundle>& policy, 
		_Out_ std::string &adpolicy, _Out_ std::string &tags, _Out_ std::string &info, _Out_ std::string &duid, _Out_ std::string &creator, _Out_ std::string &tokengroup, _Out_ DWORD &attributes);

protected:
    bool inter_logon(const NX::json_value& logon_data, bool cached);
    bool init_from_cache();
    //bool ensure_rs_executor();
	std::wstring get_user_cached_tokens_file();

    bool load_policy_bundle();
	bool load_policy_bundle_2();
	// use policy bundle tokengroup to parse the resource type, This load_tokengroup_resourcetype() must be called after load_policy_bundle_2()
	bool load_tokengroup_resourcetype();
    bool load_client_config();
    bool load_local_config();
    bool load_classify_config();
    bool load_watermark_config();
	bool load_client_dir_config();
	bool load_app_whitelist_config();

    bool update_policy(const std::wstring& serial_number, const std::wstring& content);
    bool update_client_config(const std::wstring& serial_number, const std::wstring& content);
    bool update_classify_config(const std::wstring& serial_number, const std::wstring& content);
    bool update_watermark_config(const std::wstring& serial_number, const std::wstring& content);

    bool get_document_eval_info(_In_ HANDLE h, _In_ NX::NXL::document_context& context, _In_ const unsigned char* file_token,  _Out_ eval_data& ed, _Out_ std::shared_ptr<policy_bundle>& policy);

    std::wstring normalize_image_text(const std::wstring& expandable_text);
    bool generate_watermark_image();
    bool does_watermark_image_exist();

	void adhoc_obligation_to_watermark(unsigned long long rights , const std::string& policy, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>& rightsAndWatermarks);

	std::wstring get_creo_xtop_apppath();

	static void pdp_app_preload(rmsession* pSession);

	bool is_server_mode();

protected:
    void init_dirs();
    void init_classify_config(const std::string& content);
    void init_memberships();
    bool init_logdb();
    void clear_logdb();

private:
    winsession*         _winsession;
    rmappmanager        _appmanager;
    NXLOGDB::nxlogdb    _audit_db;
    NX::RS::executor    _rs_executor;
    activity_logger     _actlogger;
	metadata_logger     _metalogger;
	upload_serv         _upload_serv;
    user_profile        _profile;
    std::wstring        _tenant_id;
    std::wstring        _rms_url;
    std::wstring        _temp_profile_dir;
    std::wstring        _protected_profile_dir;
    HWND                _notify_hwnd;
    rmsession_timer     _timer;
    std::shared_ptr<policy_bundle> _policy_bundle;
    mutable NX::utility::CRwLock _policy_bundle_lock;
	mutable NX::utility::CRwLock    _tokens_lock;

	std::map<std::wstring, std::wstring> _policy_map;
	std::map<std::wstring, std::wstring> _tokengroup_resoucetype_map;
	std::map<std::wstring, std::wstring> _locked_duid_map;
	mutable NX::utility::CRwLock    _lockedduids_lock;
	bool				_locked_duid_dirty;

    // cache
    eval_cache          _eval_cache;
    decrypt_token_cache _token_cache;
	bool				_token_cache_dirty;
	std::vector<std::pair<std::wstring, std::wstring>> _logon_callbacks;

    // config
    client_config       _client_config;
    client_local_config _local_config;
    classify_config     _classify_config;
    watermark_config    _watermark_config;
	client_dir_config   _client_dir_config;
    app_whitelist_config    _app_whitelist_config;
    std::map<std::wstring, std::wstring> _config_serial_numbers;
    bool                _watermark_image_ready;
    bool                _dwm_enabled;
	bool                _ad_hoc;
	std::wstring         _system_bucket_name;

    friend class winsession;
};

class winsession : boost::noncopyable
{
public:
    winsession();
    winsession(unsigned long session_id);
    virtual ~winsession();

    inline unsigned long get_session_id() const { return _session_id; }
    inline const std::wstring& get_windows_user_name() const { return _user_name; }
    inline const std::wstring& get_windows_user_sid() const { return _user_sid; }
    inline const std::wstring& get_temp_profiles_dir() const { return _temp_profiles_dir; }
    inline const std::wstring& get_protected_profiles_dir() const { return _protected_profiles_dir; }
    inline const std::wstring& get_profiles_dir() const { return _profiles_dir; }
    inline const rmsession& get_rm_session() const { return _rm_session; }
    inline rmsession& get_rm_session() { return _rm_session; }
    inline const NX::win::user_dirs& get_user_dirs() const { return _user_dirs; }
    inline const std::wstring& get_intermediate_dir() const { return _inter_dir; }

    inline bool empty() const { return (-1 == _session_id); }
	inline bool is_server_mode() const { return (0 == _session_id); }

    void clear();
	void execute(std::wstring &app, std::wstring &param);

private:
    void init();
    void impersonate_init(HANDLE token_handle);

    bool is_appstream();

    void set_appstream_default_rpm_folder(const std::wstring& program_data);

private:
    unsigned long   _session_id;
    NX::win::user_dirs _user_dirs;
    std::wstring    _user_name;
    std::wstring    _user_sid;
    std::wstring    _temp_profiles_dir;
    std::wstring    _protected_profiles_dir;
    std::wstring	_profiles_dir;
    rmsession       _rm_session;
    std::wstring    _inter_dir;
};

class winsession_manager : boost::noncopyable
{
public:
    winsession_manager();
    virtual ~winsession_manager();

    void add_session(unsigned long session_id);
    void remove_session(unsigned long session_id);
    std::shared_ptr<winsession> get_session(unsigned long session_id) const;

    void clear();
    bool empty();

    static std::vector<unsigned long> find_existing_session();
	std::vector<unsigned long> get_all_session_id();

private:
    std::map<unsigned long, std::shared_ptr<winsession>>    _map;
    mutable CRITICAL_SECTION _lock;
};



#endif