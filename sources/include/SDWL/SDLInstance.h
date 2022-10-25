/*!
 * \file SDLInstance.h
 *
 * \author hbwang
 * \date October 2017
 *
 * the parent class to access all data exported from SDWRmcLib Library
 */
#pragma once
#include <tuple>
#include <set>
#include <map>
#include "SDLResult.h"
#include "SDLTenant.h"
#include "SDLHttpRequest.h"
#include "SDLUser.h"

#define RPM_SAFEDIRRELATION_SAFE_DIR                            0x00000001
#define RPM_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR                0x00000002
#define RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR              0x00000004

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
#define RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR                  0x00000010
#define RPM_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR      0x00000020
#define RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR    0x00000040
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

/*
Application to use rmdsdk need call instance class first to access all other classes. Application need pass the router, working folder information.
sdk will save all information, such as user, token, ... under the working folder.
*/
class ISDRmcInstance
{
public:
	ISDRmcInstance() {};
	virtual ~ISDRmcInstance() {};

public:
	virtual SDWLResult Initialize(const std::wstring &router, const std::wstring &tenant) = 0;
	/// Initialize RMCInstance Class
	/**
	 * @pre
	 *    Call API function SDWLibCreateSDRmcInstance to obtain handle of RMCInstance Class
	 * @param
	 *    router	router URL for registered tenant. if empty, default value will be used.
	 *    tenant	tenant id string. if empty, default value will be used
	 * @return
	 *    Result: return RESULT(0) when success
	 *			SDWL_CERT_NOT_INSTALLED		certificate is not installed/fail to installed
	 *			SDWL_RMS_ERRORCODE_BASE+	RMS server error
	 */
	virtual SDWLResult Initialize(const std::wstring &workingfolder, const std::wstring &router, const std::wstring &tenant) = 0;
	/// Initialize RMCInstance Class
	/**
	* @pre
	*    Call API function SDWLibCreateSDRmcInstance to obtain handle of RMCInstance Class
	* @param
	*    workingfolder	file folder string to save SDWLInstance internal data files. if no working folder is set
	*					the data files will be saved at temporary folder set at SDWLibCreateSDRmcInstance() 
	*    router			router URL for registered tenant. if empty, default value will be used.
	*    tenant			tenant id string. if empty, default value will be used
	* @return
	*    Result: return RESULT(0) when success
	*			SDWL_PATH_NOT_FOUND			invalid working folder
	*			SDWL_CERT_NOT_INSTALLED		certificate is not installed/fail to installed
	*			SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*/

	virtual SDWLResult IsInitFinished(bool& finished) = 0;
	/// Check if RMCInstance Class has finished initialization
	/**
	 * @pre
	 *    Initialize() function need be called before calling this funciton
	 * @param
	 *    bool			on success, true if initialization has finished, false if initialization still in progess.
	 * @return
	 *    Result: return RESULT(0) when success
	 */

	virtual SDWLResult CheckSoftwareUpdate(std::string &newVersionStr, std::string &downloadURL, std::string &checksum) = 0;
	/// Check if there is a different (either newer or older) version of the software on the server for the current tenant that can be downloaded for updating
	/**
	 * @pre
	 *    SDWLibCreateSDRmcInstance() function needs to be called with productName, productMajorVer, productMinorVer, and productBuild
	 *    IsInitFinished() function need be called before calling this funciton
	 * @param
	 *    newVersionStr	return version string of the software on the server, if any
	 *    downloadURL	return URL for downloading the software, if any
	 *    checksum		return SHA1 checksum of the download package, if any
	 * @return
	 *    Result: return RESULT(0) when the checking is successful (It does not necessarily mean that there is a version available for downloaded.)
	 */

	virtual SDWLResult Save() = 0;
	/// Save SDWLInstance internal data files to specified folder
	/**
	 * @pre
	 *    IsInitFinished() function need be called before calling this funciton
	 *					Note: if no working folder is set, only tenant related information will be saved.
	 * @return
	 *			SDWL_PATH_NOT_FOUND			invalid working folder
	 */

