#include "pch.h"

#include <cassert>
#include <stdexcept>

#include "CaptureSupport.h"
#include "DebugStream.h"
#include "SafeRelease.h"
#include "TracedThrow.h"

namespace WordJewelsImager
{

CaptureSupport::CaptureSupport()
{
}

CaptureSupport::~CaptureSupport()
{
}

HRESULT CaptureSupport::AddFilterByCLSID(_In_ CaptureEngineCOM::IGraphBuilderPtr pGraph, _In_ REFGUID clsid, _Out_ CaptureEngineCOM::IBaseFilterPtr& ppF, _In_z_ LPCWSTR wszName)
{
	CaptureEngineCOM::IBaseFilterPtr pFilter(nullptr);
	HRESULT hr(S_OK);

	ppF = nullptr;

	if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pGraph is NULL, cannot continue"));
	}
	else if (FAILED(hr = pFilter.CreateInstance(clsid)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to create pFilter instance"));
	}
	else if (FAILED(hr = pGraph->AddFilter(pFilter, wszName)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to add filter to graph"));
	}
	else
	{
		ppF = pFilter;
		ppF->AddRef();
	}

	SafeRelease(&pFilter);
	return hr;
}

HRESULT CaptureSupport::FindConnectedPin(_In_ CaptureEngineCOM::IBaseFilterPtr pFilter, _In_ PIN_DIRECTION PinDir, _Out_ CaptureEngineCOM::IPinPtr& ppPin)
{
	CaptureEngineCOM::IEnumPinsPtr pEnum(nullptr);
	CaptureEngineCOM::IPinPtr pPin(nullptr);
	BOOL bFound = FALSE;
	HRESULT hr(S_OK);

	ppPin = nullptr;

	if (nullptr == pFilter)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pFilter is NULL, cannot continue"));
	}
	else if (FAILED(hr = pFilter->EnumPins(&pEnum)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to enumerate pins on the filter"));
	}
	else
	{
		while (S_OK == pEnum->Next(1, &pPin, nullptr))
		{
			BOOL bIsConnected;

			hr = CaptureSupport::IsPinConnected(pPin, &bIsConnected);
			if (SUCCEEDED(hr))
			{
				if (bIsConnected)
				{
					hr = CaptureSupport::IsPinDirection(pPin, PinDir, &bFound);
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

HRESULT CaptureSupport::FindUnconnectedPin(_In_ CaptureEngineCOM::IBaseFilterPtr pFilter, _In_ PIN_DIRECTION PinDir, _Out_ CaptureEngineCOM::IPinPtr *ppPin)
{
	IEnumPins *pEnum = nullptr;
	IPin *pPin = nullptr;
	BOOL bFound = FALSE;
	HRESULT hr;

	if (FAILED(hr = pFilter->EnumPins(&pEnum)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to enumerate pins on the filter") << std::endl;
	}
	else
	{
		while ((SUCCEEDED(hr)) && (S_OK == pEnum->Next(1, &pPin, nullptr)))
		{
			if (FAILED(hr = MatchPin(pPin, PinDir, FALSE, &bFound)))
			{
				wdebugstream() << TEXT("DEBUG: Failed to match pin on the filter") << std::endl;
				break;
			} 
			else if (bFound)
			{
				*ppPin = pPin;
				(*ppPin)->AddRef();
				break;
			}

			SafeRelease(&pPin);
		}

		if ((SUCCEEDED(hr)) && (!bFound))
		{
			hr = VFW_E_NOT_FOUND;
		}
	}

	SafeRelease(&pPin);
	SafeRelease(&pEnum);
	return hr;
}

HRESULT CaptureSupport::IsPinConnected(_In_ CaptureEngineCOM::IPinPtr pPin, _Inout_ BOOL *pResult)
{
	CaptureEngineCOM::IPinPtr pTmp(nullptr);
	HRESULT hr(S_OK);

	if (nullptr == pPin)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pPin is NULL, cannot continue"));
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

HRESULT CaptureSupport::IsPinDirection(_In_ CaptureEngineCOM::IPinPtr pPin, _In_ PIN_DIRECTION dir, _Inout_ BOOL *pResult)
{
	PIN_DIRECTION pinDir;
	HRESULT hr(S_OK);

	if (SUCCEEDED(hr = pPin->QueryDirection(&pinDir)))
	{
		*pResult = (pinDir == dir);
	}

	return hr;
}

HRESULT CaptureSupport::MatchPin(_In_ CaptureEngineCOM::IPinPtr pPin, _In_ PIN_DIRECTION direction, _In_ BOOL bShouldBeConnected, _Inout_ BOOL *pResult)
{
	assert(nullptr != pResult);

	BOOL bIsConnected = FALSE;
	BOOL bMatch = FALSE;
	HRESULT hr;

	if (SUCCEEDED(hr = IsPinConnected(pPin, &bIsConnected)))
	{
		if (bIsConnected == bShouldBeConnected)
		{
			hr = IsPinDirection(pPin, direction, &bMatch);
		}

		if (SUCCEEDED(hr))
		{
			*pResult = bMatch;
		}
	}	

	return hr;
}

HRESULT CaptureSupport::FindFilterInterface(_In_ CaptureEngineCOM::IGraphBuilderPtr pGraph, _In_ REFGUID iid, void** ppUnknown)
{
	if ((!pGraph) || (!ppUnknown))
	{
		return E_POINTER;
	}

	CaptureEngineCOM::IEnumFiltersPtr pEnum = nullptr;
	CaptureEngineCOM::IBaseFilterPtr pF = nullptr;
	HRESULT hr;

	if (FAILED(hr = pGraph->EnumFilters(&pEnum)))
	{
		return E_FAIL;
	}
	else
	{
		// Query every filter for the interface.
		while (S_OK == (hr = pEnum->Next(1, &pF, 0)))
		{
			hr = pF->QueryInterface(iid, ppUnknown);
			pF->Release();

			if (SUCCEEDED(hr))
			{
				break;
			}
		}
	}	

	SafeRelease(&pEnum);
	return hr;
}

HRESULT CaptureSupport::FindPinInterface(CaptureEngineCOM::IBaseFilterPtr pFilter, REFGUID iid, void** ppUnknown)
{
	if ((!pFilter) || (!ppUnknown))
	{
		return E_POINTER;
	}

	CaptureEngineCOM::IEnumPinsPtr pEnum = nullptr;
	HRESULT hr;

	if (FAILED(hr = pFilter->EnumPins(&pEnum)))
	{
		return E_FAIL;
	}
	else
	{
		// Query every pin for the interface.
		CaptureEngineCOM::IPinPtr pPin = nullptr;

		while (S_OK == (hr = pEnum->Next(1, &pPin, 0)))
		{
			hr = pPin->QueryInterface(iid, ppUnknown);
			pPin->Release();

			if (SUCCEEDED(hr))
			{
				break;
			}
		}
	}
	
	SafeRelease(&pEnum);
	return hr;
}

HRESULT CaptureSupport::FindInterfaceAnywhere(CaptureEngineCOM::IGraphBuilderPtr pGraph, REFGUID iid, void** ppUnknown)
{
	if ((!pGraph) || (!ppUnknown))
	{
		return E_POINTER;
	}

	CaptureEngineCOM::IEnumFiltersPtr pEnum = nullptr;
	HRESULT hr;

	if (FAILED(hr = pGraph->EnumFilters(&pEnum)))
	{
		return E_FAIL;
	}
	else
	{
		// Loop through every filter in the graph.
		CaptureEngineCOM::IBaseFilterPtr pF = 0;
		while (S_OK == pEnum->Next(1, &pF, 0))
		{
			if (FAILED(hr = pF->QueryInterface(iid, ppUnknown)))
			{
				// The filter does not expose the interface, but maybe
				// one of its pins does.
				hr = FindPinInterface(pF, iid, ppUnknown);
			}

			SafeRelease(&pF);

			if (SUCCEEDED(hr))
			{
				break;
			}
		}
	}

	SafeRelease(&pEnum);
	return hr;
}

HRESULT CaptureSupport::FindPinByName(CaptureEngineCOM::IBaseFilterPtr pFilter, const WCHAR *wszName, CaptureEngineCOM::IPinPtr *ppPin)
{
	CaptureEngineCOM::IEnumPinsPtr pEnum(nullptr);
	HRESULT hr;

	if ((nullptr == pFilter) || (nullptr == wszName) || (nullptr == ppPin))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
		hr = E_POINTER;
	}
	else if (MAX_PIN_NAME < ::wcslen(wszName))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
		hr = E_INVALIDARG;
	}
	else if (FAILED(hr = pFilter->EnumPins(&pEnum)))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
	}
	else
	{
		CaptureEngineCOM::IPinPtr pPin(nullptr);
		hr = E_FAIL;

		while (S_OK == pEnum->Next(1, &pPin, 0))
		{
			PIN_INFO PinInfo;

			if (FAILED(pPin->QueryPinInfo(&PinInfo)))
			{
				wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
			}
			else if (nullptr == PinInfo.pFilter)
			{
				wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
			}
			else
			{
				PinInfo.pFilter->Release();
			}
			
			if (0 == ::wcscmp(wszName, PinInfo.achName))
			{
				*ppPin = pPin;
				pEnum->Release();

				hr = S_OK;
				break;
			}

			SafeRelease(&pPin);
		}
	}

	SafeRelease(&pEnum);
	return hr;
}

HRESULT CaptureSupport::LoadGraphFile(CaptureEngineCOM::IGraphBuilderPtr pGraph, const WCHAR* wszName)
{
	CaptureEngineCOM::IPersistStreamPtr pPersistStream(nullptr);
	CaptureEngineCOM::IStoragePtr pStorage(nullptr);
	CaptureEngineCOM::IStreamPtr pStream(nullptr);
	HRESULT hr;

	if (S_OK != ::StgIsStorageFile(wszName))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
		hr = E_FAIL;
	}
	else if (FAILED(hr = ::StgOpenStorage(wszName, 0, STGM_TRANSACTED | STGM_READ | STGM_SHARE_DENY_WRITE, 0, 0, &pStorage)))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
	}
	else if (FAILED(hr = pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&pPersistStream))))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
	}
	else if (FAILED(hr = pStorage->OpenStream(TEXT("ActiveMovieGraph"), 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream)))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
	}
	else if (FAILED(hr = pPersistStream->Load(pStream)))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
	}
	else
	{
		// All done.
	}
	
	SafeRelease(&pStream);
	SafeRelease(&pPersistStream);
	SafeRelease(&pStorage);

	return hr;
}

