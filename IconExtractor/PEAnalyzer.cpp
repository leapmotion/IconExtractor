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
	m_hModule(nullptr),
	m_hMainIconRsrc(nullptr)
{
	memset(&iconInfo, 0, sizeof(iconInfo));

	m_hModule = LoadLibraryEx(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
	if(!m_hModule)
		throw std::runtime_error("Failed to load the specified library");

	// Determine the first-mentioned icon in the EXE:
	wstring firstName;
	EnumResourceNames(
		m_hModule,
		RT_GROUP_ICON,
		[] (HMODULE hModule, _In_ LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) -> BOOL {
			*(wstring*)lParam = lpName;
			return false;
		},
		(LONG_PTR)&firstName
	);

	// Obtain the main icon:
	m_hMainIconRsrc = (HICON)LoadImage(m_hModule, firstName.c_str(), IMAGE_ICON, width, width, 0);
	if(!m_hMainIconRsrc)
		throw std::runtime_error("Failed to find the main icon");

	// Obtain the bitmaps for this icon:
	iconInfo.cbSize = sizeof(iconInfo);
	if(!GetIconInfoEx(m_hMainIconRsrc, &iconInfo))
		throw std::runtime_error("Failed to obtain icon information");
}

PEAnalyzer::~PEAnalyzer(void)
{
	FreeLibrary(m_hModule);
	if(m_hMainIconRsrc)
		DestroyIcon(m_hMainIconRsrc);
	if(iconInfo.hbmColor)
		DeleteObject(iconInfo.hbmColor);
	if(iconInfo.hbmMask)
		DeleteObject(iconInfo.hbmMask);
}

void PEAnalyzer::Save(const wstring& path) {
	// Get the real size of the icon
	BITMAP bmpObj;
	GetObject(iconInfo.hbmColor, sizeof(bmpObj), &bmpObj);

	// Create the bitmap and the bitmap's mask:
	Bitmap bmp(iconInfo.hbmColor, nullptr);
	Bitmap bmpMask(iconInfo.hbmMask, nullptr);
	Bitmap bmpDest(bmp.GetWidth(), bmp.GetHeight(), PixelFormat32bppARGB);

	BitmapData colorData;
	BitmapData maskData;
	BitmapData destData;
	{
		Rect bounds(0, 0, bmp.GetWidth(), bmp.GetHeight());
		bmp.LockBits(&bounds, ImageLockModeRead | ImageLockModeWrite, bmp.GetPixelFormat(), &colorData);
		bmpMask.LockBits(&bounds, ImageLockModeRead | ImageLockModeWrite, bmp.GetPixelFormat(), &maskData);
		bmpDest.LockBits(&bounds, ImageLockModeRead | ImageLockModeWrite, bmp.GetPixelFormat(), &destData);
	}

	// Manual mask operation:
	union
	{
		const char* pColor;
		const DWORD* pColorPixel;
	};
	pColor = (char*)colorData.Scan0 + colorData.Height * colorData.Stride;

	union
	{
		const char* pMask;
		const DWORD* pMaskPixel;
	};
	pMask = (char*)maskData.Scan0 + maskData.Height * maskData.Stride + 4;

	union
	{
		char* pDest;
		DWORD* pDestPixel;
	};
	pDest = (char*)destData.Scan0 + destData.Height * destData.Stride;

	for(size_t y = colorData.Height; y--;) {
		pColor -= colorData.Stride;
		pMask -= maskData.Stride;
		pDest -= destData.Stride;
		for(size_t x = colorData.Width; x--;)
			if(pMaskPixel[x] & 0xFFFFFF)
				pDestPixel[x] = 0x00FF00FF;
			else
				pDestPixel[x] = pColorPixel[x] | 0xFF000000;
	}

	// Relese everything:
	bmp.UnlockBits(&colorData);
	bmpMask.UnlockBits(&maskData);
	bmpDest.UnlockBits(&destData);

	CLSID pngClsid;
	GetEncoderClsid(L"image/png", pngClsid);
	Gdiplus::Status stat = bmpDest.Save(path.c_str(), &pngClsid, nullptr);
	switch(stat) {
	case Gdiplus::Ok:
		break;
	case Gdiplus::InvalidParameter:
		throw runtime_error("GDI+ reports that an invalid parameter was passed when attempting to serialize an image");
	default:
		throw runtime_error("GDI+ returned a failure condition when attempting to serialize the output image");
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
