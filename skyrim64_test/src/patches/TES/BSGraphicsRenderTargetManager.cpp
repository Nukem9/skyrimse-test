#include "../../common.h"
#include "BSGraphicsRenderTargetManager.h"
#include "BSShaderRenderTargets.h"

using namespace BSShaderRenderTargets;

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

uint8_t *CreateDepthStencil;
__int64 hk_CreateDepthStencil(__int64 a1, unsigned int aStencilIndex, __int64 a3, __int64 a4)
{
	__int64 ret = ((decltype(&hk_CreateDepthStencil))CreateDepthStencil)(a1, aStencilIndex, a3, a4);

	// Set name for use in VS's/nvidia's debugger
	uintptr_t v1 = a1 + (0x98 * aStencilIndex);

	SetName(v1 + 0x1FB8, GetStencilName(aStencilIndex));

	g_DepthStencils[aStencilIndex] = *(ID3D11DepthStencilView **)(v1 + 0x1FB8);
	return ret;
}

uint8_t *CreateRenderTarget;
__int64 hk_CreateRenderTarget(__int64 a1, unsigned int aTargetIndex, __int64 a3, __int64 a4)
{
	__int64 ret = ((decltype(&hk_CreateRenderTarget))CreateRenderTarget)(a1, aTargetIndex, a3, a4);

	// Set name for use in VS's/nvidia's debugger
	uintptr_t v13 = a1 + (0x30 * aTargetIndex);

	SetName(v13 + 2648, "%s TEX2D", GetTargetName(aTargetIndex));
	SetName(v13 + 2664, "%s RTV", GetTargetName(aTargetIndex));
	SetName(v13 + 2672, "%s SRV", GetTargetName(aTargetIndex));
	SetName(a1 + 48 * (aTargetIndex + 56), "%s UAV", GetTargetName(aTargetIndex));

	SetName(v13 + 2656, "%s COPY TEX2D", GetTargetName(aTargetIndex));
	SetName(v13 + 2680, "%s COPY SRV", GetTargetName(aTargetIndex));

	g_RenderTargets[aTargetIndex] = *(ID3D11RenderTargetView **)(v13 + 2664);
	return ret;
}

void PatchBSGraphicsRenderTargetManager()
{
	CreateDepthStencil = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xD6A300), (PBYTE)hk_CreateDepthStencil);
	CreateRenderTarget = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xD69ED0), (PBYTE)hk_CreateRenderTarget);
}