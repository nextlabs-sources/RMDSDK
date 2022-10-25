#pragma once

#include "nxrmflt.h"

NTSTATUS nxrmfltDecryptFile(
	__in	PFLT_CALLBACK_DATA	Data,
	__in	PFLT_INSTANCE		SrcInstance,
	__in	PFLT_INSTANCE		DstInstance,
	__in	PUNICODE_STRING		SrcFileName,
	__in	PUNICODE_STRING		DstFileName
	);

NTSTATUS nxrmfltDeleteFileByNameSync(
	__in	PFLT_INSTANCE		Instance,
	__in	PUNICODE_STRING		FileName
	);

NTSTATUS nxrmfltDeleteFileByName(
	__in	PFLT_INSTANCE		Instance, 
	__in	PUNICODE_STRING		FileName
	);

NTSTATUS nxrmfltEncryptFile(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_INSTANCE				SrcInstance,
	__in	PFLT_INSTANCE				DstInstance,
	__in	PUNICODE_STRING				SrcFileName,
	__in	PUNICODE_STRING				DstFileName
	);

ULONG nxrmfltExceptionFilter(
	__in	NXRMFLT_STREAM_CONTEXT	*Ctx, 
	__in	PEXCEPTION_POINTERS		ExceptionPointer
	);

VOID nxrmfltECPCleanupCallback(
	_Inout_  PVOID EcpContext,
	_In_     LPCGUID EcpType
	);

NTSTATUS
	nxrmfltAddReparseECP(
	_Inout_		PFLT_CALLBACK_DATA			Data,
	__in		PFLT_FILE_NAME_INFORMATION	NameInfo,
	__in		PFLT_INSTANCE				Instance,
	_In_opt_	PUNICODE_STRING				SourceFileName
	);

PVOID nxrmfltGetReparseECP(
	__in PFLT_CALLBACK_DATA Data
	);

NTSTATUS nxrmfltDeleteAddedReparseECP(
	__in PFLT_CALLBACK_DATA Data
	);

NTSTATUS nxrmfltAddBlockingECP(
	_Inout_		PFLT_CALLBACK_DATA			Data,
	__in		PFLT_FILE_NAME_INFORMATION	NameInfo
	);

NXL_PROCESS_NODE* nxrmfltFindProcessNodeByProcessID(
	__in HANDLE		ProcessID
	);

BOOLEAN nxrmfltDoesFileExistEx(
	__in PFLT_INSTANCE					Instance, 
	__in PFLT_FILE_NAME_INFORMATION		NameInfo,
	__in BOOLEAN						IgnoreCase
	);

BOOLEAN nxrmfltDoesFileExist(
	__in PFLT_INSTANCE					Instance, 
	__in PUNICODE_STRING				FileName,
	__in BOOLEAN						IgnoreCase
	);

BOOLEAN nxrmfltIsFileOpened(
	__in PFLT_INSTANCE					Instance,
	__in PUNICODE_STRING				FileName,
	__in BOOLEAN						IgnoreCase,
	__out BOOLEAN						*pFileExisted
);

NTSTATUS nxrmfltGetFileSize(
	__in PFLT_INSTANCE					Instance,
	__in PUNICODE_STRING				FileName,
	__out LONGLONG						*pFileSize
);

NTSTATUS nxrmfltIsAllFileContentPresent(
	__in PFLT_INSTANCE					Instance,
	__in PFILE_OBJECT					FileObject,
	__out BOOLEAN						*pPresent
);

NXL_CACHE_NODE *nxrmfltFindFirstCachNodeByParentDirectoryHash(
	__in ULONG ParentDirectoryHash
	);

NTSTATUS nxrmfltRenameOnDiskNXLFile(
	__in PFLT_INSTANCE		Instance, 
	__in PUNICODE_STRING	CurrentName,
	__in PUNICODE_STRING	NewName
	);

NTSTATUS nxrmfltRenameOnDiskNXLFileEx(
	__in PFLT_INSTANCE					Instance, 
	__in PUNICODE_STRING				CurrentName,
	__in PUNICODE_STRING				NewName,
	__in BOOLEAN						ReplaceIfExists,
	__in HANDLE							RootDirectory,
	__out PFLT_FILE_NAME_INFORMATION	*NewNameInfo
	);

NTSTATUS nxrmfltRenameFile(
	__in PFLT_INSTANCE		Instance, 
	__in PUNICODE_STRING	CurrentName,
	__in PUNICODE_STRING	NewName
	);

