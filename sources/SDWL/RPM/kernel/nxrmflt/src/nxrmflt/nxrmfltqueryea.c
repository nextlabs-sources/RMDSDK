#include "nxrmfltdef.h"
#include "nxrmflt.h"
#include "nxrmfltnxlcachemgr.h"
#include "nxrmfltutils.h"
#include "nxrmfltqueryea.h"

extern DECLSPEC_CACHEALIGN ULONG				gTraceFlags;
extern DECLSPEC_CACHEALIGN NXRMFLT_GLOBAL_DATA	Global;

NTSTATUS HandleIsContentEncryptedEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength);

NTSTATUS HandleCheckRightsEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength);

NTSTATUS HandleTagsEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength);

NTSTATUS HandleCheckRightsNoneCacheEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength);

extern BOOLEAN IsNXLFile(PUNICODE_STRING Extension);

FLT_PREOP_CALLBACK_STATUS
	nxrmfltPreQueryEA(
	_Inout_ PFLT_CALLBACK_DATA				Data,
	_In_ PCFLT_RELATED_OBJECTS				FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID	*CompletionContext
	)
{
	FLT_PREOP_CALLBACK_STATUS CallbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

	PFLT_INSTANCE	FltInstance = NULL;

	NTSTATUS Status = STATUS_SUCCESS;

	FILE_GET_EA_INFORMATION	*pEaInfo = NULL;
	ULONG EaInfoLength = 0;

	FILE_FULL_EA_INFORMATION *pFullEaInfo = NULL;
	ULONG FullEaInfoLength = 0;

	STRING	FirstEaName = {0};
	STRING	IsContentEncyptedEa = {0};
	STRING	CheckRightsEa = {0};
	STRING	TagsEa = {0};
	STRING	CheckRightsNoneCacheEa = {0};

	NXRMFLT_INSTANCE_CONTEXT	*InstCtx = NULL;

	FltInstance = FltObjects->Instance;

	do 
	{
		if (Data->RequestorMode == KernelMode)
		{
			break;
		}

		if (!Global.ClientPort)
		{
			break;
		}

		Status = FltGetInstanceContext(FltInstance, &InstCtx);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		if (InstCtx->DisableFiltering)
		{
			break;
		}
		
		if (!FlagOn(Data->Iopb->OperationFlags, SL_RESTART_SCAN) ||
			!FlagOn(Data->Iopb->OperationFlags, SL_RETURN_SINGLE_ENTRY))
		{
			break;
		}

		pEaInfo				= Data->Iopb->Parameters.QueryEa.EaList;
		EaInfoLength		= Data->Iopb->Parameters.QueryEa.EaListLength;
		pFullEaInfo			= Data->Iopb->Parameters.QueryEa.EaBuffer;
		FullEaInfoLength	= Data->Iopb->Parameters.QueryEa.Length;

		if (EaInfoLength == 0 ||
			pEaInfo == NULL ||
			pFullEaInfo == NULL ||
			((FullEaInfoLength != (sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR))) &&
			(FullEaInfoLength != (sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_CHECK_RIGHTS) + sizeof(ULONGLONG) + sizeof(ULONGLONG) + sizeof(ULONGLONG))) &&
			(FullEaInfoLength != (sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_CHECK_RIGHTS_NONECACHE) + sizeof(ULONGLONG) + sizeof(ULONGLONG) + sizeof(ULONGLONG))) &&
			(FullEaInfoLength != (sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_TAG) + sizeof(NXL_SECTION_NAME_FILETAG) + sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_SYNC_HEADER) + 4096))))
		{
			break;
		}

		if (pEaInfo->EaNameLength == 0)
		{
			break;
		}

		RtlInitString(&IsContentEncyptedEa, NXRM_EA_IS_CONTENT_ENCRYPTED);
		RtlInitString(&CheckRightsEa, NXRM_EA_CHECK_RIGHTS);
		RtlInitString(&TagsEa, NXRM_EA_TAG);
		RtlInitString(&CheckRightsNoneCacheEa, NXRM_EA_CHECK_RIGHTS_NONECACHE);

		FirstEaName.Buffer = pEaInfo->EaName;
		FirstEaName.Length = pEaInfo->EaNameLength;
		FirstEaName.MaximumLength = pEaInfo->EaNameLength;

		if (0 == RtlCompareString(&IsContentEncyptedEa, &FirstEaName, FALSE))
		{
			Status = HandleIsContentEncryptedEa(Data, pFullEaInfo, FullEaInfoLength);

			if (NT_SUCCESS(Status))
			{
				CallbackStatus = FLT_PREOP_COMPLETE;
			}

			break;
		}
		else if (0 == RtlCompareString(&CheckRightsEa, &FirstEaName, FALSE))
		{
			Status = HandleCheckRightsEa(Data, pFullEaInfo, FullEaInfoLength);

			if (NT_SUCCESS(Status))
			{
				CallbackStatus = FLT_PREOP_COMPLETE;
			}

			break;
		}
		else if (0 == RtlCompareString(&TagsEa, &FirstEaName, FALSE))
		{
			Status = HandleTagsEa(Data, pFullEaInfo, FullEaInfoLength);

			if (NT_SUCCESS(Status))
			{
				CallbackStatus = FLT_PREOP_COMPLETE;
			}

			break;
		}
		else if (0 == RtlCompareString(&CheckRightsNoneCacheEa, &FirstEaName, FALSE))
		{
			Status = HandleCheckRightsNoneCacheEa(Data, pFullEaInfo, FullEaInfoLength);

			if (NT_SUCCESS(Status))
			{
				CallbackStatus = FLT_PREOP_COMPLETE;
			}

			break;
		}
		else
		{
			break;
		}

	} while (FALSE);

	if (InstCtx)
	{
		FltReleaseContext(InstCtx);
		InstCtx = NULL;
	}
	
	return CallbackStatus;
}

NTSTATUS HandleIsContentEncryptedEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength)
{
	NTSTATUS Status = STATUS_SUCCESS;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	PFLT_FILE_NAME_INFORMATION	pNameInfo = NULL;

	UNICODE_STRING FileName = {0};

	do 
	{
		Status = FltGetFileNameInformation(Data,
										   FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
										   &pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		Status = FltParseFileNameInformation(pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		FileName.Buffer			= pNameInfo->Name.Buffer;
		FileName.Length			= pNameInfo->Name.Length;
		FileName.MaximumLength	= pNameInfo->Name.MaximumLength;

		if (IsNXLFile(&pNameInfo->Extension))
		{
			FileName.Length -= 4 * sizeof(WCHAR);	// removing ".NXL" extension
		}

		FltAcquirePushLockShared(&Global.NxlFileCacheLock);

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

		FltReleasePushLock(&Global.NxlFileCacheLock);

		//
		// pFullEaInfo point to user mode buffer. We have to protect it with try/catch
		//
		__try
		{
			pFullEaInfo->NextEntryOffset	= 0;
			pFullEaInfo->Flags				= FILE_NEED_EA;
			pFullEaInfo->EaNameLength		= sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) - sizeof(char);
			pFullEaInfo->EaValueLength		= sizeof(UCHAR);

			memcpy(pFullEaInfo->EaName,
				   NXRM_EA_IS_CONTENT_ENCRYPTED,
				   sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED));

			*(UCHAR*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1) = pCacheNode ? NXRM_CONTENT_IS_ENCRYPTED : NXRM_CONTENT_IS_NOT_ENCRYPTED; 

			Data->IoStatus.Status		= STATUS_SUCCESS;
			Data->IoStatus.Information	= (sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_IS_CONTENT_ENCRYPTED) + sizeof(UCHAR));

		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Data->IoStatus.Status		= GetExceptionCode();
			Data->IoStatus.Information	= 0;
		}

	} while (FALSE);

	if (pNameInfo)
	{
		FltReleaseFileNameInformation(pNameInfo);
	}

	return Status;
}

