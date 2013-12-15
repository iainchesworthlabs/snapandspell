#include "Stdafx.h"
#include "CaptureDeviceCollection.h"
#include "DebugStream.h"

namespace WordJewelsImager
{

CaptureDeviceCollection::CaptureDeviceCollection(CLSID source_type) : 
	m_SourceEnum(nullptr),
	m_SourceType(source_type)
{
	if ((CLSID_AudioInputDeviceCategory != m_SourceType) && (CLSID_VideoInputDeviceCategory != m_SourceType))
	{
		wdebugstream() << TEXT("DEBUG: Invalid device category provided") << std::endl;
	}
	else
	{
		EnumerateCaptureDevices();
	}
}

CaptureDeviceCollection::~CaptureDeviceCollection()
{

}

void CaptureDeviceCollection::EnumerateCaptureDevices()
{
	CaptureEngineCOM::ICreateDevEnumPtr systemEnumerator;
	const DWORD NO_ENUM_FLAGS(0);
	uint32_t sourceIndex(0);
	HRESULT hr(S_OK);

	if (FAILED(hr = systemEnumerator.CreateInstance(CLSID_SystemDeviceEnum)))
	{
		wdebugstream() << TEXT("DEBUG: Source enumerator creation failed") << std::endl;
	}
	else if (FAILED(hr = systemEnumerator->CreateClassEnumerator(m_SourceType, &m_SourceEnum, NO_ENUM_FLAGS)))
	{
		wdebugstream() << TEXT("DEBUG: Moniker creation failed (no devices)") << std::endl;
	}
	else if (S_FALSE == hr)
	{
		wdebugstream() << TEXT("DEBUG: Moniker creation failed (device CLSID does not exist)") << std::endl;
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
				CaptureDevice deviceFound(sourceMoniker);
				if (deviceFound.GetDeviceFriendlyName().empty())
				{
					wdebugstream() << TEXT("Source = ") << sourceIndex << TEXT(" has no name....skipping") << std::endl;
					continue;
				}
				else
				{
					wdebugstream() << TEXT("Source = ") << sourceIndex << TEXT(", name = ") << deviceFound.GetDeviceFriendlyName() << std::endl;

					push_back(deviceFound);
					++sourceIndex;
				}			
			}
		} 
		while (true);

		if (0 == size())
		{
			wdebugstream() << TEXT("No devices found!") << std::endl;
		}
	}
}

}
// namespace WordJewelsImager
