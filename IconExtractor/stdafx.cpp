#include "stdafx.h"
#include <memory>
#include <stdexcept>

using namespace Gdiplus;
using namespace std;

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
