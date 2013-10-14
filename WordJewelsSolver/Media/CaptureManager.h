#pragma once
#ifndef _CAPTUREMANAGER_H
#define _CAPTUREMANAGER_H

#include <Windows.h>
#include "CaptureEngine.h"

namespace WordJewelsImager
{

class CaptureManager
{
public:
	CaptureManager();
	CaptureManager(HWND hVideoWnd);
	virtual ~CaptureManager();

public:
	void SetVideoWindow(HWND hVideoWnd);
	void DisplayVideo();
	void DisplayModeChanged();
	void RepaintVideo();
	void SetVideoPosition();

private:
	CaptureEngine m_CaptureEngine;
	HWND m_hVideoWnd;
	RECT m_rcDest;
};

}
// namespace WordJewelsImager

#endif // !_CAPTUREMANAGER_H
