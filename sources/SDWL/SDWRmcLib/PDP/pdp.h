#pragma once
#ifndef __NX_PDP_PDP_H__
#define __NX_PDP_PDP_H__

#include <shared_mutex>
#include <atomic>
#include "CEsdk.h"
#include "SDLResult.h"
#include "SDLNXLFile.h"
#include "SDLUser.h"

namespace NX {

    class SDWPDP
    {
    public:
        SDWPDP(void) : m_pdpProcessHandle(NULL), m_connHandle(NULL), m_needToReconnect(false) {};
        ~SDWPDP(void);

    private:
        SDWLResult ConnectPDPMan(void);
        SDWLResult ConnectPDPManNoLock(void);
        SDWLResult DisconnectPDPMan(void);
        SDWLResult DisconnectPDPManNoLock(void);
        std::vector<SDR_WATERMARK_INFO> SDWPDP::GetWatermarkObsFromEvalResult(
            CEAttributes& obligations, const std::wstring& useremail,
            const std::wstring& userID, const std::vector<std::pair<std::wstring, std::wstring>>& rAttrs, const std::vector<std::pair<std::wstring, std::wstring>>& uAttrs,bool doOwnerCheck);
        std::vector<SDR_OBLIGATION_INFO> SDWPDP::GetGenericObsFromEvalResult(
            CEAttributes& obligations);

		SDWLResult ParsingWaterMark(const std::wstring &userDispName, const std::wstring &userID,
			const std::vector<std::pair<std::wstring, std::wstring>> &fsoattrs, const std::vector<std::pair<std::wstring, std::wstring>> &uattrs,
			std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> &rightsAndWatermarks);
	public:
        SDWLResult Initialize(const std::wstring& pdpDir);
		bool IsPDPManRunning(void);
		SDWLResult IsReadyForEval(bool &ready);
		SDWLResult ensurePDPConnectionReady(const std::wstring &pdpDir, bool &ready);
        SDWLResult StartPDPMan(void);
        SDWLResult StopPDPMan(void);
        SDWLResult EvalRights(const std::wstring &userDispName,
			const std::wstring &useremail,
            const std::wstring &userID,
            const std::wstring &appPath,
            const std::wstring &resourceName,
            const std::wstring &resourceType,
            const std::vector<std::pair<std::wstring, std::wstring>> &rAttrs,
            const std::vector<std::pair<std::wstring, std::wstring>> &uAttrs,
            const std::wstring &bundle,
            std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>> *rightsAndWatermarks,
            std::vector<std::pair<SDRmFileRight, std::vector<SDR_OBLIGATION_INFO>>> *rightsAndObligations, bool doOwnerCheck = true);

    private:
        std::wstring        m_pdpDir;
        HANDLE              m_pdpProcessHandle;
		DWORD				m_pdpProcessId;

        // Both m_connHandle and m_needToReconnect are protected by the mutex
        // m_connHandleMtx.
        // - m_connHandle can be accessed only after shared-locking.  It can
        //   be modified only after exclusive-locking.
        // - m_needToReconnect can be set only after shared-locking.  It can
        //   be cleared only after exclusive-locking.
        std::shared_mutex   m_connHandleMtx;
        CEHandle            m_connHandle;
        std::atomic_bool    m_needToReconnect;
    };

}

#endif // __NX_PDP_PDP_H__
