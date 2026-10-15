#pragma once
#ifndef _CAPTUREMANAGER_H
#define _CAPTUREMANAGER_H

#include <Windows.h>
#include "CaptureDeviceCollection.h"
#include "CaptureEngine.h"
#include "CapturePreviewWindow.h"
#include "GraphDebugger.h"

namespace WordJewelsImager
{

class CaptureManager
{
public:
	CaptureManager();
	virtual ~CaptureManager();

public:
	void EnablePreviewAudio();
	void EnablePreviewVideo(HWND hVideoWnd);

public:
	void StartShowingVideo();
	void DisplayModeChanged();
	void HandleGraphEvent();
	void RepaintVideo();
	void SetVideoPosition();

private:
	CaptureDeviceCollection m_AvailableDevices;
	CaptureEngine m_CaptureEngine;
	CapturePreviewWindow m_PreviewWindow;
	GraphDebugger m_GraphDebugger;

private:
	DWORD m_dwGraphRegister;
	HWND m_hVideoWnd;
	RECT m_rcDest;
};

}
// namespace WordJewelsImager

#endif // _CAPTUREMANAGER_H
