
/*!
* \file SDRNXLToken.h
*
* \author hwang
* \date 2018/05/08
*
*/

#pragma once
#include "SDLResult.h"

//RMCCore headers
#include "rmccore/restful/rmtoken.h"
#include "Common/stringex.h"
#include <unordered_map>


using namespace RMCCORE;
namespace SkyDRM {
	class SDRNXLToken
	{
	public:
		SDRNXLToken();
		~SDRNXLToken();
	public:
		void Initialize(std::string userid) { m_userid = userid; }
		void AddToken(RMToken& token, time_t ttl = 0);
		void RemoveToken(const std::string &duid);
		bool FindToken(const std::string &duid, RMToken &token);

#ifdef USE_UTILS_JSON
		const std::string ExporttoJsonString(void);
		SDWLResult ImportFromJsonString(const std::string & jsonstr);
		JsonValue * ExportCachedTokenToJson(void);
		JsonValue * ExportMTokenToJson(RMToken &token);
#endif

	private:
#ifdef USE_UTILS_JSON
		JsonValue * ExportToJson(void);
		SDWLResult ImportFromJson(JsonObject * obj);
#endif

	private:
		typedef struct{
			RMToken token;
			time_t ttl;
		}TokenCache;
		std::string m_userid;

		std::unordered_map<std::string, TokenCache> m_tokencache;
	};
}

