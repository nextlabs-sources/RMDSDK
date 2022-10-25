/*!
 * \file SDLUser.h
 *
 * \author hbwang
 * \date 2017/11/14 23:05
 *
 * 
 */
#pragma once
#include "SDLResult.h"
#include "SDLFiles.h"
#include "SDLNXLFile.h"
#include "SDLTypeDef.h"
#include <string>
#include <list>
#include <utility>

/*
Application call user class to protect, decrypt, share, upload and download files. User calss can get ad-hoc or central policy
information.
*/
class ISDRmUser	
{
public:
	ISDRmUser() { };
	virtual ~ISDRmUser() { };
public:
	virtual const std::wstring GetName() = 0;
	/// Get current login user name
	/**
	 * @return
	 *    User name string
	 */
	virtual const std::wstring GetEmail() = 0;
	/// Get current login user email address for SkyDRM
	/**
	 * @return
	 *    user email string
	 */

	virtual unsigned int GetUserID() = 0;
	/// Get current login user id.
	/**
	 * @return
	 *	 user id	
	 */

	virtual USER_IDPTYPE GetIdpType() = 0;
	/// Get current login user identify protocol type
	/**
	 * @return
	 *    User IDP type
	 */

	virtual SDWLResult GetMyDriveInfo(uint64_t& usage, uint64_t& totalquota, uint64_t& vaultusage, uint64_t& vaultquota) = 0;
	/// Force sync MyDrive information from RMS server
	/**
	* @param
	*    usage        storage usage
	*    totalquota   total storage available
	*    vaultusage   vault usage
	*    vaultquota   total vault available
	* @return
	*			SDWL_SUCCESS	get the current storage information.
	*           SDWL_INVALID_DATA	user not login
	*/

	virtual const std::string GetPasscode() = 0;
	/// Get current login user passcode for local login
	/**
	* @return
	*    passcode string for current user.
	*/

	virtual std::vector<SDR_PROJECT_INFO>& GetProjectsInfo() = 0;
	/// Get projects information
	/**
	* @return
	*    projects information.
	*/

	virtual const std::string GetMembershipID(uint32_t projectid) = 0;
	/// Get user's membership-id of the project
	/**
	* @param
	*    projectid
	*		0		default tenant membership id
	* @return
	*    user's membershipid of the tenant/project.
	*/

	virtual const std::string GetMembershipID(const std::string tenantid) = 0;
	/// Get user's membership-id of the tenant
	/**
	* @param
	*    tenantid
	* @return
	*    user's membershipid of the tenant/project.
	*/

	virtual const std::string GetMembershipIDByTenantName(const std::string tokengroupname) = 0;
	/// Get user's membership-id of the tenant
	/**
	* @param
	*    tokengroupname		token group name of tenant/project
	* @return
	*    user's membershipid of the tenant/project.
	*/

	virtual SDWLResult UpdateUserInfo() = 0;
	/// Force sync User information from RMS server
	/**
	* @param
	*    None
	* @return
	*			SDWL_SUCCESS	synchronized the latest user information from RMS. 
	*           SDWL_RMS_ERRORCODE_BASE+	RMS server error	 
	*/
	virtual SDWLResult UpdateMyDriveInfo() = 0;
	/// Force sync MyDrive information from RMS server
	/**
	* @param
	*    None
	* @return
	*			SDWL_SUCCESS	synchronized the latest MyDrive information from RMS.
	*           SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*/

	virtual SDWLResult ProtectFile(const std::wstring &filepath, std::wstring& newcreatedfilePath, const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags = "", const std::string& memberid = "", bool usetimestamp = true ) = 0;
	/// Protect a local file
	/**
	* @param
	*    filepath			full path to original file
	*    rights				rights assigned to the nxl file
	*	 newcreatedfilePath full path to the created nxl file
	*    watermarkinfo		watermark structure
	*    expire				SDR_Expiration structure
	*    tags				tags
	*    memberid			membership id
	*    usetimestamp		true if timestamp is to be added to nxl file name
	* @return
	*			SDWL_PATH_NOT_FOUND			can't open the original file
	*           SDWL_INTERNAL_ERROR			fail to create nxl file, check return message for detail info.
	*
	*/

	virtual SDWLResult GetRecipients(ISDRmNXLFile * nxlfile, std::vector<std::string> &recipientsemail, std::vector<std::string> &addrecipientsemail, std::vector<std::string> &removerecipientsemail) = 0;
	/// Get Recipients list 
	/**
	* @param
	*    nxlfile		nxl file handle
	*    recipientsemail			email list of recipients existed
	*    addrecipientsemail			email list of recipients added
	*    removerecipientsemail		email list of recipients removed
	* @return
	*
	*/

	virtual SDWLResult UpdateRecipients(ISDRmNXLFile * nxlfile, const std::vector<std::string> &addrecipientsemail, const std::wstring &comment = L"") = 0;
	/// Update Recipients list 
	/**
	* @param
	*    nxlfile		nxl file handle
	*    addrecipientsemail			email list of recipients to add
	*    removerecipientsemail		email list of recipients to remove
	*	 comment		email comment to the new recipients
	* @return
	*
	*/


	virtual SDWLResult UploadFile(
        const std::wstring& nxlFilePath, 
        const std::wstring& sourcePath = L"", 
        const std::wstring& recipientEmails = L"", 
        const std::wstring& comments = L"",
        bool bOverwrite = false) = 0;
	/// upload  local file 
	/**
	* @param
	*    nxlFilePath        full path to nxl
	*    sourcePath         the source file full path  
	*    recipientEmails    the emails for uploading shared file, use a comma delimiter if there are more than one.
	*    comments           the comments for uploading shared file 
	* @return
	*     SDWL_INVALID_DATA            invalid Parameter
	*
	*/
	
	virtual SDWLResult UploadProjectFile(uint32_t projectid, const std::wstring&destfolder, ISDRmNXLFile * file, int uploadType, bool userConfirmedFileOverwrite = false) = 0;
	/// upload  project file 
	/**
	* @param
	*    projectid   projectid
	*    destfolder  dest folder
	*    file        project file to  upload
	*	 uploadType  upload type for project file upload
	*    uploadType possible values:
	*   0: copy and upload operation, System will re-encrypt the file and change the DUID, store it in the project
		1: should not happen, no upload with type = 1 will be allowed in Server. RMD must change this from 1 to 2 to preserve the current behaviour. Temporarily type 1 will be internally mapped in server to type 2. 
		2: the file was downloaded for offline use, edited and is being uploaded back. Server checks for EDIT rights and permit/deny the operation.
		3: copy and upload of system bucket file/ project token group file, it will re-encrypt the file and store it in the project.
		4: upload project token group file, it will upload the file and store it in the project.
	*    userConfirmedFileOverwrite  should overwrite duplicate name file  
	* @return
	*     SDWL_INVALID_DATA            invalid Parameter
	*
	*/


