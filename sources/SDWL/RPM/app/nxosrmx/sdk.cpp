#include "stdafx.h"
#include <chrono>
#include <iomanip>
#include "sdk.h"
#include <chrono>
#include "global_data_model.h"
#include <Windows.h>

//
NXRM_NAMESPACE
//
// SDK required hard coded;
std::string sdk_code{ "{6829b159-b9bb-42fc-af19-4a6af3c9fcf6}" };

ISDRmcInstance* pInstance = NULL;
ISDRmTenant* pTenant = NULL;
ISDRmUser* puser = NULL;


 bool init_sdk() {
	SDWLResult res = RPMGetCurrentLoggedInUser(sdk_code, pInstance, pTenant, puser);
	return 0 == res.GetCode();
}

 bool init_sdk_for_adobe() {
	 SDWLResult res = SDWLibCreateInstance(&pInstance);
	 if (0 == res.GetCode())
		 pInstance->RPMSwitchTransport(1); // use shared memory

	 return 0 == res.GetCode();
 }

 bool set_as_trused_process() {
	SDWLResult  res = pInstance->RPMNotifyRMXStatus(true, sdk_code);
	if (res.GetCode() != 0) {
		std::string strMsg = "??? RPMNotifyRMXStatus return code : " + std::to_string(res.GetCode()) + " error message : " + res.GetMsg().c_str() + "\n";
		::OutputDebugStringA(strMsg.c_str());
		return false;
	}
	res = pInstance->RPMAddTrustedProcess(::GetCurrentProcessId(), sdk_code);
	if (res.GetCode() != 0) {
		std::string strMsg = "??? RPMAddTrustedProcess return code : " + std::to_string(res.GetCode()) + " error message : " + res.GetMsg().c_str() + "\n" ;
		::OutputDebugStringA(strMsg.c_str());
		return false;
	}

	return true;
}



 bool calc_minimum_right(
	 const std::map<std::wstring, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>>& mapFileRights
	 , uint64_t& u64Right, SDR_WATERMARK_INFO& watermarkView, SDR_WATERMARK_INFO& watermarkPrint)
 {
	 bool bSetFirstWatermark = false;
	 bool bSetViewWatermark = false;
	 bool bSetPrintWatermark = false;
	 uint64_t u64MinRight = 0xffffffffffffffff;

	 for (auto item : mapFileRights)
	 {
		 uint64_t u64FileRight = 0;
		 for (auto right : item.second)
		 {
			 u64FileRight |= right.first;

			 if (!bSetFirstWatermark && (right.second.size() > 0))
			 {
				 bSetFirstWatermark = true;
				 watermarkView = right.second.at(0);
				 watermarkPrint = right.second.at(0);
			 }

			 if (!bSetViewWatermark && (right.second.size() > 0) && (right.first & RIGHT_VIEW))
			 {
				 bSetViewWatermark = true;
				 watermarkView = right.second.at(0);
			 }

			 if (!bSetPrintWatermark && (right.second.size() > 0) && (right.first & RIGHT_PRINT))
			 {
				 bSetPrintWatermark = true;
				 watermarkPrint = right.second.at(0);
			 }
		 }

		 u64MinRight = u64MinRight & u64FileRight;
	 }

	 u64Right = u64MinRight;

	 return bSetFirstWatermark;
 }

 inline std::wstring utf82utf16(const std::string& str) {
	 if (str.empty())
	 {
		 return std::wstring();
	 }
	 int num_chars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
	 std::wstring wstrTo;
	 if (num_chars)
	 {
		 wstrTo.resize(num_chars + 1);
		 if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstrTo[0], num_chars))
		 {
			 wstrTo = std::wstring(wstrTo.c_str());
			 return wstrTo;
		 }
	 }
	 return std::wstring();
 }


 class KeywordExpandable {
 public:
	 KeywordExpandable() { _default(); }
	 KeywordExpandable(const std::wstring& text) :_text(text) { _default(); }

	 inline void InsertExpandableToken(const std::wstring& key, const std::wstring& value) {
		 _tokens[key] = value;
	 }

	 std::wstring GetExpandableText() {
		 std::wstring rt = _text;
		 std::wstring tmp(rt.size(),0); 
		 std::transform(rt.begin(), rt.end(), tmp.begin(), std::tolower);
		 std::for_each(_tokens.cbegin(), _tokens.cend(), [&rt,&tmp](decltype(_tokens)::const_reference pair) {
			 //boost::ireplace_all(rt, pair.first, pair.second);
			 std::wstring t = pair.first;
			 std::transform(t.begin(), t.end(), t.begin(), std::tolower);
			 size_t pos = 0;
			 while ( (pos=tmp.find(t,pos))!= std::wstring::npos)
			 {
				 tmp.replace(pos, t.size(), pair.second.c_str(), pair.second.size());
				 rt.replace(pos, t.size(), pair.second.c_str(), pair.second.size());
				 pos++;
			 }
			 });
		 return rt;
	 }

 private:
	 void _default() {
		 const static wchar_t* USER = L"$(User)";
		 const static wchar_t* EMAIL = L"$(Email)";
		 const static wchar_t* HOST = L"$(Host)";
		 const static wchar_t* IP = L"$(IP)";
		 const static wchar_t* BREAK = L"$(Break)";  //  to \n
		 const static wchar_t* DATE = L"$(DATE)";    //  "YYYY-MM-DD"
		 const static wchar_t* TIME = L"$(TIME)";	//  "HH:mm:ss"	  

		 // TBD
		 //_tokens[USER] = L"";
		 //_tokens[EMAIL] = L"";
		 //_tokens[HOST] = L"";
		 //_tokens[IP] = L"";
		 // Cur 
		 _tokens[BREAK] = L"\n";
		 _tokens[DATE] = _get_date();
		 _tokens[TIME] = _get_time();
	 }

	 inline std::wstring _get_date() {
		 std::wstringstream ss;
		 auto time = std::time(nullptr);
		 ss << std::put_time(std::localtime(&time), L"%Y-%m-%d");
		 return ss.str();
	 }

	 inline std::wstring _get_time() {
		 std::wstringstream ss;
		 auto time = std::time(nullptr);
		 ss << std::put_time(std::localtime(&time), L"%H:%M:%S");
		 return ss.str();
	 }
	 inline std::wstring _get_datetime() {
		 std::wstringstream ss;
		 auto time = std::time(nullptr);
		 ss << std::put_time(std::localtime(&time), L"%Y-%m-%d %H:%M:%S");
		 return ss.str();
	 }

 private:
	 std::wstring _text;
	 // i.e.  $(User) ->  Osmond.Ye
	 std::map<std::wstring, std::wstring> _tokens;
 };

 inline void convert(overlay::WatermarkParam& to, SDR_WATERMARK_INFO& from) {
	 overlay::WatermarkParamBuilder builder;
	 
	 // insert new feature, in convert, expand the expandable text
	 KeywordExpandable KE(utf82utf16(from.text));
	 if (puser)
	 {
		 KE.InsertExpandableToken(L"$(User)", puser->GetEmail());
		 KE.InsertExpandableToken(L"$(Email)", puser->GetEmail());
	 }
	 else if (global.ex_user_email.size() > 0)
	 {
		 KE.InsertExpandableToken(L"$(User)", global.ex_user_email);
		 KE.InsertExpandableToken(L"$(Email)", global.ex_user_email);
	 }
	 // others like break, date,time, using default

	 builder
		 //.font_name(utf82utf16(from.fontName))
		 .font_size(from.fontSize)
		 .text_color(70, 0, 128, 21)  // using default
		 .text(KE.GetExpandableText());
		
	 if (from.rotation == CLOCKWISE) {
		 builder.text_rotation(-45);
	 }
	 else if(from.rotation == ANTICLOCKWISE) {
		 builder.text_rotation(45);
	 }
	 to = builder.Build();
 }

 void print_watermark(const SDR_WATERMARK_INFO& watermark)
 {
	 std::string text = "	text : " + watermark.text + "\n";
	 std::string fontname = "	fontname : " + watermark.fontName + "\n";
	 std::string fontcolor = "	fontcolor : " + watermark.fontColor + "\n";
	 std::string fontsize = "	fontsize : " + std::to_string(watermark.fontSize) + "\n";
	 std::string transparency = "	transparency : " + std::to_string(watermark.transparency) + "\n";
	 std::string rotation = "	rotation : " + std::to_string(watermark.rotation) + "\n";
	 std::string repeat = "	repeat : true \n";
	 if (!watermark.repeat)
	 {
		 repeat = "	repeat : false \n";
	 }

	 ::OutputDebugStringA(text.c_str());
	 ::OutputDebugStringA(fontname.c_str());
	 ::OutputDebugStringA(fontcolor.c_str());
	 ::OutputDebugStringA(fontsize.c_str());
	 ::OutputDebugStringA(transparency.c_str());
	 ::OutputDebugStringA(rotation.c_str());
	 ::OutputDebugStringA(repeat.c_str());
 }

 void print_right_watermark(uint64_t u64Rights, const SDR_WATERMARK_INFO& watermarkView, const SDR_WATERMARK_INFO& watermarkPrint)
 {
	::OutputDebugStringW(L"**********enter rights, view watermark, print watermark**********\n");

	std::wstring strRight = L"right: " + std::to_wstring(u64Rights) + L"\n";
	::OutputDebugStringW(strRight.c_str());

	std::wstring strWatermarkView = L"view watermark : \n";
	::OutputDebugStringW(strWatermarkView.c_str());
	print_watermark(watermarkView);

	std::wstring strWatermarkPrint = L"\nprint watermark : \n";
	::OutputDebugStringW(strWatermarkPrint.c_str());
	print_watermark(watermarkPrint);

	::OutputDebugStringW(L"**********leave rights, view watermark, print watermark**********\n");
 }

 bool get_right_watermark(RighMask& mask, overlay::WatermarkParam& view_overlay, overlay::WatermarkParam& print_overlay)
 {
	 std::map<std::wstring, std::vector<std::pair<SDRmFileRight, std::vector<SDR_WATERMARK_INFO>>>> mapOpenedFileRights;
	 std::wstring _useremail;
	 SDWLResult result = pInstance->RPMGetOpenedFileRights(mapOpenedFileRights, sdk_code, _useremail, 0);
	 if (0 != result.GetCode()) {
		 std::string strMsg = "RPMGetOpenedFileRights return code: " 
			 + std::to_string(result.GetCode()) + " message: " + result.GetMsg() + "\n";
		 ::OutputDebugStringA(strMsg.c_str());
		 return false;
	 }

	 if (global.ex_user_email.size() <= 0)
		 global.ex_user_email = _useremail;

	 if (mapOpenedFileRights.empty())
	 {
		 return false;
	 }

	 std::uint64_t u64Right = 0;
	 SDR_WATERMARK_INFO inter_view, inter_print;

	 calc_minimum_right(mapOpenedFileRights, u64Right, inter_view, inter_print);
	 //if (!calc_minimum_right(mapOpenedFileRights, u64Right, inter_view, inter_print))
	 //{
		// ::OutputDebugStringW(L"calc_minimum_right return false \n");
		// return false;
	 //}
	 print_right_watermark(u64Right, inter_view, inter_print);

	 // convert into os_rmx compatible format
	 if (u64Right & RIGHT_VIEW) {
		 mask |= OSRMX_RIGHT_VIEW;
	 }

	 if (u64Right & RIGHT_EDIT) {
		 mask |= OSRMX_RIGHT_EDIT;
	 }

	 if (u64Right & RIGHT_PRINT) {
		 mask |= OSRMX_RIGHT_PRINT;
	 }

	 if (u64Right & RIGHT_CLIPBOARD) {
		 mask |= OSRMX_RIGHT_CLIPBOARD;
	 }

	 if (u64Right & RIGHT_DECRYPT) {
		 mask |= OSRMX_RIGHT_CLIPBOARD;
		 mask |= OSRMX_RIGHT_SCREENCAPTURE;
	 }

	 if (u64Right & RIGHT_SCREENCAPTURE) {
		 mask |= OSRMX_RIGHT_SCREENCAPTURE;
	 }

	 if (global.flag_draw_watermark) 
	 {
		 //
		 // next version may need others
		 //
		 if (!inter_view.text.empty()) {
			 mask |= OSRMX_RIGHT_OBLIGATION_VIEW_OVERLAY;
			 convert(view_overlay, inter_view);
		 }
	 }

	 if (!inter_print.text.empty()) {
		 mask |= OSRMX_RIGHT_OBLIGATION_PRINT_OVERLAY;
		 convert(print_overlay, inter_print);
	 }

	 return true;
 }

 bool is_nxlfile_in_rpm_folder(const std::wstring& path) {
	 bool ret = false;

	 unsigned int dirStatus = 0;
	 bool fileStatus = false;
	 auto res = pInstance->RPMGetFileStatus(path, &dirStatus, &fileStatus);
	 if (res.GetCode() == 0)
	 {
		 if (dirStatus & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)) {
			 ret = fileStatus;
		 }
	 }

	 return ret;
 }

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
 bool is_rpm_folder(const std::wstring& path, uint32_t& dir_status, SDRmRPMFolderOption& option, std::wstring& file_tags) {
	 auto res = pInstance->IsRPMFolder(path, &dir_status, &option, file_tags);
	 return res;
 }

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

 bool is_sanc_folder(const std::wstring& path, uint32_t& dir_status, std::wstring& file_tags) {
	 auto res = pInstance->IsSanctuaryFolder(path, &dir_status, file_tags);
	 return res;
 }