/*
bool CaptureSupport::IsStreamStart(CaptureEngineCOM::IBaseFilterPtr pFilter)
{
	SmartPtr<IAMFilterMiscFlags> pAMMF;

	pBF->QueryInterface(__uuidof(IAMFilterMiscFlags), (void **)&pAMMF);

	if (pAMMF && pAMMF->GetMiscFlags()&AM_FILTER_MISC_FLAGS_IS_SOURCE) return(true);
	
	uint32_t nIn, nOut, nInC, nOutC;	
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	
	AM_MEDIA_TYPE mt;
	
	SmartPtr<IPin> pIn = GetFirstPin(pBF);
	
	return ((nOut > 1) || (nOut > 0 && nIn == 1 && pIn && SUCCEEDED(pIn->ConnectionMediaType(&mt)) && mt.majortype == MEDIATYPE_Stream));
}
*/

bool CaptureSupport::IsStreamEnd(CaptureEngineCOM::IBaseFilterPtr pFilter)
{
	uint32_t nIn, nOut, nInC, nOutC;
	CountPins(pFilter, nIn, nOut, nInC, nOutC);

	return (0 == nOut);
}

bool CaptureSupport::IsVideoRenderer(CaptureEngineCOM::IBaseFilterPtr pFilter)
{
	CaptureEngineCOM::IEnumPinsPtr pEnumPins(nullptr);
	CaptureEngineCOM::IPinPtr pPin(nullptr);
	uint32_t nIn, nOut, nInC, nOutC;
	bool bIsVideoRenderer(false);
	HRESULT hr;

	if ((nullptr == pFilter) || (0 >= CountPins(pFilter, nIn, nOut, nInC, nOutC)))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
		hr = E_INVALIDARG;
	}
	else if ((0 > nInC) || (0 != nOut))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
		hr = E_FAIL;
	}
	else if (FAILED(hr = pFilter->EnumPins(&pEnumPins)))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
	}
	else
	{
		AM_MEDIA_TYPE mt;

		for (uint64_t i = 0; i < (nIn + nOut); ++i)
		{
			if (FAILED(hr = pEnumPins->Next(1, &pPin, 0)))
			{
				wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
				// Try the next pin.
			}
			else if (FAILED(hr = pPin->ConnectionMediaType(&mt)))
			{
				wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
				// Try the next pin.
			}
			else
			{
				bIsVideoRenderer = (MEDIATYPE_Video == mt.majortype);
				FreeMediaType(&mt);
				break;
			}
		}

		if (bIsVideoRenderer)
		{
			// It is already known that this is a video renderer.
		}
		else
		{
			CLSID clsid;

			::memcpy(&clsid, &GUID_NULL, sizeof(CLSID));
			pFilter->GetClassID(&clsid);

			bIsVideoRenderer = ((CLSID_VideoRenderer == clsid) || (CLSID_VideoRendererDefault == clsid));
		}
	}

	SafeRelease(&pPin);
	SafeRelease(&pEnumPins);

	return bIsVideoRenderer;
}

