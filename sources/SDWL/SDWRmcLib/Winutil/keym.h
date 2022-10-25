
#pragma once

#include <SDLResult.h>
#include <string>

#include "..\Crypt\cert.h"

namespace NX {

    class RmcKeyManager
    {
    public:
        RmcKeyManager();
        RmcKeyManager(const RmcKeyManager& rhs);
        ~RmcKeyManager();

        inline bool Empty() const { return _id.empty(); }
        inline const std::wstring& GetClientId() const { return _id; }
        inline const std::vector<UCHAR>& GetClientKey() const { return _key; }

        SDWLResult Create(bool machineStore = false);
		SDWLResult Load(bool machineStore = false, bool createifnone = false);
        RmcKeyManager& operator = (const RmcKeyManager& rhs);

    private:
		SDWLResult CreateCertificate(crypt::CertContext* pcc, bool machineStore = false);
		SDWLResult FindCertificate(crypt::CertContext* pcc, bool machineStore = false);
		SDWLResult GenerateIdAndKey(const std::vector<UCHAR>& key1, const std::vector<UCHAR>& key2);

    private:
        std::wstring  _id;
        std::vector<UCHAR> _key;
    };

	extern const UCHAR DH_P_2048[256];
	extern const UCHAR DH_G_2048[256];
	extern const UCHAR DH_P_1024[128];
	extern const UCHAR DH_G_1024[128];

	SDWLResult GetDHParameterY(const std::wstring& cert, std::vector<UCHAR>& y);
}
