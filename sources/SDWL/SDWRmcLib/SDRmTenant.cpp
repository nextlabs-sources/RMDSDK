#include "SDRmTenant.h"
#include "assert.h"

using namespace SkyDRM;


CSDRmTenant::CSDRmTenant(std::wstring router, std::wstring tenant) : ISDRmTenant(), RMTenant(NX::conv::to_string(router), NX::conv::to_string(tenant))
{

}

CSDRmTenant::CSDRmTenant(std::string router, std::string tenant) : ISDRmTenant(), RMTenant(router, tenant)
{

}

CSDRmTenant::CSDRmTenant(const RMCCORE::RMTenant &tenant) : ISDRmTenant(), RMTenant(tenant)
{

}

CSDRmTenant::~CSDRmTenant()
{
}
