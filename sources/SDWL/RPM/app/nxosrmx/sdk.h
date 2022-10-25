#pragma once
#include <SDLAPI.h>   // nextlabs rm_sdk
#include "overlay.h"
#include "global_data_model.h"

#define NXRM_NAMESPACE namespace nx{ namespace rm{
#define END_NXRM_NAMESPACE }}


//
NXRM_NAMESPACE
//


typedef std::uint64_t RighMask;
#define	OSRMX_RIGHT_VIEW             0x00000001
#define	OSRMX_RIGHT_EDIT             0x00000002
#define	OSRMX_RIGHT_PRINT            0x00000004
#define	OSRMX_RIGHT_CLIPBOARD        0x00000008
#define	OSRMX_RIGHT_SAVEAS           0x00000010
#define	OSRMX_RIGHT_SCREENCAPTURE    0X00000020

#define OSRMX_RIGHT_OBLIGATION_VIEW_OVERLAY			0x10000000				
#define OSRMX_RIGHT_OBLIGATION_PRINT_OVERLAY		0x20000000


bool init_sdk();
bool init_sdk_for_adobe();
bool set_as_trused_process();
void notify_message(const std::wstring& msg);
void setwatermark(HWND hwnd, const nx::overlay::WatermarkParam param);
void clearwatermark(HWND hwnd);


bool is_nxl_file(const std::wstring& path);

bool add_print_log(const std::wstring& path, bool is_allow);

bool get_right_watermark(RighMask& mask, overlay::WatermarkParam& view_overlay, overlay::WatermarkParam& print_overlay);

bool is_nxlfile_in_rpm_folder(const std::wstring& path);

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
bool is_rpm_folder(const std::wstring& path, uint32_t& dir_status, SDRmRPMFolderOption& option, std::wstring& file_tags);

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
bool is_sanc_folder(const std::wstring& path, uint32_t& dir_status, std::wstring& file_tags);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

SDWLResult protect_file(const std::wstring& file_path, std::wstring& new_created_file_path, const std::vector<SDRmFileRight>& rights, const SDR_WATERMARK_INFO& watermark_info, const SDR_Expiration& expire, const std::string& tags = "");
#endif  // #if 0

//
END_NXRM_NAMESPACE
//