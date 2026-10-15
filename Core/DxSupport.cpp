#include "pch.h"

#include "DebugStream.h"
#include "DxSupport.h"
#include "SafeRelease.h"

// Additional library for the DirectDraw functions
#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")

CDxSupport::CDxSupport() :
	m_pDirectDraw7(nullptr),
	m_bHasHardwareOverlay(false),
	m_hDirectDraw(::LoadLibrary(TEXT("ddraw.dll")))
{
	HRESULT hr;

	if (nullptr == m_hDirectDraw)
	{
		wdebugstream() << TEXT("DEBUG: Failed to load the DDRAW.DLL") << std::endl;
	}
	else if (FAILED(hr = m_pDirectDraw7.CreateInstance(CLSID_DirectDraw7)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create DirectX COM pointer (hr = 0x") << std::hex << hr << (")") << std::endl;
	}
	else
	{
		DDCAPS hal_caps, hel_caps;
		HRESULT hr;

		::ZeroMemory(&hal_caps, sizeof(DDCAPS));
		::ZeroMemory(&hel_caps, sizeof(DDCAPS));

		hal_caps.dwSize = sizeof(DDCAPS);
		hel_caps.dwSize = sizeof(DDCAPS);

		if (FAILED(hr = m_pDirectDraw7->Initialize(nullptr)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to initialise DirectX") << std::endl;
		}
		else if (FAILED(hr = m_pDirectDraw7->GetCaps(&hal_caps, &hel_caps)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to retrieve DirectX capabilities (GetCaps failed)") << std::endl;
		}
		else
		{
			m_bHasHardwareOverlay = ((0 < hal_caps.dwMaxVisibleOverlays) ? true : false);
			wdebugstream() << TEXT("DEBUG: Successfully determined DirectX capabilities") << std::endl;
		}
	}
}

CDxSupport::~CDxSupport()
{
	SafeRelease(&m_pDirectDraw7);

	if (nullptr != m_hDirectDraw) ::FreeLibrary(m_hDirectDraw);
}

bool CDxSupport::SupportsHardwareOverlay() const
{
	return m_bHasHardwareOverlay;
}
