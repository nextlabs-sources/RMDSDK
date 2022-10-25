#include "SDRNXLToken.h"
#include "time.h"
#include "rmccore\network\httpClient.h"


using namespace SkyDRM;
using namespace RMCCORE;

#define DEFAULT_TOKEN_EXPIRED_DAYS	15

#define TOKENCACHE_KEY_NAME         "TokenList"
#define TOKENCACHE_TOKEN_KEY_NAME	"Token"
#define TOKENCACHE_TTL_ELEMENT_NAME	"Ttl"
#define TOKENCACHE_USERID_ELEMENT_NAME	"UserID"

SDRNXLToken::SDRNXLToken() : m_userid("")
{
	m_tokencache.clear();
}


SDRNXLToken::~SDRNXLToken()
{
}

void SDRNXLToken::RemoveToken(const std::string &duid)
{
	m_tokencache.erase(duid);
}

void SDRNXLToken::AddToken(RMToken& token, time_t ttl)
{
	std::string duid = token.GetDUID();
	time_t curtime = time(NULL);
	struct tm  tm;
	localtime_s(&tm, &curtime);
	tm.tm_mday += DEFAULT_TOKEN_EXPIRED_DAYS;
	time_t exptime = mktime(&tm);
	TokenCache tc;
	tc.token = token;
	if (ttl == 0)
		tc.ttl = exptime;
	else
		tc.ttl = ttl;

	m_tokencache[duid] = tc;
}

bool SDRNXLToken::FindToken(const std::string &duid, RMToken & token)
{
	bool bfound = false;

	if (m_tokencache.find(duid) != m_tokencache.end())
	{
		time_t curtime = time(NULL);
		if (curtime <= m_tokencache[duid].ttl) {
			token = m_tokencache[duid].token;
			bfound = true;
		}
	}

	return bfound;
}

#ifdef USE_UTILS_JSON
const std::string SDRNXLToken::ExporttoJsonString(void)
{
	std::string s;

	try {
		JsonStringWriter writer;
		s = writer.Write(ExportToJson());
		if (s.empty())
			throw RETMESSAGE(RMCCORE_INVALID_JSON_FORMAT, "fail to export membership info to Json");

	}
	catch (const RetValue & e) {
		throw e;
	}
	catch (...) {
		// The JSON data is NOT correct
		throw RETMESSAGE(RMCCORE_ERROR_INVALID_DATA, "Export membership Json error");
	}

	return s;
}

SDWLResult SDRNXLToken::ImportFromJsonString(const std::string &jsonstr)
{
	JsonDocument doc;
	if (!doc.LoadJsonString(jsonstr))
		return RESULT2(SDWL_INVALID_JSON_FORMAT, "fail to load data from token file!");

	JsonValue* root = doc.GetRoot();

	return ImportFromJson(root->AsObject());
}

SDWLResult SDRNXLToken::ImportFromJson(JsonObject * pvalue)
{
	SDWLResult res;

	try {
		std::string userid;
		userid = pvalue->at(TOKENCACHE_USERID_ELEMENT_NAME)->AsString()->GetString();
		if (userid.compare(m_userid))//user id is not match
			return RESULT2(SDWL_INVALID_JSON_FORMAT, "userid is not match");
		if (pvalue->at(TOKENCACHE_KEY_NAME)->AsArray())
		{
			JsonArray * tokenlist = pvalue->at(TOKENCACHE_KEY_NAME)->AsArray();

			std::for_each(tokenlist->cbegin(), tokenlist->cend(), [&](const JsonArray::value_type & item) {
				RMToken token;
				uint64_t ttl;
				JsonObject * obj = item->AsObject()->at(TOKENCACHE_TOKEN_KEY_NAME)->AsObject();
				token.ImportFromJson(obj);
				ttl = item->AsObject()->at(TOKENCACHE_TTL_ELEMENT_NAME)->AsNumber()->ToUint64();
				TokenCache tc;
				tc.token = token;
				tc.ttl = ttl;

				m_tokencache[token.GetDUID()] = tc;
			});
		}
	}
	catch (...) {
		//something wrong with the input. keep imported tokens
		res = RESULT2(ERROR_INVALID_DATA, "JSON is not correct");
	}
	return res;
}

JsonValue * SDRNXLToken::ExportToJson(void)
{
	JsonValue * nxltoken = JsonValue::CreateObject();
	try {
		nxltoken->AsObject()->set(TOKENCACHE_USERID_ELEMENT_NAME, JsonValue::CreateString(m_userid));

		JsonArray * tokenlist = JsonValue::CreateArray();
		nxltoken->AsObject()->set(TOKENCACHE_KEY_NAME, tokenlist);
		for (auto tc : m_tokencache) {
			JsonObject *token = JsonValue::CreateObject();
			token->set(TOKENCACHE_TOKEN_KEY_NAME, tc.second.token.ExportToJson());
			token->set(TOKENCACHE_TTL_ELEMENT_NAME, JsonValue::CreateNumber(tc.second.ttl));
			tokenlist->push_back(token);
		}
	}

	catch (...) {
		//something wrong with the object.
		return NULL;
	}
	return nxltoken;
}

JsonValue * SDRNXLToken::ExportCachedTokenToJson(void) {
	JsonValue * cachedToken = JsonValue::CreateObject();
	try {
		JsonArray * tokenlist = JsonValue::CreateArray();
		cachedToken->AsObject()->set(TOKENCACHE_KEY_NAME, tokenlist);
		for (auto tc : m_tokencache) {
			tokenlist->push_back(tc.second.token.ExportToJson());
		}
	}

	catch (...) {
		//something wrong with the object.
		return NULL;
	}
	return cachedToken;
}

JsonValue * SDRNXLToken::ExportMTokenToJson(RMToken &token) {
	JsonValue * cachedToken = JsonValue::CreateObject();
	try {
		JsonArray * tokenlist = JsonValue::CreateArray();
		cachedToken->AsObject()->set(TOKENCACHE_KEY_NAME, tokenlist);
		tokenlist->push_back(token.ExportToJson());
	}
	catch (...) {
		//something wrong with the object.
		return NULL;
	}
	return cachedToken;
}

#endif