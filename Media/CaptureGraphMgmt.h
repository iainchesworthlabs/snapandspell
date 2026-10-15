#pragma once
#ifndef _CAPTUREGRAPHMGMT_H
#define _CAPTUREGRAPHMGMT_H

#include "CaptureEngineCOM.h"

namespace WordJewelsImager
{

class CaptureGraphManagement
{
public:
	CaptureGraphManagement();
	virtual ~CaptureGraphManagement();

public:
	static HRESULT AddFilterToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pFilter);
	static HRESULT AddAndConnectFilterToGraph(CaptureEngineCOM::IGraphBuilderPtr pGraph, CaptureEngineCOM::IBaseFilterPtr pFilter);
};

}
// namespace WordJewelsImager

#endif // _CAPTUREGRAPHMGMT_H
