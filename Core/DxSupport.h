#pragma once
#ifndef _DX_SUPPORT_H
#define _DX_SUPPORT_H

#include <Windows.h>
#include "DirectX_COM.h"

class CDxSupport
{
public:
	CDxSupport();
	~CDxSupport();

public:
	bool SupportsHardwareOverlay() const;

private:
	DirectX_COM::IDirectDraw7Ptr m_pDirectDraw7;
	bool m_bHasHardwareOverlay;
	HMODULE m_hDirectDraw;
};

#endif // _DX_SUPPORT_H
