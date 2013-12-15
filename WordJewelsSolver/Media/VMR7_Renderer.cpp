#include "StdAfx.h"

#include <stdexcept>

#include "CaptureSupport.h"
#include "DebugStream.h"
#include "SafeRelease.h"
#include "TracedThrow.h"
#include "VMR7_Renderer.h"

namespace WordJewelsImager
{

VMR7_Renderer::VMR7_Renderer() : m_pWindowless(nullptr)
{
}

VMR7_Renderer::~VMR7_Renderer()
{
    SafeRelease(&m_pWindowless);
}

bool VMR7_Renderer::HasVideo() const 
{ 
    return (nullptr != m_pWindowless); 
}

HRESULT VMR7_Renderer::AddToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, HWND hWnd)
{
    CaptureEngineCOM::IBaseFilterPtr pVMR(nullptr);
	HRESULT hr(S_OK);

	if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pGraph is NULL, cannot continue"));
	}
	else if (FAILED(hr = CaptureSupport::AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer, pVMR, TEXT("VMR-7"))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to add VMR-7 filter to graph"));
	}
	else if (FAILED(hr = InitWindowlessVMR(pVMR, hWnd, &m_pWindowless)))
    {
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to initialise VMR-7 (windowless)"));
    }
	else
	{
	}

    SafeRelease(&pVMR);
    return hr;
}

HRESULT VMR7_Renderer::FinalizeGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph)
{
	CaptureEngineCOM::IBaseFilterPtr pFilter(nullptr);
    HRESULT hr(S_OK);
	BOOL bRemoved;

	if (nullptr == m_pWindowless)
	{
		hr = S_OK;
	}
	else if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pGraph is NULL, cannot continue"));
	}
	else if (FAILED(hr = m_pWindowless->QueryInterface(IID_PPV_ARGS(&pFilter))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to query interface for pFilter"));
	}
	else if (FAILED(hr = RemoveUnconnectedRenderer(pGraph, pFilter, &bRemoved)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to remove unconnected renderer"));
	}
	else if (bRemoved)
    {
        SafeRelease(&m_pWindowless);
    }
	else
	{
	}

    SafeRelease(&pFilter);
    return hr;
}

HRESULT VMR7_Renderer::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	HRESULT hr(S_OK);
	
	if (nullptr == m_pWindowless)
    {
        hr = S_OK;
    }
	else if (FAILED(hr = m_pWindowless->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to get native video size") << std::endl;
	}
	else
	{
		// Success.
	}

	return hr;
}

HRESULT VMR7_Renderer::UpdateVideoWindow(HWND hWnd, const LPRECT prc)
{
	HRESULT hr(S_OK);
    RECT rc;

	if (nullptr == m_pWindowless)
	{
		hr = S_OK;
	}
	else if (prc)
    {
        hr = m_pWindowless->SetVideoPosition(nullptr, prc);
    }
    else
    {
		::GetClientRect(hWnd, &rc);

        hr = m_pWindowless->SetVideoPosition(nullptr, &rc);
    }

	return hr;
}

HRESULT VMR7_Renderer::Repaint(HWND hWnd, HDC hDC)
{
	return ((m_pWindowless) ? m_pWindowless->RepaintVideo(hWnd, hDC) : S_OK);
}

HRESULT VMR7_Renderer::DisplayModeChanged()
{
	return ((m_pWindowless) ? m_pWindowless->DisplayModeChanged() : S_OK);
}

HRESULT VMR7_Renderer::InitWindowlessVMR(CaptureEngineCOM::IBaseFilterPtr pVMR, HWND hWnd, IVMRWindowlessControl **ppWC) 
{ 
	IVMRWindowlessControl *pWC(nullptr);
	IVMRFilterConfig* pConfig(nullptr);
	HRESULT hr(S_OK);
    
    // Set the rendering mode.  
    if (nullptr == pVMR)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pVMR is NULL, cannot continue"));
	}
	else if (FAILED(hr = pVMR->QueryInterface(IID_PPV_ARGS(&pConfig))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to query interface for pConfig"));
	}
	else if (FAILED(hr = pConfig->SetRenderingMode(VMRMode_Windowless)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to set rendering mode to VMR-7 (windowless)"));
	}
	else if (FAILED(hr = pVMR->QueryInterface(IID_PPV_ARGS(&pWC))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to query interface for pWC"));
	}
	else if (FAILED(hr = pWC->SetVideoClippingWindow(hWnd)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to set clipping window to hWnd"));
	}
	else if (FAILED(hr = pWC->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to set aspect ratio mode to letter box"));
	}
	else
	{
		*ppWC = pWC;
		(*ppWC)->AddRef();
	}

    SafeRelease(&pConfig);
    SafeRelease(&pWC);
    return hr;
}

}
// namespace WordJewelsImager
