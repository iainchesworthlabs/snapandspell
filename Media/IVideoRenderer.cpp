#include "pch.h"
#include "IVideoRenderer.h"

#include <stdexcept>

#include "CaptureSupport.h"
#include "DebugStream.h"
#include "SafeRelease.h"
#include "TracedThrow.h"

namespace WordJewelsImager
{

IVideoRenderer::IVideoRenderer()
{
}

IVideoRenderer::~IVideoRenderer()
{
}

HRESULT IVideoRenderer::RemoveUnconnectedRenderer(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pRenderer, BOOL *pbRemoved)
{
	CaptureEngineCOM::IPinPtr pPin(nullptr);
	HRESULT hr(S_OK);

	*pbRemoved = FALSE;
	
	if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: pGraph is NULL, cannot continue"));
	}
	else if (FAILED(hr = CaptureSupport::FindConnectedPin(pRenderer, PINDIR_INPUT, pPin)))
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

}
// namespace WordJewelsImager
