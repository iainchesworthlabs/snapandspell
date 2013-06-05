#include "ImageCapturer.h"
#include <Sti.h>
#include <WiaVideo.h>
#include "DebugLogger.h"

ImageCapturer::ImageCapturer() : ppWiaDevMgr(nullptr), pWiaPropertyStorage(nullptr), pWiaItem(nullptr)
{
	// Initialize COM for this thread
	::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	Stage0_CreateWiaDeviceManager(&ppWiaDevMgr);
	Stage1_EnumerateWiaDevices(ppWiaDevMgr);

	Stage3_CreateWiaDevice(ppWiaDevMgr, L"", &ppWiaDevice);
	Stage4_EnumerateItems(pWiaItem);
}

ImageCapturer::~ImageCapturer()
{
	// Uninitialize COM for this thread.
	::CoUninitialize();
}

HRESULT ImageCapturer::Stage0_CreateWiaDeviceManager(IWiaDevMgr2 **ppWiaDevMgr)
{
	// Validate arguments
	if (NULL == ppWiaDevMgr)
	{
		return E_INVALIDARG;
	}

	// Initialize out variables
	*ppWiaDevMgr = NULL;

	// Create an instance of the device manager
	HRESULT hr = CoCreateInstance(CLSID_WiaDevMgr2, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr2, (void**)ppWiaDevMgr); 

	// Return the result of creating the device manager
	return hr;
}

HRESULT ImageCapturer::Stage1_EnumerateWiaDevices(IWiaDevMgr2 *pWiaDevMgr)
{
	// Validate arguments
	if (nullptr == pWiaDevMgr)
	{
		return E_INVALIDARG;
	}

	// Get a device enumerator interface
	IEnumWIA_DEV_INFO *pWiaEnumDevInfo = nullptr;
	HRESULT hr = pWiaDevMgr->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &pWiaEnumDevInfo);
	if (SUCCEEDED(hr))
	{
		// Loop until you get an error or pWiaEnumDevInfo->Next returns
		// S_FALSE to signal the end of the list.
		while (S_OK == hr)
		{
			// Get the next device's property storage interface pointer
			IWiaPropertyStorage *pWiaPropertyStorage = nullptr;
			hr = pWiaEnumDevInfo->Next(1, &pWiaPropertyStorage, nullptr);

			// pWiaEnumDevInfo->Next will return S_FALSE when the list is
			// exhausted, so check for S_OK before using the returned value.
			if (hr == S_OK)
			{
				// Do something with the device's IWiaPropertyStorage*
				Stage2_ReadSomeWiaProperties(pWiaPropertyStorage);

				// Release the device's IWiaPropertyStorage*
				pWiaPropertyStorage->Release();
				pWiaPropertyStorage = nullptr;
			}
		}

		// If the result of the enumeration is S_FALSE (which is normal), change it to S_OK.
		if (S_FALSE == hr)
		{
			hr = S_OK;
		}

		// Release the enumerator
		pWiaEnumDevInfo->Release();
		pWiaEnumDevInfo = nullptr;
	}

	// Return the result of the enumeration
	return hr;
}

