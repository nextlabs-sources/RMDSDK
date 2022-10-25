#include "stdafx.h"

#include "SDRmLoginHttpRequest.h"
#include "urlmon.h"
#include "wininet.h"

#include "assert.h"

#include "rmccore/restful/rmuser.h"

using namespace SkyDRM;
using namespace NX::REST;

using namespace RMCCORE;

CSDRmLoginHttpRequest::CSDRmLoginHttpRequest() :ISDRmHttpRequest(), m_domain(L""), m_path(L""), m_method(L"")
{
	m_cookies.clear();
	m_accept_types.clear();
}


CSDRmLoginHttpRequest::~CSDRmLoginHttpRequest()
{
}

SDWLResult CSDRmLoginHttpRequest::Initialize(CSDRmTenant &tenant, RMSystemPara &param)
{
	if (tenant.GetRMSURL().length() == 0)
		return RESULT2(SDWL_NOT_FOUND, "Invalid RMS server URL");


	m_domain = tenant.GetRMSURL();
	if (m_domain.length() == 0 ) {
		return RESULT2(SDWL_INVALID_DATA, "Invalid tenant information");
	}
	if (param.GetClientID().length() == 0) {
		return RESULT2(SDWL_INVALID_DATA, "Invalid client id information");
	}
	
	try {
		RMUser user(param, tenant);
		HTTPRequest loginrequest = user.GetUserLoginURL();
		m_path = NX::conv::to_wstring(loginrequest.GetURL());
		m_method = NX::conv::to_wstring(loginrequest.GetMethod());

		// Bug: RMDC-142
		// reset m_cookies for each Initialize call
		m_cookies.clear();

		for_each(loginrequest.GetCookies().begin(), loginrequest.GetCookies().end(), [&](const RMCCORE::http::cookie &cookie) {
			m_cookies.push_back(std::pair<std::wstring, std::wstring>(NX::conv::to_wstring(cookie.first), NX::conv::to_wstring(cookie.second)));
		});
		for_each(loginrequest.GetAcceptTypes().begin(), loginrequest.GetAcceptTypes().end(), [&](const std::string &item) {
			m_accept_types.push_back(NX::conv::to_wstring(item));
		});

	}
	catch (...) {
		return RESULT2(SDWL_INVALID_DATA, "Fail to initialize Login request with current data.");
	}
	
	WCHAR szDecodedUrl[INTERNET_MAX_URL_LENGTH];
	DWORD dwout = INTERNET_MAX_URL_LENGTH;
	if (S_OK != CoInternetParseUrl(m_domain.c_str(), PARSE_DOMAIN, 0, szDecodedUrl, INTERNET_MAX_URL_LENGTH, &dwout, 0))
	{
		return RESULT2(SDWL_INVALID_DATA, "Fail to parser tenant RMS URL domain name!");
	}

	m_domain = L"https://";
	m_domain+= szDecodedUrl;


	return RESULT(0);
}