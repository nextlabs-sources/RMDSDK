#include "SDRmNXLFile.h"
#include "assert.h"

using namespace SkyDRM;
using namespace NX;

#define TOKENKEY_ELEMENT_NAME                   "Key"

CSDRmNXLFile::CSDRmNXLFile(const std::wstring &filepath) : ISDRmNXLFile(), RMNXLFile(filepath)
{
}

CSDRmNXLFile::CSDRmNXLFile(const RMNXLFile & file) : ISDRmNXLFile(), RMNXLFile(file)
{
}

CSDRmNXLFile::~CSDRmNXLFile()
{
}

CSDRmNXLFile& CSDRmNXLFile::operator= (const CSDRmNXLFile& rhs)
{
	if (this != &rhs)
	{
		m_recipients = rhs.m_recipients;
	}

	return *this;
}

const std::wstring CSDRmNXLFile::GetFileName(void)
{
	return conv::utf8toutf16(RMNXLFile::GetFileName());
}

const uint64_t CSDRmNXLFile::GetFileSize()
{
	return size();
}

bool CSDRmNXLFile::SetToken(const RMCCORE::RMToken &token)
{
	return Open(token);
}

std::string CSDRmNXLFile::GetAdhocWaterMarkString() const
{
	for (const auto ob : const_cast<CSDRmNXLFile*>(this)->getObligations()) {
		if (ob.getName().compare(OBLIGATION_NAME_WATERMARK) == 0) {
			return ob.getArg("text");
		}
	}
	return std::string();
}

bool CSDRmNXLFile::IsValidNXL(void)
{
	return IsNXL();
}

uint64_t SkyDRM::ToFileRight(std::vector<SDRmFileRight> rights)
{
	uint64_t ret = 0;
	for (SDRmFileRight right : rights) {
		ret += right;
	}
	return ret;
}

const std::vector<SDRmFileRight> CSDRmNXLFile::GetRights(void)
{
	std::vector<SDRmFileRight> filerights;

	uint64_t rights = GetNXLRights();
	


	if (rights & RIGHT_VIEW)
		filerights.push_back(RIGHT_VIEW);
	if (rights & RIGHT_EDIT)
		filerights.push_back(RIGHT_EDIT);
	if (rights & RIGHT_PRINT)
		filerights.push_back(RIGHT_PRINT);
	if (rights & RIGHT_CLIPBOARD)
		filerights.push_back(RIGHT_CLIPBOARD);
	if (rights & RIGHT_SAVEAS)
		filerights.push_back(RIGHT_SAVEAS);
	if (rights & RIGHT_DECRYPT)
		filerights.push_back(RIGHT_DECRYPT);
	if (rights & RIGHT_SCREENCAPTURE)
		filerights.push_back(RIGHT_SCREENCAPTURE);
	if (rights & RIGHT_SEND)
		filerights.push_back(RIGHT_SEND);
	if (rights & RIGHT_CLASSIFY)
		filerights.push_back(RIGHT_CLASSIFY);
	if (rights & RIGHT_SHARE)
		filerights.push_back(RIGHT_SHARE);
	if (rights & RIGHT_DOWNLOAD)
		filerights.push_back(RIGHT_DOWNLOAD);

	return filerights;
}

bool  CSDRmNXLFile::CheckRights(SDRmFileRight right)
{
	uint64_t rights = GetNXLRights();
	return (rights & right) ? true : false;
}

bool  CSDRmNXLFile::IsUploadToRMS(void)
{
	return IsUploaded();
}

bool CSDRmNXLFile::IsOpen()
{
	return Open();
}

//bool CSDRmNXLFile::IsRecipientsSynced()
//{
//	if (IsUploaded() == false)
//		return false;
//
//	return (m_recipients.GetAddRecipients().size() + m_recipients.GetRemoveRecipients().size()) > 0 ? false : true;
//}

