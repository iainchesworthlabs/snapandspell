#include "Stdafx.h"

#include <cassert>
#include <stdexcept>

#include "CapturePreviewWindow.h"
#include "DebugStream.h"
#include "DxVersion.h"
#include "EVR_Renderer.h"
#include "OSVersion.h"
#include "SafeRelease.h"
#include "TracedThrow.h"
#include "VMR7_Renderer.h"
#include "VMR9_Renderer.h"

namespace WordJewelsImager
{

CapturePreviewWindow::CapturePreviewWindow(CaptureEngine& captureEngine) :
	m_pBasicVideo(nullptr),
	m_pEvent(nullptr),
	m_pPreviewWindow(nullptr),
	m_pVideoRenderer(nullptr),
	m_CaptureEngine(captureEngine),
	m_hParent(nullptr)
{
	CDxVersion DxVersion;
	COSVersion OSVersion;

	switch (OSVersion.Version())
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

	CaptureEngineCOM::IGraphBuilderPtr pGraph;
	HRESULT hr;

	if (FAILED(hr = m_CaptureEngine.GetCaptureGraph(pGraph)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to acquire graph pointer from engine"));
	}
	else if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Graph pointer is NULL; cannot continue"));
	}
	else if (FAILED(hr = pGraph->QueryInterface(IID_PPV_ARGS(&m_pEvent))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Could not create the Media Event Ex Interfaces"));
	}
	else
	{
		// All done!
	}
}

CapturePreviewWindow::~CapturePreviewWindow()
{
	SafeRelease(&m_pPreviewWindow);
	SafeRelease(&m_pEvent);
	SafeRelease(&m_pBasicVideo);

	// Destroy the video renderer.
	m_pVideoRenderer = nullptr;
	m_hParent = nullptr;
}

_Success_(S_OK == return) HRESULT CapturePreviewWindow::SetWindow(_In_ HWND hParent)
{
	assert(nullptr != m_pVideoRenderer);
	assert(nullptr != hParent);
	assert(nullptr != m_pEvent);
	
	CaptureEngineCOM::IGraphBuilderPtr pGraph;
	HRESULT hr;

	if (nullptr != m_hParent)
	{
		wdebugstream() << TEXT("DEBUG: Cleaning up and freeing previous preview video parent") << std::endl;
		///FIXME Disconnect renderer from graph.
	}

	// Save the new window handle for subsequent use.
	m_hParent = hParent;
	
	if (FAILED(hr = m_CaptureEngine.GetCaptureGraph(pGraph)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to acquire graph pointer from engine") << std::endl;
	}
	else if (nullptr == pGraph)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Graph pointer is NULL; cannot continue"));
	}
	else if (FAILED(hr = m_pVideoRenderer->AddToGraph(pGraph, m_hParent)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to add video mixing renderer to graph") << std::endl;
	}

	/*
	else if (FAILED(hr = m_pEvent->SetNotifyWindow(reinterpret_cast<OAHWND>(m_hParent), WM_GRAPHNOTIFY, 0)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to register notification window") << std::endl;
	} 
	else if (FAILED(hr = m_pPreviewWindow->put_Owner(reinterpret_cast<OAHWND>(m_hParent))))
	{
		wdebugstream() << TEXT("DEBUG: Failed to register preview video window owner: 0x") << std::hex << hr << std::endl;
	}
	else if (FAILED(hr = m_pPreviewWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to configure preview video window styles") << std::endl;
	}
	*/

	else
	{
		wdebugstream() << TEXT("DEBUG: Successfully set preview video window") << std::endl;
	}

	// Complete window initialisation.
	::UpdateWindow(m_hParent);

	return hr;
}

_Success_(S_OK == return) HRESULT CapturePreviewWindow::GetNativeVideoSize(_Outref_ uint64_t& width, _Outref_ uint64_t& height)
{
	long tmpWidth(0), tmpHeight(0);
	HRESULT hr;

	if (nullptr == m_pVideoRenderer)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: m_pVideoRenderer is NULL; cannot continue"));
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
		hr = E_FAIL;
	}
	else if (FAILED(hr = m_pVideoRenderer->GetNativeVideoSize(&tmpWidth, &tmpHeight, nullptr, nullptr)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed get dimensions of native video"));
	}
	else
	{
		// Success.
		width = tmpWidth;
		height = tmpHeight;
	}

	return hr;
}

_Success_(S_OK == return) HRESULT CapturePreviewWindow::HandleGraphEvent()
{
	HRESULT hr = E_UNEXPECTED;

	if (!m_pEvent)
	{
		return E_UNEXPECTED;
	}
	else
	{
		LONG_PTR param1 = 0, param2 = 0;
		long evCode = 0;

		// Get the events from the queue.
		while (SUCCEEDED(hr = m_pEvent->GetEvent(&evCode, &param1, &param2, 0)))
		{
			switch (evCode)
			{
			case EC_COMPLETE:
			case EC_USERABORT:
				m_CaptureEngine.Stop();
				break;

			case EC_ERRORABORT:
				wdebugstream() << TEXT("DEBUG: Error occured during playback") << std::endl;
				m_CaptureEngine.Stop();
				break;
			}

			// Free the event data.
			if (FAILED(hr = m_pEvent->FreeEventParams(evCode, param1, param2)))
			{
				wdebugstream() << TEXT("DEBUG: Failed to free event data following processing") << std::endl;
				break;
			}
		}
	}

	return hr;
}

_Success_(S_OK == return) HRESULT CapturePreviewWindow::SetVideoPosition(_In_ HWND hwndApp, _In_ RECT rcDest)
{
	HRESULT hr;

	if (nullptr == m_pVideoRenderer)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: m_pVideoRenderer is NULL; cannot continue"));
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
		hr = E_FAIL;
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

_Success_(S_OK == return) HRESULT CapturePreviewWindow::RepaintVideo(_In_ HWND hWnd, _In_ HDC hDC)
{
	HRESULT hr;

	if (nullptr == m_pVideoRenderer)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: m_pVideoRenderer is NULL; cannot continue"));
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
		hr = E_FAIL;
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

_Success_(S_OK == return) HRESULT CapturePreviewWindow::DisplayModeChanged()
{
	HRESULT hr;

	if (nullptr == m_pVideoRenderer)
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: m_pVideoRenderer is NULL; cannot continue"));
	}
	else if (!m_pVideoRenderer->HasVideo())
	{
		wdebugstream() << TEXT("DEBUG: m_pVideoRenderer is not ready for use; cannot continue") << std::endl;
		hr = E_FAIL;
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

}
// namespace WordJewelsImager
