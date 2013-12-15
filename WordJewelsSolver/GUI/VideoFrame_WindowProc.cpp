#include "Stdafx.h"

#include <windowsx.h>

#include <cstdint>
#include <memory>
#include <mutex>
	
#include "CoInitialize.h"
#include "CaptureManager.h"
#include "CaptureSupport.h"
#include "DebugStream.h"
#include "VideoFrame_WindowProc.h"

CCoInitialize g_ComInit;
WordJewelsImager::CaptureManager* g_CaptureManager;
WordJewelsImager::CaptureSupport* g_CaptureSupport;
static std::once_flag g_CaptureInitialisation;

void VideoFrame_OnClose(HWND hWnd)
{
	::DestroyWindow(hWnd);
	hWnd = nullptr;
}

BOOL VideoFrame_OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	// Don't assume this will be called implicitly during window
	// creation as subclassed windows don't expose the WM_CREATE
	// message to the subclassee.
	//
	// NOTE NOTE NOTE: This is called from the MainWindow_DialogProc

	try
	{
		std::call_once(g_CaptureInitialisation, [&, hWnd]
		{
			if (nullptr == g_CaptureManager)
			{
				g_CaptureManager = new WordJewelsImager::CaptureManager();
			}
			else
			{
				wdebugstream() << TEXT("DEBUG: Object construction code was called twice") << std::endl;
			}

			g_CaptureManager->EnablePreviewVideo(hWnd);
		});
	}
	catch (...)
	{
		wdebugstream() << TEXT("DEBUG: Exception thrown while initialising preview window") << std::endl;
	}

	return FALSE;
}

void VideoFrame_OnDestroy(HWND hWnd)
{
	if (nullptr != g_CaptureManager)
	{
		g_CaptureManager->DisplayModeChanged();
	}

	if (nullptr != g_CaptureManager)
	{
		delete g_CaptureManager;
		g_CaptureManager = nullptr;
	}

	if (nullptr != g_CaptureSupport)
	{
		delete g_CaptureSupport;
		g_CaptureSupport = nullptr;
	}

	::PostQuitMessage(0);
}

void VideoFrame_DisplayChange(HWND hWnd, UINT bitsPerPixel, UINT cxscreen, UINT cyscreen)
{
	if (nullptr != g_CaptureManager)
	{
		g_CaptureManager->DisplayModeChanged();
	}
}

void VideoFrame_OnPaint(HWND hWnd)
{
	if (nullptr != g_CaptureManager)
	{
		g_CaptureManager->RepaintVideo();
	}
}

void VideoFrame_OnSize(HWND hWnd, UINT state, int32_t cx, int32_t cy)
{
	if (nullptr != g_CaptureManager)
	{
		g_CaptureManager->SetVideoPosition();
	}
}

void VideoFrame_OnWindowPosChanged(HWND hWnd, const LPWINDOWPOS lpwpos)
{
	if (nullptr != g_CaptureManager)
	{
		g_CaptureManager->SetVideoPosition();
	}
}

LRESULT CALLBACK VideoFrame_WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	HANDLE_MSG(hWnd, WM_CLOSE, VideoFrame_OnClose);
	HANDLE_MSG(hWnd, WM_CREATE, VideoFrame_OnCreate);
	HANDLE_MSG(hWnd, WM_DESTROY, VideoFrame_OnDestroy);
	HANDLE_MSG(hWnd, WM_DISPLAYCHANGE, VideoFrame_DisplayChange);
	HANDLE_MSG(hWnd, WM_PAINT, VideoFrame_OnPaint);
	HANDLE_MSG(hWnd, WM_SIZE, VideoFrame_OnSize);
	HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGED, VideoFrame_OnWindowPosChanged);

	case WordJewelsImager::CaptureSupport::WM_GRAPHNOTIFY:
		g_CaptureManager->HandleGraphEvent();
		break;

	default:
		break;
	}

	return ::DefSubclassProc(hWnd, msg, wParam, lParam);
}
