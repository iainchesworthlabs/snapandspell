#include "StdAfx.h"

#include <stdexcept>

#include "CaptureSupport.h"
#include "DebugStream.h"
#include "SafeRelease.h"
#include "TracedThrow.h"
#include "VMR9_Renderer.h"

namespace WordJewelsImager
{

VMR9_Renderer::VMR9_Renderer() : m_pWindowless(nullptr)
{
}

VMR9_Renderer::~VMR9_Renderer()
{
    SafeRelease(&m_pWindowless);
}

bool VMR9_Renderer::HasVideo() const 
{ 
    return (nullptr != m_pWindowless); 
}

HRESULT VMR9_Renderer::AddToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, HWND hWnd)
{
    CaptureEngineCOM::IBaseFilterPtr pVMR(nullptr);
	HRESULT hr(S_OK);

	if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pGraph is NULL, cannot continue"));
	}
	else if (FAILED(hr = CaptureSupport::AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer9, pVMR, TEXT("VMR-9"))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to add VMR-9 filter to graph"));
	}
	else if (FAILED(hr = InitWindowlessVMR9(pVMR, hWnd, &m_pWindowless)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to initialise VMR-9 (windowless)"));
    }
	else
	{
	}

    SafeRelease(&pVMR);
    return hr;
}

HRESULT VMR9_Renderer::FinalizeGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph)
{
	CaptureEngineCOM::IBaseFilterPtr pFilter(nullptr);
	HRESULT hr(S_OK);
	BOOL bRemoved;

	if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pGraph is NULL, cannot continue"));
	}
	else if (nullptr == m_pWindowless)
    {
        hr = S_OK;
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

HRESULT VMR9_Renderer::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
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

HRESULT VMR9_Renderer::UpdateVideoWindow(HWND hWnd, const LPRECT prc)
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

HRESULT VMR9_Renderer::Repaint(HWND hWnd, HDC hDC)
{
	return ((m_pWindowless) ? m_pWindowless->RepaintVideo(hWnd, hDC) : S_OK);
}

HRESULT VMR9_Renderer::DisplayModeChanged()
{
	return ((m_pWindowless) ? m_pWindowless->DisplayModeChanged() : S_OK);
}

HRESULT VMR9_Renderer::InitWindowlessVMR9(CaptureEngineCOM::IBaseFilterPtr pVMR, HWND hWnd, IVMRWindowlessControl9 **ppWC) 
{ 
    IVMRFilterConfig9 * pConfig(nullptr); 
    IVMRWindowlessControl9 *pWC(nullptr);
	HRESULT hr(S_OK);

    if (nullptr == pVMR)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pVMR is NULL, cannot continue"));
	}
	else if (FAILED(hr = pVMR->QueryInterface(IID_PPV_ARGS(&pConfig))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to query interface for pConfig"));
	}
	else if (FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Windowless)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to set rendering mode to VMR-9 (windowless)"));
	}
	else if (FAILED(hr = pVMR->QueryInterface(IID_PPV_ARGS(&pWC))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to query interface for pWC"));
	}
	else if (FAILED(hr = pWC->SetVideoClippingWindow(hWnd)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to set clipping window to hWnd"));
	}
	else if (FAILED(hr = pWC->SetAspectRatioMode(VMR9ARMode_LetterBox)))
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