#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
#endif  // #if 0

 void notify_message(const std::wstring& msg) {
	 // if same message is sent in 1 second, we will not show it again
	 static std::wstring lastmsg;
	 static std::chrono::milliseconds lastms = std::chrono::duration_cast<std::chrono::milliseconds >(
		 std::chrono::system_clock::now().time_since_epoch()
		 );
	 std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds >(
		 std::chrono::system_clock::now().time_since_epoch()
		 );
	 long long gap = std::chrono::duration_cast<std::chrono::milliseconds>(ms - lastms).count();
	 // using message to compare whether they are different from last one.
	 // it is the inefficient way, shall compare message type
	 if (lastmsg == msg && gap <= 1000)
		 return;

	 lastms = ms;
	 lastmsg = msg;
	 const std::wstring appname = nx::utils::wstr_toupper(utils::Module().GetNameW());
	 pInstance->RPMNotifyMessage(appname, appname, msg, 1);
}

void setwatermark(HWND hwnd, const nx::overlay::WatermarkParam param) {

	std::wstring strMsg = L"******setwatermark hwnd : " + std::to_wstring((uint64_t)hwnd) + L"******\n";
	::OutputDebugStringW(strMsg.c_str());

	pInstance->RPMSetViewOverlay(hwnd,
		param.text(),
		param.font_color(),
		param.font_name(),
		param.font_size(),
		param.text_rotation(),
		param.font_style(),
		param.text_alignment(),
		//{ 30,80,30,30 }
		{ 20,20,20,20 }
	);
}

