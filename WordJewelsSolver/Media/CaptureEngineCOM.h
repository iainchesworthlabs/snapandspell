#pragma once
#ifndef _CAPTUREENGINE_COM_H
#define _CAPTUREENGINE_COM_H

#include <comdef.h>
#include <comip.h>
#include <dshow.h>

namespace WordJewelsImager
{

namespace CaptureEngineCOM
{
	// A slightly more brief version of the com_ptr_t definition macro.
	#define COMPTR_TYPEDEF(InterfaceName) _COM_SMARTPTR_TYPEDEF(InterfaceName, IID_##InterfaceName)

	COMPTR_TYPEDEF(IBaseFilter);
	COMPTR_TYPEDEF(ICaptureGraphBuilder2);
	COMPTR_TYPEDEF(ICreateDevEnum);
	COMPTR_TYPEDEF(IEnumMediaTypes);
	COMPTR_TYPEDEF(IEnumMoniker);
	COMPTR_TYPEDEF(IEnumPins);
	COMPTR_TYPEDEF(IFilterGraph);
	COMPTR_TYPEDEF(IFileSinkFilter2);
	COMPTR_TYPEDEF(IGraphBuilder);
	COMPTR_TYPEDEF(IMediaControl);
	COMPTR_TYPEDEF(IMediaEvent);
	COMPTR_TYPEDEF(IMediaSeeking);
	COMPTR_TYPEDEF(IMoniker);
	COMPTR_TYPEDEF(IPin);
	COMPTR_TYPEDEF(IPropertyBag);
}
// namespace CaptureEngineCOM

}
// namespace WordJewelsImager

#endif // _CAPTUREENGINE_COM_H
