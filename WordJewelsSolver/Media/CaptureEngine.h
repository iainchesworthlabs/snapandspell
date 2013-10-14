#pragma once
#ifndef _CAPTUREENGINE_H
#define _CAPTUREENGINE_H

#include <Windows.h>
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "CaptureDevice.h"
#include "CaptureDeviceLoader.h"
#include "CaptureEngineCOM.h"
#include "CoInitialize.h"
#include "DebugStream.h"
#include "IVideoRenderer.h"
#include "SafeRelease.h"

namespace WordJewelsImager
{

class CaptureEngine
{
public:
	CaptureEngine();
	virtual ~CaptureEngine();
	
public:
	uint32_t AvailableDeviceCount() const;
	bool IsRunning() const;

public:
	HRESULT QueryDeviceInfo(const uint32_t deviceId);
	HRESULT SelectVideoDevice(const uint32_t deviceId);
	HRESULT GetNativeVideoSize(uint64_t& width, uint64_t& height);
	HRESULT SetVideoPosition(HWND hwndApp, RECT rcDest);
	HRESULT RepaintVideo(HWND hwndApp, HDC hDC);
	HRESULT DisplayModeChanged();
	HRESULT InitialiseRenderer(HWND hwndApp);
	
public:
	HRESULT Start() const;
	HRESULT Pause() const;
	HRESULT Stop() const;

private:
	HRESULT CreateVideoRenderer();
	HRESULT TearDownGraph();
	
private:
	typedef std::unique_ptr<IVideoRenderer> IVideoRendererPtr;

private:
	IVideoRendererPtr m_pVideoRenderer;
	CCoInitialize m_ComInit;

	CaptureEngineCOM::IGraphBuilderPtr         m_pGraph;
	CaptureEngineCOM::ICaptureGraphBuilder2Ptr m_pBuilder;
	CaptureEngineCOM::IMediaControlPtr         m_pControl;
	CaptureEngineCOM::IMediaEventPtr           m_pEvent;
	CaptureEngineCOM::IEnumMonikerPtr          m_pEnum;
	CaptureEngineCOM::IBaseFilterPtr           m_pCap;

	CaptureDeviceLoader m_DeviceLoader;
	bool m_HaveDevice;
};

}
// namespace WordJewelsImager

#endif // !_CAPTUREENGINE_H