	virtual SDWLResult GetCurrentTenant(ISDRmTenant ** ptenant) = 0;
	/// Get Current Tenant information
	/**
	* @pre
	*    IsInitFinished() function need be called before calling this funciton
	* @param
	*    ptenant		pointer to current using tenant class
	* @return
	*			SDWL_NOT_FOUND				initialize tenant failed. check the return code of Initialize() for more detail
	*/

	virtual SDWLResult GetLoginRequest(ISDRmHttpRequest **prequest) = 0;
	/// Get user login http request information based on current Tenant
	/**
	* @pre
	*    IsInitFinished() function need be called before calling this funciton
	* @param
	*    prequest		pointer to SkyDRM User Login http request information
	* @return
	*			SDWL_INVALID_DATA			invalid tenant information. check return message or return code of Initialize() for more detail
	*			
	*/

	virtual SDWLResult SetLoginResult(const std::string &jsonreturn, ISDRmUser **puser, const std::string &security) = 0;
	/// Set Json return from User Login Request
	/**
	 * @pre
	 *    IsInitFinished() function need be called before calling this funciton
	 * @param
	 *	  jsonreturn	Returned JSON string from SkyDRM user login http request
	 *    puser			pointer to User interface 
	 *    security		security string for authentication
	 * @return
	 *			SDWL_INVALID_DATA			invalid tenant information. check return message or return code of Initialize() for more detail
	 *          SDWL_INVALID_JSON_FORMAT	invalid json string
	 *			SDWL_RMS_ERRORCODE_BASE+	RMS server error	 *
	 */

	virtual SDWLResult GetLoginUser(const std::string &useremail, const std::string &passcode, ISDRmUser **puser) = 0;
	/// Get previous login user session
	/**
	* @pre
	*    IsInitFinished() function need be called before calling this funciton
	* @param
	*	  useremail		user email string of login user
	*     passcode		passcode assigned to login user account
	*    puser			pointer to User interface
	* @return
	*			SDWL_INVALID_DATA			invalid user email or passcode
	*           SDWL_LOGIN_REQUIRED			User session timeout, need login again
	*/

	virtual SDWLResult RPMGetLoginUser(const std::string &passcode, ISDRmUser **puser) = 0;
	/// Get previous login user session
	/**
	* @pre
	*    IsInitFinished() function need be called before calling this funciton
	* @param
	*     passcode		passcode assigned to plugin
	*    puser			pointer to User interface
	* @return
	*			SDWL_INVALID_DATA			invalid user email or passcode
	*           SDWL_LOGIN_REQUIRED			User session timeout, need login again
	*/

	virtual SDWLResult SyncUserAttributes() = 0;
	/**
	* @pre
	*    SetLoginResult() function need be called before calling this funciton
	*   
	* @return
	*			SDWL_ACCESS_DENIED			RPM driver is not ready
	*           SDWL_LOGIN_REQUIRED			User session timeout, need login again
	*/
	
	virtual bool IsRPMDriverExist() = 0;
	/// Check if RPM driver is installed
	/**
	* @return
	*			return true when driver is installed, otherwise return false.
	*/

