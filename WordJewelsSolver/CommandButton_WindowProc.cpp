#include "pch.h"

#include "CommandButton_WindowProc.h"
#include "Resource.h"

static HFONT hCommandButtonFont(nullptr);

LRESULT CALLBACK CommandButton_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_CREATE:
		if (nullptr != hCommandButtonFont)
		{
			// Do nothing as the font is already created.
		}
		else
		{
			HDC hDC = ::GetDC(nullptr);
			
			static const int FONT_SIZE = 16;
			static const int DPI_SCREEN = 72;

			LONG lfHeight(-::MulDiv(FONT_SIZE, ::GetDeviceCaps(hDC, LOGPIXELSY), DPI_SCREEN));
			::ReleaseDC(nullptr, hDC);
			
			hCommandButtonFont = CreateFont(
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

			if (nullptr == hCommandButtonFont)
			{
				// FIXME Failure...do something
			}
			else
			{
				// Assign the font to this control.
				::SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hCommandButtonFont), MAKELPARAM(TRUE, 0));
			}
		}
		return TRUE;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
