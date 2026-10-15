#pragma once
#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <cstdint>
#include <string>

#include "CallStack.h"
#include "GetErrorString.h"

namespace Diagnostics 
{

namespace Constants
{
	extern const uint32_t TOTAL_STACK_FRAMES;
}

namespace Globals
{
	extern bool NoStackTrace;
}

std::wstring GetStackTrace(int32_t skipCount, int32_t totalCount, bool noThrow);

} 
// namespace Diagnostics

#endif // DIAGNOSTICS_H