	virtual SDWLResult AddRPMDir(const std::wstring &path, uint32_t option = (SDRmRPMFolderOption::RPMFOLDER_NORMAL | SDRmRPMFolderOption::RPMFOLDER_API), const std::string &filetags = "") = 0;
	/// Add directory for RPM
	/**
	* @pre
	*	  SetLoginResult() function need be called before calling this funciton
	* @param
	*	  path			directory
	*	  option		one or more of the following OR'ed together:
	*						RPMFOLDER_NORMAL	Normal RPM folder.
	*						RPMFOLDER_OVERWRITE	1. When copying an NXL file to the RPM folder, if an existing non-NXL file in the
	*											   RPM folder has the same filename as the corresponding non-NXL file name of the
	*											   NXL file, the existing non-NXL file will be overwritten.
	*											2. When writing a non-NXL file to the RPM folder, if an existing NXL file in the
	*											   RPM folder has the same filename as the corresponding NXL file of the non-NXL
	*											   file, the existing NXL file will be overwritten.
	*						RPMFOLDER_USER		User can set only one RPM folder under her personal Windows account via SkyDRM
	*											Desktop client.
	*						RPMFOLDER_EXT		When copying an NXL file, whose filename does not have ".nxl" extension, to this
	*											RPM folder, it will automatically append ".nxl" and rename the file.
	*						RPMFOLDER_API		If set with this flag, the RPM folder will be shown in "Managed SkyDRM Folder"
	*											list of SkyDRM Desktop client. User can reset the folder to normal folder in the
	*											dialog.
    *     filetags		 If RPMFOLDER_AUTOPROTECT is passed, tags to be applied when files are copied or moved into out of the directory and protected
	* @return
	*			SDWL_INVALID_DATA			invalid path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	virtual SDWLResult AddSanctuaryDir(const std::wstring &path, const std::string &filetags = "") = 0;
	/// Add directory for Sanctuary
	/**
	* @pre
	*	  SetLoginResult() function need be called before calling this funciton
	* @param
	*	  path			directory
	*	  filetags		tags to be applied when files are copied or moved out of the directory and protected
	* @return
	*			SDWL_INVALID_DATA			invalid path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	virtual SDWLResult RemoveRPMDir(const std::wstring &path, bool bForce = false) = 0;
	/// Remove directory for RPM
	/**
	* @pre
	*	  SetLoginResult() function need be called before calling this funciton
	* @param
	*	  path			directory
	*	  bForce		force to unmark as trusted process might not quit(however, RPM service will check no File Handle is open under the folder)
	* @return
	*			SDWL_INVALID_DATA			invalid path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	virtual SDWLResult RemoveSanctuaryDir(const std::wstring &path) = 0;
	/// Remove directory for Sanctuary
	/**
	* @pre
	*	  SetLoginResult() function need be called before calling this funciton
	* @param
	*	  path			directory
	* @return
	*			SDWL_INVALID_DATA			invalid path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	virtual SDWLResult GetRPMDir(std::vector<std::wstring> &paths, SDRmRPMFolderQuery option = RPMFOLDER_MYAPIFOLDERS) = 0;
	/// Get directory for RPM
    /**
	* @param
	*	  paths			output of RPM directories
	*	  option
	*					RPMFOLDER_MYAPIFOLDERS		The API-set RPM folder for current user
	*					RPMFOLDER_ALLFOLDERS		All RPM Folders of the whole system
	*					RPMFOLDER_MYFOLDERS			All My Folders
	*					RPMFOLDER_MYRPMFOLDERS		All RPM Folders for current user
	* @return
	*/

	virtual SDWLResult SetRPMLoginResult(const std::string &jsonreturn, const std::string &security) = 0;
	/// Send login data to RPM
	/**
	* @param
	*    jsonreturn login data
	*    security   security string for authentication
	* @return
	*           SDWL_INVALID_DATA           invalid data
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMLogout() = 0;
	/// logout RPM
	/**
	* 
	* @return
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult SetRPMServiceStop(bool enable, const std::string &security) = 0;
	/// enable or disable RPM stop service
	/**
	*
	* @param
	*	  enable		stop service enable (true) or disable (false)
	*	  security		security string for authentication
	* @return
	*			SDWL_INVALID_DATA			invalid data
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult StopPDP(const std::string &security) = 0;
	/// Stop the PDP service
	/**
	*
	* @param
	*	  security		security string for authentication
	* @return
	*			SDWL_INVALID_DATA			invalid security string
	*			SDWL_NOT_READY              PDP service is not ready
	*/

	virtual SDWLResult RPMStartPDP(const std::string &security) = 0;
	/// Ask RPM service to start the PDP service
	/**
	*
	* @param
	*	  security		security string for authentication
	* @return
	*			SDWL_INVALID_DATA			invalid security string
	*			SDWL_NOT_READY              PDP service is not ready
	*/

	virtual SDWLResult GetLoginData(const std::string &useremail, const std::string &passcode, std::string &data) = 0;
	/// Get previous login data
	/**
	* @param
	*	  useremail		user email string of login user
	*	  passcode		passcode assigned to login user account
	*	  data			return login data string
	* @return
	*			SDWL_INVALID_DATA			invalid user email or passcode
	*			SDWL_LOGIN_REQUIRED			User session timeout, need login again
	*/

