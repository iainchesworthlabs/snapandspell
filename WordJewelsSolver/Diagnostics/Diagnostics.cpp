#include "Stdafx.h"
#include "Diagnostics.h"

namespace Diagnostics
{

namespace Constants
{
	const uint32_t TOTAL_STACK_FRAMES = 10;
}

namespace Globals
{
	bool NoStackTrace = false;
}

std::wstring GetStackTrace(int32_t skipCount, int32_t totalCount, bool noThrow)
{
	return Callstack::Trace(skipCount, totalCount, noThrow);
}

}
// namespace Diagnostics
