#include "nxrmfltdef.h"
#include "nxrmfltdirectorycontrol.h"
#include "nxrmfltnxlcachemgr.h"
#include "nxrmfltutils.h"

extern DECLSPEC_CACHEALIGN PFLT_FILTER			gFilterHandle;
extern DECLSPEC_CACHEALIGN ULONG				gTraceFlags;
extern DECLSPEC_CACHEALIGN NXRMFLT_GLOBAL_DATA	Global;

extern BOOLEAN is_explorer(void);
extern BOOLEAN is_java(void);

static BOOLEAN IsNXLFile(WCHAR *FileName, ULONG LengthInByte);
static NTSTATUS build_nxlcache_file_name_with_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);
static NTSTATUS build_nxlcache_file_name_without_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);

NTSTATUS build_nxlcache_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);
NTSTATUS build_nxlcache_reparse_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);

BOOLEAN strMatch(PCUNICODE_STRING String)
{
    WCHAR nameBuf[NXRMFLT_MAX_PATH];

    if (String->Length / sizeof(WCHAR) < NXRMFLT_MAX_PATH) {
        wcsncpy(nameBuf, String->Buffer, String->Length / sizeof(WCHAR));
        nameBuf[String->Length / sizeof(WCHAR)] = L'\0';
        for (size_t i = 0; i < String->Length / sizeof(WCHAR); i++) {
            nameBuf[i] = towlower(nameBuf[i]);
        }

        UNICODE_STRING KeyPath, ValueName, ValueData;
        RtlInitUnicodeString(&KeyPath, L"\\Registry\\Machine\\Software\\NextLabs\\SkyDRM");
        RtlInitUnicodeString(&ValueName, L"TestPathSubstring");

        if (!NT_SUCCESS(NkRegReadStringValue(&KeyPath, &ValueName, TRUE, NXRMFLT_TMP_TAG, &ValueData)))
        {
            return FALSE;
        }

        WCHAR strToMatch[NXRMFLT_MAX_PATH];
        size_t strToMatchLen = ValueData.Length / sizeof(WCHAR);
        for (size_t i = 0; i < strToMatchLen; i++) {
            strToMatch[i] = towlower(ValueData.Buffer[i]);
        }
        strToMatch[strToMatchLen] = L'\0';

        const BOOLEAN ret = (wcsstr(nameBuf, strToMatch) != NULL);
        ExFreePoolWithTag(ValueData.Buffer, NXRMFLT_TMP_TAG);

        return ret;
    }

    return FALSE;
}

LONG volatile logIndex = 0;

FLT_PREOP_CALLBACK_STATUS
nxrmfltPreDirectoryControl(
_Inout_ PFLT_CALLBACK_DATA				Data,
_In_ PCFLT_RELATED_OBJECTS				FltObjects,
_Flt_CompletionContext_Outptr_ PVOID	*CompletionContext
)
{
    wchar_t szBuffer[1024] = { 0 };

	if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
	{
		PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen enter, current thread PID:%p TID:%p, dir %wZ\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), &Data->Iopb->TargetFileObject->FileName));
	
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen enter, dir %wZ\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), &Data->Iopb->TargetFileObject->FileName);
            nxWriteToLogFile(szBuffer);
        }
    }

	FLT_PREOP_CALLBACK_STATUS CallbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

	PFLT_INSTANCE	FltInstance = NULL;
	
	NXRMFLT_DIRCTL_CONTEXT	*DirCtlCtx = NULL;

	WCHAR	*FullPath = NULL;
	PUNICODE_STRING	OriginalFileName = NULL;
	ULONG	DirctoryNameLength = 0;

	PFLT_FILE_NAME_INFORMATION	NameInfo = NULL;

	NTSTATUS Status = STATUS_SUCCESS;

	BOOLEAN ReleaseNameInfo = TRUE;
	BOOLEAN ReleaseFullPath = TRUE;
	BOOLEAN ReleaseDirCtlCtx = TRUE;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	NXRMFLT_INSTANCE_CONTEXT	*InstCtx = NULL;

	ULONGLONG RightsMask = 0;

	ULONG ContentDirty = 0;

	FltInstance = FltObjects->Instance;

	do 
	{
		Status = FltGetInstanceContext(FltInstance, &InstCtx);

        if (strMatch(&Data->Iopb->TargetFileObject->FileName))
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, Status=0x%08lX\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), Status);
            nxWriteToLogFile(szBuffer);
        }

		if (!NT_SUCCESS(Status))
		{
			if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, current thread PID:%p TID:%p, FltGetInstanceContext failed, Status=0x%08lX\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), Status));
            
                {
                    memset(szBuffer, 0, sizeof(szBuffer));
                    swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, FltGetInstanceContext failed, Status=0x%08lX\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), Status);
                    nxWriteToLogFile(szBuffer);
                }
            
            }
			break;
		}

		if (InstCtx->DisableFiltering)
		{
			if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, current thread PID:%p TID:%p, DisableFiltering=%u\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), (unsigned int)InstCtx->DisableFiltering));
			
                {
                    memset(szBuffer, 0, sizeof(szBuffer));
                    swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, DisableFiltering=%u\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), (unsigned int)InstCtx->DisableFiltering);
                    nxWriteToLogFile(szBuffer);
                }

            }
			break;
		}

		if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, current thread PID:%p TID:%p, ReparseInstance=%p, MinorFunction=0x%02hX, Length=%lu, CompletionFilter=0x%08lX, NotifyDirectory.Length=%lu, DirectoryBuffer=%p\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), Global.ReparseInstance, (unsigned short) Data->Iopb->MinorFunction, Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length, Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.CompletionFilter, Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length, Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer));
		
            {
                memset(szBuffer, 0, sizeof(szBuffer));
                swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, ReparseInstance=%p, MinorFunction=0x%02hX, Length=%lu, CompletionFilter=0x%08lX, NotifyDirectory.Length=%lu, DirectoryBuffer=%p\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), Global.ReparseInstance, (unsigned short)Data->Iopb->MinorFunction, Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length, Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.CompletionFilter, Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length, Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer);
                nxWriteToLogFile(szBuffer);
            }
        
        }
		if (FltInstance != Global.ReparseInstance &&
			Data->Iopb->MinorFunction == IRP_MN_QUERY_DIRECTORY &&
			Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length > 0)
		{
	
			OriginalFileName = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName;

			Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &NameInfo);

			if (!NameInfo)
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreDirectoryControl: Can't get name information when browsing directory\n"));

                {
                    memset(szBuffer, 0, sizeof(szBuffer));
                    swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl: Can't get name information when browsing directory");
                    nxWriteToLogFile(szBuffer);
                }

				break;
			}

			DirctoryNameLength = NameInfo->Name.Length / sizeof(WCHAR);

			FullPath = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);

			if (!FullPath)
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreDirectoryControl: Can't allocate from full path lookaside\n"));
				break;
			}

			DirCtlCtx = ExAllocateFromPagedLookasideList(&Global.DirCtlCtxLookaside);

			if (!DirCtlCtx)
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreDirectoryControl: Can't allocate from DirCtlCtx lookaside\n"));
				break;
			}

            DirCtlCtx->HideExt = 0; // Global.HideNXLExtension;

            if (NameInfo->Name.Length > NameInfo->Volume.Length)
            {
                UNICODE_STRING PathPart = { NameInfo->Name.Length - NameInfo->Volume.Length,
                    NameInfo->Name.Length - NameInfo->Volume.Length,
                    NameInfo->Name.Buffer + NameInfo->Volume.Length / 2 };
                if (nxrmfltInsideSafeDir(FltInstance, &PathPart, FALSE, NULL, NULL))
                {
                    DirCtlCtx->HideExt = 1;
                }
            }

			//
			// make sure it's a file instead of a filter before searching
			//
			if (OriginalFileName && OriginalFileName->Length > 3 * sizeof(WCHAR))
			{
				DirCtlCtx->FileName.Buffer = FullPath;
				DirCtlCtx->FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
				DirCtlCtx->FileName.Length = 0;

				memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

				RtlUnicodeStringCat(&DirCtlCtx->FileName, &NameInfo->Name);

				if (DirctoryNameLength != 0 && NameInfo->Name.Buffer[DirctoryNameLength - 1] != L'\\')
					RtlUnicodeStringCatString(&DirCtlCtx->FileName, L"\\");

				RtlUnicodeStringCat(&DirCtlCtx->FileName, OriginalFileName);


				//
				// find the file in NXL cache
				//

				FltAcquirePushLockShared(&Global.NxlFileCacheLock);

				pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &DirCtlCtx->FileName);

				if (pCacheNode)
				{
					if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
					{
						pCacheNode = NULL;
					}
				}

				FltReleasePushLock(&Global.NxlFileCacheLock);
			}

			if (pCacheNode)
			{
				//
				// found the file, need to modify callback data
				//
				DirCtlCtx->FileName.Buffer = FullPath;
				DirCtlCtx->FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
				DirCtlCtx->FileName.Length = 0;

				memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

				RtlUnicodeStringCat(&DirCtlCtx->FileName, OriginalFileName);
				RtlUnicodeStringCatString(&DirCtlCtx->FileName, NXRMFLT_NXL_DOTEXT);

				DirCtlCtx->OriginalFileName = (PVOID)OriginalFileName;
				DirCtlCtx->NameInfo			= NameInfo;

				/*if (Global.ClientPort)
				{
					Status = nxrmfltCheckRights(getProcessIDFromData(Data),
												Data->Thread?PsGetThreadId(Data->Thread) : PsGetCurrentThreadId(),
												pCacheNode,
												FALSE,
												&RightsMask,
												NULL,
												NULL);

					if (NT_SUCCESS(Status))
					{
						if (FlagOn(RightsMask, BUILTIN_RIGHT_VIEW))
						{
							DirCtlCtx->HideExt = 1; 
						}
					}
				}*/
	
				//
				// always hide extension for explorer
				//
				//if (is_explorer())
				//{
				//	DirCtlCtx->HideExt = 1;
				//}

				ReleaseDirCtlCtx= FALSE;
				ReleaseFullPath	= FALSE;
				ReleaseNameInfo = FALSE;

				*CompletionContext = DirCtlCtx;

				Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName = &DirCtlCtx->FileName;

				FltSetCallbackDataDirty(Data);
			}
			else
			{
				RtlInitUnicodeString(&DirCtlCtx->FileName, NULL);

				DirCtlCtx->OriginalFileName = (PVOID)OriginalFileName;
				DirCtlCtx->NameInfo = NameInfo;

				nxrmfltCheckHideNXLExtsionByProcessId(getProcessIDFromData(Data), &DirCtlCtx->HideExt);

				ReleaseDirCtlCtx = FALSE;
				ReleaseNameInfo = FALSE;

				*CompletionContext = DirCtlCtx;
			}

			CallbackStatus = FLT_PREOP_SYNCHRONIZE;
		}
		else if (Data->Iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY &&
				 FlagOn(Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.CompletionFilter, FILE_NOTIFY_CHANGE_FILE_NAME) &&
				 Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length &&
				 Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer)
		{
			
			Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &NameInfo);

			if (!NT_SUCCESS(Status) || (!NameInfo))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreDirectoryControl: Can't get name information when browsing directory\n"));

                {
                    memset(szBuffer, 0, sizeof(szBuffer));
                    swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl: Can't get name information when browsing directory [%ld] line %d PID %p: ayuen, Status=0x%08lX \n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), Status);
                    nxWriteToLogFile(szBuffer);
                }

				break;
			}

			Status = nxrmfltScanNotifyChangeDirectorySafe(&NameInfo->Name,
														  (FILE_NOTIFY_INFORMATION*)Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer,
														  Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length,
														  &ContentDirty);

			if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, current thread PID:%p TID:%p, Status=0x%08lX, ContentDirty=%lu\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), Status, ContentDirty));
			
                {
                    memset(szBuffer, 0, sizeof(szBuffer));
                    swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen, Status=0x%08lX, ContentDirty=%lu\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), Status, ContentDirty);
                    nxWriteToLogFile(szBuffer);
                }
            }
			if (NT_SUCCESS(Status) && ContentDirty)
			{
				Data->IoStatus.Status		= Status;
				Data->IoStatus.Information	= 0;

				CallbackStatus = FLT_PREOP_COMPLETE;
			}
		}

	} while (FALSE);

	if (ReleaseDirCtlCtx && DirCtlCtx)
	{
		ExFreeToPagedLookasideList(&Global.DirCtlCtxLookaside, DirCtlCtx);
	}

	if (ReleaseFullPath && FullPath)
	{
		ExFreeToPagedLookasideList(&Global.FullPathLookaside, FullPath);
	}

	if (ReleaseNameInfo && NameInfo)
	{
		FltReleaseFileNameInformation(NameInfo);
	}

	if (pCacheNode)
	{
		ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
	}

	if (InstCtx)
	{
		FltReleaseContext(InstCtx);
		InstCtx = NULL;
	}
	
    if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
    {
        PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen leave, current thread PID:%p TID:%p, dir %wZ, returning %d\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), &Data->Iopb->TargetFileObject->FileName, (int) CallbackStatus));
    
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreDirectoryControl [%ld] line %d PID %p: ayuen leave, dir %wZ, returning %d\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), &Data->Iopb->TargetFileObject->FileName, (int)CallbackStatus);
            nxWriteToLogFile(szBuffer);
        }
    }

	return CallbackStatus;
}

