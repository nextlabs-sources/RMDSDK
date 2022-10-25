/*!
 * \file SDRmLoginHttpRequest.h
 *
 * \author hbwang
 * \date 2017/11/13 12:24
 *
 * 
 */

#pragma once
#include "SDLHttpRequest.h"
#include "SDRmTenant.h"
#include "SDLResult.h"

#include "rmccore/restful/rmsyspara.h"

namespace SkyDRM {
	class CSDRmLoginHttpRequest :
		public ISDRmHttpRequest
	{
	public:
		CSDRmLoginHttpRequest();
		~CSDRmLoginHttpRequest();

	public:
		const std::wstring& GetMethod() const { return m_method; }
		const std::wstring& GetDomainUrl() const { return m_domain; }
		const std::wstring& GetPath() const { return m_path; }
		const HttpCookies& GetCookies() const { return m_cookies; }
		const HttpAcceptTypes& GetAcceptTypes() const { return m_accept_types; }
	public:
		SDWLResult Initialize(CSDRmTenant &tenant, RMCCORE::RMSystemPara &param);
	private:
		std::wstring    m_method;
		std::wstring	m_domain;
		std::wstring    m_path;
		HttpAcceptTypes m_accept_types;
		HttpCookies     m_cookies;
	};

}