#include "pch.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "CaptureEngine.h"
#include "CaptureSupport.h"
#include "SafeRelease.h"
#include "TracedThrow.h"

namespace WordJewelsImager
{

NullCaptureDevice CaptureEngine::m_NullCaptureDevice = NullCaptureDevice();

CaptureEngine::CaptureEngine() :
	m_pGraph(nullptr), 
	// m_pBuilder(nullptr),
	m_pControl(nullptr), 
	m_pEnum(nullptr),
	m_pCaptureDevice(&m_NullCaptureDevice),
	m_EngineStatus(Stopped)
{
	HRESULT hr(S_OK);

	try 
	{
		/*if (FAILED(hr = m_pBuilder.CreateInstance(CLSID_CaptureGraphBuilder2)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to create IID_CaptureGraphBuilder2") << std::endl;
			throw;
		}
		else */
		if (FAILED(hr = m_pGraph.CreateInstance(CLSID_FilterGraph)))
		{
			DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to create IID_IFilterGraph"));
		}
		/*else if (FAILED(hr = m_pBuilder->SetFiltergraph(m_pGraph)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to set filter graph") << std::endl;
			throw;
		}*/
		else if (FAILED(hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pControl))))
		{
			DiagnosticsTracedThrow(std::runtime_error("DEBUG: Could not create the Media Control"));
		}
		else
		{
			// All done, chief!
		}
	}
	catch (const _com_error& e)
	{
		wdebugstream() << TEXT("COM ERROR: ") << e.ErrorMessage() << std::endl;
		assert(false);
	}
}

CaptureEngine::~CaptureEngine()
{
	SafeRelease(&m_pEnum);
	SafeRelease(&m_pControl);
	// SafeRelease(&m_pBuilder);
	SafeRelease(&m_pGraph);

	m_pCaptureDevice = &m_NullCaptureDevice;
}


HRESULT CaptureEngine::GetCaptureGraph(CaptureEngineCOM::IGraphBuilderPtr& pGraph)
{
	pGraph = m_pGraph;
	return S_OK;
}

HRESULT CaptureEngine::SetCaptureDevice(CaptureDevice& captureDevice)
{
	static const LPCWSTR CAPTURE_DEVICE_FILTER_NAME(TEXT("Capture Filter / Device"));
	HRESULT hr;

	if (nullptr == m_pGraph)
	{
		wdebugstream() << TEXT("DEBUG: Graph pointer is NULL; cannot continue") << std::endl;
		hr = E_FAIL;
	}
	else if (FAILED(hr = m_pGraph->AddFilter(captureDevice(), CAPTURE_DEVICE_FILTER_NAME)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add video device to graph") << std::endl;
		hr = E_FAIL;
	}
	else
	{
		m_pCaptureDevice = &captureDevice;
	}

	return hr;
}

HRESULT CaptureEngine::Start()
{
	CaptureEngineCOM::IPinPtr pOutputPin;
	HRESULT hr;

	if ((Paused != m_EngineStatus) && (Stopped != m_EngineStatus))
	{
		hr = VFW_E_WRONG_STATE;
	}
	// (nullptr == m_pBuilder) || 
	else if ((nullptr == m_pControl) || (nullptr == m_pCaptureDevice) || (nullptr == (*m_pCaptureDevice)()))
	{
		wdebugstream() << TEXT("DEBUG: Cannot start capture engine; not initialised") << std::endl;
		hr = E_FAIL;
	}
	//else if (FAILED(hr = CaptureSupport::FindFilterInterface))
	//else if (FAILED(hr = CaptureSupport::FindUnconnectedPin())
	//else if (FAILED(hr = m_pGraph->Render()))
	//{

	//}
	// else if (FAILED(hr = m_pBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, (*m_pCaptureDevice)(), nullptr, nullptr)))
	//{
	//	wdebugstream() << TEXT("DEBUG: Failed to render video to the display") << std::endl;
	//}
	else if (FAILED(hr = m_pControl->Run()))
	{
		wdebugstream() << TEXT("DEBUG: Failed to start display of video") << std::endl;
	}
	else
	{
		m_EngineStatus = Running;
	}

	return hr;
}

HRESULT CaptureEngine::Pause()
{
	HRESULT hr;

	if (Running != m_EngineStatus)
	{
		hr = VFW_E_WRONG_STATE;
	}
	else if (nullptr == m_pControl)
	{
		wdebugstream() << TEXT("DEBUG: Cannot pause capture engine; internal variables not initialised") << std::endl;
		hr = E_FAIL;
	}
	else if (FAILED(hr = m_pControl->Pause()))
	{
		wdebugstream() << TEXT("DEBUG: Failed to pause display of video") << std::endl;
	}
	else
	{
		m_EngineStatus = Paused;
	}
		
	return hr;
}

HRESULT CaptureEngine::Stop()
{
	HRESULT hr;

	if ((Running != m_EngineStatus) && (Paused != m_EngineStatus))
	{
		hr = VFW_E_WRONG_STATE;
	}
	else if (nullptr == m_pControl)
	{
		wdebugstream() << TEXT("DEBUG: Cannot stop capture engine; internal variables not initialised") << std::endl;
		hr = E_FAIL;
	}
	else if (FAILED(hr = m_pControl->Stop()))
	{
		wdebugstream() << TEXT("DEBUG: Failed to stop display of video") << std::endl;
	}
	else
	{
		m_EngineStatus = Stopped;
	}

	return hr;
}

bool CaptureEngine::IsRunning() const
{
	return ((Running == m_EngineStatus) ? true : false);
}

}
// namespace WordJewelsImager
