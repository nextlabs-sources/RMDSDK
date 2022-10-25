/*!
 * \file SDLNXLFile.h
 *
 * \author hbwang
 * \date 2018/04/16 16:04
 *
 * 
 */
#include <string>
#include <vector>

#pragma once
#include "SDLTypeDef.h"


/*
ISDRmNXLFile: can get nxl file information. such as file size, rights, tags, expiration, water mark and tenant name.
*/
class ISDRmNXLFile
{
public:
	ISDRmNXLFile() {};
	virtual ~ISDRmNXLFile() {};

public:
	virtual const std::wstring GetFileName(void) = 0;
	/// Get NXL filename
	/**
	* @return
	*    Result: filename string
	*/
	virtual const uint64_t GetFileSize(void) = 0;
	/// Get NXL file size
	/**
	* @return
	*    Result: file size.
	*/

	virtual bool IsValidNXL(void) = 0;
	/// If the current file is a valid NXL file
	/**
	* @return true if file is valid NXL file. if the file is not valid NXL, the return value of following function cannot be trusted.
	*/

	virtual const std::vector<SDRmFileRight> GetRights(void) = 0;
	/// Get NXL file rights
	/**
	* @return array of rights asssigned to current file.
	*/

	virtual bool CheckRights(SDRmFileRight right) = 0;
	/// check if current file has specific right
	/**
	* @return true if file has right. when calling this function, a activity log will send to server for the request.
	*/

	virtual bool IsOpen() = 0;
	/// Is file open
	/**
	* @param
	*    none
	* @return
	*           true if file is open
	*/

	//virtual bool IsUploadToRMS(void) = 0;
	/// check if current file has been uploaded to RMS
	/**
	* @return true if file is uploaded. false if the file is not uploaded.
	*/

	//virtual bool IsRecipientsSynced(void) = 0;
	/// check if shared users (recipients) of current file have been synced/uploaded to RMS
	/**
	* @return true if recipients are uploaded. false if recipients are not uploaded.
	*/

	virtual std::string	GetTenantName() const = 0;
	/// Get tenant from the protected file
	/**
	* @param
	*    none
	* @return
	*           tenant data.
	*/

	virtual std::string GetAdhocWaterMarkString()  const= 0;

	virtual const SDR_WATERMARK_INFO GetWaterMark() = 0;
	/// Get water mark info
	/**
	* @param
	*    none
	* @return
	*           SDR_WATERMARK_INFO structure
	*/

	virtual const std::string GetTags() = 0;
	/// Get nxl file tags
	/**
	* @param
	*    none
	* @return
	*           tags string
	*/

	virtual const SDR_Expiration GetExpiration() = 0;
	/// Get nxl file expiration
	/**
	* @param
	*    none
	* @return
	*           SDR_Expiration
	*/

	virtual bool CheckExpired(std::string memberid = "") = 0;
	/// Check if nxl file is expirated
	/**
	* @param
	*    memberid   membership id
	* @return
	*           result bool
	*/

	virtual bool IsOwner(std::string membershipid) = 0;
	/// Check if user is file owner
	/**
	* @param
	*    membershipid   user's membership id of default tenant or project tenant
	* @return
	*           result bool
	*/

	virtual const std::string Get_CreatedBy() = 0;
	/// Get Creator's membershipid from NXL file header
	/**
	* @return
	*           creator's membershipid
	*/
	virtual const std::string Get_ModifiedBy() = 0;
	/// Get Midifier's membershipid from NXL file header
	/**
	* @return
	*           modifier's membershipid
	*/
	virtual const uint64_t Get_DateCreated(void) = 0;
	/// Get Creation time from NXL file header
	/**
	* @return
	*           creation time
	*/
	virtual const uint64_t Get_DateModified(void) = 0;
	/// Get Modification time from NXL file header
	/**
	* @return
	*           Modification time
	*/

	///Following functions are valid only when file is uploaded to RMS
};

