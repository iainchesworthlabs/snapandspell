#include "Stdafx.h"

#include "CaptureManager.h"
#include "cvmarkersobj.h"

namespace WordJewelsImager
{

CaptureManager::CaptureManager() : m_CaptureEngine(), m_hVideoWnd(nullptr), m_rcDest()
{
}

CaptureManager::CaptureManager(HWND hVideoWnd) : m_CaptureEngine(), m_hVideoWnd(hVideoWnd), m_rcDest()
{
}

CaptureManager::~CaptureManager()
{
}

void CaptureManager::SetVideoWindow(HWND hVideoWnd)
{
	if (nullptr == hVideoWnd)
	{
		wdebugstream() << TEXT("DEBUG: Cannot use a NULL window handle") << std::endl;
		throw;
	}
	else
	{
		m_hVideoWnd = hVideoWnd;
	}
}

void CaptureManager::DisplayVideo()
{
	uint64_t width, height; 
	HRESULT hr;

	if (0 >= m_CaptureEngine.AvailableDeviceCount())
	{
		wdebugstream() << TEXT("DEBUG: No devices are available for video display") << std::endl;
	}
	else if (m_CaptureEngine.IsRunning())
	{
		wdebugstream() << TEXT("DEBUG: Capture engine is not running...do nothing") << std::endl;
	}
	else if (FAILED(hr = m_CaptureEngine.InitialiseRenderer(m_hVideoWnd)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to initialise the video playback window") << std::endl;
	}
	else if (FAILED(hr = m_CaptureEngine.SelectVideoDevice(0))) /// FIXME remove need to hardcode first device.
	{
		wdebugstream() << TEXT("DEBUG: Failed to select the video device for display") << std::endl;
	}
	else if (FAILED(hr = m_CaptureEngine.GetNativeVideoSize(width, height)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to determine native video size") << std::endl;
	}
	else 
	{
		RECT rcSrc; 

		// Set the source rectangle.
		::SetRect(&rcSrc, 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)); 

		// Get the window client area.
		::GetClientRect(m_hVideoWnd, &m_rcDest); 

		// Set the destination rectangle.
		::SetRect(&m_rcDest, 0, 0, m_rcDest.right, m_rcDest.bottom);
		
		if (FAILED(hr = m_CaptureEngine.SetVideoPosition(m_hVideoWnd, m_rcDest)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to set video window to native window extents") << std::endl;
		}
		else if (FAILED(hr = m_CaptureEngine.Start()))
		{
			wdebugstream() << TEXT("DEBUG: Failed to start the capture engine") << std::endl;
		}
		else
		{
			wdebugstream() << TEXT("DEBUG: Successfully bound, initialised video stream") << std::endl;
		}
	}
}

void CaptureManager::DisplayModeChanged()
{
	if (!m_CaptureEngine.IsRunning())
	{
		wdebugstream() << TEXT("") << std::endl;
	}
	else
	{
		m_CaptureEngine.DisplayModeChanged();
	}
}

Concurrency::diagnostic::marker_series mySeries;

void CaptureManager::RepaintVideo()
{
	PAINTSTRUCT ps;
	RECT rcClient; 
	HRESULT hr;
	HDC hDC;

	::GetClientRect(m_hVideoWnd, &rcClient); 
	hDC = ::BeginPaint(m_hVideoWnd, &ps); 
	
	if (m_CaptureEngine.IsRunning()) 
	{ 
		Concurrency::diagnostic::span s(mySeries, TEXT("void CaptureManager::RepaintVideo()"));

		// Find the region where the application can paint by subtracting 
		// the video destination rectangle from the client area.
		
		// (Assume that m_rcDest was calculated previously.)

		HRGN rgnClient = ::CreateRectRgnIndirect(&rcClient);
		HRGN rgnVideo  = ::CreateRectRgnIndirect(&m_rcDest);
		::CombineRgn(rgnClient, rgnClient, rgnVideo, RGN_DIFF);  

		// Paint on window.
		HBRUSH hbr = ::GetSysColorBrush(COLOR_BTNFACE); 
		::FillRgn(hDC, rgnClient, hbr); 

		// Clean up.
		::DeleteObject(hbr); 
		::DeleteObject(rgnClient); 
		::DeleteObject(rgnVideo); 

		// Request the VMR to paint the video.
		if (FAILED(hr = m_CaptureEngine.RepaintVideo(m_hVideoWnd, hDC)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to repaint video window") << std::endl;
		}
	} 
	else 
	{ 
		// There is no video, so paint the whole client area. 
		::FillRect(hDC, &rcClient, reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1)); 
	} 

	::EndPaint(m_hVideoWnd, &ps);
}

void CaptureManager::SetVideoPosition()
{
}

}
// namespaceWordJewelsImager
