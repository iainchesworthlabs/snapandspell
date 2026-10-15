#pragma once
#ifndef _DIRECTX_COM_H
#define _DIRECTX_COM_H

#include <comdef.h>
#include <comip.h>
#include <ddraw.h>

namespace DirectX_COM
{

	// A slightly more brief version of the com_ptr_t definition macro.
	#define COMPTR_TYPEDEF(InterfaceName) _COM_SMARTPTR_TYPEDEF(InterfaceName, IID_##InterfaceName)

	COMPTR_TYPEDEF(IDirectDraw7);

}
// namespace DirectX_COM

#endif // _DIRECTX_COM_H
