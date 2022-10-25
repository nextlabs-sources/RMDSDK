#include "Stdafx.h"
#include "nxrmfltman.h"
#include "nxrmfltmandef.h"
#include "fltloghlp.hpp"

using namespace NX::dbg;

#ifndef MAX_ULONGLONG
#define MAX_ULONGLONG                   0xFFFFFFFFFFFFFFFF
#endif


DECLARE_NXRM_MODULE_FLTMAN();
DECLARE_NXRM_MODULE_FLT();

DECLSPEC_CACHEALIGN	LONG		g_nxrmfltmgrcreated = 0;

DWORD WINAPI nxrmfltmanWorker(_In_ LPVOID lpParameter);
BOOL Win32Path2NTPath(const WCHAR *src, WCHAR *ntpath, ULONG *ccntpath);

NXRMFLT_HANDLE __stdcall nxrmfltCreateManager(
	NXRMFLT_CALLBACK_NOTIFY		NotifyCallback, 
	LOGAPI_LOG					DebugDumpCallback,
	LOGAPI_ACCEPT				DebugDumpCheckLevelCallback,
	const WCHAR					*VirtualVolumeNTName,
	PVOID						UserContext)
{
	NXRMFLT_MANAGER	*hMgr = NULL;

	LONG nxrmfltmgrcreated = 0;

	BOOL bFailed = FALSE;

	SYSTEM_INFO SysInfo = { 0 };
	DWORD dwCPUCount = 0;

	DWORD dwThreadCount = 0;

	HRESULT hr = S_OK;

	DWORD i = 0;
	DWORD j = 0;

	NXRMFLT_CONNECTION_CONTEXT *ConnectionCtx = NULL;
	ULONG	ConnectionCtxLen = 0;

	LIST_ENTRY *ite = NULL;
	LIST_ENTRY *tmp = NULL;

	do 
	{
		nxrmfltmgrcreated = InterlockedCompareExchange(&g_nxrmfltmgrcreated, 1, 0);

		if(nxrmfltmgrcreated)
		{
			break;
		}

		hMgr = (NXRMFLT_MANAGER*)malloc(sizeof(NXRMFLT_MANAGER));

		if(!hMgr)
		{
			bFailed = TRUE;
			break;
		}

		memset(hMgr, 0, sizeof(NXRMFLT_MANAGER));

		InitializeListHead(&hMgr->MessageList);

		hMgr->DebugDumpCallback				= DebugDumpCallback;
		hMgr->DebugDumpCheckLevelCallback	= DebugDumpCheckLevelCallback;
		hMgr->NotifyCallback				= NotifyCallback;
		hMgr->UserCtx						= UserContext;
		hMgr->HideNxlExtension				= TRUE;
		hMgr->Stop							= FALSE;
		hMgr->DisableFiltering				= TRUE;
		
		GetNativeSystemInfo(&SysInfo);

		dwCPUCount = max(SysInfo.dwNumberOfProcessors, 1);
		
		dwThreadCount = min(dwCPUCount * 2, NXRMFLTMAN_MAX_PORT_THREADS);

		ConnectionCtxLen = sizeof(NXRMFLT_CONNECTION_CONTEXT);

		if (ConnectionCtxLen > 0xffff)
		{
			bFailed = TRUE;
			break;
		}

		ConnectionCtx = (NXRMFLT_CONNECTION_CONTEXT *)malloc(ConnectionCtxLen);

		if (!ConnectionCtx)
		{
			bFailed = TRUE;
			break;
		}

		ConnectionCtx->HideNxlExtension = TRUE;

		memset(ConnectionCtx->VirtualVolumeNTName, 0, sizeof(ConnectionCtx->VirtualVolumeNTName));

		memcpy(ConnectionCtx->VirtualVolumeNTName, 
			   VirtualVolumeNTName,
			   min(sizeof(ConnectionCtx->VirtualVolumeNTName) - sizeof(WCHAR), wcslen(VirtualVolumeNTName)*sizeof(WCHAR)));

		hr = FilterConnectCommunicationPort(NXRMFLT_MSG_PORT_NAME,
											0,
											ConnectionCtx,
											(WORD)ConnectionCtxLen,
											NULL,
											&hMgr->ConnectionPort);
		if(FAILED(hr))
		{
			bFailed = TRUE;
			break;
		}

		hMgr->Completion = CreateIoCompletionPort(hMgr->ConnectionPort,
												  NULL,
												  0,
												  dwThreadCount);

		if (!hMgr->Completion)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			
			bFailed = TRUE;
			break;
		}

		hMgr->StopEvent = CreateEvent(NULL,
									  TRUE,
									  FALSE,
									  NULL);

		if(!hMgr->StopEvent)
		{
			bFailed = TRUE;
			break;
		}

		for(i = 0; i < dwThreadCount; i++)
		{
			hMgr->WorkerThreadHandle[i] = CreateThread(NULL,
													   0,
													   (LPTHREAD_START_ROUTINE)nxrmfltmanWorker,
													   hMgr,
													   CREATE_SUSPENDED,
													   &hMgr->WorkerThreadId[i]);

			if(!hMgr->WorkerThreadHandle[i])
			{
				bFailed = TRUE;
				break;
			}
		}

		for(i = 0; i < dwThreadCount; i++)
		{
			ResumeThread(hMgr->WorkerThreadHandle[i]);
		}

		for(i = 0; i < dwThreadCount; i++)
		{
			NXRMFLT_MESSAGE *msg = NULL;

			msg = (NXRMFLT_MESSAGE*)malloc(sizeof(NXRMFLT_MESSAGE));

			if(msg)
			{
				memset(msg, 0, sizeof(NXRMFLT_MESSAGE));

				hr = FilterGetMessage(hMgr->ConnectionPort,
									  &msg->Header,
									  FIELD_OFFSET(NXRMFLT_MESSAGE, Ovlp),
									  &msg->Ovlp);

				if(hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
				{
					InsertHeadList(&hMgr->MessageList, &msg->Link);

					hr = S_OK;
				}
				else
				{
					free(msg);

					bFailed = TRUE;
					break;
				}
			}
			else
			{
				bFailed = TRUE;
				break;
			}
		}

	} while (FALSE);

	if(bFailed)
	{
		InterlockedExchange(&g_nxrmfltmgrcreated, 0);

		if(hMgr)
		{
			hMgr->Stop = TRUE;

			{
				HANDLE hThreads[NXRMFLTMAN_MAX_PORT_THREADS] = {0};

				ULONG ulThreads = 0;

				if(hMgr->StopEvent)
					SetEvent(hMgr->StopEvent);

				for (i = 0, j = 0; i < NXRMFLTMAN_MAX_PORT_THREADS; i++)
				{
					if(hMgr->WorkerThreadHandle[i])
					{
						hThreads[j] = hMgr->WorkerThreadHandle[i];
						j++;
						ulThreads++;
					}
				}

				if(hMgr->ConnectionPort)
				{
					CancelIoEx(hMgr->ConnectionPort, NULL);
				}

				WaitForMultipleObjects(ulThreads, hThreads, TRUE, 30*1000);
			}

			for (i = 0; i < NXRMFLTMAN_MAX_PORT_THREADS; i++)
			{
				if(hMgr->WorkerThreadHandle[i])
				{
					CloseHandle(hMgr->WorkerThreadHandle[i]);
					hMgr->WorkerThreadHandle[i] = NULL;
					hMgr->WorkerThreadId[i] = 0;
				}
			}

			if(hMgr->StopEvent)
			{
				CloseHandle(hMgr->StopEvent);
				hMgr->StopEvent = FALSE;
			}

			if(hMgr->ConnectionPort)
			{
				CloseHandle(hMgr->ConnectionPort);
				hMgr->ConnectionPort = NULL;
			}

			if(hMgr->Completion)
			{
				CloseHandle(hMgr->Completion);
				hMgr->Completion = NULL;
			}

			FOR_EACH_LIST_SAFE(ite,tmp,&hMgr->MessageList)
			{
				NXRMFLT_MESSAGE *pNode = CONTAINING_RECORD(ite, NXRMFLT_MESSAGE, Link);

				RemoveEntryList(ite);

				free(pNode);
			}

			free(hMgr);

			hMgr = NULL;
		}
	}
	
	if (ConnectionCtx)
	{
		free(ConnectionCtx);
		ConnectionCtx = NULL;
	}

	return (NXRMFLT_HANDLE)hMgr;
}