	virtual SDWLResult SetRPMDeleteCacheToken() = 0;
	/// Delete RPM cache token
	/**
	*
	* @return
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMDeleteFileToken(const std::wstring &filename) = 0;
	/**
	* @pre
	*    SetLoginResult()/SetRPMLoginResult function need be called before calling this funciton
	*
	* @return
	*			SDWL_ACCESS_DENIED			RPM driver is not ready
	*           SDWL_LOGIN_REQUIRED			User session timeout, need login again
	*/

	virtual SDWLResult RPMRegisterApp(const std::wstring &appPath, const std::string &security) = 0;
	/// Register an application that is controlled by RMX
	/**
	* @param
	*    appPath    full path to the .exe file of the application
	*    security   security string for authentication
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMUnregisterApp(const std::wstring &appPath, const std::string &security) = 0;
	/// Unregister an application that is controlled by RMX
	/**
	* @pre
	*    RPMRegisterApp() function needs to be called to register the application
	* @param
	*    appPath    full path to the .exe file of the application
	*    security   security string for authentication
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMNotifyRMXStatus(bool running, const std::string &security) = 0;
	/// Notify RPM of the status of the RMX in the caller process
	/**
	* @pre
	*    RPMRegisterApp() function needs to be called to register the .exe image file of the caller process
	* @param
	*    running    true if RMX is running (caller process will become trusted)
	*               false if RMX is not runnint (caller process will become untrusted)
	*    security   security string for authentication
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMAddTrustedProcess(unsigned long processId, const std::string &security) = 0;
	/// Add a process to the non-persistent trusted process list
	/**
	* @pre
	*    The caller process needs to be a trusted process.
	* @param
	*    processId  ID of process to be added
	*    security   security string for authentication
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMRemoveTrustedProcess(unsigned long processId, const std::string &security) = 0;
	/// Remove a process from the non-persistent trusted process list
	/**
	* @pre
	*    The caller process needs to be a trusted process.
	*    RPMAddTrustedProcess function needs to be called to add the specified process to the non-persistent trusted process list.
	* @param
	*    processId  ID of process to be removed
	*    security   security string for authentication
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMAddTrustedApp(const std::wstring &appPath, const std::string &security) = 0;
	/// Add an application to the non-persistent trusted application list
	/**
	* @pre
	*    The caller process needs to be a trusted process.
	* @param
	*    appPath    full path to the .exe file of the application
	*    security   security string for authentication
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMRemoveTrustedApp(const std::wstring &appPath, const std::string &security) = 0;
	/// Remove an application from the non-persistent trusted application list
	/**
	* @pre
	*    The caller process needs to be a trusted process.
	*    RPMAddTrustedApp function needs to be called to add the specified application to the non-persistent trusted application list.
	* @param
	*    appPath    full path to the .exe file of the application
	*    security   security string for authentication
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMGetSecretDir(std::wstring &path) = 0;
	/// Get RPM Secret Directory
	/**
	* @param
	*    path          string to return Secret Directory path
	* @return
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMEditCopyFile(const std::wstring &filepath, std::wstring& destpath) = 0;
	/// Re-protect a local file
	/**
	* @param
	*    filepath			file path of the NXL file which will be edited
	*	 destpath			empty or a RPM folder path (if empty, we will use RPM Secret Directory)
	* @return
	*	 destpath			the generated plain file full path
	*			SDWL_PATH_NOT_FOUND			can't open the original file
	*           SDWL_INTERNAL_ERROR			fail to create nxl file, check return message for detail info.
	*
	*/

  virtual SDWLResult RPMEditSaveFile(const std::wstring& filepath, const std::wstring& originalNXLfilePath = L"", bool deletesource = false, uint32_t exitedit = 0, const std::wstring& tags = L"") = 0;
	/// Re-protect a local file
	/**
	* @param
	*    filepath				full path to updated plain file
	*	 originalNXLfilePath	full path to the original nxl file; if empty "", we will search local mapping or check current folder
	*	 deletesource			delete source NXL file 
	*	 exitedit				0, not exit and not save
	*							1, not exit, but save
	*							2, exit and not save
	*							3 (and others), exit and save
  *    tags           if tags not empty, will use new tags to protect file
	* @return
	*			SDWL_PATH_NOT_FOUND			can't open the original file
	*           SDWL_INTERNAL_ERROR			fail to create nxl file, check return message for detail info.
	*
	*/

