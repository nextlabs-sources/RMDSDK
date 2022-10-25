/*
	1/7/2019:
	All string in char-set must use conform to wchar_t.
	Don't Allow Uset char anymore
*/
#pragma once


// SDWL_User_GetUserType depends
#define USER_TYPE_SKYDRM	0
#define USER_TYPE_SMAL		1
#define USER_TYPE_GOOGLE	2
#define USER_TYPE_FACEBOOK	3
#define USER_TYPE_YAHOO		4


// Web login logic required, set all those values into web cookie,
// developer does not need to know its meanings
#pragma pack(push)
#pragma pack(8)
typedef struct _NXL_LOGIN_COOKIES {
	wchar_t* key;
	wchar_t* values;

}NXL_LOGIN_COOKIES;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _RPMDir {
	wchar_t* pArr;
	UINT len;
}RPMDir;
#pragma pack(pop)


#pragma pack(push)
#pragma pack(8)
typedef struct _WaterMark {
	wchar_t* text;
	wchar_t* fontName;
	wchar_t* fontColor;
	int fontSize;
	int transparency;
	int rotation;
	int repeat;
}WaterMark;
#pragma pack(pop)


// Expiration for nxl file format
enum ExpiryType {
	never = 0,
	relative,
	absolute,
	range
};

enum RPMFolderOperation {
	RPM_FOLDER_NORMAL = 1,
	RPM_FOLDER_AUTOPROTECT = 2,
	RPM_FOLDER_OVERWRITE = 4,
	RPM_FOLDER_USER = 8,
	RPM_FOLDER_EXT = 0x10,
	RPM_FOLDER_API = 0x20
};

#pragma pack(push)
#pragma pack(8)
typedef struct _Expiration {
	ExpiryType type;
	DWORD64  start;
	DWORD64  end;
}Expiration;
#pragma pack(pop)

// List and fetech files in MyVault
#pragma pack(push)
#pragma pack(8)
typedef struct _MyVaultFileInfo {
	wchar_t* pathId;
	wchar_t* displayPath;
	wchar_t* repoId;
	wchar_t* duid;
	wchar_t* nxlName;
	//
	uint64_t lastModifiedTime;
	uint64_t creationTime;
	uint64_t sharedTime;
	wchar_t* sharedWithList;
	uint64_t size;
	//
	uint64_t is_deleted;
	uint64_t is_revoked;
	uint64_t is_shared;
	// other misc
	wchar_t* source_repo_type;
	wchar_t* source_file_displayPath;
	wchar_t* source_file_pathId;
	wchar_t* source_repo_name;
	wchar_t* source_repo_id;

}MyVaultFileInfo;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _MyDriveFileInfo {
	wchar_t* pathId;
	wchar_t* pathDisplay;
	wchar_t* name;
	uint64_t lastModified;
	uint64_t size;
	uint32_t isFolder;
}MyDriveFileInfo;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _WorkSpaceFileInfo {
	wchar_t* fileId;
	wchar_t* duid;
	wchar_t* pathDisplay;
	wchar_t* pathId;
	wchar_t* nxlName;
	wchar_t* fileType;
	//
	uint64_t lastModified;
	uint64_t created;
	uint64_t size;
	//
	uint32_t isFolder;
	//
	uint32_t ownerId;
	wchar_t* ownerDisplayName;
	wchar_t* ownerEmail;
	//
	uint32_t modifiedBy;
	wchar_t* modifedByName;
	wchar_t* modifedByEmail;
}WorkSpaceFileInfo;
#pragma pack(pop)

enum Type_DownlaodFile {
	Normal_DownLoad = 0,
	Download_For_Viewer = 1,
	Download_For_Offline = 2
};

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjtectInfo {
	DWORD id;
	wchar_t* name;
	wchar_t* displayname;
	wchar_t* description;
	int owner;
	DWORD64 totalfiles;
	wchar_t* tenantid; // should be tokengroupname
	DWORD64 isEnableAdhoc;
}ProjtectInfo;
#pragma pack(pop)

