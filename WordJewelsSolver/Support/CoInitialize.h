#pragma once
#ifndef _COINITIALIZE_H
#define _COINITIALIZE_H

#include <objbase.h>

class CCoInitialize
{
public:
	CCoInitialize();
	~CCoInitialize() ;

public:
	operator HRESULT() const;

private:
	HRESULT m_hr;
};

#endif // _COINITIALIZE_H
