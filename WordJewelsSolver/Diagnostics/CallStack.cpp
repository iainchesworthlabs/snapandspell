#include "Stdafx.h"
#include "CallStack.h"

#include <DbgHelp.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

#include "DebugStream.h"
#include "Diagnostics.h"
#include "TracedThrow.h"
#include "Win32Result.h"
#include "WithFinally.h"

// Additional library for the SymXXXX functions
#pragma comment(lib, "dbghelp.lib")

namespace Diagnostics
{

std::wstring Callstack::Trace(int32_t skipCount, int32_t totalCount, bool noThrow)
{
	// make sure we don't try to capture the stack on an error to prevent infinite recursion
	Diagnostics::Globals::NoStackTrace = true;

	std::wstring s;
	WithFinally::Run([&]() 
	{
		if (!(skipCount >= 0)) 
		{
			TracedThrow(std::invalid_argument("skipCount >= 0"), __FILE__, __LINE__, TEXT("std::invalidargument"));
		}

		try
		{
			Callstack::Init();
			s = Callstack::TraceInternal(skipCount, totalCount);
		}

		catch (const std::exception& e)
		{
			if (noThrow)
			{
				wdebugstream() << TEXT("DEBUG: Exception was thrown during stack capture, stack trace won't be available...");
				wdebugstream() << std::endl;
				wdebugstream() << TEXT("\t");
				wdebugstream() << e.what();
				wdebugstream() << std::endl;
			}
			else
			{
				throw;
			}
		}
	},
		[&]()
	{
		Callstack::Cleanup();
		Diagnostics::Globals::NoStackTrace = false;
	});

	return s;
}

void Callstack::Init()
{
	std::wstring searchPath = Callstack::GetSymbolSearchPath();

	BOOL b = ::SymInitializeW(GetCurrentProcess(), searchPath.c_str(), TRUE);
	if (!b)
	{
		// If failed to initialize, try uninitializing and then retry
		// This happens when running with some assert libraries which apparently don't uninitialize things properly
		if (HRESULT_FROM_WIN32(GetLastError()) == E_INVALIDARG)
		{
			::SymCleanup(GetCurrentProcess());
			b = ::SymInitializeW(GetCurrentProcess(), searchPath.c_str(), TRUE);
		}
	}

	Win32Result res = !!b;

	DWORD options = ::SymGetOptions();
	options |= SYMOPT_LOAD_LINES;
	options |= SYMOPT_FAIL_CRITICAL_ERRORS;
	options |= SYMOPT_CASE_INSENSITIVE;
	::SymSetOptions(options);
}

void Callstack::Cleanup()
{
	::SymCleanup(GetCurrentProcess());
}

std::wstring Callstack::TraceInternal(int32_t skipCount, int32_t totalCount)
{
	std::wostringstream sstm;

	HANDLE proc(::GetCurrentProcess());

	CONTEXT c;
	::memset(&c, 0, sizeof(CONTEXT));
	::RtlCaptureContext(&c);

	STACKFRAME64 s;
	::memset(&s, 0, sizeof(STACKFRAME64));

	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrStack.Offset = c.Esp;
	s.AddrStack.Mode = AddrModeFlat;

	std::vector<uint8_t> sym;
	sym.resize(sizeof(IMAGEHLP_SYMBOL64)+64); // TEST: make sure this still works, I used to do a new here
	IMAGEHLP_SYMBOL64* psym = reinterpret_cast<IMAGEHLP_SYMBOL64*>(sym.data());
	::memset(psym, 0, sizeof(IMAGEHLP_SYMBOL64)+64);
	psym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	psym->MaxNameLength = 64;

	IMAGEHLP_LINE64 line;
	::memset(&line, 0, sizeof(IMAGEHLP_LINE64));
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	IMAGEHLP_MODULE64 module;
	::memset(&module, 0, sizeof(IMAGEHLP_MODULE64));
	module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

	// HACK HACK HACK - skip first 6 frames since it'll be the call to callstack::trace
	skipCount += 7;

	int32_t frame = 0;

	for (frame = 0 ; /* DO NOTHING */ ; frame++)
	{
		if (!Callstack::Execute("StackWalk64", [&]()
		{
			return ::StackWalk64(IMAGE_FILE_MACHINE_I386, proc, ::GetCurrentThread(), &s, &c, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr);
		})) 
		break;

		if (s.AddrPC.Offset == s.AddrFrame.Offset)
		{
			break; // endless callstack ?
		}

		if (s.AddrPC.Offset != 0 && frame >= skipCount)
		{
			DWORD offset = 0;

			// Get line info
			if (!Callstack::Execute("SymGetLineFromAddr64", [&]() { return SymGetLineFromAddr64(proc, s.AddrPC.Offset, &offset, &line); }))
			{
				break;
			}

			// Get module info
			if (!Callstack::Execute("SymGetModuleInfo64", [&]() { return SymGetModuleInfo64(proc, s.AddrPC.Offset, &module); }))
			{
				break;
			}

			// Get symbol info
			DWORD64 offset2 = 0;
			if (!Callstack::Execute("SymGetSymFromAddr64", [&]() { return SymGetSymFromAddr64(proc, s.AddrPC.Offset, &offset2, psym); }))
			{
				break;
			}

			char* moduleName = ((0 != module.ModuleName[0]) ? module.ModuleName : "(no module)");
			
			// tabulation logic:
			// 2 chars: frame number
			// 32 chars (max): module name
			// 64 chars (max): symbol name, as defined above 
			// 10 chars (max): offset (for 32 bit) // NYI: X86
			// total: 108 max, so we'll make it 64 which should be closer to average

			sstm << std::endl;

			sstm << std::right << std::setw(2) << std::setfill(TEXT('0'));
			sstm << frame << TEXT(" ");

			sstm << std::left << std::setw(96) << std::setfill(TEXT(' '));
			sstm << [&moduleName, &psym, &s]() -> std::wstring
			{
				std::wostringstream sstmTmp;
				sstmTmp << moduleName;
				sstmTmp << TEXT("!");
				sstmTmp << psym->Name;
				sstmTmp << TEXT(" + 0x");
				sstmTmp << std::hex;
				sstmTmp << std::right;
				sstmTmp << std::setw(8);
				sstmTmp << std::setfill(TEXT('0'));
				sstmTmp << s.AddrPC.Offset;
				return sstmTmp.str();
			}();

			if (0 == line.FileName[0])
			{
				// Line info not available
				sstm << TEXT(" (source code not available)") << std::endl;
			}
			else
			{
				sstm << line.FileName << TEXT(", ");
				sstm << line.LineNumber;
			}
		}

		if ((0 == s.AddrReturn.Offset) || ((totalCount > 0) && (frame - skipCount >= totalCount - 1)))
		{
			break;
		}
	}

	return sstm.str();
}

bool Callstack::Execute(const char* function, std::function<BOOL(void)> f)
{
	BOOL b = f();
	if (!b)
	{
		DWORD err = ::GetLastError();
		if ((ERROR_INVALID_ADDRESS == err) || (ERROR_NOACCESS == err))
		{
			return false; // we are done
		}
		else
		{
			Win32Result res;
			res.setMessage(function);
			res = false;
		}
	}

	return true;
}

std::wstring Callstack::GetSymbolSearchPath()
{
	std::wstring s;
	std::vector<wchar_t> v;

	// Add current directory
	DWORD bufferSize = GetCurrentDirectory(0, nullptr);
	if (bufferSize <= 0) 
	{
		wdebugstream() << TEXT("DEBUG: GetCurrentDirectory failed to get size: ") << std::hex << ::GetLastError() << std::endl;
	}
	if (bufferSize > 0)
	{
		v.resize(bufferSize);
		DWORD i = GetCurrentDirectory(bufferSize, v.data());
		if (i <= 0)
		{
			wdebugstream() << TEXT("DEBUG: GetCurrentDirectory failed to get data: ") << std::hex << ::GetLastError() << std::endl;
		}
		if (i >= bufferSize)
		{
			wdebugstream() << TEXT("DEBUG: GetCurrentDirectory: buffer not large enough") << std::endl;
		}

		if (i > 0 && i < bufferSize)
		{
			s.append(v.data());
			s.append(TEXT(";"));
		}
	}

	static const wchar_t* vars[] = { TEXT("_NT_SYMBOL_PATH"), TEXT("_NT_ALTERNATE_SYMBOL_PATH") };

	// Add environment variables
	std::for_each(std::begin(vars), std::end(vars), [&v, &s](const wchar_t*& var)
	{
		DWORD bufferSize = ::GetEnvironmentVariable(var, NULL, 0);
		if (bufferSize <= 0 && GetLastError() != ERROR_ENVVAR_NOT_FOUND)
		{
			wdebugstream() << TEXT("DEBUG: GetEnvironmentVariable failed: ") << std::hex << ::GetLastError() << std::endl;
		}

		if (bufferSize > 0)
		{
			v.resize(bufferSize);
			DWORD i = ::GetEnvironmentVariable(var, v.data(), bufferSize);
			if (i == 0)
			{
				wdebugstream() << TEXT("DEBUG: GetEnvironmentVariable failed: ") << std::hex << ::GetLastError() << std::endl;
			}
			if (i >= bufferSize)
			{
				wdebugstream() << TEXT("DEBUG: GetEnvironmentVariable: buffer not large enough") << std::endl;
			}
			if (i > 0 && i < bufferSize)
			{
				s.append(v.data());
				s.append(TEXT(";"));
			}
		}
	});

	wdebugstream() << TEXT("DEBUG: Using symbol search path: ") << s << std::endl;

	return s;
}

}
// namespace Diagnostics
