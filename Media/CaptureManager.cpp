#include "pch.h"

#include "CaptureManager.h"
#include "DebugStream.h"

namespace WordJewelsImager
{

CaptureManager::CaptureManager() : 
	m_AvailableDevices(),
	m_CaptureEngine(), 
	m_PreviewWindow(m_CaptureEngine),
	m_GraphDebugger(),
	m_dwGraphRegister(0),
	m_hVideoWnd(nullptr),
	m_rcDest()
{
	static const uint32_t DEFAULT_DEVICE = 0;
	CaptureEngineCOM::IGraphBuilderPtr pGraph;
	HRESULT hr;

	// Enable debugging.
	if (FAILED(hr = m_CaptureEngine.GetCaptureGraph(pGraph)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to acquire graph pointer from engine") << std::endl;
	}
	else if (FAILED(hr = GraphDebugger::AddGraphToROT(pGraph, &m_dwGraphRegister)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to register graph in Running Object Table") << std::endl;
	}
	else if (0 >= m_AvailableDevices.size())
	{
		wdebugstream() << TEXT("DEBUG: There are no capture devices available in the system") << std::endl;
	}
	else if (FAILED(hr = m_CaptureEngine.SetCaptureDevice(m_AvailableDevices[DEFAULT_DEVICE])))
	{
		wdebugstream() << TEXT("DEBUG: Failed to select the video device for display") << std::endl;
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Successfully initialised Capture Manager") << std::endl;
	}
}

CaptureManager::~CaptureManager()
{
	GraphDebugger::RemoveGraphFromROT(m_dwGraphRegister);
}

void CaptureManager::EnablePreviewVideo(HWND hVideoWnd)
{
	uint64_t width, height;
	HRESULT hr;
	RECT rcSrc;

	if (nullptr == hVideoWnd)
	{
		wdebugstream() << TEXT("DEBUG: Cannot use a NULL window handle") << std::endl;
	}
	else if (m_CaptureEngine.IsRunning())
	{
		wdebugstream() << TEXT("DEBUG: Capture engine is already running...do not try again") << std::endl;
	}
	else if (FAILED(hr = m_PreviewWindow.SetWindow(hVideoWnd)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set the preview window with supplied handle") << std::endl;
	}
	else if (FAILED(hr = m_PreviewWindow.GetNativeVideoSize(width, height)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to determine native video size") << std::endl;
	}
	else if (0 == ::SetRect(&rcSrc, 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set the source rectangle coordinates") << std::endl;
	}
	else if (0 == ::GetClientRect(hVideoWnd, &m_rcDest))
	{
		wdebugstream() << TEXT("DEBUG: Failed to get the client rectangle") << std::endl;
	}
	else if (0 == ::SetRect(&m_rcDest, 0, 0, m_rcDest.right, m_rcDest.bottom))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set destination rectangle coordinates") << std::endl;
	}
	else if (FAILED(hr = m_PreviewWindow.SetVideoPosition(hVideoWnd, m_rcDest)))
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
		
		m_hVideoWnd = hVideoWnd;
	}
}

void CaptureManager::DisplayModeChanged()
{
	if (!m_CaptureEngine.IsRunning())
	{
		wdebugstream() << TEXT("DEBUG: The engine is not running...cannot alter video display") << std::endl;
	}
	else
	{
		m_PreviewWindow.DisplayModeChanged();
	}
}

void CaptureManager::HandleGraphEvent()
{
	HRESULT hr;

	if (FAILED(hr = m_PreviewWindow.HandleGraphEvent()))
	{
		wdebugstream() << TEXT("DEBUG: Failed to handle graph event sent to window") << std::endl;
	}
	else
	{
		// Success
	}
}

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
		if (FAILED(hr = m_PreviewWindow.RepaintVideo(m_hVideoWnd, hDC)))
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