/*
bool CaptureSupport::IsSplitter(CaptureEngineCOM::IBaseFilterPtr pFilter, bool fCountConnectedOnly)
{

}

bool CaptureSupport::IsMultiplexer(CaptureEngineCOM::IBaseFilterPtr pFilter, bool fCountConnectedOnly)
{

}

bool CaptureSupport::IsAudioWaveRenderer(CaptureEngineCOM::IBaseFilterPtr pFilter)
{

}
*/

void CaptureSupport::FreeMediaTypeFields(AM_MEDIA_TYPE *pMediaType)
{
	if (0 != pMediaType->cbFormat) 
	{
		::CoTaskMemFree(reinterpret_cast<void*>(pMediaType->pbFormat));
		pMediaType->cbFormat = 0;
		pMediaType->pbFormat = nullptr;

	}

	if (nullptr != pMediaType->pUnk)
	{
		// pUnk should be unused, but this is safest
		pMediaType->pUnk->Release();
		pMediaType->pUnk = nullptr;
	}
}

void CaptureSupport::FreeMediaType(AM_MEDIA_TYPE *pMediaType)
{
	if (nullptr != pMediaType)
	{
		FreeMediaTypeFields(pMediaType);
		::CoTaskMemFree(reinterpret_cast<void *>(pMediaType));
	}
}

}
// namespaceWordJewelsImager
