#include "Stdafx.h"
#include "StringConversion.h"

#include <memory>
#include <sstream>
#include <vector>
#include "DebugStream.h"

std::wstring string_to_wstring(const std::string& str)
{
	std::wostringstream wstr;
	wstr << str.c_str();
	return wstr.str();
}

std::string wstring_to_string(const std::wstring& wstr)
{
	// Conversion buffer for |wcstombs| calls.
	const size_t buf_size = wstr.length() + 1;
	std::unique_ptr<char[]> temp_str(new (std::nothrow) char[buf_size]);

	if (!temp_str)
	{
		wdebugstream() << TEXT("DEBUG: Can't convert wstring of length = ") << wstr.length() << std::endl;
		return std::string("<empty>");
	}

	::memset(temp_str.get(), 0, buf_size);
	size_t num_converted = 0;

	::wcstombs_s(&num_converted, temp_str.get(), buf_size, wstr.c_str(), wstr.length()*sizeof(wchar_t));
	std::string str = temp_str.get();

	return str;
}
