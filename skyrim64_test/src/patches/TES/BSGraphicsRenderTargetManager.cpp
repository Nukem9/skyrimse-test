#include "../../common.h"
#include "BSShader/BSShaderRenderTargets.h"
#include "BSGraphicsRenderTargetManager.h"
#include <array>

using namespace BSShaderRenderTargets;
std::vector<std::pair<ID3D11ShaderResourceView *, std::string>> g_ResourceViews;

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

void SetResourceName(ID3D11DeviceChild *Resource, const char *Format, ...)
{
	if (!Resource)
		return;

	char buffer[1024];
	va_list va;

	va_start(va, Format);
	int len = _vsnprintf_s(buffer, _TRUNCATE, Format, va);
	va_end(va);

	Resource->SetPrivateData(WKPDID_D3DDebugObjectName, len, buffer);
}

uint8_t *CreateDepthStencil;
__int64 hk_CreateDepthStencil(__int64 a1, unsigned int aStencilIndex, __int64 a3, __int64 a4)
{
	// BSGraphics::Renderer::CreateDepthStencilTarget
	__int64 ret = ((decltype(&hk_CreateDepthStencil))CreateDepthStencil)(a1, aStencilIndex, a3, a4);

	// Set name for use in VS's/nvidia's debugger
	uintptr_t v1 = a1 + (0x98 * aStencilIndex);

	SetName(v1 + 0x1FB8, "%s TEX2D", GetDepthStencilName(aStencilIndex));

	for (int i = 0; i < *(int *)(a4 + 8); i++)
	{
		if (i >= 128)
			__debugbreak();

		unsigned int v15 = 19 * aStencilIndex;

		SetName(a1 + 0x1FC0 + 8 * (v15 + i), "%s SLICE%d DSV0", GetDepthStencilName(aStencilIndex), i);
		SetName(a1 + 0x2000 + 8 * (v15 + i), "%s SLICE%d DSV1", GetDepthStencilName(aStencilIndex), i);

		g_DepthStencils[aStencilIndex][i][0] = *(ID3D11DepthStencilView **)(a1 + 0x1FC0 + 8 * (v15 + i));
		g_DepthStencils[aStencilIndex][i][1] = *(ID3D11DepthStencilView **)(a1 + 0x2000 + 8 * (v15 + i));
	}

	g_DepthStencilTextures[aStencilIndex] = *(ID3D11Texture2D **)(v1 + 0x1FB8);
	return ret;
}

#include "BSGraphicsRenderer.h"
struct RenderTargetData
{
	ID3D11Texture2D *Texture;
	ID3D11Texture2D *TextureCopy;
	ID3D11RenderTargetView *RTV;		// For "Texture"
	ID3D11ShaderResourceView *SRV;		// For "Texture"
	ID3D11ShaderResourceView *SRVCopy;	// For "TextureCopy"
	ID3D11UnorderedAccessView *UAV;		// For "Texture"
};
static_assert(sizeof(RenderTargetData) == 0x30);
static_assert_offset(RenderTargetData, Texture, 0x0);
static_assert_offset(RenderTargetData, TextureCopy, 0x8);
static_assert_offset(RenderTargetData, RTV, 0x10);
static_assert_offset(RenderTargetData, SRV, 0x18);
static_assert_offset(RenderTargetData, SRVCopy, 0x20);
static_assert_offset(RenderTargetData, UAV, 0x28);

struct BSGraphics_Renderer_WIP
{
	char _pad0[0xA58];
	RenderTargetData m_RenderTargets[RENDER_TARGET_COUNT];
};

void aCreateRenderTarget(BSGraphics_Renderer_WIP *Renderer, uint32_t TargetIndex, const char *Name, const BSGraphics::RenderTargetProperties *Properties);

// Supposed to be in this file
struct RenderTargetManager
{
	BSGraphics::RenderTargetProperties pRenderTargetDataA[RENDER_TARGET_COUNT];

	void CreateRenderTarget(uint32_t TargetIndex, const BSGraphics::RenderTargetProperties *Properties)
	{
		AssertMsg(TargetIndex < RENDER_TARGET_COUNT, "Wrong target index");
		AssertMsg(TargetIndex != 0, "Framebuffer properties come from the renderer");

		memcpy(&pRenderTargetDataA[TargetIndex], Properties, sizeof(BSGraphics::RenderTargetProperties));
		aCreateRenderTarget((BSGraphics_Renderer_WIP *)(g_ModuleBase + 0x304E490), TargetIndex, GetRenderTargetName(TargetIndex), Properties);
	}
};

