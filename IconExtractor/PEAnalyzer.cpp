#include "stdafx.h"
#include "PEAnalyzer.h"
#include <gdiplus.h>
#include <Gdiplusimaging.h>
#include <vector>

using namespace Gdiplus;
using namespace std;

void GetEncoderClsid(const WCHAR* format, CLSID& clsid);

PEAnalyzer::PEAnalyzer(const wstring& path, int width):
	m_path(path),
	m_hModule(nullptr)
{
	memset(&iconInfo, 0, sizeof(iconInfo));

	m_hModule = LoadLibraryEx(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if(!m_hModule)
		throw std::runtime_error("Failed to load the specified library");

	// Obtain the main icon:
	shared_ptr<remove_pointer<HICON>::type> hMainIconRsrc(
		(HICON)LoadImage(m_hModule, L"MAINICON", IMAGE_ICON, width, 0, 0),
		DestroyIcon
	);
	if(!hMainIconRsrc)
		throw std::runtime_error("Failed to find the main icon");

	// Obtain the bitmaps for this icon:
	iconInfo.cbSize = sizeof(iconInfo);
	if(!GetIconInfoEx(hMainIconRsrc.get(), &iconInfo))
		throw std::runtime_error("Failed to obtain icon information");
}

PEAnalyzer::~PEAnalyzer(void)
{
	FreeLibrary(m_hModule);
	if(iconInfo.hbmColor)
		DeleteObject(iconInfo.hbmColor);
	if(iconInfo.hbmMask)
		DeleteObject(iconInfo.hbmMask);
}

void PEAnalyzer::Save(const wstring& path) {
	// Create the bitmap and the bitmap's mask:
	Bitmap bmp(iconInfo.hbmColor, nullptr);
	
	// Start off with the backing bitmap:
	Graphics dc(&bmp);

	ImageAttributes attr;

	{
		// The mask is monochromatic, and is an XOR mask.
		Bitmap bmpMask(iconInfo.hbmMask, nullptr);

		HBITMAP compatBitmap;
		{
			HDC hDc = dc.GetHDC();
			compatBitmap = CreateCompatibleBitmap(hDc, bmp.GetWidth(), bmp.GetHeight());
			dc.ReleaseHDC(hDc);
		}

		Bitmap bmpMaskCompatible(compatBitmap, nullptr);
		Graphics maskDC(&bmpMaskCompatible);

		maskDC.DrawImage(&bmpMask, 0, 0, 0, 0, bmpMask.GetWidth(), bmpMask.GetHeight(), UnitPixel);

		Pen blackPen(Color(255, 0, 0, 0), 3);
		dc.DrawLine(&blackPen, Point(10, 10), Point(20, 20));
		
		HDC hDst = dc.GetHDC();
		HDC hSrc = maskDC.GetHDC();
		BitBlt(
			hDst,
			0,
			0,
			bmp.GetWidth(),
			bmp.GetHeight(),
			hSrc,
			0,
			0,
			SRCCOPY
		);
		dc.ReleaseHDC(hDst);
		maskDC.ReleaseHDC(hSrc);
	}

	{
		CLSID pngClsid;
		GetEncoderClsid(L"image/png", pngClsid);
		Gdiplus::Status stat = bmp.Save(path.c_str(), &pngClsid, nullptr);
		switch(stat) {
		case Gdiplus::Ok:
			break;
		case Gdiplus::InvalidParameter:
			throw runtime_error("GDI+ reports that an invalid parameter was passed when attempting to serialize an image");
		default:
			throw runtime_error("GDI+ returned a failure condition when attempting to serialize the output image");
		}
	}
}

void GetEncoderClsid(const WCHAR* format, CLSID& clsid)
{
	UINT num = 0;
	UINT size = 0;
	GetImageEncodersSize(&num, &size);
	if(!size)
		throw std::runtime_error("Failed to obtain the number of image encoders");

	unique_ptr<ImageCodecInfo> imageCodecs((ImageCodecInfo*)new char[size]);
	memset(imageCodecs.get(), 0, size);
	GetImageEncoders(num, size, imageCodecs.get());

	for(size_t i = num; i--; )
		if(!wcscmp(imageCodecs.get()[i].MimeType, format))
		{
			clsid = imageCodecs.get()[i].Clsid;
			return;
		}

	throw std::runtime_error("Failed to obtain the number of image encoders");
}
