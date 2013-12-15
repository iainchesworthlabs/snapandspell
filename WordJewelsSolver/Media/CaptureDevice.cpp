#include "Stdafx.h"

#include <cassert>
#include <stdexcept>
#include "CaptureDevice.h"
#include "DebugStream.h"
#include "SafeRelease.h"
#include "TracedThrow.h"

namespace WordJewelsImager
{

const BSTR CaptureDevice::DESCRIPTION = TEXT("Description");
const BSTR CaptureDevice::FRIENDLY_NAME = TEXT("FriendlyName");

CaptureDevice::CaptureDevice(IMoniker *pMoniker) :
	ICaptureDevice(),
	m_pMoniker(pMoniker),
	m_pPropBag(nullptr),
	m_pCapFilter(nullptr),
	m_VarData(),
	m_PBC(nullptr)
{
	assert(nullptr != pMoniker);
	HRESULT hr;

	if (FAILED(hr = ::CreateBindCtx(0, &m_PBC)))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to create a bind context"));
	}
	else if (FAILED(hr = pMoniker->BindToStorage(m_PBC, 0, IID_IPropertyBag, reinterpret_cast<void**>(&m_pPropBag))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to bind to moniker property bag"));
	}
	else if (FAILED(hr = pMoniker->BindToObject(m_PBC, 0, IID_IBaseFilter, reinterpret_cast<void**>(&m_pCapFilter))))
	{
		DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to bind to moniker and retrieve filter object"));
	}
	else
	{
		::VariantInit(&m_VarData);

		wdebugstream() << TEXT("DEBUG: New CaptureDevice created") << std::endl;
		wdebugstream() << TEXT("DEBUG: CaptureDevice \"FriendlyName\": ") << GetDeviceFriendlyName() << std::endl;
		wdebugstream() << TEXT("DEBUG: CaptureDevice \"Description\": ") << GetDeviceDescription() << std::endl;
	}	
}

CaptureDevice::CaptureDevice(const CaptureDevice& rhs)
{
	assert(nullptr != rhs.m_pMoniker);
	assert(nullptr != rhs.m_pPropBag);
	assert(nullptr != rhs.m_pCapFilter);

	m_pMoniker = rhs.m_pMoniker;
	m_pPropBag = rhs.m_pPropBag;
	m_pCapFilter = rhs.m_pCapFilter;
		
	::VariantInit(&m_VarData);
		
	wdebugstream() << TEXT("DEBUG: Copy made of existing CaptureDevice") << std::endl;
	wdebugstream() << TEXT("DEBUG: CaptureDevice \"FriendlyName\": ") << GetDeviceFriendlyName() << std::endl;
	wdebugstream() << TEXT("DEBUG: CaptureDevice \"Description\": ") << GetDeviceDescription() << std::endl;
}

CaptureDevice::~CaptureDevice()
{
	SafeRelease(&m_pCapFilter);
	SafeRelease(&m_pPropBag);
	SafeRelease(&m_pMoniker);

	::VariantClear(&m_VarData);
	SafeRelease(&m_PBC);
}

CaptureDevice& CaptureDevice::operator=(const CaptureDevice& rhs)
{
	if (this != &rhs)
	{
		ICaptureDevice::operator=(rhs);

		m_pMoniker = rhs.m_pMoniker;
		m_pPropBag = rhs.m_pPropBag;
		m_pCapFilter = rhs.m_pCapFilter;

		::VariantInit(&m_VarData);

		wdebugstream() << TEXT("DEBUG: Assignment of existing CaptureDevice") << std::endl;
		wdebugstream() << TEXT("DEBUG: CaptureDevice \"FriendlyName\": ") << GetDeviceFriendlyName() << std::endl;
		wdebugstream() << TEXT("DEBUG: CaptureDevice \"Description\": ") << GetDeviceDescription() << std::endl;
	}

	return *this;
}

std::wstring CaptureDevice::GetDeviceFriendlyName()
{
	return ReadProperty(FRIENDLY_NAME);
}

std::wstring CaptureDevice::GetDeviceDescription()
{
	return ReadProperty(DESCRIPTION);
}

CaptureEngineCOM::IBaseFilterPtr& CaptureDevice::operator()()
{
	return m_pCapFilter;
}

std::wstring CaptureDevice::ReadProperty(const BSTR sPropertyName)
{
	std::wstring sDeviceProperty(TEXT("UnknownProperty"));
	HRESULT hr(S_OK);

	// Get description or friendly name.
	if (FAILED(hr = m_pPropBag->Read(sPropertyName, &m_VarData, 0)))
	{
		wdebugstream() << TEXT("DEBUG: Failed to read \"" << sPropertyName << "\" field (0x") << std::hex << hr << TEXT(")") << std::endl;
		hr = m_pPropBag->Read(L"FriendlyName", &m_VarData, 0);
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Read device property text: ") << m_VarData.bstrVal << std::endl;
		sDeviceProperty = std::wstring(static_cast<const wchar_t*>(m_VarData.bstrVal));
	}

	::VariantClear(&m_VarData); 
	return sDeviceProperty;
}

}
// namespaceWordJewelsImager
