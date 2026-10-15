#include "pch.h"
#include <array>
#include <future>
#include <thread>

#include "CommandButton_WindowProc.h"
#include "DebugStream.h"
#include "LetterControl_WindowProc.h"
#include "ListBox_WindowProc.h"
#include "MainWindow_DialogProc.h"
#include "Resource.h"
#include "VideoFrame_WindowProc.h"
#include "Win32GameBoardNode.h"
#include "WordJewelsEngine.h"

// The actual engine of the solver for Word Jewels
WordJewelsEngine::Engine g_WordJewelsSolver;

BOOL MainDialog_InitDialog(HWND hWndDlg, HANDLE, LPARAM)
{
	static const uint32_t StatusBarIdentifier = 10000;
	INT_PTR ReturnValueIsSet(0);
	LONG ReturnValue(0);

	try
	{
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC00), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC01), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC02), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC03), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC04), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC10), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC11), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC12), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC13), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC14), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC20), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC21), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC22), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC23), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC24), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC30), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC31), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC32), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC33), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC34), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC40), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC41), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC42), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC43), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_EDIT_LOC44), LetterControl_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_LIST_WORDS), ListBox_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_BUTTON_ABOUT), CommandButton_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_BUTTON_EXIT), CommandButton_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_BUTTON_SOLVE), CommandButton_WindowProc, 0, 0);
		::SetWindowSubclass(::GetDlgItem(hWndDlg, IDC_VIDEO_CAPTUREFRAME), VideoFrame_WindowProc, 0, 0);

		// COUGH SPLUTTER ACK - trigger the preview window.
		VideoFrame_OnCreate(::GetDlgItem(hWndDlg, IDC_VIDEO_CAPTUREFRAME), nullptr);

		// Create the status bar in which the various statuses can be displayed.
		::CreateWindowEx(0, STATUSCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWndDlg, reinterpret_cast<HMENU>(StatusBarIdentifier), ::GetModuleHandle(nullptr), nullptr);
		int32_t StatusSegmentWidths[] = {150, -1};
		::SendDlgItemMessage(hWndDlg, StatusBarIdentifier, SB_SETPARTS, static_cast<WPARAM>(sizeof(StatusSegmentWidths) / sizeof(uint32_t)), reinterpret_cast<LPARAM>(StatusSegmentWidths));
		::SendDlgItemMessage(hWndDlg, StatusBarIdentifier, SB_SETTEXT, MAKEWPARAM(0, 0), reinterpret_cast<LPARAM>(TEXT("TBC")));

		// Direct the system to set the keyboard focus to the specified control 
		ReturnValueIsSet = TRUE;
	}
	catch (...)
	{
		DWORD dwExitCode(0);

		::GetExitCodeThread(::GetCurrentThread(), &dwExitCode);
		::PostQuitMessage(dwExitCode);
	}

	return ReturnValueIsSet;
}

void MainDialog_OnClose(HWND hWndDlg)
{
	::DestroyWindow(hWndDlg);
	hWndDlg = nullptr;
}

void MainDialog_OnDestroy(HWND hWndDlg)
{
	::PostQuitMessage(0);
}

void MainDialog_OnPaint(HWND hWndDlg)
{
	PAINTSTRUCT ps;
	HDC hDC;

	hDC = ::BeginPaint(hWndDlg, &ps);
	::EndPaint(hWndDlg, &ps);
}

