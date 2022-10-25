#include "stdafx.h"
#include "utils.h"
#include "overlay.h"


using namespace Gdiplus;

NXOVERLAY_NAMESPACE

#ifdef _DEBUG

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
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

bool SaveFileAsBitmap(Gdiplus::Image* image, std::wstring path)
{

	CLSID clsid;
	GetEncoderClsid(L"image/bmp", &clsid);

	image->Save(path.c_str(), &clsid);

	return false;
}

#endif // DEBUG

//// calculate the size art-text used
//Gdiplus::SizeF CalcTextSizeF(const Gdiplus::Font& font, const Gdiplus::StringFormat& strFormat, const std::wstring& szText)
//{
//	using namespace Gdiplus;
//	Gdiplus::GraphicsPath graphicsPathObj;
//	Gdiplus::FontFamily fontfamily;
//	font.GetFamily(&fontfamily);
//	graphicsPathObj.AddString(szText.c_str(), -1, &fontfamily, font.GetStyle(), font.GetSize(), PointF(0, 0), &strFormat);
//	Gdiplus::RectF rcBound;
//	graphicsPathObj.GetBounds(&rcBound);
//	return Gdiplus::SizeF(rcBound.Width, rcBound.Height);
//}

Gdiplus::SizeF CalcTextSizeF(const Gdiplus::Graphics& drawing_surface, const std::wstring& szText, const Gdiplus::StringFormat& strFormat, const Font& font)
{
	Gdiplus::RectF rcBound;
	drawing_surface.MeasureString(szText.c_str(), -1, &font, Gdiplus::PointF(0, 0), &strFormat, &rcBound);
	return Gdiplus::SizeF(rcBound.Width, rcBound.Height);
}

Gdiplus::PointF CaculateRotated(Gdiplus::PointF& org, int angle)
{
	static const double PI = std::acos(-1);
	Gdiplus::PointF rt;

	double radians = angle * PI / 180;

#pragma warning(push)
#pragma warning(disable: 4244)
	rt.X = org.X * std::cos(radians) - org.Y * std::sin(radians);
	rt.Y = org.X * std::sin(radians) + org.Y * std::cos(radians);
#pragma warning(pop)
	return rt;
}

Gdiplus::RectF CaculateOutbound(Gdiplus::PointF(&org)[4])
{

	std::vector<Gdiplus::REAL> Xs, Ys;
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

Gdiplus::RectF CalculateMinimumEnclosingRectAfterRotate(const Gdiplus::SizeF& size, int rotate)
{
	PointF org[4] = {
		{0,0},{0,size.Height},{size.Width, size.Height},  {size.Width, 0}
	};

	PointF org_r[4];
	for (int i = 0; i < 4; i++) {
		org_r[i] = CaculateRotated(org[i], rotate);
	}

	return CaculateOutbound(org_r);
}

Gdiplus::Bitmap* DrawOverlayBitmap(const Gdiplus::Graphics& drawing_surface, const WatermarkParam& param)
{

	Gdiplus::FontFamily fontfamily(param.font_name().c_str());
	Gdiplus::Font font(&fontfamily, (REAL)param.font_size(), param.font_style(), Gdiplus::UnitPixel);
	Gdiplus::SolidBrush brush(param.font_gdi_color());
	Gdiplus::StringFormat str_format; {
		str_format.SetAlignment(param.text_alignment());
		str_format.SetLineAlignment(param.line_alignment());
	}
	Gdiplus::SizeF str_size = CalcTextSizeF(drawing_surface, param.text(), str_format, font);
	Gdiplus::RectF str_enclosing_rect = CalculateMinimumEnclosingRectAfterRotate(str_size, param.text_rotation());

	Gdiplus::REAL surface_size = 2 * std::ceil(std::hypot(str_size.Width, str_size.Height));

	Gdiplus::Bitmap bitmap_canvas((INT)surface_size, (INT)surface_size, PixelFormat32bppARGB);
	Gdiplus::Graphics g(&bitmap_canvas);
	// make a good quality
	//g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	// set centre point as the base point
	g.TranslateTransform(surface_size / 2, surface_size / 2);
	g.RotateTransform((REAL)param.text_rotation());
	// set string
	g.DrawString(param.text().c_str(), -1, &font,
		Gdiplus::RectF(0, 0, str_size.Width, str_size.Height),
		&str_format, &brush);
	g.ResetTransform();
	g.Flush();

	// since drawing org_point is the centre, 
	Gdiplus::RectF absolute_layout = str_enclosing_rect;
	absolute_layout.Offset(surface_size / 2, surface_size / 2);


	// request bitmap is the partly clone with absolute_layout
	return bitmap_canvas.Clone(absolute_layout, PixelFormat32bppARGB);
}

//void to_bitmap(Gdiplus::Graphics& g, std::wstring path) {
//
//	HDC hdc = g.GetHDC();
//
//	int dwdeviceWidth = ::GetDeviceCaps(hdc, PHYSICALWIDTH);
//	int dwdeviceHigh = ::GetDeviceCaps(hdc, PHYSICALHEIGHT);
//
//	Gdiplus::Bitmap bm(dwdeviceWidth, dwdeviceHigh, &g);
//
//	SaveFileAsBitmap(&bm, path);
//
//}

void draw_overlay(HDC hdc, const WatermarkParam& param)
{
	using namespace Gdiplus;
	if (hdc == NULL) return;
	if (param.text().empty()) return;

	int dwdeviceWidth = ::GetDeviceCaps(hdc, PHYSICALWIDTH);
	int dwdeviceHigh = ::GetDeviceCaps(hdc, PHYSICALHEIGHT);

	Gdiplus::RectF canvas(0, 0, (REAL)dwdeviceWidth, (REAL)dwdeviceHigh);
	Gdiplus::Graphics g(hdc);
	g.SetPageUnit(Gdiplus::UnitPoint);
	//g.SetSmoothingMode(SmoothingModeHighQuality);
	//g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::Bitmap* bm_overlay = DrawOverlayBitmap(g, param);

	if (bm_overlay == NULL) {
		DEVLOG(L"bm_overlay is NULL in draw_overlay");
		return;
	}

	//#ifdef _DEBUG
	//		SaveFileAsBitmap(bm_overlay, LR"(D:\allTestFile\bitmap.bmp)");
	//#endif // _DEBUG


	Gdiplus::TextureBrush brush(bm_overlay, Gdiplus::WrapModeTile);
	g.FillRectangle(&brush, canvas);
	delete bm_overlay;
	//#ifdef  _DEBUG
	//	to_bitmap(g, LR"(D:\allTestFile\bitmap2.bmp)");
	//#endif //  _DEBUG

}


END_NXOVERLAY_NAMESPACE