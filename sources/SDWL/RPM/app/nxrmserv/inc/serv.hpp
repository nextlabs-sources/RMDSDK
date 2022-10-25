
#pragma once
#ifndef __NXRM_SVC_HPP__
#define __NXRM_SVC_HPP__

#include <mutex>
#include <set>
#include <nudf\service.hpp>
#include <nudf\json.hpp>
#include <nudf\async_pipe_with_iocp.hpp>

#include "session.hpp"
#include "rsapi.hpp"
#include "config.hpp"
#include "serv_core.hpp"
#include "serv_flt.hpp"
#include "key_manager.hpp"
#include "vhd_manager.hpp"
#include "nxrmflt.h"

#include "..\..\..\..\..\..\sources\SDWL\SDWRmcLib\PDP\pdp.h"




extern BOOL __stdcall check_log_accept(unsigned long level);
extern LONG __stdcall write_log(const wchar_t* msg);


// FORWARD class
class rmserv;

class rmserv_conf
{
public:
    rmserv_conf();
    rmserv_conf(const std::wstring& key_path);
    rmserv_conf(unsigned long log_level, unsigned long log_size, unsigned long log_rotation, unsigned long delay_time = 0, bool disable_auinstall = false, bool disable_antitampering= false);
    ~rmserv_conf();

	inline unsigned long get_no_vhd() const { return _no_vhd; }
  inline unsigned long get_network_timeout() const { return _network_timeout; }
  inline unsigned long get_delay_seconds() const { return _delay_seconds; }
    inline unsigned long get_log_level() const { return _log_level; }
    inline unsigned long get_log_size() const { return _log_size; }
    inline unsigned long get_log_rotation() const { return _log_rotation; }
    inline const std::set<std::wstring>& get_trusted_apps() const { return _trusted_apps; }

	inline bool is_enable_antitampering() const { return (_disable_antitampering == 0); }
	inline bool is_disable_autoupgrade_install() const { return (_disable_autoupgrade_install == 1);}

    inline void set_log_level(unsigned long level) { _log_level = level; }
    inline void set_log_size(unsigned long size) { _log_size = size; }
    inline void set_log_rotation(unsigned long rotation) { _log_rotation = rotation; }

    rmserv_conf& operator = (const rmserv_conf& other);

    void clear();
    void load(const std::wstring& key_path = std::wstring()) noexcept;
    void load_from_xmlfile(const std::wstring& file);
    void apply(const std::wstring& key_path = std::wstring());

	inline unsigned long get_auth_type() const { return _api_auth_type; }
	inline unsigned long get_api_app_id() const { return _api_app_id; }
	inline std::wstring get_api_app_key() const { return _api_app_key; }
	inline std::wstring get_api_email() const { return _api_email; }
	inline std::wstring get_api_private_cert() const { return _api_privatecert; }
	inline std::wstring get_api_working_folder() const { return _api_working_folder; }

private:
	unsigned long   _no_vhd;
  unsigned long   _network_timeout;
  unsigned long   _delay_seconds;
	unsigned long	_disable_autoupgrade_install;
	unsigned long	_disable_antitampering;
    unsigned long   _log_level;
    unsigned long   _log_size;
    unsigned long   _log_rotation;
    std::set<std::wstring>  _trusted_apps;

	// API User
	unsigned long	_api_auth_type; // 0: normal; 1: API User
	unsigned long	_api_app_id;
	std::wstring	_api_app_key;
	std::wstring	_api_email;
	std::wstring	_api_privatecert;
	std::wstring	_api_working_folder;
};

class router_config
{
public:
    router_config();
    router_config(const router_config& other);
    router_config(router_config&& other);
    ~router_config();

