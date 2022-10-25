#pragma once

#include <string>
#include <vector>
#include <set>
#include "SDLResult.h"
#include "nxrmdrv.h"
#include "SDLTypeDef.h"


	typedef enum NXSERV_REQUEST {
		CTL_SERV_UNKNOWN = 0,
		CTL_SERV_QUERY_STATUS,
		CTL_SERV_QUERY_PRODUCT_INFO,
		CTL_SERV_QUERY_RMS_SETTINGS,
		CTL_SERV_VERIFY_SECURITY,
		CTL_SERV_QUERY_RMC_SETTINGS,            // 5
		CTL_SERV_QUERY_LOG_SETTINGS,
		CTL_SERV_QUERY_LOGON_URL,
		CTL_SERV_FINALIZE_LOGIN,
		CTL_SERV_QUERY_LOGON_STATUS,
		CTL_SERV_UPDATE_POLICY,                 // 10
		CTL_SERV_QUERY_SOFTWARE_UPDATE,
		CTL_SERV_SET_LOGSETTING,
		CTL_SERV_SET_MEMBERSHIP,
		CTL_SERV_SET_SECUREMODE,
		CTL_SERV_REGISTER_NOTIFICATION,         // 15
		CTL_SERV_LOG_SHARING,
		CTL_SERV_USER_LOGOUT,
		CTL_SERV_COLLECT_DEBUGLOG,
		CTL_SERV_ACTIVITY_NOTIFICATION,
		CTL_SERV_SET_DWM_STATUS,                // 20
		CTL_SERV_EXPORT_ACTIVITY_LOG,
#ifdef SDWRMCLIB_FOR_INSTALLER
		CTL_SERV_SET_SERVICE_STOP_NO_SECURITY,
#else
		CTL_SERV_UNUSED_0,
#endif
		CTL_SERV_INSERT_DIR,
		CTL_SERV_REMOVE_DIR,
		CTL_SERV_CLIENT_ID,                     // 25
#ifdef SDWRMCLIB_FOR_INSTALLER
		CTL_SERV_STOP_SERVICE_NO_SECURITY,
#else
		CTL_SERV_UNUSED_1,
#endif
		CTL_SERV_POLICY_BUNDLE,
		CTL_SERV_SET_SERVICE_STOP,
		CTL_SERV_SYNC_USER_ATTR,
		CTL_SERV_SET_CACHED_TOKEN,              // 30
		CTL_SERV_DELETE_CACHED_TOKEN,
		CTL_SERV_DELETE_FILE_TOKEN,
		CTL_SERV_GET_SECRET_DIR,
		CTL_SERV_SET_ROUTER,
		CTL_SERV_REGISTER_APP,                  // 35
		CTL_SERV_UNREGISTER_APP,
		CTL_SERV_NOTIFY_RMX_STATUS,
		CTL_SERV_ADD_TRUSTED_PROCESS,
		CTL_SERV_REMOVE_TRUSTED_PROCESS,
		CTL_RPM_FOLDER,                         // 40
		CTL_SERV_GET_USER_INFO,
		CTL_SERV_DELETE_FILE,
		CTL_SERV_GET_RIGHTS,
		CTL_SERV_GET_FILE_STATUS,
		CTL_SERV_SET_APP_REGISTRY,              // 45
		CTL_SERV_LAUNCH_PDP,
		CTL_SERV_COPY_FILE,
		CTL_SERV_IS_APP_REGISTERED,
		CTL_SERV_GET_PROTECTED_PROFILES_DIR,
		CTL_SERV_POPUP_NEW_TOKEN,               // 50
		CTL_SERV_FIND_CACHED_TOKEN,
		CTL_SERV_REMOVE_CACHED_TOKEN,
		CTL_SERV_ADD_ACTIVITY_LOG,
		CTL_SERV_ADD_NXL_METADATA,
		CTL_SERV_REQUEST_LOGIN,                 // 55
		CTL_SERV_REQUEST_LOGOUT,
		CTL_SERV_NOTIFY_MESSAGE,
		CTL_SERV_READ_FILE_TAGS,
		CTL_SERV_DELETE_FOLDER,
		CTL_SERV_ADD_TRUSTED_APP,               // 60
		CTL_SERV_REMOVE_TRUSTED_APP,
		CTL_SERV_LOCK_NXL_FORSYNC,
		CTL_SERV_RUN_REGISTRY_CMD,
		CTL_SERV_QUERY_OPENED_FILE_RIGHTS,
		CTL_SERV_SET_FILE_ATTRIBUTES,           // 65
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		CTL_SERV_INSERT_SANCTUARY_DIR,
		CTL_SERV_REMOVE_SANCTUARY_DIR,
		CTL_SERV_IS_SANCTUARY_DIR,
#else
		CTL_SERV_UNUSED_2,
		CTL_SERV_UNUSED_3,
		CTL_SERV_UNUSED_4,
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		CTL_SERV_GET_FILE_ATTRIBUTES,
		CTL_SERV_WINDOWS_ENCRYPT_FILE, // 70
		CTL_SERV_GET_RPM_FOLDER,
		CTL_SERV_QUERY_APIUSER
	} NXSERV_REQUEST;

