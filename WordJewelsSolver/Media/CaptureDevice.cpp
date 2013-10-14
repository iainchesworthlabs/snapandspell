#include "Stdafx.h"

#include <cassert>
#include "CaptureDevice.h"
#include "DebugStream.h"

namespace WordJewelsImager
{

const BSTR CaptureDevice::DESCRIPTION = TEXT("Description");
const BSTR CaptureDevice::FRIENDLY_NAME = TEXT("FriendlyName");

CaptureDevice::CaptureDevice(IMoniker *pMoniker, IPropertyBag *pPropBag) : m_pMoniker(pMoniker), m_pPropBag(pPropBag), m_VarData()
{
	assert(nullptr != pMoniker);
	assert(nullptr != pPropBag);

	::VariantInit(&m_VarData);

	wdebugstream() << TEXT("DEBUG: New CaptureDevice created") << std::endl;
	wdebugstream() << TEXT("DEBUG: CaptureDevice \"FriendlyName\": ") << GetDeviceFriendlyName() << std::endl;
	wdebugstream() << TEXT("DEBUG: CaptureDevice \"Description\": ") << GetDeviceDescription() << std::endl;
}

CaptureDevice::~CaptureDevice()
{
	if (nullptr != m_pPropBag)
	{
		m_pPropBag->Release();
		m_pPropBag = nullptr;
	}

	if (nullptr != m_pMoniker)
	{
		m_pMoniker->Release();
		m_pMoniker = nullptr;
	}

	::VariantClear(&m_VarData); 
}

std::wstring CaptureDevice::GetDeviceFriendlyName()
{
	return ReadProperty(FRIENDLY_NAME);
}

std::wstring CaptureDevice::GetDeviceDescription()
{
	return ReadProperty(DESCRIPTION);
}

IMoniker* CaptureDevice::GetMoniker() const
{
	return m_pMoniker;
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
