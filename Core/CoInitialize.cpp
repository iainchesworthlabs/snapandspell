#include "pch.h"
#include "CoInitialize.h"
#include "DebugStream.h"

CCoInitialize::CCoInitialize() : m_hr(::CoInitializeEx(nullptr, COINIT_MULTITHREADED)) 
{ 
	wdebugstream() << TEXT("DEBUG: Initialising the COM library for use by the calling thread...");
	switch(m_hr)
	{
	case S_OK:
		wdebugstream() << TEXT("...initialized successfully on this thread.") << std::endl;
		break;

	case S_FALSE:
		wdebugstream() << TEXT("...is already initialized on this thread.") << std::endl;
		break;

	case RPC_E_CHANGED_MODE:
		wdebugstream() << TEXT("...WARNING: the concurrency model for this thread was multithread apartment (MTA).") << std::endl;
		break;

	case E_INVALIDARG:
	case E_OUTOFMEMORY:
	case E_UNEXPECTED:
	default:
		wdebugstream() << TEXT("...failed!.") << std::endl;
		break;
	}
}

CCoInitialize::~CCoInitialize() 
{ 
	if (SUCCEEDED(m_hr)) 
	{
		wdebugstream() << TEXT("DEBUG: Closing the COM library on the current thread") << std::endl;
		::CoUninitialize();
	}
}

CCoInitialize::operator HRESULT() const 
{ 
	return m_hr; 
}
