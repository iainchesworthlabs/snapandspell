#pragma once
#ifndef _CAPTUREDEVICE_H
#define _CAPTUREDEVICE_H

#include <ObjIdl.h>
#include <memory>
#include <string>

#include "CaptureEngineCOM.h"
#include "ICaptureDevice.h"

namespace WordJewelsImager
{

class CaptureDevice : public ICaptureDevice
{
public:
	CaptureDevice(IMoniker *pMoniker);
	CaptureDevice(const CaptureDevice& rhs);
	virtual ~CaptureDevice();

public:
	CaptureDevice& operator=(const CaptureDevice& rhs);

public:
	std::wstring GetDeviceFriendlyName();
	std::wstring GetDeviceDescription();

public:
	virtual CaptureEngineCOM::IBaseFilterPtr& operator()();

private:
	std::wstring ReadProperty(const BSTR sPropertyName);

private:
	static const BSTR DESCRIPTION;
	static const BSTR FRIENDLY_NAME;

private:
	CaptureEngineCOM::IMonikerPtr m_pMoniker;
	CaptureEngineCOM::IPropertyBagPtr m_pPropBag;
	CaptureEngineCOM::IBaseFilterPtr m_pCapFilter;

	VARIANT m_VarData;
};

}
// namespace WordJewelsImager

#endif // !_CAPTUREDEVICE_H
