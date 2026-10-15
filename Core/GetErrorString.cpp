#include "pch.h"
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
			const LPCTSTR lpErrorMessage = TEXT("Failed with error %#010x: %s");
			const int32_t errorMessageSize = 32;

			lpDisplayBuf = reinterpret_cast<LPVOID>(::LocalAlloc(LMEM_ZEROINIT, ((stringLength + errorMessageSize) * sizeof(TCHAR))));
			if (nullptr == lpDisplayBuf)
			{
				wdebugstream() << TEXT("DEBUG: Failed to allocate memory for error message buffer") << std::endl;
			}
			else if (FAILED(::StringCchPrintf(reinterpret_cast<LPTSTR>(lpDisplayBuf), ::LocalSize(lpDisplayBuf) / sizeof(TCHAR), lpErrorMessage, dwLastError, reinterpret_cast<LPTSTR>(lpMsgBuf))))
			{
				wdebugstream() << TEXT("DEBUG: Failed to create error message string during reporting diagnostics") << std::endl;
			}
			else
			{
				wdebugstream() << reinterpret_cast<LPTSTR>(lpDisplayBuf);
			}

			::LocalFree(lpDisplayBuf);
		}
	}

	::LocalFree(lpMsgBuf);
}

}
// namespace Diagnostics
