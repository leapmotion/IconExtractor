#pragma once
#include <memory>
#include <string>
#include <type_traits>

using std::unique_ptr;
using std::wstring;
using std::remove_pointer;

template<class T, void fn(T*)>
class Fn
{
	T* operator()(T p) const {return fn(p);}
};

class PEAnalyzer
{
public:
	PEAnalyzer(const wstring& path, int width = 0);
	~PEAnalyzer(void);

private:
	wstring m_path;

	HMODULE m_hModule;
	ICONINFOEXW iconInfo;

public:
	/// <summary>
	/// Saves the desired frame index of the specified icon
	/// </summary>
	void Save(const wstring& path);
};

