#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#define NTDDI_VERSION			NTDDI_WIN7
#define _WIN32_WINNT			_WIN32_WINNT_WIN7
#define WINVER					_WIN32_WINNT_WIN7
#define _WIN32_IE				_WIN32_IE_IE80

// Windows Header Files
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>

// C Runtime Library
#include <tchar.h>