    inline bool empty() const { return _router.empty(); }
    inline const std::wstring& get_router() const { return _router; }
    inline const std::wstring& get_tenant_id() const { return _tenant_id; }
	void set_router(std::wstring& router) { _router = router; }
	void set_tenant_id(std::wstring& tenant_id) { _tenant_id = tenant_id; };
	/*const std::wstring& get_workingfolder() const { return _workingfolder; }
	const std::wstring& get_tempfolder() const { return _tempfolder; }
	const std::wstring& get_sdklibfolder() const { return _pdpDir; }
	void set_workingfolder(std::wstring& workingfolder) { _workingfolder = workingfolder; }
	void set_tempfolder(std::wstring& tempfolder) { _tempfolder = tempfolder; }
	void set_sdklibfolder(std::wstring& sdklibfolder) { _pdpDir = sdklibfolder; } 
	std::wstring    _workingfolder;
	std::wstring    _tempfolder;
	std::wstring    _pdpDir;
	*/
    router_config& operator = (const router_config& other);
    router_config operator = (router_config&& other);
	bool SetRouterRegistry(const std::wstring& router, const std::wstring& tenant);
    void clear();
    void load_from_file(const std::wstring& file);
    void load_from_xmlfile(const std::wstring& file);

private:
    std::wstring    _router;
    std::wstring    _tenant_id;
	
};

class core_context_profile
{
public:
    core_context_profile() {}
    ~core_context_profile() {}

    void add_context(const std::wstring& image, unsigned __int64 checksum, const std::vector<unsigned __int64>& data);
    std::vector<unsigned __int64> find_context(const std::wstring& image, unsigned __int64 checksum);

private:
    NX::utility::CRwLock _lock;
    std::map<std::wstring/*image path*/, std::map<unsigned __int64 /*module checksum*/, std::vector<unsigned __int64>/*context data*/>> _context_map;

    friend class rmserv;
};

class pipe_server : public NX::async_pipe_with_iocp::server
{
public:
	pipe_server();
	virtual ~pipe_server();
	virtual void on_read(unsigned char* data, unsigned long* size, bool* write_response);

private:
	friend class rmserv;
};

typedef enum _SDRmRPMFolderConfig {
    RPMFOLDER_REMOVE = 0,
    RPMFOLDER_ADD,
    RPMFOLDER_CHECK_MYRPM,
    RPMFOLDER_CHECK_MYFOLDER,
    RPMFOLDER_CHECK_MYAPIRPM,
    RPMFOLDER_CHECK_RPM,
    RPMFOLDER_CHECK_ADD_MYFOLDER
} SDRmRPMFolderConfig;

class rmserv : public NX::win::service_instance
{
public:
    rmserv();
    virtual ~rmserv();

    virtual void clear();

    virtual void on_start(int argc, const wchar_t** argv);
    virtual void on_stop() noexcept;
    virtual void on_pause();
    virtual void on_resume();
    virtual void on_preshutdown() noexcept;
    virtual void on_shutdown() noexcept;
    virtual long on_session_logon(_In_ WTSSESSION_NOTIFICATION* wtsn) noexcept;
    virtual long on_session_logoff(_In_ WTSSESSION_NOTIFICATION* wtsn) noexcept;
	virtual bool CheckFileOpen();
	
	bool IsFileOpen(unsigned long session_id, std::string& response);

    bool initialize();
    std::string process_request(unsigned long session_id, unsigned long process_id, const std::string& s);
    void save_core_context_cache();

    inline router_config& get_router_config() { return _router_conf; }
    inline vhd_manager& get_vhd_manager() { return _vhd_manager; }
    inline winsession_manager& get_win_session_manager() { return _win_session_manager; }
    inline drvcore_serv& get_coreserv() { return _serv_core; }
    inline drvflt_serv& get_fltserv() { return _serv_flt; }
    inline const std::wstring& get_client_id() const { return _client_id; }
    inline core_context_cache& get_core_context_cache() { return _corecontext_cache; }
	inline rmserv_conf & get_service_conf() { return _serv_conf; }
	const std::wstring GetDeviceID() const { return _deviceid; }
	void set_client_id(std::wstring client_id) { _client_id = client_id;  }

