#include "stdafx.h"
#include "PEAnalyzer.h"
#include <Gdiplusimaging.h>
#include <fstream>
#include <vector>

using namespace Gdiplus;
using namespace std;

#define FLIP(x) ((x >> 24) | (((x >> 16) & 0xFF) << 8) | (((x >> 8) & 0xFF) << 16) | ((x & 0xFF) << 24))

PEAnalyzer::PEAnalyzer(const wstring& path, size_t width):
	m_path(path),
	m_hModule(nullptr),
	m_pIcon(nullptr),
	m_pIconEntry(nullptr)
{
	m_hModule = LoadLibraryEx(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
	if(!m_hModule)
		throw std::runtime_error("Failed to load the specified library");

	// Determine the first-mentioned icon in the EXE:
	LPWSTR firstName = nullptr;
	EnumResourceNames(
		m_hModule,
		RT_GROUP_ICON,
		[] (HMODULE hModule, _In_ LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) -> BOOL {
			*(LPCWSTR*)lParam =
				IS_INTRESOURCE(lpName) ?
				lpName :
				_wcsdup(lpName);
			return false;
		},
		(LONG_PTR)&firstName
	);

	// Lock down the resource proper:
	const GRPICONDIR* pIconGroup;
	{
		HRSRC hResource = FindResource(m_hModule, firstName, RT_GROUP_ICON);
		HGLOBAL hGlobal = LoadResource(m_hModule, hResource);
		pIconGroup = (GRPICONDIR*)LockResource(hGlobal);
		if(!pIconGroup)
			throw runtime_error("Failed to lock the global resource");
	}

	// Validate the icon structure:
	if(pIconGroup->idReserved)
		throw runtime_error("Icon structure appears to be corrupted");
	if(pIconGroup->idType != 1)
		throw runtime_error("Unrecognized icon type");

	// Find the largest icon not larger than the requested size:
	size_t i;
	size_t iClosest = 0;
	size_t bestWidth = 0;
	for(i = pIconGroup->idCount; i--;) {
		// Now load the descendant resource:
		HRSRC hTargetIcon = FindResource(m_hModule, MAKEINTRESOURCE(pIconGroup->idEntries[i].nID), RT_ICON);
		HGLOBAL hGlobal = LoadResource(m_hModule, hTargetIcon);
		ICONIMAGE* pIcon = (ICONIMAGE*)LockResource(hGlobal);
		if(!pIcon)
			throw runtime_error("Failed to obtain an icon by its resource identifier, as described by the RT_GROUP_ICON structure");

		size_t curWidth;
		if(pIconGroup->idEntries[i].bWidth)
			curWidth = pIcon->icHeader.biWidth;
		else {
			// PNG image, process the header:
			PNGHEADER& hdr = *(PNGHEADER*)pIcon;
			if(
				hdr.marker != 0x89 ||
				hdr.magic != 'GNP' ||
				hdr.crlf != '\r\n' ||
				hdr.stop != 0x1A ||
				hdr.lf != '\n'
			)
				throw runtime_error("Executable image has a corrupted PNG icon image header");

			// Try to find the IHDR chunk:
			auto pngChunk = (PNGCHUNK*)(&hdr + 1);
			if(pngChunk->type != 'RDHI')
				throw runtime_error("Expected teh IHDR chunk at the beginning of the PNG; was not found");
				
			// Extract width:
			IHDR* pIhdr = (IHDR*)pngChunk;
			curWidth = FLIP(pIhdr->width);
		}
		
		if(width < curWidth || curWidth < bestWidth)
			continue;

		m_pIcon = pIcon;
		m_pIconEntry = &pIconGroup->idEntries[i];
		bestWidth = curWidth;
	}
}

PEAnalyzer::~PEAnalyzer(void)
{
	FreeLibrary(m_hModule);
}

void PEAnalyzer::SaveAsIcon(const wstring& path)
{
	// Create the bitmap and the bitmap's mask:
	const auto& hdr = m_pIcon->icHeader;

	vector<DWORD> rgba(hdr.biWidth * hdr.biHeight);
	const BYTE* pMaskPayload = (BYTE*)&m_pIcon->icColors[hdr.biClrUsed] + hdr.biWidth * hdr.biHeight * hdr.biBitCount / (8 * 2);
	DWORD maskStride = 4 * ((hdr.biWidth + 31) / 32);

	// Fix up the mask map:
	BYTE maskMap[8];
	for(size_t i = 8; i--;)
		maskMap[i] = 1 << (7 - i);

	// Construct image:
	DWORD* pDest = rgba.data();
	switch(hdr.biBitCount) {
	case 8:
		{
			const DWORD* pMappingTable = (DWORD*)m_pIcon->icColors;
			const BYTE* pColorPayload = (BYTE*)&m_pIcon->icColors[m_pIcon->icHeader.biClrUsed];
			for(size_t y = hdr.biHeight / 2; y--;)
			{
				for(size_t x = hdr.biWidth; x--;)
					if(pMaskPayload[x >> 3] & maskMap[x & 7])
						pDest[x] = 0;
					else
						pDest[x] = 0xFF000000 | pMappingTable[pColorPayload[x]];
				pColorPayload += hdr.biWidth;
				pMaskPayload += maskStride;
				pDest += hdr.biWidth;
			}
		}
		break;
	case 32:
		{
			const DWORD* pColorPayload = (DWORD*)m_pIcon->icColors;
			for(size_t y = hdr.biHeight / 2; y--;)
			{
				for(size_t x = hdr.biWidth; x--;)
					if(pMaskPayload[x >> 3] & maskMap[x & 7])
						pDest[x] = 0;
					else
						pDest[x] = 0xFF000000 | pColorPayload[x];
				pColorPayload += hdr.biWidth;
				pMaskPayload += maskStride;
				pDest += hdr.biWidth;
			}
		}
		break;
	case 24:
		throw runtime_error("24-bits-per-pixel icon resources are currently unsupported");
	default:
		throw runtime_error("Embedded icon at the requested size has an unsupported bit depth");
	}
	
	// Top half of the bitmap:
	Bitmap bmp(
		hdr.biWidth,
		hdr.biHeight / 2,
		hdr.biWidth * 4,
		PixelFormat32bppARGB,
		(BYTE*)rgba.data()
	);

	CLSID pngClsid;
	GetEncoderClsid(L"image/png", pngClsid);
	Gdiplus::Status stat = bmp.Save(path.c_str(), &pngClsid, nullptr);
	switch(stat) {
	case Gdiplus::Ok:
		break;
	case Gdiplus::InvalidParameter:
		throw runtime_error("GDI+ reports that an invalid parameter was passed when attempting to serialize an image");
	case Gdiplus::Win32Error:
		throw runtime_error("A Win32 error occurred while attempting to serialize the image");
	default:
		throw runtime_error("GDI+ returned a failure condition when attempting to serialize the output image");
	}
}

void PEAnalyzer::SaveAsPng(const wstring& path)
{
	// Create the bitmap and the bitmap's mask:
	const auto& hdr = m_pIcon->icHeader;

	DWORD dwWritten;
	HANDLE hFile = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	WriteFile(hFile, m_pIcon, m_pIconEntry->dwBytesInRes, &dwWritten, nullptr);
	CloseHandle(hFile);
}

void PEAnalyzer::Save(const wstring& path)
{
	if(m_pIconEntry->bWidth)
		SaveAsIcon(path);
	else
		SaveAsPng(path);
}