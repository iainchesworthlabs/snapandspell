#include "Stdafx.h"
#include "DebugStream.h"
#include "Diagnostics.h"
#include "StringConversion.h"
#include "WJSErrorHandler.h"

namespace Diagnostics
{

WJSErrorHandler::WJSErrorHandler(const std::wstring& msg) : 
	std::logic_error(wstring_to_string(msg)),
	m_Stack(),
	m_GotStack(false) 
{
}

WJSErrorHandler::WJSErrorHandler(const char* msg) : 
	std::logic_error(msg), 
	m_Stack(),
	m_GotStack(false) 
{
}

const std::wstring& WJSErrorHandler::Stack()
{
	if (!m_GotStack)
	{
		if (Diagnostics::Globals::NoStackTrace)
		{
			m_Stack = std::wstring(TEXT("Callstack not available"));
		}
		else
		{
			m_Stack = GetStackTrace(2, Diagnostics::Constants::TOTAL_STACK_FRAMES, true);
			m_GotStack = true;
		}
	}

	return m_Stack;
}

}
// namespace Diagnostics
