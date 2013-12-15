#include "Stdafx.h"
#include "TracedThrow.h"
#include "Win32Error.h"
#include "Win32Result.h"

namespace Diagnostics
{

Win32Result::Win32Result() : 
	m_HR(S_OK), 
	m_Msg(nullptr) 
{
}

Win32Result::Win32Result(const HRESULT hr) : 
	m_Msg(nullptr)
{
	setHr(hr);
}

Win32Result::Win32Result(const bool b) : 
	m_Msg(nullptr)
{
	HRESULT hr = S_OK;

	if (!b)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
	}

	this->setHr(hr);
}

Win32Result::Win32Result(const Win32Result_NoThrow& r) : 
	m_Msg(nullptr)
{
	setHr(r.hr, true); // nothrow
}

Win32Result& Win32Result::operator=(const HRESULT hr)
{
	setHr(hr);
	return *this;
}

Win32Result& Win32Result::operator=(const bool b)
{
	HRESULT hr = S_OK;

	if (!b)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
	}

	setHr(hr);
	return *this;
}

Win32Result& Win32Result::operator=(const Win32Result_NoThrow& r)
{
	setHr(r.hr, true); // no throw
	return *this;
}

void Win32Result::setMessage(const char* msg)
{
	m_Msg = msg;
}

void Win32Result::setHr(HRESULT hr, const bool noThrow)
{
	m_HR = hr;

	if (FAILED(hr))
	{
		if (!noThrow)
		{
			TracedThrow(Win32Error(hr, m_Msg), __FILE__, __LINE__, TEXT("Win32Error"));
		}
		else
		{
			// if not throwing, at least trace the error
			// NOTE: I am not going to do this trace since in theory, if we are using noThrow,
			// that means the code knows what it's doing and the failed HRESULT is a valid one
			// BAJA_TRACE(boost::format("WARNING: win32_result: setting HRESULT to failed value: 0x%08x. noThrow=true") % hr);
		}
	}
}

}
// namespace Diagnostics
