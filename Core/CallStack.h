#pragma once
#ifndef CALLSTACK_H
#define CALLSTACK_H

#include <cstdint>
#include <functional>
#include <string>

namespace Diagnostics
{

class Callstack
{
public:
	static std::wstring Trace(int32_t skipCount = 0, int32_t totalCount = -1, bool noThrow = false);

private:
	static void Init();
	static void Cleanup();
	static std::wstring TraceInternal(int32_t skipCount, int32_t totalCount);
	static bool Execute(const char* function, std::function<BOOL(void)> f);
	static std::wstring GetSymbolSearchPath();
	
private:
	int32_t m_FrameCount;
	bool m_NoThrow;
	bool m_Init;
};

}
// namespace Diagnostics

#endif // CALLSTACK_H