	const std::wstring& get_pdp_dir() const { return m_pdpDir; }
	void set_pdp_dir(std::wstring filepath) { m_pdpDir = filepath; }
	void set_registered_apps(const std::set<std::wstring>& apps) { m_registeredApps = apps; }
	void set_trusted_apps_from_config(unsigned long session_id, const std::map<std::wstring, std::vector<unsigned char>>& apps);
	
	bool is_file_opened() { return _fileopen; }
	bool RefreshDirectory();
	unsigned long AddDirectoryConfig(std::wstring dir, SDRmRPMFolderConfig action = RPMFOLDER_ADD, const std::wstring& fileTags = L"{}", const std::wstring &wsid = L"", const std::wstring &rmsuid = L"", const std::wstring &stroption = L"1");
	std::wstring get_dirs();
	void parse_dirs(const std::wstring& str);
	void InitMarkDirecory();
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	unsigned long AddSanctuaryDirectoryConfig(std::wstring& dir, bool add = true, const std::wstring& fileTags = L"{}");
	std::wstring get_sanctuary_dirs();
	void parse_sanctuary_dirs(const std::wstring& str);
	void InitSanctuaryDirecory();
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	bool UpdatePDPDirectory(const std::wstring dir);

	std::wstring get_profiles_dir();
	std::wstring get_config_dir();
	// pdp
	void InitializePDP();
	void set_protected_profiles_dir(const std::wstring dir) { _protected_profiles_dir = dir; }
	bool IsInitFinished(bool& finished);
	bool ensurePDPConnectionReady(bool& finished);
	bool IsProcessTrusted(unsigned long process_id);
	bool EvalRights(unsigned long processId,
		const std::wstring &userDispName,
		const std::wstring &useremail,
		const std::wstring &userID,
		const std::wstring &appPath,
		const std::wstring &resourceName,
		const std::wstring &resourceType,
		const std::vector<std::pair<std::wstring, std::wstring>> &attrs,
		const std::vector<std::pair<std::wstring, std::wstring>> &userAttrs,
		const std::wstring &bundle, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> *rightsAndWatermarks,
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> *rightsAndObligations, bool bcheckowner = true);

	bool PDPEvalRights(const std::wstring &userDispName,
		const std::wstring &useremail,
		const std::wstring &userID,
		const std::wstring &appPath,
		const std::wstring &resourceName,
		const std::wstring &resourceType,
		const std::vector<std::pair<std::wstring, std::wstring>> &attrs,
		const std::vector<std::pair<std::wstring, std::wstring>> &userAttrs,
		const std::wstring &bundle, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> *rightsAndWatermarks,
		std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> *rightsAndObligations);

	bool StartPDPMan(void);
	bool StopPDPMan(void);

	void ProcessNotification(unsigned long processId, const std::wstring& image_path, bool create);
	bool IsPDPProcess(ULONG processId);

	void FileRightsNotification(unsigned long processId, std::wstring& strFile, ULONGLONG ullRights);
	void handle_nxl_denied_file(unsigned long processId, std::wstring& strFile);
	void ProcessExitNotification(unsigned long processId, const std::wstring& image_path);

	inline bool is_server_mode() const { return _server_mode; }
	inline bool is_central_config() const { return (_serv_conf.get_auth_type() == 1); }

private:
	bool VerifySecurityString(const NX::json_value& parameters, std::string &failResponse);
	bool IsAppInIgnoreSignatureDirs(const std::wstring& appPath);
	std::set<std::wstring> RegisteredAppsGet(void);
	bool RegisteredAppsCheckExist(const std::wstring& appPath);
	void RegisteredAppsAdd(const std::wstring& appPath);
	bool RegisteredAppsRemove(const std::wstring& appPath);
	bool TrustedProcessIdsCheckExist(unsigned long processId);
	void TrustedProcessIdsAdd(unsigned long processId, unsigned int sessionid);
	unsigned int GetSessionIdOfProcess(unsigned long processId);
	bool TrustedProcessIdsRemove(unsigned long processId);
	bool NonPersistentTrustedAppPathsCheckExist(const std::wstring& appPath);
	void NonPersistentTrustedAppPathsAdd(const std::wstring& appPath);
	bool NonPersistentTrustedAppPathsRemove(const std::wstring& appPath);
	void EffectiveTrustedAppsResolve(void);
	bool EffectiveTrustedAppsCheckExist(const std::wstring& appPath);
	void force_unset_RPM_folder(std::wstring filepath, unsigned long session_id, unsigned long process_id);

