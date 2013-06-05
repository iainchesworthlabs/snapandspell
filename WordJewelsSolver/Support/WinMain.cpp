#include "Stdafx.h"
#include "WinMain.h"
#define WINMAIN

#if defined (WINMAIN)

#include <string>
#include <sstream>

#include "DebugStream.h"
#include "GetErrorString.h"
#include "MainWindow_DialogProc.h"
#include "Resource.h"

#include "CaptureEngine.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		INITCOMMONCONTROLSEX picce;
		HWND hWndDlg;

		picce.dwSize = sizeof(INITCOMMONCONTROLSEX);
		picce.dwICC = ICC_STANDARD_CLASSES;

		if (FALSE == ::InitCommonControlsEx(&picce))
		{
			// Failed to initialise the common controls.
			GetErrorString(::GetLastError());
		}
		else if (nullptr == (hWndDlg = ::CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINWINDOW), nullptr, MainWindow_DialogProc)))
		{
			// Failed to create the window.
			GetErrorString(::GetLastError());
		}
		else
		{
			BOOL bReturn;
			MSG Msg;

			::ShowWindow(hWndDlg, SW_SHOW);

			while (0 != (bReturn = ::GetMessage(&Msg, nullptr, 0, 0)))
			{
				if (-1 == bReturn)
				{
					// Handle the error and possibly exit.
					GetErrorString(::GetLastError());
				}
				else if (!::IsDialogMessage(hWndDlg, &Msg))
				{
					::TranslateMessage(&Msg);
					::DispatchMessage(&Msg);
				}
				else
				{
					// Do nothing.
				}
			}
		}
	}

	catch (const std::wstring& exceptionMsg)
	{
		wdebugstream() << "WordJewels Solver: " << exceptionMsg << std::endl;
	}

	catch (const std::string& exceptionMsg)
	{
		std::wstringstream wss;
		wss << exceptionMsg.c_str();

		wdebugstream() << "WordJewels Solver: " << wss << std::endl;
	}

	catch (...)
	{
		wdebugstream() << "WordJewels Solver: Unknown Exception" << std::endl;
	}

	return 0;
}

#endif // defined (WINMAIN)
