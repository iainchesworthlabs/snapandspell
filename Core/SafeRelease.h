#pragma once
#ifndef _SAFERELEASE_H
#define _SAFERELEASE_H

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

#endif // _SAFERELEASE_H
