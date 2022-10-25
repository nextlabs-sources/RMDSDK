/*!
 * \file SDRmUser.h
 * \date 2017/11/15 14:22
 *
 * \author hbwang
 *
*/
#pragma once
#include "SDLUser.h"
#include "SDRmTenant.h"
#include "SDLResult.h"

//RMCCore headers
#include "rmccore/restful/rmuser.h"
#include "rmccore/restful/rmtoken.h"
#include "rmccore/restful/rmmembership.h"
#include "rmccore/restful/rmproject.h"
#include "rmccore/restful/rmmydrive.h"
#include "rmccore/restful/rmclassification.h"

#include "Common/stringex.h"
#include "SDRSecureFile.h"
#include "SDRFiles.h"
#include "SDRNXLToken.h"
#include "PDP/pdp.h"
#include "SDRPartialDownload.h"

const int minTokenCount = 50;
const int requiredTokenCount = 100;

namespace SkyDRM {
	class CSDRmUser :
		public ISDRmUser, public RMCCORE::RMUser
	{
	public:
		CSDRmUser();
		~CSDRmUser();

		SDWLResult SetSystemParameters(const RMCCORE::RMSystemPara & param);
		SDWLResult SetSDKLibFolder(const std::wstring & path);
		SDWLResult IsInitFinished(bool& finished);
		SDWLResult SetTenant(const RMCCORE::RMTenant & tenant);
		const std::wstring GetName() { return NX::conv::to_wstring(m_name); }
		const std::wstring GetEmail() { return NX::conv::to_wstring(m_email); }
		USER_IDPTYPE GetIdpType() { return (USER_IDPTYPE) m_idptype; }
		unsigned int GetUserID() { return m_userid;}
		const std::string GetPasscode() { return m_passcode; }
		SDWLResult GetMyDriveInfo(uint64_t& usage, uint64_t& totalquota, uint64_t& vaultusage, uint64_t& vaultquota);
		
        SDWLResult UploadFile(
            const std::wstring& nxlFilePath, 
            const std::wstring& sourcePath = L"", 
            const std::wstring& recipientEmails = L"", 
            const std::wstring& comments = L"",
            bool bOverwrite = false);
		
		SDWLResult UploadProjectFile(uint32_t projectid, const std::wstring&destfolder, ISDRmNXLFile * file, int uploadType, bool userConfirmedFileOverwrite = false);

		bool IsUserLogin();
		SDWLResult AddActivityLog(const std::wstring& nxlFilePath, RM_ActivityLogOperation op, RM_ActivityLogResult result);
		SDWLResult AddActivityLog(ISDRmNXLFile * file, RM_ActivityLogOperation op, RM_ActivityLogResult result);
		SDWLResult AddActivityLog(const RMActivityLog &act_log);
		virtual SDWLResult GetFingerPrintWithoutToken(const std::wstring& nxlFilePath, SDR_NXL_FILE_FINGER_PRINT& fingerPrint) override;
		virtual SDWLResult GetFingerPrint(const std::wstring& nxlFilePath, SDR_NXL_FILE_FINGER_PRINT& fingerPrint, bool doOwnerCheck = false) override;
		SDWLResult GetNxlFileOriginalExtention(const std::wstring &nxlFilePath, std::wstring &oriFileExtention);
		SDWLResult GetNXLFileActivityLog(ISDRmNXLFile * file, uint64_t startPos, uint64_t count, RM_ActivityLogSearchField searchField, const std::string &searchText, RM_ActivityLogOrderBy orderByField, bool orderByReverse);
		SDWLResult GetActivityInfo(const std::wstring &fileName, std::vector<SDR_FILE_ACTIVITY_INFO>& info);
		SDWLResult GetListProjects(uint32_t pageId, uint32_t pageSize, const std::string& orderBy, RM_ProjectFilter filter);
		SDWLResult GetListProjectAdmins();
		SDWLResult GetTenantAdmins();

		SDWLResult ProjectDownloadFile(const unsigned int projectid, const std::string& pathid, std::wstring& downloadPath, RM_ProjectFileDownloadType type);
		SDWLResult GetProjectListFiles(const uint32_t projectid, uint32_t pageId, uint32_t pageSize, const std::string& orderBy,
			                           const std::string& pathId, const std::string& searchString, std::vector<SDR_PROJECT_FILE_INFO>& listfiles);

        SDWLResult ProjectListFile(
            const uint32_t projectId,
            uint32_t pageId,
            uint32_t pageSize,
            const std::string& orderBy,
            const std::string& pathId,
            const std::string& searchString,
            RM_FILTER_TYPE filter,
            std::vector<SDR_PROJECT_SHARED_FILE>& listfiles);

        SDWLResult ProjectListSharedWithMeFiles(
            uint32_t projectId,
            uint32_t pageId,
            uint32_t pageSize,
            const std::string& orderBy,
            const std::string& searchString,
            std::vector<SDR_PROJECT_SHAREDWITHME_FILE>& listfiles);

        SDWLResult ProjectDownloadSharedWithMeFile(
            uint32_t projectId,
            const std::wstring& transactionCode,
            const std::wstring& transactionId,
            std::wstring& targetFolder,
            bool forViewer);

        SDWLResult ProjectPartialDownloadSharedWithMeFile(
            uint32_t projectId,
            const std::wstring& transactionCode,
            const std::wstring& transactionId,
            std::wstring& targetFolder,
            bool forViewer);

        SDWLResult ProjectReshareSharedWithMeFile(
            uint32_t projectId,
            const std::string& transactionId,
            const std::string& transactionCode,
            const std::string emaillist,
            const std::vector<uint32_t>& recipients,
            SDR_PROJECT_RESHARE_FILE_RESULT& result);

        SDWLResult ProjectUpdateSharedFileRecipients(
            const std::string& duid,
            const std::map<std::string, std::vector<uint32_t>>& recipients,
            const std::string& comment,
            std::map<std::string, std::vector<uint32_t>>& results);

        SDWLResult ProjectShareFile(
            uint32_t projectId,
            const std::vector<uint32_t> &recipients,
            const std::string &fileName,
            const std::string &filePathId,
            const std::string &filePath,
            const std::string &comment,
            SDR_PROJECT_SHARE_FILE_RESULT& result);

        SDWLResult ProjectRevokeSharedFile(const std::string& duid);

        SDWLResult ProjectFileIsExist(
            uint32_t projectId,
            const std::string& pathId,
            bool& bExist);

        SDWLResult ProjectGetNxlFileHeader(
            uint32_t projectId,
            const std::string& pathId,
            std::string& targetFolder);

        SDWLResult ProjectFileOverwrite(
            uint32_t projectid,
            const std::wstring &parentPathId,
            ISDRmNXLFile* file,
            bool overwrite = false);

        SDWLResult GetRepositories(std::vector<SDR_REPOSITORY>& vecRepository);

        SDWLResult GetRepositoryAccessToken(
            const std::wstring& repoId,
            std::wstring& accessToken);

        SDWLResult GetRepositoryAuthorizationUrl(
            const std::wstring& type,
            const std::wstring& name,
            std::wstring& authURL);

        SDWLResult UpdateRepository(
            const std::wstring& repoId,
            const std::wstring& token,
            const std::wstring& name);

        SDWLResult RemoveRepository(const std::wstring& repoId);

        SDWLResult AddRepository(
            const std::wstring& name,
            const std::wstring& type,
            const std::wstring& accountName,
            const std::wstring& accountId,
            const std::wstring& token,
            const std::wstring& perference,
            uint64_t createTime,
            bool isShared,
            std::wstring& repoId);

        SDWLResult GetRepositoryServiceProvider(std::vector<std::wstring>& serviceProvider);

        SDWLResult MyVaultFileIsExist(
            const std::string& pathId,
            bool& bExist);

        SDWLResult MyVaultGetNxlFileHeader(
            const std::string& pathId,
            std::string& targetFolder);


		SDWLResult MyVaultDownloadFile(const std::string& downloadPath, std::wstring& targetFolder, uint32_t downloadtype);
		SDWLResult GetMyVaultFiles(uint32_t pageId, uint32_t pageSize, const std::string& orderBy,
			const std::string& searchString, std::vector<SDR_MYVAULT_FILE_INFO>& listfiles);

		SDWLResult MyDriveListFiles(const std::wstring& pathId, std::vector<SDR_MYDRIVE_FILE_INFO>& listfiles);
		SDWLResult MyDriveDownloadFile(const std::wstring& downloadPath, std::wstring& targetFolder);
		
        SDWLResult MyDriveUploadFile(
            const std::wstring& pathId, 
            const std::wstring& parentPathId, 
            bool overwrite = false);
		
        SDWLResult MyDriveCreateFolder(const std::wstring&name, const std::wstring&parentfolder);
		SDWLResult MyDriveDeleteItem(const std::wstring&pathId);
		SDWLResult MyDriveCopyItem(const std::wstring&srcPathId, const std::wstring&destPathId);
		SDWLResult MyDriveMoveItem(const std::wstring&srcPathId, const std::wstring&destPathId);
		SDWLResult MyDriveCreateShareURL(const std::wstring&pathId, std::wstring&sharedURL);

		SDWLResult SharedWithMeDownloadFile(const std::wstring& transactionCode, const std::wstring& transactionId, std::wstring& targetFolder, bool forViewer);
		SDWLResult SharedWithMeDownloadPartialFile(const std::wstring& transactionCode, const std::wstring& transactionId, std::wstring& targetFolder, bool forViewer);
		SDWLResult SharedWithMeReShareFile(const std::string& transactionId, const std::string& transactionCode, const std::string emaillist);
		SDWLResult GetSharedWithMeFiles(uint32_t pageId, uint32_t pageSize, const std::string& orderBy,
			const std::string& searchString, std::vector<SDR_SHAREDWITHME_FILE_INFO>& listfiles);

		SDWLResult WorkspaceDownloadFile(const std::string& downloadPath, std::wstring& targetFolder, uint32_t downloadtype);
		SDWLResult GetWorkspaceFiles(
            uint32_t pageId, 
            uint32_t pageSize, 
            const std::string& path, 
            const std::string& orderBy,
			const std::string& searchString, 
            std::vector<SDR_WORKSPACE_FILE_INFO>& listfiles);
		
        SDWLResult WorkspaceFileIsExist(
            const std::string& pathId,
            bool& bExist);

        SDWLResult WorkspaceGetNxlFileHeader(
            const std::string& pathId,
            std::string& targetFolder);

        SDWLResult WorkspaceFileOverwrite(
            const std::wstring& destfolder,
            ISDRmNXLFile* file,
            bool overwrite = false);
        
        SDWLResult ChangeRightsOfWorkspaceFile(const std::wstring &originalNXLfilePath, const std::string &fileName, const std::string &parentPathId,
			const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags);
		SDWLResult ClassifyWorkspaceFile(const std::wstring &nxlfilepath, const std::string &fileName, const std::string &parentPathId, const std::string &fileTags);
		SDWLResult UploadWorkspaceFile(const std::wstring&destfolder, ISDRmNXLFile * file, bool overwrite);
		SDWLResult GetWorkspaceFileMetadata(const std::string& pathid, SDR_FILE_META_DATA& metadata);

		bool CheckRights(const std::wstring& nxlfilepath, SDRmFileRight right);
		SDWLResult GetRights(const std::wstring& nxlfilepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true);
		SDWLResult GetFileRightsFromCentralPolicies(const std::wstring& nxlFilePath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true);
		SDWLResult GetResourceRightsFromCentralPolicies(const std::wstring& resourceName, const std::wstring& resourceType, const std::vector<std::pair<std::wstring, std::wstring>> &attrs, std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> &rightsAndObligations);
		SDWLResult GetHeartBeatInfo();
		//bool GetPolicyBundle(const std::wstring& tenantName, std::string& policyBundle);
		const SDR_WATERMARK_INFO GetWaterMarkInfo();
		SDWLResult GetClassification(const std::string &tenantid, std::vector<SDR_CLASSIFICATION_CAT>& cats);
		//SDWLResult GetAllPolicyBundle(std::unordered_map<std::string, std::string>& policymp);

		SDWLResult InitializeLogin(CSDRmTenant & tenant);
        SDWLResult SetLoginResult(const std::string& strJson);
		SDWLResult LogoutUser();

		SDWLResult ProtectFile(const std::wstring &filepath, std::wstring& newcreatedfilePath, const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags = "", const std::string& memberid = "", bool usetimestamp = true);
		SDWLResult ProtectFileFrom(const std::wstring &srcplainfile, const std::wstring& originalnxlfile, std::wstring& output);
		SDWLResult ChangeRightsOfFile(const std::wstring &oldnxlfilepath, const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags = "");
		SDWLResult ChangeRightsOfProjectFile(const std::wstring &oldnxlfilepath, unsigned int projectid, const std::string &fileName, const std::string &parentPathId, const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags = "");
    SDWLResult ReProtectFile(const std::wstring& filepath, const std::wstring& originalNXLfilePath,   const std::wstring& newtags= L"");
		SDWLResult GetRecipients(ISDRmNXLFile * nxlfile, std::vector<std::string> &recipientsemail, std::vector<std::string> &addrecipientsemail, std::vector<std::string> &removerecipientsemail);
		SDWLResult UpdateRecipients(ISDRmNXLFile * nxlfile, const std::vector<std::string> &addrecipientsemail, const std::wstring &comment = L"");
		SDWLResult ShareFileFromMyVault(const std::wstring &filepath, const std::vector<std::string> &recipients, const std::string &repositoryId, const std::string &fileName, const std::string &filePathId, const std::string &filePath, const std::string &comment);
		SDWLResult CloseFile(ISDRmNXLFile * file);
		SDWLResult OpenFile(const std::wstring &nxlfilepath, ISDRmNXLFile ** file);
		bool IsFileProtected(const std::wstring &filepath);
		const std::string GetMembershipID(uint32_t projectid);
		const std::string GetMembershipID(const std::string tokengroupname);
		const std::string GetMembershipIDByTenantName(const std::string tenant);
		//SDWLResult GetFilePath(const std::wstring &filename, std::wstring &targetfilepath);
		SDWLResult UpdateUserInfo();
		SDWLResult UpdateMyDriveInfo();
		uint32_t GetHeartBeatFrequency() { return m_heartbeat.GetFrequency(); }
	
        SDWLResult ImportDataFromJson(const nlohmann::json& root);

        nlohmann::json ExportDataToJson();

        std::string BuildDynamicEvalRequest();

		SDWLResult FileRead(const std::wstring& file, std::string& s);
		SDWLResult FileWrite(const std::wstring& file, std::string& s);
		void SetRWFile(SDRSecureFile& File, const std::wstring &workFolder);

		SDWLResult GetSavedFiles();
		SDWLResult GetMiscFile();
		SDWLResult Save();
		SDWLResult SaveMisc();
		SDWLResult SaveAttribs();
		SDWLResult GetAttribsFromFile();
		std::vector<SDR_PROJECT_INFO>& GetProjectsInfo() { return m_projectsInfo; }

		SDWLResult SyncUserAttributes(std::string& jsonreturn);
		SDWLResult GetFileDuid(const std::wstring &nxlfilepath, std::string& duid);
		SDWLResult GetNXLFileMetadata(const std::wstring &nxlfilepath, const std::string& pathid, SDR_FILE_META_DATA& metadata);
		SDWLResult GetProjectFileMetadata(const std::wstring &nxlfilepath, const std::string& projectid, const std::string& pathid, SDR_FILE_META_DATA& metadata);
		SDWLResult GetProjectFileMetadata(const std::string& projectid, const std::string& pathid, SDR_FILE_META_DATA& metadata);
		SDWLResult GetProjectFileRights(const std::string& projectid, const std::string& pathid, std::vector<SDRmFileRight>& rights);
		SDWLResult GetMyVaultFileRights(const std::string& duid, const std::string& pathid, std::vector<SDRmFileRight>& rights);

		SDWLResult RPMEditCopyFile(const std::wstring &filepath, std::wstring& destpath, const std::wstring hiddenrpmfolder);
		SDWLResult RPMEditSaveFile(const std::wstring &filepath, const std::wstring& originalNXLfilePath = L"", bool deletesource = false, uint32_t exitedit = 0, const std::wstring& tags=L"");
		SDWLResult RPMGetRights(const std::wstring& filepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks);
        void get_all_dirs(const std::string& str, std::vector<std::string>& vec);
		bool IsTenantAdhocEnabled() { return m_adhoc; };
		const std::string GetSystemProjectTenantId() { return m_systemProjectTenantId; }
		const std::string GetSystemProjectTenant() { return m_systemProjectTenant; }
		const std::string GetDefaultTenantId() { return m_default_tenantid; }
		const std::string GetDefaultTokenGroupName() { return m_defaultmembership.GetTokenGroupName();}
		SDWLResult PDSetupHeader(const unsigned char* header, long header_len, int64_t* contentLength, unsigned int* contentOffset);
		SDWLResult PDDecryptPartial(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len, const unsigned char* header, long header_len);
		SDWLResult PDSetupHeaderEx(const unsigned char* header, long header_len, int64_t* contentLength, unsigned int* contentOffset, void *&context);
		SDWLResult PDDecryptPartialEx(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len, const unsigned char* header, long header_len, void *context);
		void PDTearDownHeaderEx(void *context);

		bool HasAdminRights(const std::wstring &nxlfilepath);
		SDWLResult UpdateNXLMetaData(const std::wstring &nxlfilepath, bool bRetry = true);
		SDWLResult UpdateNXLMetaData(ISDRmNXLFile* file, bool bRetry = true);
		SDWLResult UpdateNXLMetaDataEx(const std::wstring &nxlfilepath, const std::string &fileTags, const std::string &existingFileTags, const std::string &fileHeader);
		SDWLResult ClassifyProjectFile(const std::wstring &nxlfilepath, unsigned int projectid, const std::string &fileName, const std::string &parentPathId, const std::string &fileTags);
		SDWLResult ResetSourcePath(ISDRmNXLFile* file, const std::wstring & sourcepath);
		SDWLResult ResetSourcePath(const std::wstring & nxlfilepath, const std::wstring & sourcepath);
		SDWLResult LockFileSync(const std::wstring &nxlfilepath);
		SDWLResult ResumeFileSync(const std::wstring &nxlfilepath);
		SDWLResult ReProtectSystemBucketFile(const std::wstring &originalNXLfilePath);
		SDWLResult ChangeFileToMember(const std::wstring &filepath, const std::string &memberid = "");
		SDWLResult RenameFile(const std::wstring &filepath, const std::string &name = "");

		SDWLResult CopyNxlFile(const std::string& srcFileName, const std::string& srcFilePath, RM_NxlFileSpaceType srcSpaceType, const std::string& srcSpaceId,
			const std::string& destFileName, const std::string& destFilePath, RM_NxlFileSpaceType destSpaceType, const std::string& destSpaceId,
			bool bOverwrite = false, const std::string& transactionCode = "", const std::string& transactionId = "");

        SDWLResult DecryptNXLFile(ISDRmNXLFile * file, const std::wstring &targetfilepath);
		BOOL IsAPIUser();

	private:
		SDWLResult IsMembershipExist(const std::string& ID, RMCCORE::RMMembership& rm);
		std::wstring GetTargetFilePath(const std::wstring filepath, bool usetimestamp);
		void GeneratePassCode();		
		SDWLResult ProjectCreateTempFile(const std::wstring& projectId, const std::wstring& pathId, std::wstring& tmpFilePath);
		std::wstring GetTempDirectory();
		SDWLResult GetFileToken(CSDRmNXLFile* nxlfile, RMToken& token);
		SDWLResult GetFileToken(const std::string& file_ownerid, const std::string &agreement, const std::string& duid, uint32_t ml, const std::string& policy, const std::string& tags, RMToken &filetoken);
		RMToken PopToken(const std::string membershipid = "");
		RMToken QueryTokenFromRMS(const std::string membershipid, int protectionType, const std::string &fileTagsOrPolicy);

		bool CheckRights(CSDRmNXLFile* nxlfile, SDRmFileRight right);
		SDWLResult GetRights(CSDRmNXLFile* nxlfile, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks);
		SDWLResult GetFileRightsFromCentralPolicies(CSDRmNXLFile* nxlfile, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true);
		SDWLResult GetFileRightsFromCentralPolicies(const std::string &tenantname, const std::string &tags, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true);
		SDWLResult GetFileRightsFromCentralPolicies(const uint32_t projectid, const std::string &tags, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true);
		SDWLResult PDPEvaluateRights(const std::string &filetokenGroup, const std::string &filetags, const std::wstring &resourcename, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true);
		std::vector<SDR_CLASSIFICATION_CAT> CopyClassification(std::vector<CLASSIFICATION_CAT>& classif);
		SDWLResult GetCertificate(CSDRmTenant& info, RMCCORE::RMMembership* rm);
		SDWLResult OpenFileForMetaData(const std::wstring &nxlfilepath, ISDRmNXLFile ** file);
		SDWLResult OpenFileWithToken(const std::wstring &nxlfilepath, ISDRmNXLFile ** file, bool bIgnoreRight, RMToken &filetoken);
		SDWLResult FindCachedToken(const std::string &duid, RMToken & token);
		SDWLResult AddCachedToken(const RMToken & token);
		SDWLResult RemoveCachedToken(const std::string & duid);
		SDWLResult UpdateUserPreference(const uint32_t option, const uint64_t start = 0, const uint64_t end = 0, const std::wstring watermark = L"");
		SDWLResult GetUserPreference(uint32_t &option, uint64_t &start, uint64_t &end, std::wstring &watermark);
		SDWLResult GetTenantPreference(bool* adhoc, bool* workspace, int* heartbeat, int* sysprojectid, std::string &sysprojecttenantid, const std::string& tenantid = "");
		SDWLResult SyncTenantPreference(const std::string& tenantid = "");

		SDWLResult RPMEdit_Map(const std::wstring &filepath, const std::wstring &nxlfilepath);
		SDWLResult RPMEdit_UnMap(const std::wstring &filepath);
		SDWLResult RPMEdit_FindMap(const std::wstring &filepath, std::wstring &nxlfilepath);

		bool IsTenantAdmin(const std::string &email);
		bool IsProjectAdmin(const std::string &project_tenantid, const std::string &email);

        SDWLResult SaveFileToTargetFolder(
            const std::string& pathId,
            const std::wstring& tmpFilePath,
            std::string& targetFolder,
            const NX::REST::http::HttpHeaders& headers);

        bool IsFileTagValid(
            const std::string& tag,
            std::string& outTag);

		SDWLResult GetSharedWorkspaceListFiles(const std::string& repoId, uint32_t pageId, uint32_t pageSize, const std::string& orderBy, const std::string& pathId, const std::string& searchString, std::vector<SDR_SHARED_WORKSPACE_FILE_INFO>& listfiles);
		SDWLResult GetSharedWorkspaceFileMetadata(const std::string& repoId, const std::string& pathId, SDR_SHARED_WORKSPACE_FILE_METADATA& metadata);
		SDWLResult UploadSharedWorkspaceFile(const std::string& repoId, const std::wstring &destfolder, ISDRmNXLFile* file, int uploadType, bool userConfirmedFileOverwrite = false);
		SDWLResult DownloadSharedWorkspaceFile(const std::string& repoId, const std::string& pathId, std::wstring& targetFolder, uint32_t downloadtype, bool isNXL = false);
		SDWLResult IsSharedWorkspaceFileExist(const std::string& repoId, const std::string& pathId, bool& bExist);
		SDWLResult GetWorkspaceNxlFileHeader(const std::string& repoId, const std::string& pathId, std::string& targetFolder);
	private:
		NX::SDWPDP      m_PDP;
		std::string			m_passcode;

		SDRNXLToken		m_tokenslist;  // assigned to nxl file
		SDRSecureFile   m_File;
		SDRFiles        m_localFile;
		RMHeartbeat     m_heartbeat;
		Watermark       m_watermark;
		std::unordered_map<std::wstring, std::vector<SDR_FILE_ACTIVITY_INFO>> m_fileActivityMap;
		std::vector<SDR_PROJECT_INFO> m_projectsInfo;
		std::string     m_policyBundle;
		bool			m_adhoc;
		bool			m_workspace;
		int				m_heartbeatFrequency;   // tenant heartbeat frequency (in minutes)
		int				m_systemProjectId;
		std::string		m_systemProjectTenantId;
		std::string		m_systemProjectTenant;
		SDRPartialDownload m_partialDownload;

		CRITICAL_SECTION _listproject_lock;
		CRITICAL_SECTION _listprojectfiles_lock;
		CRITICAL_SECTION _downloadprojectfile_lock;

		std::vector<std::pair<std::string, std::vector<std::string>>> m_ProjectAdminList; // pair<project-tenant-id, array of tenant-admin email>
		std::vector<std::string> m_TenantAdminList; // emails

		std::map<std::string, SDR_FILE_METADATA> m_nxlmetadatalist;
	public:
		CSDRmcInstance	*m_SDRmcInstance;
	};

}