	virtual SDWLResult CloseFile(ISDRmNXLFile * file) = 0;
	/// Open a local NXL file
	/**
	* @param
	*    file[in]		pointer to nxl file class handle.
	* @return
	*/

	virtual SDWLResult OpenFile(const std::wstring &nxlfilepath, ISDRmNXLFile ** file) = 0;
	/// Open a local NXL file
	/**
	* @param
	*    nxlfilepath		full path to nxl file
	*    file[out]		pointer to pointer fo nxl file class handle . NULL if failed.
	* @return
	*			SDWL_ACCESS_DENIED	User doesn't has right to open target file
	*           SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*/

	virtual SDWLResult OpenFileForMetaData(const std::wstring &nxlfilepath, ISDRmNXLFile ** file) = 0;
	/// Open a local NXL file
	/**
	* @param
	*    nxlfilepath		full path to nxl file
	*    file[out]		pointer to pointer fo nxl file class handle . NULL if failed.
	* @return
	*			SDWL_ACCESS_DENIED	User doesn't has right to open target file
	*           SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*/

	virtual SDWLResult LogoutUser() = 0;
	/// Logout current user
	/**
	* @param
	*    none
	* @return
	*            SDWL_INVALID_DATA            Invalid logout query return
	*           ERROR_INVALID_DATA          Invalid logout result
	*/

	virtual SDWLResult GetNXLFileActivityLog(ISDRmNXLFile * file, uint64_t startPos, uint64_t count, RM_ActivityLogSearchField searchField, const std::string &searchText, RM_ActivityLogOrderBy orderByField, bool orderByReverse) = 0;
	/// Get file activity log
	/**
	* @param
	*    file       nxl file class
	*    startPos   the first returned log on sort
	*    count      the maximum log number returned in this query
	*    searchField   field name to be searched.
	*    searchText    the search text in the searching filed above(empty means all values)
	*    orderByField  the ordered by field for return result
	*    orderByReverse flag for return result order
	* @return
	*            SDWL_INVALID_DATA          Invalid log query return
	*           ERROR_INVALID_DATA          Invalid log result
	*/

	
	virtual SDWLResult AddActivityLog(const std::wstring &fileName, RM_ActivityLogOperation op, RM_ActivityLogResult result) = 0;
	/// Add file activity information
	/**
	* @param
	*    fileName   nxl file name
	*    op		file activity information
	*    result		access result: deny or allow
	* @return
	*           SDWL_NOT_FOUND              File not found
	*/

	virtual SDWLResult AddActivityLog(ISDRmNXLFile * file, RM_ActivityLogOperation op, RM_ActivityLogResult result) = 0;
	/// Add file activity information
	/**
	* @param
	*    file   nxl file
	*    op		file activity information
	*    result		access result: deny or allow
	* @return
	*           SDWL_NOT_FOUND              File not found
	*/

	virtual SDWLResult GetActivityInfo(const std::wstring &fileName, std::vector<SDR_FILE_ACTIVITY_INFO>& info) = 0;
	/// Get file activity information
	/**
	* @param
	*    fileName   nxl file name
	*    info       file activity information
	* @return
	*           SDWL_NOT_FOUND              File not found
	*/

	virtual SDWLResult GetListProjects(uint32_t pageId, uint32_t pageSize, const std::string& orderBy, RM_ProjectFilter filter) = 0;
	/// Get projects list
	/**
	* @param
	*    pageId       Page number (start from 1)
	*    pageSize     Number of records to be returned
	*    orderBy      Pass string as "lastActionTime" or "name" 
	*    filter       Which projects to return
	* @return
	*           SDWL_INVALID_DATA          Invalid list projects query return
	*           ERROR_INVALID_DATA          Invalid list projects result
	*/

	virtual SDWLResult ProjectDownloadFile(const uint32_t projectid, const std::string& pathid, std::wstring& downloadPath, RM_ProjectFileDownloadType type) = 0;
	/// Project download file
	/**
	* @param
	*    project    project class
	*    pathid     pathid
	*    downloadPath  in/out
	*					in	targetfolder
	*					out	final saved file full path
	*    orderBy    order sequence
	*    bviewonly  view only
	* @return
	*            ERROR_INVALID_DATA          Invalid download file result
	*/


	virtual SDWLResult GetProjectListFiles(const uint32_t projectid, uint32_t pageId, uint32_t pageSize, const std::string& orderBy, const std::string& pathId, const std::string& searchString, std::vector<SDR_PROJECT_FILE_INFO>& listfiles) = 0;
	/// Get project files
	/**
	* @param
	*    projectid    project id
	*    pageId       Page number (start from 1)
	*    pageSize     Number of records to be returned
	*    orderBy      A comma-separated list of sort keys.
	*    pathid       pathid
	*    searchString  search specific string
	*    listfiles    return files list
	* @return
	*           SDWL_INVALID_DATA          Invalid project list files query return
	*            ERROR_INVALID_DATA          Invalid project list files result
	*/

    virtual SDWLResult ProjectListFile(
        const uint32_t projectId,
        uint32_t pageId,
        uint32_t pageSize,
        const std::string& orderBy,
        const std::string& pathId,
        const std::string& searchString,
        RM_FILTER_TYPE filter,
        std::vector<SDR_PROJECT_SHARED_FILE>& listfiles) = 0;
    /// List all the files in the project (projectId)
    /// List all the shared files in the project(projectId)
    /// List all the revoked files in the project(projectId)
    /**
    * @param
    *    projectId : project ID, example projectId = 7
    *    pageId : page number, start from 1
    *    pageSize : Number of records to be returned
    *    orderBy : A comma - separated list of sort keys, example : orderBy = "-creationTime,name"
    *    pathId : path id, example if the full "pathId" is 
    *               "/test7/normal-4-2020-03-10-15-04-16.txt.nxl", pathId can be "/test7/"
    *    searchString : search specific string, Search will be done based on name
    *    filter :  E_ALLSHARED ("allShared"), E_ALLFILES ("allFiles"), E_REVOKED ("revoked")
    *    listfiles : return the SDR_PROJECT_SHARED_FILE file list
    * @return
    *    ERROR_INVALID_DATA         Invalid project list files result
    */

    virtual SDWLResult ProjectListSharedWithMeFiles(
        uint32_t projectId,
        uint32_t pageId,
        uint32_t pageSize,
        const std::string& orderBy,
        const std::string& searchString,
        std::vector<SDR_PROJECT_SHAREDWITHME_FILE>& listfiles) = 0;
    /// List the project files shared to the specify project (projectId)
    /**
    * @param
    *    projectId : project ID, example projectId = 7
    *    pageId : page number, start from 1
    *    pageSize : Number of records to be returned
    *    orderBy : A comma - separated list of sort keys, example : orderBy = "-creationTime,name"
    *    searchString : search specific string, Search will be done based on name
    *    listfiles : return the SDR_PROJECT_SHAREDWITHME_FILE file list
    * @return
    *    ERROR_INVALID_DATA         Invalid project list files result
    */
    
