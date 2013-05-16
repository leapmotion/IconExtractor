#pragma once

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <gdiplus.h>

void GetEncoderClsid(const WCHAR* format, CLSID& clsid);
