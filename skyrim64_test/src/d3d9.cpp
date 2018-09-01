#include "common.h"
#include "d3dconv.h"

#if SKYRIM64_CREATIONKIT_DLL

typedef long D3DCOLOR;
typedef void *IDirect3D9;

D3D_EXPORT
IDirect3D9 *D3DAPI Direct3DCreate9(UINT SDKVersion)
{
	static auto ptrDirect3DCreate9 = []()
	{
		char systemDir[1024];
		uint32_t len = GetSystemDirectory(systemDir, ARRAYSIZE(systemDir));

		Assert(len != 0);

		strcat_s(systemDir, "\\d3d9.dll");
		return (decltype(&Direct3DCreate9))GetProcAddress(LoadLibraryA(systemDir), "Direct3DCreate9");
	}();

	return ptrDirect3DCreate9(SDKVersion);
}

D3D_EXPORT
INT D3DAPI D3DPERF_BeginEvent(D3DCOLOR col, LPCWSTR wszName)
{
	return 0;
}

D3D_EXPORT
INT D3DAPI D3DPERF_EndEvent()
{
	return 0;
}

#endif // SKYRIM64_CREATIONKIT_DLL