    virtual SDWLResult ProjectDownloadSharedWithMeFile(
        uint32_t projectId,
        const std::wstring& transactionCode,
        const std::wstring& transactionId,
        std::wstring& targetFolder,
        bool forViewer) = 0;
    /// Download a project file from the specify project (projectId) which is shared by other project
    /**
    * @param
    *    projectId : project ID, example projectId = 7
    *    transactionCode : transaction code
    *    transactionId : transaction ID
    *    targetFolder : the folder the file will save to
    *    forViewer : if set false, the project file will be "downloaded as a copy" with system bucket token group
    * @return
    *    ERROR_FILE_EXISTS         file already exists locally
    */

    virtual SDWLResult ProjectPartialDownloadSharedWithMeFile(
        uint32_t projectId,
        const std::wstring& transactionCode,
        const std::wstring& transactionId,
        std::wstring& targetFolder,
        bool forViewer) = 0;
    /// Partially Download a project file from the specify project (projectId) which is shared by other project
    /**
    * @param
    *    projectId : project ID, example projectId = 7
    *    transactionCode : transaction code
    *    transactionId : transaction ID
    *    targetFolder : the folder the file will save to
    *    forViewer : if set false, the project file will be "downloaded as a copy" with system bucket token group
    * @return
    *    ERROR_FILE_EXISTS         file already exists locally
    */

    virtual SDWLResult ProjectReshareSharedWithMeFile(
        uint32_t projectId,
        const std::string& transactionId,
        const std::string& transactionCode,
        const std::string emaillist,
        const std::vector<uint32_t>& recipients,
        SDR_PROJECT_RESHARE_FILE_RESULT& result) = 0;
    /// Share a project file which is shared by other project to another project
    /**
    * @param
    *    projectId : project ID, example projectId = 7
    *    transactionCode : transaction code
    *    transactionId : transaction ID
    *    emaillist : email list
    *    recipients : the project ID list, the file will be reshared with
    *    result : return SDR_PROJECT_RESHARE_FILE_RESULT
    * @return
    *    ERROR_INVALID_DATA         JSON response is not correct
    */

    virtual SDWLResult ProjectUpdateSharedFileRecipients(
        const std::string& duid,
        const std::map<std::string, std::vector<uint32_t>>& recipients,
        const std::string& comment,
        std::map<std::string, std::vector<uint32_t>>& results) = 0;
    /// Update a shared file's recipients, like add a new projectId, or remove an exist projectId 
    /**
    * @param
    *    duid: nxl file duid
    *    recipients : new recipient list, remove recipient list
    *    comment : some comments for this update
    *    results : return the file's recipient status, like alreadySharedList, newRecipients
    * @return
    *    ERROR_INVALID_DATA         JSON response is not correct
    */

    virtual SDWLResult ProjectShareFile(
        uint32_t projectId,
        const std::vector<uint32_t> &recipients,
        const std::string &fileName,
        const std::string &filePathId,
        const std::string &filePath,
        const std::string &comment,
        SDR_PROJECT_SHARE_FILE_RESULT& result) = 0;
     /// Share a project file to other projects 
     /**
     * @param
     *    projectId: the project ID, the file will be shared from
     *    recipients : the project ID list, the file will be shared with
     *    fileName : the shared file's file name
     *    filePathId : the shared file's path ID
     *    filePath : the shared file's path
     *    comment : the comments for this file sharing
     *    result : return the sharing file result, SDR_PROJECT_SHARE_FILE_RESULT
     * @return
     *    ERROR_INVALID_DATA         JSON response is not correct
     */

    virtual SDWLResult ProjectRevokeSharedFile(const std::string& duid) = 0;
    /// Revoke a shared project file 
    /**
    * @param
    *    duid : the shared file's duid
    * @return
    *    ERROR_INVALID_DATA         JSON response is not correct
    */

    virtual SDWLResult ProjectFileIsExist(
        uint32_t projectId,
        const std::string& pathId,
        bool& bExist) = 0;
    /// Check if the project file is exist or not 
    /**
    * @param
    *    projectId [in]: the project ID
    *    pathId [in]: the project file pathId
    *    bExist [out]: if the file exist, bExist = true, else bExist = false
    * @return
    *    ERROR_INVALID_DATA         JSON response is not correct
    */

    virtual SDWLResult ProjectGetNxlFileHeader(
        uint32_t projectId,
        const std::string& pathId,
        std::string& targetFolder) = 0;
    /// Get the project file NXL Header 
    /**
    * @param
    *    projectId [in]: the project ID
    *    pathId [in]: the project file pathId
    *    targetFolder [in out]: pass the file SaveTo folder and return the file path
    * @return
    *    ERROR_SUCCESS
    */

    virtual SDWLResult ProjectFileOverwrite(
        uint32_t projectid,
        const std::wstring& parentPathId,
        ISDRmNXLFile* file,
        bool overwrite = false) = 0;
    /// Overwrite the project file 
    /**
    * @param
    *    projectId [in]: the project ID
    *    parentPathId [in ]: the project file's parent pathId
    *    file [in]: the project file
    *    overwrite [in]: if the overwrite=ture, the file will be replaced if a file with same file path exists
    * @return
    *    ERROR_SUCCESS
    */