FLT_POSTOP_CALLBACK_STATUS
	nxrmfltPostDirectoryControl(
	_Inout_ PFLT_CALLBACK_DATA		Data,
	_In_ PCFLT_RELATED_OBJECTS		FltObjects,
	_In_opt_ PVOID					CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS	Flags
	)
{
    wchar_t szBuffer[1024] = { 0 };

    if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
    {
        PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPostDirectoryControl [%ld] line %d PID %p: ayuen enter, current thread PID:%p TID:%p, dir %wZ, Flags=0x%08lX\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), &Data->Iopb->TargetFileObject->FileName, Flags));
    
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPostDirectoryControl [%ld] line %d PID %p: ayuen enter, dir %wZ, Flags=0x%08lX\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), &Data->Iopb->TargetFileObject->FileName, Flags);
            nxWriteToLogFile(szBuffer);
        }
    }

	FLT_POSTOP_CALLBACK_STATUS	CallbackStatus = FLT_POSTOP_FINISHED_PROCESSING;

	NTSTATUS Status = STATUS_SUCCESS;

	PUNICODE_STRING UniArgFileName = { 0 };
	FILE_INFORMATION_CLASS FileInformationClass;
	ULONG FileIndex = 0;

	NXRMFLT_INSTANCE_CONTEXT	*InstCtx = NULL;

	ULONG i = 0;
	PUCHAR Buffer = NULL;
	CLONG UserBufferLength = 0;

	const UNICODE_STRING EmptryString = { 0 };

	BOOLEAN RestartScan = FALSE;
	BOOLEAN ReturnSingleEntry = FALSE;
	BOOLEAN IndexSpecified = FALSE;

	BOOLEAN InitialQuery = TRUE;
	BOOLEAN StopLoop = FALSE;
	BOOLEAN HideNXLExtension = FALSE;
	BOOLEAN PushLockAcquired = FALSE;

	NXL_CACHE_NODE	*pNode = NULL;

	FLT_FILE_NAME_INFORMATION *pNameInfo = NULL;

	WCHAR	*FullPath = NULL;

	NXRMFLT_DIRCTL_CONTEXT *DirCtlCtx = NULL;

	BOOLEAN	UpdateQueryResults = FALSE;

	PFLT_INSTANCE	FltInstance = FltObjects->Instance;

	do 
	{
		Status					= Data->IoStatus.Status;
		FileInformationClass	= Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
		FileIndex				= Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileIndex;
		UniArgFileName			= Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName;
		
        HideNXLExtension = FALSE; // Global.HideNXLExtension ? TRUE : FALSE;

		RestartScan				= BooleanFlagOn(Data->Iopb->OperationFlags, SL_RESTART_SCAN);
		ReturnSingleEntry		= BooleanFlagOn(Data->Iopb->OperationFlags, SL_RETURN_SINGLE_ENTRY);
		IndexSpecified			= BooleanFlagOn(Data->Iopb->OperationFlags, SL_INDEX_SPECIFIED);
		FileInformationClass	= Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
		DirCtlCtx				= (NXRMFLT_DIRCTL_CONTEXT *)CompletionContext;

		if (DirCtlCtx->FileName.Length)
		{
			Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName = DirCtlCtx->OriginalFileName;

			if (!DirCtlCtx->HideExt)
			{
				break;
			}
		}

		if (!DirCtlCtx->HideExt)
		{
			break;
		}

		if (!NT_SUCCESS(Data->IoStatus.Status))
		{
			break;
		}

		Status = FltGetInstanceContext(FltInstance, &InstCtx);

		if(!InstCtx)
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostDirectoryControl: Failed to get instance ctx! Status is %x\n",Status));
			break;
		}

		HideNXLExtension = DirCtlCtx->HideExt ? TRUE: FALSE;

		//
		//
		// TO DO: don't use FltLockUserBuffer and MmGetSystenAddressForMdlSafe to improve performance
		//
		if (!Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress)
		{
			Buffer = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
		}
		else
		{
			Status = FltLockUserBuffer(Data);

			if (!NT_SUCCESS(Status))
			{
				//
				// out of memory?
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostDirectoryControl: Failed to lock user buffer! Status is %x.\n",Status));
				break;
			}

			Buffer = MmGetSystemAddressForMdlSafe(Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress, NormalPagePriority);
		}

		UserBufferLength = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length;

		pNameInfo = DirCtlCtx->NameInfo;

		FullPath = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);

		if (!FullPath)
		{
			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostDirectoryControl: Can't allocate full path buffer from lookaside.\n"));

			break;
		}

		//
		//  Display the input values.
		//
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, ("nxrmfltPostDirectoryControl...\n"));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->Status               = %08lx\n", Status));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->Length               = %08lx\n", UserBufferLength));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->FileName             = %wZ\n", UniArgFileName ? UniArgFileName : &EmptryString));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->FileInformationClass = %08lx\n", FileInformationClass));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->FileIndex            = %08lx\n", FileIndex));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->UserBuffer           = %08lp\n", Buffer));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->RestartScan          = %08lx\n", RestartScan));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->ReturnSingleEntry    = %08lx\n", ReturnSingleEntry));
		PT_DBG_PRINT(PTDBG_TRACE_DIRCONTROL, (" ->IndexSpecified       = %08lx\n", IndexSpecified));


		switch (FileInformationClass)
		{
		case FileIdBothDirectoryInformation:
			{
	
				__try
				{

					PFILE_ID_BOTH_DIR_INFORMATION IdBothDirInfo = NULL;

					IdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)Buffer;

					BOOLEAN bNeedScanAgain = FALSE;

					while (!StopLoop)
					{
						NT_ASSERT((ULONG_PTR)(IdBothDirInfo) < (ULONG_PTR)((UCHAR*)Buffer + UserBufferLength));

						if (!IdBothDirInfo->NextEntryOffset)
						{
							StopLoop = TRUE;
						}

						if (!FlagOn(IdBothDirInfo->FileAttributes,FILE_ATTRIBUTE_DIRECTORY) &&
							IdBothDirInfo->FileNameLength >= 5 * sizeof(WCHAR) &&
							IsNXLFile(IdBothDirInfo->FileName, IdBothDirInfo->FileNameLength))
						{
							UNICODE_STRING	FileName;
							UNICODE_STRING	TmpName;
							ULONG DirHash = 0;

							FileName.Buffer = FullPath;
							FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileName.Length = 0;

							if (HideNXLExtension)
							{
								//
								// removing ".nxl" extsion
								//
								IdBothDirInfo->FileNameLength -= 4 * sizeof(WCHAR);
							}

							TmpName.Buffer = IdBothDirInfo->FileName;
							TmpName.Length = TmpName.MaximumLength = HideNXLExtension ? (USHORT)IdBothDirInfo->FileNameLength : (USHORT)(IdBothDirInfo->FileNameLength - 4 * sizeof(WCHAR));

							memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

							RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

							if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
								RtlUnicodeStringCatString(&FileName, L"\\");
							
							RtlHashUnicodeString(&FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

							RtlUnicodeStringCat(&FileName, &TmpName);

							FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

							PushLockAcquired = TRUE;

							do
							{
								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if (!pNode)
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode can't find file %wZ in cache\n", &FileName));

									pNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

									if (pNode)
									{
										memset(pNode, 0, sizeof(NXL_CACHE_NODE));

										Status = build_nxlcache_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										pNode->FileAttributes = IdBothDirInfo->FileAttributes;
										pNode->FileID = IdBothDirInfo->FileId;
										pNode->Flags |= FlagOn(IdBothDirInfo->FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
										pNode->Instance = FltInstance;
										pNode->ParentDirectoryHash = DirHash;
										pNode->OnRemoveOrRemovableMedia = FALSE;
										pNode->LastWriteProcessId = NULL;

										ExInitializeRundownProtection(&pNode->NodeRundownRef);

										Status = build_nxlcache_reparse_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											if (pNode->ReleaseFileName)
											{
												ExFreePoolWithTag(pNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
												RtlInitUnicodeString(&pNode->FileName, NULL);
											}

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &FileName));

										AddNXLNodeToCache(&Global.NxlFileCache, pNode);

										bNeedScanAgain = TRUE;
									}
								}
								else
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode found file %wZ in cache\n", &FileName));

									pNode->FileAttributes = IdBothDirInfo->FileAttributes;
									pNode->FileID = IdBothDirInfo->FileId;

									if (FlagOn(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED))
									{
										//
										// This means ejected device come back. Files on this device could have been changed
										// We need to clear the NXRMFLT_FLAG_CTX_ATTACHED flag to force a copy when opening
										// There are two possible outcome as clear the NXRMFLT_FLAG_CTX_ATTACHED flag:
										//		A) File was opened and still opened by some process. Copying should fail
										//		B) File is closed by copying. Content is overwritten. Copying should be OK
										//
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED);
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									}
								}

							} while (FALSE);

							FltReleasePushLock(&Global.NxlFileCacheLock);

							PushLockAcquired = FALSE;
						}

						do
						{
							PFILE_ID_BOTH_DIR_INFORMATION NextIdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((UCHAR*)IdBothDirInfo + IdBothDirInfo->NextEntryOffset);
							UNICODE_STRING	FileName = { 0 };
							UNICODE_STRING	TmpName = { 0 };
							BOOLEAN	CtxAttached = FALSE;
							BOOLEAN	AttachingCtx = FALSE;

							do 
							{
								if(StopLoop)
								{
									break;
								}

								FileName.Buffer = FullPath;
								FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
								FileName.Length = 0;

								TmpName.Buffer = NextIdBothDirInfo->FileName;
								TmpName.Length = TmpName.MaximumLength = (USHORT)NextIdBothDirInfo->FileNameLength;

								memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

								RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

								if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
									RtlUnicodeStringCatString(&FileName, L"\\");

								RtlUnicodeStringCat(&FileName, &TmpName);

								FltAcquirePushLockShared(&Global.NxlFileCacheLock);

								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if(pNode)
								{
									CtxAttached = FlagOn(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									AttachingCtx = FlagOn(pNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
								}

								FltReleasePushLock(&Global.NxlFileCacheLock);

								if (!pNode) // go loop to handle this file.
									break;
								//hide the temporary unencrypted file
								if(!NextIdBothDirInfo->NextEntryOffset)
								{
									IdBothDirInfo->NextEntryOffset = 0;
									StopLoop = TRUE;
								}
								else
								{
									IdBothDirInfo->NextEntryOffset += NextIdBothDirInfo->NextEntryOffset;
									NextIdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((UCHAR*)IdBothDirInfo + IdBothDirInfo->NextEntryOffset);
								}

								//
								// in case there is a unencrypted version exist but there is no ctx attached to it or in the process of attaching
								//
								if((!CtxAttached) && (!AttachingCtx))
								{
									Status = nxrmfltDeleteFileByName(FltInstance, &FileName);

									PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltIdBothDirPostDirectoryControl: found %wZ but there is no Ctx attach to it!!! Delete return Status %x.\n", &FileName, Status));
								}
							} while (TRUE);

						} while (FALSE);

						IdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((UCHAR*)IdBothDirInfo + IdBothDirInfo->NextEntryOffset);
					}

					if (bNeedScanAgain)
					{
						IdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)Buffer;
						do
						{
							if (!IdBothDirInfo->NextEntryOffset)
							{
								break;
							}

							PFILE_ID_BOTH_DIR_INFORMATION NextIdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((UCHAR*)IdBothDirInfo + IdBothDirInfo->NextEntryOffset);
							UNICODE_STRING	FileName = { 0 };
							UNICODE_STRING	TmpName = { 0 };

							FileName.Buffer = FullPath;
							FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileName.Length = 0;

							TmpName.Buffer = NextIdBothDirInfo->FileName;
							TmpName.Length = TmpName.MaximumLength = (USHORT)NextIdBothDirInfo->FileNameLength;

							memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

							RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

							if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
								RtlUnicodeStringCatString(&FileName, L"\\");

							RtlUnicodeStringCat(&FileName, &TmpName);

							FltAcquirePushLockShared(&Global.NxlFileCacheLock);
							pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);
							FltReleasePushLock(&Global.NxlFileCacheLock);

							if (pNode && NextIdBothDirInfo->FileId.QuadPart != pNode->FileID.QuadPart)
							{
								//hide the temporary unencrypted file
								if (!NextIdBothDirInfo->NextEntryOffset)
								{
									IdBothDirInfo->NextEntryOffset = 0;
									break;
								}
								else
								{
									IdBothDirInfo->NextEntryOffset += NextIdBothDirInfo->NextEntryOffset;
								}
							}
							IdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((UCHAR*)IdBothDirInfo + IdBothDirInfo->NextEntryOffset);
						} while (TRUE);
					}
				}
				__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					if (PushLockAcquired)
					{
						FltReleasePushLock(&Global.NxlFileCacheLock);
						PushLockAcquired = FALSE;
					}
				}

				break;
			}
		case FileFullDirectoryInformation:
			{
			
				__try
				{

					PFILE_FULL_DIR_INFORMATION FullDirInfo = NULL;

					FullDirInfo = (PFILE_FULL_DIR_INFORMATION)Buffer;

					while (!StopLoop)
					{
						NT_ASSERT((ULONG_PTR)(FullDirInfo) < (ULONG_PTR)((UCHAR*)Buffer + UserBufferLength));

						if (!FullDirInfo->NextEntryOffset)
						{
							StopLoop = TRUE;
						}

						if (!FlagOn(FullDirInfo->FileAttributes,FILE_ATTRIBUTE_DIRECTORY) &&
							FullDirInfo->FileNameLength >= 5 * sizeof(WCHAR) &&
							IsNXLFile(FullDirInfo->FileName, FullDirInfo->FileNameLength))
						{
							UNICODE_STRING	FileName;
							UNICODE_STRING	TmpName;
							ULONG DirHash = 0;

							FileName.Buffer = FullPath;
							FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileName.Length = 0;
							
							if (HideNXLExtension)
							{
								//
								// removing ".nxl" extsion
								//
								FullDirInfo->FileNameLength -= 4 * sizeof(WCHAR);
							}

							TmpName.Buffer = FullDirInfo->FileName;
							TmpName.Length = TmpName.MaximumLength = HideNXLExtension ? (USHORT)FullDirInfo->FileNameLength : (USHORT)(FullDirInfo->FileNameLength - 4 * sizeof(WCHAR));

							memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

							RtlUnicodeStringCat(&FileName, &pNameInfo->Name);
						
							if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
								RtlUnicodeStringCatString(&FileName, L"\\");

							RtlHashUnicodeString(&FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

							RtlUnicodeStringCat(&FileName, &TmpName);

							FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

							PushLockAcquired = TRUE;

							do
							{
								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if (!pNode)
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode can't find file %wZ in cache\n", &FileName));

									pNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

									if (pNode)
									{
										memset(pNode, 0, sizeof(NXL_CACHE_NODE));

										Status = build_nxlcache_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										pNode->FileAttributes			= FullDirInfo->FileAttributes;
										pNode->Flags					|= FlagOn(FullDirInfo->FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
										pNode->Instance					= FltInstance;
										pNode->ParentDirectoryHash		= DirHash;
										pNode->OnRemoveOrRemovableMedia = FALSE;
										pNode->LastWriteProcessId		= NULL;

										ExInitializeRundownProtection(&pNode->NodeRundownRef);

										Status = build_nxlcache_reparse_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											if (pNode->ReleaseFileName)
											{
												ExFreePoolWithTag(pNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
												RtlInitUnicodeString(&pNode->FileName, NULL);
											}

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &FileName));

										AddNXLNodeToCache(&Global.NxlFileCache, pNode);
									}
								}
								else
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode found file %wZ in cache\n", &FileName));

									pNode->FileAttributes = FullDirInfo->FileAttributes;

									if (FlagOn(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED))
									{
										//
										// This means ejected device come back. Files on this device could have been changed
										// We need to clear the NXRMFLT_FLAG_CTX_ATTACHED flag to force a copy when opening
										// There are two possible outcome as clear the NXRMFLT_FLAG_CTX_ATTACHED flag:
										//		A) File was opened and still opened by some process. Copying should fail
										//		B) File is closed by copying. Content is overwritten. Copying should be OK
										//
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED);
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									}
								}
					
							} while (FALSE);

							FltReleasePushLock(&Global.NxlFileCacheLock);

							PushLockAcquired = FALSE;
						}
						
						do
						{
							PFILE_FULL_DIR_INFORMATION NextFullDirInfo = (PFILE_FULL_DIR_INFORMATION)((UCHAR*)FullDirInfo + FullDirInfo->NextEntryOffset);
							PFILE_FULL_DIR_INFORMATION NextNextFullDirInfo = NULL;
							UNICODE_STRING	FileName = { 0 };
							UNICODE_STRING	TmpName = { 0 };
							UNICODE_STRING	TmpName2 = { 0 };
							BOOLEAN	CtxAttached = FALSE;
							BOOLEAN AttachingCtx = FALSE;
							BOOLEAN NextFileIsTempDecryptedFileOfNextNextFile = FALSE;

							do 
							{
								if(StopLoop)
								{
									break;
								}

								FileName.Buffer = FullPath;
								FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
								FileName.Length = 0;

								TmpName.Buffer = NextFullDirInfo->FileName;
								TmpName.Length = TmpName.MaximumLength = (USHORT)NextFullDirInfo->FileNameLength;

								memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

								RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

								if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
									RtlUnicodeStringCatString(&FileName, L"\\");

								RtlUnicodeStringCat(&FileName, &TmpName);

								FltAcquirePushLockShared(&Global.NxlFileCacheLock);

								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if(pNode)
								{
									CtxAttached = FlagOn(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									AttachingCtx = FlagOn(pNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);

									if(NextFullDirInfo->NextEntryOffset)
									{
										NextNextFullDirInfo = (PFILE_FULL_DIR_INFORMATION)((UCHAR*)NextFullDirInfo + NextFullDirInfo->NextEntryOffset);
										TmpName2.Buffer = NextNextFullDirInfo->FileName;
										TmpName2.Length = TmpName2.MaximumLength = (USHORT)NextNextFullDirInfo->FileNameLength;
										NextFileIsTempDecryptedFileOfNextNextFile = (nxrmfltCompareFinalComponent(&pNode->OriginalFileName, &TmpName2, TRUE) == 0);
									}
									else
									{
										NextFileIsTempDecryptedFileOfNextNextFile = FALSE;
									}
								}

								FltReleasePushLock(&Global.NxlFileCacheLock);

								if (!pNode) // go loop to handle this file.
									break;
								if (!NextFileIsTempDecryptedFileOfNextNextFile)
									break;
								//hide the temporary unencrypted file
								if(!NextFullDirInfo->NextEntryOffset)
								{
									FullDirInfo->NextEntryOffset = 0;
									StopLoop = TRUE;
								}
								else
								{
									FullDirInfo->NextEntryOffset += NextFullDirInfo->NextEntryOffset;
									NextFullDirInfo = (PFILE_FULL_DIR_INFORMATION)((UCHAR*)FullDirInfo + FullDirInfo->NextEntryOffset);
								}

								//
								// in case there is a unencrypted version exist but there is no ctx attached to it 
								//
								if((!CtxAttached) && (!AttachingCtx))
								{
									Status = nxrmfltDeleteFileByName(FltInstance, &FileName);

									PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltFullDirPostDirectoryControl: found %wZ but there is no Ctx attach to it!!! Delete return Status %x\n", &FileName, Status));
								}
							} while (TRUE);

						} while (FALSE);
						
						FullDirInfo = (PFILE_FULL_DIR_INFORMATION)((UCHAR*)FullDirInfo + FullDirInfo->NextEntryOffset);
					}
				}
				__except (nxrmfltExceptionFilter(NULL,GetExceptionInformation()))
				{
					if (PushLockAcquired)
					{
						FltReleasePushLock(&Global.NxlFileCacheLock);
						PushLockAcquired = FALSE;
					}
				}
				
				break;
			}
		case FileDirectoryInformation:
			{
				
				__try
				{
					PFILE_DIRECTORY_INFORMATION DirInfo = NULL;

					DirInfo = (PFILE_DIRECTORY_INFORMATION)Buffer;

					while (!StopLoop)
					{
						NT_ASSERT((ULONG_PTR)(DirInfo) < (ULONG_PTR)((UCHAR*)Buffer + UserBufferLength));

						if (!DirInfo->NextEntryOffset)
						{
							StopLoop = TRUE;
						}

						if (!FlagOn(DirInfo->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) &&
							DirInfo->FileNameLength >= 5 * sizeof(WCHAR) && 
							IsNXLFile(DirInfo->FileName, DirInfo->FileNameLength))
						{
							UNICODE_STRING	FileName;
							UNICODE_STRING	TmpName;
							ULONG DirHash = 0;

							FileName.Buffer = FullPath;
							FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileName.Length = 0;

							if (HideNXLExtension)
							{
								//
								// removing ".nxl" extsion
								//
								DirInfo->FileNameLength -= 4 * sizeof(WCHAR);
							}

							TmpName.Buffer = DirInfo->FileName;
							TmpName.Length = TmpName.MaximumLength = HideNXLExtension ? (USHORT)DirInfo->FileNameLength : (USHORT)(DirInfo->FileNameLength - 4 * sizeof(WCHAR));

							memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

							RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

							if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
								RtlUnicodeStringCatString(&FileName, L"\\");

							RtlHashUnicodeString(&FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

							RtlUnicodeStringCat(&FileName, &TmpName);

							FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

							PushLockAcquired = TRUE;

							do
							{
								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if (!pNode)
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode can't find file %wZ in cache\n", &FileName));

									pNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

									if (pNode)
									{
										memset(pNode, 0, sizeof(NXL_CACHE_NODE));

										Status = build_nxlcache_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										pNode->FileAttributes = DirInfo->FileAttributes;
										pNode->Flags |= FlagOn(DirInfo->FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
										pNode->Instance = FltInstance;
										pNode->ParentDirectoryHash = DirHash;
										pNode->OnRemoveOrRemovableMedia = FALSE;
										pNode->LastWriteProcessId = NULL;

										ExInitializeRundownProtection(&pNode->NodeRundownRef);

										Status = build_nxlcache_reparse_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											if (pNode->ReleaseFileName)
											{
												ExFreePoolWithTag(pNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
												RtlInitUnicodeString(&pNode->FileName, NULL);
											}

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &FileName));

										AddNXLNodeToCache(&Global.NxlFileCache, pNode);
									}
								}
								else
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode found file %wZ in cache\n", &FileName));

									pNode->FileAttributes = DirInfo->FileAttributes;

									if (FlagOn(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED))
									{
										//
										// This means ejected device come back. Files on this device could have been changed
										// We need to clear the NXRMFLT_FLAG_CTX_ATTACHED flag to force a copy when opening
										// There are two possible outcome as clear the NXRMFLT_FLAG_CTX_ATTACHED flag:
										//		A) File was opened and still opened by some process. Copying should fail
										//		B) File is closed by copying. Content is overwritten. Copying should be OK
										//
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED);
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									}

								}

							} while (FALSE);

							FltReleasePushLock(&Global.NxlFileCacheLock);

							PushLockAcquired = FALSE;
						}

						do
						{
							PFILE_DIRECTORY_INFORMATION NextDirInfo = (PFILE_DIRECTORY_INFORMATION)((UCHAR*)DirInfo + DirInfo->NextEntryOffset);
							UNICODE_STRING	FileName = { 0 };
							UNICODE_STRING	TmpName = { 0 };
							BOOLEAN	CtxAttached = FALSE;
							BOOLEAN AttachingCtx = FALSE;

							do 
							{
								if(StopLoop)
								{
									break;
								}

								FileName.Buffer = FullPath;
								FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
								FileName.Length = 0;

								TmpName.Buffer = NextDirInfo->FileName;
								TmpName.Length = TmpName.MaximumLength = (USHORT)NextDirInfo->FileNameLength;

								memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

								RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

								if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
									RtlUnicodeStringCatString(&FileName, L"\\");

								RtlUnicodeStringCat(&FileName, &TmpName);

								FltAcquirePushLockShared(&Global.NxlFileCacheLock);

								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if(pNode)
								{
									CtxAttached = FlagOn(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									AttachingCtx = FlagOn(pNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
								}

								FltReleasePushLock(&Global.NxlFileCacheLock);

								if (!pNode) // go loop to handle this file.
									break;
								//hide the temporary unencrypted file
								if(!NextDirInfo->NextEntryOffset)
								{
									DirInfo->NextEntryOffset = 0;
									StopLoop = TRUE;
								}
								else
								{
									DirInfo->NextEntryOffset += NextDirInfo->NextEntryOffset;
									NextDirInfo = (PFILE_DIRECTORY_INFORMATION)((UCHAR*)DirInfo + DirInfo->NextEntryOffset);
								}

								//
								// in case there is a unencrypted version exist but there is no ctx attached to it 
								//
								if((!CtxAttached) && (!AttachingCtx))
								{
									Status = nxrmfltDeleteFileByName(FltInstance, &FileName);

									PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltDirInfoPostDirectoryControl: found %wZ but there is no Ctx attach to it!!! Delete return Status %x\n", &FileName, Status));
								}
							} while (TRUE);

						} while (FALSE);

						DirInfo = (PFILE_DIRECTORY_INFORMATION)((UCHAR*)DirInfo + DirInfo->NextEntryOffset);
					}
				}
				__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					if (PushLockAcquired)
					{
						FltReleasePushLock(&Global.NxlFileCacheLock);
						PushLockAcquired = FALSE;
					}
				}
				break;
			}

		case FileIdFullDirectoryInformation:

		{
			__try
			{
				PFILE_ID_FULL_DIR_INFORMATION IdFullDirInfo = NULL;

				IdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)Buffer;

				BOOLEAN bNeedScanAgain = FALSE;

				while (!StopLoop)
				{
					NT_ASSERT((ULONG_PTR)(IdFullDirInfo) < (ULONG_PTR)((UCHAR*)Buffer + UserBufferLength));

					if (!IdFullDirInfo->NextEntryOffset)
					{
						StopLoop = TRUE;
					}

					if (!FlagOn(IdFullDirInfo->FileAttributes,FILE_ATTRIBUTE_DIRECTORY) &&
						IdFullDirInfo->FileNameLength >= 5*sizeof(WCHAR) &&
						IsNXLFile(IdFullDirInfo->FileName, IdFullDirInfo->FileNameLength))
					{
						UNICODE_STRING	FileName;
						UNICODE_STRING	TmpName;
						ULONG DirHash = 0;

						FileName.Buffer = FullPath;
						FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
						FileName.Length = 0;

						if (HideNXLExtension)
						{
							//
							// removing ".nxl" extsion
							//
							IdFullDirInfo->FileNameLength -= 4 * sizeof(WCHAR);
						}

						TmpName.Buffer = IdFullDirInfo->FileName;
						TmpName.Length = TmpName.MaximumLength = HideNXLExtension ? (USHORT)IdFullDirInfo->FileNameLength : (USHORT)(IdFullDirInfo->FileNameLength - 4 * sizeof(WCHAR));

						memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

						RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

						if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
							RtlUnicodeStringCatString(&FileName, L"\\");

						RtlHashUnicodeString(&FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

						RtlUnicodeStringCat(&FileName, &TmpName);

						FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

						PushLockAcquired = TRUE;

						do
						{
							pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

							if (!pNode)
							{
								PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode can't find file %wZ in cache\n", &FileName));

								pNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

								if (pNode)
								{
									memset(pNode, 0, sizeof(NXL_CACHE_NODE));

									Status = build_nxlcache_file_name(pNode, &FileName);

									if (!NT_SUCCESS(Status))
									{
										//
										// ERROR case. No memory
										//
										PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

										ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
										pNode = NULL;
										break;
									}

									pNode->FileAttributes			= IdFullDirInfo->FileAttributes;
									pNode->Flags					|= FlagOn(IdFullDirInfo->FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
									pNode->FileID					= IdFullDirInfo->FileId;
									pNode->Instance					= FltInstance;
									pNode->ParentDirectoryHash		= DirHash;
									pNode->OnRemoveOrRemovableMedia = FALSE;
									pNode->LastWriteProcessId		= NULL;

									ExInitializeRundownProtection(&pNode->NodeRundownRef);

									Status = build_nxlcache_reparse_file_name(pNode, &FileName);

									if (!NT_SUCCESS(Status))
									{
										//
										// ERROR case. No memory
										//
										PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

										if (pNode->ReleaseFileName)
										{
											ExFreePoolWithTag(pNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
											RtlInitUnicodeString(&pNode->FileName, NULL);
										}

										ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
										pNode = NULL;
										break;
									}

									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &FileName));

									AddNXLNodeToCache(&Global.NxlFileCache, pNode);

									bNeedScanAgain = TRUE;
								}
							}
							else
							{
								PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode found file %wZ in cache\n", &FileName));

								pNode->FileAttributes = IdFullDirInfo->FileAttributes;
								pNode->FileID = IdFullDirInfo->FileId;

								if (FlagOn(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED))
								{
									//
									// This means ejected device come back. Files on this device could have been changed
									// We need to clear the NXRMFLT_FLAG_CTX_ATTACHED flag to force a copy when opening
									// There are two possible outcome as clear the NXRMFLT_FLAG_CTX_ATTACHED flag:
									//		A) File was opened and still opened by some process. Copying should fail
									//		B) File is closed by copying. Content is overwritten. Copying should be OK
									//
									ClearFlag(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED);
									ClearFlag(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
								}

							}

						} while (FALSE);

						FltReleasePushLock(&Global.NxlFileCacheLock);

						PushLockAcquired = FALSE;
					}

					do
					{
						PFILE_ID_FULL_DIR_INFORMATION NextIdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((UCHAR*)IdFullDirInfo + IdFullDirInfo->NextEntryOffset);
						UNICODE_STRING	FileName = { 0 };
						UNICODE_STRING	TmpName = { 0 };
						BOOLEAN	CtxAttached = FALSE;
						BOOLEAN AttachingCtx = FALSE;
						
						do 
						{
							if(StopLoop)
							{
								break;
							}

							FileName.Buffer = FullPath;
							FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileName.Length = 0;

							TmpName.Buffer = NextIdFullDirInfo->FileName;
							TmpName.Length = TmpName.MaximumLength = (USHORT)NextIdFullDirInfo->FileNameLength;

							memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

							RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

							if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
								RtlUnicodeStringCatString(&FileName, L"\\");

							RtlUnicodeStringCat(&FileName, &TmpName);

							FltAcquirePushLockShared(&Global.NxlFileCacheLock);

							pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

							if(pNode)
							{
								CtxAttached = FlagOn(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
								AttachingCtx = FlagOn(pNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
							}

							FltReleasePushLock(&Global.NxlFileCacheLock);

							if (!pNode) // go loop to handle this file.
								break;
							//hide the temporary unencrypted file
							if(!NextIdFullDirInfo->NextEntryOffset)
							{
								IdFullDirInfo->NextEntryOffset = 0;
								StopLoop = TRUE;
							}
							else
							{
								IdFullDirInfo->NextEntryOffset += NextIdFullDirInfo->NextEntryOffset;
								NextIdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((UCHAR*)IdFullDirInfo + IdFullDirInfo->NextEntryOffset);
							}

							//
							// in case there is a unencrypted version exist but there is no ctx attached to it 
							//
							if((!CtxAttached) && (!AttachingCtx))
							{
								Status = nxrmfltDeleteFileByName(FltInstance, &FileName);

								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltIdFullDirPostDirectoryControl: found %wZ but there is no Ctx attach to it!!! Delete return Status %x\n", &FileName, Status));
							}
						} while (TRUE);

					} while (FALSE);

					IdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((UCHAR*)IdFullDirInfo + IdFullDirInfo->NextEntryOffset);
				}

				if (bNeedScanAgain)
				{
					IdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)Buffer;
					do
					{
						if (!IdFullDirInfo->NextEntryOffset)
						{
							break;
						}

						PFILE_ID_FULL_DIR_INFORMATION NextIdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((UCHAR*)IdFullDirInfo + IdFullDirInfo->NextEntryOffset);
						UNICODE_STRING	FileName = { 0 };
						UNICODE_STRING	TmpName = { 0 };

						FileName.Buffer = FullPath;
						FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
						FileName.Length = 0;

						TmpName.Buffer = NextIdFullDirInfo->FileName;
						TmpName.Length = TmpName.MaximumLength = (USHORT)NextIdFullDirInfo->FileNameLength;

						memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

						RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

						if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
							RtlUnicodeStringCatString(&FileName, L"\\");

						RtlUnicodeStringCat(&FileName, &TmpName);

						FltAcquirePushLockShared(&Global.NxlFileCacheLock);
						pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);
						FltReleasePushLock(&Global.NxlFileCacheLock);

						if (pNode && NextIdFullDirInfo->FileId.QuadPart != pNode->FileID.QuadPart)
						{
							//hide the temporary unencrypted file
							if (!NextIdFullDirInfo->NextEntryOffset)
							{
								IdFullDirInfo->NextEntryOffset = 0;
								break;
							}
							else
							{
								IdFullDirInfo->NextEntryOffset += NextIdFullDirInfo->NextEntryOffset;
							}
						}
						IdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((UCHAR*)IdFullDirInfo + IdFullDirInfo->NextEntryOffset);
					} while (TRUE);
				}
			}
			__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
			{
				if (PushLockAcquired)
				{
					FltReleasePushLock(&Global.NxlFileCacheLock);
					PushLockAcquired = FALSE;
				}
			}
			break;

		}

		case FileNamesInformation:
			{
				__try
				{

					PFILE_NAMES_INFORMATION FileNamesInfo = NULL;

					FileNamesInfo = (PFILE_NAMES_INFORMATION)Buffer;

					while (!StopLoop)
					{
						NT_ASSERT((ULONG_PTR)(FileNamesInfo) < (ULONG_PTR)((UCHAR*)Buffer + UserBufferLength));

						if (!FileNamesInfo->NextEntryOffset)
						{
							StopLoop = TRUE;
						}

						if (FileNamesInfo->FileNameLength >= 5 * sizeof(WCHAR) &&
							IsNXLFile(FileNamesInfo->FileName, FileNamesInfo->FileNameLength))
						{
							UNICODE_STRING	FileName;
							UNICODE_STRING	TmpName;
							ULONG DirHash = 0;
							
							FileName.Buffer = FullPath;
							FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileName.Length = 0;

							if (HideNXLExtension)
							{
								//
								// removing ".nxl" extsion
								//
								FileNamesInfo->FileNameLength -= 4 * sizeof(WCHAR);
							}

							TmpName.Buffer = FileNamesInfo->FileName;
							TmpName.Length = TmpName.MaximumLength = HideNXLExtension ? (USHORT)FileNamesInfo->FileNameLength : (USHORT)(FileNamesInfo->FileNameLength - 4 * sizeof(WCHAR));

							memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

							RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

							if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
								RtlUnicodeStringCatString(&FileName, L"\\");

							RtlHashUnicodeString(&FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

							RtlUnicodeStringCat(&FileName, &TmpName);

							FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

							PushLockAcquired = TRUE;
							
							do
							{
								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if (!pNode)
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode can't find file %wZ in cache\n", &FileName));

									pNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

									if (pNode)
									{
										memset(pNode, 0, sizeof(NXL_CACHE_NODE));

										Status = build_nxlcache_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										pNode->Instance					= FltInstance;
										pNode->ParentDirectoryHash		= DirHash;
										pNode->OnRemoveOrRemovableMedia = FALSE;
										pNode->LastWriteProcessId		= NULL;

										ExInitializeRundownProtection(&pNode->NodeRundownRef);

										Status = build_nxlcache_reparse_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											if (pNode->ReleaseFileName)
											{
												ExFreePoolWithTag(pNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
												RtlInitUnicodeString(&pNode->FileName, NULL);
											}

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &FileName));

										AddNXLNodeToCache(&Global.NxlFileCache, pNode);
									}
								}
								else
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode found file %wZ in cache\n", &FileName));

									if (FlagOn(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED))
									{
										//
										// This means ejected device come back. Files on this device could have been changed
										// We need to clear the NXRMFLT_FLAG_CTX_ATTACHED flag to force a copy when opening
										// There are two possible outcome as clear the NXRMFLT_FLAG_CTX_ATTACHED flag:
										//		A) File was opened and still opened by some process. Copying should fail
										//		B) File is closed by copying. Content is overwritten. Copying should be OK
										//
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED);
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									}

								}

							} while (FALSE);

							FltReleasePushLock(&Global.NxlFileCacheLock);

							PushLockAcquired = FALSE;
						}
						
						do
						{
							PFILE_NAMES_INFORMATION NextFileNamesInfo = (PFILE_NAMES_INFORMATION)((UCHAR*)FileNamesInfo + FileNamesInfo->NextEntryOffset);
							UNICODE_STRING	FileName = { 0 };
							UNICODE_STRING	TmpName = { 0 };
							BOOLEAN CtxAttached = FALSE;
							BOOLEAN AttachingCtx = FALSE;

							do 
							{
								if(StopLoop)
								{
									break;
								}

								FileName.Buffer = FullPath;
								FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
								FileName.Length = 0;

								TmpName.Buffer = NextFileNamesInfo->FileName;
								TmpName.Length = TmpName.MaximumLength = (USHORT)NextFileNamesInfo->FileNameLength;

								memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

								RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

								if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
									RtlUnicodeStringCatString(&FileName, L"\\");

								RtlUnicodeStringCat(&FileName, &TmpName);

								FltAcquirePushLockShared(&Global.NxlFileCacheLock);

								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if(pNode)
								{
									CtxAttached = FlagOn(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									AttachingCtx = FlagOn(pNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
								}
								
								FltReleasePushLock(&Global.NxlFileCacheLock);

								if (!pNode) // go loop to handle this file.
									break;
								//hide the temporary unencrypted file
								if(!NextFileNamesInfo->NextEntryOffset)
								{
									FileNamesInfo->NextEntryOffset = 0;
									StopLoop = TRUE;
								}
								else
								{
									FileNamesInfo->NextEntryOffset += NextFileNamesInfo->NextEntryOffset;
									NextFileNamesInfo = (PFILE_NAMES_INFORMATION)((UCHAR*)FileNamesInfo + FileNamesInfo->NextEntryOffset);
								}

								//
								// in case there is a unencrypted version exist but there is no ctx attached to it 
								//
								if((!CtxAttached) && (!AttachingCtx))
								{
									Status = nxrmfltDeleteFileByName(FltInstance, &FileName);

									PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltFileNamePostDirectoryControl: found %wZ but there is no Ctx attach to it!!! Delete return Status %x\n", &FileName, Status));
								}
							} while (TRUE);

						} while (FALSE);

						FileNamesInfo = (PFILE_NAMES_INFORMATION)((UCHAR*)FileNamesInfo + FileNamesInfo->NextEntryOffset);
					}
				}
				__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					if (PushLockAcquired)
					{
						FltReleasePushLock(&Global.NxlFileCacheLock);
						PushLockAcquired = FALSE;
					}
				}
				break;
			}

		case FileBothDirectoryInformation:
			{
				__try
				{
					PFILE_BOTH_DIR_INFORMATION BothDirInfo = NULL;

					BothDirInfo = (PFILE_BOTH_DIR_INFORMATION)Buffer;

					while (!StopLoop)
					{
						NT_ASSERT((ULONG_PTR)(BothDirInfo) < (ULONG_PTR)((UCHAR*)Buffer + UserBufferLength));

						if (!BothDirInfo->NextEntryOffset)
						{
							StopLoop = TRUE;
						}

						if (!FlagOn(BothDirInfo->FileAttributes,FILE_ATTRIBUTE_DIRECTORY) &&
							BothDirInfo->FileNameLength >= 5 * sizeof(WCHAR) &&
							IsNXLFile(BothDirInfo->FileName, BothDirInfo->FileNameLength))
						{
							UNICODE_STRING	FileName;
							UNICODE_STRING	TmpName;
							ULONG DirHash = 0;

							FileName.Buffer = FullPath;
							FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileName.Length = 0;
							
							if (HideNXLExtension)
							{
								//
								// removing ".nxl" extsion
								//
								BothDirInfo->FileNameLength -= 4 * sizeof(WCHAR);
							}

							TmpName.Buffer = BothDirInfo->FileName;
							TmpName.Length = TmpName.MaximumLength = HideNXLExtension ? (USHORT)BothDirInfo->FileNameLength : (USHORT)(BothDirInfo->FileNameLength - 4 * sizeof(WCHAR));

							memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

							RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

							if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
								RtlUnicodeStringCatString(&FileName, L"\\");

							RtlHashUnicodeString(&FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

							RtlUnicodeStringCat(&FileName, &TmpName);

							FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

							PushLockAcquired = TRUE;

							do
							{
								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if (!pNode)
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode can't find file %wZ in cache\n", &FileName));

									pNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

									if (pNode)
									{
										memset(pNode, 0, sizeof(NXL_CACHE_NODE));

										Status = build_nxlcache_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										pNode->FileAttributes = BothDirInfo->FileAttributes;
										pNode->Flags |= FlagOn(BothDirInfo->FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
										pNode->Instance = FltInstance;
										pNode->ParentDirectoryHash = DirHash;
										pNode->OnRemoveOrRemovableMedia = FALSE;
										pNode->LastWriteProcessId = NULL;

										ExInitializeRundownProtection(&pNode->NodeRundownRef);

										Status = build_nxlcache_reparse_file_name(pNode, &FileName);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &FileName));

											if (pNode->ReleaseFileName)
											{
												ExFreePoolWithTag(pNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
												RtlInitUnicodeString(&pNode->FileName, NULL);
											}

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNode);
											pNode = NULL;
											break;
										}

										PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &FileName));

										AddNXLNodeToCache(&Global.NxlFileCache, pNode);
									}
								}
								else
								{
									PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!FindNXLNode found file %wZ in cache\n", &FileName));

									pNode->FileAttributes = BothDirInfo->FileAttributes;

									if (FlagOn(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED))
									{
										//
										// This means ejected device come back. Files on this device could have been changed
										// We need to clear the NXRMFLT_FLAG_CTX_ATTACHED flag to force a copy when opening
										// There are two possible outcome as clear the NXRMFLT_FLAG_CTX_ATTACHED flag:
										//		A) File was opened and still opened by some process. Copying should fail
										//		B) File is closed by copying. Content is overwritten. Copying should be OK
										//
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED);
										ClearFlag(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									}

								}

							} while (FALSE);

							FltReleasePushLock(&Global.NxlFileCacheLock);

							PushLockAcquired = FALSE;
						}
						
						do
						{
							PFILE_BOTH_DIR_INFORMATION NextBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)((UCHAR*)BothDirInfo + BothDirInfo->NextEntryOffset);
							PFILE_BOTH_DIR_INFORMATION NextNextBothDirInfo = NULL;
							UNICODE_STRING	FileName = { 0 };
							UNICODE_STRING	TmpName = { 0 };
							UNICODE_STRING	TmpName2 = { 0 };
							BOOLEAN CtxAttached = FALSE;
							BOOLEAN AttachingCtx = FALSE;
							BOOLEAN NextFileIsTempDecryptedFileOfNextNextFile = FALSE;

							do 
							{
								if(StopLoop)
								{
									break;
								}

								FileName.Buffer = FullPath;
								FileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
								FileName.Length = 0;

								TmpName.Buffer = NextBothDirInfo->FileName;
								TmpName.Length = TmpName.MaximumLength = (USHORT)NextBothDirInfo->FileNameLength;

								memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

								RtlUnicodeStringCat(&FileName, &pNameInfo->Name);

								if (pNameInfo->Name.Length != 0 && pNameInfo->Name.Buffer[pNameInfo->Name.Length / sizeof(WCHAR) - 1] != L'\\')
									RtlUnicodeStringCatString(&FileName, L"\\");

								RtlUnicodeStringCat(&FileName, &TmpName);

								FltAcquirePushLockShared(&Global.NxlFileCacheLock);

								pNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileName);

								if(pNode)
								{
									CtxAttached = FlagOn(pNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									AttachingCtx = FlagOn(pNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);

									if(NextBothDirInfo->NextEntryOffset)
									{
										NextNextBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)((UCHAR*)NextBothDirInfo + NextBothDirInfo->NextEntryOffset);
										TmpName2.Buffer = NextNextBothDirInfo->FileName;
										TmpName2.Length = TmpName2.MaximumLength = (USHORT)NextNextBothDirInfo->FileNameLength;
										NextFileIsTempDecryptedFileOfNextNextFile = (nxrmfltCompareFinalComponent(&pNode->OriginalFileName, &TmpName2, TRUE) == 0);
									}
									else
									{
										NextFileIsTempDecryptedFileOfNextNextFile = FALSE;
									}
								}

								FltReleasePushLock(&Global.NxlFileCacheLock);

								if (!pNode) // go loop to handle this file.
									break;
								if (!NextFileIsTempDecryptedFileOfNextNextFile)
									break;
								//hide the temporary unencrypted file
								if(!NextBothDirInfo->NextEntryOffset)
								{
									BothDirInfo->NextEntryOffset = 0;
									StopLoop = TRUE;
								}
								else
								{
									BothDirInfo->NextEntryOffset += NextBothDirInfo->NextEntryOffset;
									NextBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)((UCHAR*)BothDirInfo + BothDirInfo->NextEntryOffset);
								}

								//
								// in case there is a unencrypted version exist but there is no ctx attached to it 
								//
								if((!CtxAttached) && (!AttachingCtx))
								{
									Status = nxrmfltDeleteFileByName(FltInstance, &FileName);

									PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltBothDirPostDirectoryControl: found %wZ but there is no Ctx attach to it!!! Delete return Status %x\n", &FileName, Status));
								}
							} while (TRUE);

						} while (FALSE);

						BothDirInfo = (PFILE_BOTH_DIR_INFORMATION)((UCHAR*)BothDirInfo + BothDirInfo->NextEntryOffset);
					}
				}
				__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					if (PushLockAcquired)
					{
						FltReleasePushLock(&Global.NxlFileCacheLock);
						PushLockAcquired = FALSE;
					}
				}
				break;
			}

		case FileObjectIdInformation:

			break;
		case FileReparsePointInformation:
			
			break;

		default:
			break;
		}
		
	} while (FALSE);

	if (DirCtlCtx)
	{
		if (DirCtlCtx->FileName.Length)
		{
			ExFreeToPagedLookasideList(&Global.FullPathLookaside, (PVOID)DirCtlCtx->FileName.Buffer);
		}

		FltReleaseFileNameInformation(DirCtlCtx->NameInfo);

		ExFreeToPagedLookasideList(&Global.DirCtlCtxLookaside, DirCtlCtx);
	}

	if (FullPath)
	{
		ExFreeToPagedLookasideList(&Global.FullPathLookaside, (PVOID)FullPath);
	}

	if (InstCtx)
	{
		FltReleaseContext(InstCtx);
	}

    if (strMatch(&Data->Iopb->TargetFileObject->FileName))///ayuen
    {
        PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPostDirectoryControl [%ld] line %d PID %p: ayuen leave, current thread PID:%p TID:%p, dir %wZ, returning %d\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), PsGetCurrentProcessId(), PsGetCurrentThreadId(), &Data->Iopb->TargetFileObject->FileName, (int) CallbackStatus));
    
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPostDirectoryControl [%ld] line %d PID %p: ayuen leave, dir %wZ, returning %d\n", InterlockedIncrement(&logIndex), __LINE__, getProcessIDFromData(Data), &Data->Iopb->TargetFileObject->FileName, (int)CallbackStatus);
            nxWriteToLogFile(szBuffer);
        }
    }
	return CallbackStatus;
}

