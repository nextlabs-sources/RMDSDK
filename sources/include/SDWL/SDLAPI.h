/*!
 * \file SDLAPI.h
 *
 * \author hbwang
 * \date October 2017
 *
 * The entry functions for RMD SDK Library.
 */

#pragma once
#include "SDLInstance.h"

void SDWLibInit(void);
/// Initialize the library.
/**
 * @post
 *    SDWLibCleanup must be called to clean up the library.
 */

void SDWLibCleanup(void);
/// Clean up the library.
/**
 * @pre
 *    SDWLibInit must have been called to initialize the library.
 */

DWORD SDWLibGetVersion(void);
/// Return the version number of the module.
/**
 * @return
 *    The format of version is AABBCCCC major.minor.build.
 */

SDWLResult SDWLibCreateInstance(ISDRmcInstance ** pInstance);
/// Create an ISDRmcInstance instance.
/**
* @param
*
* @return
*          SDWL_NOT_ENOUGH_MEMORY      not enough memory
* @post
*    SDWLibDeleteRmcInstance must be called to delete this instance.
*
* Note: One ISDRmcInstance instance should be used for one user only.  It should not be re-used for a different user.
*/

SDWLResult SDWLibCreateSDRmcInstance(const WCHAR * sdklibfolder, const WCHAR * tempfolder, ISDRmcInstance ** pInstance, const char * clientid = NULL, uint32_t id = 0);
/// Create an ISDRmcInstance instance.
/**
 * @param
 *    sdklibfolder      fullpath of top folder of RMD SDK library files
 *    tempfolder        fullpath of temporary folder for RMD SDK library
 *    pInstance         return ISDRmcInstance pointer.  If ISDRmcInstance pointer is NULL, check return result;
 *    clientid          internal use only
 *    id                internal use only
 * @return
 *          SDWL_NOT_ENOUGH_MEMORY      not enough memory
 * @post
 *    SDWLibDeleteRmcInstance must be called to delete this instance.
 *
 * Note: One ISDRmcInstance instance should be used for one user only.  It should not be re-used for a different user.
 */

SDWLResult SDWLibCreateSDRmcInstance(const CHAR * productName, uint32_t productMajorVer, uint32_t productMinorVer, uint32_t productBuild, const WCHAR * sdklibfolder, const WCHAR * tempfolder, ISDRmcInstance ** pInstance, const char * clientid = NULL, uint32_t id = 0);
/// Create an ISDRmcInstance instance, and allow software update checking later.
/**
 * @param
 *    productName       name of product
 *    productMajorVer   major version number of product
 *    productMinorVer   minor version number of product
 *    productBuild      build number of product
 *    sdklibfolder      fullpath of top folder of RMD SDK library files
 *    tempfolder        fullpath of temporary folder for RMD SDK library
 *    pInstance         return ISDRmcInstance pointer.  If ISDRmcInstance pointer is NULL, check return result;
 *    clientid          internal use only
 *    id                internal use only
 * @return
 *          SDWL_NOT_ENOUGH_MEMORY      not enough memory
 * @post
 *    SDWLibDeleteRmcInstance must be called to delete this instance.
 *
 * Note: One ISDRmcInstance instance should be used for one user only.  It should not be re-used for a different user.
 */

void SDWLibDeleteRmcInstance(ISDRmcInstance *pInstance);
///Delete an ISDRmcInstance instance.
/**
 * @pre
 *    SDWLibCreateSDRmcInstance must have been called to created this instance.
 * @param
 *    pInstance     ISDRmcInstance instance.
 */

SDWLResult RPMGetCurrentLoggedInUser(std::string &passcode, ISDRmcInstance *&pInstance, ISDRmTenant *&pTenant, ISDRmUser *&pUser);
///Get current login user
/**
* @pre
*    no need to initialize the instance
* @param
*    passcode       plugin passcode to be authorized
*    pInstance      return ISDRmcInstance pointer.  If ISDRmcInstance pointer is NULL, check return result;
*    pTenant        return ISDRmTenant pointer.  If ISDRmTenant pointer is NULL, check return result;
*    pUser          return ISDRmUser pointer.  If ISDRmUser pointer is NULL, check return result;
*/