    virtual SDWLResult GetRepositories(std::vector<SDR_REPOSITORY>& vecRepository) = 0;
    /// Get the repositories for the RMS
    /**
    * @param
    *    vecRepository [out]: return the repository list
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult GetRepositoryAccessToken(
        const std::wstring& repoId,
        std::wstring& accessToken) = 0;
    /// Get the repository token from the RMS
    /**
    * @param
    *    repoId [in] : the repository ID
    *    accessToken [out] : the access token
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult GetRepositoryAuthorizationUrl(
        const std::wstring& type,
        const std::wstring& name,
        std::wstring& authURL) = 0;
    /// Get the repository authorization url from the RMS
    /**
    * @param
    *    type [in] : the repository type, that can be "DROPBOX","GOOGLE_DRIVE","ONE_DRIVE", etc
    *    name [in] : the repository name
    *    authURL [out] : the authorization url returned from the RMS
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult UpdateRepository(
        const std::wstring& repoId,
        const std::wstring& token,
        const std::wstring& name) = 0;
    /// Update the repository name or token
    /**
    * @param
    *    repoId [in] : the repository ID
    *    token [in] : the repository token
    *    name [out] : the repository name
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult RemoveRepository(const std::wstring& repoId) = 0;
    /// Remove the repository from the RMS
    /**
    * @param
    *    repoId [in] : the repository ID
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult AddRepository(
        const std::wstring& name,
        const std::wstring& type,
        const std::wstring& accountName,
        const std::wstring& accountId,
        const std::wstring& token,
        const std::wstring& perference,
        uint64_t createTime,
        bool isShared,
        std::wstring& repoId) = 0;
    /// Add a new repository in RMS server
    /**
    * @param
    *    name [in] : the repository name
    *    type [in] : the repository type
    *    accountName [in] : the repository account name
    *    accountId [in] : the repository account Id
    *    token [in] : the repository token
    *    perference [in] : the repository perference
    *    createTime [in] : the repository create time
    *    isShared [in] : ture is shared, else not
    *    repoId [in] : the repository repoId, returned from RMS
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult GetRepositoryServiceProvider(std::vector<std::wstring>& serviceProvider) = 0;
    /// Get Repository Service Provider
    /**
    * @param
    *    serviceProvider [out] : the repository provider list (like : GOOGLE_DRIVE, DROPBOX)
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult MyVaultFileIsExist(
        const std::string& pathId,
        bool& bExist) = 0;
    /// Check if myVault file is exist or not
    /**
    * @param
    *    pathId [in] : myVault file pathId
    *    bExist [out]: if the file exist, bExist = true, else bExist = false
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult MyVaultGetNxlFileHeader(
        const std::string& pathId,
        std::string& targetFolder) = 0;
    /// Get myVault file NXL Header
    /**
    * @param
    *    pathId [in] : myVault file pathId
    *    targetFolder [in out]: pass the file SaveTo folder and return the file path
    * @return
    *    ERROR_INVALID_DATA          Invalid download file result
    */

	virtual SDWLResult GetMyVaultFiles(uint32_t pageId, uint32_t pageSize, const std::string& orderBy, const std::string& searchString, std::vector<SDR_MYVAULT_FILE_INFO>& listfiles) = 0;
	/// Get project files
	/**
	* @param
	*    pageId       Page number (start from 1)
	*    pageSize     Number of records to be returned
	*    orderBy      A comma-separated list of sort keys.
	*    searchString  search specific string
	*    listfiles    return files list
	* @return
	*           SDWL_INVALID_DATA          Invalid project list files query return
	*            ERROR_INVALID_DATA          Invalid project list files result
	*/
		
	virtual SDWLResult MyVaultDownloadFile(const std::string& pathid, std::wstring& downloadPath, uint32_t downloadtype) = 0;
	/// MyVault download file
	/**
	* @param
	*    pathid     pathid
	*    downloadPath  in/out
	*					in	targetfolder
	*					out	final saved file full path
	*    downloadtype  0 normal 1 for viewer (nxl header) 2 offline
	* @return
	*            ERROR_INVALID_DATA          Invalid download file result
	*/

	virtual SDWLResult GetSharedWithMeFiles(uint32_t pageId, uint32_t pageSize, const std::string& orderBy, const std::string& searchString, std::vector<SDR_SHAREDWITHME_FILE_INFO>& listfiles) = 0;
	/// Get project files
	/**
	* @param
	*    pageId       Page number (start from 1)
	*    pageSize     Number of records to be returned
	*    orderBy      A comma-separated list of sort keys.
	*    searchString  search specific string
	*    listfiles    return files list
	* @return
	*           SDWL_INVALID_DATA          Invalid project list files query return
	*            ERROR_INVALID_DATA          Invalid project list files result
	*/

	virtual SDWLResult SharedWithMeDownloadFile(const std::wstring& transactionCode, const std::wstring& transactionId, std::wstring& targetFolder, bool forViewer) = 0;
	/// Project download file
	/**
	* @param
	*    transactionCode     transactionCode
	*    transactionId     transactionId
	*    targetFolder  in/out
	*					in	targetfolder
	*					out	final saved file full path
	*    forViewer
	* @return
	*            ERROR_INVALID_DATA          Invalid download file result
	*/

	virtual SDWLResult SharedWithMeDownloadPartialFile(const std::wstring& transactionCode, const std::wstring& transactionId, std::wstring& targetFolder, bool forViewer) = 0;
	/// ShareWithMe download file
	/**
	* @param
	*    transactionCode     transactionCode
	*    transactionId     transactionId
	*    targetFolder  in/out
	*					in	targetfolder
	*					out	final saved file full path - file content only include NXL file header
	*    forViewer
	* @return
	*            ERROR_INVALID_DATA          Invalid download file result
	*/

	virtual SDWLResult SharedWithMeReShareFile(const std::string& transactionId, const std::string& transactionCode, const std::string emaillist) = 0;
	/// ShareWithMe reshare file to others
	/**
	* @param
	*    transactionCode     transactionCode
	*    transactionId     transactionId
	*    emaillist  shared to users, e.g. "abc@hello.com, xyz@hello.com"
	* @return
	*            ERROR_INVALID_DATA          Invalid result
	*/

	virtual SDWLResult WorkspaceDownloadFile(const std::string& pathId, std::wstring& targetFolder, uint32_t downloadtype) = 0;
	/// Download workspace file
	/**
	* @param
	*	pathId         pathid
	*	targetFolder   in/out
	*				    in	targetfolder 	
	*                   out final saved file full path - the file is nxl header content only
	*   downloadtype    0 normal 1 for viewer (nxl header) 2 offline
	* @return
	*	ERROR_INVALID_DATA          Invalid download file result
	*/

	virtual SDWLResult GetWorkspaceFiles(
        uint32_t pageId, 
        uint32_t pageSize, 
        const std::string& path, 
        const std::string& orderBy,
		const std::string& searchString, 
        std::vector<SDR_WORKSPACE_FILE_INFO>& listfiles) = 0;
	/// Get worksapce files
	/**
	* @param
	*    pageId       Page number (start from 1)
	*    pageSize     Number of records to be returned
	*    path         path 
	*    orderBy      A comma-separated list of sort keys.
	*    searchString  search specific string
	*    listfiles    return files list
	* @return
	*           SDWL_INVALID_DATA          Invalid workspace list files query return
	*           ERROR_INVALID_DATA          Invalid workspace list files result
	*/

    virtual SDWLResult WorkspaceFileIsExist(
        const std::string& pathId,
        bool& bExist) = 0;
    /// Check if the workspace file is exist or not
    /**
    * @param
    *    pathId [in]: the workspace file pathId
    *    bExist [out]: if the file exist, bExist = true, else bExist = false
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult WorkspaceGetNxlFileHeader(
        const std::string& pathId,
        std::string& targetFolder) = 0;
    /// Get the workspace file NXL header
    /**
    * @param
    *    pathId [in]: the workspace file pathId
    *    targetFolder [in out]: pass the file SaveTo folder and return the file path
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

    virtual SDWLResult WorkspaceFileOverwrite(
        const std::wstring& parentPathId,
        ISDRmNXLFile* file,
        bool overwrite = false) = 0;
    /// Overwrite the workspace file
    /**
    * @param
    *    parentPathId [in ]: the workspace file's parent pathId
    *    file [in]: the workspace file
    *    overwrite [in]: if the overwrite=ture, the file will be replaced if a file with same file path exists
    * @return
    *    ERROR_INVALID_DATA JSON response is not correct
    */

