#include "pch.h"
#include "WinMain.h"
#define WINMAIN

#if defined (WINMAIN)

#include <stdexcept>
#include <string>
#include <sstream>

#include "CaptureEngine.h"
#include "DebugStream.h"
#include "GetErrorString.h"
#include "MainWindow_DialogProc.h"
#include "Resource.h"
#include "TracedThrow.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	int32_t returnCode(0);

	try
	{
		INITCOMMONCONTROLSEX picce;
		HWND hWndDlg;

		picce.dwSize = sizeof(INITCOMMONCONTROLSEX);
		picce.dwICC = ICC_STANDARD_CLASSES;

		if (FALSE == ::InitCommonControlsEx(&picce))
		{
			// Failed to initialise the common controls.
			Diagnostics::GetErrorString(::GetLastError());
		}
		else if (nullptr == (hWndDlg = ::CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINWINDOW), nullptr, MainWindow_DialogProc)))
		{
			// Failed to create the window.
			Diagnostics::GetErrorString(::GetLastError());
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
					Diagnostics::GetErrorString(::GetLastError());
					
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
		wdebugstream() << TEXT("WordJewels Solver: ") << exceptionMsg << std::endl;
		returnCode = -1;
	}

	catch (const std::string& exceptionMsg)
	{
		std::wstring wstmp(exceptionMsg.cbegin(), exceptionMsg.cend());
		wdebugstream() << TEXT("WordJewels Solver: ") << wstmp << std::endl;
		returnCode = -1;
	}

	catch (const std::exception& e)
	{
		DiagnosticsTracedThrow(e);
		returnCode = -1;
	}
	
	wdebugstream() << TEXT("DEBUG: Exitting application") << std::endl;
	return returnCode;
}

#endif // defined (WINMAIN)
