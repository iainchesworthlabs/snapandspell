#include "Stdafx.h"
#include "NullCaptureDevice.h"

namespace WordJewelsImager
{
NullCaptureDevice::NullCaptureDevice() : 
	ICaptureDevice(),
	m_pCapFilter(nullptr)
{
}

NullCaptureDevice::~NullCaptureDevice()
{
}

CaptureEngineCOM::IBaseFilterPtr& NullCaptureDevice::operator()()
{
	return m_pCapFilter;
}

}
// namespace WordJewelsImager
