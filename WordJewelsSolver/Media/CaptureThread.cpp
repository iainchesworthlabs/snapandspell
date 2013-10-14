#include "Stdafx.h"

#include "CaptureThread.h"

namespace WordJewelsImager
{

CaptureThread::CaptureThread() : m_EventOccurred(), m_ImagerThread(), m_EventMutex(), m_CaptureManager(), m_CaptureSupport() 
{
	for (bool& b : m_Events)
	{
		b = false;
	}

	m_ImagerThread = std::thread(&CaptureThread::CaptureThreadFunc, this);
}

CaptureThread::~CaptureThread()
{
}

void CaptureThread::SetVideoWindow(HWND hVideoWnd)
{
	m_CaptureManager.SetVideoWindow(hVideoWnd);
}

void CaptureThread::TriggerCreateEvent()
{
	TriggerEvent(SupportedEvents::Create);
}

void CaptureThread::TriggerDestroyEvent()
{
	TriggerEvent(SupportedEvents::Destroy);
}

void CaptureThread::TriggerDisplayChangeEvent()
{
	TriggerEvent(SupportedEvents::DisplayChange);
}

void CaptureThread::TriggerPaintEvent()
{
	TriggerEvent(SupportedEvents::Paint);
}

void CaptureThread::TriggerSizeEvent()
{
	TriggerEvent(SupportedEvents::Size);
}

void CaptureThread::TriggerWindowPosChangedEvent()
{
	TriggerEvent(SupportedEvents::WindowPosChanged);
}

void CaptureThread::TriggerEvent(CaptureThread::SupportedEvents eventType)
{
	{
		std::unique_lock<std::mutex> lock(m_EventMutex);
		m_Events[eventType] = true;
	}

	m_EventOccurred.notify_one();
}

void CaptureThread::CaptureThreadFunc()
{
	// Do any pre-use initialisation (rather than deferring it)

	do
	{
		std::unique_lock<std::mutex> lock(m_EventMutex);
		m_EventOccurred.wait(lock, [&, this]()
		{
			return true;
		});

		// Handle each of the event types....
		if (true == m_Events[SupportedEvents::Create])
		{
			m_CaptureManager.DisplayVideo();
		}
		if (true == m_Events[SupportedEvents::Destroy])
		{
		}
		if (true == m_Events[SupportedEvents::DisplayChange])
		{
			m_CaptureManager.DisplayModeChanged();
		}
		if (true == m_Events[SupportedEvents::Paint])
		{
			m_CaptureManager.RepaintVideo();
		}
		if (true == m_Events[SupportedEvents::Size])
		{
			m_CaptureManager.SetVideoPosition();
		}
		if (true == m_Events[SupportedEvents::WindowPosChanged])
		{
			m_CaptureManager.SetVideoPosition();
		}

		// Reset any of the triggered flags.
		for (bool& b : m_Events)
		{
			b = false;
		}
	}
	while (true);
}

}
// namespace WordJewelsImager
