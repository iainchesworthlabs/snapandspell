#include "Stdafx.h"

#include <cstdint>
#include <string>

#include "CaptureEngine.h"
#include "DxVersion.h"
#include "EVR_Renderer.h"
#include "OSVersion.h"
#include "SafeRelease.h"
#include "VMR7_Renderer.h"
#include "VMR9_Renderer.h"

namespace WordJewelsImager
{

CaptureEngine::CaptureEngine() : 
	m_pVideoRenderer(nullptr),
	m_ComInit(),
	m_pGraph(nullptr), 
	m_pBuilder(nullptr),
	m_pControl(nullptr), 
	m_pEvent(nullptr),
	m_pEnum(nullptr), 
	m_pCap(nullptr),
	m_DeviceLoader(),
	m_HaveDevice(false)
{
	HRESULT hr(S_OK);

	try 
	{
		if (FAILED(m_ComInit))
		{
			wdebugstream() << TEXT("DEBUG: Could not initialize COM library") << std::endl;
			throw;
		}
		else if (CaptureDeviceLoader::Success != m_DeviceLoader.Initialise())
		{
			wdebugstream() << TEXT("DEBUG: Failed to initialise device loader") << std::endl;
			throw;
		}
		else if (FAILED(hr = m_pBuilder.CreateInstance(CLSID_CaptureGraphBuilder2)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to create IID_CaptureGraphBuilder2") << std::endl;
			throw;
		}
		else if (FAILED(hr = m_pGraph.CreateInstance(CLSID_FilterGraph)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to create IID_IFilterGraph") << std::endl;
			throw;
		}
		else if (FAILED(hr = m_pBuilder->SetFiltergraph(m_pGraph)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to set filter graph") << std::endl;
			throw;
		}
		else if (FAILED(hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pControl))))
		{
			wdebugstream() << TEXT("DEBUG: Could not create the Media Control") << std::endl;
			throw;
		}
		else if (FAILED(hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pEvent))))
		{
			wdebugstream() << TEXT("DEBUG: Could not create the Media Event Interfaces") << std::endl;
			throw;
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
	SafeRelease(&m_pCap);
	SafeRelease(&m_pEnum);
	SafeRelease(&m_pEvent);
	SafeRelease(&m_pControl);
	SafeRelease(&m_pBuilder);
	SafeRelease(&m_pGraph);
}

uint32_t CaptureEngine::AvailableDeviceCount() const
{
	return m_DeviceLoader.GetNumSources();
}

bool CaptureEngine::IsRunning() const
{
	return m_HaveDevice;
}

HRESULT CaptureEngine::QueryDeviceInfo(const uint32_t deviceId)
{
	return S_FALSE;
}

HRESULT CaptureEngine::SelectVideoDevice(const uint32_t deviceId)
{
	CaptureEngineCOM::IBaseFilterPtr pBaseFilter(nullptr);
	HRESULT hr(S_FALSE);

	if (nullptr == m_pGraph)
	{
		wdebugstream() << TEXT("DEBUG: Graph pointer is NULL; cannot continue") << std::endl;
		throw;
	}
	else if (nullptr == (pBaseFilter = m_DeviceLoader.GetSource(deviceId)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to select a video device") << std::endl;
	}
	else if (FAILED(hr = m_pGraph->AddFilter(pBaseFilter, TEXT("Capture Filter"))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add video device to graph") << std::endl;
		throw;
	}
	else
	{
		m_pCap = pBaseFilter;
		m_HaveDevice = true;
	}

	return hr;
}

HRESULT CaptureEngine::GetNativeVideoSize(uint64_t& width, uint64_t& height)
{
	long tmpWidth(0), tmpHeight(0);
	HRESULT hr(S_OK);

	if (nullptr == m_pVideoRenderer)
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is NULL; cannot continue") << std::endl;
		throw;
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
	}
	else if (FAILED(hr = m_pVideoRenderer->GetNativeVideoSize(&tmpWidth, &tmpHeight, nullptr, nullptr)))
	{
		wdebugstream() << TEXT("DEBUG: Failed get dimensions of native video") << std::endl;
		throw;
	}
	else
	{
		// Success.
		width = tmpWidth;
		height = tmpHeight;
	}

	return hr;
}

HRESULT CaptureEngine::SetVideoPosition(HWND hwndApp, RECT rcDest)
{
	HRESULT hr(S_OK);

	if (nullptr == m_pVideoRenderer)
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is NULL; cannot continue") << std::endl;
		throw;
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
	}
	else if (FAILED(hr = m_pVideoRenderer->UpdateVideoWindow(hwndApp, &rcDest)))
	{
		wdebugstream() << TEXT("DEBUG: Failed set dimension and position of output video") << std::endl;
	}
	else
	{
		// Success.
	}

	return hr;
}

HRESULT CaptureEngine::RepaintVideo(HWND hWnd, HDC hDC)
{
	HRESULT hr(S_OK);

	if (nullptr == m_pVideoRenderer)
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is NULL; cannot continue") << std::endl;
		throw;
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
	}
	else if (FAILED(hr = m_pVideoRenderer->Repaint(hWnd, hDC)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to repaint video in window") << std::endl;
	}
	else
	{
		// Success.
	}

	return hr;
}

HRESULT CaptureEngine::DisplayModeChanged()
{
	HRESULT hr(S_OK);

	if (nullptr == m_pVideoRenderer)
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is NULL; cannot continue") << std::endl;
		throw;
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
	}
	else if (FAILED(hr = m_pVideoRenderer->DisplayModeChanged()))
	{
		wdebugstream() << TEXT("DEBUG: Failed to manage display mode change") << std::endl;
	}
	else
	{
		// Success.
	}

	return hr;
}

HRESULT CaptureEngine::InitialiseRenderer(HWND hwndApp)
{
	CDxVersion DxVersion;
	COSVersion OSVersion;
	HRESULT hr(S_OK);

	switch(OSVersion.Version())
	{
	case COSVersion::Windows8:
	case COSVersion::Windows7:
	case COSVersion::Vista:
		wdebugstream() << TEXT("DEBUG: Using EVR renderer....") << std::endl;
		m_pVideoRenderer = IVideoRendererPtr(new EVR_Renderer());
		break;

	case COSVersion::XP:
		switch (DxVersion.Version())
		{
		case CDxVersion::DirectX_9c:
		case CDxVersion::DirectX_9b:
		case CDxVersion::DirectX_9a:
		case CDxVersion::DirectX_9:
			wdebugstream() << TEXT("DEBUG: Using VMR-9 renderer....") << std::endl;
			m_pVideoRenderer = IVideoRendererPtr(new VMR9_Renderer());
			break;

		default:
			wdebugstream() << TEXT("DEBUG: Using VMR-7 renderer....") << std::endl;
			m_pVideoRenderer = IVideoRendererPtr(new VMR7_Renderer());
			break;
		}
		break;

	default:
		//TODO Determine whether hardware overlays are supported to pick the renderer.
		//m_pVideoRenderer = new OverlayMixer_Renderer();
		//m_pVideoRenderer = new LegacyVideo_Renderer();
		break;
	}

	if (FAILED(hr = m_pVideoRenderer->AddToGraph(m_pGraph, hwndApp)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add video mixing renderer to graph") << std::endl;
		throw;
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Configured and initialised video in window") << std::endl;
	}

	return hr; 
}

HRESULT CaptureEngine::Start() const
{
	HRESULT hr;

	if ((nullptr == m_pBuilder) || (nullptr == m_pCap) || (nullptr == m_pControl))
	{
		wdebugstream() << TEXT("DEBUG: Cannot start capture engine; not initialised") << std::endl;
	}
	else if (FAILED(hr = m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCap, nullptr, nullptr)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to render video to the display") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_pControl->Run()))
	{
		wdebugstream() << TEXT("DEBUG: Failed to start display of video") << std::endl;
		throw;
	}
	else
	{
		// Success.
	}

	return hr;
}

HRESULT CaptureEngine::Pause() const
{
	return S_FALSE;
}

HRESULT CaptureEngine::Stop() const
{
	return S_FALSE;
}

}
// namespace WordJewelsImager
