#include "pch.h"

#include "LetterControl_WindowProc.h"
#include "Resource.h"

static HFONT hLetterControlFont(nullptr);

LRESULT CALLBACK LetterControl_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_GETDLGCODE:
		if (nullptr != hLetterControlFont)
		{
			// Do nothing as the font is already created.
		}
		else
		{
			static const int FONT_SIZE = 18;
			static const int DPI_SCREEN = 72;

			HDC hDC = ::GetDC(nullptr);
			LONG lfHeight(-::MulDiv(FONT_SIZE, ::GetDeviceCaps(hDC, LOGPIXELSY), DPI_SCREEN));
			::ReleaseDC(nullptr, hDC);
			
			hLetterControlFont = CreateFont(
				lfHeight, 
				0, 
				0, 
				0, 
				FW_DONTCARE, 
				TRUE, 
				FALSE, 
				FALSE, 
				DEFAULT_CHARSET, 
				OUT_OUTLINE_PRECIS, 
				CLIP_DEFAULT_PRECIS, 
				CLEARTYPE_QUALITY, 
				VARIABLE_PITCH, 
				TEXT("Segoe UI"));

			if (nullptr == hLetterControlFont)
			{
				// FIXME Failure...do something
			}
			else
			{
				// Do nothing
			}
		}

		// Assign the font to this control.
		::SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hLetterControlFont), MAKELPARAM(TRUE, 0));

		// Limit the number of letters that can be held.
		::SendMessage(hWnd, EM_SETLIMITTEXT, static_cast<WPARAM>(1), 0);
		
		// Capture only WM_CHAR messages
		return DLGC_WANTCHARS;

	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			{ int a = 1 + wParam;}
			break;

		case EN_UPDATE:
			{ int a = 2 + wParam;}
			break;

		case EN_MAXTEXT:
			// The user updated the text...check the length before 
			// accepting the change (assuming that can happen)...
			{ int a = 3 + wParam;}
			break;
		}
		break;

	case WM_CHAR:
		if (!::IsCharAlpha(static_cast<WCHAR>(wParam)))
		{
			// Not alphabetic character...prevent it.
			return 0;
		}
		else
		{
			// Alphabetic character...allow it.
		}
		break;

	case WM_SETFOCUS:
		// Select all the text within the edit control.
		::SendMessage(hWnd, EM_SETSEL, static_cast<WPARAM>(0), static_cast<LPARAM>(-1));
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
