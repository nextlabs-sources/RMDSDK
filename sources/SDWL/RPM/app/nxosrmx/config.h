#pragma once
/*
 Designed for all configuartions, i.e. String values, rights, parameters 
*/

#define NXCONFIG_NAMESPACE namespace nx{ namespace config{
#define END_NXCONFIG_NAMESPACE }}

//
NXCONFIG_NAMESPACE
//

const std::wstring msg_deny_using_clipboard = L"You do not have permission to perform the copy operation.";
const std::wstring msg_deny_using_print = L"You do not have permission to perform the print operation. ";
const std::wstring msg_deny_screen_capture = L"You do not have permission to perform the print screen operation.";
const std::wstring msg_deny_show_saveas_dialog = L"You do not have permission to perform the save as operation.";
const std::wstring msg_deny_common_msg = L"You don't have sufficient rights to perform this action.";


#ifdef _DEBUG

void load_debug_dev_configruaitons();  // for developing phase to easy alter params

#endif // _DEBUG

void load_app_whitelist_config();

//
END_NXCONFIG_NAMESPACE
//