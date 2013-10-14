#pragma once
#ifndef _DXVERSION_H
#define _DXVERSION_H

class CDxVersion
{
public:
	enum DxVersions
	{
		DirectX_11_2, DirectX_11_1, DirectX_11, 
		DirectX_10_1, DirectX_10, 
		DirectX_9c, DirectX_9b, DirectX_9a, DirectX_9,
		DirectX_8_2, DirectX_8_1b, DirectX_8_1a, DirectX_8_1, DirectX_8a, DirectX_8,
		DirectX_7_1, DirectX_7a, DirectX_7, 
		DirectX_6_1a, DirectX_6_1, DirectX_6, 
		DirectX_5_2, DirectX_5, 
		DirectX_4,
		DirectX_3b, DirectX_3a, DirectX_3,
		DirectX_2a, DirectX_2,
		DirectX_1,
		UnknownVersion
	};

	enum DxVariants
	{
		UnknownVariant
	};

public:
	CDxVersion();
	~CDxVersion();
	
public:
	DxVersions Version() const;
	DxVariants Variant() const;

private:
	bool LoadDirectSetupDlls();
	bool LoadDirectInputDlls();
	void Process_DirectX5(DWORD dwVersion, DWORD dwRevision);
	void Process_DirectX6(DWORD dwVersion, DWORD dwRevision);
	void Process_DirectX7(DWORD dwVersion, DWORD dwRevision);
	void Process_DirectX8(DWORD dwVersion, DWORD dwRevision);
	void Process_DirectX9(DWORD dwVersion, DWORD dwRevision);

private:
	mutable DxVersions m_DxVersion;
	mutable DxVariants m_DxVariant;

private:
	HMODULE m_hDirect3D11;
	HMODULE m_hDirect3D10;
	HMODULE m_hDirect3D9;
	HMODULE m_hDirectSetup;
	HMODULE m_hDirectInput;
};

#endif // _DXVERSION_H
