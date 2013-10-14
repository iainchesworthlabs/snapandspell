#include "StdAfx.h"
#include "IVideoRenderer.h"

#include "DebugStream.h"
#include "SafeRelease.h"

namespace WordJewelsImager
{

IVideoRenderer::IVideoRenderer()
{
}

IVideoRenderer::~IVideoRenderer()
{
}

HRESULT IVideoRenderer::AddFilterByCLSID(CaptureEngineCOM::IGraphBuilderPtr pGraph, REFGUID clsid, CaptureEngineCOM::IBaseFilterPtr& ppF, LPCWSTR wszName)
{
	CaptureEngineCOM::IBaseFilterPtr pFilter(nullptr);
	HRESULT hr(S_OK);
		
	ppF = nullptr;

	if (nullptr == pGraph)
	{
		wdebugstream() << TEXT("DEBUG: pGraph is NULL, cannot continue") << std::endl;
		throw;
	}
	else if (FAILED(hr = pFilter.CreateInstance(clsid)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create pFilter instance") << std::endl;
		throw;
	}
	else if (FAILED(hr = pGraph->AddFilter(pFilter, wszName)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add filter to graph") << std::endl;
		throw;
	}
	else
	{
		ppF = pFilter;
		ppF->AddRef();
	}

	SafeRelease(&pFilter);
	return hr;
}

HRESULT IVideoRenderer::RemoveUnconnectedRenderer(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pRenderer, BOOL *pbRemoved)
{
	CaptureEngineCOM::IPinPtr pPin(nullptr);
	HRESULT hr(S_OK);

	*pbRemoved = FALSE;
	
	if (nullptr == pGraph)
	{
		wdebugstream() << TEXT("DEBUG: pGraph is NULL, cannot continue") << std::endl;
		throw;
	}
	else if (FAILED(hr = FindConnectedPin(pRenderer, PINDIR_INPUT, pPin)))
	{
		// If this function fails, it means the renderer is not connected to anything, so we remove it
		hr = pGraph->RemoveFilter(pRenderer);
        *pbRemoved = TRUE;
	}
	else
	{
		// If this function succeeds, the renderer is connected, so we don't remove it.
		SafeRelease(&pPin);
	}

    return hr;
}

HRESULT IVideoRenderer::IsPinConnected(CaptureEngineCOM::IPinPtr pPin, BOOL *pResult)
{
	CaptureEngineCOM::IPinPtr pTmp(nullptr);
    HRESULT hr(S_OK);

	if (nullptr == pPin)
	{
		wdebugstream() << TEXT("DEBUG: pPin is NULL, cannot continue") << std::endl;
		throw;
	}
	else if (SUCCEEDED(hr = pPin->ConnectedTo(&pTmp)))
	{
		*pResult = TRUE;
	}
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        // The pin is not connected.
        *pResult = FALSE;
        hr = S_OK;
    }
	else
    {
		// Call to ->ConnectedTo() failed.
    }

    SafeRelease(&pTmp);
    return hr;
}

HRESULT IVideoRenderer::IsPinDirection(CaptureEngineCOM::IPinPtr pPin, PIN_DIRECTION dir, BOOL *pResult)
{
	PIN_DIRECTION pinDir;
    HRESULT hr(S_OK);
    
	if (SUCCEEDED(hr = pPin->QueryDirection(&pinDir)))
    {
        *pResult = (pinDir == dir);
    }

    return hr;
}

HRESULT IVideoRenderer::FindConnectedPin(CaptureEngineCOM::IBaseFilterPtr pFilter, PIN_DIRECTION PinDir, CaptureEngineCOM::IPinPtr& ppPin)
{
	CaptureEngineCOM::IEnumPinsPtr pEnum(nullptr);
    CaptureEngineCOM::IPinPtr pPin(nullptr);
	BOOL bFound = FALSE;
	HRESULT hr(S_OK);

	ppPin = nullptr;
	
	if (nullptr == pFilter)
	{
		wdebugstream() << TEXT("DEBUG: pFilter is NULL, cannot continue") << std::endl;
		throw;
	}
	else if (FAILED(hr = pFilter->EnumPins(&pEnum)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to enumerate pins on the filter") << std::endl;
		throw;
	}
	else
	{
		while (S_OK == pEnum->Next(1, &pPin, nullptr))
		{
			BOOL bIsConnected;

			hr = IsPinConnected(pPin, &bIsConnected);
			if (SUCCEEDED(hr))
			{
				if (bIsConnected)
				{
					hr = IsPinDirection(pPin, PinDir, &bFound);
				}
			}
			else if (FAILED(hr))
			{
				pPin->Release();
			}
			else if (bFound)
			{
				ppPin = pPin;
			}
			else
			{
				pPin->Release();
			}
		}

		pEnum->Release();

		if (!bFound)
		{
			hr = VFW_E_NOT_FOUND;
		}
	}

    return hr;
}

}
// namespace WordJewelsImager
