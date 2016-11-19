#include "stdafx.h"

// D3D11 hooks that probably don't do anything

HRESULT WINAPI hk_CreateDXGIFactory(REFIID riid, void **ppFactory)
{
	if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory3), ppFactory)))
		return S_OK;

	if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory2), ppFactory)))
		return S_OK;

	return CreateDXGIFactory1(__uuidof(IDXGIFactory), ppFactory);
}

HRESULT WINAPI hk_D3D11CreateDeviceAndSwapChain(
	_In_opt_        IDXGIAdapter         *pAdapter,
					D3D_DRIVER_TYPE      DriverType,
					HMODULE              Software,
					UINT                 Flags,
	_In_opt_  const D3D_FEATURE_LEVEL    *pFeatureLevels,
					UINT                 FeatureLevels,
					UINT                 SDKVersion,
	_In_opt_  const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
	_Out_opt_       IDXGISwapChain       **ppSwapChain,
	_Out_opt_       ID3D11Device         **ppDevice,
	_Out_opt_       D3D_FEATURE_LEVEL    *pFeatureLevel,
	_Out_opt_       ID3D11DeviceContext  **ppImmediateContext
)
{
	static D3D_FEATURE_LEVEL testFeatureLevels[] =
	{
		(D3D_FEATURE_LEVEL)0xc100,	// D3D_FEATURE_LEVEL_12_1
		(D3D_FEATURE_LEVEL)0xc000,	// D3D_FEATURE_LEVEL_12_0
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Loop to get the highest available feature level; SkyrimSE originally uses D3D_FL_9_1
	D3D_FEATURE_LEVEL level;
	HRESULT hr;

	for (int i = 0; i < ARRAYSIZE(testFeatureLevels); i++)
	{
		hr = D3D11CreateDeviceAndSwapChain(
			pAdapter,
			DriverType,
			Software,
			Flags,
			&testFeatureLevels[i],
			1,
			SDKVersion,
			pSwapChainDesc,
			ppSwapChain,
			ppDevice,
			&level,
			ppImmediateContext);

		// Exit if device was created
		if (SUCCEEDED(hr))
		{
			if (pFeatureLevel)
				*pFeatureLevel = level;

			break;
		}
	}

	return hr;
}

void PatchD3D11()
{
	PatchIAT(hk_CreateDXGIFactory, "dxgi.dll", "CreateDXGIFactory");
	PatchIAT(hk_D3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain");
}