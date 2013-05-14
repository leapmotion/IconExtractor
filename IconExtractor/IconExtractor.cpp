#include "stdafx.h"
#include "PEAnalyzer.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

void PrintUsage(wchar_t* argv[]) {
	// EXE name first, followed by the desired dimensions
	cout << "Usage:" << endl
			<< argv[0] << " <img> [width]" << endl;
}

int wmain(int argc, wchar_t* argv[])
{
	wstring imgName;
	int width;
	switch(argc) {
	default:
		return PrintUsage(argv), -1;
	case 3:
		{
			wstringstream sstr(argv[2]);
			if(!(sstr >> width))
				return PrintUsage(argv), -1;
		} 
	case 2:
		imgName = argv[1];
	}

	// Try to construct an extractor:
	try {
		PEAnalyzer analyzer(imgName, width);

		// Iterate through known images, and find the largest image that doesn't go over
		// the requested size:
	} catch(std::exception& ex) {
		cout << ex.what() << endl;
		return -1;
	}

	return 0;
}

