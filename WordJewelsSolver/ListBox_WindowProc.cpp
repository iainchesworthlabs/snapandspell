#include "pch.h"

#include "ListBox_WindowProc.h"
#include "Resource.h"

static HFONT hListBoxFont(nullptr);

LRESULT CALLBACK ListBox_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_GETDLGCODE:
		if (nullptr != hListBoxFont)
		{
			// Do nothing as the font is already created.
		}
		else
		{
			static const int FONT_SIZE = 12;
			static const int DPI_SCREEN = 72;

			HDC hDC = ::GetDC(nullptr);
			LONG lfHeight(-::MulDiv(FONT_SIZE, ::GetDeviceCaps(hDC, LOGPIXELSY), DPI_SCREEN));
			::ReleaseDC(nullptr, hDC);
			
			hListBoxFont = CreateFont(
				lfHeight, 
				0, 
				0, 
				0, 
				FW_DONTCARE, 
				FALSE, 
				FALSE, 
				FALSE, 
				DEFAULT_CHARSET, 
				OUT_OUTLINE_PRECIS, 
				CLIP_DEFAULT_PRECIS, 
				CLEARTYPE_QUALITY, 
				VARIABLE_PITCH, 
				TEXT("Segoe UI"));

			if (nullptr == hListBoxFont)
			{
				// FIXME Failure...do something
			}
			else
			{
				// Do nothing
			}
		}

		// Assign the font to this control.
		::SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hListBoxFont), MAKELPARAM(TRUE, 0));
		
		// Capture all keyboard keys
		return DLGC_WANTALLKEYS;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
