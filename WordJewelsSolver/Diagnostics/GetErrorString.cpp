#include "Stdafx.h"
#include "DebugStream.h"
#include "GetErrorString.h"

namespace Diagnostics
{

void GetErrorString(DWORD dwLastError)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;

	if (0 == ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr))
	{
		wdebugstream() << TEXT("DEBUG: Failed to retrieve system error message") << std::endl;
	}
	else
	{
		const int stringLength(::lstrlen(static_cast<LPCTSTR>(lpMsgBuf)));

		if (0 >= stringLength)
		{
			wdebugstream() << TEXT("DEBUG: Zero-length string provided by system for error message") << std::endl;
		}
		else
		{
			lpDisplayBuf = reinterpret_cast<LPVOID>(::LocalAlloc(LMEM_ZEROINIT, (stringLength * sizeof(TCHAR))));
			if (nullptr == lpDisplayBuf)
			{
				wdebugstream() << TEXT("DEBUG: Failed to allocate memory for error message buffer") << std::endl;
			}
			else
			{
				wdebugstream() << lpDisplayBuf;
				::LocalFree(lpDisplayBuf);
			}
		}

		::LocalFree(lpMsgBuf);
	}
}

}
// namespace Diagnostics
