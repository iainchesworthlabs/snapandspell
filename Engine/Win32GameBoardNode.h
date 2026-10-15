#pragma once
#ifndef WIN32GAMEBOARDNODE_H
#define WIN32GAMEBOARDNODE_H

#include <Windows.h>
#include <string>
#include "GameBoardNode.h"

class Win32GameBoardNode
{
public:
	Win32GameBoardNode(HWND hWndDlg, int nControlId);
	Win32GameBoardNode(const Win32GameBoardNode& rhs);
	virtual ~Win32GameBoardNode();

public:
	Win32GameBoardNode& operator=(const Win32GameBoardNode& rhs);

public:
	WordJewelsEngine::GameBoardNode AsNode() const;
	std::wstring AsString() const;

private:
	HWND m_EditControl;
	HLOCAL m_OrgEditCtlMemory;
	HLOCAL m_NewEditCtlMemory;
	bool m_IsMultiLineControl;
};

#endif // WIN32GAMEBOARDNODE_H