const std::string CSDRmNXLFile::ExportDataToString(void)
{
    std::string s;
    try {
        nlohmann::json root = nlohmann::json::object();
        root[DATA_KEY_NAME] = ExportToJson();
        s = root.dump();
        if (s.empty())
            throw RETMESSAGE(RMCCORE_ERROR_INVALID_DATA, "fail to export NXL File info to Json");

    }
    catch (...) {
        // The JSON data is NOT correct
        throw RETMESSAGE(RMCCORE_ERROR_INVALID_DATA, "Export NXL File Info Json error");
    }

    return s;
}

SDWLResult CSDRmNXLFile::ImportDataFromString(std::string jsonstr)
{
    SDWLResult res = RESULT(0);

    try {

        nlohmann::json root = nlohmann::json::parse(jsonstr);
        if (!root.is_object())
        {
            return RESULT2(SDWL_INVALID_JSON_FORMAT, "fail to load data from tenant file!");
        }

        const nlohmann::json userData = root.at(DATA_KEY_NAME);
        ImportFromJson(userData);

        bool val = Open();
    }
    catch (...) {
        // The JSON data is NOT correct
        res = RESULT2(ERROR_INVALID_DATA, "JSON response is not correct");
    }

    return res;
}

const SDR_WATERMARK_INFO CSDRmNXLFile::GetWaterMark() {
	SDR_WATERMARK_INFO info;
	RMNXLFile * nxlfile = (RMNXLFile *)this;
	Obligations obs = nxlfile->getObligations();
	for (Obligation ob : obs)
	{
		if (ob.getName().compare(OBLIGATION_NAME_WATERMARK) == 0)
		{
			info.text = ob.getArg("text");
			info.fontName = ob.getArg("FontName");

			try
			{
				info.fontSize = std::stoi(ob.getArg("FontSize"));
			}
			catch (...) {
				info.fontSize = 0;
			}

			if (ob.getArg("FontColor").size())
				info.fontColor = ob.getArg("FontColor");

			try
			{
				info.repeat = std::stoi(ob.getArg("Repeat")) ? true : false;
			}
			catch (...) {
				info.repeat = false;
			}

			try
			{
				info.rotation = (WATERMARK_ROTATION)std::stoi(ob.getArg("Rotation"));
			}
			catch (...) {
				info.rotation = NOROTATION;
			}

			try
			{
				info.transparency = std::stoi(ob.getArg("Transparency"));
			}
			catch (...) {
				info.transparency = 0;
			}
		}
	}
	return info;
}

const std::string CSDRmNXLFile::GetTags() {
	return ((RMNXLFile *)this)->GetTags();
}

const SDR_Expiration CSDRmNXLFile::GetExpiration() 
{
	SDR_Expiration expiration;
	RMNXLFile * nxlfile = (RMNXLFile *)this;
	if (!nxlfile->GetNXLExpiration())
		return expiration;
	RMCCORE::CONDITION::Expiry expiry = nxlfile->GetExpiration();
	expiration.type = (IExpiryType)expiry.getType();
	expiration.start = expiry.getStartDate();
	expiration.end = expiry.getEndDate();
	return expiration;
}

bool CSDRmNXLFile::CheckExpired(std::string userid)
{
	//if (IsOwner(userid)) // for creator, he might not be the owner with full rights in case file is in project/workspace; -- Raymond
	//	return false;

	SDR_Expiration expiration = GetExpiration();
	if (expiration.type != NEVEREXPIRE) {
		uint64_t nowTime = (uint64_t)RMCCORE::NXTime::nowTime() * 1000;
		if (expiration.type == RANGEEXPIRE) {
			if (IsOwner(userid))
				return !(expiration.end >= nowTime); // fix 61802; for range time, if creator, just check the end time
			else
				return !(expiration.start <= nowTime && expiration.end >= nowTime);
		}

		if (expiration.type == RELATIVEEXPIRE || expiration.type == ABSOLUTEEXPIRE) {
			return nowTime > expiration.end;
		}
	}
	return false;

}

bool CSDRmNXLFile::IsOwner(std::string membershipid)
{
	std::string	 ownerid = GetOwnerID();
	if (ownerid.compare(membershipid) != 0)
		return false;
	else
		return true;
}

