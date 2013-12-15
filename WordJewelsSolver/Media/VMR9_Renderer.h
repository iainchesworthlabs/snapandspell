#pragma once
#ifndef _VMR9_RENDERER_H
#define _VMR9_RENDERER_H

#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>
#include "CaptureEngineCOM.h"
#include "IVideoRenderer.h"

namespace WordJewelsImager
{

class VMR9_Renderer : public IVideoRenderer
{
public:
    VMR9_Renderer();
    ~VMR9_Renderer();

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
	HRESULT InitWindowlessVMR9(CaptureEngineCOM::IBaseFilterPtr pVMR, HWND hWnd, IVMRWindowlessControl9 **ppWc);

private:
	IVMRWindowlessControl9 *m_pWindowless;
};

}
// namespace WordJewelsImager

#endif // _VMR9_RENDERER_H