HRESULT ImageCapturer::Stage2_ReadSomeWiaProperties(IWiaPropertyStorage *pWiaPropertyStorage)
{
	// Validate arguments
	if (nullptr == pWiaPropertyStorage)
	{
		return E_INVALIDARG;
	}

	// Declare PROPSPECs and PROPVARIANTs, and initialize them to zero.
	PROPSPEC PropSpec[3] = {0};
	PROPVARIANT PropVar[3] = {0};

	// How many properties are you querying for?
	const ULONG c_nPropertyCount = sizeof(PropSpec)/sizeof(PropSpec[0]);

	// Define which properties you want to read: 

	// Device ID.  This is what you would use to create the device.
	PropSpec[0].ulKind = PRSPEC_PROPID;
	PropSpec[0].propid = WIA_DIP_DEV_ID;

	// Device Name
	PropSpec[1].ulKind = PRSPEC_PROPID;
	PropSpec[1].propid = WIA_DIP_DEV_NAME;

	// Device description
	PropSpec[2].ulKind = PRSPEC_PROPID;
	PropSpec[2].propid = WIA_DIP_DEV_DESC;

	// Ask for the property values
	HRESULT hr = pWiaPropertyStorage->ReadMultiple(c_nPropertyCount, PropSpec, PropVar);
	if (SUCCEEDED(hr))
	{
		// IWiaPropertyStorage::ReadMultiple will return S_FALSE if some properties could not 
		// be read, so you have to check the return types for each requested item.

		// Check the return type for the device ID
		if (VT_BSTR == PropVar[0].vt)
		{
			// Do something with the device ID
			DebugLogger() << TEXT("WIA_DIP_DEV_ID: ") << PropVar[0].bstrVal << std::endl;
		}

		// Check the return type for the device name
		if (VT_BSTR == PropVar[1].vt)
		{
			// Do something with the device name
			DebugLogger() << TEXT("WIA_DIP_DEV_NAME: ") << PropVar[1].bstrVal << std::endl;
		}

		// Check the return type for the device description
		if (VT_BSTR == PropVar[2].vt)
		{
			//
			// Do something with the device description
			//
			DebugLogger() << TEXT("WIA_DIP_DEV_DESC: ") << PropVar[2].bstrVal << std::endl;
		}

		// Free the returned PROPVARIANTs
		FreePropVariantArray( c_nPropertyCount, PropVar );
	}

	// Return the result of reading the properties
	return hr;
}

HRESULT ImageCapturer::Stage3_CreateWiaDevice(IWiaDevMgr2 *pWiaDevMgr, BSTR bstrDeviceID, IWiaItem2 **ppWiaDevice) 
{
	// Validate arguments
	if (nullptr == pWiaDevMgr || nullptr == bstrDeviceID || nullptr == ppWiaDevice)
	{
		return E_INVALIDARG;
	}

	// Initialize out variables
	*ppWiaDevice = nullptr;

	// Create the WIA Device
	HRESULT hr = pWiaDevMgr->CreateDevice(0, bstrDeviceID, ppWiaDevice);

	// Return the result of creating the device
	return hr;
}


HRESULT ImageCapturer::Stage4_EnumerateItems(IWiaItem2 *pWiaItem)
{
	// Validate arguments
	if (nullptr == pWiaItem)
	{
		return E_INVALIDARG;
	}

	// Get the item type for this item.
	LONG lItemType = 0;
	HRESULT hr = pWiaItem->GetItemType(&lItemType);
	if (SUCCEEDED(hr))
	{
		// If it is a folder, or it has attachments, enumerate its children.
		if (lItemType & WiaItemTypeFolder || lItemType & WiaItemTypeHasAttachments)
		{
			// Get the child item enumerator for this item.
			IEnumWiaItem2 *pEnumWiaItem = nullptr;

			hr = pWiaItem->EnumChildItems(nullptr, &pEnumWiaItem);
			if (SUCCEEDED(hr))
			{
				// Loop until you get an error or pEnumWiaItem->Next returns S_FALSE to signal the end of the list.
				while (S_OK == hr)
				{
					// Get the next child item.
					IWiaItem2 *pChildWiaItem = nullptr;
					hr = pEnumWiaItem->Next(1, &pChildWiaItem, nullptr);

					// pEnumWiaItem->Next will return S_FALSE when the list is
					// exhausted, so check for S_OK before using the returned value.
					if (S_OK == hr)
					{
						// Recurse into this item.
						hr = Stage4_EnumerateItems(pChildWiaItem);

						// Release this item.
						pChildWiaItem->Release();
						pChildWiaItem = nullptr;
					}
				}

				// If the result of the enumeration is S_FALSE (which
				// is normal), change it to S_OK.
				if (S_FALSE == hr)
				{
					hr = S_OK;
				}

				// Release the enumerator.
				pEnumWiaItem->Release();
				pEnumWiaItem = nullptr;
			}
		}
	}
	return  hr;
}
