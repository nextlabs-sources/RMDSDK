#pragma once

#include "nxrmflt.h"
#include <nudf\shared\logdef.h>
#include <nudf\shared\keydef.h>
#include <nudf\shared\rightsdef.h>

#ifdef _X86_
#ifdef NXRMFLTMAN_EXPORTS
#define NXRMFLTMAN_API __declspec(dllexport)
#else
#define NXRMFLTMAN_API __declspec(dllimport)
#endif
#else
#define NXRMFLTMAN_API
#endif

#ifndef NXRMFLT_MAX_PATH
#define NXRMFLT_MAX_PATH	(260)
#endif

typedef PVOID	NXRMFLT_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

	/************************************************************************/
	/*                                                                      */
	/*	call back functions													*/
	/*                                                                      */
	/************************************************************************/

	typedef ULONG (__stdcall *NXRMFLT_CALLBACK_NOTIFY)(
		ULONG				type,
		PVOID				msg,
		ULONG				Length,
		PVOID				msgctx,
		PVOID				userctx);

	NXRMFLT_HANDLE NXRMFLTMAN_API __stdcall nxrmfltCreateManager(
		NXRMFLT_CALLBACK_NOTIFY		NotifyCallback, 
		LOGAPI_LOG					DebugDumpCallback,
		LOGAPI_ACCEPT				DebugDumpCheckLevelCallback,
		const WCHAR					*VirtualVolumeNTName,
		PVOID						UserContext); 

	HRESULT	NXRMFLTMAN_API __stdcall nxrmfltReplyCheckRights(
		NXRMFLT_HANDLE				hMgr, 
		PVOID						msgctx, 
		NXRMFLT_CHECK_RIGHTS_REPLY	*reply);

	HRESULT NXRMFLTMAN_API __stdcall nxrmfltReplyQueryToken(
		NXRMFLT_HANDLE					hMgr,
		PVOID							msgctx,
		ULONG							Status,
		NXRMFLT_QUERY_TOKEN_REPLY		*reply);

	HRESULT NXRMFLTMAN_API __stdcall nxrmfltReplyAcquireToken(
		NXRMFLT_HANDLE					hMgr,
		PVOID							msgctx,
		ULONG							Status,
		NXRMFLT_ACQUIRE_TOKEN_REPLY		*reply);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	HRESULT	NXRMFLTMAN_API __stdcall nxrmfltReplyCheckTrust(
		NXRMFLT_HANDLE				hMgr,
		PVOID						msgctx,
		NXRMFLT_CHECK_TRUST_REPLY	*reply);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetSaveAsForecast(
		NXRMFLT_HANDLE				hMgr,
		ULONG						ProcessId,
		CONST WCHAR					*SrcFileName,
		CONST WCHAR					*SaveAsFileName);

    HRESULT NXRMFLTMAN_API __stdcall nxrmfltManageSafeDirectory(
        NXRMFLT_HANDLE				hMgr,
        ULONG                       op,
        CONST WCHAR					*safeDir);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    HRESULT NXRMFLTMAN_API __stdcall nxrmfltManageSanctuaryDirectory(
        NXRMFLT_HANDLE				hMgr,
        ULONG                       op,
        CONST WCHAR					*sanctuaryDir);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetPolicyChanged(NXRMFLT_HANDLE	hMgr);
	
	HRESULT	NXRMFLTMAN_API __stdcall nxrmfltStartFiltering(NXRMFLT_HANDLE hMgr);

	HRESULT	NXRMFLTMAN_API __stdcall nxrmfltStopFiltering(NXRMFLT_HANDLE hMgr);

	HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetLogonSessionCreated(
		NXRMFLT_HANDLE					hMgr, 
		NXRMFLT_LOGON_SESSION_CREATED	*CreateInfo);

	HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetLogonSessionTerminated(
		NXRMFLT_HANDLE						hMgr, 
		NXRMFLT_LOGON_SESSION_TERMINATED	*TerminateInfo);

	HRESULT	NXRMFLTMAN_API __stdcall nxrmfltCloseManager(NXRMFLT_HANDLE hMgr);

	HRESULT NXRMFLTMAN_API __stdcall nxrmfltSetCleanProcessCache(
		NXRMFLT_HANDLE					hMgr,
		NXRMFLT_CLEAN_PROCESS_CACHE		*ProcessInfo);

#ifdef __cplusplus
}
#endif