#define LOGIN_MEMBERSHIP_KEY_NAME				"memberships" /* copy from CORE */
#define LOGIN_USER_PREFERENCES					"preferences"

#define NXSERV_REQUEST_PARAM_CODE				"code"
#define NXSERV_REQUEST_PARAM_RESULT				"result"
#define NXSERV_REQUEST_PARAM_MESSAGE			"message"
#define NXSERV_REQUEST_PARAM_PARAMETERS			"parameters"
#define NXSERV_REQUEST_PARAM_FILEPATH			"filePath"
#define NXSERV_REQUEST_PARAM_FILE_ATTRIBUTES	"fileAttributes"
#define NXSERV_REQUEST_PARAM_SOURCEPATH			"srcPath"
#define NXSERV_REQUEST_PARAM_DELSOURCE			"deleteSource"
#define NXSERV_REQUEST_PARAM_CLIENTID			"clientid"
#define NXSERV_REQUEST_PARAM_ENABLE				"enable"
#define NXSERV_REQUEST_PARAM_APPPATH			"appPath"
#define NXSERV_REQUEST_PARAM_SECURITY			"security"
#define NXSERV_REQUEST_PARAM_RUNNING			"running"
#define NXSERV_REQUEST_PARAM_PROCESSID			"processId"

#define NXRMDRV_MSG_TYPE_QUERY_SERVICE		(0x9000000D)

#define NXSERV_REQUEST_SERVICE_DATA_SIZE	(15384)  // used in regular cases

#define NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE	(2048 + REQUEST_PACKET_OFFSET_SIZE)  //only used in login data

	typedef struct _QUERY_SERVICE_REQUEST {

		ULONG					ProcessId;

		ULONG					ThreadId;

		ULONG					SessionId;

		UCHAR					Data[NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE];

	}QUERY_SERVICE_REQUEST, *PQUERY_SERVICE_REQUEST;

	typedef struct _QUERY_SERVICE_REQUEST_BIG {

		ULONG					ProcessId;

		ULONG					ThreadId;

		ULONG					SessionId;

		UCHAR					Data[NXSERV_REQUEST_SERVICE_DATA_BIG_SIZE];

	}QUERY_SERVICE_REQUEST_BIG, *PQUERY_SERVICE_REQUEST_BIG;

class drvcore_mgr
{
public:
	drvcore_mgr();
	~drvcore_mgr();

	bool isDriverExist();
    SDWLResult insert_dir(const std::wstring& path, uint32_t option = 0, const std::wstring& filetags = L"");
	SDWLResult remove_dir(const std::wstring& path, bool bForce = false);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	SDWLResult insert_sanctuary_dir(const std::wstring& path, const std::wstring& filetags);
	SDWLResult remove_sanctuary_dir(const std::wstring& path);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	SDWLResult set_clientid(const std::wstring& clientid);
    SDWLResult set_login_result(const std::string& strJson, const std::string& security);
	SDWLResult add_cached_token(const std::string &token_id, const std::string &token_otp, const std::string &token_value);
	SDWLResult remove_cached_token(const std::string &token_id);
    SDWLResult set_user_attr(const std::string& strJson);
	SDWLResult logout();
	std::vector<unsigned char> send_request(unsigned long type, const void* request_data, unsigned long data_size, unsigned long timeout = 30000);

