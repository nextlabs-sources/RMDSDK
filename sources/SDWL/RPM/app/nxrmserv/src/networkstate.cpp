
#include <Windows.h>
#include <assert.h>
#include <wininet.h>
#include <string>
#include "networkstate.hpp"

BOOL CheckInternetConnection(std::wstring strHost)
{
	//std::wstring strHost = L"https://rmtest.nextlabs.solutions";
	BOOL bval = InternetCheckConnection(strHost.c_str(), FLAG_ICC_FORCE_CONNECTION, 0L);
	return bval;
}