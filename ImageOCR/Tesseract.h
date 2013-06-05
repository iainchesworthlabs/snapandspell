#pragma once

#ifndef _TESSERACT_H
#define _TESSERACT_H

#include <Windows.h>

// NB: Tesseract needs to have it's include areas mapped.
#pragma warning(push)
#pragma warning(disable : 4005) // warning : 'MAX_PATH' : macro redefinition
#include <baseapi.h>
#pragma warning(pop)

#ifndef INCLUDE_LIBRARY
#define INCLUDE_LIBRARY
#if defined(_DEBUG)
#pragma comment(lib, "libtesseract-debug.lib")
#elif defined(NDEBUG)
#pragma comment(lib, "libtesseract.lib")
#else
#error Cannot determine which Tesseract library to use...define DEBUG or NDEBUG!
#endif
#endif // INCLUDE_LIBRARY

namespace ImageOCR
{
	static const char* TESSERACT_LANGUAGE("eng");
};
// namespace ImageOCR


#endif // _TESSERACT_H
