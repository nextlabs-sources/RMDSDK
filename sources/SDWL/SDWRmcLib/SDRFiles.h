#pragma once
/*!
* \file SDRFiles.h
*
* \author stsai
* \date 2018/04/19
*
*/
#ifndef __SDRFiles_H__
#define __SDRFiles_H__

#pragma once
#include "SDLFiles.h"
#include "SDLResult.h"

//RMCCore headers
#include "rmccore/restful/rmuser.h"
#include "rmccore/restful/rmtoken.h"
#include "rmccore/restful/rmmembership.h"
#include "rmccore/format/nxlfmt.h"
#include "rmccore/format/nxlfile.h"

#include "Common/stringex.h"
#include "SDRSecureFile.h"
#include "SDRmNXLFile.h"


namespace SkyDRM {
	class SDRFiles : 
		public ISDRFiles,
		protected RMCCORE::NXLFMT::File
	{
	public:
		SDRFiles() : ISDRFiles(), RMCCORE::NXLFMT::File() {};
		~SDRFiles();
		std::vector<std::wstring> InitFileList();
		SDRFiles& operator = (const SDRFiles& rhs) {
			if (this != &rhs)
			{
				m_workFolder = rhs.m_workFolder;
				m_nxlFleList = rhs.m_nxlFleList;
			}
			return *this;
		}

	public: // impl ISDRFiles
		virtual inline size_t GetListNumber(void) const { return m_nxlFleList.size(); };
		virtual std::vector<std::wstring> GetList() const ;
		virtual ISDRmNXLFile* GetFile(size_t index) const ;
		virtual ISDRmNXLFile* GetFile(const std::wstring& filepath)const ;
		virtual bool RemoveFile(ISDRmNXLFile* file);
		
	public :
		inline void AddFile(const CSDRmNXLFile&  file) { m_nxlFleList.push_back(file); };
		inline std::wstring GetWorkFolder(void) const { return m_workFolder; }		
		inline void SetWorkFolder(const std::wstring& workFolder) { m_workFolder = workFolder; };

	private:
		std::wstring m_workFolder;
		std::list<CSDRmNXLFile> m_nxlFleList;
	};

}
#endif // !__SDRFiles_H__
