#pragma once
/*!
* \file ISDRFiles.h
*
* \author stsai
* \date 2018/04/19 
*
*
*/
#ifndef __ISDRFiles_H__
#define __ISDRFiles_H__

#pragma once

#include <string>
#include <vector>
#include "SDLNXLFile.h"

/*
ISDRFiles: local files manager. This class will store all the files information under working/user id directory.
*/
class ISDRFiles
{
public:
	ISDRFiles() { };
	virtual ~ISDRFiles() { };

	virtual size_t GetListNumber(void) const = 0;
	/// Get files list number
	/**
	* @return size of files
	*/

	virtual std::vector<std::wstring> GetList() const = 0;
	/// Get files list 
	/**
	* @return array of files
	*
	*/

	virtual ISDRmNXLFile* GetFile(size_t index)const  = 0;
	/// Get NXL file class by index
	/**
	* @param
	*    index		index to file list
	* @return NXL file class
	*
	*/

	virtual ISDRmNXLFile* GetFile(const std::wstring& filename)const  = 0;
	/// Get NXL file class by name
	/**
	* @param
	*    filename		file name
	* @return NXL file class
	*
	*/

	virtual bool RemoveFile(ISDRmNXLFile* file) = 0;
	/// Remove file by class reference
	/**
	* @param
	*    file		ISDRmNXLFile class reference
	* @return true if file removed success or return fail.
	*
	*/

};

#endif // !__ISDRFiles_H__