HRESULT __stdcall nxrmfltStopFiltering(NXRMFLT_HANDLE hMgr)
{
	NXRMFLT_MANAGER *pMgr = NULL;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	pMgr->DisableFiltering = TRUE;

	return ERROR_SUCCESS;
}

HRESULT __stdcall nxrmfltStartFiltering(NXRMFLT_HANDLE hMgr)
{
	NXRMFLT_MANAGER *pMgr = NULL;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	pMgr->DisableFiltering = FALSE;

	return ERROR_SUCCESS;
}

HRESULT __stdcall nxrmfltCloseManager(NXRMFLT_HANDLE hMgr)
{
	ULONG uRet = ERROR_SUCCESS;

	HANDLE hThreads[NXRMFLTMAN_MAX_PORT_THREADS] = {0};
	ULONG ulThreads = 0;

	ULONG i = 0;
	ULONG j = 0;

	LIST_ENTRY *ite = NULL;
	LIST_ENTRY *tmp = NULL;

	NXRMFLT_MANAGER *pMgr = NULL;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	do 
	{
		if(!hMgr)
		{
			uRet = ERROR_INVALID_PARAMETER;
			break;
		}

		pMgr->Stop = TRUE;

		if(pMgr->StopEvent)
			SetEvent(pMgr->StopEvent);

		for (i = 0, j = 0; i < NXRMFLTMAN_MAX_PORT_THREADS; i++)
		{
			if(pMgr->WorkerThreadHandle[i])
			{
				hThreads[j] = pMgr->WorkerThreadHandle[i];
				j++;
				ulThreads++;
			}
		}

		if(pMgr->ConnectionPort)
		{
			CancelIoEx(pMgr->ConnectionPort, NULL);
		}

		WaitForMultipleObjects(ulThreads, hThreads, TRUE, 30*1000);

		for (i = 0; i < NXRMFLTMAN_MAX_PORT_THREADS; i++)
		{
			if(pMgr->WorkerThreadHandle[i])
			{
				CloseHandle(pMgr->WorkerThreadHandle[i]);
				pMgr->WorkerThreadHandle[i] = NULL;
				pMgr->WorkerThreadId[i] = 0;
			}
		}

		if(pMgr->StopEvent)
		{
			CloseHandle(pMgr->StopEvent);
			pMgr->StopEvent = FALSE;
		}

		if(pMgr->ConnectionPort)
		{
			CloseHandle(pMgr->ConnectionPort);
			pMgr->ConnectionPort = NULL;
		}

		if(pMgr->Completion)
		{
			CloseHandle(pMgr->Completion);
			pMgr->Completion = NULL;
		}

		FOR_EACH_LIST_SAFE(ite,tmp,&pMgr->MessageList)
		{
			NXRMFLT_MESSAGE *pNode = CONTAINING_RECORD(ite, NXRMFLT_MESSAGE, Link);

			RemoveEntryList(ite);

			free(pNode);
		}

		free(hMgr);

		hMgr = NULL;

		InterlockedExchange(&g_nxrmfltmgrcreated, 0);

	} while (FALSE);

	return 0;
}

