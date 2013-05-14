#pragma once
#include <memory>
#include <string>
#include <type_traits>

using std::unique_ptr;
using std::wstring;
using std::remove_pointer;

class PEAnalyzer
{
public:
	PEAnalyzer(const wstring& path);
	~PEAnalyzer(void);

private:
	wstring m_path;

	HMODULE m_hModule;
	HANDLE m_hAppIco;

public:
	HANDLE GetApplicationIcon(void) const {return m_hAppIco;}
};

