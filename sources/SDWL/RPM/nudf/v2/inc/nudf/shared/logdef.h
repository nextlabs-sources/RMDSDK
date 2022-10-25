

#ifndef __NUDF_SHARE_LOGDEF_H__
#define __NUDF_SHARE_LOGDEF_H__

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif



//
//  Log Level
//
//Following debug log level definition copied from dbglog.hpp. 
typedef enum _LOGLEVEL {
	LL_CRITICAL = 0,
	LL_ERROR,
	LL_WARNING,
	LL_INFO,
	LL_INFO_1,
	LL_INFO_2,
	LL_INFO_3,
	LL_INFO_4,
	LL_INFO_5,
	LL_INFO_6,
	LL_INFO_7,
	LL_INFO_8,
	LL_INFO_9,
	LL_DEBUG,
	LL_DETAIL,
	LL_ALL
//	LOGUSER = 200
} LOGLEVEL;

#define LOGNUNKNOWN     "UKN"
#define LOGNCRITICAL    "CRI"
#define LOGNERROR       "ERR"
#define LOGNWARNING     "WAR"
#define LOGNINFO        "INF"
#define LOGNDEBUG       "DBG"
#define LOGNDETAIL      "DTL"
#define LOGNUSER        "USR"

//
//  Log Functions
//

typedef BOOL (WINAPI *LOGAPI_ACCEPT)(_In_ ULONG Level);
typedef LONG (WINAPI *LOGAPI_LOG)(_In_ ULONG level,_In_ LPCWSTR Info);

#ifdef __cplusplus
}
#endif


#endif  // #ifndef __NUDF_SHARE_LOGDEF_H__