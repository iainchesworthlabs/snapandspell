#pragma once
#ifndef _ICAPTUREDEVICE_H
#define _ICAPTUREDEVICE_H

#include "CaptureEngineCOM.h"

namespace WordJewelsImager
{

class ICaptureDevice
{
public:
	ICaptureDevice();
	virtual ~ICaptureDevice();

public:
	virtual CaptureEngineCOM::IBaseFilterPtr& operator()() = 0;
};

}
// namespace WordJewelsImager

#endif // _ICAPTUREDEVICE_H
