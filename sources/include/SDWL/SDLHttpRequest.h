/*!
 * \file SDLHttpRequest.h
 *
 * \author hbwang
 * \date 2017/11/13 11:47
 *
 * 
 */

#include <string>
#include <vector>
#pragma once

typedef std::pair<std::wstring, std::wstring> HttpCookie;
typedef std::vector<HttpCookie> HttpCookies;
typedef std::vector<std::wstring> HttpAcceptTypes;

class ISDRmHttpRequest
{
public:
	ISDRmHttpRequest() {};
	virtual ~ISDRmHttpRequest() {};

public:
	virtual const std::wstring& GetMethod() const = 0;
	/// HTTP Method use for the SkyDRM HTTP request
	/**
	* @return
	*    if string length is Zero, check Instance API return error code for detail.
	*/	
	virtual const std::wstring& GetDomainUrl() const = 0;
	/// HTTP Domain name used for the SkyDRM HTTP request
	/**
	* @return
	*	if string length is zero, check Instance API return error code for detail
	*	url includes the http protocol should be used.
	*/
	virtual const std::wstring& GetPath() const = 0;
	/// HTTP Path used for the SkyDRM HTTP request
	/**
	* @return
	*	if string length is zero, check Instance API return error code for detail.
	*/
	virtual const HttpCookies& GetCookies() const = 0;
	/// Cookies need be set for the SkyDRM HTTP request
	/**
	 * @return
	 *    vector of string pairs for required Cookies <Name, Value>
	 *	  if there is no Cookie need be set, vector size is zero.
	 */
	virtual const HttpAcceptTypes& GetAcceptTypes() const = 0;
	/// Brief description of method
	/**
	 * @return
	 *	vector of strings for accept types
	 *	if no limitation, vector size is zero.
	 */
};