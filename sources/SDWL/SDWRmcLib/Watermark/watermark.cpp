#include "../stdafx.h"
#include <cctype>
#include <atlstr.h>
#include "watermark.h"
#include <cmath>  // using sin cos
#include <functional>
#include <TlHelp32.h>


using namespace Gdiplus;
#pragma comment(lib,"gdiplus.lib")

#ifndef MAKEULONGLONG
#define MAKEULONGLONG(ldw, hdw) ((ULONGLONG(hdw) << 32) | ((ldw) & 0xFFFFFFFF))
#endif

#ifndef MAXULONGLONG
#define MAXULONGLONG ((ULONGLONG)~((ULONGLONG)0))
#endif

using namespace std;



namespace {

	ULONG_PTR gGidplusToken;
	Gdiplus::GdiplusStartupInput gGdipulsInput;

	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid){
		UINT  num = 0;          // number of image encoders
		UINT  size = 0;         // size of the image encoder array in bytes

		ImageCodecInfo* pImageCodecInfo = NULL;

		GetImageEncodersSize(&num, &size);
		if (size == 0)
			return -1;  // Failure

		pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
		if (pImageCodecInfo == NULL)
			return -1;  // Failure

		GetImageEncoders(num, size, pImageCodecInfo);

		for (UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}
		}

		free(pImageCodecInfo);
		return -1;  // Failure
	}


	// calculate the size art-text used
	Gdiplus::SizeF CalcTextSizeF(const Gdiplus::Font& font, const Gdiplus::StringFormat& strFormat, const CString& szText){
		Gdiplus::GraphicsPath graphicsPathObj;
		Gdiplus::FontFamily fontfamily;
		font.GetFamily(&fontfamily);
		graphicsPathObj.AddString(szText, -1,&fontfamily,font.GetStyle(),font.GetSize(),PointF(0, 0), &strFormat);
		Gdiplus::RectF rcBound;
		graphicsPathObj.GetBounds(&rcBound);
		return Gdiplus::SizeF(rcBound.Width, rcBound.Height);
	}


	Gdiplus::SizeF CalcTextSizeF(
		const Gdiplus::Graphics& drawing_surface,
		const CString& szText,
		const Gdiplus::StringFormat& strFormat,
		const Font& font){
		Gdiplus::RectF rcBound;
		drawing_surface.MeasureString(szText, -1, &font, Gdiplus::PointF(0, 0), &strFormat, &rcBound);
		return Gdiplus::SizeF(rcBound.Width, rcBound.Height);
	}

	DWORD GetMainThreadID() {
		DWORD dwProcID = ::GetCurrentProcessId();
		DWORD dwMainThreadID = 0;
		ULONGLONG ullMinCreateTime = MAXULONGLONG;

		HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hThreadSnap != INVALID_HANDLE_VALUE) {
			THREADENTRY32 th32;
			th32.dwSize = sizeof(THREADENTRY32);
			BOOL bOK = TRUE;
			for (bOK = Thread32First(hThreadSnap, &th32); bOK;
				bOK = Thread32Next(hThreadSnap, &th32)) {
				if (th32.th32OwnerProcessID == dwProcID) {
					HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION,
						TRUE, th32.th32ThreadID);
					if (hThread) {
						FILETIME afTimes[4] = { 0 };
						if (GetThreadTimes(hThread,
							&afTimes[0], &afTimes[1], &afTimes[2], &afTimes[3])) {
							ULONGLONG ullTest = MAKEULONGLONG(afTimes[0].dwLowDateTime,
								afTimes[0].dwHighDateTime);
							if (ullTest && ullTest < ullMinCreateTime) {
								ullMinCreateTime = ullTest;
								dwMainThreadID = th32.th32ThreadID; // let it be main... :)
							}
						}
						CloseHandle(hThread);
					}
				}
			}
			CloseHandle(hThreadSnap);
		}
		return dwMainThreadID;
	}

	std::vector<std::wstring> GetInstalledFonts(){
		Gdiplus::InstalledFontCollection ifc;
		std::vector<std::wstring> rt;
		auto count = ifc.GetFamilyCount();
		int actualFound = 0;

		Gdiplus::FontFamily* buf = new Gdiplus::FontFamily[count];
		ifc.GetFamilies(count, buf, &actualFound);
		for (int i = 0; i < actualFound; i++) {
			wchar_t name[0x20] = { 0 };
			buf[i].GetFamilyName(name);
			rt.push_back(name);
		}

		delete[] buf;
		return rt;
	}

	bool iequal(const std::wstring& l, const std::wstring& r) {
		if (l.size() != r.size()) {
			return false;
		}

		return std::equal(l.begin(), l.end(), r.begin(), r.end(), [](wchar_t i, wchar_t j) {
			return std::tolower(i) == std::tolower(j);
		});

	}

	class MsgAntiReenter {
	public:
		MsgAntiReenter() {
			if (InitializeCriticalSectionAndSpinCount(&cs, 0x80004000) == FALSE) {
				InitializeCriticalSection(&cs);
			}
		}
		~MsgAntiReenter() {
			DeleteCriticalSection(&cs);
		}
		void thread_enable(void) {
			EnterCriticalSection(&cs);
			disabled_thread[GetCurrentThreadId()]--;
			LeaveCriticalSection(&cs);
		}

		void thread_disable(void) {
			DWORD tid = GetCurrentThreadId();
			EnterCriticalSection(&cs);
			if (disabled_thread.find(tid) == disabled_thread.end()) {
				disabled_thread[tid] = 0;
			}
			disabled_thread[tid]++;
			LeaveCriticalSection(&cs);
		}

		bool is_thread_disabled(void) {
			bool result = false;
			DWORD tid = GetCurrentThreadId();
			EnterCriticalSection(&cs);
			if (disabled_thread.find(tid) != disabled_thread.end()) {
				if (disabled_thread[tid] > 0) {
					result = true;
				}
			}
			LeaveCriticalSection(&cs);
			return result;
		}

	private:
		CRITICAL_SECTION     cs;
		std::map<DWORD, int>  disabled_thread;
	};
	class MsgAntiRenter_Control {
	public:
		MsgAntiRenter_Control(MsgAntiReenter& mar) :_mar(mar) { mar.thread_disable(); }
		~MsgAntiRenter_Control() { _mar.thread_enable(); }
	private:
		MsgAntiReenter& _mar;
	};


	MsgAntiReenter g_mar;

} // end anonymous namespace

