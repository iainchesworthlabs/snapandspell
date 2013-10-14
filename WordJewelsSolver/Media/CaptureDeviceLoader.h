#pragma once
#ifndef _CAPTUREDEVICE_LOADER_H
#define _CAPTUREDEVICE_LOADER_H

#include <dshow.h>
#include <cstdint>
#include <map>
#include <string>
#include "CaptureEngineCOM.h"

namespace WordJewelsImager
{

class CaptureDeviceLoader 
{
public:
	enum CaptureDeviceResults
	{
		InvalidArgument,
		NoDeviceFound,
		Success
	};

public:
	CaptureDeviceLoader();
	~CaptureDeviceLoader();

public:
	CaptureDeviceResults Initialise(CLSID source_type = CLSID_VideoInputDeviceCategory);
	uint32_t GetNumSources() const;
	std::wstring GetSourceName(uint32_t index);
	CaptureEngineCOM::IBaseFilterPtr GetSource(uint32_t index);
	CaptureEngineCOM::IBaseFilterPtr GetSource(const std::wstring name);

private:
	std::wstring GetStringProperty(const CaptureEngineCOM::IPropertyBagPtr& prop_bag, std::wstring prop_name);
	std::wstring GetMonikerFriendlyName(const CaptureEngineCOM::IMonikerPtr& moniker);
	CaptureDeviceResults FindAllSources();

private:
	CaptureEngineCOM::IEnumMonikerPtr m_SourceEnum;
	std::map<int, std::wstring> m_Sources;
	CLSID m_SourceType;
};

}
// namespace WordJewelsImager

#endif // _CAPTUREDEVICE_LOADER_H
