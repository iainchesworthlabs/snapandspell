#pragma once
#ifndef _CAPTURETHREAD_H
#define _CAPTURETHREAD_H

#include "CaptureManager.h"
#include "CaptureSupport.h"
#include "DebugStream.h"

#include <condition_variable>
#include <thread>

namespace WordJewelsImager
{

class CaptureThread
{
public:
	CaptureThread();
	virtual ~CaptureThread();

public:
	void SetVideoWindow(HWND hVideoWnd);

public:
	void TriggerCreateEvent();
	void TriggerDestroyEvent();
	void TriggerDisplayChangeEvent();
	void TriggerPaintEvent();
	void TriggerSizeEvent();
	void TriggerWindowPosChangedEvent();

private:
	enum SupportedEvents
	{
		Create, Destroy, DisplayChange, Paint, Size, WindowPosChanged, Count
	};

private:
	void TriggerEvent(SupportedEvents eventType);

private:
	std::condition_variable m_EventOccurred;
	bool m_Events[SupportedEvents::Count];
	std::thread m_ImagerThread;
	std::mutex m_EventMutex;
	void CaptureThreadFunc();

private:
	// The imager to decode video capture of the screen
	CaptureManager m_CaptureManager;
	CaptureSupport m_CaptureSupport;
};

}
// namespace WordJewelsImager

#endif // _CAPTURETHREAD_H
