#include "stdafx.h"
#include "common/celog2/celog.h"
#include "SDRPartialDownload.h"

#define CELOG_CUR_MODULE "rmdsdk"
#define CELOG_CUR_FILE CELOG_FILEPATH_SOURCES_SDWL_SDWRMCLIB_SDRPARTIALDOWNLOAD_CPP

using namespace SkyDRM;
using namespace NX;


SDRPartialDownload::SDRPartialDownload() : RMCCORE::NXLFMT::File()
{
}


SDRPartialDownload::~SDRPartialDownload()
{
}

SDWLResult SDRPartialDownload::BuildNxlHeader(const unsigned char* header, long len)
{
	CELOG_ENTER;
	SDWLResult res;
	errno_t err;
	RMCCORE::NXLFMT::FileError ferror;

	if (len < sizeof(NXL_HEADER2))
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header length is too small"));

	err = memcpy_s(&m_header, sizeof(NXL_HEADER2), header, sizeof(NXL_HEADER2));
	if (err)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header data is incorrect"));
	ferror = m_header.validate();
	if (FeSuccess != ferror)
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header data is incorrect"));

	SetHeader(m_header);

	if (len > sizeof(NXL_HEADER2)) {
		m_extraHeader.assign(header + sizeof(NXL_HEADER2), header + len);
	} else {
		m_extraHeader.clear();
	}

	CELOG_RETURN_VAL_T(res);
}

SDWLResult SDRPartialDownload::DecryptData(const unsigned char* in, long in_len, long offset, unsigned char* out, long* out_len)
{
	CELOG_ENTER;
	SDWLResult res;
	uint32_t blocksize = m_header.getBlockSize();
	long contentLength = in_len;
	uint32_t bytesToRead = 512;
	const unsigned char* buf = in;
	unsigned char* outbuf = out;
	long outlen = 0;
	

	if (0 != (offset % blocksize))
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "offset is invalid alignment"));

	if (offset >= (long)m_header.dynamic.contentLength)  
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "offset is bigger than content length"));

	while (contentLength > 0)
	{
		if (bytesToRead > (uint32_t)contentLength)
			bytesToRead = blocksize;

		if (!RMCCORE::CRYPT::AesDecrypt(m_tokenCek.data(), (uint32_t)m_tokenCek.size(), buf, bytesToRead, m_header.fixed.keys.ivSeed,
			offset, blocksize, outbuf, bytesToRead, &bytesToRead))
		{
			CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "decrypt data error"));
		}
		if ((offset + bytesToRead) > (uint64_t)m_header.dynamic.contentLength)
		{
			bytesToRead = (uint32_t)(m_header.dynamic.contentLength - offset);
			outlen += bytesToRead;
			break;
		}

		if (bytesToRead == 0)
			break;

		offset += bytesToRead;
		contentLength -= bytesToRead;
		buf += bytesToRead;
		outbuf += bytesToRead;
		outlen += bytesToRead;
	}
	*out_len = outlen;

	CELOG_RETURN_VAL_T(res);
}

SDWLResult SDRPartialDownload::GetPolicy(std::string& policy)
{
	CELOG_ENTER;
	CELOG_RETURN_VAL_T(GetSessionCommon(NXL_SECTION_NAME_FILEPOLICY, policy));
}

SDWLResult SDRPartialDownload::GetTags(std::string& tags)
{
	CELOG_ENTER;
	CELOG_RETURN_VAL_T(GetSessionCommon(NXL_SECTION_NAME_FILETAG, tags));
}

SDWLResult SDRPartialDownload::GetSessionCommon(const std::string& sectionName, std::string& str)
{
	CELOG_ENTER;
	NXL_SECTION2 *sectionInfo = m_header.findSection(sectionName);
	if (sectionInfo == nullptr) {
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "Section info not found in header"));
	}

	if (sectionInfo->offset + sectionInfo->size > sizeof(NXL_HEADER2) + m_extraHeader.size()) {
		CELOG_LOGA(CELOG_ERROR, "Header too short.  %s section offset=%u, size=%u.  Header size = %lu\n", sectionName.c_str(), sectionInfo->offset, sectionInfo->size, (unsigned long) (sizeof(NXL_HEADER2) + m_extraHeader.size()));
		CELOG_RETURN_VAL_T(RESULT2(SDWL_INVALID_DATA, "header too short"));
	}

	const auto it = m_extraHeader.begin() + (sectionInfo->offset - sizeof(NXL_HEADER2));
	str = std::string(it, it + sectionInfo->dataSize);

	CELOG_RETURN_VAL_T(RESULT(0));
}

void SDRPartialDownload::SetToken(const RMToken& filetoken)
{ 
	m_filetoken = filetoken; 
	std::string key = filetoken.GetKey();
	std::vector<uint8_t> tokenCek = NX::hstobin(key);
	setTokenKey(tokenCek);
	m_tokenCek = getEncryptKey();
}