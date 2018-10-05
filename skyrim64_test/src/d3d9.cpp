#include "common.h"
#include "d3dconv.h"

#if SKYRIM64_CREATIONKIT_DLL

typedef long D3DCOLOR;
typedef void *IDirect3D9;

HMODULE GetD3D9()
{
	static HMODULE hD3D9 = []()
	{
		char systemDir[1024];
		uint32_t len = GetSystemDirectory(systemDir, ARRAYSIZE(systemDir));

		Assert(len != 0);

		strcat_s(systemDir, "\\d3d9.dll");
		return LoadLibraryA(systemDir);
	}();

	// Can't call LoadLibrary multiple times - it might fuck up and return this dll
	return hD3D9;
}

D3D_EXPORT
IDirect3D9 *D3DAPI Direct3DCreate9(UINT SDKVersion)
{
	static auto ptrDirect3DCreate9 = []()
	{
		return (decltype(&Direct3DCreate9))GetProcAddress(GetD3D9(), "Direct3DCreate9");
	}();

	return ptrDirect3DCreate9(SDKVersion);
}

D3D_EXPORT
INT D3DAPI D3DPERF_BeginEvent(D3DCOLOR col, LPCWSTR wszName)
{
	static auto ptrD3DPERF_BeginEvent = []()
	{
		return (decltype(&D3DPERF_BeginEvent))GetProcAddress(GetD3D9(), "D3DPERF_BeginEvent");
	}();

	return ptrD3DPERF_BeginEvent(col, wszName);
}

D3D_EXPORT
INT D3DAPI D3DPERF_EndEvent()
{
	static auto ptrD3DPERF_EndEvent = []()
	{
		return (decltype(&D3DPERF_EndEvent))GetProcAddress(GetD3D9(), "D3DPERF_EndEvent");
	}();

	return ptrD3DPERF_EndEvent();
}

#endif // SKYRIM64_CREATIONKIT_DLL