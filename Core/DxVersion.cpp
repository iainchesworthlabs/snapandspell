#include "pch.h"
#include "DebugStream.h"
#include "DxVersion.h"
#include "OSVersion.h"

CDxVersion::CDxVersion() : 
	m_DxVersion(UnknownVersion),
	m_DxVariant(UnknownVariant),
	m_hDirect3D11(::LoadLibrary(TEXT("d3d11.dll"))),
	m_hDirect3D10(::LoadLibrary(TEXT("d3d10.dll"))),
	m_hDirect3D9(::LoadLibrary(TEXT("d3d9.dll"))),
	m_hDirectInput(::LoadLibrary(TEXT("dinput.dll"))),
	m_hDirectSetup(::LoadLibrary(TEXT("dsetup.dll")))
{
	if (nullptr != m_hDirect3D11)
	{
		wdebugstream() << TEXT("DEBUG: Detected DirectX 11") << std::endl;
		m_DxVersion = DirectX_11;
	}
	else if (nullptr != m_hDirect3D10)
	{
		wdebugstream() << TEXT("DEBUG: Detected DirectX 10") << std::endl;
		m_DxVersion = DirectX_10;
	}
	else if (LoadDirectSetupDlls() && (UnknownVersion != m_DxVersion))
	{
		// Version determined to be DirectX 5 through 9...note the left-hand shortcut
	}
	else if (LoadDirectInputDlls())
	{
		///FIXME
	}
	else
	{
		///FIXME
	}
}

CDxVersion::~CDxVersion()
{
	if (nullptr != m_hDirectSetup) ::FreeLibrary(m_hDirectSetup);
	if (nullptr != m_hDirectInput) ::FreeLibrary(m_hDirectInput);
	if (nullptr != m_hDirect3D9)   ::FreeLibrary(m_hDirect3D9);
	if (nullptr != m_hDirect3D10)  ::FreeLibrary(m_hDirect3D10);
	if (nullptr != m_hDirect3D11)  ::FreeLibrary(m_hDirect3D11);
}

CDxVersion::DxVersions CDxVersion::Version() const
{
	return m_DxVersion;
}

CDxVersion::DxVariants CDxVersion::Variant() const
{
	return m_DxVariant;
}

bool CDxVersion::LoadDirectInputDlls()
{
	bool returnValue(false);

	if (nullptr == m_hDirectInput)
	{
		wdebugstream() << TEXT("DEBUG: Failed to load the DINPUT.DLL") << std::endl;
	}
	else
	{
		wdebugstream() << TEXT("DEBUG: Succeeded in loading the DINPUT.DLL") << std::endl;
		///FIXME
	}

	return returnValue;
}

bool CDxVersion::LoadDirectSetupDlls()
{
	bool returnValue(false);

	if (nullptr == m_hDirectSetup)
	{
		wdebugstream() << TEXT("DEBUG: Failed to load the DSETUP.DLL") << std::endl;
	}
	else
	{
		typedef HRESULT(WINAPI *DSETUPCREATE)(DWORD*,DWORD*);
		const DSETUPCREATE hDirectXSetupGetVersion(reinterpret_cast<DSETUPCREATE>(::GetProcAddress(m_hDirectSetup, "DirectXSetupGetVersion")));

		DWORD dwRevision, dwVersion;

		if (nullptr == hDirectXSetupGetVersion)
		{
			wdebugstream() << TEXT("DEBUG: Failed to get the function pointer for \"DirectXSetupGetVersion()\"") << std::endl;
		}
		else if (FAILED(hDirectXSetupGetVersion(&dwVersion, &dwRevision)))
		{
			wdebugstream() << TEXT("DEBUG: Failed to query for the DirectX version via Setup API") << std::endl;
		}
		else
		{
			switch (dwVersion)
			{
			case 0x00040005: Process_DirectX5(dwVersion, dwRevision); break;
			case 0x00040006: Process_DirectX6(dwVersion, dwRevision); break;
			case 0x00040007: Process_DirectX7(dwVersion, dwRevision); break;
			case 0x00040008: Process_DirectX8(dwVersion, dwRevision); break;
			case 0x00040009: Process_DirectX9(dwVersion, dwRevision); break;

			default:
				wdebugstream() << TEXT("DEBUG: Failed to determine the actual DirectX version via Setup API") << std::endl;
				break;
			}

			returnValue = true;
		}
	}

	return returnValue;
}

void CDxVersion::Process_DirectX5(DWORD dwVersion, DWORD dwRevision)
{
	wdebugstream() << TEXT("DEBUG: Detected DirectX 5") << std::endl;
	m_DxVersion = DirectX_5;
}

void CDxVersion::Process_DirectX6(DWORD dwVersion, DWORD dwRevision)
{
	wdebugstream() << TEXT("DEBUG: Detected DirectX 6") << std::endl;
	m_DxVersion = DirectX_6;
}

void CDxVersion::Process_DirectX7(DWORD dwVersion, DWORD dwRevision)
{
	wdebugstream() << TEXT("DEBUG: Detected DirectX 7") << std::endl;
	m_DxVersion = DirectX_7;
}

void CDxVersion::Process_DirectX8(DWORD dwVersion, DWORD dwRevision)
{
	wdebugstream() << TEXT("DEBUG: Detected DirectX 8") << std::endl;

	switch (dwRevision)
	{
	case 0x0001032A:
	case 0x00010371:
		m_DxVersion = DirectX_8_1;
		break;

	case 0x00010385:
		m_DxVersion = DirectX_8_1a; // 8.1a or b...stupid versioning
		break;

	case 0x00020386:
		m_DxVersion = DirectX_8_2;
		break;

	default:
		m_DxVersion = DirectX_8;
		break;
	}
}

void CDxVersion::Process_DirectX9(DWORD dwVersion, DWORD dwRevision)
{
	wdebugstream() << TEXT("DEBUG: Detected DirectX 9") << std::endl;

	switch (dwRevision)
	{
	case 0x00000384: m_DxVersion = DirectX_9; break;
	case 0x00000385: m_DxVersion = DirectX_9a; break;
	case 0x00000386: m_DxVersion = DirectX_9b; break;
	case 0x00000387: m_DxVersion = DirectX_9b; break; // 9.0b or c...stupid versioning
	case 0x00000388: m_DxVersion = DirectX_9c; break;
	default: m_DxVersion = DirectX_9; break;
	}
}