NTSTATUS nxrmfltBuildNamesInStreamContext(
	__in NXRMFLT_STREAM_CONTEXT		*Ctx, 
	__in UNICODE_STRING				*FileName
	);

NTSTATUS nxrmfltBuildRenameNodeFromCcb(
	__in NXRMFLT_STREAMHANDLE_CONTEXT	*Ccb,
	__inout NXL_RENAME_NODE				*RenameNode
	);	

VOID nxrmfltFreeRenameNode(
	__in NXL_RENAME_NODE	*RenameNode
	);

NXL_RENAME_NODE *nxrmfltFindRenameNodeFromCcb(
	__in NXRMFLT_STREAMHANDLE_CONTEXT	*Ccb
	);

NTSTATUS nxrmfltCopyOnDiskNxlFile(
	__in PFLT_INSTANCE		SrcInstance,
	__in UNICODE_STRING		*SrcFileName,
	__in PFLT_INSTANCE		DstInstance,
	__in UNICODE_STRING		*DstFileName);

HANDLE nxrmfltReferenceReparseFile(
	__in PFLT_INSTANCE		Instance,
	__in UNICODE_STRING		*FileName);

VOID nxrmfltFreeProcessNode(
	__in NXL_PROCESS_NODE *ProcessNode
	);

NTSTATUS nxrmfltCheckRights(
	__in HANDLE				ProcessId,
	__in HANDLE				ThreadId,
	__in NXL_CACHE_NODE		*pCacheNode,
	__in ULONG				IgnoreCache,
	__inout ULONGLONG		*RightsMask,
	__inout_opt ULONGLONG	*CustomRights,
	__inout_opt	ULONGLONG	*EvaluationId
	);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
NTSTATUS nxrmfltCheckTrust(
	__in HANDLE				ProcessId,
	__out BOOLEAN			*Trusted
	);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

VOID NTAPI nxrmfltDeleteFileByNameWorkProc(
	__in PFLT_GENERIC_WORKITEM	FltWorkItem, 
	__in PVOID					FltObject, 
	__in_opt PVOID				Context);

NTSTATUS nxrmfltGetDeviceInfo(
	IN		PDEVICE_OBJECT				TargetDeviceObject,
	IN OUT  WCHAR						*DeviceName,
	IN		ULONG						DeviceNameLength,
	IN OUT	WCHAR						*SerialNumber,
	IN		ULONG						SerialNumberLength,
	IN OUT	STORAGE_BUS_TYPE			*BusType
	);

BOOLEAN nxrmfltIsProcessDirty(IN HANDLE ProcessId);

NTSTATUS nxrmfltGuessSourceFileFromProcessCache(IN HANDLE ProcessId, IN OUT PUNICODE_STRING SourceFileName);

NTSTATUS nxrmfltSendBlockNotificationMsg(IN PFLT_CALLBACK_DATA Data, IN PUNICODE_STRING FileName, IN NXRMFLT_BLOCK_REASON Reason);

NTSTATUS nxrmfltSendFileErrorMsg(IN PFLT_INSTANCE Instance, IN ULONG SessionId, IN PUNICODE_STRING FileName, IN NTSTATUS ErrorCode);

NTSTATUS nxrmfltCopyTags(
	__in HANDLE				ProcessId,
	__in ULONG				SessionId,
	__in PFLT_INSTANCE		SrcInstance,
	__in UNICODE_STRING		*SrcFileName,
	__in PFLT_INSTANCE		DstInstance,
	__in UNICODE_STRING		*DstFileName);

NTSTATUS nxrmfltPurgeRightsCache(IN PFLT_INSTANCE Instance, IN ULONG FileNameHash);

NTSTATUS nxrmfltForceAccessCheck(
	__in PFLT_INSTANCE		Instance,
	__in PUNICODE_STRING	FileName,  
	__in ACCESS_MASK		DesiredAccess, 
	__in ULONG				FileAttributes,
	__in ULONG				ShareAccess,
	__in ULONG				CreateOptions);

NTSTATUS nxrmfltCheckHideNXLExtsionByProcessId(
	__in HANDLE				ProcessId,
	__inout ULONG			*HideExt
);

