#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#define NTDDI_VERSION			NTDDI_WIN7
#define _WIN32_WINNT			_WIN32_WINNT_WIN7
#define WINVER					_WIN32_WINNT_WIN7
#define _WIN32_IE				_WIN32_IE_IE80

//#if !(defined) UNICODE
//#define UNICODE
//#define _UNICODE
//#endif

#define STRICT
#define STRICT_TYPED_ITEMIDS

// Windows Header Files
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>

// C Runtime Library
#include <tchar.h>
