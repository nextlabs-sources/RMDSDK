#pragma once
/*
Dependency:
	wtl, atl, Gdiplus
Principle: (for view watermark)
	using watermark child window with WS_EX_TOPMOST, WS_EX_LAYERED, WS_EX_TOOLWINDOW, WS_EX_TRANSPARENT
	using Gdi+ to draw string and make it as a Brush
	using text-brush to fill watermark child window

	using SetWindowsHook to sniff WM_XXX msg send to target window
	using UpdateLayeredWindow to make sure watermark content precisely conver on target window
*/
#include <atlbase.h>
#include <atltypes.h>
#include ".\wtl\Include\atlapp.h"
#include ".\wtl\Include\atlcrack.h"
#include <mutex>
#include <gdiplus.h>

/*
Allow user to config overlay's 
*/
class OverlayConfig {
	friend class OverlayConfigBuilder;
public:
	enum FontStyle {
		FS_Regular,
		FS_Bold,
		FS_Italic,
		FS_BoldItalic
	};

	enum TextAlignment {
		TA_Left,
		TA_Centre,
		TA_Right
	};

	OverlayConfig() {}
public: // getter;
	inline const std::wstring& GetString() { return m_str; }

	inline const std::wstring& GetFontName() { return m_font_name; }

	inline Gdiplus::FontStyle GetGdiFontStyle(){
		if (m_font_style == FS_Regular) {
			return Gdiplus::FontStyle::FontStyleRegular;
		}
		if (m_font_style == FS_Bold) {
			return Gdiplus::FontStyle::FontStyleBold;
		}
		if (m_font_style == FS_Italic) {
			return Gdiplus::FontStyle::FontStyleItalic;
		}
		if (m_font_style == FS_BoldItalic) {
			return Gdiplus::FontStyle::FontStyleBoldItalic;
		}

		// Notice: return FontStyleRegular by default
		return Gdiplus::FontStyle::FontStyleRegular;
	}

	inline Gdiplus::StringAlignment GetGdiTextAlignment() {
		if (m_text_alignment == TA_Left) {
			return Gdiplus::StringAlignment::StringAlignmentNear;
		}
		if (m_text_alignment == TA_Centre) {
			return Gdiplus::StringAlignment::StringAlignmentCenter;
		}
		if (m_text_alignment == TA_Right) {
			return Gdiplus::StringAlignment::StringAlignmentFar;
		}
		return Gdiplus::StringAlignment::StringAlignmentCenter;
	}

	inline Gdiplus::StringAlignment GetGdiLineAlignment() {
		if (m_line_alignment == TA_Left) {
			return Gdiplus::StringAlignment::StringAlignmentNear;
		}
		if (m_line_alignment == TA_Centre) {
			return Gdiplus::StringAlignment::StringAlignmentCenter;
		}
		if (m_line_alignment == TA_Right) {
			return Gdiplus::StringAlignment::StringAlignmentFar;
		}
		return Gdiplus::StringAlignment::StringAlignmentCenter;
	}

	inline Gdiplus::Color GetFontColor() {return Gdiplus::Color(m_font_color_A, m_font_color_R, m_font_color_G, m_font_color_B);}
	inline int GetFontSize() {return this->m_font_size;}
	inline int GetFontRotation() {return this->m_font_rotation;}
	inline CRect GetDisplayOffset() { return this->m_display_offset; }


public:
	bool operator ==(const OverlayConfig& rh) {return IsSameConfig(rh);}

	bool operator !=(const OverlayConfig& rh) {return !IsSameConfig(rh);}
private:
	bool IsSameConfig(const OverlayConfig& rh);

	inline void ResetAllParamByDefault() {
		m_str = L"";
		m_font_name = L"Arial";
		m_font_size = 20;
		m_font_rotation = -20;
		m_font_color_A = 50;
		m_font_color_R = 75;
		m_font_color_G = 75;
		m_font_color_B = 75;
		m_font_style = OverlayConfig::FontStyle::FS_Regular;
		m_line_alignment = OverlayConfig::TextAlignment::TA_Centre;
		m_text_alignment = OverlayConfig::TextAlignment::TA_Centre;
		m_display_offset = CRect();
		
	}

private:
	std::wstring m_str;
	int m_font_size;
	int m_font_rotation;  //i.e.  -10, -90, 10, 30, 45
	// RGB[0,0,0] is black, RGB[255,255,255] is white
	unsigned char m_font_color_A; // [0,255]: 0: fully transparent, 255: opacity
	unsigned char m_font_color_R;	// [0,255]  
	unsigned char m_font_color_G; // [0,255]
	unsigned char m_font_color_B; // [0,255]
	FontStyle m_font_style;
	TextAlignment m_text_alignment;
	TextAlignment m_line_alignment;
	std::wstring m_font_name;
	CRect m_display_offset;
};

class OverlayConfigBuilder {
	OverlayConfig _config;
public:
	OverlayConfigBuilder() {
		// set default
		_config.ResetAllParamByDefault();
	}

	OverlayConfigBuilder& SetString(const std::wstring& str) {
		_config.m_str = str;
		return *this;
	}

	OverlayConfigBuilder& SetFontName(const std::wstring& font_name) {
		if (font_name.empty()) {
			_config.m_font_name = GetDefaultFontName();
			return *this;
		}
		if (IsFontNameSupported(font_name)) {
			_config.m_font_name = font_name;
		}
		else {
			//throw std::exception("font name is not supported");
			_config.m_font_name = GetDefaultFontName();
		}
		return *this;
	}

