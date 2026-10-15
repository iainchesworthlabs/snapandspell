#include "pch.h"

#include "CaptureSupport.h"
#include "DebugStream.h"
#include "OverlayMixerRenderer.h"
#include "SafeRelease.h"

namespace WordJewelsImager
{

OverlayMixer_Renderer::OverlayMixer_Renderer()
{
}

OverlayMixer_Renderer::~OverlayMixer_Renderer()
{
}

bool OverlayMixer_Renderer::HasVideo() const
{
	return (false);
}

HRESULT OverlayMixer_Renderer::AddToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, HWND hWnd)
{
	return E_FAIL;
}

HRESULT OverlayMixer_Renderer::FinalizeGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph)
{
	return E_FAIL;
}

HRESULT OverlayMixer_Renderer::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	return E_FAIL;
}

HRESULT OverlayMixer_Renderer::UpdateVideoWindow(HWND hWnd, const LPRECT prc)
{
	return E_FAIL;
}

HRESULT OverlayMixer_Renderer::Repaint(HWND hWnd, HDC hDC)
{
	return E_FAIL;
}

HRESULT OverlayMixer_Renderer::DisplayModeChanged()
{
	return E_FAIL;
}

}
// namespace WordJewelsImager