static BOOLEAN IsNXLFile(WCHAR *FileName, ULONG LengthInByte)
{
	BOOLEAN bRet = FALSE;
	
	WCHAR *p = NULL;

	UNICODE_STRING	Ext = { 0 };
	UNICODE_STRING	NXLExt = { 0 };

	do
	{
		p = FileName + LengthInByte/sizeof(WCHAR) + 1 - sizeof(NXRMFLT_NXL_DOTEXT) / sizeof(WCHAR);

		Ext.Buffer	= p;
		Ext.Length = Ext.MaximumLength = sizeof(NXRMFLT_NXL_DOTEXT) - sizeof(WCHAR);
	
		bRet = (RtlCompareUnicodeString(&Global.NXLFileDotExtsion, &Ext, TRUE) == 0);

	} while (FALSE);

	return bRet;
}

static NTSTATUS build_nxlcache_file_name_without_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	USHORT Length = FileName->Length + sizeof(NXRMFLT_NXL_DOTEXT) - sizeof(WCHAR);

	do 
	{
		if (Length > sizeof(pNode->FileNameFastBuffer))
		{
			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = ExAllocatePoolWithTag(PagedPool, Length, NXRMFLT_NXLCACHE_TAG);

			if (pNode->FileName.Buffer)
			{
				pNode->ReleaseFileName = TRUE;

				pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = Length;
				pNode->FileName.Length = FileName->Length;
				pNode->OriginalFileName.Length = Length;

				memcpy(pNode->FileName.Buffer, FileName->Buffer, FileName->Length);

				memcpy(pNode->FileName.Buffer + FileName->Length / sizeof(WCHAR), NXRMFLT_NXL_DOTEXT, (sizeof(NXRMFLT_NXL_DOTEXT)-sizeof(WCHAR)));
			}
			else
			{
				//
				// ERROR case. No memory
				//

				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
		}
		else
		{
			//
			// use fast buffer
			//

			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = pNode->FileNameFastBuffer;
			pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = sizeof(pNode->FileNameFastBuffer);
			pNode->FileName.Length = FileName->Length;
			pNode->OriginalFileName.Length = Length;

			memcpy(pNode->FileName.Buffer, FileName->Buffer, FileName->Length);

			memcpy(pNode->FileName.Buffer + FileName->Length / sizeof(WCHAR), NXRMFLT_NXL_DOTEXT, (sizeof(NXRMFLT_NXL_DOTEXT)-sizeof(WCHAR)));
		}

		RtlHashUnicodeString(&pNode->FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &pNode->FileNameHash);

	} while (FALSE);

	return Status;
}

