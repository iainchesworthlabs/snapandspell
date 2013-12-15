#pragma once
#ifndef _VIDEOFRAME_DIALOGPROC_H_
#define _VIDEOFRAME_DIALOGPROC_H_

#include <Windows.h>

void VideoFrame_OnClose(HWND hWnd);
BOOL VideoFrame_OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
void VideoFrame_OnDestroy(HWND hWnd);
void VideoFrame_DisplayChange(HWND hWnd, UINT bitsPerPixel, UINT cxscreen, UINT cyscreen);
void VideoFrame_OnPaint(HWND hWnd);
void VideoFrame_OnSize(HWND hWnd, UINT state, int32_t cx, int32_t cy);
void VideoFrame_OnWindowPosChanged(HWND hWnd, const LPWINDOWPOS lpwpos);

LRESULT CALLBACK VideoFrame_WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

#endif // _VIDEOFRAME_DIALOGPROC_H_