	virtual SDWLResult ChangeRightsOfWorkspaceFile(const std::wstring &originalNXLfilePath, const std::string &fileName, const std::string &parentPathId,
		const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags) = 0;
	/// Change the workspace file rights.
	/**
	* @param
	*    originalNXLfilePath		NXL file path
	*    fileName			file name listed in workspace
	*    parentPathId		folder path the file stored in workspace
	*    rights				new rights assigned to the nxl file
	*    watermarkinfo		watermark structure
	*    expire				SDR_Expiration structure
	*    tags				tags
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult UploadWorkspaceFile(const std::wstring&destfolder, ISDRmNXLFile * file, bool overwrite) = 0;
	/// upload file to worksapce
	/**
	* @param
	*    destfolder  dest folder
	*    file        the file to  upload
	*    overwrite   If overwrite when the file existed.
	* @return
	*     SDWL_INVALID_DATA            invalid Parameter
	*
	*/

	virtual SDWLResult GetWorkspaceFileMetadata(const std::string& pathid, SDR_FILE_META_DATA& metadata) = 0;
	/// Get workspace file meta data
	/**
	* @param
	*    pathid  file pathid
	*    metadata  output file meta data
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*            SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*/

	virtual SDWLResult GetFingerPrintWithoutToken(const std::wstring& nxlFilePath, SDR_NXL_FILE_FINGER_PRINT& fingerPrint) = 0;
	virtual SDWLResult GetFingerPrint(const std::wstring& nxlFilePath, SDR_NXL_FILE_FINGER_PRINT& fingerPrint, bool doOwnerCheck = false)=0;
	/// Get Nxl File's finger print
	/*
		special designed for c# rmsdk api stub easy to check all about nxl file inforamtions
		wrapper some stable single api into a syntetic fingprint struct
	*/

	virtual SDWLResult GetNxlFileOriginalExtention(const std::wstring &nxlFilePath, std::wstring &oriFileExtention) = 0;

	virtual SDWLResult GetRights(const std::wstring& nxlfilepath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true) = 0;
	/// Get the rights granted to the current user, no matter the file is a central policy file or ad-hoc policy file
	/**
	* @param
	*    nxlfilepath            full path to NXL file
	*    rightsAndWatermarks    return rights assigned to the NXL file, and their associated watermark obligations
	* @return
	*
	*/

	virtual SDWLResult GetFileRightsFromCentralPolicies(const std::wstring& nxlFilePath, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true) = 0;
	/// Get the rights granted to the current user by central policies for the passed file.
	/**
	* @param
	*    nxlFilePath            full path to NXL file
	*    rightsAndWatermarks    return rights assigned to the NXL file, and their associated watermark obligations
	* @return
	*           SDWL_NOT_FOUND          Not policy bundle for file's tenant found
	*
	*/
	virtual SDWLResult GetFileRightsFromCentralPolicies(const std::string &tenantname, const std::string &tags, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true) = 0;
	/// Get the rights granted to the current user by central policies for the passed tags on project.
	/**
	* @param
	*    tenantname            project tenant name
	*    tags					tags json string
	*    rightsAndWatermarks    return rights assigned to the NXL file, and their associated watermark obligations
	* @return
	*           SDWL_NOT_FOUND          Not policy bundle for file's tenant found
	*
	*/
	virtual SDWLResult GetFileRightsFromCentralPolicies(const uint32_t projectid, const std::string &tags, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks, bool doOwnerCheck = true) = 0;
	/// Get the rights granted to the current user by central policies for the passed tags on project.
	/**
	* @param
	*    projectid            project id
	*    tags					tags json string
	*    rightsAndWatermarks    return rights assigned to the NXL file, and their associated watermark obligations
	* @return
	*           SDWL_NOT_FOUND          Not policy bundle for file's tenant found
	*
	*/

	virtual SDWLResult GetResourceRightsFromCentralPolicies(const std::wstring& resourceName, const std::wstring& resourceType, const std::vector<std::pair<std::wstring, std::wstring>> &attrs, std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> &rightsAndObligations) = 0;
	/// Get the rights granted to the current user by central policies for the passed resource.
	/**
	* @param
	*    resourceName           name of resource
	*    resourceType           type of resource (e.g. "fso", "portal", token group resouce type), if the empty, will use default tenant resource type to evaluate rights.
	*    attrs                  resource attributes
	*    rightsAndObligations   return rights assigned to the resource, and their associated obligations
	* @return
	*           SDWL_NOT_FOUND          Not policy bundle for current user's tenant found
	*/

	virtual SDWLResult GetHeartBeatInfo() = 0;
	/// Get heart beat infomation
	/**
	* @param
	*    none
	* @return
	*           SDWL_INVALID_DATA          Invalid heart beat query return
	*           ERROR_INVALID_DATA          Invalid heart beat result
	*/

	//virtual bool GetPolicyBundle(const std::wstring& tenantName, std::string& policyBundle) = 0;
	/// Get policy bundle
	/**
	* @param
	*    tenantName  tenant name
	*    policyBundle  policy bundle
	* @return
	*           success return true, else return false
	*/

	virtual const SDR_WATERMARK_INFO GetWaterMarkInfo() = 0;
	/// Get water mark info
	/**
	* @param
	*    none
	* @return
	*           SDR_WATERMARK_INFO structure
	*/

	virtual bool IsFileProtected(const std::wstring &filepath) = 0;
	/// Check file is protected or not
	/**
	* @param
	*    filepath  full path to file 
	* @return
	*           if it's nxl file return true, otherwise return false
	*/

	virtual SDWLResult GetClassification(const std::string &tokengroupname, std::vector<SDR_CLASSIFICATION_CAT>& cats) = 0;
	/// Get Classification Profile
	/**
	* @param
	*    tokengroupname  project's token group name
	* @return
	*            SDWL_INVALID_DATA          Invalid heart beat query return
	*           ERROR_INVALID_DATA          Invalid heart beat result
	*/

	//virtual SDWLResult GetFilePath(const std::wstring &filename, std::wstring &targetfilepath) = 0;
	/// Get file path from file name
	/**
	* @param
	*    filename  file name
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*            SDWL_NOT_FOUND          File is not in the working directory
	*
	*/