DWORD WINAPI nxrmfltmanWorker(_In_ LPVOID lpParameter)
{
	HRESULT hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;

	NXRMFLT_MESSAGE *msg = NULL;
	OVERLAPPED *pOvlp = NULL;
	NXRM_CHECK_RIGHTS_REPLY *reply = NULL;
	NXRM_CHECK_RIGHTS_REPLY default_reply = {0};

	NXRM_ACQUIRE_TOKEN_REPLY *acquire_token_reply = NULL;
	NXRM_ACQUIRE_TOKEN_REPLY default_acquire_token_reply = { 0 };

	NXRM_QUERY_TOKEN_REPLY *query_token_reply = NULL;
	NXRM_QUERY_TOKEN_REPLY default_query_token_reply = { 0 };

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	NXRM_CHECK_TRUST_REPLY *check_trust_reply = NULL;
	NXRM_CHECK_TRUST_REPLY default_check_trust_reply = { 0 };
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	DWORD outSize = 0;
	ULONG_PTR key = 0;

	BOOL success = FALSE;
	BOOL DisableFiltering = FALSE;	

	pMgr = (NXRMFLT_MANAGER *)lpParameter;

	while(!pMgr->Stop)
	{
		msg = NULL;
		reply = NULL;

		success = GetQueuedCompletionStatus(pMgr->Completion, &outSize, &key, &pOvlp, INFINITE);

		if(!success)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());

			//
			//  The completion port handle associated with it is closed 
			//  while the call is outstanding, the function returns FALSE, 
			//  *lpOverlapped will be NULL, and GetLastError will return ERROR_ABANDONED_WAIT_0
			//

			if (hr == E_HANDLE) 
			{
				LOGMAN_ERROR(pMgr, "Completion port becomes unavailable (%08X).", hr);
				hr = S_OK;
			}
			else if (hr == HRESULT_FROM_WIN32(ERROR_ABANDONED_WAIT_0)) 
			{
				LOGMAN_ERROR(pMgr, "Completion port was closed (%08X).", hr);
				hr = S_OK;
			}

			continue;
		}

		msg = CONTAINING_RECORD(pOvlp, NXRMFLT_MESSAGE, Ovlp);

		//
		// use in stack variable in case other thread change pMgr->DisableFiltering
		// between two ifs below
		//
		DisableFiltering = pMgr->DisableFiltering;

		if(pMgr->NotifyCallback && (!DisableFiltering))
		{
			switch (msg->Notification.Type)
			{
			case NXRMFLT_MSG_TYPE_CHECK_RIGHTS:

				reply = (NXRM_CHECK_RIGHTS_REPLY *) malloc(sizeof(NXRM_CHECK_RIGHTS_REPLY));

				if(reply)
				{
					reply->ReplyHeader.MessageId		= msg->Header.MessageId;
					reply->ReplyHeader.Status			= 0;
					reply->CheckRigtsReply.RightsMask	= 0;
					reply->CheckRigtsReply.CustomRights	= 0;
					reply->CheckRigtsReply.EvaluationId = MAX_ULONGLONG;

					pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_CHECK_RIGHTS, (PVOID)(&(msg->Notification.CheckRightsMsg)), sizeof(msg->Notification.CheckRightsMsg), (PVOID)reply, pMgr->UserCtx);
				}

				break;
			
			case NXRMFLT_MSG_TYPE_QUERY_TOKEN:

				query_token_reply = (NXRM_QUERY_TOKEN_REPLY *) malloc(sizeof(NXRM_QUERY_TOKEN_REPLY));

				if (query_token_reply)
				{
					query_token_reply->ReplyHeader.MessageId			= msg->Header.MessageId;
					query_token_reply->ReplyHeader.Status				= 0;
					memset(&query_token_reply->QueryTokenReply, 0, sizeof(query_token_reply->QueryTokenReply));

					pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_QUERY_TOKEN, (PVOID)(&(msg->Notification.QueryTokenMsg)), sizeof(msg->Notification.QueryTokenMsg), (PVOID)query_token_reply, pMgr->UserCtx);
				}

				break;

			case NXRMFLT_MSG_TYPE_ACQUIRE_TOKEN:

				acquire_token_reply = (NXRM_ACQUIRE_TOKEN_REPLY *) malloc(sizeof(NXRM_ACQUIRE_TOKEN_REPLY));

				if (acquire_token_reply)
				{
					acquire_token_reply->ReplyHeader.MessageId	= msg->Header.MessageId;
					acquire_token_reply->ReplyHeader.Status		= 0;
					memset(&acquire_token_reply->AcquireTokenReply, 0, sizeof(acquire_token_reply->AcquireTokenReply));

					pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_ACQUIRE_TOKEN, (PVOID)(&(msg->Notification.AcquireTokenMsg)), sizeof(msg->Notification.AcquireTokenMsg), (PVOID)acquire_token_reply, pMgr->UserCtx);
				}

				break;

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			case NXRMFLT_MSG_TYPE_CHECK_TRUST:

				check_trust_reply = (NXRM_CHECK_TRUST_REPLY *) malloc(sizeof(NXRM_CHECK_TRUST_REPLY));

				if (check_trust_reply)
				{
					check_trust_reply->ReplyHeader.MessageId	= msg->Header.MessageId;
					check_trust_reply->ReplyHeader.Status		= 0;
					memset(&check_trust_reply->CheckTrustReply, 0, sizeof(check_trust_reply->CheckTrustReply));

					pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_CHECK_TRUST, (PVOID)(&(msg->Notification.CheckTrustMsg)), sizeof(msg->Notification.CheckTrustMsg), (PVOID)check_trust_reply, pMgr->UserCtx);
				}

				break;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

			case NXRMFLT_MSG_TYPE_BLOCK_NOTIFICATION:

				pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_BLOCK_NOTIFICATION, (PVOID)(&(msg->Notification.BlockMsg)), sizeof(msg->Notification.BlockMsg), NULL, pMgr->UserCtx);

				break;

			case NXRMFLT_MSG_TYPE_FILE_ERROR_NOTIFICATION:

				pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_FILE_ERROR_NOTIFICATION, (PVOID)(&(msg->Notification.FileErrorMsg)), sizeof(msg->Notification.FileErrorMsg), NULL, pMgr->UserCtx);

				break;

			case NXRMFLT_MSG_TYPE_PURGE_CACHE_NOTIFICATION:

				pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_PURGE_CACHE_NOTIFICATION, (PVOID)(&(msg->Notification.PurgeCacheMsg)), sizeof(msg->Notification.PurgeCacheMsg), NULL, pMgr->UserCtx);

				break;

			case NXRMFLT_MSG_TYPE_PROCESS_NOTIFICATION:

				pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_PROCESS_NOTIFICATION, (PVOID)(&(msg->Notification.ProcessMsg)), sizeof(msg->Notification.ProcessMsg), NULL, pMgr->UserCtx);

				break;
			
			case NXRMFLT_MSG_TYPE_ACTIVITY_LOG:

				pMgr->NotifyCallback(NXRMFLT_MSG_TYPE_ACTIVITY_LOG, (PVOID)(&(msg->Notification.ActivityLogMsg)), sizeof(msg->Notification.ActivityLogMsg), NULL, pMgr->UserCtx);

				break;

			default:
				break;
			}

		}

		//
		// if message is a checkrighs message AND in the case of no callback or can't allocate memory or filtering is disabled, default allow
		//
		if((pMgr->NotifyCallback == NULL || reply == NULL || DisableFiltering) && msg->Notification.Type == NXRMFLT_MSG_TYPE_CHECK_RIGHTS)
		{
			default_reply.ReplyHeader.MessageId			= msg->Header.MessageId;
			default_reply.ReplyHeader.Status			= 0;
			default_reply.CheckRigtsReply.RightsMask	= 0;
			default_reply.CheckRigtsReply.CustomRights	= 0;
			default_reply.CheckRigtsReply.EvaluationId	= 0;

			hr = FilterReplyMessage(pMgr->ConnectionPort,
									&default_reply.ReplyHeader,
									sizeof(default_reply));

			if(FAILED(hr))
			{
				LOGMAN_ERROR(pMgr, "Failed to reply to nxrmflt! (%08X)", hr);
			}
		}

		if ((pMgr->NotifyCallback == NULL || query_token_reply == NULL || DisableFiltering) && msg->Notification.Type == NXRMFLT_MSG_TYPE_QUERY_TOKEN)
		{
			default_query_token_reply.ReplyHeader.MessageId = msg->Header.MessageId;
			default_query_token_reply.ReplyHeader.Status = 0;

			memset(&default_query_token_reply.QueryTokenReply,
				   0,
				   sizeof(default_query_token_reply.QueryTokenReply));

			hr = FilterReplyMessage(pMgr->ConnectionPort,
									&default_query_token_reply.ReplyHeader,
									sizeof(default_query_token_reply));

			if (FAILED(hr))
			{
				LOGMAN_ERROR(pMgr, "Failed to reply to nxrmflt! (%08X)", hr);
			}
		}

		if ((pMgr->NotifyCallback == NULL || acquire_token_reply == NULL || DisableFiltering) && msg->Notification.Type == NXRMFLT_MSG_TYPE_ACQUIRE_TOKEN)
		{
			default_acquire_token_reply.ReplyHeader.MessageId	= msg->Header.MessageId;
			default_acquire_token_reply.ReplyHeader.Status		= 0;
			
			memset(&default_acquire_token_reply.AcquireTokenReply,
				   0,
				   sizeof(default_acquire_token_reply.AcquireTokenReply));

			hr = FilterReplyMessage(pMgr->ConnectionPort,
									&default_acquire_token_reply.ReplyHeader,
									sizeof(default_acquire_token_reply));

			if (FAILED(hr))
			{
				LOGMAN_ERROR(pMgr, "Failed to reply to nxrmflt! (%08X)", hr);
			}
		}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
		if((pMgr->NotifyCallback == NULL || check_trust_reply == NULL || DisableFiltering) && msg->Notification.Type == NXRMFLT_MSG_TYPE_CHECK_TRUST)
		{
			default_check_trust_reply.ReplyHeader.MessageId		= msg->Header.MessageId;
			default_check_trust_reply.ReplyHeader.Status		= 0;
			default_check_trust_reply.CheckTrustReply.Trusted	= FALSE;

			hr = FilterReplyMessage(pMgr->ConnectionPort,
									&default_check_trust_reply.ReplyHeader,
									sizeof(FILTER_REPLY_HEADER) + sizeof(default_check_trust_reply.CheckTrustReply));

			if(FAILED(hr))
			{
				LOGMAN_ERROR(pMgr, "Failed to reply to nxrmflt! (%08X)", hr);
			}
		}
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

		if(pMgr->Stop)
		{
			continue;
		}

		hr = FilterGetMessage(pMgr->ConnectionPort,
							  &msg->Header,
							  FIELD_OFFSET(NXRMFLT_MESSAGE, Ovlp),
							  &msg->Ovlp);

		if(hr == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
		{
			LOGMAN_ERROR(pMgr, "FilterGetMessage aborted! (%08X)", hr);
			continue;
		}
		else if(hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			LOGMAN_ERROR(pMgr, "Failed to get message from nxrmflt! (%08X)", hr);
			continue;
		}

	}

	return hr;
}

