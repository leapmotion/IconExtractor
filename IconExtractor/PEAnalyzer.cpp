#include "stdafx.h"
#include "PEAnalyzer.h"
#include <gdiplus.h>
#include <vector>

using namespace std;

class AutoHandle
{
public:
	AutoHandle(HANDLE hHnd):
		m_hHnd(hHnd)
	{}
	~AutoHandle(void) {
		if(m_hHnd)
			CloseHandle(m_hHnd);
	}

	HANDLE m_hHnd;
};

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
	HBITMAP bitmap = iconInfo.hbmColor;
	LPCWSTR filename = path.c_str();
	HDC hDC = nullptr;

	BITMAP bmp; 
	WORD cClrBits; 
	HANDLE hf; // file handle 
	DWORD dwTotal; // total count of bytes 
	DWORD cb; // incremental count of bytes 
	DWORD dwTmp;

	// create the bitmapinfo header information
	if(!GetObject(bitmap, sizeof(BITMAP), (LPSTR)&bmp))
		throw runtime_error("Could not retrieve bitmap info");

	// Convert the color format to a count of bits. 
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
	if (cClrBits == 1) 
		cClrBits = 1; 
	else if (cClrBits <= 4) 
		cClrBits = 4; 
	else if (cClrBits <= 8) 
		cClrBits = 8; 
	else if (cClrBits <= 16) 
		cClrBits = 16; 
	else if (cClrBits <= 24) 
		cClrBits = 24; 
	else cClrBits = 32; 




	// Allocate memory for the BITMAPINFO structure.
	PBITMAPINFO pbmi;
	PBITMAPINFOHEADER pbih; // bitmap info-header 
	if (cClrBits != 24)
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<< cClrBits)); 
	else 
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER)); 

	// Initialize the fields in the BITMAPINFO structure. 

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	pbmi->bmiHeader.biWidth = bmp.bmWidth; 
	pbmi->bmiHeader.biHeight = bmp.bmHeight; 
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
	if (cClrBits < 24) 
		pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

	// If the bitmap is not compressed, set the BI_RGB flag. 
	pbmi->bmiHeader.biCompression = BI_RGB; 

	// Compute the number of bytes in the array of color 
	// indices and store the result in biSizeImage. 
	pbmi->bmiHeader.biSizeImage = (pbmi->bmiHeader.biWidth + 7) / 8 * pbmi->bmiHeader.biHeight * cClrBits; 

	// Set biClrImportant to 0, indicating that all of the 
	// device colors are important. 
	pbmi->bmiHeader.biClrImportant = 0; 

	pbih = (PBITMAPINFOHEADER) pbmi; 
	
	vector<BYTE> bits(pbih->biSizeImage);

	// Retrieve the color table (RGBQUAD array) and the bits 
	if(!GetDIBits(hDC, bitmap, 0, pbih->biHeight, bits.data(), pbmi, DIB_RGB_COLORS))
		throw runtime_error("writeBMP::GetDIB error");

	// Now open file and save the data
	hf = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr); 
	if(hf == INVALID_HANDLE_VALUE)
		throw runtime_error("Could not create file for writing");

	BITMAPFILEHEADER hdr; // bitmap file-header 
	hdr.bfType = 'MB'; // 0x42 = "B" 0x4d = "M"

	// Compute the size of the entire file. 
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed  * sizeof(RGBQUAD) + pbih->biSizeImage);
	hdr.bfReserved1 = 0; 
	hdr.bfReserved2 = 0; 

	// Compute the offset to the array of color indices. 
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD); 

	// Copy the BITMAPFILEHEADER into the .BMP file. 
	if (!WriteFile(hf, &hdr, sizeof(BITMAPFILEHEADER), &dwTmp, NULL))
			throw runtime_error("Could not write in to file");

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
	if (!WriteFile(hf, pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD),  &dwTmp, nullptr))
		throw runtime_error("Could not write in to file");

	// Copy the array of color indices into the .BMP file. 
	dwTotal = cb = pbih->biSizeImage; 
	if(!WriteFile(hf, bits.data(), cb, &dwTmp, nullptr))
		throw runtime_error("Could not write in to file");
}