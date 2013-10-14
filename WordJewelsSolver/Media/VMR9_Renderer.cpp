#include "StdAfx.h"

#include "DebugStream.h"
#include "SafeRelease.h"
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
		wdebugstream() << TEXT("DEBUG: pGraph is NULL, cannot continue") << std::endl;
		throw;
	}
	else if (FAILED(hr = AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer9, pVMR, TEXT("VMR-9"))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add VMR-9 filter to graph") << std::endl;
		throw;
	}
	else if (FAILED(hr = InitWindowlessVMR9(pVMR, hWnd, &m_pWindowless)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to initialise VMR-9 (windowless)") << std::endl;
		throw;
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
		wdebugstream() << TEXT("DEBUG: pGraph is NULL, cannot continue") << std::endl;
		throw;
	}
	else if (nullptr == m_pWindowless)
    {
        hr = S_OK;
    }
	else if (FAILED(hr = m_pWindowless->QueryInterface(IID_PPV_ARGS(&pFilter))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to query interface for pFilter") << std::endl;
		throw;
	}
	else if (FAILED(hr = RemoveUnconnectedRenderer(pGraph, pFilter, &bRemoved)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to remove unconnected renderer") << std::endl;
		throw;
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
		wdebugstream() << TEXT("DEBUG: pVMR is NULL, cannot continue") << std::endl;
		throw;
	}
	else if (FAILED(hr = pVMR->QueryInterface(IID_PPV_ARGS(&pConfig))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to query interface for pConfig") << std::endl;
		throw;
	}
	else if (FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Windowless)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set rendering mode to VMR-9 (windowless)") << std::endl;
		throw;
	}
	else if (FAILED(hr = pVMR->QueryInterface(IID_PPV_ARGS(&pWC))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to query interface for pWC") << std::endl;
		throw;
	}
	else if (FAILED(hr = pWC->SetVideoClippingWindow(hWnd)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set clipping window to hWnd") << std::endl;
		throw;
	}
	else if (FAILED(hr = pWC->SetAspectRatioMode(VMR9ARMode_LetterBox)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set aspect ratio mode to letter box") << std::endl;
		throw;
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
