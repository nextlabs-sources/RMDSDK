// CopyHookObj.cpp : Implementation of CCopyHookObj

#include "stdafx.h"
#include "CopyHookObj.h"
#include "dllmain.h"
#include <boost/algorithm/string.hpp>

// CCopyHookObj

STDMETHODIMP_(UINT) CCopyHookObj::CopyCallback(HWND hwnd, UINT wFunc, UINT wFlags, LPCWSTR pszSrcFile, DWORD dwSrcAttribs, LPCWSTR pszDestFile, DWORD dwDestAttribs)
{
	if (wFunc == FO_MOVE && g_isDllhost && pszSrcFile != NULL && pszDestFile != NULL)
	{
		std::wstring wstrSrcFile = pszSrcFile;
		std::wstring wstrDestFile = pszDestFile;

		if (wstrSrcFile.size() > 1 && wstrDestFile.size() > 1 && !boost::algorithm::iequals(wstrSrcFile.substr(0, 2), wstrDestFile.substr(0, 2)))
		{
			RPMFolderRelation SrcRPMRelation = GetFolderRelation(wstrSrcFile);
#ifdef _DEBUG
			AssertRPMFolderRelation(SrcRPMRelation);
#endif
			if (SrcRPMRelation.bRPMFolder || SrcRPMRelation.bRPMAncestralFolder)
			{
				return IDNO;
			}
			else if (SrcRPMRelation.bRPMInheritedFolder)
			{
				RPMFolderRelation DestRPMRelation = GetFolderRelation(wstrDestFile);
#ifdef _DEBUG
				AssertRPMFolderRelation(DestRPMRelation);
#endif
				if (!DestRPMRelation.bRPMFolder && !DestRPMRelation.bRPMInheritedFolder)
				{
					return IDNO;
				}
			}
		}

		return IDNO;
	}

	return IDOK;
}

