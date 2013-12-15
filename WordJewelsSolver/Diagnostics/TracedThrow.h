#pragma once
#ifndef TRACEDTHROW_H
#define TRACEDTHROW_H

#include <cstdint>
#include <functional>
#include <sstream>

#include "DebugStream.h"
#include "Diagnostics.h"

namespace Diagnostics
{

template <class BOGUS_TYPE> struct statics
{
	static std::function<void(const wchar_t *)> OnExceptionThrown;
};

std::function<void(const wchar_t *)> statics<int>::OnExceptionThrown;

inline void setOnExceptionThrown(std::function<void(const wchar_t *)> f)
{
	statics<int>::OnExceptionThrown = f;
}

template <class EXCEPTION_TYPE>
void TraceException(const EXCEPTION_TYPE& e, const char* file, uint32_t line, bool traceStack, const wchar_t* etype)
{
	std::wostringstream sstm;
	if (etype) 
	{
		sstm << TEXT("DEBUG: Details: ") << etype << std::endl;
	}

	sstm << TEXT("DEBUG: What(): ") << e.what();

	if (file)
	{
		sstm << std::endl << TEXT("DEBUG: Filename: ") << file;
	}

	if (0 <= line)
	{
		sstm << std::endl << TEXT("DEBUG: Line number: ") << line;
	}

	if (!Diagnostics::Globals::NoStackTrace && traceStack)
	{
		sstm << std::endl << TEXT("DEBUG: Stack Trace: ") << GetStackTrace(2 /* skip two frames */, Diagnostics::Constants::TOTAL_STACK_FRAMES, true /* no throw */);
	}

	wdebugstream() << sstm.str() << std::endl;

	if (statics<int>::OnExceptionThrown)
	{
		statics<int>::OnExceptionThrown(sstm.str().c_str());
	}
}

template <class EXCEPTION_TYPE>
void TraceException(const EXCEPTION_TYPE& e, const wchar_t* etype)
{
	TraceException(e, nullptr, -1, false, etype);
}

template <class EXCEPTION_TYPE>
void TracedThrow(const EXCEPTION_TYPE& e, const char* file, uint32_t line, const wchar_t* etype)
{
	// Causes a breakpoint in code, where the user will be prompted to run the debugger.
	__debugbreak();

	wdebugstream() << TEXT("--------------------------------------------------------------------------------") << std::endl;
	wdebugstream() << TEXT("DEBUG: EXCEPTION THROWN...") << std::endl;
	TraceException(e, file, line, true, etype);
	wdebugstream() << TEXT("--------------------------------------------------------------------------------") << std::endl;

	throw e;
}

#define DiagnosticsTracedThrow(a) Diagnostics::TracedThrow(a, __FILE__, __LINE__, TEXT("L##a"))

}
// namespace Diagnostics

#endif // TRACEDTHROW_H
