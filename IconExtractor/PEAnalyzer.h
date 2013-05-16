#pragma once
#include <memory>
#include <string>
#include <type_traits>

using std::unique_ptr;
using std::wstring;
using std::remove_pointer;

#pragma pack(push, 1)
struct ICONIMAGE
{
   BITMAPINFOHEADER   icHeader;      // DIB header
   RGBQUAD         icColors[1];   // Color table
   BYTE            icXOR[1];      // DIB bits for XOR mask
   BYTE            icAND[1];      // DIB bits for AND mask
};
struct GRPICONDIRENTRY
{
   BYTE   bWidth;               // Width, in pixels, of the image
   BYTE   bHeight;              // Height, in pixels, of the image
   BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
   BYTE   bReserved;            // Reserved
   WORD   wPlanes;              // Color Planes
   WORD   wBitCount;            // Bits per pixel
   DWORD   dwBytesInRes;         // how many bytes in this resource?
   WORD   nID;                  // the ID
};

struct GRPICONDIR
{
   WORD            idReserved;   // Reserved (must be 0)
   WORD            idType;       // Resource type (1 for icons)
   WORD            idCount;      // How many images?
   GRPICONDIRENTRY   idEntries[1]; // The entries for each image
};

struct PNGHEADER {
	unsigned marker:8;
	unsigned magic:24;
	unsigned crlf:16;
	unsigned stop:8;
	unsigned lf:8;
};

struct PNGCHUNK {
	DWORD length;
	DWORD type;
};

struct IHDR:
	public PNGCHUNK
{
	DWORD width;
	DWORD height;
	BYTE biDepth;
	BYTE colorType;
	BYTE compression;
	BYTE filter;
	BYTE interlace;
};
#pragma pack(pop)

class PEAnalyzer
{
public:
	PEAnalyzer(const wstring& path, int width = 0);
	~PEAnalyzer(void);

private:
	wstring m_path;

	HMODULE m_hModule;
	const GRPICONDIRENTRY* m_pIconEntry;

	const ICONIMAGE* m_pIcon;

	void SaveAsIcon(const wstring& path);
	void SaveAsPng(const wstring& path);

public:
	/// <summary>
	/// Saves the desired frame index of the specified icon
	/// </summary>
	void Save(const wstring& path);
};