NTSTATUS nxrmfltScanNotifyChangeDirectorySafe(
	__in PUNICODE_STRING			DirName, 
	__in PFILE_NOTIFY_INFORMATION	NotifyInfo, 
	__in ULONG						Length,
	__inout ULONG					*ContentDirty
	);

NTSTATUS nxrmfltDuplicateNXLFileAndItsRecords(
	__in HANDLE						RequestorProcessId,
	__in ULONG						RequestorSessionId,
	__in PUNICODE_STRING			SrcFileName,
	__in PUNICODE_STRING			DstFileName,
	__in PFLT_INSTANCE				DstInstance,
	__in PUNICODE_STRING			DstDirName
	);

NTSTATUS nxrmfltBuildAdobeRenameNode(
	__in PUNICODE_STRING			SrcFileName,
	__in PUNICODE_STRING			DstFileName);

void nxrmfltFreeAdobeRenameNode(__in PADOBE_RENAME_NODE	pNode);

NTSTATUS nxrmfltBlockPreCreate(
	__inout PFLT_CALLBACK_DATA			Data,
	__in	PUNICODE_STRING				ReparseFileName,
	__in	PFLT_FILE_NAME_INFORMATION	NameInfo
	);

PFLT_INSTANCE nxrmfltFindInstanceByFileName(__in UNICODE_STRING *FileName);

LONG nxrmfltCompareFinalComponent(
	__in UNICODE_STRING *FileName1, 
	__in UNICODE_STRING *FileName2, 
	__in BOOLEAN		CaseInSensitive);

void FindFinalComponent(
	__in UNICODE_STRING *FileName, 
	__out UNICODE_STRING *FinalComponent);

LONG nxrmfltCompareParentComponent(
	__in UNICODE_STRING *FileName1,
	__in UNICODE_STRING *FileName2,
	__in BOOLEAN		CaseInSensitive);

void WipeFinalComponent(
	__in UNICODE_STRING *FileName,
	__in UNICODE_STRING *ResultComponent);

NTSTATUS nxrmfltSendProcessNotification(
	__in HANDLE		ProcessId,
	__in ULONG		SessionId,
	__in BOOLEAN	Create,
	__in ULONGLONG	Flags,
	__in_opt PCUNICODE_STRING ImageFileName);

VOID NTAPI nxrmfltSendProcessNotificationWorkProc(
	__in PFLT_GENERIC_WORKITEM	FltWorkItem,
	__in PVOID					FltObject,
	__in_opt PVOID				Context);

VOID nxrmfltFreeSessionCacheNode(
	__in NXL_SESSION_CACHE_NODE *SessionCacheNode
);

NTSTATUS nxrmfltQueryToken(
	__in HANDLE				ProcessId,
	__in ULONG				SessionId,
	__in PCUNICODE_STRING	FileName,
	__out NXL_TOKEN			*Token
);

NTSTATUS nxrmfltAcquireToken(
	__in ULONG				SessionId,
	__in PCSTRING			OwnerId,	
	__out PNXL_CREATE_INFO	CreateInfo
);

NTSTATUS nxrmfltConvertNTPath2Win32Path(
	__in PFLT_INSTANCE	Instance,
	__in PCUNICODE_STRING NTFullPath,
	__inout UNICODE_STRING *Win32FullPath
);

NTSTATUS nxrmfltCreateEmptyNXLFileFromExistingNXLFile(
	__in PFLT_INSTANCE		Instance,
	__in HANDLE				ProcessId,
	__in ULONG				SessionId,
	__in PUNICODE_STRING	FileName,
	__in PNXL_CACHE_NODE	pCacheNode
);

NTSTATUS nxrmfltFetchOwnerIdFromFile(
	__in PFLT_INSTANCE		Instance,
	__in PCUNICODE_STRING	FileName,
	__inout PSTRING			OwnerId
);

NTSTATUS nxrmfltFetchUdidFromFile(
	__in PFLT_INSTANCE		Instance,
	__in PCUNICODE_STRING	FileName,
	__inout NXL_UDID*		Udid
);

NTSTATUS nxrmfltCreateEmptyNXLFile(
	__in PFLT_INSTANCE		Instance,
	__in ULONG				SessionId,
	__in PCUNICODE_STRING	FileName,
	__in PCUNICODE_STRING	RealExtension
);

NTSTATUS nxrmfltFetchIVSeedFromFile(
	__in PFLT_INSTANCE		Instance,
	__in PCUNICODE_STRING	FileName,
	_Out_writes_bytes_(16)	UCHAR *IvSeed
);

