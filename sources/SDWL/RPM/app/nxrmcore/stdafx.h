// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
// Windows Header Files
#include <windows.h>


//c++
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iterator>

// reference additional headers your program requires here


inline void dev_log(const wchar_t* str) {
#ifdef _DEBUG
	::OutputDebugStringW(str);
#endif // _DEBUG
}
#ifdef DEVLOG
#error "can't be this'"
#else
#ifdef _DEBUG
#define DEVLOG(str)   dev_log((str))
#else
#define DEVLOG(str) (0)
#endif // _DEBUG
#endif // DEVLOG