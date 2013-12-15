#pragma once
#ifndef _CAPTURESUPPORT_H
#define _CAPTURESUPPORT_H

#include <cstdint>
#include <stdexcept>

#include "CaptureEngineCOM.h"
#include "TracedThrow.h"

namespace WordJewelsImager
{

class CaptureSupport
{
public:
	CaptureSupport();
	virtual ~CaptureSupport();

public:
	static const uint32_t WM_GRAPHNOTIFY = (WM_APP+1);

public:
	static HRESULT AddFilterByCLSID(CaptureEngineCOM::IGraphBuilderPtr pGraph, REFGUID clsid, CaptureEngineCOM::IBaseFilterPtr& ppF, LPCWSTR wszName);
	static HRESULT GetNextFilter(CaptureEngineCOM::IBaseFilterPtr pFilter, PIN_DIRECTION PinDir, CaptureEngineCOM::IBaseFilterPtr *ppNext);
	static HRESULT FindConnectedPin(CaptureEngineCOM::IBaseFilterPtr pFilter, PIN_DIRECTION PinDir, CaptureEngineCOM::IPinPtr& ppPin);
	static HRESULT FindUnconnectedPin(CaptureEngineCOM::IBaseFilterPtr pFilter, PIN_DIRECTION PinDir, CaptureEngineCOM::IPinPtr *ppPin);
	static HRESULT IsPinConnected(CaptureEngineCOM::IPinPtr pPin, BOOL *pResult);
	static HRESULT IsPinDirection(CaptureEngineCOM::IPinPtr pPin, PIN_DIRECTION dir, BOOL *pResult);
	static HRESULT MatchPin(CaptureEngineCOM::IPinPtr pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult);

public:
	static HRESULT ConnectFilters(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IPinPtr pOut, CaptureEngineCOM::IBaseFilterPtr pDest);
	static HRESULT ConnectFilters(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pSrc, CaptureEngineCOM::IPinPtr pIn);
	static HRESULT ConnectFilters(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pSrc, CaptureEngineCOM::IBaseFilterPtr pDest);

public:
	static HRESULT FindFilterInterface(CaptureEngineCOM::IGraphBuilderPtr pGraph, REFGUID iid, void** ppUnknown);
	static HRESULT FindInterfaceAnywhere(CaptureEngineCOM::IGraphBuilderPtr pGraph, REFGUID iid, void** ppUnknown);
	static HRESULT FindPinInterface(CaptureEngineCOM::IBaseFilterPtr pFilter, REFGUID iid, void** ppUnknown);
	static HRESULT FindPinByName(CaptureEngineCOM::IBaseFilterPtr pFilter, const WCHAR *wszName, CaptureEngineCOM::IPinPtr *ppPin);

public:
	static HRESULT LoadGraphFile(CaptureEngineCOM::IGraphBuilderPtr pGraph, const WCHAR* wszName);

public:
	static void NukeDownstream(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pFilter);
	static void NukeDownstream(CaptureEngineCOM::IPinPtr pPin, CaptureEngineCOM::IBaseFilterPtr pFilter);
	static void NukeUpstream(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pFilter);
	static void NukeUpstream(CaptureEngineCOM::IPinPtr pPin, CaptureEngineCOM::IBaseFilterPtr pFilter);

public:
	static bool IsStreamStart(CaptureEngineCOM::IBaseFilterPtr pFilter);
	static bool IsStreamEnd(CaptureEngineCOM::IBaseFilterPtr pFilter);
	static bool IsVideoRenderer(CaptureEngineCOM::IBaseFilterPtr pFilter);
	static bool IsSplitter(CaptureEngineCOM::IBaseFilterPtr pFilter, bool fCountConnectedOnly);
	static bool IsMultiplexer(CaptureEngineCOM::IBaseFilterPtr pFilter, bool fCountConnectedOnly);
	static bool IsAudioWaveRenderer(CaptureEngineCOM::IBaseFilterPtr pFilter);

public:
	static int CountPins(CaptureEngineCOM::IBaseFilterPtr pBF, uint32_t& nIn, uint32_t& nOut, uint32_t& nInC, uint32_t& nOutC)
	{
		DiagnosticsTracedThrow(std::runtime_error("NOT IMPLEMENTED"));
		return 0;
	}

private:
	static void FreeMediaTypeFields(AM_MEDIA_TYPE *pMediaType);
	static void FreeMediaType(AM_MEDIA_TYPE *pMediaType);
};

}
// namespace WordJewelsImager

#endif // _CAPTURESUPPORT_H