	virtual SDWLResult GetUserPreference(uint32_t &option, uint64_t &start, uint64_t &end, std::wstring &watermark) = 0;
	/// Update User Preference
	/**
	* @param
	*    option  out option number
	*    start   out time start date, only use in option 3
	*    end     out time end date, use in option 2, 3
	*    Watermark   out  "$(User)\n$(Date) $(Time)"
	*
	*		note for option "1" - relativeDay
	*					year + month = start (high 32bits + low 32bits)
	*					week + day = end (high 32bits + low 32bits)

	* uint32_t year, month;
	* uint64_t start = ((u64)year) << 32 | month;

	* @return
	*            200 success, the rest is http error code
	*
	*/

	virtual SDWLResult UpdateUserPreference(const uint32_t option, const uint64_t start=0, const uint64_t end=0, const std::wstring watermark = L"") = 0;
	/// Update User Preference
	/**
	* @param
	*    option  option number
	*    start   time start date, only use in option 3
	*    end     time end date, use in option 2, 3
	*    Watermark     "$(User)\n$(Date) $(Time)"
	*
	*		note for option "1" - relativeDay
	*					year + month = start (high 32bits + low 32bits)
	*					week + day = end (high 32bits + low 32bits)

	* uint32_t year, month;
	* uint64_t start = ((u64)year) << 32 | month;

	* @return
	*            200 success, the rest is http error code
	*
	*/

	virtual SDWLResult GetNXLFileMetadata(const std::wstring &nxlfilepath, const std::string& pathid, SDR_FILE_META_DATA& metadata) = 0;
	/// Get file meta data
	/**
	* @param
	*    nxlfilepath  file name
	*    pathid  file pathid
	*    metadata  output file meta data
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*            SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*
	*/

	virtual SDWLResult GetProjectFileMetadata(const std::wstring &nxlfilepath, const std::string& projectid, const std::string& pathid, SDR_FILE_META_DATA& metadata) = 0;
	/// Get file meta data
	/**
	* @param
	*    nxlfilepath  file name
	*    projectid    projectid
	*    pathid  file pathid
	*    metadata  output file meta data
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*            SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*
	*/

	virtual SDWLResult GetProjectFileMetadata(const std::string& projectid, const std::string& pathid, SDR_FILE_META_DATA& metadata) = 0;
	/// Get file meta data
	/**
	* @param
	*    projectid    projectid
	*    pathid  file pathid
	*    metadata  output file meta data
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*            SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*
	*/

	virtual uint32_t GetHeartBeatFrequency() = 0;
	/// Get client heartbeat frequency setting from RMS
	///		before calling this API, make sure the GetHeartBeatInfo is called
	/**
	* @return
	*            frequency       client frequency setting from RMS (in minutes)
	*
	*/

	virtual SDWLResult GetTenantPreference(bool* adhoc, bool* workspace, int* heartbeat, int* sysprojectid, std::string &sysprojecttenantid, const std::string& tenantid = "") = 0;
	/// Get Tenant Preference
	/**
	* @param
	*    adhoc       adhoc enable or disable
	*    workspace       workspace enable or disable
	*    heartbeat   heartbeat frequency (in minutes)
	*	 sysprojectid	system project id
	*	 sysprojecttenantid	system project tenant id
    *    tenantid    tenant id
	*
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*            SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*
	*/

	virtual bool IsTenantAdhocEnabled() = 0;
	/// Check file is protected or not
	/**
	* @param
	*    
	* @return
	*           if tenant is adhoc enabled return true, otherwise return false
	*/

	virtual SDWLResult GetProjectFileRights(const std::string& projectid, const std::string& pathid, std::vector<SDRmFileRight>& rights) = 0;
	/// Get project file meta data
	/**
	* @param
	*    projectid    projectid
	*    pathid  file pathid
	*    rights  output file rights
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*
	*/

	virtual SDWLResult GetMyVaultFileRights(const std::string& duid, const std::string& pathid, std::vector<SDRmFileRight>& rights) = 0;
	/// Get MyVault file meta data
	/**
	* @param
	*    duid    duid
	*    pathid  file pathid
	*    rights  output file rights
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*
	*/

	virtual const std::string GetSystemProjectTenantId() = 0;
	/// Get system project tenantId
	/**
	*
	* @return
	*			system project tenantId
	*/

	virtual const std::string GetDefaultTenantId() = 0;
	/// Get user default tenantId
	/**
	*
	* @return
	*			default tenantId
	*/
	virtual const std::string GetDefaultTokenGroupName() = 0;
	/// Get token group name of default tenant
	/**
	*
	* @return
	*			token group name of default tenant
	*/

	virtual SDWLResult PDSetupHeader(const unsigned char* header, long header_len, int64_t* contentLength, unsigned int* contentOffset) = 0;
	/// Set up the file header data for partial decryption
	/**
	* @param
	*    header  nxl file header
	*    header_len  header buffer length
	*    contentLength  content length
	*    contentOffset  content offset
	*
	*       note for header and header_len:
	*           For Ad-Hoc Policy files:
	*                   The passed header needs to be big enough to include
	*                   both the fixed header and the FilePolicy section.
	*                   Currently this means it needs to be at least 12288
	*                   bytes.
	*           For Central Policy files:
	*                   The passed header needs to be big enough to include
	*                   the fixed header, the FilePolicy section, and the
	*                   FileTag section.  Currently this means it needs to be
	*                   at least 16384 bytes.
	*
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*
	*/

	virtual SDWLResult PDDecryptPartial(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len, const unsigned char* header, long header_len) = 0;
	/// Partial decrypt file data
	/**
	* @pre
	*    PDSetupHeader must have been called.
	*
	* @param
	*    in      input buffer address
	*    in_len  input buffer length
	*    offset  starting length, must be multiple blocksize
	*    out     out buffer address
	*    out_len output buffer length
	*    header  nxl file header
	*    header_len  header buffer length
	*
	*       note for header and header_len:
	*           For Ad-Hoc Policy files:
	*                   The passed header needs to be big enough to include
	*                   both the fixed header and the FilePolicy section.
	*                   Currently this means it needs to be at least 12288
	*                   bytes.
	*           For Central Policy files:
	*                   The passed header needs to be big enough to include
	*                   the fixed header, the FilePolicy section, and the
	*                   FileTag section.  Currently this means it needs to be
	*                   at least 16384 bytes.
	*
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*
	*/

	virtual SDWLResult PDSetupHeaderEx(const unsigned char* header, long header_len, int64_t* contentLength, unsigned int* contentOffset, void *&context) = 0;
	/// Set up the file header data for partial decryption, thread-safe
	/**
	* @param
	*    header  nxl file header
	*    header_len  header buffer length
	*    contentLength  content length
	*    contentOffset  content offset
	*    context        return context pointer.  If context pointer is NULL, check return result.
	*
	*       note for header and header_len:
	*           For Ad-Hoc Policy files:
	*                   The passed header needs to be big enough to include
	*                   both the fixed header and the FilePolicy section.
	*                   Currently this means it needs to be at least 12288
	*                   bytes.
	*           For Central Policy files:
	*                   The passed header needs to be big enough to include
	*                   the fixed header, the FilePolicy section, and the
	*                   FileTag section.  Currently this means it needs to be
	*                   at least 16384 bytes.
	*
	* @return
	*            SDWL_NOT_ENOUGH_MEMORY  not enough memory
	*            SDWL_INVALID_DATA       Invalid data
	*
	* @post
	*    PDTearDownHeaderEx must be called to tear down the header info.
	*
	*/

