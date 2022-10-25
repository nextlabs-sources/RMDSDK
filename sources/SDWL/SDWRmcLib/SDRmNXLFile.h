/*!
 * \file SDRmNXLFile.h
 * \date 2018/04/20 12:09
 *
 * \author hbwang
 *
 /*/

#pragma once
#include "SDLNXlFile.h"
#include "SDLResult.h"
#include <string>

//RMCCore headers
#include "rmccore/network/httpConst.h"
#include "rmccore/restful/rmnxlfile.h"
#include "rmccore/restful/rmrecipients.h"
#include "rmccore/restful/rmtoken.h"

#include "Common/stringex.h"

#define DATA_KEY_NAME         "UserData"
#define TOKEN_KEY_NAME        "Token"

namespace SkyDRM {
	class CSDRmNXLFile :
		public ISDRmNXLFile, public RMCCORE::RMNXLFile
	{
	public:
		CSDRmNXLFile(const std::wstring &path);
		CSDRmNXLFile(const RMNXLFile & file);
		~CSDRmNXLFile();
	public:
		CSDRmNXLFile& operator = (const CSDRmNXLFile& rhs);

		const std::wstring GetFileName(void);
		const uint64_t GetFileSize(void);
		std::string	GetTenantName() const { return GetTenant(); }
		bool SetToken(const RMCCORE::RMToken &token);
		void ResetToken() { RemoveToken(); };
		
		std::string GetAdhocWaterMarkString()  const override;

		const RMCCORE::RMRecipients GetFileRecipients(void) { return m_recipients; }
		void SetFileRecipients(RMCCORE::RMRecipients &recipients) { m_recipients = recipients; }

        const std::string ExportDataToString(void);
		SDWLResult ImportDataFromString(std::string jsonstr);

		//bool IsRecipientsSynced();
		const SDR_WATERMARK_INFO GetWaterMark();
		const std::string	GetTags();
		const SDR_Expiration GetExpiration();
		bool CheckExpired(std::string userid = "");
		bool IsOwner(std::string membershipid);
		const std::string Get_CreatedBy() { return GetCreatedBy(); }
		const std::string Get_ModifiedBy() { return GetModifiedBy(); }
		const uint64_t Get_DateCreated(void) { return GetDateCreated(); }
		const uint64_t Get_DateModified(void) { return GetDateModified(); }
	public: // nxl file properties
		bool IsValidNXL(void);
		const std::vector<SDRmFileRight> GetRights(void);
		bool CheckRights(SDRmFileRight right);
		bool IsUploadToRMS(void);
		bool IsOpen();

	private:
		// RMCCORE::RMRecipients m_recipients; //keep a copy for offline modify purpose.
		//std::string m_key;
	};

	uint64_t ToFileRight(std::vector<SDRmFileRight> rights);
}


