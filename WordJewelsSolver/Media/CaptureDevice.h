#pragma once
#ifndef _CAPTUREDEVICE_H
#define _CAPTUREDEVICE_H

#include <ObjIdl.h>
#include <string>

namespace WordJewelsImager
{

class CaptureDevice
{
public:
	CaptureDevice(IMoniker *pMoniker, IPropertyBag *pPropBag);
	virtual ~CaptureDevice();

public:
	std::wstring GetDeviceFriendlyName();
	std::wstring GetDeviceDescription();

public:
	IMoniker* GetMoniker() const;

private:
	std::wstring ReadProperty(const BSTR sPropertyName);

private:
	static const BSTR DESCRIPTION;
	static const BSTR FRIENDLY_NAME;

private:
	IMoniker* m_pMoniker;
	IPropertyBag *m_pPropBag;
	VARIANT m_VarData;
};

}
// namespace WordJewelsImager

#endif // !_CAPTUREDEVICE_H
