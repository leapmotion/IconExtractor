#include "stdafx.h"
#include "PEAnalyzer.h"

PEAnalyzer::PEAnalyzer(const wstring& path, int width):
	m_path(path),
	m_hModule(nullptr),
	m_hAppIco(nullptr)
{
	m_hModule = LoadLibraryEx(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if(!m_hModule)
		throw std::runtime_error("Failed to load the specified library");

	// Extract the ICO resource from this image:
	m_hAppIco = LoadImage(
		m_hModule,
		IDI_APPLICATION,
		IMAGE_ICON,
		width,
		0,
		LR_CREATEDIBSECTION
	);
}

PEAnalyzer::~PEAnalyzer(void)
{
	if(m_hModule)
		FreeLibrary(m_hModule);
	if(m_hAppIco)
		CloseHandle(m_hAppIco);
}
