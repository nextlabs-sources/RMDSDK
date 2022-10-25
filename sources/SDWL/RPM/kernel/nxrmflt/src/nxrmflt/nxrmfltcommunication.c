#include "nxrmfltdef.h"
#include "nxrmflt.h"
#include "nxrmfltcommunication.h"
#include "nxrmfltutils.h"

extern NXRMFLT_GLOBAL_DATA	Global;

NTSTATUS nxrmfltPrepareServerPort(VOID)
{
	NTSTATUS Status = STATUS_SUCCESS;

	OBJECT_ATTRIBUTES	ObjectAttributes = { 0 };
	UNICODE_STRING		nxrmfltPortName = { 0 };
	LONG				maxConnections = 1;
	SECURITY_DESCRIPTOR	*pSD = NULL;

	RtlInitUnicodeString(&nxrmfltPortName, NXRMFLT_MSG_PORT_NAME);

	do 
	{
		Status = FltBuildDefaultSecurityDescriptor(&pSD, FLT_PORT_ALL_ACCESS);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		InitializeObjectAttributes(&ObjectAttributes,
								   &nxrmfltPortName,
								   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
								   NULL,
								   pSD);

		Status = FltCreateCommunicationPort(Global.Filter,
											&Global.ServerPort,
											&ObjectAttributes,
											NULL,
											nxrmfltConnectNotifyCallback,
											nxrmfltDisconnectNotifyCallback,
											nxrmfltMessageNotifyCallback,
											maxConnections);

		FltFreeSecurityDescriptor(pSD);
		
	} while (FALSE);

	return Status;
}

NTSTATUS
	nxrmfltConnectNotifyCallback (
	_In_ PFLT_PORT ClientPort,
	_In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID *ConnectionCookie
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	NXRMFLT_CONNECTION_CONTEXT *ConnCtx = (NXRMFLT_CONNECTION_CONTEXT*)ConnectionContext;

	do 
	{
		if(!ConnCtx || SizeOfContext != sizeof(NXRMFLT_CONNECTION_CONTEXT))
		{
			Status = STATUS_INVALID_PARAMETER_3;
			break;
		}

        Global.HideNXLExtension = ConnCtx->HideNxlExtension;
		Global.PortProcessId	= PsGetCurrentProcessId();
		Global.ClientPort		= ClientPort;
		
	} while (FALSE);

	return Status;
}

VOID
	nxrmfltDisconnectNotifyCallback(
	_In_opt_ PVOID ConnectionCookie
	)
{
	LIST_ENTRY *ite = NULL;
	LIST_ENTRY *tmp = NULL;

	LIST_ENTRY SessionOnStack = { 0 };

	InitializeListHead(&SessionOnStack);

	FltCloseClientPort(Global.Filter, &Global.ClientPort);

	//
	// Cleanup session cache
	//

	//
	// step 1: Queue each session cache node to an on stack list (avoid waiting for rundown while hold a lock)
	//
	FltAcquirePushLockExclusive(&Global.SessionCacheLock);

	FOR_EACH_LIST_SAFE(ite, tmp, &Global.SessionCache)
	{
		NXL_SESSION_CACHE_NODE *pNode = CONTAINING_RECORD(ite, NXL_SESSION_CACHE_NODE, Link);

		RemoveEntryList(ite);

		InsertTailList(&SessionOnStack, &pNode->Link);
	}

	FltReleasePushLock(&Global.SessionCacheLock);

	//
	// step 2: Wait for rundown complete and free each session cache node
	//
	ite = NULL;
	tmp = NULL;

	FOR_EACH_LIST_SAFE(ite, tmp, &SessionOnStack)
	{
		NXL_SESSION_CACHE_NODE *pNode = CONTAINING_RECORD(ite, NXL_SESSION_CACHE_NODE, Link);

		RemoveEntryList(ite);

		//
		// Wait for all other threads rundown
		//
		ExWaitForRundownProtectionRelease(&pNode->NodeRundownRef);

		ExRundownCompleted(&pNode->NodeRundownRef);

		nxrmfltFreeSessionCacheNode(pNode);
	}

	Global.ClientPort			= NULL;
	Global.PortProcessId		= 0;
	Global.HideNXLExtension		= TRUE;

	return;
}