namespace gdi {

	vector<wstring> GetInstalledFonts()
	{
		Gdiplus::InstalledFontCollection ifc;
		vector<wstring> rt;
		auto count = ifc.GetFamilyCount();
		int actualFound = 0;

		Gdiplus::FontFamily* buf = new Gdiplus::FontFamily[count];
		ifc.GetFamilies(count, buf, &actualFound);
		for (int i = 0; i < actualFound; i++) {
			wchar_t name[0x20] = { 0 };
			buf[i].GetFamilyName(name);
			rt.push_back(name);
		}

		delete[] buf;
		return rt;
	}

	bool SaveFileAsBitmap(Gdiplus::Image * image, std::wstring path)
	{

		CLSID clsid;
		GetEncoderClsid(L"image/bmp", &clsid);

		image->Save(path.c_str(), &clsid);

		return false;
	}

	Gdiplus::PointF CaculateRotated(Gdiplus::PointF& org, int angle)
	{
		static const double PI = std::acos(-1);
		Gdiplus::PointF rt;

		double radians = angle * PI / 180;

		rt.X = (Gdiplus::REAL)(org.X*std::cos(radians) - org.Y*std::sin(radians));
		rt.Y = (Gdiplus::REAL)(org.X*std::sin(radians) + org.Y*std::cos(radians));

		return rt;
	}


	// 给定四个点 如何计算 最小矩形正好包含所有信息?
	// 最小的x,y是原点,  最小的x和最大的x的差值就是宽	
	Gdiplus::RectF CaculateOutbound(Gdiplus::PointF(&org)[4])
	{

		vector<Gdiplus::REAL> Xs, Ys;
		for (int i = 0; i < 4; i++) {
			Xs.push_back(org[i].X);
			Ys.push_back(org[i].Y);
		}

		std::sort(Xs.begin(), Xs.end());
		std::sort(Ys.begin(), Ys.end());

		Gdiplus::REAL width = Xs.back() - Xs.front();
		Gdiplus::REAL height = Ys.back() - Ys.front();

		return Gdiplus::RectF(Xs.front(), Ys.front(), width, height);
	}

	// 给定一个矩形,默认水平放置,计算其旋转后可以包围此矩形的最小矩形
	Gdiplus::RectF CalculateMinimumEnclosingRectAfterRotate(const Gdiplus::SizeF & size, int rotate)
	{
		PointF org[4] = {
			{0,0},{0,size.Height},{size.Width, size.Height},  {size.Width, 0}
		};

		PointF org_r[4];
		for (int i = 0; i < 4; i++) {
			org_r[i] = gdi::CaculateRotated(org[i], rotate);
		}

		return gdi::CaculateOutbound(org_r);
	}
}