HRESULT __stdcall nxrmfltReplyCheckRights(
	NXRMFLT_HANDLE				hMgr, 
	PVOID						msgctx, 
	NXRMFLT_CHECK_RIGHTS_REPLY	*chkrighsreply)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;
	NXRM_CHECK_RIGHTS_REPLY *reply = NULL;

	pMgr = (NXRMFLT_MANAGER *)hMgr;
	reply = (NXRM_CHECK_RIGHTS_REPLY *)msgctx;

	memcpy(&reply->CheckRigtsReply,
		   chkrighsreply,
		   min(sizeof(reply->CheckRigtsReply), sizeof(*chkrighsreply)));

	do 
	{

		hr = FilterReplyMessage(pMgr->ConnectionPort,
								&reply->ReplyHeader,
								sizeof(FILTER_REPLY_HEADER) + sizeof(reply->CheckRigtsReply));

		if(FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "ReplyCheckRights failed to reply message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if(reply)
	{
		memset(reply, 0, sizeof(NXRM_CHECK_RIGHTS_REPLY));
		free(reply);
	}

	return hr;
}

HRESULT NXRMFLTMAN_API __stdcall nxrmfltReplyQueryToken(
	NXRMFLT_HANDLE					hMgr,
	PVOID							msgctx,
	ULONG							Status,
	NXRMFLT_QUERY_TOKEN_REPLY		*querytokenreply)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;
	NXRM_QUERY_TOKEN_REPLY *reply = NULL;

	pMgr = (NXRMFLT_MANAGER *)hMgr;
	reply = (NXRM_QUERY_TOKEN_REPLY *)msgctx;

	reply->ReplyHeader.Status = Status;

	memcpy(&reply->QueryTokenReply,
		   querytokenreply,
		   min(sizeof(reply->QueryTokenReply), sizeof(*querytokenreply)));

	do
	{

		hr = FilterReplyMessage(pMgr->ConnectionPort,
								&reply->ReplyHeader,
								(Status == 0) ? sizeof(FILTER_REPLY_HEADER) + sizeof(reply->QueryTokenReply) : sizeof(FILTER_REPLY_HEADER));

		if (FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "Failed to reply query token message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if (reply)
	{
		memset(reply, 0, sizeof(NXRM_QUERY_TOKEN_REPLY));
		free(reply);
	}

	return hr;
}

HRESULT NXRMFLTMAN_API __stdcall nxrmfltReplyAcquireToken(
	NXRMFLT_HANDLE					hMgr,
	PVOID							msgctx,
	ULONG							Status,
	NXRMFLT_ACQUIRE_TOKEN_REPLY		*acquiretokenreply)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;
	NXRM_ACQUIRE_TOKEN_REPLY *reply = NULL;

	pMgr = (NXRMFLT_MANAGER *)hMgr;
	reply = (NXRM_ACQUIRE_TOKEN_REPLY *)msgctx;

	reply->ReplyHeader.Status = Status;

	memcpy(&reply->AcquireTokenReply,
		   acquiretokenreply,
		   min(sizeof(reply->AcquireTokenReply), sizeof(*acquiretokenreply)));

	do
	{

		hr = FilterReplyMessage(pMgr->ConnectionPort,
								&reply->ReplyHeader,
								(Status == 0) ? sizeof(FILTER_REPLY_HEADER) + sizeof(reply->AcquireTokenReply) : sizeof(FILTER_REPLY_HEADER));

		if (FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "Failed to reply AcquireToken message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if (reply)
	{
		memset(reply, 0, sizeof(NXRM_ACQUIRE_TOKEN_REPLY));
		free(reply);
	}

	return hr;
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

HRESULT __stdcall nxrmfltReplyCheckTrust(
	NXRMFLT_HANDLE				hMgr,
	PVOID						msgctx,
	NXRMFLT_CHECK_TRUST_REPLY	*checktrustreply)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;
	NXRM_CHECK_TRUST_REPLY *reply = NULL;

	pMgr = (NXRMFLT_MANAGER *)hMgr;
	reply = (NXRM_CHECK_TRUST_REPLY *)msgctx;

	memcpy(&reply->CheckTrustReply,
		   checktrustreply,
		   min(sizeof(reply->CheckTrustReply), sizeof(*checktrustreply)));

	do
	{

		hr = FilterReplyMessage(pMgr->ConnectionPort,
								&reply->ReplyHeader,
								sizeof(FILTER_REPLY_HEADER) + sizeof(reply->CheckTrustReply));

		if(FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "ReplyCheckTrust failed to reply message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if(reply)
	{
		memset(reply, 0, sizeof(NXRM_CHECK_TRUST_REPLY));
		free(reply);
	}

	return hr;
}

#else   // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

// Need to define a dummy function here even when
// NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR is not defined, because
// nxrmfltman.def still needs to export a function under this name.
HRESULT __stdcall nxrmfltReplyCheckTrust(
	NXRMFLT_HANDLE				hMgr,
	PVOID						msgctx,
	VOID						*checktrustreply)
{
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetSaveAsForecast(
	NXRMFLT_HANDLE				hMgr,
	ULONG						ProcessId,
	CONST WCHAR					*SrcFileName,
	CONST WCHAR					*SaveAsFileName)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;

	NXRMFLT_COMMAND_MSG	*Msg = NULL;

	NXRMFLT_SAVEAS_FORECAST *pSaveAsForecast = NULL;

	ULONG CommandLength = 0;

	ULONG BytesReturn = 0;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	WCHAR NTPath[MAX_PATH] = { 0 };
	ULONG ccNTPath = 0;

	DWORD dwret;

	do 
	{
		if (!SaveAsFileName)
		{
			hr = __HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

			LOGMAN_ERROR(pMgr, "SaveAsFileName can't be NULL! (%08X)", hr);

			break;
		}

		CommandLength = sizeof(NXRMFLT_COMMAND_MSG) - sizeof(UCHAR) + sizeof(NXRMFLT_SAVEAS_FORECAST);

		Msg = (NXRMFLT_COMMAND_MSG *)malloc(CommandLength);

		if (!Msg)
		{
			hr = E_OUTOFMEMORY;

			LOGMAN_ERROR(pMgr, "Failed to build SaveAs forecast command! (%08X)", hr);

			break;
		}

		memset(Msg, 0, CommandLength);

		Msg->Command = nxrmfltSaveAsForecast;
		Msg->Size = sizeof(NXRMFLT_SAVEAS_FORECAST);
		
		pSaveAsForecast = (NXRMFLT_SAVEAS_FORECAST *)Msg->Data;

		pSaveAsForecast->ProcessId = ProcessId;

		dwret = GetLongPathNameW(SaveAsFileName, pSaveAsForecast->SaveAsFileName, sizeof(pSaveAsForecast->SaveAsFileName));

		if (0 == dwret || dwret >= sizeof(pSaveAsForecast->SaveAsFileName)) {
			WCHAR * pfind;
			memcpy(NTPath, SaveAsFileName, min(sizeof(NTPath) - sizeof(WCHAR), wcslen(SaveAsFileName) * sizeof(WCHAR)));
			pfind = wcsrchr(NTPath, L'\\');
			if (pfind) {
				//try convert path only
				* pfind = 0;
				dwret = GetLongPathNameW(NTPath, pSaveAsForecast->SaveAsFileName, sizeof(pSaveAsForecast->SaveAsFileName));
				if (dwret && dwret < sizeof(pSaveAsForecast->SaveAsFileName)) {
					const WCHAR *pfilename;
					pfilename = wcsrchr(SaveAsFileName, L'\\');
					wcscat_s(pSaveAsForecast->SaveAsFileName, pfilename);
				}
				else {
					//user original string
					memcpy(pSaveAsForecast->SaveAsFileName,
						SaveAsFileName,
						min(sizeof(pSaveAsForecast->SaveAsFileName) - sizeof(WCHAR), wcslen(SaveAsFileName) * sizeof(WCHAR)));
				}
			}
			else {
				//user original string
				memcpy(pSaveAsForecast->SaveAsFileName,
					SaveAsFileName,
					min(sizeof(pSaveAsForecast->SaveAsFileName) - sizeof(WCHAR), wcslen(SaveAsFileName) * sizeof(WCHAR)));
			}
		}

		if (0 == lstrcmpiW(pSaveAsForecast->SaveAsFileName + (wcslen(pSaveAsForecast->SaveAsFileName) - 4), L".nxl"))
		{//found nxl extension. remove it
			pSaveAsForecast->SaveAsFileName[wcslen(pSaveAsForecast->SaveAsFileName) - 4] = 0;
		}

		memset(NTPath, 0, sizeof(NTPath));

		ccNTPath = sizeof(NTPath) / sizeof(WCHAR);

		if (!Win32Path2NTPath(pSaveAsForecast->SaveAsFileName, NTPath, &ccNTPath))
		{
			LOGMAN_ERROR(pMgr, "Failed to convert path %s\n", pSaveAsForecast->SaveAsFileName);
			break;
		}

		memset(pSaveAsForecast->SaveAsFileName, 0, sizeof(pSaveAsForecast->SaveAsFileName));
		memcpy(pSaveAsForecast->SaveAsFileName,
				NTPath,
			   min(sizeof(pSaveAsForecast->SaveAsFileName) - sizeof(WCHAR), wcslen(NTPath)*sizeof(WCHAR)));
		
		if (SrcFileName)
		{
			dwret = GetLongPathNameW(SrcFileName, pSaveAsForecast->SourceFileName, sizeof(pSaveAsForecast->SourceFileName));

			if (0 == dwret || dwret >= sizeof(pSaveAsForecast->SourceFileName)) {
				//user original string
				memcpy(pSaveAsForecast->SourceFileName,
					SrcFileName,
					min(sizeof(pSaveAsForecast->SourceFileName) - sizeof(WCHAR), wcslen(SrcFileName) * sizeof(WCHAR)));
			}

			if (0== lstrcmpiW(pSaveAsForecast->SourceFileName + (wcslen(pSaveAsForecast->SourceFileName) - 4), L".nxl"))
			{//found nxl extension. remove it
				pSaveAsForecast->SourceFileName[wcslen(pSaveAsForecast->SourceFileName) - 4] = 0;
			}

			memset(NTPath, 0, sizeof(NTPath));

			ccNTPath = sizeof(NTPath) / sizeof(WCHAR);

			if (!Win32Path2NTPath(pSaveAsForecast->SourceFileName, NTPath, &ccNTPath))
			{
				LOGMAN_ERROR(pMgr, "Failed to convert path %s\n", pSaveAsForecast->SourceFileName);
				break;
			}
			memset(pSaveAsForecast->SourceFileName, 0, sizeof(pSaveAsForecast->SourceFileName));
			memcpy(pSaveAsForecast->SourceFileName,
				NTPath,
				min(sizeof(pSaveAsForecast->SourceFileName) - sizeof(WCHAR), wcslen(NTPath) * sizeof(WCHAR)));

		}
		else
		{
			memset(pSaveAsForecast->SourceFileName, 0, sizeof(pSaveAsForecast->SourceFileName));
		}
		
		hr = FilterSendMessage(pMgr->ConnectionPort,
							   Msg,
							   CommandLength,
							   NULL,
							   0,
							   &BytesReturn);
		if(FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "SetSaveAsForecast failed to reply message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if (Msg)
	{
		free(Msg);
	}

	return hr;
}

HRESULT NXRMFLTMAN_API __stdcall nxrmfltManageSafeDirectory(
    NXRMFLT_HANDLE				hMgr,
    ULONG                       op,
	CONST WCHAR					*safeDir)
{
    HRESULT	hr = S_OK;

    NXRMFLT_MANAGER *pMgr = NULL;

    NXRMFLT_COMMAND_MSG	*Msg = NULL;

    NXRMFLT_SAFEDIR_INFO *pSafeDirInfo = NULL;

    ULONG CommandLength = 0;

    ULONG BytesReturn = 0;

    pMgr = (NXRMFLT_MANAGER *)hMgr;

    WCHAR NTPath[MAX_PATH] = { 0 };
    ULONG ccNTPath = 0;

    DWORD dwret;

    do
    {
        if (!safeDir)
        {
            hr = __HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

            LOGMAN_ERROR(pMgr, "SafeDir can't be NULL! (%08X)", hr);

            break;
        }

        if (op > 2)
        {
            hr = __HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

            LOGMAN_ERROR(pMgr, "SafeDir op code can only be 0, 1, or 2 can't be NULL! (%08X)", hr);

            break;
        }

        CommandLength = offsetof(NXRMFLT_COMMAND_MSG, Data) + sizeof(NXRMFLT_SAFEDIR_INFO);

        Msg = (NXRMFLT_COMMAND_MSG *)malloc(CommandLength);

        if (!Msg)
        {
            hr = E_OUTOFMEMORY;

            LOGMAN_ERROR(pMgr, "Failed to build SaveAs forecast command! (%08X)", hr);

            break;
        }

        memset(Msg, 0, CommandLength);

        Msg->Command = nxrmfltManageSafeDir;
        Msg->Size = sizeof(NXRMFLT_SAFEDIR_INFO);

        pSafeDirInfo = (PNXRMFLT_SAFEDIR_INFO)Msg->Data;

        pSafeDirInfo->Cmd = op;

		if (1 == pSafeDirInfo->Cmd)
		{
			std::wstring wstrSafeDir = safeDir;
			wchar_t ext = wstrSafeDir[wstrSafeDir.size() - 1];
			wstrSafeDir.pop_back();
			wchar_t option = wstrSafeDir[wstrSafeDir.size() - 1];
			wstrSafeDir.pop_back();

			dwret = GetLongPathNameW(wstrSafeDir.c_str(), pSafeDirInfo->SafeDirPath, NXRMFLT_MAX_PATH);

			pSafeDirInfo->SafeDirPath[wcslen(pSafeDirInfo->SafeDirPath)] = option;
			pSafeDirInfo->SafeDirPath[wcslen(pSafeDirInfo->SafeDirPath)] = ext;
		}
		else
		{
			dwret = GetLongPathNameW(safeDir, pSafeDirInfo->SafeDirPath, NXRMFLT_MAX_PATH);
		}

        if (0 == dwret || dwret >= NXRMFLT_MAX_PATH)
        {
            hr = E_INVALIDARG;

            LOGMAN_ERROR(pMgr, "Failed to convert safe dir to long path! (%08X)", hr);

            break;
        }

        hr = FilterSendMessage(pMgr->ConnectionPort,
            Msg,
            CommandLength,
            NULL,
            0,
            &BytesReturn);
        if (FAILED(hr))
        {
            LOGMAN_ERROR(pMgr, "ManageSafeDirectory failed to reply message to nxrmflt! (%08X)", hr);
        }

    } while (FALSE);

    if (Msg)
    {
        free(Msg);
    }

    return hr;
}

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

HRESULT NXRMFLTMAN_API __stdcall nxrmfltManageSanctuaryDirectory(
    NXRMFLT_HANDLE				hMgr,
    ULONG                       op,
	CONST WCHAR					*sanctuaryDir)
{
    HRESULT	hr = S_OK;

    NXRMFLT_MANAGER *pMgr = NULL;

    NXRMFLT_COMMAND_MSG	*Msg = NULL;

    NXRMFLT_SANCTUARYDIR_INFO *pSanctuaryDirInfo = NULL;

    ULONG CommandLength = 0;

    ULONG BytesReturn = 0;

    pMgr = (NXRMFLT_MANAGER *)hMgr;

    WCHAR NTPath[MAX_PATH] = { 0 };
    ULONG ccNTPath = 0;

    DWORD dwret;

    do
    {
        if (!sanctuaryDir)
        {
            hr = __HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

            LOGMAN_ERROR(pMgr, "SanctuaryDir can't be NULL! (%08X)", hr);

            break;
        }

        if (op > 2)
        {
            hr = __HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

            LOGMAN_ERROR(pMgr, "SanctuaryDir op code can only be 0, 1, or 2 (%08X)", hr);

            break;
        }

        CommandLength = offsetof(NXRMFLT_COMMAND_MSG, Data) + sizeof(NXRMFLT_SANCTUARYDIR_INFO);

        Msg = (NXRMFLT_COMMAND_MSG *)malloc(CommandLength);

        if (!Msg)
        {
            hr = E_OUTOFMEMORY;

            LOGMAN_ERROR(pMgr, "Failed to build Manage Sanctuary Directory command! (%08X)", hr);

            break;
        }

        memset(Msg, 0, CommandLength);

        Msg->Command = nxrmfltManageSanctuaryDir;
        Msg->Size = sizeof(NXRMFLT_SANCTUARYDIR_INFO);

        pSanctuaryDirInfo = (PNXRMFLT_SANCTUARYDIR_INFO)Msg->Data;

        pSanctuaryDirInfo->Cmd = op;

        dwret = GetLongPathNameW(sanctuaryDir, pSanctuaryDirInfo->SanctuaryDirPath, NXRMFLT_MAX_PATH);

        if (0 == dwret || dwret >= NXRMFLT_MAX_PATH)
        {
            hr = E_INVALIDARG;

            LOGMAN_ERROR(pMgr, "Failed to convert sanctuary dir to long path! (%08X)", hr);

            break;
        }

        hr = FilterSendMessage(pMgr->ConnectionPort,
            Msg,
            CommandLength,
            NULL,
            0,
            &BytesReturn);
        if (FAILED(hr))
        {
            LOGMAN_ERROR(pMgr, "ManageSanctuaryDirectory failed to reply message to nxrmflt! (%08X)", hr);
        }

    } while (FALSE);

    if (Msg)
    {
        free(Msg);
    }

    return hr;
}

#else   // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

// Need to define a dummy function here even when
// NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR is not defined, because
// nxrmfltman.def still needs to export a function under this name.
HRESULT NXRMFLTMAN_API __stdcall nxrmfltManageSanctuaryDirectory(
    NXRMFLT_HANDLE				hMgr,
    ULONG                       op,
	CONST WCHAR					*sanctuaryDir)
{
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetPolicyChanged(NXRMFLT_HANDLE	hMgr)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;

	NXRMFLT_COMMAND_MSG	*Msg = NULL;

	ULONG CommandLength = 0;

	ULONG BytesReturn = 0;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	do 
	{
		CommandLength = sizeof(NXRMFLT_COMMAND_MSG) - sizeof(UCHAR);

		Msg = (NXRMFLT_COMMAND_MSG *)malloc(CommandLength);

		if (!Msg)
		{
			hr = E_OUTOFMEMORY;

			LOGMAN_ERROR(pMgr, "Failed to build policy changed command due to out of memory! (%08X)", hr);

			break;
		}

		memset(Msg, 0, CommandLength);

		Msg->Command = nxrmfltPolicyChanged;
		Msg->Size = 0;

		hr = FilterSendMessage(pMgr->ConnectionPort,
							   Msg,
							   CommandLength,
							   NULL,
							   0,
							   &BytesReturn);
		if(FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "Failed to send policy changed message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if (Msg)
	{
		free(Msg);
	}

	return hr;
}

HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetLogonSessionCreated(
	NXRMFLT_HANDLE					hMgr,
	NXRMFLT_LOGON_SESSION_CREATED	*CreateInfo)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;

	NXRMFLT_COMMAND_MSG	*Msg = NULL;

	ULONG CommandLength = 0;

	ULONG BytesReturn = 0;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	do
	{
		CommandLength = sizeof(NXRMFLT_COMMAND_MSG) - sizeof(UCHAR) + sizeof(NXRMFLT_LOGON_SESSION_CREATED);

		Msg = (NXRMFLT_COMMAND_MSG *)malloc(CommandLength);

		if (!Msg)
		{
			hr = E_OUTOFMEMORY;

			LOGMAN_ERROR(pMgr, "Failed to build session start command! (%08X)", hr);

			break;
		}

		memset(Msg, 0, CommandLength);

		Msg->Command = nxrmfltSessionCreated;
		Msg->Size = sizeof(NXRMFLT_LOGON_SESSION_CREATED);

		memcpy(Msg->Data,
			   CreateInfo,
			   sizeof(NXRMFLT_LOGON_SESSION_CREATED));

		hr = FilterSendMessage(pMgr->ConnectionPort,
							   Msg,
							   CommandLength,
							   NULL,
							   0,
							   &BytesReturn);
		if (FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "Failed to send session created message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if (Msg)
	{
		free(Msg);
	}

	return hr;
}

HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetLogonSessionTerminated(
	NXRMFLT_HANDLE						hMgr,
	NXRMFLT_LOGON_SESSION_TERMINATED	*TerminateInfo)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;

	NXRMFLT_COMMAND_MSG	*Msg = NULL;

	ULONG CommandLength = 0;

	ULONG BytesReturn = 0;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	do
	{
		CommandLength = sizeof(NXRMFLT_COMMAND_MSG) - sizeof(UCHAR) + sizeof(NXRMFLT_LOGON_SESSION_TERMINATED);

		Msg = (NXRMFLT_COMMAND_MSG *)malloc(CommandLength);

		if (!Msg)
		{
			hr = E_OUTOFMEMORY;

			LOGMAN_ERROR(pMgr, "Failed to build session terminated command! (%08X)", hr);

			break;
		}

		memset(Msg, 0, CommandLength);

		Msg->Command = nxrmfltSessionTerminated;
		Msg->Size = sizeof(NXRMFLT_LOGON_SESSION_TERMINATED);

		memcpy(Msg->Data,
			   TerminateInfo,
			   sizeof(NXRMFLT_LOGON_SESSION_TERMINATED));

		hr = FilterSendMessage(pMgr->ConnectionPort,
							   Msg,
							   CommandLength,
							   NULL,
							   0,
							   &BytesReturn);
		if (FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "Failed to send session terminated message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if (Msg)
	{
		free(Msg);
	}

	return hr;
}

HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetCleanProcessCache(
	NXRMFLT_HANDLE						hMgr,
	NXRMFLT_CLEAN_PROCESS_CACHE	*ProcessInfo)
{
	HRESULT	hr = S_OK;

	NXRMFLT_MANAGER *pMgr = NULL;

	NXRMFLT_COMMAND_MSG	*Msg = NULL;

	ULONG CommandLength = 0;

	ULONG BytesReturn = 0;

	pMgr = (NXRMFLT_MANAGER *)hMgr;

	do
	{
		CommandLength = sizeof(NXRMFLT_COMMAND_MSG) - sizeof(UCHAR) + sizeof(NXRMFLT_CLEAN_PROCESS_CACHE);

		Msg = (NXRMFLT_COMMAND_MSG *)malloc(CommandLength);

		if (!Msg)
		{
			hr = E_OUTOFMEMORY;

			LOGMAN_ERROR(pMgr, "Failed to build clean process cache command! (%08X)", hr);

			break;
		}

		memset(Msg, 0, CommandLength);

		Msg->Command = nxrmfltCleanProcessCache;
		Msg->Size = sizeof(NXRMFLT_CLEAN_PROCESS_CACHE);

		memcpy(Msg->Data,
			ProcessInfo,
			sizeof(NXRMFLT_CLEAN_PROCESS_CACHE));

		hr = FilterSendMessage(pMgr->ConnectionPort,
			Msg,
			CommandLength,
			NULL,
			0,
			&BytesReturn);
		if (FAILED(hr))
		{
			LOGMAN_ERROR(pMgr, "Failed to send clean process cache message to nxrmflt! (%08X)", hr);
		}

	} while (FALSE);

	if (Msg)
	{
		free(Msg);
	}

	return hr;
}

BOOL Win32Path2NTPath(const WCHAR *src, WCHAR *ntpath, ULONG *ccntpath)
{
	BOOL bRet = FALSE;

	LONG dstlength = 0;

	ULONG srclength = 0;
	ULONG CharCount = 0;

	WCHAR DeviceName[MAX_PATH] = { 0 };

	WCHAR DosDrive[16] = { 0 };

	WCHAR *p = (WCHAR*)src;

	do
	{
		srclength = (ULONG)wcslen(p);

		if (srclength < 4)
		{
			dstlength = 0;
			break;
		}

		if (!wcsstr(src, L"\\"))
		{
			//
			// in the case of there is no "\" in path
			//
			if (*ccntpath > wcslen(src))
			{
				dstlength = (LONG)swprintf_s(ntpath, *ccntpath, L"%s", src);

				if (dstlength != -1)
				{
					bRet = TRUE;
				}
				else
				{
					dstlength = 0;
				}
			}
			else
			{
				dstlength = (ULONG)(wcslen(DeviceName) + wcslen(p));
			}

			break;
		}

		if (src[0] == L'\\' &&
			src[1] == L'\\' &&
			src[2] == L'.' &&
			src[3] == L'\\')
		{
			dstlength = 0;
			break;
		}

		//
		// skip \\?\ prefix
		// 
		if (src[0] == L'\\' &&
			src[1] == L'\\' &&
			src[2] == L'?' &&
			src[3] == L'\\')
		{
			p += 4;
		}

		srclength = (ULONG)wcslen(p);

		if (srclength < 2)
		{
			dstlength = 0;
			break;
		}

		//
		// p point to C:\\path\filename.ext
		//			  ^
		//			  |
		//			  p
		//

		memcpy(DosDrive,
			p,
			2 * sizeof(WCHAR));

		p += 2;

		//
		// p point to C:\\path\filename.ext
		//				^
		//				|
		//				p
		//


		CharCount = QueryDosDeviceW(DosDrive,
			DeviceName,
			(sizeof(DeviceName) / sizeof(WCHAR)) - 1);

		if (CharCount == 0)
		{
			//
			// not a DOS path
			//
			dstlength = 0;
		}

		if (*ccntpath > wcslen(DeviceName) + wcslen(p))
		{
			dstlength = (LONG)swprintf_s(ntpath, *ccntpath, L"%s%s", DeviceName, p);

			if (dstlength != -1)
			{
				bRet = TRUE;
			}
			else
			{
				dstlength = 0;
			}
		}
		else
		{
			dstlength = (ULONG)(wcslen(DeviceName) + wcslen(p));
		}

	} while (FALSE);

	*ccntpath = dstlength;

	return bRet;
}
