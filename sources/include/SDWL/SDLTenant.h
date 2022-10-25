/*!
 * \file SDLTenant.h
 *
 * \author hbwang
 * \date November 2017
 *
 * 
 */
#include <string>

#pragma once
class ISDRmTenant
{
public:
	ISDRmTenant() {};
	virtual ~ISDRmTenant() {};

public:
	virtual const std::wstring GetTenant(void) = 0;
	/// Get Tenant ID string for RMS
	/**
	* @return
	*    Result: tenant string. If no tenant is set, default tenant id will be returned
	*/
	virtual const std::wstring GetRouterURL(void) = 0;
	/// Get Router URL string for RMS 
	/**
	* @return
	*    Result: Router URL string. if no router url is set, default router url will be returned
	*/

	virtual const std::wstring GetRMSURL(void) = 0;
	/// Get RMS Restful URL associated with Tenant ID and Router URL 
	/**
	* @return RMS URL string. empty string if the information is not available
	*/
};