    SDWLResult set_service_stop(bool enable, const std::string& security);
#ifdef SDWRMCLIB_FOR_INSTALLER
	SDWLResult set_service_stop_no_security(bool enable);
	SDWLResult stop_service_no_security();
#endif
	SDWLResult delete_cheched_token();
	SDWLResult delete_file_token(const std::wstring& filename);
	SDWLResult get_secret_dir(std::wstring& filepath);
	SDWLResult set_router_key(std::wstring router, std::wstring tenant, std::wstring workingfolder, std::wstring tempfolder, std::wstring sdklibfolder);
	SDWLResult register_app(const std::wstring& appPath, const std::string& security);
	SDWLResult unregister_app(const std::wstring& appPath, const std::string& security);
	SDWLResult notify_rmx_status(bool running, const std::string& security);
	SDWLResult add_trusted_process(unsigned long processId, const std::string& security);
	SDWLResult remove_trusted_process(unsigned long processId, const std::string& security);
	SDWLResult add_trusted_app(const std::wstring& appPath, const std::string& security);
	SDWLResult remove_trusted_app(const std::wstring& appPath, const std::string& security);
	SDWLResult is_rpm_folder(const std::wstring& path, uint32_t* dirstatus, SDRmRPMFolderOption* option, std::wstring& filetags);
    //
    // option
    //  0 all API folders for current user
    //  1 all RPM folders in this machine
    //  2 MyFolders for current user
    //
    SDWLResult get_rpm_folder(std::vector<std::wstring> &folders, SDRmRPMFolderQuery option = RPMFOLDER_MYAPIFOLDERS);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	SDWLResult is_sanctuary_folder(const std::wstring& path, uint32_t* dirstatus, std::wstring& filetags);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	SDWLResult get_user_info(std::wstring& router, std::wstring& tenant, std::wstring& workingfolder, std::wstring& tempfolder, std::wstring& sdklibfolder, bool &blogin);
	SDWLResult delete_file(const std::wstring& filepath);
	SDWLResult delete_folder(const std::wstring& folderpath);
	SDWLResult copy_file(const std::wstring& srcpath, const std::wstring& destpath, bool deltesource = false);
	SDWLResult get_rights(const std::wstring& filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, int option = 0); // option: 0, get user rights on file; 1, get file rigts set on file (ad-hoc)
	SDWLResult get_file_status(const std::wstring& filename, uint32_t* dirstatus, bool* filestatus);
	SDWLResult get_file_info(const std::wstring &filepath, std::string &duid, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &userRightsAndWatermarks,
		std::vector<SDRmFileRight> &rights, SDR_WATERMARK_INFO &waterMark, SDR_Expiration &expiration, std::string &tags,
		std::string &tokengroup, std::string &creatorid, std::string &infoext, DWORD &attributes, DWORD &isRPMFolder, DWORD &isNXLFile, bool checkOwner=true);
	SDWLResult set_file_attributes(const std::wstring &filePath, DWORD fileAttributes);
	SDWLResult set_app_key(const std::wstring& key, const std::wstring& subkey, const std::wstring& data, uint32_t op, const std::string &security);
	SDWLResult launch_pdp_process();
	SDWLResult is_app_registered(const std::wstring& appPath, bool& registered);
	SDWLResult get_protected_profiles_dir(std::wstring& path);
	SDWLResult popup_new_token(const std::wstring& membershipid, std::string &token_id, std::string &token_otp, std::string &token_value);
	SDWLResult find_cached_token(const std::wstring& duid, std::string &token_id, std::string &token_otp, std::string &token_value, time_t &token_ttl);
    SDWLResult add_activity_log(const std::string& strJson);
    SDWLResult add_nxl_metadata(const std::string& strJson);
	SDWLResult verify_security(const std::string& security);
	SDWLResult request_login(const std::wstring &callback_cmd, const std::wstring &callback_cmd_param);
	SDWLResult request_logout(uint32_t option = 0); // 0: tell RPM Service Mananger to logout; 1: ask RPM Service whether can logout or not
	SDWLResult notify_message(const std::wstring &app, const std::wstring &target, const std::wstring &message, uint32_t msgtype, const std::wstring &operation, uint32_t result, uint32_t fileStatus);
	SDWLResult read_file_tags(const std::wstring& filepath, std::wstring &tags);
    SDWLResult lock_file_forsync(const std::string& strJson);
	SDWLResult request_run_registry_command(const std::string& reg_cmd, const std::string &security);
	SDWLResult request_opened_file_rights(std::map<std::wstring, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>>& mapOpenedFileRights, const std::string &security, std::wstring &ex_useremail, unsigned long ulProcessId = 0);

	SDWLResult get_file_attributes(const std::wstring &filePath, DWORD &fileAttributes);
	SDWLResult windows_encrypt_file(const std::wstring &FilePath);

	SDWLResult request_apiuser_logindata(std::wstring& apiuser_logindata);

	SDWLResult request_is_apiuser();

	void setTransportOption(unsigned int option);

private:
	SDWLResult get_response_status(std::string& result);
	SDWLResult get_registry_response_status(const std::string& response);
	std::vector<SDRmFileRight> to_sdrm_fileright(uint64_t u64Rights);
	SDWLResult insert_dir_common(NXSERV_REQUEST req, const std::wstring& filepath, uint32_t option = 0, const std::wstring& filetags = L"");
	SDWLResult remove_dir_common(NXSERV_REQUEST req, const std::wstring& filepath, bool bForce = false);
	SDWLResult is_dir_common(NXSERV_REQUEST req, const std::wstring& filepath, uint32_t* dirstatus, SDRmRPMFolderOption* option, std::wstring& filetags);

private:
	HANDLE  _h;
	CRITICAL_SECTION _lock;
	unsigned long _session_id;
	unsigned int _t_option; // default: 0, using pipe to communicate; 1: use driver/shared memory to communicate; 2: use socket;
};
