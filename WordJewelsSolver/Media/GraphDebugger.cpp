#include "Stdafx.h"

// #include <dvdmedia.h>

#include <cstdint>
#include <string>

#include "DebugStream.h"
#include "GraphDebugger.h"
#include "SafeRelease.h"

#define QueryFilterInfoReleaseGraph(fi) if ((fi).pGraph) (fi).pGraph->Release();
#define QueryPinInfoReleaseFilter(pi) if ((pi).pFilter) (pi).pFilter->Release();

namespace WordJewelsImager
{

GraphDebugger::GraphDebugger()
{
}

GraphDebugger::~GraphDebugger()
{
}

// If built for debug this will display the media type details. We convert the
// major and subtypes into strings and also ask the base classes for a string
// description of the subtype, so MEDIASUBTYPE_RGB565 becomes RGB 565 16 bit
// We also display the fields in the BITMAPINFOHEADER structure, this should
// succeed as we do not accept input types unless the format is big enough

	/*
void WINAPI GraphDebugger::DisplayType(LPCTSTR label, const AM_MEDIA_TYPE *pmtIn)
{

	// Dump the GUID types and a short description
	wdebugstream() << TEXT("DEBUG: ") << label << TEXT("Major: ") << GuidNames[pmtIn->majortype] << TEXT("Minor: ") << GuidNames[pmtIn->subtype] << std::endl;
	wdebugstream() << TEXT("DEBUG: Subtype description: ") << GetSubtypeName(&pmtIn->subtype) << std::endl;

	// Dump the generic media types
	wdebugstream() << TEXT("DEBUG: Is ") << ((pmtIn->bTemporalCompression) ? TEXT("not") : TEXT("")) << TEXT(" temporally compressed") << std::endl;
	wdebugstream() << TEXT("DEBUG: Sample Size: ");
	if (pmtIn->bFixedSizeSamples) 
	{
		wdebugstream() << pmtIn->lSampleSize << std::endl;
	}
	else
	{
		wdebugstream() << TEXT("variable") << std::endl;
	}

	if (pmtIn->formattype == FORMAT_VideoInfo) 
	{
		VIDEOINFOHEADER *pVideoInfo = (VIDEOINFOHEADER *)pmtIn->pbFormat;
		DisplayRECT(TEXT("Source rectangle"),pVideoInfo->rcSource);
		DisplayRECT(TEXT("Target rectangle"),pVideoInfo->rcTarget);
		DisplayBITMAPINFO(HEADER(pmtIn->pbFormat));
	}
	else if (pmtIn->formattype == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *pVideoInfo2 = (VIDEOINFOHEADER2 *)pmtIn->pbFormat;
		DisplayRECT(TEXT("Source rectangle"),pVideoInfo2->rcSource);
		DisplayRECT(TEXT("Target rectangle"),pVideoInfo2->rcTarget);
		wdebugstream() << TEXT("DEBUG: Aspect Ratio: ") << pVideoInfo2->dwPictAspectRatioX << TEXT(":") << pVideoInfo2->dwPictAspectRatioY << std::endl;
		DisplayBITMAPINFO(&pVideoInfo2->bmiHeader);
	} 
	else if (pmtIn->majortype == MEDIATYPE_Audio)
	{
		wdebugstream() << TEXT("DEBUG: Format type: ") << GuidNames[pmtIn->formattype] << std::endl;
		wdebugstream() << TEXT("DEBUG: Subtype: ") << GuidNames[pmtIn->subtype] << std::endl;

		if ((MEDIASUBTYPE_MPEG1Packet != pmtIn->subtype) && (sizeof(PCMWAVEFORMAT) < pmtIn->cbFormat))
		{
			// Dump the contents of the WAVEFORMATEX type-specific format structure
			WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pmtIn->pbFormat;
			wdebugstream() << TEXT("DEBUG: wFormatTag = ") << pwfx->wFormatTag << std::endl;
			wdebugstream() << TEXT("DEBUG: nChannels = ") << pwfx->nChannels << std::endl;
			wdebugstream() << TEXT("DEBUG: nSamplesPerSec = ") << pwfx->nSamplesPerSec << std::endl;
			wdebugstream() << TEXT("DEBUG: nAvgBytesPerSec = ") << pwfx->nAvgBytesPerSec << std::endl;
			wdebugstream() << TEXT("DEBUG: nBlockAlign = ") << pwfx->nBlockAlign << std::endl;
			wdebugstream() << TEXT("DEBUG: wBitsPerSample = ") << pwfx->wBitsPerSample << std::endl;

			// PCM uses a WAVEFORMAT and does not have the extra size field
			if (sizeof(WAVEFORMATEX) < pmtIn->cbFormat) 
			{
				wdebugstream() << TEXT("DEBUG: cbSize = ") << pwfx->cbSize << std::endl;
			}
		} 
		else 
		{
			///FIXME add other types here.
		}
	} 
	else 
	{
		wdebugstream() << TEXT("DEBUG: Format type: ") << GuidNames[pmtIn->formattype] << std::endl;
	}
}

void WINAPI GraphDebugger::DumpGraph(IFilterGraph *pGraph, DWORD dwLevel)
{
	wdebugstream() << TEXT("DEBUG: DumpGraph [") << pGraph << TEXT("]") << std::endl;

	IEnumFilters *pFilters;
	IBaseFilter *pFilter;
	ULONG n;

	if(nullptr == pGraph)
	{
		wdebugstream() << TEXT("DEBUG: pGraph is a null pointer") << std::endl;
	}
	else if (FAILED(pGraph->EnumFilters(&pFilters))) 
	{
		wdebugstream() << TEXT("DEBUG: pGraph->EnumFilters() failed") << std::endl;
	}
	else
	{
		FILTER_INFO info;
		PIN_INFO pinInfo;
		IEnumPins *pins;
		IPin *pPin;
		HRESULT hr;
		
		while (S_OK == pFilters->Next(1, &pFilter, &n))
		{
			if (FAILED(pFilter->QueryFilterInfo(&info))) 
			{
				wdebugstream() << TEXT("DEBUG:    Filter [") << pFilter << TEXT("] -- failed QueryFilterInfo()") << std::endl;
			} 
			else 
			{
				wdebugstream() << TEXT("DEBUG:    Filter [") << pFilter << TEXT("] '") << info.achName << TEXT("'") << std::endl;
				QueryFilterInfoReleaseGraph(info);

				if (FAILED(pFilter->EnumPins(&pins))) 
				{
					wdebugstream() << TEXT("DEBUG: pFilter->EnumPins() failed") << std::endl;
				} 
				else 
				{
					while (S_OK == pins->Next(1, &pPin, &n)) 
					{
						if (FAILED(pPin->QueryPinInfo(&pinInfo))) 
						{
							wdebugstream() << TEXT("DEBUG:        Pin [") << pPin << TEXT("] -- failed QueryPinInfo()") << std::endl;
						} 
						else 
						{
							QueryPinInfoReleaseFilter(pinInfo);
							IPin *pPinConnected(nullptr);
							
							hr = pPin->ConnectedTo(&pPinConnected);
							if (pPinConnected) 
							{
								wdebugstream() << TEXT("DEBUG:        Pin [") << pPin << TEXT("] '") << pinInfo.achName << TEXT("' [") << ((PINDIR_INPUT == pinInfo.dir) ? TEXT("In") : TEXT("Out")) << TEXT ("] ") << pPinConnected << std::endl;
								pPinConnected->Release();

								///TODO perhaps we should really dump the type both ways as a sanity check?
								if (PINDIR_OUTPUT == pinInfo.dir) 
								{
									AM_MEDIA_TYPE mt;

									if (FAILED(pPin->ConnectionMediaType(&mt))) 
									{
										///FIXME
									}
									else
									{
										DisplayType(TEXT("Connection Type"), &mt);
										::FreeMediaType(mt);
									}
								}
							} 
							else 
							{
								wdebugstream() << TEXT("DEBUG:        Pin [") << pPin << TEXT("] '") << pinInfo.achName << TEXT("' [") << ((PINDIR_INPUT == pinInfo.dir) ? TEXT("In") : TEXT("Out")) << TEXT ("]") << std::endl;
							}
						}

						pPin->Release();
					}

					pins->Release();
				}
			}

			pFilter->Release();
		}
	}

	pFilters->Release();
}
*/

HRESULT WINAPI GraphDebugger::AddGraphToROT(IUnknown* pUnkGraph, DWORD *pdwRegister)
{
	HRESULT hr;
	WCHAR wsz[128];
	IMoniker *pMoniker(nullptr);
	IRunningObjectTable *pROT(nullptr);

	if (FAILED(hr = ::StringCchPrintfW(wsz, NUMELMS(wsz), TEXT("FilterGraph %08x pid %08x"), reinterpret_cast<DWORD_PTR>(pUnkGraph), ::GetCurrentProcessId())))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create string for moniker") << std::endl;
	}
	else if (FAILED(hr = ::CreateItemMoniker(L"!", wsz, &pMoniker)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create a moniker") << std::endl;
	}

	// Use the ROTFLAGS_REGISTRATIONKEEPSALIVE to ensure a strong reference
	// to the object.  Using this flag will cause the object to remain
	// registered until it is explicitly revoked with the Revoke() method.
	//
	// Not using this flag means that if GraphEdit remotely connects
	// to this graph and then GraphEdit exits, this object registration
	// will be deleted, causing future attempts by GraphEdit to fail until
	// this application is restarted or until the graph is registered again.

	else if (FAILED(hr = ::GetRunningObjectTable(0, &pROT)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to retrieve the Running Object Table") << std::endl;
	}
	else if (FAILED(hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, pMoniker, pdwRegister)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to register object in Running Object Table") << std::endl;
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Successfully registered object into Running Object Table") << std::endl;
	}

	SafeRelease(&pMoniker);
	SafeRelease(&pROT);
	
	return hr;
}

void WINAPI GraphDebugger::RemoveGraphFromROT(DWORD dwRegister)
{
	IRunningObjectTable *pROT(nullptr);
	HRESULT hr;

	if (FAILED(hr = ::GetRunningObjectTable(0, &pROT)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to retrieve the Running Object Table") << std::endl;
	}
	else if (nullptr == pROT)
	{
		wdebugstream() << TEXT("DEBUG: Failed to retrieve pointer to Running Object Table") << std::endl;
	}
	else if (FAILED(hr = pROT->Revoke(dwRegister)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to register object in Running Object Table") << std::endl; 
	}
	else 
	{
		wdebugstream() << TEXT("DEBUG: Successfully unregistered object into Running Object Table") << std::endl;
	}

	SafeRelease(&pROT);
}

}
// namespace WordJewelsImager