OverlayWindow::~OverlayWindow() {
	if (IsWindow()) {
		this->ShowWindow(SW_HIDE);
		this->DestroyWindow();
	}
}

LRESULT OverlayWindow::OnPosChanged(WINDOWPOS* lpwndpos)
{
	// Get WaterMark Window Parent
	HWND hparent = ::GetParent(m_hWnd);
	if (hparent)
	{
		// am I ahead of my parent window
		HWND hwndnext = m_hWnd;
		BOOL bfoundparent = false;

		for (int i = 0; i < 8; i++) // try to find parent windows in next 8 same level window
		{
			hwndnext = ::GetWindow(hwndnext, GW_HWNDNEXT);

			std::wstring strMsg = L"*** OnPosChanged::GetWindow ===\n hwnd :" + std::to_wstring((uint64_t)m_hWnd)
				+ L" hwndnext : " + std::to_wstring((uint64_t)hwndnext)
				+ L" hparent : " + std::to_wstring((uint64_t)hparent)
				+ L" ***\n";

			::OutputDebugStringW(strMsg.c_str()); 

			if (hwndnext == hparent)
			{
				bfoundparent = true;
				break;
			}

			if (::IsWindow(hwndnext) == false)
				break;
		}

		if (bfoundparent == false)
		{
			// watermark window is not before target/parent window, reset it
			BOOL bRet = ::SetWindowPos(hparent, m_hWnd, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

			std::wstring strMsg = L"*** OnPosChanged::SetWindowPos ";
			if (!bRet)
			{
				DWORD dwErr = ::GetLastError();
				strMsg += L" , error : " + std::to_wstring(dwErr);
			}
			strMsg += L" bRet : " + std::to_wstring(bRet) 
				+ L" hwnd : " + std::to_wstring((uint64_t)m_hWnd)
				+ L" hwndnext : " + std::to_wstring((uint64_t)hwndnext)
				+ L" hparent : " + std::to_wstring((uint64_t)hparent)
				+ L" ***\n";
			::OutputDebugStringW(strMsg.c_str());
		}
	}

	return 0;
}

void OverlayWindow::Init(const OverlayConfig& ol, HWND target) {
	SetOverlay(ol);
	Create(target);
	_PrepareOverly();
}

void OverlayWindow::UpdateOverlaySizePosStatus(HWND target)
{
	CRect targetRC;
	if (target == NULL) {
		// user physical device may changed ,you get it each tiem
		targetRC = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	}
	else {
		::GetWindowRect(target, &targetRC);
		// this is a temp offset, since we dont want to include non-client area
		//targetRC.left += 20;
		//targetRC.top += 30;
		//targetRC.right -= 20; // remove right edge effect
		//targetRC.bottom -= 30;// remove status bar if exist;
	}

	if (OldTarget.EqualRect(targetRC)) {
		//OutputDebugStringA("same CRect, return directly");
		return;
	}
	else {
		OldTarget = targetRC;
	}

	targetRC.DeflateRect(_config.GetDisplayOffset());
	// make layered wnd always covered the targert Wnd
	MoveWindow(targetRC,false);

	BLENDFUNCTION blend = { AC_SRC_OVER ,0,0xFF,AC_SRC_ALPHA };
	//CPoint p(targetRC.left, targetRC.right);
	CPoint p(0, 0);
	CPoint dstpt(targetRC.left, targetRC.top);
	CSize s(targetRC.Width(), targetRC.Height());
	
	// draw in Screen, but always get target wnd's region info
	if (!::UpdateLayeredWindow(this->m_hWnd,
		NULL,
		&dstpt, &s, 
		*pmdc,&p,   // src dc and {left,top}
		NULL,&blend, ULW_ALPHA)  // using alpha blend,
		) {
		// error occured
		auto err = ::GetLastError();
		std::string strErr = "Faied call UpdateLayeredWindow,error is ";
		strErr += std::to_string(err);
		::OutputDebugStringA(strErr.c_str());
	}
}

void OverlayWindow::HideWnd() {
	if (IsWindow()) {
		this->ShowWindow(SW_HIDE);
	}
}

Gdiplus::Bitmap * OverlayWindow::_GetOverlayBitmap(const Gdiplus::Graphics& drawing_surface)
{
	CString overlay_str(_config.GetString().c_str());	
	Gdiplus::FontFamily fontfamily(_config.GetFontName().c_str());
	Gdiplus::Font font(&fontfamily,(Gdiplus::REAL)_config.GetFontSize(),_config.GetGdiFontStyle(), Gdiplus::UnitPixel);
	Gdiplus::SolidBrush brush(_config.GetFontColor());
	Gdiplus::StringFormat str_format;
	str_format.SetAlignment(_config.GetGdiTextAlignment());
	str_format.SetLineAlignment(_config.GetGdiLineAlignment());
	Gdiplus::SizeF str_size = CalcTextSizeF(drawing_surface, overlay_str,str_format,font);
	Gdiplus::RectF str_enclosing_rect = gdi::CalculateMinimumEnclosingRectAfterRotate(str_size, _config.GetFontRotation());

	Gdiplus::REAL surface_size = 2 * std::ceil(std::hypot(str_size.Width, str_size.Height));

	Gdiplus::Bitmap surface((INT)surface_size, (INT)surface_size, PixelFormat32bppARGB);
	Gdiplus::Graphics g(&surface);
	// make a good quality
	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	// set centre point as the base point
	g.TranslateTransform(surface_size / 2, surface_size / 2);
	g.RotateTransform((Gdiplus::REAL)_config.GetFontRotation());
	// set string
	g.DrawString(overlay_str.GetString(), -1, &font,
		Gdiplus::RectF(0, 0, str_size.Width, str_size.Height),
		&str_format,&brush);
	g.ResetTransform();
	g.Flush();

	// since drawing org_point is the centre, 
	Gdiplus::RectF absolute_layout = str_enclosing_rect;
	absolute_layout.Offset(surface_size / 2, surface_size / 2);


	// request bitmap is the partly clone with absolute_layout
	return surface.Clone(absolute_layout, PixelFormat32bppARGB);
}


void OverlayWindow::_DrawOverlay(HDC dcScreen, LPRECT lpRestrictDrawingRect)
{
	if (dcScreen == NULL) {
		return;
	}
	CRect rc(lpRestrictDrawingRect);
	Gdiplus::Graphics g(dcScreen);
	// make a good quality
	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	// beging drawing
	Gdiplus::Bitmap* bk = _GetOverlayBitmap(g);
	Gdiplus::TextureBrush brush(bk,Gdiplus::WrapModeTile);
	Gdiplus::RectF surface(0,0,(Gdiplus::REAL)rc.Width(), (Gdiplus::REAL)rc.Height());
	g.FillRectangle(&brush, surface);
	delete bk;
}

void OverlayWindow::_PrepareOverly()
{
	// Get Whole Screen pixels
	CRect ScreenRC = { 
		GetSystemMetrics(SM_XVIRTUALSCREEN),
		GetSystemMetrics(SM_YVIRTUALSCREEN),
		GetSystemMetrics(SM_CXVIRTUALSCREEN)+100,
		GetSystemMetrics(SM_CYVIRTUALSCREEN) + 100 };

	// Get a large surface to draw overlay
	CDC dc=::GetDC(NULL);
	pmdc = new CMemoryDC(dc, ScreenRC);
	_DrawOverlay(*pmdc, ScreenRC);
}




//
//  classController
//
//

ViewOverlyController* ViewOverlyController::sgIns = NULL;
std::recursive_mutex ViewOverlyController::sgRMutex;
#ifdef ViewOverlyControllerScopeGurad
	#error this is Impossible
#endif // ViewOverlyControllerScopeGurad
#define ViewOverlyControllerScopeGurad std::lock_guard<std::recursive_mutex> g(sgRMutex)

ViewOverlyController & ViewOverlyController::getInstance()
{
	ViewOverlyControllerScopeGurad;
	if (sgIns == NULL) {
		// init gdi++
		Gdiplus::GdiplusStartup(&gGidplusToken, &gGdipulsInput, NULL);

		sgIns = new ViewOverlyController();
	}
	return *sgIns;
	
}


ViewOverlyController::~ViewOverlyController()
{
	if (_swhHook != NULL) {
		::UnhookWindowsHookEx(_swhHook);
	}
	// deinit gdi++
	if (gGidplusToken != NULL) {
		Gdiplus::GdiplusShutdown(gGidplusToken);
		gGidplusToken = NULL;
	}

}

void ViewOverlyController::Attach(HWND target, const OverlayConfig& config, int tid)
{
	ViewOverlyControllerScopeGurad;

	if (_wnds.find(target) != _wnds.end()) {
		// has got
		if (_wnds[target]->_config != config) {
			_wnds[target]->SetOverlay(config);
		}
	}
	else {
		std::shared_ptr<OverlayWindow> spWnd(new OverlayWindow());
		spWnd->Init(config,target);
		_wnds[target] = spWnd;
		SetOverlyTarget(target);
	}
}

void ViewOverlyController::Detach(HWND target)
{
	ViewOverlyControllerScopeGurad;

	if (_wnds.find(target) != _wnds.end()) {
		_wnds[target]->HideWnd();
		_wnds.erase(target);
	}
}

void ViewOverlyController::Clear()
{
	ViewOverlyControllerScopeGurad;
	_wnds.clear();
}

void ViewOverlyController::SetOverlyTarget(HWND target)
{
	ViewOverlyControllerScopeGurad;

	if (_swhHook == NULL) {
		// call this function on UI thread
		_swhHook = ::SetWindowsHookEx(WH_CALLWNDPROCRET,	// after wnd had processed the message
			ViewOverlyController::HookProxy,
			NULL,
			//::GetMainThreadID()
			::GetCurrentThreadId()
		);

		if (_swhHook == NULL) {
			throw new std::exception("failed, call SetWindowsHookEx");
		}
	}

	if (_wnds.find(target) != _wnds.end()) {
		_wnds[target]->UpdateOverlaySizePosStatus(target);
	}
}

void ViewOverlyController::UpdateWatermark(HWND target) {
	ViewOverlyControllerScopeGurad;
	if (_wnds.find(target) != _wnds.end()) {
		_wnds[target]->UpdateOverlaySizePosStatus(target);
	}
}

LRESULT ViewOverlyController::OnMessageHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code < 0 || lParam == 0) {
		return ::CallNextHookEx(_swhHook, code, wParam, lParam);
	}

	// extra code, to avoid meaningless reentrance
	if (g_mar.is_thread_disabled()) {
		return ::CallNextHookEx(_swhHook, code, wParam, lParam);
	}
	MsgAntiRenter_Control auto_control(g_mar);

	CWPRETSTRUCT* p = (CWPRETSTRUCT*)lParam;
	// may be main window moving
	UINT msg = p->message;
	HWND t = p->hwnd;
	ViewOverlyControllerScopeGurad;
	if(_wnds.empty() || _wnds.find(t) == _wnds.end()){
		return ::CallNextHookEx(_swhHook, code, wParam, lParam);
	}

	switch (msg)
	{
	case WM_MOVE:
	case WM_MOVING:
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
	case WM_SHOWWINDOW:
	//case WM_SIZE:
	//case WM_SIZING:
	case WM_SYSCOMMAND:
	{
		_wnds[t]->UpdateOverlaySizePosStatus(t);
		break;
	}
	case WM_DESTROY:
	{
		// target wnd wants destory tell to destory overlay wnd
		_wnds[t]->HideWnd();
		_wnds.erase(t);
		break;
	}
	default:
		break;
	}
	
	return ::CallNextHookEx(_swhHook, code, wParam, lParam);
}

