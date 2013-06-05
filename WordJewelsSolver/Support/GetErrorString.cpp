#include "Stdafx.h"
#include "DebugStream.h"
#include "GetErrorString.h"

void GetErrorString(DWORD dwLastError)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;

	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);
	lpDisplayBuf = reinterpret_cast<LPVOID>(::LocalAlloc(LMEM_ZEROINIT, (::lstrlen((LPCTSTR)lpMsgBuf) * sizeof(TCHAR)))); 
	wdebugstream() << lpDisplayBuf;

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}