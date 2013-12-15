#pragma once
#ifndef _EVR_RENDERER_H
#define _EVR_RENDERER_H

#include <evr.h>
#include "CaptureEngineCOM.h"
#include "IVideoRenderer.h"

namespace WordJewelsImager
{

class EVR_Renderer : public IVideoRenderer
{
public:
    EVR_Renderer();
    ~EVR_Renderer();

public:
    bool HasVideo() const;

public:
    HRESULT AddToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, HWND hwnd);
    HRESULT FinalizeGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph);
	HRESULT GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight);
    HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc);
    HRESULT Repaint(HWND hwnd, HDC hdc);
	HRESULT DisplayModeChanged();

public:
	HRESULT InitialiseEVR(CaptureEngineCOM::IBaseFilterPtr pEVR, HWND hWnd, IMFVideoDisplayControl **ppWc);

private:
	CaptureEngineCOM::IBaseFilterPtr m_pEVR;
	IMFVideoDisplayControl *m_pVideoDisplay;
};

}
// namespace WordJewelsImager

#endif // _EVR_RENDERER_H