LRESULT ViewOverlyController::HookProxy(int code, WPARAM wParam, LPARAM lParam)
{	
	return getInstance().OnMessageHook(code, wParam, lParam);
}

//
//  for config of overlay
// 

std::wstring OverlayConfigBuilder::GetDefaultFontName()
{
	// using Gdipuls provided default 
	std::vector<wchar_t> buf(0x30, 0);
	Gdiplus::FontFamily::GenericSansSerif()->GetFamilyName(buf.data(), LANG_NEUTRAL);
	return std::wstring(buf.data());
}

bool OverlayConfigBuilder::IsFontNameSupported(const std::wstring & font_name)
{
	auto cont = GetInstalledFonts();
	for (auto& i : cont) {
		if (iequal(i, font_name)) {
			return true;
		}
	}
	return false;
}

bool OverlayConfig::IsSameConfig(const OverlayConfig & rh)
{
	if (this == &rh) {
		return true;
	}
	// check all 
	if (this->m_str != rh.m_str) {return false;}
	if (this->m_font_size != rh.m_font_size) {return false;}
	if (this->m_font_rotation != rh.m_font_rotation) { return false; }
	if (this->m_font_color_A != rh.m_font_color_A) {return false;}
	if (this->m_font_color_R != rh.m_font_color_R) {return false;}
	if (this->m_font_color_G != rh.m_font_color_G) {return false;}
	if (this->m_font_color_B != rh.m_font_color_B) {return false;}
	if (this->m_font_style!=rh.m_font_style) { return false; }
	if (!iequal(this->m_font_name, rh.m_font_name)) { return false; }
	if (this->m_display_offset != rh.m_display_offset) { return false; }

	return true;
}
