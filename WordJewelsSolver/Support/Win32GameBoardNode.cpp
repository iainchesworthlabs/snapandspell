#include "Stdafx.h"
#include <CommCtrl.h>
#include <cstdint>

#include "DebugStream.h"
#include "Win32GameBoardNode.h"

Win32GameBoardNode::Win32GameBoardNode(HWND hWndDlg, int nControlId) :
	m_EditControl(nullptr),
	m_OrgEditCtlMemory(nullptr),
	m_NewEditCtlMemory(nullptr),
	m_IsMultiLineControl(false)
{
	if (nullptr == hWndDlg)
	{
		wdebugstream() << TEXT("DEBUG: Invalid window handle provided to Win32GameBoardNode.  GetLastError: ") << std::hex << ::GetLastError() << std::endl;
		throw std::runtime_error("");
	}
	else if (nullptr == (m_EditControl = ::GetDlgItem(hWndDlg, nControlId)))
	{
		wdebugstream() << TEXT("DEBUG: Invalid control id provided to Win32GameBoardNode.  GetLastError: ") << std::hex << ::GetLastError() << std::endl;
		throw std::runtime_error("");
	}
	else if (ES_MULTILINE & ::GetWindowLongPtr(m_EditControl, GWL_STYLE))
	{
		const uint32_t cbCh = sizeof(TCHAR) > 1 ? sizeof(TCHAR) : /*::IsUsingComCtlV6()*/ TRUE ? sizeof(WCHAR) : sizeof(char);
		static const uint32_t cchTextLength = 1;

		m_OrgEditCtlMemory = reinterpret_cast<HANDLE>(::SendMessage(m_EditControl, EM_GETHANDLE, 0, 0));
		m_NewEditCtlMemory = ::LocalReAlloc(m_OrgEditCtlMemory, cbCh * cchTextLength, LMEM_MOVEABLE);
		if (!m_NewEditCtlMemory)
		{
			wdebugstream() << TEXT("DEBUG: Failed to create new buffer for edit control text.  GetLastError: ") << std::hex << ::GetLastError() << std::endl;
			throw std::runtime_error("");
		}
		else
		{
			// LocalLock, assign string, LocalUnlock
			::SendMessage(m_EditControl, EM_SETHANDLE, reinterpret_cast<WPARAM>(m_NewEditCtlMemory), 0);
		}

		m_IsMultiLineControl = true;
	}
	else
	{
		// This is a single line edit control...need to do things differently
		// as EM_GET/SETHANDLE don't apply.
	}
}

Win32GameBoardNode::Win32GameBoardNode(const Win32GameBoardNode& rhs):
	m_EditControl(rhs.m_EditControl),
	m_OrgEditCtlMemory(rhs.m_OrgEditCtlMemory),
	m_NewEditCtlMemory(rhs.m_NewEditCtlMemory),
	m_IsMultiLineControl(rhs.m_IsMultiLineControl)
{
}

Win32GameBoardNode::~Win32GameBoardNode()
{
}

Win32GameBoardNode& Win32GameBoardNode::operator=(const Win32GameBoardNode& rhs)
{
	if (&rhs == this)
	{
		// Prevent self-assignment.
	}
	else
	{
		// Do any assignment
	}

	return *this;
}

WordJewelsEngine::GameBoardNode Win32GameBoardNode::AsNode() const
{
	return WordJewelsEngine::GameBoardNode(AsString());
}

std::wstring Win32GameBoardNode::AsString() const
{
	std::wstring returnValue(L""); // Defaults to empty

	if (m_IsMultiLineControl)
	{
		// Multiple Line Control
		throw std::runtime_error("DEBUG: Win32GameBoardNode::AsString() - Not Implemented (Multi-Line Support)");
	}
	else
	{
		static const uint32_t EXPECTED_LENGTH = 1;
		static const uint32_t LENGTH_TO_COPY = 2;
		wchar_t tmpLetters[LENGTH_TO_COPY];

		::ZeroMemory(tmpLetters, LENGTH_TO_COPY);

		if (EXPECTED_LENGTH > ::SendMessage(m_EditControl, WM_GETTEXTLENGTH, 0, 0))
		{
			wdebugstream() << TEXT("DEBUG: Invalid text length in control.  GetLastError: ") << std::hex << ::GetLastError() << std::endl;
		}
		else if (EXPECTED_LENGTH != ::SendMessage(m_EditControl, WM_GETTEXT, static_cast<WPARAM>(LENGTH_TO_COPY), reinterpret_cast<LPARAM>(tmpLetters)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to capture control's window text.  GetLastError: ") << std::hex << ::GetLastError() << std::endl;
		}
		else
		{
			// Update the string with the actual letter to be captured.
			returnValue = std::wstring(tmpLetters, EXPECTED_LENGTH);
		}
	}

	return returnValue;
}