#pragma once
#ifndef _CAPTUREDEVICE_COLLECTION_H
#define _CAPTUREDEVICE_COLLECTION_H

#include <dshow.h>

#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "CaptureDevice.h"
#include "CaptureEngineCOM.h"

namespace WordJewelsImager
{

class CaptureDeviceCollection : public std::vector<CaptureDevice>
{
public:
	CaptureDeviceCollection(CLSID source_type = CLSID_VideoInputDeviceCategory);
	virtual ~CaptureDeviceCollection();

private:
	void EnumerateCaptureDevices();

private:
	CaptureEngineCOM::IEnumMonikerPtr m_SourceEnum;
	const CLSID m_SourceType;
};

}
// namespace WordJewelsImager

#endif // _CAPTUREDEVICE_COLLECTION_H
