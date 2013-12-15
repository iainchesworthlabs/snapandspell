#include "Stdafx.h"
#include "OSVersion.h"
#include "DebugStream.h"

COSVersion::COSVersion() : m_WindowsVersion(UnknownVersion), m_WindowsVariant(UnknownVariant), m_WindowsArchitecture(UnknownArchitecture)
{
	BOOL bOsVersionInfoEx;
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	///FIXME PGPI pGPI;
	///FIXME DWORD dwType;

	::ZeroMemory(&si, sizeof(SYSTEM_INFO));
	::ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	bOsVersionInfoEx = ::GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osvi));

	if (!bOsVersionInfoEx)
	{
		wdebugstream() << TEXT("DEBUG: Failed to determine windows version information") << std::endl;
	}
	else
	{
		// Call ::GetNativeSystemInfo if supported or ::GetSystemInfo otherwise.
		static const LPCSTR GET_NATIVE_SYSTEM_INFO_NAME = "GetNativeSystemInfo";
		static const HMODULE KERNEL32_HANDLE = ::GetModuleHandle(TEXT("kernel32.dll"));
		
		if (0 == KERNEL32_HANDLE)
		{
			wdebugstream() << TEXT("DEBUG: Failed to get handle to Kernel32") << std::endl;
		}
		else
		{
			pGNSI = reinterpret_cast<PGNSI>(::GetProcAddress(KERNEL32_HANDLE, GET_NATIVE_SYSTEM_INFO_NAME));
			if (nullptr != pGNSI)
			{
				pGNSI(&si);
			}
			else
			{
				::GetSystemInfo(&si);
			}

			// Interpret the returned information...
			if ((VER_PLATFORM_WIN32_NT == osvi.dwPlatformId) && (4 <= osvi.dwMajorVersion))
			{
				switch (osvi.dwMajorVersion)
				{
				case 6:
					Decode_Win7_Win8_Win2008(osvi, si);
					break;

				case 5:
					Decode_Win2003_XP_HomeServer_StorageServer(osvi, si);
					break;

				default:
					break;
				}
			}
		}
	}
}

COSVersion::~COSVersion()
{
}

COSVersion::WindowsVersions COSVersion::Version() const
{
	return m_WindowsVersion;
}

COSVersion::WindowsVariants COSVersion::Variant() const
{
	return m_WindowsVariant;
}

void COSVersion::Decode_Win7_Win8_Win2008(OSVERSIONINFOEX& osvi, SYSTEM_INFO& si) const
{
	switch (osvi.dwMinorVersion)
	{
	case 0:
		m_WindowsVersion = ((VER_NT_WORKSTATION == osvi.wProductType) ? Vista : Server2008);
		break;

	case 1:
		m_WindowsVersion = ((VER_NT_WORKSTATION == osvi.wProductType) ? Windows7 : Server2008_R2);
		break;

	case 2:
		m_WindowsVersion = ((VER_NT_WORKSTATION == osvi.wProductType) ? Windows8 : Server2012);
		break;

	default:
		wdebugstream() << TEXT("DEBUG: Failed to determine this version of Windows (family = ") << osvi.dwMajorVersion << TEXT(")") << std::endl;
		break;
	}
}

void COSVersion::Decode_Win2003_XP_HomeServer_StorageServer(OSVERSIONINFOEX& osvi, SYSTEM_INFO& si) const
{
	switch (osvi.dwMinorVersion)
	{
	case 2:
		if (::GetSystemMetrics(SM_SERVERR2))
		{
			m_WindowsVersion = Server2003_R2;
		}
		else if (osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER)
		{
			m_WindowsVersion = StorageServer2003;
		}
		else if (osvi.wSuiteMask & VER_SUITE_WH_SERVER)
		{
			m_WindowsVersion = HomeServer;
		}
		else if ((VER_NT_WORKSTATION == osvi.wProductType) && (PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture))
		{
			m_WindowsVersion = XP;
			// m_WindowsVariant = 
			m_WindowsArchitecture = x64;
			
		}
		else
		{
			m_WindowsVersion = Server2003;
		}
		break;

	case 1:
		m_WindowsVersion = XP;
		m_WindowsVariant = ((osvi.wSuiteMask & VER_SUITE_PERSONAL) ? Home : Professional);
		// m_WindowsArchitecture = 
		break;

	case 0:
		m_WindowsVersion = Windows2000;

		if (VER_NT_WORKSTATION == osvi.wProductType )
		{
			m_WindowsVariant = Professional;
		}
		else
		{
			if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
			{ 
				m_WindowsVariant = Datacenter;
			}
			else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
			{
				m_WindowsVariant = AdvancedServer;
			}
			else 
			{
				m_WindowsVariant = Server;
			}
		}
		break;

	default:
		break;
	}
}
