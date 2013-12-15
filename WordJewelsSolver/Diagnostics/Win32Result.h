#pragma once
#ifndef WIN32RESULT_H
#define WIN32RESULT_H

#include <Windows.h>


namespace Diagnostics
{

struct Win32Result_NoThrow
{
	HRESULT hr;
	explicit Win32Result_NoThrow(HRESULT hr_) : hr(hr_) {}
};

class Win32Result
{
public:
	Win32Result();
	Win32Result(const HRESULT hr);
	Win32Result(const bool b);
	Win32Result(const Win32Result_NoThrow& r);

public:
	Win32Result& operator=(const HRESULT hr);
	Win32Result& operator=(const bool b);
	Win32Result& operator=(const Win32Result_NoThrow& r);

public:
	void setMessage(const char* msg);

private:
	void setHr(HRESULT hr, const bool noThrow = false);

private:
	HRESULT m_HR;
	const char* m_Msg;
};

}
// namespace Diagnostics

#endif // WIN32RESULT_H
