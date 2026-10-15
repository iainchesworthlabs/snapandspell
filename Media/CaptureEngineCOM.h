#pragma once
#ifndef _CAPTUREENGINE_COM_H
#define _CAPTUREENGINE_COM_H

#include <Windows.h>
#include <comdef.h>
#include <comip.h>
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>

namespace WordJewelsImager
{

namespace CaptureEngineCOM
{
	// A slightly more brief version of the com_ptr_t definition macro.
	#define COMPTR_TYPEDEF(InterfaceName) _COM_SMARTPTR_TYPEDEF(InterfaceName, IID_##InterfaceName)

	COMPTR_TYPEDEF(IBaseFilter);
	COMPTR_TYPEDEF(ICaptureGraphBuilder2);
	COMPTR_TYPEDEF(ICreateDevEnum);
	COMPTR_TYPEDEF(IEnumFilters);
	COMPTR_TYPEDEF(IEnumMediaTypes);
	COMPTR_TYPEDEF(IEnumMoniker);
	COMPTR_TYPEDEF(IEnumPins);
	COMPTR_TYPEDEF(IFilterGraph);
	COMPTR_TYPEDEF(IFileSinkFilter2);
	COMPTR_TYPEDEF(IGraphBuilder);
	COMPTR_TYPEDEF(IMediaControl);
	COMPTR_TYPEDEF(IMediaEventEx);
	COMPTR_TYPEDEF(IMediaSeeking);
	COMPTR_TYPEDEF(IMediaPosition);
	COMPTR_TYPEDEF(IMoniker);
	COMPTR_TYPEDEF(IPin);
	COMPTR_TYPEDEF(IPropertyBag);
	COMPTR_TYPEDEF(IVideoWindow);
	COMPTR_TYPEDEF(IBasicVideo2);
	COMPTR_TYPEDEF(IVMRWindowlessControl);
	COMPTR_TYPEDEF(IVMRWindowlessControl9);
	COMPTR_TYPEDEF(IStorage);
	COMPTR_TYPEDEF(IPersistStream);
	COMPTR_TYPEDEF(IStream);
}
// namespace CaptureEngineCOM

}
// namespace WordJewelsImager

#endif // _CAPTUREENGINE_COM_H
