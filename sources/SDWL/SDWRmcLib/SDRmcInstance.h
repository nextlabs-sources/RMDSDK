#pragma once
#include "nxversion.h"
#include "SDLInstance.h"
#include "SDLResult.h"
#include "Winutil/keym.h"

#include "SDRmTenant.h"
#include "SDRmUser.h"
#include "rmccore\restful\rmsyspara.h"
#include "SDRmLoginHttpRequest.h"
#include "SDRSecureFile.h"
#include "RPM\drvcore_mgr.h"

const DWORD SDWIN_LIB_VERSION_NUMBER = (VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (BUILD_NUMBER << 0);

#include <string>
namespace SkyDRM {
	class CSDRmcInstance :
		public ISDRmcInstance
	{
	public:
		CSDRmcInstance();
		CSDRmcInstance(const std::string &productName, uint32_t productMajorVer, uint32_t productMinorVer, uint32_t productBuild, const std::wstring &sdklibfolder, const std::wstring &tempfolder = L"", const std::string &clientid = "", RMPlatformID id = RMPlatformID::RPWindowsDesktop);
		~CSDRmcInstance();
	public:
		void Init(const std::string &productName, uint32_t productMajorVer, uint32_t productMinorVer, uint32_t productBuild, const std::wstring &sdklibfolder, const std::wstring &tempfolder = L"", const std::string &clientid = "", RMPlatformID id = RMPlatformID::RPWindowsDesktop);
		SDWLResult Initialize(const std::wstring &router, const std::wstring &tenant);
		SDWLResult Initialize(const std::wstring &workingfolder, const std::wstring &router, const std::wstring &tenant);
		SDWLResult IsInitFinished(bool &finished);

		SDWLResult CheckSoftwareUpdate(std::string &newVersionStr, std::string &downloadURL, std::string &checksum);
		SDWLResult Save();
		SDWLResult GetCurrentTenant(ISDRmTenant ** ptenant);
		SDWLResult GetLoginRequest(ISDRmHttpRequest **prequest);
		SDWLResult SetLoginResult(const std::string &jsonreturn, ISDRmUser **puser, const std::string &security);
		SDWLResult GetLoginData(const std::string &useremail, const std::string &passcode, std::string &data);

		SDWLResult GetLoginUser(const std::string &useremail, const std::string &passcode, ISDRmUser **puser);
		SDWLResult Logout();
		SDWLResult SetRPMRegKey(std::wstring router, std::wstring tenant);
	
		SDWLResult SetWorkingFolder(std::wstring workingfolder);
		SDWLResult QueryTenantInfo(CSDRmTenant & info);
		SDWLResult UpdateTenantInfo(CSDRmTenant & info);
		SDWLResult SyncUserAttributes();

		// RPM functions
		SDWLResult RPMAddCachedToken(const RMToken & token);
		SDWLResult RPMRemoveCachedToken(const std::string &duid);
		SDWLResult RPMDeleteFileToken(const std::wstring &filename);

		SDWLResult RPMRegisterApp(const std::wstring &appPath, const std::string &security);
		SDWLResult RPMUnregisterApp(const std::wstring &appPath, const std::string &security);
		SDWLResult RPMNotifyRMXStatus(bool running, const std::string &security);
		SDWLResult RPMAddTrustedProcess(unsigned long processId, const std::string &security);
		SDWLResult RPMRemoveTrustedProcess(unsigned long processId, const std::string &security);
		SDWLResult RPMAddTrustedApp(const std::wstring &appPath, const std::string &security);
		SDWLResult RPMRemoveTrustedApp(const std::wstring &appPath, const std::string &security);
		SDWLResult RPMGetSecretDir(std::wstring &path);
		SDWLResult RPMEditCopyFile(const std::wstring &filepath, std::wstring& destpath);
    SDWLResult RPMEditSaveFile(const std::wstring& filepath, const std::wstring& originalNXLfilePath = L"", bool deletesource = false, uint32_t exitedit = 0, const std::wstring& tags = L"");
		SDWLResult RPMGetRights(const std::wstring& filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks);
		SDWLResult RPMGetLoginUser(const std::string &passcode, ISDRmUser **puser);
		SDWLResult IsRPMFolder(const std::wstring &path, uint32_t* dirstatus, SDRmRPMFolderOption* option, std::wstring& filetags);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		SDWLResult IsSanctuaryFolder(const std::wstring &path, uint32_t* dirstatus, std::wstring& filetags);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		SDWLResult RPMSetAppRegistry(const std::wstring& subkey, const std::wstring& name, const std::wstring& data, uint32_t op, const std::string &security);
		SDWLResult RPMIsAppRegistered(const std::wstring &appPath, bool& registered);
		SDWLResult RPMPopupNewToken(const std::wstring &membershipid, RMToken &token);
		SDWLResult RPMFindCachedToken(const std::wstring &duid, RMToken &token, time_t &ttl);
        SDWLResult RPMAddActivityLog(const std::string& strJson);
        SDWLResult RPMAddNXLMetadata(const std::string& strJson);
		SDWLResult RPMRequestLogin(const std::wstring &callback_cmd, const std::wstring &callback_cmd_param);
		SDWLResult RPMRequestLogout(bool* isAllow, uint32_t option = 0);
		SDWLResult RPMNotifyMessage(const std::wstring &app, const std::wstring &target, const std::wstring &message, uint32_t msgtype = 0, const std::wstring &operation = L"", uint32_t result = 0, uint32_t fileStatus = 0);
		// msgtype:		0: cmd; 1: log; 2: notification
		SDWLResult RPMReadFileTags(const std::wstring& filepath, std::wstring &tags); // tags will be returned with json format
        SDWLResult RPMLockFileForSync(const std::string& strJson);
		SDWLResult RPMRequestRunRegCmd(const std::string& reg_cmd, const std::string &security);

		SDWLResult RPMRegisterFileAssociation(const std::wstring& fileextension, const std::wstring& apppath, const std::string &security);
		SDWLResult RPMUnRegisterFileAssociation(const std::wstring& fileextension, const std::string &security);

		SDWLResult RPMGetOpenedFileRights(std::map<std::wstring, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>>& mapOpenedFileRights, const std::string &security, std::wstring &ex_useremail, unsigned long ulProcessId);

		SDWLResult RPMQueryAPIUser(std::wstring &apiuser_logindata);

	    // rpm driver
		bool IsRPMDriverExist();

		SDWLResult AddRPMDir(const std::wstring &path, uint32_t option = (SDRmRPMFolderOption::RPMFOLDER_NORMAL | SDRmRPMFolderOption::RPMFOLDER_API), const std::string &filetags = "");
		SDWLResult RemoveRPMDir(const std::wstring &path, bool bForce = false);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		SDWLResult AddSanctuaryDir(const std::wstring &path, const std::string &filetags = "");
		SDWLResult RemoveSanctuaryDir(const std::wstring &path);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		SDWLResult GetRPMDir(std::vector<std::wstring> &paths, SDRmRPMFolderQuery option = RPMFOLDER_MYAPIFOLDERS);
		SDWLResult SetRPMLoginResult(const std::string &jsonreturn, const std::string &security);
		SDWLResult RPMLogout();
		SDWLResult SetRPMServiceStop(bool enable, const std::string &security);
		SDWLResult StopPDP(const std::string &security);
		SDWLResult RPMStartPDP(const std::string &security);
		SDWLResult SetRPMDeleteCacheToken();
		SDWLResult RPMGetCurrentUserInfo(std::wstring& router, std::wstring& tenant, std::wstring& workingfolder, std::wstring& tempfolder, std::wstring& sdklibfolder, bool &blogin);
		SDWLResult RPMDeleteFile(const std::wstring &filepath);
		SDWLResult RPMDeleteFolder(const std::wstring &folderpath);
		SDWLResult RPMCopyFile(const std::wstring &srcpath, const std::wstring &destpath, bool deletesource = false);

		SDWLResult RPMGetFileRights(const std::wstring &filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, int option = 1); // option: 0, user rights on file; 1, file rights set on NXL header
		SDWLResult RPMGetFileStatus(const std::wstring &filename, uint32_t* dirstatus, bool* filestatus);
		SDWLResult RPMGetFileInfo(const std::wstring &filepath, std::string &duid, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &userRightsAndWatermarks,
			std::vector<SDRmFileRight> &rights, SDR_WATERMARK_INFO &waterMark, SDR_Expiration &expiration, std::string &tags,
			std::string &tokengroup, std::string &creatorid, std::string &infoext, DWORD &attributes, DWORD &isRPMFolder, DWORD &isNXLFile, bool checkOwner = true);
		
		/**
		* @param
		*    filePaht the nxl file path
		*    dwFileAttributes	The file attributes to set for the file. This parameter can be one or more values, the values same like Win API SetFileAttributes:
		*
			FILE_ATTRIBUTE_ARCHIVE 32 (0x20)

			FILE_ATTRIBUTE_HIDDEN 2 (0x2)

			FILE_ATTRIBUTE_NORMAL 128 (0x80)

			FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 8192 (0x2000)

			FILE_ATTRIBUTE_OFFLINE 4096 (0x1000)

			FILE_ATTRIBUTE_READONLY 1 (0x1)

			FILE_ATTRIBUTE_SYSTEM 4 (0x4)

			FILE_ATTRIBUTE_TEMPORARY 256 (0x100)

		* @return
		*    Result: return RESULT(0) when success
		*/
		SDWLResult RPMSetFileAttributes(const std::wstring &nxlFilePaht, DWORD dwFileAttributes);
		SDWLResult RPMGetFileAttributes(const std::wstring &nxlFilePath, DWORD &dwFileAttributes);
		SDWLResult RPMWindowsEncryptFile(const std::wstring &FilePath);

		void RPMResetNXLinFolder(const std::wstring &filepath);

		// Inherited via ISDRmcInstance
		virtual SDWLResult RPMSetViewOverlay(void * target_window, 
			const std::wstring & overlay_text, 
			const std::tuple<unsigned char, unsigned char, unsigned char, unsigned char>& font_color,
			const std::wstring & font_name, 
			int font_size, int font_rotation, int font_sytle, int text_alignment,
			const std::tuple<int, int, int, int>& display_offset) override;
		virtual SDWLResult RPMSetViewOverlay(void * target_window, const std::wstring &nxlfilepath, const std::tuple<int, int, int, int>& display_offset = { 0,0,0,0 }) override;
		virtual SDWLResult RPMSetViewOverlay(void * target_window, const SDR_WATERMARK_INFO &watermark, const std::tuple<int, int, int, int>& display_offset = { 0,0,0,0 }) override;
		virtual SDWLResult RPMClearViewOverlay(void * target_window) override;

		virtual SDWLResult RPMClearViewOverlayAll() override;

		// Set Transport Option
		void RPMSwitchTransport(unsigned int option) { m_drvcoreMgr.setTransportOption(option); }
		BOOL RPMIsAPIUser();

	private:
		SDWLResult SetRPMClientId(const std::string &clientid);
		SDWLResult SetRPMUserAttr(const std::string &jsonreturn);

	private:		
		SDWLResult SaveTenantInfo(void);
		SDWLResult SaveInstanceInfo(void);
		SDWLResult SaveCurrentUser(void);
		SDWLResult SaveLoginInfoForSSO(void);

		SDWLResult LoadTenantInfo(const std::wstring& folder);
		SDWLResult LoadInstanceInfo(void);
		SDWLResult LoadUserInfo(const std::wstring& userid);		

		// following APIs are to assist RPM Folder create
		void load_nxl_files(const std::wstring& folder, std::vector<std::wstring> &v);
		void reset_nxl_files(std::vector<std::wstring> &v);

        bool IsFileTagValid(
            const std::string& tag,
            std::string& outTag);

	private:
		NX::RmcKeyManager	m_keyManager;
		SDWLResult PrepareClientKey(void);
		SDWLResult CheckKeyManager(void);
		SDWLResult QueryTenantServerInfo(CSDRmTenant& info);

		SDWLResult SecureFileRead(const std::wstring& file, std::string& s);
		SDWLResult SecureFileWrite(const std::wstring& file, const std::string& s);

	private:
		std::wstring m_strTempFolder;
		std::wstring m_strWorkFolder;
		std::wstring m_strSDKLibFolder;

		std::wstring m_strDeviceID;
		std::string m_clientid;
		std::string m_loginData;
		std::wstring m_rpmSecretDir;

		RMCCORE::RMSystemPara m_sysparam;
		std::vector<CSDRmTenant> m_arrTenant;

		CSDRmTenant m_CurTenant;
		CSDRmLoginHttpRequest m_LoginRequest;
		CSDRmUser m_CurUser;
		SDRSecureFile   m_File;

		SDWLResult AddTenant(CSDRmTenant info);
		drvcore_mgr m_drvcoreMgr;
		CRITICAL_SECTION    _lock;
		NX::SDWPDP m_PDP;

		
};

}//SkyDRM