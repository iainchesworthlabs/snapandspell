#pragma once
#ifndef WIN32ERROR_H
#define WIN32ERROR_H

#include <Windows.h>
#include <string>

#include "WJSErrorHandler.h"

namespace Diagnostics
{

class Win32Error : public WJSErrorHandler
{
public:
	Win32Error(HRESULT hr, const std::wstring& msg);
	Win32Error(HRESULT hr, const char* msg);

public:
	HRESULT hr() const;
	const char* what() const throw();

private:
	HRESULT m_HR;
	std::wstring m_What;
};

}
// namespace Diagnostics

#endif // WIN32ERROR_H