//static NTSTATUS build_nxlcache_reparse_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName)
//{
//	NTSTATUS Status = STATUS_SUCCESS;
//
//	ULONG TotalReparsePathLength = 0;
//
//	ULONG i = 0;
//
//	WCHAR *p = NULL;
//
//	do 
//	{
//		TotalReparsePathLength = (sizeof(NXRMFLT_DEBUG_TEMP_REPARSE_PATH) / sizeof(WCHAR)) - 1;
//
//		TotalReparsePathLength += FileName->Length;
//
//		if (TotalReparsePathLength > sizeof(pNode->ReparseFileNameFastBuffer))
//		{
//			pNode->ReparseFileName.Buffer = ExAllocatePoolWithTag(PagedPool, TotalReparsePathLength, NXRMFLT_NXLCACHE_TAG);
//
//			if (pNode->ReparseFileName.Buffer)
//			{
//				pNode->ReleaseReparseName = TRUE;
//
//				pNode->ReparseFileName.MaximumLength = (USHORT)TotalReparsePathLength;
//
//			}
//			else
//			{
//				//
//				// ERROR case. No memory
//				//
//
//				Status = STATUS_INSUFFICIENT_RESOURCES;
//				break;
//			}
//		}
//		else
//		{
//			//
//			// use fast buffer
//			//
//
//			pNode->ReparseFileName.Buffer			= pNode->ReparseFileNameFastBuffer;
//			pNode->ReparseFileName.MaximumLength	= sizeof(pNode->ReparseFileNameFastBuffer);
//		}
//
//		pNode->ReparseFileName.Length = 0;
//
//		RtlUnicodeStringCatString(&pNode->ReparseFileName, NXRMFLT_DEBUG_TEMP_REPARSE_PATH);
//
//		RtlUnicodeStringCat(&pNode->ReparseFileName, FileName);
//
//		for (i = sizeof(NXRMFLT_DEBUG_TEMP_REPARSE_PATH) / sizeof(WCHAR); i <= TotalReparsePathLength; i++)
//		{
//			p = pNode->ReparseFileName.Buffer + i;
//
//			if (*p == L'\\')
//			{
//				*p = L'_';
//			}
//
//			if (*p >= L'A' && *p <= L'Z')
//			{
//				*p += L'a' - L'A';
//			}
//		}
//
//	} while (FALSE);
//
//	return Status;
//}

