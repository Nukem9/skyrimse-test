#include "../../stdafx.h"

void SetName(uintptr_t Resource, const char *Format, ...)
{
	char buffer[1024];
	va_list va;

	va_start(va, Format);
	int len = vsprintf_s(buffer, Format, va);
	va_end(va);

	auto resource = *(ID3D11DeviceChild **)Resource;

	if (resource)
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, len, buffer);
}

uint8_t *sub_140D68A20;
__int64 hk_sub_140D68A20(__int64 a1, unsigned int a2, __int64 a3, __int64 a4)
{
	__int64 ret = ((decltype(&hk_sub_140D68A20))sub_140D68A20)(a1, a2, a3, a4);

	// Set name for use in VS's/nvidia's debugger
	uintptr_t v13 = a1 + 48i64 * a2;

	SetName(v13 + 2648, "%s TEX2D", g_RenderTargetNames[a2]);
	SetName(v13 + 2664, "%s RTV", g_RenderTargetNames[a2]);
	SetName(v13 + 2672, "%s SRV", g_RenderTargetNames[a2]);
	SetName(a1 + 48 * (a2 + 56), "%s UAV", g_RenderTargetNames[a2]);

	SetName(v13 + 2656, "%s COPY TEX2D", g_RenderTargetNames[a2]);
	SetName(v13 + 2680, "%s COPY SRV", g_RenderTargetNames[a2]);

	return ret;
}

void PatchBSGraphicsRenderTargetManager()
{
	sub_140D68A20 = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xD68A20), (PBYTE)hk_sub_140D68A20);
}