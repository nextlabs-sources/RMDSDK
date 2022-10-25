#pragma once

#define NXOVERLAY_NAMESPACE namespace nx {namespace overlay {
#define END_NXOVERLAY_NAMESPACE }}



NXOVERLAY_NAMESPACE

class WatermarkParam {
	friend class WatermarkParamBuilder;
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

	typedef std::tuple<unsigned char, unsigned char, unsigned char, unsigned char> color_tuple;

	WatermarkParam() {}
public:
	inline std::wstring text() const { return m_text; }
	inline std::wstring font_name() const { return m_font_name; }
	inline int font_size() const { return m_font_size; }
	inline int text_rotation() const { return m_text_rotation; }
	inline Gdiplus::Color font_gdi_color() const { return Gdiplus::Color(m_font_color_A, m_font_color_R, m_font_color_G, m_font_color_B); }
	inline color_tuple font_color() const { return color_tuple(m_font_color_A, m_font_color_R, m_font_color_G, m_font_color_B); }
	inline Gdiplus::FontStyle font_style()const {
		switch (m_font_style) {
		case FS_Regular:
			return Gdiplus::FontStyle::FontStyleRegular;
		case FS_Bold:
			return Gdiplus::FontStyle::FontStyleBold;
		case FS_Italic:
			return Gdiplus::FontStyle::FontStyleItalic;
		case FS_BoldItalic:
			return Gdiplus::FontStyle::FontStyleBoldItalic;
		default:
			return Gdiplus::FontStyle::FontStyleRegular;
		}
	}
	inline Gdiplus::StringAlignment text_alignment()const {
		switch (m_text_alignment) {
		case TA_LEFT:
			return Gdiplus::StringAlignment::StringAlignmentNear;
		case TA_Right:
			return Gdiplus::StringAlignment::StringAlignmentFar;
		case TA_Centre:
			return Gdiplus::StringAlignment::StringAlignmentCenter;
		default:
			return Gdiplus::StringAlignment::StringAlignmentCenter;
		}
	}
	inline Gdiplus::StringAlignment line_alignment()const {
		switch (m_line_alignment) {
		case TA_LEFT:
			return Gdiplus::StringAlignment::StringAlignmentNear;
		case TA_Right:
			return Gdiplus::StringAlignment::StringAlignmentFar;
		case TA_Centre:
			return Gdiplus::StringAlignment::StringAlignmentCenter;
		default:
			return Gdiplus::StringAlignment::StringAlignmentCenter;
		}
	}

public:
	inline bool operator ==(const WatermarkParam& rh) { return IsSameConfig(rh); }
	inline bool operator !=(const WatermarkParam& rh) { return !IsSameConfig(rh); }
	inline WatermarkParam& operator =(const WatermarkParam& rh) {
		if (this != &rh) {
			this->m_text = rh.m_text;
			this->m_font_size = rh.m_font_size;
			this->m_font_style = rh.m_font_style;
			this->m_font_name = rh.m_font_name;
			this->m_text_rotation = rh.m_text_rotation;
			this->m_font_color_A = rh.m_font_color_A;
			this->m_font_color_R = rh.m_font_color_R;
			this->m_font_color_G = rh.m_font_color_G;
			this->m_font_color_B = rh.m_font_color_B;
			this->m_text_alignment = rh.m_text_alignment;
			this->m_line_alignment = rh.m_line_alignment;
		}
		return *this;
	}
private:
	inline bool IsSameConfig(const WatermarkParam& rh) {
		if (this == &rh) {
			return true;
		}
		// check all 
		if (this->m_text != rh.m_text) { return false; }
		if (this->m_font_size != rh.m_font_size) { return false; }
		if (this->m_text_rotation != rh.m_text_rotation) { return false; }
		if (this->m_font_color_A != rh.m_font_color_A) { return false; }
		if (this->m_font_color_R != rh.m_font_color_R) { return false; }
		if (this->m_font_color_G != rh.m_font_color_G) { return false; }
		if (this->m_font_color_B != rh.m_font_color_B) { return false; }
		if (this->m_font_style != rh.m_font_style) { return false; }
		if (this->m_font_name != rh.m_font_name) { return false; }

		return true;
	}
	inline void ResetAllParamByDefault() {
		m_text = L"";
		m_font_name = L"Arial";
		m_font_size = 20;
		m_text_rotation = 45;
		m_font_color_A = 50;
		m_font_color_R = 75;
		m_font_color_G = 75;
		m_font_color_B = 75;
		m_font_style = WatermarkParam::FontStyle::FS_Regular;
		m_line_alignment = WatermarkParam::TextAlignment::TA_Centre;
		m_text_alignment = WatermarkParam::TextAlignment::TA_Centre;

	}

private:
	std::wstring m_text;
	int m_text_rotation;  //i.e.  -10, -90, 10, 30, 45
	TextAlignment m_text_alignment;
	TextAlignment m_line_alignment;
	int m_font_size;
	// RGB[0,0,0] is black, RGB[255,255,255] is white
	unsigned char m_font_color_A; // [0,255]: 0: fully transparent, 255: opacity
	unsigned char m_font_color_R;	// [0,255]  
	unsigned char m_font_color_G; // [0,255]
	unsigned char m_font_color_B; // [0,255]
	FontStyle m_font_style;
	std::wstring m_font_name;
};