enum ProjtectFilter {
	All = 0,
	OwnedByMe = 1,
	OwnedByOther = 2
};

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjectFileInfo {
	wchar_t* id;
	wchar_t* duid;
	wchar_t* displayPath;
	wchar_t* pathId;
	wchar_t* nxlName;
	//// new added
	uint64_t lastModifiedTime;
	uint64_t creationTime;
	uint64_t filesize;
	int isFolder;
	uint32_t ownerId;
	wchar_t*	ownerDisplayName;
	wchar_t*   ownerEmail;

}ProjectFileInfo;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjectClassifacationLables {
	wchar_t* name;
	int32_t multiseclect;
	int32_t mandatory;
	wchar_t* labels; // separate by ;  "a;b;c;d;e;f;g;"
	wchar_t* isdefaults;  // 1-true;0-false  separate by ;  "1;0;1;1;1;1"
}ProjectClassifacationLables;
#pragma pack(pop)

/* Begin: for sharing transaction type */
enum FilterType {
	AllShared = 0,
	AllFiles,
	Revoked
};

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjectUserInfo {
	uint32_t userId;
	wchar_t* displayName;
	wchar_t* email;
}ProjectUserInfo;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjectSharedFile { // Shared file from the project.
	wchar_t* id;
	wchar_t* duid;
	wchar_t* pathDisplay;
	wchar_t* pathId;
	wchar_t* name;
	wchar_t* fileType;
	uint64_t lastModified;
	uint64_t creationTime;
	uint64_t size;
	// 
	uint64_t isFolder;
	uint64_t isShared;
	uint64_t isRevoked;
	//
	ProjectUserInfo owner;
	ProjectUserInfo lastModifiedUser;
	wchar_t* sharedWithProject; // Separate by ';' like "1;3;5;"
}ProjectSharedFile;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjectSharedWithMeFile {
	wchar_t* duid;
	wchar_t* name;
	wchar_t* fileType;
	uint64_t size;
	uint64_t sharedDate;
	wchar_t* sharedBy;
	wchar_t* transactionId;
	wchar_t* transactionCode;
	wchar_t* comment;
	uint64_t isOwner;
	uint32_t protectType;
	wchar_t* sharedByProject;
	DWORD64 rights; // Should covert from sdk  "Vector<SDRmFileRight>" by "bit or" opration
}ProjectSharedWithMeFile;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjectReshareFileResult {
	uint32_t protectType;
	wchar_t* newTransactionId;
	wchar_t* newSharedList; // Separate by ';' like "1;3;5"
	wchar_t* alreadySharedList; // Separate by ';' like "1;3;5"
}ProjectReshareFileResult;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _ProjectShareFileResult {
	wchar_t* name;
	wchar_t* duid;
	wchar_t* filePathId;
	wchar_t* transactionId;
	wchar_t* newSharedList; // Separate by ';'  like "1;3;5"
	wchar_t* alreadySharedList; // Separate by ';' like "1;3;5"
}ProjectShareFileResult;
#pragma pack(pop)

/* End: for sharing transaction type */

#pragma pack(push)
#pragma pack(8)
typedef struct _SharedWithMeFileInfo {
	wchar_t*  duid;
	wchar_t*  nxlName;
	wchar_t*  fileType; // "png", "txt", "doc"
	uint64_t	size;
	uint64_t    sharedDateMillis;
	wchar_t*  sharedbyWhoEmail;	// who shares to you, i.e. "osmond.@nextalbs.com"
	wchar_t*  transactionId;
	wchar_t*  transactionCode;
	wchar_t*  sharedlinkUrl;	// a url you can view it online
	wchar_t*  rights;		// json array str like " ["VIEW","SHARE","DOWNLOAD"] "
	wchar_t*  comments;	// i.e. "Keng shared this file to you"
	uint64_t         isOwner;
}SharedWithMeFileInfo;
#pragma pack(pop)