// Supposed to be in BSGraphicsRenderer.cpp
void aCreateRenderTarget(BSGraphics_Renderer_WIP *Renderer, uint32_t TargetIndex, const char *Name, const BSGraphics::RenderTargetProperties *Properties)
{
	auto device = BSGraphics::Renderer::GetGlobals()->m_Device;
	auto data = &Renderer->m_RenderTargets[TargetIndex];

	HRESULT hr = S_OK;
	DXGI_FORMAT dxgiFormat = (DXGI_FORMAT)Properties->eFormat;

	if (Properties->iMipLevel == -1)
	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = Properties->uiWidth;
		texDesc.Height = Properties->uiHeight;
		texDesc.MipLevels = (Properties->bAllowMipGeneration == false);
		texDesc.ArraySize = 1;
		texDesc.Format = dxgiFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		if (Properties->bSupportUnorderedAccess)
			texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

		if (Properties->bAllowMipGeneration)
			texDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

		hr |= device->CreateTexture2D(&texDesc, nullptr, &data->Texture);
		hr |= device->CreateRenderTargetView(data->Texture, nullptr, &data->RTV);
		hr |= device->CreateShaderResourceView(data->Texture, nullptr, &data->SRV);

		if (SUCCEEDED(hr))
			BSGraphics::Renderer::GetGlobals()->m_DeviceContext->ClearRenderTargetView(data->RTV, NiColorA::BLACK.Data());

		SetResourceName(data->Texture, "%s TEX2D", Name);
		SetResourceName(data->RTV, "%s RTV", Name);
		SetResourceName(data->SRV, "%s SRV", Name);

		if (Properties->bCopyable)
		{
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			hr |= device->CreateTexture2D(&texDesc, nullptr, &data->TextureCopy);
			hr |= device->CreateShaderResourceView(data->TextureCopy, nullptr, &data->SRVCopy);

			SetResourceName(data->TextureCopy, "%s COPY TEX2D", Name);
			SetResourceName(data->SRVCopy, "%s COPY SRV", Name);
		}

		if (Properties->bSupportUnorderedAccess)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = dxgiFormat;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;

			hr |= device->CreateUnorderedAccessView(data->Texture, &uavDesc, &data->UAV);

			SetResourceName(data->UAV, "%s UAV", Name);
		}
	}
	else
	{
		ID3D11Texture2D *textureTarget = Renderer->m_RenderTargets[Properties->uiTextureTarget].Texture;

		AssertMsg(textureTarget, "Can't create a render texture on a specified mip level because the texture has not been created.");

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = dxgiFormat;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		hr |= device->CreateRenderTargetView(textureTarget, &rtvDesc, &data->RTV);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = dxgiFormat;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = Properties->iMipLevel;
		srvDesc.Texture2D.MipLevels = 1;

		hr |= device->CreateShaderResourceView(textureTarget, &srvDesc, &data->SRV);

		if (Properties->bSupportUnorderedAccess)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = dxgiFormat;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = Properties->iMipLevel;

			hr |= device->CreateUnorderedAccessView(textureTarget, &uavDesc, &data->UAV);
		}

		SetResourceName(data->RTV, "%s MIP%d RTV", Name, Properties->iMipLevel);
		SetResourceName(data->SRV, "%s MIP%d SRV", Name, Properties->iMipLevel);
		SetResourceName(data->UAV, "%s MIP%d UAV", Name, Properties->iMipLevel);
	}

	AssertMsgVa(SUCCEEDED(hr), "Render target '%s' creation failed (Mip: %d)", Name, Properties->iMipLevel);

	g_RenderTargetTextures[TargetIndex] = data->Texture;
	g_RenderTargets[TargetIndex] = data->RTV;
}

void PatchBSGraphicsRenderTargetManager()
{
	CreateDepthStencil = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xD6A330), (PBYTE)hk_CreateDepthStencil);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xD741F0), &RenderTargetManager::CreateRenderTarget);
}