NTSTATUS HandleCheckRightsEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength)
{
	NTSTATUS Status = STATUS_SUCCESS;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	PFLT_FILE_NAME_INFORMATION	pNameInfo = NULL;

	ULONGLONG RightsMask = 0;
	ULONGLONG CustomRights = 0;
	ULONGLONG EvaluationId = 0;

	ULONGLONG *p = NULL;

	UNICODE_STRING FileName = {0};

	do 
	{
		if (!Global.ClientPort)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Status = FltGetFileNameInformation(Data,
										   FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
										   &pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		Status = FltParseFileNameInformation(pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		FileName.Buffer			= pNameInfo->Name.Buffer;
		FileName.Length			= pNameInfo->Name.Length;
		FileName.MaximumLength	= pNameInfo->Name.MaximumLength;

		if (IsNXLFile(&pNameInfo->Extension))
		{
			FileName.Length -= 4 * sizeof(WCHAR);	// removing ".NXL" extension
		}

		FltAcquirePushLockShared(&Global.NxlFileCacheLock);

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

		if (pCacheNode)
		{
			if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
			{
				pCacheNode = NULL;
			}
		}

		FltReleasePushLock(&Global.NxlFileCacheLock);

		if (!pCacheNode)
		{
			//
			// not a NXL file?
			//
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Status = nxrmfltCheckRights(getProcessIDFromData(Data),
									Data->Thread?PsGetThreadId(Data->Thread) : PsGetCurrentThreadId(),
									pCacheNode,
									FALSE,
									&RightsMask,
									&CustomRights,
									&EvaluationId);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		//
		// pFullEaInfo point to user mode buffer. We have to protect it with try/catch
		//
		__try
		{
			
			pFullEaInfo->NextEntryOffset	= 0;
			pFullEaInfo->Flags				= FILE_NEED_EA;
			pFullEaInfo->EaNameLength		= sizeof(NXRM_EA_CHECK_RIGHTS) - sizeof(char);
			pFullEaInfo->EaValueLength		= sizeof(RightsMask) + sizeof(CustomRights) + sizeof(EvaluationId);

			memcpy(pFullEaInfo->EaName,
				   NXRM_EA_CHECK_RIGHTS,
				   sizeof(NXRM_EA_CHECK_RIGHTS));

			p = (ULONGLONG*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

			*(p + 0) = RightsMask;
			*(p + 1) = CustomRights;
			*(p + 2) = EvaluationId;

			Data->IoStatus.Status		= STATUS_SUCCESS;
			Data->IoStatus.Information	= (sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_CHECK_RIGHTS) + sizeof(RightsMask) + sizeof(CustomRights) + sizeof(EvaluationId));

		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Data->IoStatus.Status		= GetExceptionCode();
			Data->IoStatus.Information	= 0;
		}

	} while (FALSE);

	if (pNameInfo)
	{
		FltReleaseFileNameInformation(pNameInfo);
	}

	if (pCacheNode)
	{
		ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
	}

	return Status;

}

NTSTATUS HandleTagsEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength)
{
	NTSTATUS Status = STATUS_SUCCESS;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	PFLT_FILE_NAME_INFORMATION	pNameInfo = NULL;

	UNICODE_STRING FileName = {0};

	HANDLE				SourceFileHandle = NULL;

	FILE_OBJECT			*SourceFileObject = NULL;

	OBJECT_ATTRIBUTES	SourceObjectAttribute = {0};

	IO_STATUS_BLOCK		IoStatus = {0};

	FILE_STANDARD_INFORMATION FileStandardInfo = { 0 };

	FILE_POSITION_INFORMATION FilePositionInfo = { 0 };

	NXL_SECTION_2 SectionInfo = { 0 };

	LIST_ENTRY	*ite = NULL;

	UCHAR *pEaValue = NULL;

	ULONG  TotalTagsSize = 0;

	BOOLEAN	RestoreSourceFile = FALSE;

	ULONG SessionId = 0;

	NXL_TOKEN Token = { 0 };

	UCHAR IvSeed[16] = { 0 };

	do 
	{
		if (!Global.ClientPort)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Status = FltGetFileNameInformation(Data,
										   FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
										   &pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		Status = FltParseFileNameInformation(pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		FileName.Buffer			= pNameInfo->Name.Buffer;
		FileName.Length			= pNameInfo->Name.Length;
		FileName.MaximumLength	= pNameInfo->Name.MaximumLength;

		if (IsNXLFile(&pNameInfo->Extension))
		{
			FileName.Length -= 4 * sizeof(WCHAR);	// removing ".NXL" extension
		}

		FltAcquirePushLockShared(&Global.NxlFileCacheLock);

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

		if (pCacheNode)
		{
			if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
			{
				pCacheNode = NULL;
			}
		}

		FltReleasePushLock(&Global.NxlFileCacheLock);

		if (!pCacheNode)
		{
			//
			// not a NXL file?
			//
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Status = FltGetRequestorSessionId(Data, &SessionId);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		InitializeObjectAttributes(&SourceObjectAttribute,
								   &pCacheNode->OriginalFileName,
								   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
								   NULL,
								   NULL);

		Status = FltCreateFileEx2(Global.Filter,
								  pCacheNode->Instance,
								  &SourceFileHandle,
								  &SourceFileObject,
								  FILE_GENERIC_READ,
								  &SourceObjectAttribute,
								  &IoStatus,
								  NULL,
								  FILE_ATTRIBUTE_NORMAL,
								  FILE_SHARE_VALID_FLAGS,
								  FILE_OPEN,
								  FILE_NON_DIRECTORY_FILE | FILE_SEQUENTIAL_ONLY | FILE_SYNCHRONOUS_IO_NONALERT,
								  NULL,
								  0,
								  IO_IGNORE_SHARE_ACCESS_CHECK,
								  NULL);

		if (!NT_SUCCESS(Status))
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
						 ("nxrmflt!HandleTagsEa: FltCreateFileEx2 return %x for %wZ\n",
						 Status,
						 &pCacheNode->OriginalFileName));
			break;
		}

		Status = FltQueryInformationFile(pCacheNode->Instance,
										 SourceFileObject,
										 &FileStandardInfo,
										 sizeof(FileStandardInfo),
										 FileStandardInformation,
										 NULL);

		if (!NT_SUCCESS(Status))
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
						 ("nxrmflt!HandleTagsEa: FltQueryInformationFile -> FileStandardInformation return %x for %wZ\n",
						 Status,
						 &pCacheNode->OriginalFileName));

			break;
		}

		Status = FltQueryInformationFile(pCacheNode->Instance,
										 SourceFileObject,
										 &FilePositionInfo,
										 sizeof(FilePositionInfo),
										 FilePositionInformation,
										 NULL);

		if (!NT_SUCCESS(Status))
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
						 ("nxrmflt!HandleTagsEa: FltQueryInformationFile -> FilePositionInformation return %x for %wZ\n",
						 Status,
						 &pCacheNode->OriginalFileName));

			break;
		}

		RestoreSourceFile = TRUE;

		if(FileStandardInfo.EndOfFile.QuadPart < sizeof(NXL_HEADER_2))
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
						 ("nxrmflt!HandleTagsEa: File %wZ is not a valid Nxl file. File size %I64u is too small.\n",
						 &pCacheNode->OriginalFileName,
						 FileStandardInfo.EndOfFile.QuadPart));

			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Status = NXLCheck(pCacheNode->Instance, SourceFileObject);

		if(!NT_SUCCESS(Status))
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
						 ("nxrmflt!HandleTagsEa: File %wZ is not a valid Nxl file. Status is %x\n",
						 &pCacheNode->OriginalFileName,
						 Status));

			break;
		}

		//
		// not necessary, just for debugging purpose
		Status = NXLQuerySection(pCacheNode->Instance,
								 SourceFileObject,
								 NXL_SECTION_NAME_FILETAG,
								 &SectionInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}
		
		Status = nxrmfltFetchIVSeedFromFile(pCacheNode->Instance,
										    &pCacheNode->OriginalFileName,
											IvSeed);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		Status = nxrmfltQueryToken(getProcessIDFromData(Data),
								   SessionId,
								   &pCacheNode->OriginalFileName,
								   &Token);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		//
		// pFullEaInfo point to user mode buffer. We have to protect it with try/catch
		//
		__try
		{

			pFullEaInfo->NextEntryOffset	= (ULONG)(sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_TAG) + sizeof(NXL_SECTION_NAME_FILETAG));
			pFullEaInfo->Flags				= FILE_NEED_EA;
			pFullEaInfo->EaNameLength		= sizeof(NXRM_EA_TAG) - sizeof(char);
			pFullEaInfo->EaValueLength		= sizeof(NXL_SECTION_NAME_FILETAG);

			memcpy(pFullEaInfo->EaName,
				   NXRM_EA_TAG,
				   sizeof(NXRM_EA_TAG));

			pEaValue = ((UCHAR*)pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

			memcpy(pEaValue, NXL_SECTION_NAME_FILETAG, sizeof(NXL_SECTION_NAME_FILETAG));

			pFullEaInfo = (FILE_FULL_EA_INFORMATION *)((UCHAR*)pFullEaInfo + pFullEaInfo->NextEntryOffset);

			pFullEaInfo->NextEntryOffset	= 0;
			pFullEaInfo->Flags				= FILE_NEED_EA;
			pFullEaInfo->EaNameLength		= sizeof(NXRM_EA_SYNC_HEADER) - 1;
			pFullEaInfo->EaValueLength		= (USHORT)TotalTagsSize;

			memcpy(pFullEaInfo->EaName, 
				   NXRM_EA_SYNC_HEADER, 
				   sizeof(NXRM_EA_SYNC_HEADER));

			pEaValue = ((UCHAR*)pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

			Status = NXLGetSectionData(pCacheNode->Instance,
									   SourceFileObject,
									   NXL_SECTION_NAME_FILETAG,
									   Token.Token,
									   IvSeed,
									   pEaValue,
									   &TotalTagsSize);
			if (!NT_SUCCESS(Status))
			{
				break;
			}

		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Data->IoStatus.Status		= GetExceptionCode();
			Data->IoStatus.Information	= 0;
		}

		Data->IoStatus.Status		= STATUS_SUCCESS;
		Data->IoStatus.Information	= sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_TAG) + sizeof(NXL_SECTION_NAME_FILETAG) + \
									  sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_SYNC_HEADER) + TotalTagsSize; 

	} while (FALSE);


	if (RestoreSourceFile)
	{
		//
		// restore source file attributes
		//

		Status = FltSetInformationFile(pCacheNode->Instance,
									   SourceFileObject,
									   &FilePositionInfo,
									   sizeof(FilePositionInfo),
									   FilePositionInformation);

	}

	if (SourceFileHandle)
	{
		FltClose(SourceFileHandle);
		SourceFileHandle = NULL;
	}

	if (SourceFileObject)
	{
		ObDereferenceObject(SourceFileObject);
	}

	if (pNameInfo)
	{
		FltReleaseFileNameInformation(pNameInfo);
	}

	if (pCacheNode)
	{
		ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
	}

	return Status;

}