// Shared Workspace
#pragma pack(push)
#pragma pack(8)
typedef struct _SharedWorkspaceFileInfo {
	wchar_t* fileId;
	wchar_t* path;
	wchar_t* pathId;
	wchar_t* fileName;
	wchar_t* fileType;
	//
	uint64_t isProtectedFile;
	uint64_t lastModified;
	uint64_t creationTime;
	uint32_t size;
	uint64_t isFolder;
}SharedWorkspaceFileInfo;
#pragma pack(pop)


#pragma pack(push)
#pragma pack(8)
typedef struct _NXL_FILE_ACTIVITY_INFO {
	wchar_t* duid;
	wchar_t* email;
	wchar_t* operation;
	wchar_t* deviceType;
	wchar_t* deviceId;
	wchar_t* accessResult;
	DWORD64 accessTime;
} NXL_FILE_ACTIVITY_INFO;
#pragma pack(pop)


#pragma pack(push)
#pragma pack(8)
typedef struct _NXL_FILE_FINGER_PRINT {
	wchar_t* name;
	wchar_t* localPath;

	DWORD64 fileSize;
	DWORD64 fileCreated;
	DWORD64 fileModified;
	DWORD64 isOwner;
	DWORD64 isFromMyVault;
	DWORD64 isFromProject;
	DWORD64 isFromSystemBucket;
	DWORD64 projectId;         // only enabled when isFromProject=true;
	DWORD64 isByAdHocPolicy;
	DWORD64 IsByCentrolPolicy;

	wchar_t* tags;          // only enabled when IsByCentrolPolicy=true;
	Expiration expiration;  // only enabled when isByAdHocPolicy=true;
	wchar_t* adHocWatermar; // only enabled when isByAdHocPolicy=true;
	DWORD64 rights; // both adhoc and centrol policy

	bool hasAdminRights;
	wchar_t* duid;
} NXL_FILE_FINGER_PRINT;
#pragma pack(pop)


#pragma pack(push)
#pragma pack(8)
typedef struct _MYVAULT_FILE_META_DATA {
	wchar_t* name;
	wchar_t* fileLink;

	DWORD64 lastModify;
	DWORD64 protectOn;
	DWORD64 sharedOn;
	DWORD64 isShared;
	DWORD64 isDeleted;
	DWORD64 isRevoked;
	DWORD64 protectionType;
	DWORD64 isOwner;
	DWORD64 isNxl;

	wchar_t* recipents;
	wchar_t* pathDisplay;
	
	wchar_t* pathId;
	wchar_t* tags;
	
	Expiration expiration;
} MYVAULT_FILE_META_DATA;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _WORKSPACE_FILE_META_DATA {
	wchar_t* name;
	wchar_t* fileLink;

	DWORD64 lastModify;
	DWORD64 protectOn;
	DWORD64 sharedOn;
	DWORD64 isShared;
	DWORD64 isDeleted;
	DWORD64 isRevoked;
	DWORD64 protectionType;
	DWORD64 isOwner;
	DWORD64 isNxl;

	wchar_t* recipents;
	wchar_t* pathDisplay;

	wchar_t* pathId;
	wchar_t* tags;

	Expiration expiration;
} WORKSPACE_FILE_META_DATA;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(8)
typedef struct _CENTRAL_RIGHTS {
	DWORD64 rights;
	WaterMark* watermarks;
	DWORD64 watermarkLenth;
} CENTRAL_RIGHTS;
#pragma pack(pop)

// Repository
#pragma pack(push)
#pragma pack(8)
typedef struct _RepositoryInfo 
{
	uint32_t isShard;
	uint32_t isDefault;
	//
	uint64_t createTime;
	uint64_t updateTime;
	//
	wchar_t* repoId;
	wchar_t* name;
	wchar_t* type;
	wchar_t* providerClass;
	wchar_t* accountName;
	wchar_t* accountId;
	wchar_t* token;
	wchar_t* preference;
}RepositoryInfo;
#pragma pack(pop)

enum NxlFileSpaceType {
	local_drive = 0,
	my_vault,
	shared_with_me,
	enterprise_workspace,
	project,
	sharepoint_online, // Shared workspace
	//Personal repository
	box,
	dropbox,
	google_drive,
	one_drive
};

