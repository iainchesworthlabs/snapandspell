#pragma once
#ifndef _GETERRORSTRING_H
#define _GETERRORSTRING_H

#include <Windows.h>

namespace Diagnostics
{
	
void GetErrorString(DWORD dwLastError);

}
// namespace Diagnostics

#endif // _GETERRORSTRING_H