	virtual SDWLResult RPMGetRights(const std::wstring& filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks) = 0;
	/// Get the rights of plain file
	/**
	* @param
	*    filepath				full path to the plain file
	*	 rightsAndWatermarks				rights and watermark for the file for current user
	* @return
	*			SDWL_PATH_NOT_FOUND			can't open the original file
	*
	*/

	virtual SDWLResult IsRPMFolder(const std::wstring &path, uint32_t* dirstatus, SDRmRPMFolderOption* option, std::wstring& filetags) = 0;
	/// Check directory is RPM folder
	/**
	* @param
	*     path          directory
	*     dirstatus     directory status, zero or more RPM_SAFEDIRRELATION_xxx flags OR'ed together
	*	  option		one or more of SDRmRPMFolderOption OR'ed together
	*	  If RPMFOLDER_AUTOPROTECT is returned:
	*		  filetags		tags to be applied when files are copied or moved into out of the directory and protected
	* @return
	*			SDWL_INVALID_DATA			invalid path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	virtual SDWLResult IsSanctuaryFolder(const std::wstring &path, uint32_t* dirstatus, std::wstring& filetags) = 0;
	/// Check directory is RPM folder
	/**
	* @param
	*     path          directory
	*     dirstatus     directory status, zero or more RPM_SANCTUARYDIRRELATION_xxx flags OR'ed together
	*     filetags      tags to be applied when files are copied or moved out of the directory and protected
	* @return
	*			SDWL_INVALID_DATA			invalid path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	virtual SDWLResult RPMGetCurrentUserInfo(std::wstring& router, std::wstring& tenant, std::wstring& workingfolder, std::wstring& tempfolder, std::wstring& sdklibfolder, bool &blogin) = 0;
	/// Get user login information, RPM must be already login
	/**
	* @param
	*	  router			router
	*	  tenant			tenant id
	*	  workingfolder		working folder
	*	  tempfolder		temp folder
	*	  sdklibfolder		sdk lib folder
	*	  blogin			output		user is logined or not
	* @return
	*           0                           success
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMDeleteFile(const std::wstring &filepath) = 0;
	/// Check directory is RPM folder
	/**
	* @param
	*	  filepath		directory
	* @return
	*			SDWL_INVALID_DATA			invalid file path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMCopyFile(const std::wstring &srcpath, const std::wstring &destpath, bool deletesource = false) = 0;
	/// Delete file via service
	/**
	* @param
	*	  srcpath		source file path
	*	  destpath		dest file path
	*	  deletesource	delete source file after copy
	* @return
	*			SDWL_INVALID_DATA			invalid file path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMGetFileRights(const std::wstring &filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, int option = 1) = 0;
	/**
	*  @param
	*	  filepath		file name
	*     rights        file rights
	*	  option		0: get user rights on file; 1: get file rights set in NXL header if ad-hoc file
	*
	* @return
	*			SDWL_INVALID_DATA			invalid file path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMGetFileStatus(const std::wstring &filename, uint32_t* dirstatus, bool* filestatus) = 0;
	/**
	*  @param
	*	  filename		file name
	*     dirstatus     file status, zero or more RPM_SAFEDIRRELATION_xxx flags OR'ed together
	*     filestatus    true: nxl file, false: non-nxl file
	*
	* @return
	*			SDWL_INVALID_DATA			invalid file path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMSetAppRegistry(const std::wstring& subkey, const std::wstring& name, const std::wstring& data, uint32_t op, const std::string &security) = 0;
	/**
	*  @param
	*	  subkey		the name of the registry subkey to be opened
	*     name          the name of the value to be set
	*     data          the data to be stored
	*     op            0: set data, 1: remove name 
	*     security      security code to authenticate with RPM Service
	*
	* @return
	*			SDWL_INVALID_DATA			invalid file path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMIsAppRegistered(const std::wstring &appPath, bool& registered ) = 0;
	/// Check an application is registered or not
	/**
	* @param
	*    appPath     full path to the .exe file of the application
	*    registered  application registered return true else return false: 
	* @return
	*           SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMRequestLogin(const std::wstring &callback_cmd, const std::wstring &callback_cmd_param) = 0;
	/// Ask to do login
	/**
	* @param
	*    callback_cmd		 when login successfully, it shall run this COMMAND as the callback
	*    callback_cmd_param	 it is the callback command's run param
	* @return
	*           SDWL_INTERNAL_ERROR         Error like there is already a user logined
	*/

