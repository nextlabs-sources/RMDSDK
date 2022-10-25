#include "stdafx.h"
#include "global_data_model.h"



bool isOfficeProcess(const std::wstring& exe_name) {
	static std::vector<std::wstring> buf{
		L"WINWORD.EXE",
		L"POWERPNT.EXE",
		L"EXCEL.EXE"
	};

	return std::any_of(buf.begin(), buf.end(), [&exe_name](const std::wstring& str) {
		if (exe_name.empty() && str.empty()) {
			return true;
		}
		if (exe_name.length() != str.length()) {
			return false;
		}
		auto rt = std::mismatch(exe_name.begin(),exe_name.end(), 
			str.begin(), str.end(),[](wchar_t l, wchar_t r) {
				return std::tolower(l) == std::tolower(r);
			});
		return rt.first==exe_name.end() && rt.second==str.end();
		});

}

inline bool isSapVEViwer(const std::wstring& exe_name) {
	const static std::wstring sap = L"VEViewer.exe";

	if (exe_name.length() != sap.length()) {
		return false;
	}
	
	return 0 == _wcsnicmp(exe_name.c_str(), sap.c_str(), exe_name.length());
}

inline bool isBentleyView(const std::wstring& exe_name) {
	const static std::wstring sap = L"BentleyView.exe";

	if (exe_name.length() != sap.length()) {
		return false;
	}

	return 0 == _wcsnicmp(exe_name.c_str(), sap.c_str(), exe_name.length());
}

inline bool isAdobeReader(const std::wstring& exe_name) {
	const static std::wstring adreader = L"AcroRd32.exe";

	if (exe_name.length() != adreader.length()) {
		return false;
	}
	return 0 == _wcsnicmp(exe_name.c_str(), adreader.c_str(), exe_name.length());
}

inline bool isAdobeAcrobat(const std::wstring& exe_name) {
	const static std::wstring acrobat = L"acrobat.exe";

	if (exe_name.length() != acrobat.length()) {
		return false;
	}
	return 0 == _wcsnicmp(exe_name.c_str(), acrobat.c_str(), exe_name.length());
}

inline bool is3dTool(const std::wstring& exe_name) {
	const static std::wstring _3dTool = L"3D-Tool";
	if (exe_name.length() < _3dTool.length()) {
		return false;
	}
	return std::wstring::npos != exe_name.find(_3dTool, 0);

}

inline bool isVisView(const std::wstring& exe_name) {
	const static std::wstring _visviewe = L"VisView.exe";
	if (exe_name.length() != _visviewe.length()) {
		return false;
	}
	return 0 == _wcsnicmp(exe_name.c_str(), _visviewe.c_str(), exe_name.length());
}

inline bool isEdge(const std::wstring& exe_name) {
	const static std::wstring _edge = L"edge.exe";
	if (exe_name.length() != _edge.length()) {
		return false;
	}

	return 0 == _wcsnicmp(exe_name.c_str(), _edge.c_str(), exe_name.length());
}

inline bool isJt2go(const std::wstring  exe_name) {
	const static std::wstring _visview = L"VisView.exe";
	const static std::wstring _visview_ng = L"VisView_NG.exe";

	return 0 == _wcsnicmp(exe_name.c_str(), _visview.c_str(), exe_name.length()) ||
		   0 == _wcsnicmp(exe_name.c_str(), _visview_ng.c_str(), exe_name.length());
}

inline bool isVisViewMatchIgnoreHook() {
	bool rt = false;

	wchar_t* cmdLine = ::GetCommandLineW();
	if (cmdLine == NULL) {
		return rt;
	}

	//::OutputDebugStringW(cmdLine);

	static const std::wstring target1 = L"VISVIEW_RMI_SERVER LICENSING_EXTENSIONS_MASK";
	static const std::wstring target2 = L"LICENSING_EXTENSIONS_MASK";
	
	std::wstring m = cmdLine;

	auto icontain = [](const std::wstring& m, const std::wstring& s) {
		
		return m.end() != std::search(m.begin(), m.end(), target1.begin(), target1.end(), [](wchar_t l, wchar_t r) { return std::toupper(l) == std::toupper(r); });
	};

	return icontain(m, target1) && icontain(m, target2);

}


void init_common_in_global()
{
	// pid;
	global.process_id = ::GetCurrentProcessId();
	// ui tid, 
	global.main_thread_id = ::GetCurrentThreadId();

	// gdi+
	//Gdiplus::GdiplusStartup(&global.gdiplusToken, &global.gdipulsInput, NULL);

	global.process_exit_event = ::CreateEventW(NULL, TRUE, FALSE, NULL);
	if (NULL == global.process_exit_event)
	{
		std::wstring strMsg = L"process_exit_event create failed , err = " + std::to_wstring(::GetLastError()) + L"\n";
		DEVLOG(strMsg.c_str());
	}

	std::wstring strEvent = L"Global\\EVENT_osrmx_" + std::to_wstring(global.process_id);
	global.nxlfile_open_event = ::CreateEventW(NULL, FALSE, FALSE, strEvent.c_str());
	if (NULL == global.nxlfile_open_event)
	{
		std::wstring strMsg = L"nxlfile_open_event create failed , err = " + std::to_wstring(::GetLastError()) + L"\n" ;
		DEVLOG(strMsg.c_str());
	}


	nx::utils::Module m;
	global.process_wpath = m.GetPathW();
	global.process_path = m.GetPathA();
	global.process_name = m.GetNameA();
	global.process_wname = m.GetNameW();

	global.is_office_process = isOfficeProcess(global.process_wname);
	global.is_sap_veviewer_process = isSapVEViwer(global.process_wname);
	global.is_BentleyView_process = isBentleyView(global.process_wname);
	global.is_adobe_reader_process = isAdobeReader(global.process_wname);
	global.is_adobe_acrobat_process = isAdobeAcrobat(global.process_wname);
	global.is_3d_tool_process = is3dTool(global.process_wname);
	global.is_jt2go_process = isJt2go(global.process_wname);
	global.is_visview_process = isVisView(global.process_wname);
	global.is_edge_process = isEdge(global.process_wname);

	if (global.is_visview_process) {
		global.is_match_visview_process_ignore_api_hook = isVisViewMatchIgnoreHook();
	}
	else {
		global.is_match_visview_process_ignore_api_hook = false;
	}

#ifdef _DEBUG
	std::wostringstream oss;
	oss << L"{"
		<< L"\nPID: " << std::showbase << std::hex << global.process_id
		<< L"\nUI TID: " << global.main_thread_id
		<< L"\nPath: " << global.process_wpath
		<< L"\nName: " << global.process_wname
		<< L"\nIs_office_process" << global.is_office_process
		<< L"\n}\n";
	DEVLOG(oss.str().c_str());
#endif // _DEBUG

}
