#pragma once
#include "SDLTenant.h"
#include "SDLResult.h"
#include "Network/http_client.h"

#include <string>

//RMCCore headers
#include "rmccore/network/httpConst.h"
#include "rmccore/restful/rmtenant.h"
#include "Common/stringex.h"

namespace SkyDRM {
	class CSDRmTenant :
		public ISDRmTenant, public RMCCORE::RMTenant
	{
	public:
		CSDRmTenant(const std::wstring router = L"", const std::wstring tenant = L"");
		CSDRmTenant(const std::string router, const std::string tenant);
		CSDRmTenant(const RMCCORE::RMTenant &tenant);
		~CSDRmTenant();
	public:
		const std::wstring GetTenant(void) { return NX::conv::to_wstring(m_tenant); }
		const std::wstring GetRouterURL(void) { return NX::conv::to_wstring(m_routeurl); }
		const std::wstring GetRMSURL(void) { return NX::conv::to_wstring(m_serverurl); }

	private:
		friend class CSDRmcInstance;
	};
}

