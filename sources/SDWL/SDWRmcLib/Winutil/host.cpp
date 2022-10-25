
#include "..\stdafx.h"

#include "host.h"
#include "..\common\string.h"

using namespace NX;
using namespace NX::win;


const HostInfo& NX::win::GetLocalHostInfo()
{
	static const HostInfo hostinfo(HostInfo::QueryLocalHostInfo());
	return hostinfo;
}

HostInfo::HostInfo()
{
}

HostInfo::HostInfo(const HostInfo& rhs) : _hostName(rhs._hostName), _domainName(rhs._domainName), _fqdn(rhs._fqdn)
{
}

HostInfo::~HostInfo()
{
}

HostInfo& HostInfo::operator = (const HostInfo& rhs)
{
	if (this != &rhs) {
		_hostName = rhs._hostName;
		_domainName = rhs._domainName;
		_fqdn = rhs._fqdn;
	}
	return *this;
}

HostInfo HostInfo::QueryLocalHostInfo()
{
	HostInfo info;
	unsigned long size = MAX_PATH;
	if (!GetComputerNameExW(ComputerNameDnsFullyQualified, NX::wstring_buffer(info._fqdn, MAX_PATH), &size)) {
		info._fqdn.clear();
	}
	size = MAX_PATH;
	if (!GetComputerNameExW(ComputerNameDnsHostname, NX::wstring_buffer(info._hostName, MAX_PATH), &size)) {
		info._hostName.clear();
	}
	size = MAX_PATH;
	if (!GetComputerNameExW(ComputerNameDnsDomain, NX::wstring_buffer(info._domainName, MAX_PATH), &size)) {
		info._domainName.clear();
	}
	return info;
}