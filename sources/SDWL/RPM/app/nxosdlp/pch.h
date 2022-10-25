#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <combaseapi.h>
#include <shellapi.h>

#include <Ole2.h>
#include <OleDlg.h>		// some ole standard dialog, like ,insert-dialog

// c++
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <cctype>


/*------------------------------------
	global helper
-------------------------------------*/
inline void dev_log(const wchar_t* str) {
#ifdef _DEBUG
	::OutputDebugStringW(str);
#endif // _DEBUG
}

#ifdef DEVLOG
#error "can't be this, someone has defined DEVLOG"
#endif

#ifdef DEVLOG_FUN
#error "can't be this, someone has defined DEVLOG_FUN"
#endif


#ifdef _DEBUG
#define DEVLOG(str)   dev_log((str))
#define DEVLOG_FUN    dev_log(( __FUNCTIONW__ L"\n"  ))
#else
#define DEVLOG(str) (0)
#define DEVLOG_FUN (0);
#endif // _DEBUG

#endif //PCH_H



