#pragma once
#ifndef _CAPTUREPREVIEWWINDOW_H
#define _CAPTUREPREVIEWWINDOW_H

#include <memory>

#include "CaptureEngine.h"
#include "CaptureEngineCOM.h"
#include "CaptureSupport.h"
#include "IVideoRenderer.h"

namespace WordJewelsImager
{

class CapturePreviewWindow
{
public:
	CapturePreviewWindow(CaptureEngine& captureEngine);
	virtual ~CapturePreviewWindow();

public:
	_Success_(S_OK == return) HRESULT SetWindow(_In_ HWND hParent);
	_Success_(S_OK == return) HRESULT GetNativeVideoSize(_Outref_ uint64_t& width, _Outref_ uint64_t& height);
	_Success_(S_OK == return) HRESULT SetVideoPosition(_In_ HWND hwndApp, _In_ RECT rcDest);
	_Success_(S_OK == return) HRESULT HandleGraphEvent();
	_Success_(S_OK == return) HRESULT RepaintVideo(_In_ HWND hwndApp, _In_ HDC hDC);
	_Success_(S_OK == return) HRESULT DisplayModeChanged();

private:
	CaptureEngineCOM::IBasicVideo2Ptr  m_pBasicVideo;
	CaptureEngineCOM::IMediaEventExPtr m_pEvent;
	CaptureEngineCOM::IVideoWindowPtr  m_pPreviewWindow;

private:
	typedef std::unique_ptr<IVideoRenderer> IVideoRendererPtr;
	IVideoRendererPtr m_pVideoRenderer;
	CaptureEngine& m_CaptureEngine;
	HWND m_hParent;
};

}
// namespace WordJewelsImager

#endif // _CAPTUREPREVIEWWINDOW_H