NTSTATUS
	nxrmfltMessageNotifyCallback (
	_In_ PVOID ConnectionCookie,
	_In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize,
	_Out_writes_bytes_to_opt_(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize,
	_Out_ PULONG ReturnOutputBufferLength
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	NXRMFLT_COMMAND		Command = nxrmfltInvalidCommand;

	NXRMFLT_COMMAND_MSG *Msg = (NXRMFLT_COMMAND_MSG *)InputBuffer;

	NXRMFLT_SAVEAS_FORECAST *pSaveAsForecast = NULL;
	
	NXL_SAVEAS_NODE *pSaveAsNode = NULL;

	NXL_SESSION_CACHE_NODE *pSessionCacheNode = NULL;

	NXRMFLT_LOGON_SESSION_CREATED *pLogonSession = NULL;

	PVOID CommandData = NULL;

	ULONG CommandDataLength = 0;

	LIST_ENTRY *ite = NULL;

	*ReturnOutputBufferLength = 0;

	do 
	{
		if ((InputBuffer == NULL) ||
			(InputBufferSize < (FIELD_OFFSET(NXRMFLT_COMMAND_MSG, Command) +
								sizeof(NXRMFLT_COMMAND)))) 
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		__try
		{
			Command = ((PNXRMFLT_COMMAND_MSG) InputBuffer)->Command;

			CommandData = (PVOID)(((PNXRMFLT_COMMAND_MSG) InputBuffer)->Data);

			CommandDataLength = ((PNXRMFLT_COMMAND_MSG) InputBuffer)->Size;
		}
		__except(nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
		{
			Status = GetExceptionCode();
			break;
		}
		
		switch (Command)
		{
		case nxrmfltSaveAsForecast:

			do 
			{
				if (CommandDataLength != sizeof(NXRMFLT_SAVEAS_FORECAST))
				{
					Status = STATUS_INVALID_PARAMETER;
					break;
				}

				pSaveAsNode = ExAllocateFromPagedLookasideList(&Global.SaveAsExpireLookaside);

				if (!pSaveAsNode)
				{
					Status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}

				memset(pSaveAsNode, 0, sizeof(NXL_SAVEAS_NODE));

				__try
				{
					pSaveAsForecast = (NXRMFLT_SAVEAS_FORECAST *)CommandData;

					memcpy(pSaveAsNode->SaveAsFileNameBuf,
						   pSaveAsForecast->SaveAsFileName,
						   min(sizeof(pSaveAsNode->SaveAsFileNameBuf) - sizeof(WCHAR), sizeof(pSaveAsForecast->SaveAsFileName) - sizeof(WCHAR)));

					memcpy(pSaveAsNode->SourceFileNameBuf,
						   pSaveAsForecast->SourceFileName,
						   min(sizeof(pSaveAsNode->SourceFileNameBuf) - sizeof(WCHAR), sizeof(pSaveAsForecast->SourceFileName) - sizeof(WCHAR)));

					pSaveAsNode->ProcessId = (HANDLE)pSaveAsForecast->ProcessId;
				}
				__except(nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					Status = GetExceptionCode();
					break;
				}

				ExInitializeRundownProtection(&pSaveAsNode->NodeRundownRef);
				
				RtlInitUnicodeString(&pSaveAsNode->SaveAsFileName, pSaveAsNode->SaveAsFileNameBuf);

				pSaveAsNode->SourceFileName.Buffer			= pSaveAsNode->SourceFileNameBuf;
				pSaveAsNode->SourceFileName.Length			= wcslen(pSaveAsNode->SourceFileNameBuf) * sizeof(WCHAR);
				pSaveAsNode->SourceFileName.MaximumLength	= sizeof(pSaveAsNode->SourceFileNameBuf);

				FltAcquirePushLockExclusive(&Global.ExpireTableLock);

				InsertHeadList(&Global.ExpireTable, &pSaveAsNode->Link);

				FltReleasePushLock(&Global.ExpireTableLock);

				pSaveAsNode = NULL;

			} while (FALSE);

			break;

		case nxrmfltPolicyChanged:

			if (CommandDataLength != 0)
			{
				Status = STATUS_INVALID_PARAMETER;
				break;
			}

			FltAcquirePushLockExclusive(&Global.NxlProcessListLock);

			FOR_EACH_LIST(ite, &Global.NxlProcessList)
			{
				rb_root RightsCache = {0};
				rb_node *rb_ite = NULL;
				rb_node *rb_tmp = NULL;

				NXL_PROCESS_NODE *pNode = CONTAINING_RECORD(ite, NXL_PROCESS_NODE, Link);

				FltAcquirePushLockExclusive(&pNode->RightsCacheLock);

				RightsCache = pNode->RightsCache;

				pNode->RightsCache.rb_node = NULL;

				FltReleasePushLock(&pNode->RightsCacheLock);

				RB_EACH_NODE_SAFE(rb_ite, rb_tmp, &RightsCache)
				{
					NXL_RIGHTS_CACHE_NODE *pRightsCacheNode = CONTAINING_RECORD(rb_ite, NXL_RIGHTS_CACHE_NODE, Node);

					rb_erase(rb_ite, &RightsCache);

					pRightsCacheNode->FileNameHash		= 0;
					pRightsCacheNode->RightsMask		= MAX_ULONGLONG;

					ExFreeToPagedLookasideList(&Global.NXLRightsCacheLookaside, pRightsCacheNode);
				}
			}

			FltReleasePushLock(&Global.NxlProcessListLock);

			break;

		case nxrmfltSessionCreated:

			do
			{
				NXL_SESSION_CACHE_NODE *pTmpSessionCacheNode = NULL;

				if (CommandDataLength != sizeof(NXRMFLT_LOGON_SESSION_CREATED))
				{
					Status = STATUS_INVALID_PARAMETER;
					break;
				}

				pSessionCacheNode = ExAllocateFromPagedLookasideList(&Global.SessionCacheLookaside);

				if (!pSessionCacheNode)
				{
					Status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}

				memset(pSessionCacheNode, 0, sizeof(NXL_SESSION_CACHE_NODE));

				pSessionCacheNode->Flags = 0;

				pSessionCacheNode->ExpireTick = 0;

				ExInitializeRundownProtection(&pSessionCacheNode->NodeRundownRef);

				FltInitializePushLock(&pSessionCacheNode->TokenCacheLock);

				pSessionCacheNode->TokenCache.rb_node = NULL;

				__try
				{

					pLogonSession = (NXRMFLT_LOGON_SESSION_CREATED *)CommandData;

					memcpy(pSessionCacheNode->DefOwnerId,
						   pLogonSession->Default_OwnerId,
						   min(sizeof(pSessionCacheNode->DefOwnerId), sizeof(pLogonSession->Default_OwnerId)));

					pSessionCacheNode->SessionId = pLogonSession->SessoinId;
				}
				__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					Status = GetExceptionCode();
					break;
				}

				FltAcquirePushLockExclusive(&Global.SessionCacheLock);

				FOR_EACH_LIST(ite, &Global.SessionCache)
				{
					pTmpSessionCacheNode = CONTAINING_RECORD(ite, NXL_SESSION_CACHE_NODE, Link);

					if (pTmpSessionCacheNode->SessionId == pSessionCacheNode->SessionId)
					{
						break;
					}
					else
					{
						pTmpSessionCacheNode = NULL;
					}
				}

				if (!pTmpSessionCacheNode)
				{
					InsertHeadList(&Global.SessionCache, &pSessionCacheNode->Link);
					pSessionCacheNode = NULL;
				}
				else
				{
					Status = STATUS_INVALID_PARAMETER;
				}

				FltReleasePushLock(&Global.SessionCacheLock);

			} while (FALSE);

			break;

		case nxrmfltSessionTerminated:

			do 
			{
				ULONG SessionId = 0;

				NXRMFLT_LOGON_SESSION_TERMINATED *pSessionTerminated = NULL;

				if (CommandDataLength != sizeof(NXRMFLT_LOGON_SESSION_TERMINATED))
				{
					Status = STATUS_INVALID_PARAMETER;
					break;
				}

				__try
				{

					pSessionTerminated = (NXRMFLT_LOGON_SESSION_TERMINATED *)CommandData;

					SessionId = pSessionTerminated->SessionId;
				}
				__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					Status = GetExceptionCode();
					break;
				}

				FltAcquirePushLockExclusive(&Global.SessionCacheLock);

				FOR_EACH_LIST(ite, &Global.SessionCache)
				{
					pSessionCacheNode = CONTAINING_RECORD(ite, NXL_SESSION_CACHE_NODE, Link);

					if (pSessionCacheNode->SessionId == SessionId)
					{
						break;
					}
					else
					{
						pSessionCacheNode = NULL;
					}
				}

				if (pSessionCacheNode)
				{
					RemoveEntryList(&pSessionCacheNode->Link);
				}

				FltReleasePushLock(&Global.SessionCacheLock);

				if (pSessionCacheNode)
				{
					//
					// Wait for all other threads rundown
					//
					ExWaitForRundownProtectionRelease(&pSessionCacheNode->NodeRundownRef);

					ExRundownCompleted(&pSessionCacheNode->NodeRundownRef);

					nxrmfltFreeSessionCacheNode(pSessionCacheNode);

					pSessionCacheNode = NULL;
				}

			} while (FALSE);

			break;

        case nxrmfltManageSafeDir:

            do
            {
                UNICODE_STRING DirPath = { 0 };
                ULONG opCode = 0;
                PNXRMFLT_SAFEDIR_INFO pSafeDirInfo = NULL;

                if (CommandDataLength != sizeof(NXRMFLT_SAFEDIR_INFO))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                __try
                {

                    pSafeDirInfo = (PNXRMFLT_SAFEDIR_INFO)CommandData;
                    opCode = pSafeDirInfo->Cmd;
                    if (0 == pSafeDirInfo->Cmd)
                    {
                        RtlInitUnicodeString(&DirPath, pSafeDirInfo->SafeDirPath);
                        Status = nxrmfltRemoveSafeDir(NULL, &DirPath);
                    }
                    else if (1 == pSafeDirInfo->Cmd)
                    {
                        RtlInitUnicodeString(&DirPath, pSafeDirInfo->SafeDirPath);
                        BOOLEAN autoAppendNxlExt, overwrite;
                        autoAppendNxlExt = (DirPath.Buffer[DirPath.Length / sizeof(WCHAR) - 1] == L'1');
                        overwrite = (DirPath.Buffer[DirPath.Length / sizeof(WCHAR) - 2] == L'1');
                        DirPath.Length -= 2 * sizeof(WCHAR);
                        DirPath.MaximumLength -= 2 * sizeof(WCHAR);

                        Status = nxrmfltInsertSafeDir(NULL, &DirPath, overwrite, autoAppendNxlExt);
                    }
                    else if (2 == pSafeDirInfo->Cmd)
                    {
                        nxrmfltRemoveAllSafeDir();
                    }
                    else
                    {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                }
                __except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
                {
                    Status = GetExceptionCode();
                    break;
                }

            } while (FALSE);

            break;

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
        case nxrmfltManageSanctuaryDir:

            do
            {
                UNICODE_STRING DirPath = { 0 };
                ULONG opCode = 0;
                PNXRMFLT_SANCTUARYDIR_INFO pSanctuaryDirInfo = NULL;

                if (CommandDataLength != sizeof(NXRMFLT_SANCTUARYDIR_INFO))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                __try
                {

                    pSanctuaryDirInfo = (PNXRMFLT_SANCTUARYDIR_INFO)CommandData;
                    opCode = pSanctuaryDirInfo->Cmd;
                    if (0 == pSanctuaryDirInfo->Cmd)
                    {
                        RtlInitUnicodeString(&DirPath, pSanctuaryDirInfo->SanctuaryDirPath);
                        Status = nxrmfltRemoveSanctuaryDir(NULL, &DirPath);
                    }
                    else if (1 == pSanctuaryDirInfo->Cmd)
                    {
                        RtlInitUnicodeString(&DirPath, pSanctuaryDirInfo->SanctuaryDirPath);
                        Status = nxrmfltInsertSanctuaryDir(NULL, &DirPath);
                    }
                    else if (2 == pSanctuaryDirInfo->Cmd)
                    {
                        nxrmfltRemoveAllSanctuaryDir();
                    }
                    else
                    {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                }
                __except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
                {
                    Status = GetExceptionCode();
                    break;
                }

            } while (FALSE);

            break;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

		case nxrmfltCleanProcessCache:

			do
			{
				NXL_PROCESS_NODE *pNode = NULL;
				LIST_ENTRY *ite = NULL;

				ULONG ProcessId = 0;

				NXRMFLT_CLEAN_PROCESS_CACHE *pCleanProcessCache = NULL;

				if (CommandDataLength != sizeof(NXRMFLT_CLEAN_PROCESS_CACHE))
				{
					Status = STATUS_INVALID_PARAMETER;
					break;
				}

				__try
				{

					pCleanProcessCache = (NXRMFLT_CLEAN_PROCESS_CACHE *)CommandData;

					ProcessId = pCleanProcessCache->ProcessId;
				}
				__except (nxrmfltExceptionFilter(NULL, GetExceptionInformation()))
				{
					Status = GetExceptionCode();
					break;
				}

				FltAcquirePushLockExclusive(&Global.NxlProcessListLock);

				FOR_EACH_LIST(ite, &Global.NxlProcessList)
				{
					pNode = CONTAINING_RECORD(ite, NXL_PROCESS_NODE, Link);

					if (pNode->ProcessId == (HANDLE)ProcessId)
					{
						RemoveEntryList(&pNode->Link);
						break;
					}
					else
					{
						pNode = NULL;
					}
				}

				FltReleasePushLock(&Global.NxlProcessListLock);

				if (NULL != pNode)
				{
					nxrmfltFreeProcessNode(pNode);
				}
			} while (FALSE);

			break;

		default:
			break;
		}

	} while (FALSE);
	
	if (pSaveAsNode)
	{
		ExFreeToPagedLookasideList(&Global.SaveAsExpireLookaside, pSaveAsNode);
		pSaveAsNode = NULL;
	}

	if (pSessionCacheNode)
	{
		//
		// this means error happens when inserting new session record
		nxrmfltFreeSessionCacheNode(pSessionCacheNode);
		pSessionCacheNode = NULL;
	}

	return Status;
}

NTSTATUS nxrmfltCloseServerPort(VOID)
{
	NTSTATUS Status = STATUS_SUCCESS;

	do 
	{
		Global.PortProcessId = 0;
		
		if (Global.ServerPort)
		{
			FltCloseCommunicationPort(Global.ServerPort);
			Global.ServerPort = NULL;
		}

	} while (FALSE);

	return Status;
}