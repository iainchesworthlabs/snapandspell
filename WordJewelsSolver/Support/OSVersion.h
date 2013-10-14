#pragma once
#ifndef _OSVERSION_H
#define _OSVERSION_H

class COSVersion
{
public:
	enum WindowsVersions
	{
		Windows8, Windows7, Vista, XP, Windows2000, 
		Server2012, Server2008_R2, Server2008, Server2003_R2, StorageServer2003, HomeServer, Server2003,
		UnknownVersion
	};

	enum WindowsVariants
	{
		Ultimate, Professional, HomePremium, HomeBasic, Home, Enterprise, Business,
		Starter, ClusterServer, Datacenter, Datacenter_Core, Enterprise_Itanium,
		SmallBusinessServer, SmallBusinessServer_Premium, Standard, Standard_Core,
		WebServer, WebServer_Core, AdvancedServer, Server, UnknownVariant
	};

	enum WindowsArchitectures
	{
		x64, x86, ARM, UnknownArchitecture
	};
	
public:
	COSVersion();
	~COSVersion();

public:
	WindowsVersions Version() const;
	WindowsVariants Variant() const;

private:
	typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
	typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

private:
	void Decode_Win7_Win8_Win2008(OSVERSIONINFOEX& osvi, SYSTEM_INFO& si) const;
	void Decode_Win2003_XP_HomeServer_StorageServer(OSVERSIONINFOEX& osvi, SYSTEM_INFO& si) const;
	void Decode_WindowsVariant(OSVERSIONINFOEX& osvi, SYSTEM_INFO& si) const;

private:
	mutable WindowsVersions m_WindowsVersion;
	mutable WindowsVariants m_WindowsVariant;
	mutable WindowsArchitectures m_WindowsArchitecture;
};

#endif // _OSVERSION_H