INT_PTR CALLBACK MainWindow_DialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ReturnValueIsSet(0);
	LONG ReturnValue(0);

	switch (uMsg)
	{
	HANDLE_MSG(hWndDlg, WM_INITDIALOG, MainDialog_InitDialog);
	HANDLE_MSG(hWndDlg, WM_CLOSE, MainDialog_OnClose);
	HANDLE_MSG(hWndDlg, WM_DESTROY, MainDialog_OnDestroy);
	HANDLE_MSG(hWndDlg, WM_PAINT, MainDialog_OnPaint);

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BUTTON_ABOUT:
			break;

		case IDC_BUTTON_EXIT:
			// Disable the solve button before playing with the worker thread.
			::EnableWindow(::GetDlgItem(hWndDlg, IDC_BUTTON_SOLVE), FALSE);

			// Destroy the dialog box.
			::DestroyWindow(hWndDlg);
			hWndDlg = nullptr;
			ReturnValueIsSet = TRUE;
			break;

		case IDC_BUTTON_SOLVE:
			// Not joinable means the thread is not active; let's see
			{
				::EnableWindow(::GetDlgItem(hWndDlg, IDC_BUTTON_SOLVE), FALSE);
				ListBox_ResetContent(::GetDlgItem(hWndDlg, IDC_LIST_WORDS));

				// Create the asynchronous worker task that will go and 
				// try to solve for words.
				//
				// NOTE: hWndDlg needs to be by value to ensure that the 
				//       window handle does not become NULL when it goes
				//       out of scope at the end of the WindowProc.

				std::packaged_task<int64_t()> task([hWndDlg]() -> int64_t
				{
					int64_t discoveredWords(0);

					try
					{
						(g_WordJewelsSolver.m_Gameboard).load(0, 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC00).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC01).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC02).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC03).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC04).AsNode());
						g_WordJewelsSolver.m_Gameboard.load(1, 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC10).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC11).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC12).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC13).AsNode(),  
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC14).AsNode());
						g_WordJewelsSolver.m_Gameboard.load(2, 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC20).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC21).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC22).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC23).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC24).AsNode());
						g_WordJewelsSolver.m_Gameboard.load(3, 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC30).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC31).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC32).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC33).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC34).AsNode());
						g_WordJewelsSolver.m_Gameboard.load(4, 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC40).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC41).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC42).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC43).AsNode(), 
							Win32GameBoardNode(hWndDlg, IDC_EDIT_LOC44).AsNode());

						g_WordJewelsSolver.m_Searcher.FindWords(
							g_WordJewelsSolver.m_Gameboard, 
							g_WordJewelsSolver.m_Dictionary, 
							g_WordJewelsSolver.m_DiscoveredWords);

						// rules.AddRule(Rule_Cloner(Rule_IsUniqueWord()), Rule_IsUniqueWord::RULE_DEFAULT_PRIORITY);
						// rules.AddRule(Rule_Cloner(Rule_ThreeLettersOrMore()), Rule_ThreeLettersOrMore::RULE_DEFAULT_PRIORITY);
						// rules.Execute(discoveredwords);

						std::sort(g_WordJewelsSolver.m_DiscoveredWords.begin(), g_WordJewelsSolver.m_DiscoveredWords.end(), g_WordJewelsSolver.MetaWordSorter);

						// std::unique(discoveredwords.begin(), discoveredwords.end());
						// std::remove_if(discoveredwords.begin(), discoveredwords.end(), [](const WordsList::value_type& metaword) -> bool { return (2 >= std::get<0>(metaword).length()); });

						///FIXME need to pass a words object into the searcher and the rules engine....
						
						wdebugstream() << L"DEBUG: Found " << g_WordJewelsSolver.m_DiscoveredWords.count() << L" words in the provided board." << std::endl;
						
						WordJewelsEngine::WordsList::iterator it(g_WordJewelsSolver.m_DiscoveredWords.begin());
						WordJewelsEngine::WordsList::iterator itend(g_WordJewelsSolver.m_DiscoveredWords.end());
						
						std::for_each(it, itend, [&hWndDlg](WordJewelsEngine::WordsList::value_type metaWord)
						{
							const std::wstring& textToDisplay(std::get<0>(metaWord));
							wdebugstream() << TEXT(" - ") << textToDisplay << std::endl;

							if (LB_ERR == ListBox_InsertString(::GetDlgItem(hWndDlg, IDC_LIST_WORDS), -1, textToDisplay.c_str()))
							{
								wdebugstream() << TEXT("DEBUG: Failed to insert word into list view.  GetLastError: ") << std::hex << ::GetLastError() << std::endl; 
							}
						});

						// Load the list box with the discovered words!
						///FIXME

						discoveredWords = g_WordJewelsSolver.m_DiscoveredWords.count();
					}
					catch (...)
					{
						wdebugstream() << TEXT("DEBUG: Failure during gameboard processing") << std::endl;
						discoveredWords = 0;
					}

					// Re-enable the window.
					::EnableWindow(::GetDlgItem(hWndDlg, IDC_BUTTON_SOLVE), TRUE);
					return discoveredWords;
				}
				);

				std::future<int64_t> returnValue(task.get_future());
				std::thread(std::move(task)).detach();
			}
			break;

		case IDC_EDIT_LOC00:
		case IDC_EDIT_LOC01:
		case IDC_EDIT_LOC02:
		case IDC_EDIT_LOC03:
		case IDC_EDIT_LOC04:
		case IDC_EDIT_LOC10:
		case IDC_EDIT_LOC11:
		case IDC_EDIT_LOC12:
		case IDC_EDIT_LOC13:
		case IDC_EDIT_LOC14:
		case IDC_EDIT_LOC20:
		case IDC_EDIT_LOC21:
		case IDC_EDIT_LOC22:
		case IDC_EDIT_LOC23:
		case IDC_EDIT_LOC24:
		case IDC_EDIT_LOC30:
		case IDC_EDIT_LOC31:
		case IDC_EDIT_LOC32:
		case IDC_EDIT_LOC33:
		case IDC_EDIT_LOC34:
		case IDC_EDIT_LOC40:
		case IDC_EDIT_LOC41:
		case IDC_EDIT_LOC42:
		case IDC_EDIT_LOC43:
		case IDC_EDIT_LOC44:
			switch(HIWORD(wParam))
			{
			case EN_CHANGE:
			case EN_UPDATE:
			case EN_MAXTEXT:
				// Reflect any notification messages back into the subclassed window.
				::SendMessage(::GetDlgItem(hWndDlg, LOWORD(wParam)), uMsg, wParam, lParam);
				break;

			default:
				// Do nothing with the message.
				break;
			}
		}
		break;
	}

	// Set the desired return value (as required), note bReturnValue must be TRUE.
	::SetWindowLong(hWndDlg, DWL_MSGRESULT, ReturnValue);
	return ReturnValueIsSet;
}
