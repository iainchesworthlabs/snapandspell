#include "Stdafx.h"

#include "CaptureThread.h"
#include "DebugStream.h"
#include "VideoFrame_WindowProc.h"

#include <cstdint>
#include <memory>

// The imager to decode video capture of the screen
WordJewelsImager::CaptureThread g_CaptureThread;
std::once_flag g_CaptureInitialisation;

void VideoFrame_OnClose(HWND hWnd)
{
	::DestroyWindow(hWnd);
	hWnd = nullptr;
}

BOOL VideoFrame_OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	g_CaptureThread.TriggerCreateEvent();
	return FALSE;
}

void VideoFrame_OnDestroy(HWND hWnd)
{
	::PostQuitMessage(0);
}

void VideoFrame_DisplayChange(HWND hWnd, UINT bitsPerPixel, UINT cxscreen, UINT cyscreen)
{
	g_CaptureThread.TriggerDisplayChangeEvent();
}

DWORD VideoFrame_GetDlgCode(HWND hWnd, LPMSG pMsg)
{
	DWORD dwResult(0);

	return dwResult;
}

void VideoFrame_LeftButtonDoubleClick(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	std::call_once(g_CaptureInitialisation, [&, hWnd]
	{
		g_CaptureThread.SetVideoWindow(hWnd);
		g_CaptureThread.TriggerCreateEvent();
	});
}

void VideoFrame_OnPaint(HWND hWnd)
{
	// Request the VMR to paint the video.
	g_CaptureThread.TriggerPaintEvent();
}

void VideoFrame_OnSize(HWND hWnd, UINT state, int32_t cx, int32_t cy)
{
	g_CaptureThread.TriggerSizeEvent();
}

void VideoFrame_OnWindowPosChanged(HWND hWnd, const LPWINDOWPOS lpwpos)
{
	g_CaptureThread.TriggerWindowPosChangedEvent();
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
	HANDLE_MSG(hWnd, WM_LBUTTONDBLCLK, VideoFrame_LeftButtonDoubleClick);
	HANDLE_MSG(hWnd, WM_PAINT, VideoFrame_OnPaint);
	HANDLE_MSG(hWnd, WM_SIZE, VideoFrame_OnSize);
	HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGED, VideoFrame_OnWindowPosChanged);
	}

	return ::DefSubclassProc(hWnd, msg, wParam, lParam);
}