NTSTATUS build_nxlcache_reparse_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	ULONG TotalReparsePathLength = 0;

	ULONG i = 0;

	WCHAR *p = NULL;

	do 
	{
		TotalReparsePathLength = FileName->Length;

		if (TotalReparsePathLength > sizeof(pNode->ReparseFileNameFastBuffer))
		{
			pNode->ReparseFileName.Buffer = ExAllocatePoolWithTag(PagedPool, TotalReparsePathLength, NXRMFLT_NXLCACHE_TAG);

			if (pNode->ReparseFileName.Buffer)
			{
				pNode->ReleaseReparseName = TRUE;

				pNode->ReparseFileName.MaximumLength = (USHORT)TotalReparsePathLength;

			}
			else
			{
				//
				// ERROR case. No memory
				//

				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
		}
		else
		{
			//
			// use fast buffer
			//

			pNode->ReparseFileName.Buffer			= pNode->ReparseFileNameFastBuffer;
			pNode->ReparseFileName.MaximumLength	= sizeof(pNode->ReparseFileNameFastBuffer);
		}

		pNode->ReparseFileName.Length = 0;

		RtlUnicodeStringCat(&pNode->ReparseFileName, FileName);

	} while (FALSE);

	return Status;
}

static NTSTATUS build_nxlcache_file_name_with_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	USHORT Length = FileName->Length;

	do
	{
		if (Length > sizeof(pNode->FileNameFastBuffer))
		{
			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = ExAllocatePoolWithTag(PagedPool, Length, NXRMFLT_NXLCACHE_TAG);

			if (pNode->FileName.Buffer)
			{
				pNode->ReleaseFileName = TRUE;

				pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = Length;
				pNode->FileName.Length = pNode->OriginalFileName.Length = Length;

				memcpy(pNode->FileName.Buffer, FileName->Buffer, Length);
			}
			else
			{
				//
				// ERROR case. No memory
				//

				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
		}
		else
		{
			//
			// use fast buffer
			//

			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = pNode->FileNameFastBuffer;
			pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = sizeof(pNode->FileNameFastBuffer);
			pNode->FileName.Length = pNode->OriginalFileName.Length = Length;

			memcpy(pNode->FileName.Buffer, FileName->Buffer, Length);
		}

		RtlHashUnicodeString(&pNode->FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &pNode->FileNameHash);

	} while (FALSE);

	return Status;

}

NTSTATUS build_nxlcache_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName)
{
	//
	// FileName does NOT include ".nxl" extension when Global.HideNXLExtension is TRUE
	//
	if (Global.HideNXLExtension)
	{
		return build_nxlcache_file_name_without_nxl_extension(pNode, FileName);
	}
	else
	{
		return build_nxlcache_file_name_with_nxl_extension(pNode, FileName);
	}
}