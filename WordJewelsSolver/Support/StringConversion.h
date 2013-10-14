#pragma once
#ifndef _STRING_CONVERSION_H
#define _STRING_CONVERSION_H

#include <string>

std::wstring string_to_wstring(const std::string& str);
std::string wstring_to_string(const std::wstring& wstr);

#endif // _STRING_CONVERSION_H
