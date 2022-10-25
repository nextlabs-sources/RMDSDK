/*!
 * \file SDLError.h
 *
 * \author hbwang
 * \date October 2017
 *
 * The error code is the same as Windows error code - winerror.h.
 *
 */
#pragma once
#include "winerror.h"

#define SDWL_SUCCESS						ERROR_SUCCESS
#define SDWL_PATH_NOT_FOUND					ERROR_PATH_NOT_FOUND
#define SDWL_INVALID_JSON_FORMAT			ERROR_BAD_FORMAT
#define SDWL_INVALID_DATA					ERROR_INVALID_DATA 
#define SDWL_CERT_NOT_INSTALLED				ERROR_INSTALL_FAILURE
#define SDWL_NOT_READY						ERROR_NOT_READY
#define SDWL_NOT_ENOUGH_MEMORY				ERROR_NOT_ENOUGH_MEMORY
#define SDWL_NOT_FOUND						ERROR_NOT_FOUND
#define SDWL_INTERNAL_ERROR					ERROR_INTERNAL_ERROR
#define SDWL_LOGIN_REQUIRED					ERROR_NOT_LOGGED_ON
#define SDWL_ACCESS_DENIED					ERROR_ACCESS_DENIED
#define SDWL_BUSY							ERROR_BUSY
#define SDWL_ALREADY_EXISTS					ERROR_ALREADY_EXISTS


/*
 Sepcial designed for specific nxl relevant errors

*/
#define SDWL_NXL_BASE						0xE000
#define SDWL_NXL_INSUFFICIENT_RIGHTS		(SDWL_NXL_BASE+1)
#define SDWL_NXL_NOT_AUTHORIZE	            (SDWL_NXL_BASE+2)

/************************************************************************/
/* RMS Error Code base starts from 0xF000                               */
/* for API return code from RMS, use the error code minus Error code    */
/*   base to get the result.                                            */
/*   For example, error code 61940 means 500 (61940-0xF000)             */
/************************************************************************/
#define SDWL_RMS_ERRORCODE_BASE				0xF000  //61440
