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
#include "CaptureDeviceCollection.h"
#include "CaptureEngineCOM.h"
#include "DebugStream.h"
#include "ICaptureDevice.h"
#include "IVideoRenderer.h"
#include "NullCaptureDevice.h"
#include "SafeRelease.h"

namespace WordJewelsImager
{

class CaptureEngine
{
	static NullCaptureDevice m_NullCaptureDevice;

public:
	CaptureEngine();
	virtual ~CaptureEngine();

public:
	HRESULT GetCaptureGraph(CaptureEngineCOM::IGraphBuilderPtr& pGraph);
	HRESULT SetCaptureDevice(CaptureDevice& captureDevice);
	
public:
	HRESULT Start();
	HRESULT Pause();
	HRESULT Stop();
	bool IsRunning() const;

private:
	enum EngineStates
	{
		Running, Paused, Stopped
	};

private:
	CaptureEngineCOM::IGraphBuilderPtr         m_pGraph;
	//CaptureEngineCOM::ICaptureGraphBuilder2Ptr m_pBuilder;
	CaptureEngineCOM::IMediaControlPtr         m_pControl;
	CaptureEngineCOM::IMediaPositionPtr        m_pPosition;
	CaptureEngineCOM::IEnumMonikerPtr          m_pEnum;

	ICaptureDevice* m_pCaptureDevice; ///FIXME why can't this be a reference???
	EngineStates m_EngineStatus;
};

}
// namespace WordJewelsImager

#endif // !_CAPTUREENGINE_H