VOID nxrmfltFreeExpiredTokenCacheNode(
	__in rb_root		*TokenCache,
	__in LARGE_INTEGER	*CurrentTime
);

NTSTATUS nxrmfltSendEditActivityLog(
	__in ULONG				SessionId,
	__in HANDLE				ProcessId,
	__in PFLT_INSTANCE		Instance,
	__in PUNICODE_STRING	FileName
);

NTSTATUS nxrmfltInsertSafeDir(
	__in_opt PFLT_INSTANCE	Instance,
	__in PCUNICODE_STRING	DirPath,
	__in BOOLEAN			Overwrite,
	__in BOOLEAN			AutoAppendNxlExt
	);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
NTSTATUS nxrmfltInsertSanctuaryDir(
    __in_opt PFLT_INSTANCE	Instance,
    __in PCUNICODE_STRING	DirPath
    );
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

NTSTATUS nxrmfltRemoveSafeDir(
    __in_opt PFLT_INSTANCE	Instance,
    __in PCUNICODE_STRING	DirPath
    );

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
NTSTATUS nxrmfltRemoveSanctuaryDir(
    __in_opt PFLT_INSTANCE	Instance,
    __in PCUNICODE_STRING	DirPath
    );
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

VOID nxrmfltRemoveAllSafeDir(
    );

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
VOID nxrmfltRemoveAllSanctuaryDir(
    );
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

BOOLEAN nxrmfltInsideSafeDir(
	__in PFLT_INSTANCE		Instance,
	__in PCUNICODE_STRING	FileName,
	__in BOOLEAN			IgnoreSelf,
	__out_opt BOOLEAN*		Overwrite,
	__out_opt BOOLEAN*		AutoAppendNxlExt
	);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
BOOLEAN nxrmfltInsideSanctuaryDir(
    __in PFLT_INSTANCE	Instance,
    __in PCUNICODE_STRING	FileName,
    __in BOOLEAN IgnoreSelf
    );
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

BOOLEAN nxrmfltInsideSafeDirNtPath(
    __in PFLT_INSTANCE	Instance,
    __in PCUNICODE_STRING	FileName,
	__in BOOLEAN IgnoreSelf
    );

BOOLEAN nxrmfltIsSafeDir(
	__in PFLT_INSTANCE	Instance,
	__in PCUNICODE_STRING	FilePath
	);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
BOOLEAN nxrmfltIsSanctuaryDir(
	__in PFLT_INSTANCE	Instance,
	__in PCUNICODE_STRING	FilePath
	);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

ULONG nxrmfltGetSafeDirRelation(
	__in PFLT_INSTANCE	Instance,
	__in PCUNICODE_STRING	FilePath
);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
ULONG nxrmfltGetSanctuaryDirRelation(
	__in PFLT_INSTANCE	Instance,
	__in PCUNICODE_STRING	FilePath
);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

VOID nxrmfltGetDotNXLCount(
	__in PCUNICODE_STRING	FilePath,
	__out int*				DotNXLCount,
	__out BOOLEAN*			bContainColon
);

BOOLEAN nxrmfltEndWithDotNXL(
	__in PCUNICODE_STRING	FilePath
);

BOOLEAN is_nxrmserv(void);

BOOLEAN is_process_a_service(__in PEPROCESS  Process);

HANDLE getProcessIDFromData(__in PFLT_CALLBACK_DATA Data);

BOOLEAN is_adobe_like_process(void);

BOOLEAN is_IE_like_process(void);
BOOLEAN is_msoffice_process(void);

BOOLEAN is_pwexplorer_like_process(void);

BOOLEAN nxrmfltIsTmpExt(
	__in PUNICODE_STRING Extension
);

BOOLEAN nxrmfltIsZipExt(
	__in PUNICODE_STRING Extension
);

BOOLEAN nxrmfltIsUserLoggedOn(
	__in ULONG	SessionId
);

NTSTATUS nxrmfltSyncFileAttributes(
    __in PFLT_INSTANCE                  Instance,
    __in PUNICODE_STRING                FileName,
    __in PFILE_BASIC_INFORMATION        FileBasicInfo,
    __in BOOLEAN                        IgnoreCase);

BOOLEAN nxSetLogFileName(
	__in PCWCHAR LogFileName
);

BOOLEAN nxWriteToLogFile(
	__in PWCHAR Msg
);

