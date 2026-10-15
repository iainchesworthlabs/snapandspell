#pragma once
#ifndef _DEBUGSTREAM_H
#define _DEBUGSTREAM_H

#include <Windows.h>
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <string>

template <class CharT, class TraitsT = std::char_traits<CharT>>
class basic_debugbuf : public std::basic_stringbuf<CharT, TraitsT>
{
public:

    virtual ~basic_debugbuf() noexcept
    {
        sync();
    }

protected:

    int sync()
    {
        output_debug_string(this->str().c_str());
        this->str(std::basic_string<CharT>());    // Clear the string buffer

        return 0;
    }

    void output_debug_string(const CharT *text) 
	{
	}
};

template<>
void basic_debugbuf<char>::output_debug_string(const char *text)
{
    ::OutputDebugStringA(text);
}

template<>
void basic_debugbuf<wchar_t>::output_debug_string(const wchar_t *text)
{
    ::OutputDebugStringW(text);
}

template<class CharT, class TraitsT = std::char_traits<CharT> >
class basic_dostream : public std::basic_ostream<CharT, TraitsT>
{
public:

    basic_dostream() : std::basic_ostream<CharT, TraitsT>(new basic_debugbuf<CharT, TraitsT>()) 
	{
	}

    virtual ~basic_dostream() noexcept 
    {
        delete this->rdbuf();
    }
};

typedef basic_dostream<char>    debugstream;
typedef basic_dostream<wchar_t> wdebugstream;

#endif // _DEBUGSTREAM_H
