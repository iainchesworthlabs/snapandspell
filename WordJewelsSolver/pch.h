#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#define NTDDI_VERSION			NTDDI_WIN7
#define _WIN32_WINNT			_WIN32_WINNT_WIN7
#define WINVER					_WIN32_WINNT_WIN7
#define _WIN32_IE				_WIN32_IE_IE80

#define STRICT
#define STRICT_TYPED_ITEMIDS

#define NOMINMAX

// Windows Header Files
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>

// C Runtime Library
#include <tchar.h>

#if defined (_MSC_VER)

// Static Analysis Library
#include <sal.h>

#else

#define _In_
#define _Out_
#define _Inout_
#define _In_z_
#define _Inout_z_
#define _In_reads_(s)
#define _In_reads_bytes_(s) 
#define _In_reads_z_(s) 
#define _In_reads_or_z_(s) 
#define _Out_writes_(s)
#define _Out_writes_bytes_(s)
#define _Out_writes_z_(s)
#define _Inout_updates_(s)
#define _Inout_updates_bytes_(s)
#define _Inout_updates_z_(s)
#define _Out_writes_to_(s,c)
#define _Out_writes_bytes_to_(s,c)
#define _Out_writes_all_(s)
#define _Out_writes_bytes_all_(s)
#define _Inout_updates_to_(s,c)
#define _Inout_updates_bytes_to_(s,c)
#define _Inout_updates_all_(s)
#define _Inout_updates_bytes_all_(s)
#define _In_reads_to_ptr_(p)
#define _In_reads_to_ptr_z_(p)
#define _Out_writes_to_ptr_(p) 
#define _Out_writes_to_ptr_z_(p) 


#define _Outptr_
#define _Outptr_opt_ 
#define _Outptr_result_maybenull_
#define _Outptr_opt_result_maybenull_


#define _Outref_
#define _Ret_
#define _Field_

#endif // defined (MSVC)
