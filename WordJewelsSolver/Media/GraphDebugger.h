#pragma once
#ifndef _GRAPHDEBUGGER_H
#define _GRAPHDEBUGGER_H

#include <Windows.h>
#include <dshow.h>

namespace WordJewelsImager
{

class GraphDebugger
{
public:
	GraphDebugger();
	virtual ~GraphDebugger();

private:
	static void WINAPI DisplayRECT(LPCTSTR szLabel, const RECT& rc);
	static void WINAPI DisplayBITMAPINFO(const BITMAPINFOHEADER* pbmi);
	static void WINAPI DisplayType(LPCTSTR label, const AM_MEDIA_TYPE *pmtIn);
	static void WINAPI FreeMediaType();

public:
	static HRESULT WINAPI DumpGraph(IFilterGraph *pGraph, DWORD dwLevel);
	static HRESULT WINAPI AddGraphToROT(IUnknown* pUnkGraph, DWORD *pdwRegister);
	static void WINAPI RemoveGraphFromROT(DWORD dwRegister);
};

}
// namespace WordJewelsImager

#endif // _GRAPHDEBUGGER_H
