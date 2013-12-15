#pragma once
#ifndef _IVIDEORENDERER_H
#define _IVIDEORENDERER_H

#include <Windows.h>
#include <dshow.h>
#include "CaptureEngineCOM.h"

namespace WordJewelsImager
{

class IVideoRenderer
{
public:
	IVideoRenderer();
	virtual ~IVideoRenderer();

public:
	virtual bool HasVideo() const = 0;

public:
    virtual HRESULT AddToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, HWND hWnd) = 0;
    virtual HRESULT FinalizeGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph) = 0;
	virtual HRESULT GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight) = 0;
    virtual HRESULT UpdateVideoWindow(HWND hWnd, const LPRECT prc) = 0;
    virtual HRESULT Repaint(HWND hWnd, HDC hDC) = 0;
    virtual HRESULT DisplayModeChanged() = 0;

public:
	HRESULT RemoveUnconnectedRenderer(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pRenderer, BOOL *pbRemoved);
};

}
// WordJewelsImager

#endif // _IVIDEORENDERER_H