	virtual SDWLResult RPMRequestLogout(bool* isAllow, uint32_t option = 0) = 0;
	/// Request if allow logout by sending broadcast message, so all recipients(e.g. rmd, viewer) should register
	/// the message code(uint type) both 'WM_CHECK_IF_ALLOW_LOGOUT'(50005) and 'WM_START_LOGOUT_ACTION'(50006) if want to listen.
	/**
	* @param
	*    isAllow     the result that if allow logout, the value is true or false.
	*    option		 0:execute logout action (default value); 1:Check if allow to logout now.
	* @return
	*           SDWL_INTERNAL_ERROR   Error like send broadcast message failed, user not login etc.
	*/

	virtual SDWLResult RPMNotifyMessage(const std::wstring &app, const std::wstring &target, const std::wstring &message, uint32_t msgtype = 0, const std::wstring &operation = L"", uint32_t result = 0, uint32_t fileStatus = 0) = 0;
	/// Notify or log messages to RPM service manager
	/**
	* @param
	*    app		 application name (caller)
	*    target		 the target of this activity (example, a new protected file name)
	*    message	 the message needs to be logged or notified (example, "You are not authorized to view the file.")
	*    msgtyp		 0: log message; 1: popup bubble to notify
	*    operation	 the operation of the activity (example, "Upload")
	*    result		 the operation result, 0: failed, 1: succeed
	*    fileStatus  mainly used for rmd, display file icon by file status. 0: Unknown, 1: Online, 2: Offline, 3: WaitingUpload
	* @return
	*           SDWL_INTERNAL_ERROR         User not login
	*/
	virtual SDWLResult RPMReadFileTags(const std::wstring& filepath, std::wstring &tags) = 0; 
	/// File tag section of the NXL file.
	/// this is normally called by trusted application on NXL file under RPM folder
	// tags will be returned with json format
	/**
	* @param
	*    path		 NXL file path (under RPM folder)
	*    tags		 string of tag section in json format
	* @return
	*           SDWL_INTERNAL_ERROR         User not login
	*/
	   

	// wait for more good encapsulation
	virtual SDWLResult RPMSetViewOverlay(
		void* target_window,  // using as HANDLE
		const std::wstring& overlay_text,
		// set color for ARGB, defualt is [75,150,75,75]
		const std::tuple<unsigned char, unsigned char, unsigned char, unsigned char>& font_color,
		const std::wstring& font_name,
		int font_size,  // recomment [10,100]
		int font_rotation,
		int font_sytle,   // 0:regular, 1:bold, 2 italic 3 bold_italic
		int text_alignment,   //  0 left, 1 centre, 2 right 
		const std::tuple<int,int,int,int>& display_offset // {left,upper,right,bottom},  offset of effect overlay display area
	) = 0;

	/*
	* @param
	*    target_window	 window handle which will be attached with WaterMark
	*    watermark		 struct of WaterMark, including text, font, font size, font color, rotation
	*					 font text support these macros, $(user), $(date), $(time), $(break), $(host)
	*    display_offset	 WaterMark Window' offset to target window
	* @return
	*     SDWL_INTERNAL_ERROR         User not login
	*/
	virtual SDWLResult RPMSetViewOverlay(void * target_window, const SDR_WATERMARK_INFO &watermark, const std::tuple<int, int, int, int>& display_offset = { 0,0,0,0 }) = 0;

	/*
	* @param
	*    target_window	 window handle which will be attached with WaterMark
	*    nxlfilepath	 NXL file of which the watermark defined in this NXL file will be used
	*					 struct of WaterMark, including text, font, font size, font color, rotation
	*					 font text support these macros, $(user), $(date), $(time), $(break), $(host)
	*								$(tags-in-NXL-header), example $(security_level)
	*                    note: the nxl file is not in RPM folder 
	*    display_offset	 WaterMark Window' offset to target window
	* @return
	*     SDWL_INTERNAL_ERROR         User not login
	*/
	virtual SDWLResult RPMSetViewOverlay(void * target_window, const std::wstring &nxlfilepath, const std::tuple<int, int, int, int>& display_offset = { 0,0,0,0 }) = 0;

