#pragma once
#ifndef _VMR7_RENDERER_H
#define _VMR7_RENDERER_H

#include <dshow.h>
#include "CaptureEngineCOM.h"
#include "IVideoRenderer.h"

namespace WordJewelsImager
{

class VMR7_Renderer : public IVideoRenderer
{
public:
    VMR7_Renderer();
    ~VMR7_Renderer();

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
	HRESULT InitWindowlessVMR(CaptureEngineCOM::IBaseFilterPtr pVMR, HWND hWnd, IVMRWindowlessControl **ppWc);

private:
	IVMRWindowlessControl *m_pWindowless;
};

}
// namespace WordJewelsImager

#endif // _VMR9_RENDERER_H
