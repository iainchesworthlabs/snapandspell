#pragma once
#ifndef _CAPTUREENGINE_H
#define _CAPTUREENGINE_H

#include <Windows.h>
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>
#include <cstdint>
#include <vector>
#include "CaptureDevice.h"
#include "DebugStream.h"

namespace WordJewelsImager
{

class CaptureEngine
{
public:
	CaptureEngine();
	virtual ~CaptureEngine();
	
public:
	uint32_t AvailableDeviceCount() const;
	HRESULT QueryDeviceInfo(const uint32_t deviceId);
	HRESULT SelectVideoDevice(const uint32_t deviceId);
	HRESULT GetNativeVideoSize(uint64_t& width, uint64_t& height);
	HRESULT SetVideoPosition(RECT rcSrc, RECT rcDest);
	HRESULT RepaintVideo(HWND hWnd, HDC hDC);
	HRESULT InitWindowlessVMR(HWND hwndApp);

public:
	bool IsBoundToDevice() const;
	
private:
	HRESULT InitCaptureGraphBuilder();
	HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum);
	HRESULT EnumerateDeviceInformation(IEnumMoniker **ppEnum);
	
public:
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

private:
	IGraphBuilder *m_pGraph;
	ICaptureGraphBuilder2 *m_pBuild;
	IBaseFilter *m_pCap;
	IMediaControl *m_pControl;
	IMediaEvent *m_pEvent;
	IVMRWindowlessControl9 *m_pWc;
	std::vector<CaptureDevice> m_Devices;
	bool m_IsBound;
};

}
// namespace WordJewelsImager

#endif // !_CAPTUREENGINE_H
