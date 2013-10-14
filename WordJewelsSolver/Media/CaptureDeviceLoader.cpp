#include "Stdafx.h"
#include "CaptureDeviceLoader.h"
#include "DebugStream.h"

namespace WordJewelsImager
{

CaptureDeviceLoader::CaptureDeviceLoader() : 
	m_SourceEnum(),
	m_Sources(),
	m_SourceType(GUID_NULL)
{
}

CaptureDeviceLoader::~CaptureDeviceLoader()
{
}

CaptureDeviceLoader::CaptureDeviceResults CaptureDeviceLoader::Initialise(CLSID source_type)
{
	CaptureDeviceResults returnResult;

	if ((CLSID_AudioInputDeviceCategory != source_type) && (CLSID_VideoInputDeviceCategory != source_type))
	{
		wdebugstream() << TEXT("DEBUG: Invalid device category provided") << std::endl;
		returnResult = InvalidArgument;
	}
	else 
	{
		m_SourceType = source_type;
		returnResult = FindAllSources();
	}

	return returnResult;
}

uint32_t CaptureDeviceLoader::GetNumSources() const 
{ 
	return m_Sources.size(); 
}

std::wstring CaptureDeviceLoader::GetSourceName(uint32_t index) 
{ 
	return m_Sources[index]; 
}

CaptureEngineCOM::IBaseFilterPtr CaptureDeviceLoader::GetSource(uint32_t index)
{
	CaptureEngineCOM::IBaseFilterPtr returnResult(nullptr);

	if (static_cast<size_t>(index) >= m_Sources.size()) 
	{
		wdebugstream() << TEXT("DEBUG: ") << index << TEXT(" is not a valid source index") << std::endl;
	}
	else
	{
		returnResult = GetSource(GetSourceName(index));
	}

	return returnResult;
}

CaptureEngineCOM::IBaseFilterPtr CaptureDeviceLoader::GetSource(const std::wstring name)
{
	CaptureEngineCOM::IBaseFilterPtr returnResult(nullptr);
	CaptureEngineCOM::IMonikerPtr sourceMoniker;
	HRESULT hr(S_OK);

	if (name.empty()) 
	{
		wdebugstream() << TEXT("DEBUG: Empty source name") << std::endl;
	}
	else if (FAILED(hr = m_SourceEnum->Reset()))
	{
		wdebugstream() << TEXT("DEBUG: Cannot reset source enumerator; hr = ") << std::hex << hr << std::dec << ")" << std::endl;
	}
	else
	{
		do
		{
			if (FAILED(hr = m_SourceEnum->Next(1, &sourceMoniker, nullptr)))
			{
				wdebugstream() << TEXT("DEBUG: Device not found") << std::endl;
				break;
			}
			else if (S_FALSE == hr)
			{
				wdebugstream() << TEXT("DEBUG: Device not found") << std::endl;
				break;
			}
			else if (!sourceMoniker)
			{
				wdebugstream() << TEXT("DEBUG: Device not found") << std::endl;
				break;
			}
			else
			{
				std::wstring sourceName(GetMonikerFriendlyName(sourceMoniker));
				if (sourceName == name) 
				{
					break;
				}
			}
		}
		while (true);

		if (FAILED(hr = sourceMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, reinterpret_cast<void**>(&returnResult))))
		{
			wdebugstream() << TEXT("DEBUG: Cannot bind filter; hr = ") << std::hex << hr << std::dec << ")" << std::endl;
		}
		else
		{
			// Success
		}
	}

	return returnResult;
}

std::wstring CaptureDeviceLoader::GetStringProperty(const CaptureEngineCOM::IPropertyBagPtr& prop_bag, std::wstring prop_name) 
{
	std::wstring name;
	HRESULT hr(S_OK);
	VARIANT var;
	
	VariantInit(&var);

	if (SUCCEEDED(hr = prop_bag->Read(prop_name.c_str(), &var, nullptr))) 
	{
		name = V_BSTR(&var);
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Failed to read property bag; hr = ") << std::hex << hr << std::dec << ")" << std::endl;
	}

	VariantClear(&var);
	return name;
}

std::wstring CaptureDeviceLoader::GetMonikerFriendlyName(const CaptureEngineCOM::IMonikerPtr& moniker) 
{
	CaptureEngineCOM::IPropertyBagPtr props;
	std::wstring name;
	HRESULT hr(S_OK);

	if (!moniker) 
	{
		wdebugstream() << TEXT("DEBUG: Moniker is invalid; unexpected") << std::endl;
	}
	else
	{		
		if (S_OK == (hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, reinterpret_cast<void**>(&props))))
		{
			const wchar_t* const FRIENDLY_NAME(TEXT("FriendlyName"));
			name = GetStringProperty(props, FRIENDLY_NAME);
			if (!name.empty()) 
			{
				wdebugstream() << TEXT("DEBUG: Moniker friendly name property missing or empty") << std::endl;
			}
			else
			{
				// Success
			}
		}
	}

	return name;
}

CaptureDeviceLoader::CaptureDeviceResults CaptureDeviceLoader::FindAllSources()
{
	CaptureEngineCOM::ICreateDevEnumPtr systemEnumerator;
	CaptureDeviceResults returnResults;
	const DWORD NO_ENUM_FLAGS(0);
	uint32_t sourceIndex(0);
	HRESULT hr(S_OK);
	
	if (FAILED(hr = systemEnumerator.CreateInstance(CLSID_SystemDeviceEnum)))
	{
		wdebugstream() << TEXT("DEBUG: Source enumerator creation failed") << std::endl;
		returnResults = NoDeviceFound;
	}
	else if (FAILED(hr = systemEnumerator->CreateClassEnumerator(m_SourceType, &m_SourceEnum, NO_ENUM_FLAGS)))
	{
		wdebugstream() << TEXT("DEBUG: Moniker creation failed (no devices)") << std::endl;
		returnResults = NoDeviceFound;
	}
	else if (S_FALSE == hr)
	{
		wdebugstream() << TEXT("DEBUG: Moniker creation failed (device CLSID does not exist)") << std::endl;
		returnResults = NoDeviceFound;
	}
	else
	{
		do 
		{
			IMonikerPtr sourceMoniker;
		
			if (FAILED(hr = m_SourceEnum->Next(1, &sourceMoniker, nullptr)))
			{
				wdebugstream() << TEXT("DEBUG: Done enumerating sources, found ") << sourceIndex << TEXT(" sources") << std::endl;
				break;
			}
			else if (S_FALSE == hr)
			{
				wdebugstream() << TEXT("DEBUG: Done enumerating sources, found ") << sourceIndex << TEXT(" sources") << std::endl;
				break;
			}
			else if (!sourceMoniker)
			{
				wdebugstream() << TEXT("DEBUG: Done enumerating sources, found ") << sourceIndex << TEXT(" sources") << std::endl;
				break;
			}
			else
			{
				std::wstring name(GetMonikerFriendlyName(sourceMoniker));
				if (name.empty()) 
				{
					wdebugstream() << TEXT("Source = ") << sourceIndex << TEXT(" has no name....skipping") << std::endl;
					continue;
				}

				wdebugstream() << TEXT("Source = ") << sourceIndex << TEXT(", name = ") << name.c_str() << std::endl;
				m_Sources[sourceIndex] = name;
				++sourceIndex;
			}
		}
		while (true);

		if (0 == m_Sources.size()) 
		{
			wdebugstream() << TEXT("No devices found!") << std::endl;
			returnResults = NoDeviceFound;
		}

		returnResults = Success;
	}

	return returnResults;
}

}
// namespace WordJewelsImager
