#include "Stdafx.h"

#include "CaptureEngine.h"
#include <cstdint>
#include <string>

namespace WordJewelsImager
{

CaptureEngine::CaptureEngine() : 
	m_pGraph(nullptr), 
	m_pBuild(nullptr), 
	m_pCap(nullptr), 
	m_pControl(nullptr), 
	m_pEvent(nullptr), 
	m_pWc(nullptr), 
	m_Devices(),
	m_IsBound(false)
{
	IEnumMoniker *pEnum(nullptr);
	IMoniker* pMoniker(nullptr);
	HRESULT hr(S_OK);

	if (FAILED(hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
	{
		wdebugstream() << TEXT("DEBUG: Could not initialize COM library") << std::endl;
		throw;
	}
	else if (FAILED(hr = InitCaptureGraphBuilder()))
	{
		wdebugstream() << TEXT("DEBUG: Could not create the Filter Graph Manager") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_pGraph->QueryInterface(IID_IMediaControl, reinterpret_cast<void **>(&m_pControl))))
	{
		wdebugstream() << TEXT("DEBUG: Could not create the Media Control") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_pGraph->QueryInterface(IID_IMediaEvent, reinterpret_cast<void **>(&m_pEvent))))
	{
		wdebugstream() << TEXT("DEBUG: Could not create the Media Event Interfaces") << std::endl;
		throw;
	}
	else if (FAILED(hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to enumerate devices") << std::endl;
		throw;
	}
	else
	{
		/*else if (FAILED(hr = InitWindowlessVMR(hWnd)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to initialise windowless VMR") << std::endl;
			throw;
		}
		else if (FAILED(hr = ConfigureWindowlessVMRClient(hWnd)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to set window rectangle for video") << std::endl;
			throw;
		}
		else if (FAILED(hr = m_pBuild->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pCap, nullptr, nullptr)))
		{
			wdebugstream() << TEXT("DEBUG: RenderStream() failed") << std::endl;
			throw;
		}
		else if (FAILED(hr = m_pControl->Run()))
		{
			wdebugstream() << TEXT("DEBUG: Run() failed") << std::endl;
			throw;
		}
		else 
		{
			long evCode;
				
			if (FAILED(hr = m_pEvent->WaitForCompletion(INFINITE, &evCode)))
			{
				wdebugstream() << TEXT("DEBUG: WaitForCompletion() failed") << std::endl;
				throw;
			}
			else if (FAILED(hr = m_pControl->Stop()))
			{
				wdebugstream() << TEXT("DEBUG: Stop() failed") << std::endl;
				throw;
			}
			else
			{
				pMoniker->Release();
				pEnum->Release();
			}
		}*/
	}
}

CaptureEngine::~CaptureEngine()
{
	if (nullptr != m_pWc)
	{
		m_pWc->Release();
		m_pWc = nullptr;
	}

	if (nullptr != m_pEvent)
	{
		m_pEvent->Release();
		m_pEvent = nullptr;
	}
		
	if (nullptr != m_pGraph)
	{
		m_pGraph->Release();
		m_pGraph = nullptr;
	}
		
	if (nullptr != m_pControl)
	{
		m_pControl->Release();
		m_pControl = nullptr;
	}
		
	if (nullptr != m_pCap)
	{
		m_pCap->Release();
		m_pCap = nullptr;
	}
		
	if (nullptr != m_pBuild)
	{
		m_pBuild->Release();
		m_pBuild = nullptr;
	}
		
	if (nullptr != m_pGraph)
	{
		m_pGraph->Release();
		m_pGraph = nullptr;
	}

	::CoUninitialize();
}

uint32_t CaptureEngine::AvailableDeviceCount() const
{
	return m_Devices.size();
}

HRESULT CaptureEngine::QueryDeviceInfo(const uint32_t deviceId)
{
	return S_FALSE;
}

HRESULT CaptureEngine::SelectVideoDevice(const uint32_t deviceId)
{
	HRESULT hr(S_FALSE);

	if (AvailableDeviceCount() <= deviceId)
	{
		wdebugstream() << TEXT("DEBUG: Selected a video device which does not exist") << std::endl;
	}
	else if (nullptr == m_Devices[deviceId].GetMoniker())
	{
		wdebugstream() << TEXT("DEBUG: Provided moniker was NULL; this should not happen") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_Devices[deviceId].GetMoniker()->BindToObject(0, 0, IID_IBaseFilter, reinterpret_cast<void**>(&m_pCap))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to bind to moniker") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_pGraph->AddFilter(m_pCap, TEXT("Capture Filter"))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add video device to graph") << std::endl;
		throw;
	}
	else
	{
		m_IsBound = true;
	}

	return hr;
}

HRESULT CaptureEngine::GetNativeVideoSize(uint64_t& width, uint64_t& height)
{
	long tmpWidth(0), tmpHeight(0);
	HRESULT hr;

	if (FAILED(hr = m_pWc->GetNativeVideoSize(&tmpWidth, &tmpHeight, nullptr, nullptr)))
	{
		///FIXME
		wdebugstream() << TEXT("DEBUG: Failed get dimensions of native video") << std::endl;
		throw;
	}
	else
	{
		width = tmpWidth;
		height = tmpHeight;
	}

	return hr;
}

HRESULT CaptureEngine::SetVideoPosition(RECT rcSrc, RECT rcDest)
{
	HRESULT hr;

	if (FAILED(hr = m_pWc->SetVideoPosition(&rcSrc, &rcDest)))
	{
		///FIXME
		wdebugstream() << TEXT("DEBUG: Failed set dimension and position of output video") << std::endl;
		throw;
	}
	else
	{
	}

	return hr;
}

HRESULT CaptureEngine::RepaintVideo(HWND hWnd, HDC hDC)
{
	HRESULT hr;

	if (FAILED(hr = m_pWc->RepaintVideo(hWnd, hDC)))
	{
		///FIXME
		wdebugstream() << TEXT("DEBUG: Failed repaint video in window") << std::endl;
		throw;
	}
	else
	{
	}

	return hr;
}

HRESULT CaptureEngine::InitWindowlessVMR(HWND hwndApp)
{
	IVMRFilterConfig9* pConfig(nullptr); 
	IBaseFilter* pVmr(nullptr);
	HRESULT hr;

	if (FAILED(hr = ::CoCreateInstance(CLSID_VideoMixingRenderer9, nullptr, CLSCTX_INPROC, IID_IBaseFilter, reinterpret_cast<void**>(&pVmr))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create IBaseFilter") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_pGraph->AddFilter(pVmr, L"Video Mixing Renderer")))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set filter graph") << std::endl;
		throw;
	}
	else if (FAILED(hr = pVmr->QueryInterface(IID_IVMRFilterConfig9, reinterpret_cast<void**>(&pConfig))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create IID_IVMRFilterConfig") << std::endl;
		throw;
	}
	else if (FAILED(hr = pConfig->SetRenderingMode(VMRMode_Windowless)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set rendering mode to VMRMode_Windowless") << std::endl;
		throw;
	}
	else if (FAILED(hr = pVmr->QueryInterface(IID_IVMRWindowlessControl9, reinterpret_cast<void**>(&m_pWc))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create IID_IVMRWindowlessControl") << std::endl;
		throw;
	}
	else if (FAILED (hr = m_pWc->SetVideoClippingWindow(hwndApp)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set clipping window") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_pBuild->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCap, nullptr, pVmr)))
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
		wdebugstream() << TEXT("DEBUG: Configured and initialised video in window") << std::endl;
	}

	pConfig->Release();
	pVmr->Release();
	return hr; 
}

bool CaptureEngine::IsBoundToDevice() const
{
	return m_IsBound;
}

HRESULT CaptureEngine::InitCaptureGraphBuilder()
{
	HRESULT hr;

	if (FAILED(hr = ::CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, reinterpret_cast<void**>(&m_pBuild))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create IID_ICaptureGraphBuilder2") << std::endl;
		throw;
	}
	else if (FAILED(hr = ::CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, reinterpret_cast<void**>(&m_pGraph))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create IID_IGraphBuilder") << std::endl;
		throw;
	}
	else if (FAILED(hr = m_pBuild->SetFiltergraph(m_pGraph)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to set filter graph") << std::endl;
		throw;
	}
	else
	{
	}

	return hr;
}

HRESULT CaptureEngine::EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	HRESULT hr;
		
	if (FAILED(hr = ::CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to create ICreateDevEnum") << std::endl;
		throw;
	}
	else if (FAILED(pDevEnum->CreateClassEnumerator(category, ppEnum, 0)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to enumerate specified category") << std::endl;
		throw;
	}
	else if (S_FALSE == hr)
	{
		wdebugstream() << TEXT("DEBUG: Failed to find a suitable device") << std::endl;

		// The category is empty. Treat as an error.
		hr = VFW_E_NOT_FOUND;
		pDevEnum->Release();
	}
	else if (FAILED(EnumerateDeviceInformation(ppEnum)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to enumerate specific device information") << std::endl;
	}
	else
	{
		// No nothing.
	}

	return hr;
}

HRESULT CaptureEngine::EnumerateDeviceInformation(IEnumMoniker **ppEnum)
{
	IMoniker* pTempMoniker(nullptr);

	while (S_OK == (*ppEnum)->Next(1, &pTempMoniker, nullptr))
	{
		IPropertyBag *pPropBag(nullptr);
		HRESULT hr;

		if (FAILED(hr = pTempMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag))))
		{
			wdebugstream() << TEXT("DEBUG: Failed to bind to storage (0x") << std::hex << hr << TEXT(")") << std::endl;

			pTempMoniker->Release();
			continue;  
		}
		else
		{
			m_Devices.push_back(CaptureDevice(pTempMoniker, pPropBag));
		}
	}

	return S_OK;
}

}
// namespace WordJewelsImager
