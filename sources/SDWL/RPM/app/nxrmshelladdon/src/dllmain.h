// dllmain.h : Declaration of module class

#include <string>
#include "SDLTypeDef.h"

class CnxrmshelladdonModule : public ATL::CAtlDllModuleT< CnxrmshelladdonModule >
{
public:
	DECLARE_LIBID(LIBID_nxrmshelladdonLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_NXRMSHELLADDON, "{3b8687f5-332a-4794-9fd6-bc9e23bfb8be}")
};

extern class CnxrmshelladdonModule _AtlModule;

extern BOOL g_isDllhost;
extern BOOL g_otherProcess;

typedef struct _RPMFolderRelation
{
	bool bUnknownRelation : 1;
	bool bNormalFolder : 1;

	// For RPM folder
	struct {
		bool bRPMAncestralFolder : 1;
		bool bRPMFolder : 1;
		bool bRPMInheritedFolder : 1;
	};

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	// For Sanctuary folder
	struct {
		bool bSanctuaryAncestralFolder : 1;
		bool bSanctuaryFolder : 1;
		bool bSanctuaryInheritedFolder : 1;
	};
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	wchar_t BoolToWchar(bool b) const {
		return b ? L't' : L'f';
	}
	std::wstring ToString(void) const {
		return std::wstring() +
			L'{' +
			BoolToWchar(bUnknownRelation) + L',' +
			BoolToWchar(bNormalFolder) + L',' +
			L'{' +
			BoolToWchar(bRPMAncestralFolder) + L',' +
			BoolToWchar(bRPMFolder) + L',' +
			BoolToWchar(bRPMInheritedFolder) +
			L'}' +
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			L',' + L'{' +
			BoolToWchar(bSanctuaryAncestralFolder) + L',' +
			BoolToWchar(bSanctuaryFolder) + L',' +
			BoolToWchar(bSanctuaryInheritedFolder) +
			L'}' +
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			L'}';
	}
} RPMFolderRelation;

typedef enum _RPMFileRelation
{
	UnknownRelation,
	NXLFileInNormalDir,
	NonNXLFileInNormalDir,
	NXLFileInRPMDir,
	NonNXLFileInRPMDir,
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	NXLFileInSanctuaryDir,
	NonNXLFileInSanctuaryDir,
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	RPMFileRelationMax
} RPMFileRelation;

typedef union _RPMRelation
{
	RPMFolderRelation   folder;
	RPMFileRelation     file;
} RPMRelation;

#ifdef _DEBUG
void AssertRPMFolderRelation(RPMFolderRelation r);
void AssertRPMFileRelation(RPMFileRelation r);
void AssertRPMRelation(bool isFolder, RPMRelation r);
#endif

RPMFolderRelation GetFolderRelation(const std::wstring& folderPath, SDRmRPMFolderOption *pRpmFolderOption = NULL, std::wstring *pFileTags = NULL);
RPMFolderRelation GetFolderRelationUsingParent(const std::wstring& folderPath, SDRmRPMFolderOption *pRpmFolderOption = NULL, std::wstring *pFileTags = NULL);