	void init_server_mode();

protected:
    void init_log();
    void log_init_info();
    bool init_keys();
    bool init_vhd();
    bool init_drvcore();
    bool init_drvflt();
    void init_core_context_cache();


    void initialize_sspi() noexcept;
    void emulate_browser();

    void set_ready_flag(bool b);
    bool is_service_ready();

    std::string on_request_query_general_status(unsigned long session_id, unsigned long process_id);
    std::string on_request_query_product_info(unsigned long session_id, unsigned long process_id);
    std::string on_request_query_rms_settings(unsigned long session_id, unsigned long process_id);
    std::string on_request_verify_security(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
    std::string on_request_query_rmc_settings(unsigned long session_id, unsigned long process_id);
    std::string on_request_query_log_settings(unsigned long session_id, unsigned long process_id);
    std::string on_request_query_login_urls(unsigned long session_id, unsigned long process_id);
    std::string on_request_finalize_login(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_set_user_attr(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_set_cached_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
    std::string on_request_query_login_status(unsigned long session_id, unsigned long process_id);
    std::string on_request_check_profile_update(unsigned long session_id, unsigned long process_id);
    std::string on_request_check_software_update(unsigned long session_id, unsigned long process_id);
    std::string on_request_set_log_settings(unsigned long session_id, unsigned long process_id, const NX::json_object& parameters);
    std::string on_request_set_default_membership(unsigned long session_id, unsigned long process_id, const NX::json_object& parameters);
    std::string on_request_set_default_security_mode(unsigned long session_id, unsigned long process_id, int mode);
    std::string on_request_register_nodification_handler(unsigned long session_id, unsigned long process_id, HWND h);
    std::string on_request_log_sharing_transaction(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
    std::string on_request_logout(unsigned long session_id, unsigned long process_id);
    std::string on_request_collect_debugdump(unsigned long session_id, unsigned long process_id);
    std::string on_request_activity_notify(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
    std::string on_request_set_dwm_status(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
    std::string on_request_query_activity_records(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_insert_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_remove_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	std::string on_request_insert_sanctuary_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_remove_sanctuary_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	std::string on_request_set_clientid(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_set_policy_bundle(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_set_service_stop(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_set_service_stop_no_security(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_stop_service_no_security(unsigned long session_id, unsigned long process_id);
	std::string on_request_delete_cached_token(unsigned long session_id, unsigned long process_id);
	std::string on_request_delete_file_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_get_secret_dir(unsigned long session_id, unsigned long process_id);
	std::string on_request_set_router(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_register_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_unregister_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_notify_rmx_status(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_add_trusted_process(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_remove_trusted_process(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_add_trusted_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_remove_trusted_app(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_rpm_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
    std::string on_request_get_rpm_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	std::string on_request_is_sanctuary_directory(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	std::string on_request_user_info(unsigned long session_id, unsigned long process_id);
	std::string on_request_delete_file(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_delete_folder(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_get_rights(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_get_file_status(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_set_app_registry(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_launch_pdp(unsigned long session_id, unsigned long process_id);
	std::string on_request_copy_file(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_is_app_registered(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_get_protected_profiles_dir(unsigned long session_id, unsigned long process_id);
	std::string on_request_pop_new_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_find_file_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_remove_cached_token(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_add_activity_log(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_add_metadata(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_login(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_logout(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_notify_message(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_read_file_tags(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_run_registry_command(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_opened_file_rights(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_set_file_attributes(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_get_file_attributes(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);
	std::string on_request_windows_encrypt_file(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);

	std::string on_request_lock_localfile(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);

	std::string on_request_query_apiuser(unsigned long session_id, unsigned long process_id, const NX::json_value& parameters);

	bool IsPDPProcess(std::vector<std::pair<unsigned long, std::wstring>>& protected_processes);
	void GetNonPDPProcessId(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes, std::set<unsigned long>& pidSet);
    std::wstring GetNonPDPProcesses(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes);

	// The following three version methods are used for judging when remove\un-mark RPM directory (Added for fixed bug 70192).
	// exclude nxrmviewer process.
	bool IsPDPProcessEx(std::vector<std::pair<unsigned long, std::wstring>>& protected_processes);
	void GetNonPDPProcessIdEx(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes, std::set<unsigned long>& pidSet);
	std::wstring GetNonPDPProcessesEx(const std::vector<std::pair<unsigned long, std::wstring>>& protected_processes);

    bool IsFileInMarkDirOpened(const std::set<unsigned long>& pidSet, const std::wstring& markedDir);
	bool EnumerateOpenedFiles(const std::set<unsigned long>& pidSet, std::vector<std::wstring>& openFiles);
	bool VerifyEmbeddedSignature(const std::wstring& sourceFile, LONG *pStatus, unsigned long sessionId = 0);
	bool GetFileHash(const std::wstring& sourceFile, std::vector<unsigned char> &hash);

	void handle_whilelist_inherite(unsigned long ulProcessID, const std::wstring& image_path, bool bCreate);

	std::string handle_request_opened_file_rights(unsigned long session_id, unsigned long ulProcessID);

	bool update_denied_nxlfile_map(const std::wstring& strKey);

	std::wstring gen_denied_nxlfile_key(unsigned long processId, std::wstring& strFile);

private:
    bool                _ready;
    CRITICAL_SECTION    _lock;
	CRITICAL_SECTION    _pdplock;
    CRITICAL_SECTION    _rpmlock;
    router_config       _router_conf;
    rmserv_conf         _serv_conf;
    drvcore_serv        _serv_core;
    drvflt_serv         _serv_flt;
    nxmaster_key        _master_key;
    vhd_manager         _vhd_manager;
    winsession_manager  _win_session_manager;
    std::wstring        _client_id; // This is the thumb print of master key
    core_context_cache  _corecontext_cache;
	std::wstring         _deviceid;

	NX::SDWPDP           m_PDP;
	std::wstring         m_pdpDir;

    //
    // m_markDir
    //  std::wstring    folder path
    //  wchar_t         overwrite
    //  wchar_t         append NXL ext
    //  wchar_t         auto-protect
    //  std::wstring    file tags
    //  std::wstring    user-windows-sid
    //  std::wstring    rms-user-id
    //  std::wstring    option
    //
	std::vector<std::tuple<std::wstring, wchar_t, wchar_t, wchar_t, std::wstring, std::wstring, std::wstring, std::wstring>> m_markDir;

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	std::vector<std::pair<std::wstring, std::wstring>> m_sanctuaryDir;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	std::wstring         _protected_profiles_dir;
	std::mutex			 m_mtxPDP;
	std::set<std::wstring>  m_registeredApps;
	std::map<unsigned long, unsigned int> m_trustedProcessIds;
	std::set<std::wstring> m_nonPersistentTrustedAppPaths;
	std::map<std::wstring, std::vector<unsigned char>>  m_trustedAppsFromConfig;
	std::map<std::wstring, std::vector<unsigned char>>  m_effectiveTrustedApps;
	std::map<std::wstring, uint64_t>	m_mapDeniedNxlFile;
	bool                 _fileopen;

	bool				 _server_mode;
	std::string			 _server_login_data;

	pipe_server			 _ipc_server;
};

#endif