class WatermarkParamBuilder {
	WatermarkParam _config;
public:
	WatermarkParamBuilder() { _config.ResetAllParamByDefault(); }

	inline WatermarkParamBuilder& text(const std::wstring& str) {
		_config.m_text = str;
		return *this;
	}

	inline WatermarkParamBuilder& font_name(const std::wstring& font_name) {
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

	inline WatermarkParamBuilder& font_size(int size) {
		if (size <= 0 || size > 100) {
			size = 20;// bydefault
		}
		_config.m_font_size = size;
		return *this;
	}
	inline WatermarkParamBuilder& text_rotation(int rotation) {
		_config.m_text_rotation = rotation;
		return *this;
	}

	inline WatermarkParamBuilder& text_transparentcy(unsigned char A) {
		_config.m_font_color_A = A;
		return *this;
	}

	inline WatermarkParamBuilder& text_color(unsigned char R, unsigned char G, unsigned char B) {
		_config.m_font_color_R = R;
		_config.m_font_color_G = G;
		_config.m_font_color_B = B;
		return *this;
	}

	inline WatermarkParamBuilder& text_color(unsigned char A, unsigned char R, unsigned char G, unsigned char B) {
		_config.m_font_color_A = A;
		_config.m_font_color_R = R;
		_config.m_font_color_G = G;
		_config.m_font_color_B = B;
		return *this;
	}

	inline WatermarkParamBuilder& text_color(DWORD32 ARGB) {
		return text_color(LOBYTE(ARGB >> 24), LOBYTE(ARGB >> 16), LOBYTE(ARGB >> 8), LOBYTE(ARGB));
	}


	inline WatermarkParamBuilder& font_style(WatermarkParam::FontStyle fs) {
		_config.m_font_style = fs;
		return *this;
	}

	inline WatermarkParamBuilder& text_alignment(WatermarkParam::TextAlignment	alignment) {
		_config.m_text_alignment = alignment;
		return *this;
	}

	inline WatermarkParamBuilder& line_alignment(WatermarkParam::TextAlignment	alignment) {
		_config.m_line_alignment = alignment;
		return *this;
	}

	inline WatermarkParam Build() {
		ThrowIfInvalidParam();
		return _config;
	}
private:
	inline void ThrowIfInvalidParam() {
		if (_config.m_text.length() < 1) {
			throw std::exception("too little chars in watermark string");
		}
	}
	inline std::wstring GetDefaultFontName() {
		// using Gdipuls provided default 
		std::vector<wchar_t> buf(0x30, 0);
		Gdiplus::FontFamily::GenericSansSerif()->GetFamilyName(buf.data(), LANG_NEUTRAL);
		return std::wstring(buf.data());
	}
	inline bool IsFontNameSupported(const std::wstring& font_name) {
		auto cont = GetInstalledFonts();
		for (auto& i : cont) {
			if (iequal(i, font_name)) {
				return true;
			}
		}
		return false;
	}
	inline bool iequal(const std::wstring& l, const std::wstring& r) {
		if (l.size() != r.size()) { return false; }

		return std::equal(l.begin(), l.end(), r.begin(), r.end(), [](wchar_t i, wchar_t j) {
			return std::tolower(i) == std::tolower(j);
		});

	}
	inline std::vector<std::wstring> GetInstalledFonts()
	{
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
};


void draw_overlay(HDC hdc, const WatermarkParam& param);


END_NXOVERLAY_NAMESPACE