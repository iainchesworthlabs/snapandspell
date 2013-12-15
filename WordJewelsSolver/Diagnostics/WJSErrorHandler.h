#pragma once
#ifndef _WJSERRORHANDLER_H
#define _WJSERRORHANDLER_H

#include <Windows.h>
#include <stdexcept>
#include <string>

namespace Diagnostics
{

class WJSErrorHandler : public std::logic_error
{
public:
	WJSErrorHandler(const std::wstring& msg);
	WJSErrorHandler(const char* msg);

public:
	const std::wstring& Stack();

private:
	std::wstring m_Stack;
	bool m_GotStack;
};

}
// namespace Diagnostics

#endif // _WJSERRORHANDLER_H
