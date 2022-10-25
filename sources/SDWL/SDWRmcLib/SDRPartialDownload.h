#pragma once
/*!
* \file SDRPartialDownload.h
* \date 2019/02/19 
*
* \author stsai
*
/*/

#pragma once
#include "SDLNXlFile.h"
#include "SDLResult.h"
#include <string>

//RMCCore headers
#include "rmccore/network/httpConst.h"
#include "rmccore/format/nxlfile.h"
#include "rmccore/restful/rmnxlfile.h"
#include "rmccore/restful/rmrecipients.h"
#include "rmccore/restful/rmtoken.h"
#include "rmccore/crypto/crypto.h"
#include "rmccore/crypto/aes.h"

#include "Common/stringex.h"
#include "SDRNXLToken.h"


namespace SkyDRM {
	class SDRPartialDownload : public RMCCORE::NXLFMT::File
	{
	public:
		SDRPartialDownload();
		~SDRPartialDownload();

		int64_t GetContentLength() { return m_header.dynamic.contentLength; };
		SDWLResult BuildNxlHeader(const unsigned char* header, long len);
		SDWLResult DecryptData(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len);
		void GetHeader(RMCCORE::NXLFMT::Header& header) { header = m_header; };
		SDWLResult GetPolicy(std::string& policy);
		SDWLResult GetTags(std::string& policy);
		void SetToken(const RMToken& filetoken);

	private:
		SDWLResult GetSessionCommon(const std::string& sectionName, std::string& str);

	private:
		RMCCORE::NXLFMT::Header m_header;
		std::vector<unsigned char> m_extraHeader;
		RMToken m_filetoken;
		std::vector<uint8_t> m_tokenCek;
	};

}