	virtual SDWLResult PDDecryptPartialEx(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len, const unsigned char* header, long header_len, void *context) = 0;
	/// Parital decrypt file data, thread-safe
	/**
	* @pre
	*    PDSetupHeaderEx must have been called.
	*
	* @param
	*    in      input buffer address
	*    in_len  input buffer length
	*    offset  starting length, must be multiple blocksize
	*    out     out buffer address
	*    out_len output buffer length
	*    header  nxl file header
	*    header_len  header buffer length
	*    context     context returned by PDSetupHeaderEx
	*
	*       note for header and header_len:
	*           For Ad-Hoc Policy files:
	*                   The passed header needs to be big enough to include
	*                   both the fixed header and the FilePolicy section.
	*                   Currently this means it needs to be at least 12288
	*                   bytes.
	*           For Central Policy files:
	*                   The passed header needs to be big enough to include
	*                   the fixed header, the FilePolicy section, and the
	*                   FileTag section.  Currently this means it needs to be
	*                   at least 16384 bytes.
	*
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*
	*/

	virtual void PDTearDownHeaderEx(void *context) = 0;
	/// Tear down the file header for partial decryption, thread-safe
	/**
	* @pre
	*    PDSetupHeaderEx must have been called.
	*
	* @param
	*    context     context returned by PDSetupHeaderEx
	*
	*/

	virtual SDWLResult ChangeRightsOfFile(const std::wstring &filepath, const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags = "") = 0;
	/// Change the rights of the file
	/**
	* @param
	*    filepath			full path to original NXL file
	*    rights				new rights assigned to the nxl file
	*    watermarkinfo		watermark structure
	*    expire				SDR_Expiration structure
	*    tags				tags
	* @return
	*			SDWL_PATH_NOT_FOUND			can't open the original file
	*           SDWL_INTERNAL_ERROR			fail to create nxl file, check return message for detail info.
	*
	*/

	virtual bool HasAdminRights(const std::wstring &nxlfilepath) = 0;
	/// Check current user whether he has the Admin Rights on the file
	/**
    * @pre
    *    GetListProjects must have been called first
	* @param
	*    nxlfilepath		full path to original NXL file
	* @return
	*			true			has the 'Admin Rights' on the file
	*           false			no Admin Rights
	*
	*/

	virtual SDWLResult UpdateNXLMetaData(const std::wstring &nxlfilepath, bool bRetry = true) = 0;
	/// Sync the file metadata to RMS
	/**
	* @param
	*    nxlfilepath		full path to NXL file
	* @return
	*			SUCCESS
	*           ERROR_INVALID_DATA
	*
	*/

	virtual SDWLResult UpdateNXLMetaData(ISDRmNXLFile* file, bool bRetry = true) = 0;
	/// Sync the file metadata to RMS
	/**
	* @param
	*    ISDRmNXLFile		file pointer of NXL file
	* @return
	*			SUCCESS			
	*           ERROR_INVALID_DATA
	*
	*/

	virtual SDWLResult ResetSourcePath(const std::wstring & nxlfilepath, const std::wstring & sourcepath) = 0;
	/// Set the source path for NXL file in case that new protect file is from a temp file
	/**
	* @param
	*    nxlfilepath		new protected NXL file path (not uploaded)
	*    sourcepath			exact source file path for the new protected file
	* @return
	*			SUCCESS
	*
	*/
	virtual SDWLResult ShareFileFromMyVault(const std::wstring &filepath, const std::vector<std::string> &recipients, const std::string &repositoryId, const std::string &fileName, const std::string &filePathId, const std::string &filePath, const std::string &comment) = 0;
	/// Share the file protected under MyVault to new recipients
	/**
	* @param
	*    filepath		NXL file path
	*    recipients		array of recipient email
	*    repositoryId	MyVault repository id
	*    fileName		fileName shown in MyVault file list
	*    filePath		filePath shown in MyVault file list
	*    filePathId		filePathId shown in MyVault file list
	*    comment		comment added to shared recipients
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult ChangeRightsOfProjectFile(const std::wstring &oldnxlfilepath, unsigned int projectid, const std::string &fileName, const std::string &parentPathId, const std::vector<SDRmFileRight> &rights, const SDR_WATERMARK_INFO &watermarkinfo, const SDR_Expiration &expire, const std::string& tags = "") = 0;
	/// Re-classify the central policy file stored in project
	/**
    * @pre
    *    GetListProjects must have been called first
	* @param
	*    oldnxlfilepath		NXL file path
	*    projectid			project id that this file belongs
	*    fileName			file name listed in project
	*    parentPathId		folder path the file stored in project
	*    rights				new rights assigned to the nxl file
	*    watermarkinfo		watermark structure
	*    expire				SDR_Expiration structure
	*    tags				tags
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult LockFileSync(const std::wstring &nxlfilepath) = 0;
	virtual SDWLResult ResumeFileSync(const std::wstring &nxlfilepath) = 0;

	virtual SDWLResult DecryptNXLFile(ISDRmNXLFile* file, const std::wstring &targetfilepath) = 0;

	virtual SDWLResult ReProtectSystemBucketFile(const std::wstring &originalNXLfilePath) = 0;
	virtual SDWLResult ChangeFileToMember(const std::wstring &filepath, const std::string &memberid = "") = 0;
	virtual SDWLResult RenameFile(const std::wstring &filepath, const std::string &name = "") = 0;

	virtual SDWLResult MyDriveListFiles(const std::wstring& pathId, std::vector<SDR_MYDRIVE_FILE_INFO>& listfiles) = 0;
	/**
	* @param
	*    pathId				folder path in MyDrive, example "/", "/abc/"
	*    listfiles			file and folder list in above folder
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult MyDriveDownloadFile(const std::wstring& downloadPath, std::wstring& targetFolder) = 0;
	/**
	* @param
	*    downloadPath			file path in MyDrive, example "/abc/abc.docx"
	*    targetFolder			output folder in local drive
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult MyDriveUploadFile(
        const std::wstring& pathId, 
        const std::wstring& parentPathId, 
        bool overwrite = false) = 0;
	/**
	* @param
	*    pathId				file path in local drive
	*    parentPathId		the file's parent pathId in MyDrive
	*    overwrite			not used now
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult MyDriveCreateFolder(const std::wstring&name, const std::wstring&parentfolder) = 0;
	/**
	* @param
	*    name				new folder name
	*    parentfolder		parent folder path in MyDrive, example "/"
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult MyDriveDeleteItem(const std::wstring&pathId) = 0;
	/**
	* @param
	*    pathId				file path in MyDrive
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult MyDriveCopyItem(const std::wstring&srcPathId, const std::wstring&destPathId) = 0;
	/**
	* @param
	*    srcPathId			source file path in MyDrive
	*    destPathId			dest file path in MyDrive
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult MyDriveMoveItem(const std::wstring&srcPathId, const std::wstring&destPathId) = 0;
	/**
	* @param
	*    srcPathId			source file path in MyDrive
	*    destPathId			dest file path in MyDrive
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult MyDriveCreateShareURL(const std::wstring&pathId, std::wstring&sharedURL) = 0;
	/**
	* @param
	*    pathId				file path in MyDrive
	*    sharedURL			generated shared URL link of the file
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult ProtectFileFrom(const std::wstring &srcplainfile, const std::wstring& originalnxlfile, std::wstring& output) = 0;
	/**
	* @param
	*    srcplainfile		native file path which need to be protected
	*    originalnxlfile	the original NXL file which the new file will use the same rights to protect
	*							it can be a NXL file in non-RPM folder
	*							it can be the NXL file with/without NXL extension in RPM folder
	*	 output				
	*					in	output folder
	*					out	generated file path
	* @return
	*			SUCCESS
	*
	*/

