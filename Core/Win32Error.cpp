#include "pch.h"
#include "StringConversion.h"
#include "Win32Error.h"

#include <codecvt>
#include <iomanip>
#include <sstream>

namespace Diagnostics
{

Win32Error::Win32Error(HRESULT hr, const std::wstring& msg) : 
	WJSErrorHandler(msg),
	m_HR(hr),
	m_What()
{
	std::wostringstream theString;

	theString << TEXT("DEBUG: Win32 Error - ");
	theString << TEXT("HR: 0x") << std::hex << std::setw(8) << std::setfill(TEXT('0')) << hr;
	theString << TEXT(" (Last Error: 0x") << std::hex << std::setw(8) << std::setfill(TEXT('0')) << ::GetLastError() << TEXT(")");
	if (!msg.empty()) theString << TEXT(" ") << msg;
	theString << std::endl;

	m_What = theString.str();
}

Win32Error::Win32Error(HRESULT hr, const char* msg) : 
	// Delegated constructor!!
	Win32Error(hr, 
		// Conditional variable contents!!
		((nullptr == msg) ? 
			[& msg]() -> std::wstring 
			{ 
				// Yay! Let's go and convert the narrow char to a wide char!!....
				// Really...should this c'tor take a wchar_t?
				std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t> converter;
				std::wstring converted(converter.from_bytes(msg));
				return converted;
			}() 
		: 
		std::wstring(TEXT(""))))
{
}

HRESULT Win32Error::hr() const 
{ 
	return m_HR; 
}

const char* Win32Error::what() const noexcept
{
	return wstring_to_string(m_What).c_str();
}

}
// namespace Diagnostics