	OverlayConfigBuilder& SetFontSize(int size) {
		if (size <= 0) {
			size = 20;// bydefault
		}
		else if (size > 72) {
			size = 72;
		}
		_config.m_font_size = size;
		return *this;
	}
	OverlayConfigBuilder& SetFontRotation(int rotation) {
		_config.m_font_rotation = rotation;
		return *this;
	}

	OverlayConfigBuilder& SetFontTransparency(unsigned char A) {
		_config.m_font_color_A = A;
		return *this;
	}


	OverlayConfigBuilder& SetFontColor(unsigned char A, unsigned char R, unsigned char G, unsigned char B) {
		_config.m_font_color_A = A;
		_config.m_font_color_R = R;
		_config.m_font_color_G = G;
		_config.m_font_color_B = B;
		return *this;
	}

	OverlayConfigBuilder& SetFontColor(unsigned char R, unsigned char G, unsigned char B) {
		_config.m_font_color_R = R;
		_config.m_font_color_G = G;
		_config.m_font_color_B = B;
		return *this;
	}

	OverlayConfigBuilder& SetFontStyle(OverlayConfig::FontStyle fs) {
		_config.m_font_style = fs;
		return *this;
	}

	OverlayConfigBuilder& SetTextAlignment(OverlayConfig::TextAlignment	alignment) {
		_config.m_text_alignment = alignment;
		return *this;
	}

	OverlayConfigBuilder& SetLineAlignment(OverlayConfig::TextAlignment	alignment) {
		_config.m_line_alignment = alignment;
		return *this;
	}

	OverlayConfigBuilder& SetDisplayOffset(const RECT& rc) {
		_config.m_display_offset = rc;
		return *this;
	}


	OverlayConfig Build() {		
		ThrowIfInvalidParam();
		return _config;
	}
private:
	inline void ThrowIfInvalidParam() {
		if (_config.m_str.length() < 1) {
			throw std::exception("too little chars in watermark string");
		}
	}
	std::wstring GetDefaultFontName();
	bool IsFontNameSupported(const std::wstring& font_name);
};

typedef 
CWinTraits<	WS_POPUP | WS_VISIBLE | WS_DISABLED,
			//WS_EX_TOPMOST | WS_EX_LAYERED |	WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT     // topmost will result print output to screen
			WS_EX_LAYERED |	WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE
		  >OverlayWindowTraits;

/*
with WS_EX_LAYERED
with WS_EX_TOOLWINDOW, floating and convering the target windows,

Notice:
	- using controller to change this wnd's size and make it finedly convered with target wnd
	- tar_wnd changing, changing this object simultaneously
			   
*/
class OverlayWindow : public CWindowImpl<OverlayWindow, CWindow, OverlayWindowTraits>
{
	friend class ViewOverlyController;
public:
	OverlayWindow() :OldTarget(-1,-1,-1,-1) {}
	~OverlayWindow();
private:
	inline void SetOverlay(const OverlayConfig& config) {_config = config;}
	void Init(const OverlayConfig& ol, HWND target);
	void UpdateOverlaySizePosStatus(HWND target);
	inline void HideWnd();
	void _DrawOverlay(HDC dc, LPRECT lpRect);
	void _PrepareOverly();

private:
	Gdiplus::Bitmap* _GetOverlayBitmap(const Gdiplus::Graphics& drawing_surface);

public: // section for wnd_registration and msg_handler
	DECLARE_WND_CLASS_EX(L"NextlabsOverlay", 0, (HBRUSH)(COLOR_WINDOW + 1));
	// don't need handle msg, using win_default_msg_handler is enough
	BEGIN_MSG_MAP_EX(OverlayWindow)
		MSG_WM_WINDOWPOSCHANGED(OnPosChanged)
	END_MSG_MAP()

	LRESULT OnPosChanged(WINDOWPOS* lpwndpos);
private:
	OverlayConfig _config;
	CMemoryDC* pmdc;
	CRect OldTarget;
};

class ViewOverlyController {
	HHOOK _swhHook; // standard by ::SetWindowsHookEx
	std::map<HWND, std::shared_ptr<OverlayWindow> > _wnds; // targetDocWnd -> OverlayWnd
public:
	static ViewOverlyController& getInstance();
private:
	ViewOverlyController():_swhHook(NULL) {};
	~ViewOverlyController();
public:
	void Attach(HWND target, const OverlayConfig& config, int tid = 0);
	void Detach(HWND target);	
	void Clear();	// clear all watermarks
	void SetOverlyTarget(HWND target);
	void UpdateWatermark(HWND target);
private:
	LRESULT OnMessageHook(int code, WPARAM wParam, LPARAM lParam);
private:
	static ViewOverlyController* sgIns;
	static std::recursive_mutex sgRMutex; // this class mutex
	static LRESULT CALLBACK HookProxy(int code, WPARAM wParam, LPARAM lParam);
};



// some helpers
//namespace gdi {
//	std::vector<std::wstring> GetInstalledFonts();
//	bool SaveFileAsBitmap(Gdiplus::Image* image, std::wstring path);
//	Gdiplus::PointF CaculateRotated(Gdiplus::PointF& org, int angle);
//	Gdiplus::RectF CaculateOutbound(Gdiplus::PointF(&org)[4]);
//	Gdiplus::RectF CalculateMinimumEnclosingRectAfterRotate(const Gdiplus::SizeF& size, int rotate);
//}