	virtual SDWLResult GetSharedWorkspaceListFiles(const std::string& repoId, uint32_t pageId, uint32_t pageSize, const std::string& orderBy, const std::string& pathId, const std::string& searchString, std::vector<SDR_SHARED_WORKSPACE_FILE_INFO>& listfiles) = 0;
	/// Get shared workspace files
	/**
	* @param
	*    repoId		  repository id
	*    pageId       Page number (start from 1)
	*    pageSize     Number of records to be returned
	*    orderBy      A comma-separated list of sort keys.
	*    pathid       pathid
	*    searchString  search specific string
	*    listfiles    return files list
	* @return
	*           SDWL_INVALID_DATA          Invalid list files query return
	*            ERROR_INVALID_DATA          Invalid list files result
	*/

	virtual SDWLResult GetSharedWorkspaceFileMetadata(const std::string& repoId, const std::string& pathId, SDR_SHARED_WORKSPACE_FILE_METADATA& metadata) = 0;
	/// Get file meta data
	/**
	* @param
	*    repoId    repository id
	*    pathId  file pathid
	*    metadata  output file meta data
	* @return
	*            SDWL_INVALID_DATA       Invalid data
	*            SDWL_RMS_ERRORCODE_BASE+	RMS server error
	*
	*/

	virtual SDWLResult UploadSharedWorkspaceFile(const std::string& repoId, const std::wstring &destfolder, ISDRmNXLFile* file, int uploadType, bool userConfirmedFileOverwrite = false) = 0;
	/// upload file 
	/**
	* @param
	*    repoId   repository id
	*    destfolder  dest folder
	*    file        file to  upload
	* @return
	*     SDWL_INVALID_DATA            invalid Parameter
	*
	*/

	virtual SDWLResult DownloadSharedWorkspaceFile(const std::string& repoId, const std::string& pathId, std::wstring& targetFolder, uint32_t downloadtype, bool isNXL) = 0;
	/// Download file
	/**
	* @param
	*   repoId	       repository id
	*	pathId         pathid
	*	targetFolder   in/out
	*				    in	targetfolder
	*                   out final saved file full path - the file is nxl header content only
	*   downloadtype    0 normal 1 for viewer (nxl header) 2 offline
	* @return
	*	ERROR_INVALID_DATA          Invalid download file result
	*/

	virtual SDWLResult IsSharedWorkspaceFileExist(const std::string& repoId, const std::string& pathId, bool& bExist) = 0;
	/// Check if file is exist or not
	/**
	* @param
	*    repoId	[in]: repository id
	*    pathId [in]: file pathId
	*    bExist [out]: if the file exist, bExist = true, else bExist = false
	* @return
	*    ERROR_INVALID_DATA JSON response is not correct
	*/

	virtual SDWLResult GetWorkspaceNxlFileHeader(const std::string& repoId, const std::string& pathId, std::string& targetFolder) = 0;
	/// Get file NXL header
	/**
	* @param
	*    repoId [in]: repository id
	*    pathId [in]: file pathId
	*    targetFolder [in out]: pass the file SaveTo folder and return the file path
	* @return
	*    ERROR_INVALID_DATA JSON response is not correct
	*/

	virtual SDWLResult CopyNxlFile(const std::string& srcFileName, const std::string& srcFilePath, RM_NxlFileSpaceType srcSpaceType, const std::string& srcSpaceId,
		const std::string& destFileName, const std::string& destFolderPath, RM_NxlFileSpaceType destSpaceType, const std::string& destSpaceId, 
		bool bOverwrite = false, const std::string& transactionCode = "", const std::string& transactionId = "") = 0;
	/// Copy nxl file from one space to another: Add nxl file\download\save as
	/**
	* @param
	*    srcFileName [in]: source file name
	*    srcFilePath [in]: source file path, for the space outside rms(e.g. LOCAL_DRIVE), this is the full path(e.g. C:\Allen\text.nxl.txt);
	*                      for the space inside rms(e.g. WORKSPACE), this is the path id(e.g. \Allen\text.nxl.txt) 
	*    srcSpaceType [in]: source space type
	*    srcSpaceId   [in]: source space id, this is the repoId or project id(for project). For myvault space, this is "mydrive id", also we can
	*                       skip the paramerter.
	*    destFileName [in]: destination file name
	*    destFolderPath [in]: destination folder path\pathId, for the space outside rms(e.g. LOCAL_DRIVE), this is the target selected folder(e.g. C:\Allen);
	*                       for the space inside rms(e.g. WORKSPACE), this is the path id(e.g. \Allen) 
	*    destSpaceType [in]: destination space type
	*    destSpaceId   [in]: destination space id, this is the repoId or project id(for project). For myvault space, this is "mydrive id", also we can
	*                        skip the paramter.
	*    bOverwrite    [in]: if is overwrite the file if exists the same file.
	*    transactionCode [in]: transaction code 
	*    transactionId [in]:  transaction id
	* @return
	*    ERROR_INVALID_DATA JSON response is not correct
	*/

	virtual SDWLResult ReProtectFile(const std::wstring &filepath, const std::wstring& originalNXLfilePath, const std::wstring& newtags=L"") = 0;
	/*
	  @param
	  filepath[in]: source normal file path
	  originalNXLfilePath[in]: nxl file path
	*/

};