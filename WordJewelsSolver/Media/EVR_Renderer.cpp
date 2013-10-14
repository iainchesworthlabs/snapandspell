#include "StdAfx.h"

#include "DebugStream.h"
#include "EVR_Renderer.h"
#include "SafeRelease.h"

namespace WordJewelsImager
{

EVR_Renderer::EVR_Renderer() : m_pEVR(nullptr), m_pVideoDisplay(nullptr)
{
}

EVR_Renderer::~EVR_Renderer()
{
	SafeRelease(&m_pVideoDisplay);
	SafeRelease(&m_pEVR);
}

bool EVR_Renderer::HasVideo() const
{
	return (nullptr != m_pVideoDisplay);
}

HRESULT EVR_Renderer::AddToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, HWND hWnd)
{
	CaptureEngineCOM::IBaseFilterPtr pEVR(nullptr);
	HRESULT hr(S_OK);

	if (nullptr == pGraph)
	{
		wdebugstream() << TEXT("DEBUG: pGraph is NULL; cannot continue") << std::endl;
	}
	else if (FAILED(hr = AddFilterByCLSID(pGraph, CLSID_EnhancedVideoRenderer, pEVR, TEXT("EVR"))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add EVR CLSID to pGraph") << std::endl;
	}
	else if (FAILED(hr = InitialiseEVR(pEVR, hWnd, &m_pVideoDisplay)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to initialise EVR renderer") << std::endl;
	}

	// Note: Because IMFVideoDisplayControl is a service interface,
    // you cannot QI the pointer to get back the IBaseFilter pointer.
    // Therefore, we need to cache the IBaseFilter pointer.

	else
	{
		m_pEVR = pEVR;
		m_pEVR->AddRef();
	}

	::SafeRelease(&pEVR);
    return hr;
}

HRESULT EVR_Renderer::FinalizeGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph)
{
	HRESULT hr(S_OK);
	BOOL bRemoved;

    if (nullptr == m_pEVR)
    {
        hr = S_OK;
    }
	else if (nullptr == pGraph)
	{
		wdebugstream() << TEXT("DEBUG: pGraph is NULL; cannot continue") << std::endl;
	}
	else if (FAILED(hr = RemoveUnconnectedRenderer(pGraph, m_pEVR, &bRemoved)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to remove EVR CLSID from pGraph") << std::endl;
	}
	else if (bRemoved)
    {
        SafeRelease(&m_pEVR);
        SafeRelease(&m_pVideoDisplay);
    }
	else
	{
		// Success.
	}

    return hr;
}

HRESULT EVR_Renderer::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	SIZE szVideo, szARVideo;
	HRESULT hr(S_OK);
	
	if (nullptr == m_pVideoDisplay)
    {
        hr = S_OK;
    }
	else if (FAILED(hr = m_pVideoDisplay->GetNativeVideoSize(&szVideo, &szARVideo)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to get native video size") << std::endl;
	}
	else
	{
		// Success.

		if (nullptr != lpWidth) *lpWidth = szVideo.cx;
		if (nullptr != lpHeight) *lpHeight = szVideo.cy;
		if (nullptr != lpARWidth) *lpARWidth = szARVideo.cx;
		if (nullptr != lpARHeight) *lpARHeight = szARVideo.cy;
	}

	return hr;
}

HRESULT EVR_Renderer::UpdateVideoWindow(HWND hWnd, const LPRECT prc)
{
	HRESULT hr(S_OK);
    RECT rc;

    if (nullptr == m_pVideoDisplay)
    {
        hr = S_OK;
    }
	else if ((prc) && (FAILED(hr = m_pVideoDisplay->SetVideoPosition(nullptr, prc))))
    {
		wdebugstream() << TEXT("DEBUG: Failed to set video position in video") << std::endl;
    }
    else
    {
		::GetClientRect(hWnd, &rc);

        if (FAILED(hr = m_pVideoDisplay->SetVideoPosition(nullptr, &rc)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to set video position in video") << std::endl;
		}
		else
		{
			// Success.
		}
    }

	return hr;
}

HRESULT EVR_Renderer::Repaint(HWND hWnd, HDC hDC)
{
	return ((m_pVideoDisplay) ? m_pVideoDisplay->RepaintVideo() : S_OK);
}

HRESULT EVR_Renderer::DisplayModeChanged()
{
    // The EVR does not require any action in response to WM_DISPLAYCHANGE.
    return S_OK;
}

HRESULT EVR_Renderer::InitialiseEVR(CaptureEngineCOM::IBaseFilterPtr pEVR, HWND hWnd, IMFVideoDisplayControl **ppDisplayControl) 
{ 
    IMFVideoDisplayControl *pDisplay(nullptr);
	IMFGetService *pGS(nullptr);
    HRESULT hr(S_OK);

	if (nullptr == pEVR)
	{
		wdebugstream() << TEXT("DEBUG: pEVR is NULL; cannot continue") << std::endl;
	}
	else if (FAILED(hr = pEVR->QueryInterface(IID_PPV_ARGS(&pGS))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to query interface to IMFGetService") << std::endl;
	}
	else if (FAILED(hr = pGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplay))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to get video render service") << std::endl;
	}
	else if (FAILED(hr = pDisplay->SetVideoWindow(hWnd)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set video window handle") << std::endl;
	}
	else if (FAILED(hr = pDisplay->SetAspectRatioMode(MFVideoARMode_PreservePicture)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set aspect ratio mode of video") << std::endl;
	}
	else
	{
		// Return the IMFVideoDisplayControl pointer to the caller.
		*ppDisplayControl = pDisplay;
		(*ppDisplayControl)->AddRef();
	}

    SafeRelease(&pGS);
    SafeRelease(&pDisplay);
    return hr; 
} 

}
// namespace WordJewelsImager
