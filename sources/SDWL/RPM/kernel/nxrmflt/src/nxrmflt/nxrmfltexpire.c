#include "nxrmfltdef.h"
#include "nxrmflt.h"
#include "nxrmfltexpire.h"
#include "nxrmfltutils.h"

extern DECLSPEC_CACHEALIGN ULONG				gTraceFlags;
extern DECLSPEC_CACHEALIGN NXRMFLT_GLOBAL_DATA	Global;

void nxrmfltExpireThreadProc(PVOID ThreadCtx)
{
	NTSTATUS Status = STATUS_SUCCESS;

	LARGE_INTEGER Timeout = {0};
	LARGE_INTEGER CurrentUTCTime = { 0 };

	LIST_ENTRY	ExpiredNodeList = {0};
	LIST_ENTRY	AdobeExpiredNodeList = {0};

	LIST_ENTRY *ite = NULL;
	LIST_ENTRY *tmp = NULL;

	LIST_ENTRY ActiveSessionsList = { 0 };

	Timeout.QuadPart = NXRMFLT_SAVEAS_EXPIRE_TIMEOUT_1S * 5;

	while(TRUE)
	{
		Status = KeWaitForSingleObject(&Global.ExpireStopEvent, Executive, KernelMode, FALSE, &Timeout);

		if (Status == STATUS_SUCCESS)
		{
			break;
		}
		else if (Status == STATUS_TIMEOUT)
		{
			InitializeListHead(&ExpiredNodeList);

			FltAcquirePushLockExclusive(&Global.ExpireTableLock);

			FOR_EACH_LIST_SAFE(ite, tmp, &Global.ExpireTable)
			{
				NXL_SAVEAS_NODE *pNode = CONTAINING_RECORD(ite, NXL_SAVEAS_NODE, Link);

				NT_ASSERT(ite->Flink);

				if (pNode->ExpireTick >= 12)	// expire in 60 seconds
				{
					RemoveEntryList(ite);

					InsertHeadList(&ExpiredNodeList, &pNode->Link);
				}
				else
				{
					pNode->ExpireTick++;
				}
			}

			FltReleasePushLock(&Global.ExpireTableLock);

			InitializeListHead(&AdobeExpiredNodeList);

			FltAcquirePushLockExclusive(&Global.AdobeRenameExpireTableLock);

			FOR_EACH_LIST_SAFE(ite, tmp, &Global.AdobeRenameExpireTable)
			{
				ADOBE_RENAME_NODE *pNode = CONTAINING_RECORD(ite, ADOBE_RENAME_NODE, Link);

				NT_ASSERT(ite->Flink);

				if (pNode->ExpireTick >= 6)		// expire in 30 seconds
				{
					RemoveEntryList(ite);

					InsertHeadList(&AdobeExpiredNodeList, &pNode->Link);
				}
				else
				{
					pNode->ExpireTick++;
				}
			}

			FltReleasePushLock(&Global.AdobeRenameExpireTableLock);

			NT_ASSERT(ExpiredNodeList.Flink);

			//
			// free expire table
			//
			FOR_EACH_LIST_SAFE(ite, tmp, &ExpiredNodeList)
			{
				NXL_SAVEAS_NODE *pNode = CONTAINING_RECORD(ite, NXL_SAVEAS_NODE, Link);

				NT_ASSERT(ite->Flink);

				RemoveEntryList(ite);

				//
				// Wait for all other threads rundown
				//
				ExWaitForRundownProtectionRelease(&pNode->NodeRundownRef);

				ExRundownCompleted(&pNode->NodeRundownRef);

				memset(pNode, 0, sizeof(NXL_SAVEAS_NODE));

				ExFreeToPagedLookasideList(&Global.SaveAsExpireLookaside, pNode);
			}
			
			//
			// free Adobe rename expire table
			//
			FOR_EACH_LIST_SAFE(ite, tmp, &AdobeExpiredNodeList)
			{
				ADOBE_RENAME_NODE *pNode = CONTAINING_RECORD(ite, ADOBE_RENAME_NODE, Link);

				RemoveEntryList(ite);

				nxrmfltFreeAdobeRenameNode(pNode);
			}
			//
			// expire Token cache for each session
			//
			KeQuerySystemTime(&CurrentUTCTime);

			InitializeListHead(&ActiveSessionsList);

			FltAcquirePushLockShared(&Global.SessionCacheLock);

			ite = NULL;

			FOR_EACH_LIST(ite, &Global.SessionCache)
			{
				NXL_SESSION_CACHE_NODE *pNode = CONTAINING_RECORD(ite, NXL_SESSION_CACHE_NODE, Link);

				pNode->ExpireTick++;

				if ((pNode->ExpireTick % 120) == 0)		// 10 minutes
				{
					ExAcquireRundownProtection(&pNode->NodeRundownRef);

					InsertTailList(&ActiveSessionsList, &pNode->Link2);
				}
			}

			FltReleasePushLock(&Global.SessionCacheLock);

			ite = NULL;
			tmp = NULL;

			FOR_EACH_LIST_SAFE(ite, tmp, &ActiveSessionsList)
			{
				NXL_SESSION_CACHE_NODE *pNode = CONTAINING_RECORD(ite, NXL_SESSION_CACHE_NODE, Link2);

				FltAcquirePushLockExclusive(&pNode->TokenCacheLock);

				nxrmfltFreeExpiredTokenCacheNode(&pNode->TokenCache, &CurrentUTCTime);

				FltReleasePushLock(&pNode->TokenCacheLock);

				RemoveEntryList(ite);

				ExReleaseRundownProtection(&pNode->NodeRundownRef);
			}
		}
		else
		{
			ASSERT(!"THIS SHOULD NEVER HAPPEN!!!");
		}
	}

	PsTerminateSystemThread(Status);
}