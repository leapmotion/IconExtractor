#include "stdafx.h"
#include "PEAnalyzer.h"
#include <gdiplus.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace Gdiplus;
using namespace std;

void PrintUsage(wchar_t* argv[]) {
	// EXE name first, followed by the desired dimensions
	cout << "Usage:" << endl
			<< argv[0] << " <img> <outname> [width]" << endl
			<< " If the width is set to -1, or is omitted, the largest available image will be returned instead" << endl
			<< " The width must not be zero" << endl;
}

class GdiPlusInitializer {
public:
	GdiPlusInitializer(void)
	{
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	}

	~GdiPlusInitializer(void)
	{
		GdiplusShutdown(gdiplusToken);
	}

private:
	ULONG_PTR gdiplusToken;
};
GdiPlusInitializer g_initializer;

int wmain(int argc, wchar_t* argv[])
{
	// Argument parsing
	wstring imgName;
	wstring outName = L"out.png";
	size_t width = -1;
	switch(argc) {
	case 4:
		if(
			(wstringstream(argv[3]) >> width) &&
			width
		)
		{
	case 3:
			outName = argv[2];
	case 2:
			imgName = argv[1];
			break;
		}
	default:
		return PrintUsage(argv), -1;
	}

	// Try to construct an extractor:
	try {
		PEAnalyzer analyzer(imgName, width);
		analyzer.Save(outName);
	} catch(std::exception& ex) {
		cout << ex.what() << endl;
		return -1;
	}

	return 0;
}

