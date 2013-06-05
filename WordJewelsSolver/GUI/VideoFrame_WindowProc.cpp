#include "Stdafx.h"

#include "CaptureManager.h"
#include "CaptureSupport.h"
#include "DebugStream.h"
#include "VideoFrame_WindowProc.h"

#include <cstdint>

// The imager to decode video capture of the screen
WordJewelsImager::CaptureManager g_CaptureManager;
WordJewelsImager::CaptureSupport g_CaptureSupport;

void VideoFrame_OnClose(HWND hWnd)
{
	::DestroyWindow(hWnd);
	hWnd = nullptr;
}

BOOL VideoFrame_OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	g_CaptureManager.DisplayVideo();
	return FALSE;
}

void VideoFrame_OnDestroy(HWND hWnd)
{
	::PostQuitMessage(0);
}

void VideoFrame_DisplayChange(HWND hWnd, UINT bitsPerPixel, UINT cxscreen, UINT cyscreen)
{
	g_CaptureManager.DisplayModeChanged();
}

DWORD VideoFrame_GetDlgCode(HWND hWnd, LPMSG pMsg)
{
	static bool setmeup(true);
	DWORD dwResult(0);

	if (setmeup)
	{
		setmeup = false;
		g_CaptureManager.SetVideoWindow(hWnd);
		g_CaptureManager.DisplayVideo();
	}

	return dwResult;
}

void VideoFrame_OnPaint(HWND hWnd)
{
	// Request the VMR to paint the video.
	g_CaptureManager.RepaintVideo();
}

void VideoFrame_OnSize(HWND hWnd, UINT state, int32_t cx, int32_t cy)
{
	g_CaptureManager.SetVideoPosition();
}

void VideoFrame_OnWindowPosChanged(HWND hWnd, const LPWINDOWPOS lpwpos)
{
	g_CaptureManager.SetVideoPosition();
}

LRESULT CALLBACK VideoFrame_WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	HANDLE_MSG(hWnd, WM_CLOSE, VideoFrame_OnClose);
	HANDLE_MSG(hWnd, WM_CREATE, VideoFrame_OnCreate);
	HANDLE_MSG(hWnd, WM_DESTROY, VideoFrame_OnDestroy);
	HANDLE_MSG(hWnd, WM_DISPLAYCHANGE, VideoFrame_DisplayChange);
	HANDLE_MSG(hWnd, WM_GETDLGCODE, VideoFrame_GetDlgCode);
	HANDLE_MSG(hWnd, WM_PAINT, VideoFrame_OnPaint);
	HANDLE_MSG(hWnd, WM_SIZE, VideoFrame_OnSize);
	HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGED, VideoFrame_OnWindowPosChanged);
	}

	return ::DefSubclassProc(hWnd, msg, wParam, lParam);
}
