#include "pch.h"

#include "CaptureGraphMgmt.h"
#include "CaptureSupport.h"
#include "DebugStream.h"
#include "SafeRelease.h"

namespace WordJewelsImager
{

CaptureGraphManagement::CaptureGraphManagement()
{
}

CaptureGraphManagement::~CaptureGraphManagement()
{
}

HRESULT CaptureGraphManagement::AddAndConnectFilterToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pFilter)
{
	CaptureEngineCOM::IEnumFiltersPtr pEnum(nullptr);
	CaptureEngineCOM::IBaseFilterPtr pFirstFilter(nullptr);
	ULONG cFetched(0);
	HRESULT hr;

	if ((nullptr == pGraph) || (nullptr == pFilter))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
		hr = E_INVALIDARG;
	}
	else if (FAILED(hr = pGraph->EnumFilters(&pEnum)))
	{
		wdebugstream() << TEXT("DEBUG: FIXME") << std::endl;
	}
	else if (FAILED(hr = pEnum->Next(1, &pFirstFilter, &cFetched)))
	{

	}
	else
	{
		// All done.
	}

	SafeRelease(&pEnum);
	return hr;
}

}
// namespace WordJewelsImager
