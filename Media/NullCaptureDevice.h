#pragma once
#ifndef _NULLCAPTUREDEVICE_H
#define _NULLCAPTUREDEVICE_H

#include "CaptureEngineCOM.h"
#include "ICaptureDevice.h"

namespace WordJewelsImager
{

class NullCaptureDevice : public ICaptureDevice
{
public:
	NullCaptureDevice();
	virtual ~NullCaptureDevice();

public:
	virtual CaptureEngineCOM::IBaseFilterPtr& operator()();

private:
	CaptureEngineCOM::IBaseFilterPtr m_pCapFilter;
};

}
// namespace WordJewelsImager

#endif // _NULLCAPTUREDEVICE_H
