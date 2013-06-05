#pragma once
#include <Windows.h>
#include <Wia.h>

#pragma comment(lib, "WiaGuid.lib")

class ImageCapturer
{
public:
	ImageCapturer();
	virtual ~ImageCapturer();

private:
	HRESULT Stage0_CreateWiaDeviceManager(IWiaDevMgr2 **ppWiaDevMgr);
	HRESULT Stage1_EnumerateWiaDevices(IWiaDevMgr2 *pWiaDevMgr);
	HRESULT Stage2_ReadSomeWiaProperties(IWiaPropertyStorage *pWiaPropertyStorage);
	HRESULT Stage3_CreateWiaDevice(IWiaDevMgr2 *pWiaDevMgr, BSTR bstrDeviceID, IWiaItem2 **ppWiaDevice);
	HRESULT Stage4_EnumerateItems(IWiaItem2 *pWiaItem);

private:
	IWiaDevMgr2 *ppWiaDevMgr;
	IWiaPropertyStorage *pWiaPropertyStorage;
	IWiaItem2 *ppWiaDevice;
	IWiaItem2 *pWiaItem;
};