	virtual SDWLResult RPMClearViewOverlay(
		void* target_window
	) = 0;

	virtual SDWLResult RPMClearViewOverlayAll() = 0;

	virtual SDWLResult RPMRequestRunRegCmd(const std::string& reg_cmd, const std::string &security) = 0;
	/// Request service to execute registry operation, for easy to use, please use CRegistryServiceEntry
	/**
	* @param
	*    reg_cmd	json format command, service will parse this and execute
	*    security	security token
	* @return
	*           GetCode 0 for success, else failed
	*/

	virtual SDWLResult RPMDeleteFolder(const std::wstring &folderpath) = 0;
	/// Check directory is RPM folder
	/**
	* @param
	*	  filepath		directory
	* @return
	*			SDWL_INVALID_DATA			invalid file path
	*			SDWL_NOT_READY              RPM driver/service is not ready
	*/

	virtual SDWLResult RPMRegisterFileAssociation(const std::wstring& fileextension, const std::wstring& apppath, const std::string &security) = 0;
	/// Register file association
	/**
	* @param
	*		fileexetionsion	like: .txt, .doc
	*		apppath: if apppath is empty, SDK will auto fill the apppath, else will use apppath directly
	* @return
	*			SDWL_NOT_FOUND				Can't find the file associatoin.
	*			SDWL_INVALID_DATA			m_pInstance is NULL
	*/

	virtual SDWLResult RPMUnRegisterFileAssociation(const std::wstring& fileextension, const std::string &security) = 0;
	/// UnRegister file association
	/**
	* @param
	*		fileexetionsion	like: .txt, .doc
	* @return
	*			SDWL_INVALID_DATA			m_pInstance is NULL
	*/

	virtual SDWLResult RPMGetOpenedFileRights(std::map<std::wstring, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>>& mapOpenedFileRights, const std::string &security, std::wstring &ex_useremail, unsigned long ulProcessId) = 0;
	/**
	*  @param
	*	mapOpenedFileRights all the file and corresponding rights by opened by the process
	*	security	security token
	*	ulProcessId	0 means current process, else specify process
	*
	* @return
	*
	*/
	virtual void RPMSwitchTransport(unsigned int option) = 0;
	/**
	*  @param
	*	option	
	*		0 pipeline communication
	*		1 shared memory
	*
	* @return
	*
	*/

	virtual SDWLResult RPMGetFileInfo(const std::wstring &filepath, std::string &duid, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &userRightsAndWatermarks,
		std::vector<SDRmFileRight> &rights, SDR_WATERMARK_INFO &waterMark, SDR_Expiration &expiration, std::string &tags,
		std::string &tokengroup, std::string &creatorid, std::string &infoext, DWORD &attributes, DWORD &isRPMFolder, DWORD &isNXLFile, bool checkOwner = true) = 0;

	
	virtual SDWLResult RPMSetFileAttributes(const std::wstring &nxlFilePath, DWORD dwFileAttributes) = 0;
	/**
	* @param
	*    nxlFilePath		NXL file path
	*    dwFileAttributes	The file attributes to set for the file. This parameter can be one or more values, the values same like Win API SetFileAttributes:
	*                           FILE_ATTRIBUTE_ARCHIVE 32 (0x20)
	*                           FILE_ATTRIBUTE_HIDDEN 2 (0x2)
	*                           FILE_ATTRIBUTE_NORMAL 128 (0x80)
	*                           FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 8192 (0x2000)
	*                           File_ATTRIBUTE_OFFLINE 4096 (0x1000)
	*                           FILE_ATTRIBUTE_READONLY 1 (0x1)
	*                           FILE_ATTRIBUTE_SYSTEM 4 (0x4)
	*                           FILE_ATTRIBUTE_TEMPORARY 256 (0x100)
	* @return
	*    Result: return RESULT(0) when success
	*/

	virtual SDWLResult RPMGetFileAttributes(const std::wstring &nxlFilePath, DWORD &dwFileAttributes)=0;

	virtual SDWLResult RPMWindowsEncryptFile(const std::wstring &FilePath) = 0;
};