NTSTATUS HandleCheckRightsNoneCacheEa(PFLT_CALLBACK_DATA Data, FILE_FULL_EA_INFORMATION *pFullEaInfo, ULONG FullEaInfoLength)
{
	NTSTATUS Status = STATUS_SUCCESS;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	PFLT_FILE_NAME_INFORMATION	pNameInfo = NULL;

	ULONGLONG RightsMask = 0;
	ULONGLONG CustomRights = 0;
	ULONGLONG EvaluationId = 0;

	ULONGLONG *p = NULL;

	UNICODE_STRING FileName = { 0 };

	do
	{
		if (!Global.ClientPort)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Status = FltGetFileNameInformation(Data,
										   FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
										   &pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		Status = FltParseFileNameInformation(pNameInfo);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		FileName.Buffer = pNameInfo->Name.Buffer;
		FileName.Length = pNameInfo->Name.Length;
		FileName.MaximumLength = pNameInfo->Name.MaximumLength;

		if (IsNXLFile(&pNameInfo->Extension))
		{
			FileName.Length -= 4 * sizeof(WCHAR);	// removing ".NXL" extension
		}

		FltAcquirePushLockShared(&Global.NxlFileCacheLock);

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

		if (pCacheNode)
		{
			if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
			{
				pCacheNode = NULL;
			}
		}

		FltReleasePushLock(&Global.NxlFileCacheLock);

		if (!pCacheNode)
		{
			//
			// not a NXL file?
			//
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Status = nxrmfltCheckRights(getProcessIDFromData(Data),
									Data->Thread ? PsGetThreadId(Data->Thread) : PsGetCurrentThreadId(),
									pCacheNode,
									TRUE,
									&RightsMask,
									&CustomRights,
									&EvaluationId);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		//
		// pFullEaInfo point to user mode buffer. We have to protect it with try/catch
		//
		__try
		{

			pFullEaInfo->NextEntryOffset = 0;
			pFullEaInfo->Flags = FILE_NEED_EA;
			pFullEaInfo->EaNameLength = sizeof(NXRM_EA_CHECK_RIGHTS_NONECACHE) - sizeof(char);
			pFullEaInfo->EaValueLength = sizeof(RightsMask) + sizeof(CustomRights) + sizeof(EvaluationId);

			memcpy(pFullEaInfo->EaName,
				   NXRM_EA_CHECK_RIGHTS_NONECACHE,
				   sizeof(NXRM_EA_CHECK_RIGHTS_NONECACHE));

			p = (ULONGLONG*)(pFullEaInfo->EaName + pFullEaInfo->EaNameLength + 1);

			*(p + 0) = RightsMask;
			*(p + 1) = CustomRights;
			*(p + 2) = EvaluationId;

			Data->IoStatus.Status = STATUS_SUCCESS;
			Data->IoStatus.Information = (sizeof(FILE_FULL_EA_INFORMATION) - 1 + sizeof(NXRM_EA_CHECK_RIGHTS_NONECACHE) + sizeof(RightsMask) + sizeof(CustomRights) + sizeof(EvaluationId));

		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Data->IoStatus.Status = GetExceptionCode();
			Data->IoStatus.Information = 0;
		}

	} while (FALSE);

	if (pNameInfo)
	{
		FltReleaseFileNameInformation(pNameInfo);
	}

	if (pCacheNode)
	{
		ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
	}

	return Status;

}