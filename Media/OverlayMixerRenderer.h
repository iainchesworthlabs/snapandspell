#pragma once
#ifndef _OVERLAYMIXER_RENDERER_H
#define _OVERLAYMIXER_RENDERER_H

#include "CaptureEngineCOM.h"
#include "IVideoRenderer.h"

namespace WordJewelsImager
{

class OverlayMixer_Renderer : public IVideoRenderer
{
public:
	OverlayMixer_Renderer();
	~OverlayMixer_Renderer();

public:
	bool HasVideo() const;

public:
	HRESULT AddToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, HWND hWnd);
	HRESULT FinalizeGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph);
	HRESULT GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight);
	HRESULT UpdateVideoWindow(HWND hWnd, const LPRECT prc);
	HRESULT Repaint(HWND hWnd, HDC hDC);
	HRESULT DisplayModeChanged();
};

}
// namespace WordJewelsImager

#endif // _OVERLAYMIXER_RENDERER_H