void clearwatermark(HWND hwnd) {
	pInstance->RPMClearViewOverlay(hwnd);
}

bool is_nxl_file(const std::wstring& path)
{
	// sanity check
	if (path.length() < 3) {
		return false;
	}
	// using a fast way to judebg
	if (!pInstance) {
		return false;
	}
	// judge whether path is in rmp folder
	unsigned int dir_status = 0;
	bool file_in_rmp_folder = false;
	if (0 != pInstance->RPMGetFileStatus(path, &dir_status, &file_in_rmp_folder).GetCode()) {
		return false;
	}
	// for osrmx senario, an nxl file must be in RMP folder
	if (!file_in_rmp_folder) {
		return false;
	}
	// judege the hiden nxl file exist
	DWORD fattr= ::GetFileAttributes(path.c_str());
	if (fattr == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	// file exist now, I hope it's not a folder, anti-foolish coding
	if (fattr & FILE_ATTRIBUTE_DIRECTORY) {
		return false;
	}
	return true;
}

bool add_print_log(const std::wstring& path, bool is_allow)
{
	// sanity check
	if (!is_nxl_file(path)) {
		return false;
	}
	if (path.empty()) {
		return false;
	}

	if (!puser) {
		return false;
	}

	return 0 != puser->AddActivityLog(path, RL_OPrint, is_allow ? RL_RAllowed : RL_RDenied).GetCode();


}

// The following is commented out because we are deferring AutoProtect support
// for "copy" and "move" commands to after Makalu release.
#if 0
SDWLResult protect_file(const std::wstring& file_path, std::wstring& new_created_file_path, const std::vector<SDRmFileRight>& rights, const SDR_WATERMARK_INFO& watermark_info, const SDR_Expiration& expire, const std::string& tags)
{
	if (!puser) {
		return RESULT(SDWL_LOGIN_REQUIRED);
	}
	auto res = puser->ProtectFile(file_path, new_created_file_path, rights, watermark_info, expire, tags, puser->GetMembershipID(0), false);
	return res;
}
#endif  // #if 0



//
END_NXRM_NAMESPACE
//