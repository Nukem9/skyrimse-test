#include "../../common.h"
#include "../TES/BSShader/BSShaderRenderTargets.h"
#include "BSGraphicsRenderTargetManager_CK.h"

void BSGraphics_CK::Renderer::SetResourceName(ID3D11DeviceChild *Resource, const char *Format, ...)
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

void BSGraphics_CK::Renderer::CreateRenderTarget(uint32_t TargetIndex, const char *Name, const RenderTargetProperties *Properties)
{
	Assert(TargetIndex < RENDER_TARGET_COUNT && TargetIndex != RENDER_TARGET_NONE);

	auto device = *(ID3D11Device **)OFFSET(0x56B6B40, 1530);
	auto data = &pRenderTargets[TargetIndex];

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
		ID3D11Texture2D *textureTarget = pRenderTargets[Properties->uiTextureTarget].Texture;

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
}

void BSGraphics_CK::Renderer::CreateDepthStencil(uint32_t TargetIndex, const char *Name, const DepthStencilTargetProperties *Properties)
{
	Assert(TargetIndex < RENDER_TARGET_COUNT && TargetIndex != RENDER_TARGET_NONE);

	auto device = *(ID3D11Device **)OFFSET(0x56B6B40, 1530);
	auto data = &pDepthStencils[TargetIndex];

	HRESULT hr = S_OK;

	// Create backing texture
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = Properties->uiWidth;
	texDesc.Height = Properties->uiHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = Properties->uiArraySize;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	if (Properties->Use16BitsDepth)
		texDesc.Format = DXGI_FORMAT_R16_TYPELESS;
	else
		texDesc.Format = DXGI_FORMAT_R32_TYPELESS;

	if (Properties->Stencil)
		texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	hr |= device->CreateTexture2D(&texDesc, nullptr, &data->Texture);
	SetResourceName(data->Texture, "%s TEX2D", Name);

	// Depth stencil 1 (read / write / main)
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc1;
	dsvDesc1.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc1.Flags = 0;
	dsvDesc1.Texture2D.MipSlice = 0;

	if (Properties->Use16BitsDepth)
		dsvDesc1.Format = DXGI_FORMAT_D16_UNORM;
	else
		dsvDesc1.Format = DXGI_FORMAT_D32_FLOAT;

	if (Properties->Stencil)
		dsvDesc1.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Depth stencil 2 (read only / copy)
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc2;
	dsvDesc2.Format = dsvDesc1.Format;
	dsvDesc2.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc2.Flags = D3D11_DSV_READ_ONLY_DEPTH;
	dsvDesc2.Texture2D.MipSlice = 0;

	if (Properties->Stencil)
		dsvDesc2.Flags |= D3D11_DSV_READ_ONLY_STENCIL;

	for (uint32_t i = 0; i < Properties->uiArraySize; i++)
	{
		if (Properties->uiArraySize > 1)
		{
			dsvDesc1.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc1.Texture2DArray.MipSlice = 0;
			dsvDesc1.Texture2DArray.ArraySize = 1;
			dsvDesc1.Texture2DArray.FirstArraySlice = i;

			dsvDesc2.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc2.Texture2DArray.MipSlice = 0;
			dsvDesc2.Texture2DArray.ArraySize = 1;
			dsvDesc2.Texture2DArray.FirstArraySlice = i;
		}

		hr |= device->CreateDepthStencilView(data->Texture, &dsvDesc1, &data->PrimaryViews[i]);
		hr |= device->CreateDepthStencilView(data->Texture, &dsvDesc2, &data->SecondaryViews[i]);

		SetResourceName(data->PrimaryViews[i], "%s DSV PRI SLICE%u", Name, i);
		SetResourceName(data->SecondaryViews[i], "%s DSV SEC SLICE%u", Name, i);
	}

	// Shader resource access
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	if (Properties->Use16BitsDepth)
		srvDesc.Format = DXGI_FORMAT_R16_UNORM;
	else
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	if (Properties->Stencil)
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	if (Properties->uiArraySize > 1)
	{
		srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = Properties->uiArraySize;
	}

	hr |= device->CreateShaderResourceView(data->Texture, &srvDesc, &data->DepthSRV);

	if (Properties->Stencil)
	{
		srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		hr |= device->CreateShaderResourceView(data->Texture, &srvDesc, &data->StencilSRV);
	}

	SetResourceName(data->DepthSRV, "%s DEPTH SRV", Name);
	SetResourceName(data->StencilSRV, "%s STENCIL SRV", Name);

	AssertMsgVa(SUCCEEDED(hr), "Depth stencil target '%s' creation failed", Name);
}

void BSGraphics_CK::Renderer::CreateCubemapRenderTarget(uint32_t TargetIndex, const char *Name, const CubeMapRenderTargetProperties *Properties)
{
	Assert(TargetIndex < RENDER_TARGET_CUBEMAP_COUNT);

	auto device = *(ID3D11Device **)OFFSET(0x56B6B40, 1530);
	auto data = &pCubemapRenderTargets[TargetIndex];

	HRESULT hr = S_OK;

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = Properties->uiWidth;
	texDesc.Height = Properties->uiHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = (DXGI_FORMAT)Properties->eFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	hr |= device->CreateTexture2D(&texDesc, nullptr, &data->Texture);
	SetResourceName(data->Texture, "%s TEX2D", Name);

	// Create a separate render target for each side
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.FirstArraySlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 1;

	for (uint32_t i = 0; i < 6; i++)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;

		hr |= device->CreateRenderTargetView(data->Texture, &rtvDesc, &data->CubeSideRTV[i]);
		SetResourceName(data->Texture, "%s SIDE %u RTV", Name, i);
	}

	// Shader sampler
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;

	hr |= device->CreateShaderResourceView(data->Texture, &srvDesc, &data->SRV);
	SetResourceName(data->Texture, "%s SRV", Name);

	AssertMsgVa(SUCCEEDED(hr), "Cubemap render target '%s' creation failed", Name);
}

void BSGraphics_CK::Renderer::DestroyRenderTarget(uint32_t TargetIndex)
{
	auto data = &pRenderTargets[TargetIndex];

	if (data->Texture)
		data->Texture->Release();

	if (data->TextureCopy)
		data->TextureCopy->Release();

	if (data->RTV)
		data->RTV->Release();

	if (data->SRV)
		data->SRV->Release();

	if (data->SRVCopy)
		data->SRVCopy->Release();

	if (data->UAV)
		data->UAV->Release();

	memset(data, 0, sizeof(BSGraphics_CK::RenderTargetData));
}

void BSGraphics_CK::Renderer::DestroyDepthStencil(uint32_t TargetIndex)
{
	auto data = &pDepthStencils[TargetIndex];

	if (data->Texture)
		data->Texture->Release();

	for (ID3D11DepthStencilView *view : data->PrimaryViews)
	{
		if (view)
			view->Release();
	}

	for (ID3D11DepthStencilView *view : data->SecondaryViews)
	{
		if (view)
			view->Release();
	}

	if (data->DepthSRV)
		data->DepthSRV->Release();

	if (data->StencilSRV)
		data->StencilSRV->Release();

	memset(data, 0, sizeof(BSGraphics_CK::DepthStencilData));
}

void BSGraphics_CK::Renderer::DestroyCubemapRenderTarget(uint32_t TargetIndex)
{
	auto data = &pCubemapRenderTargets[TargetIndex];

	if (data->Texture)
		data->Texture->Release();

	for (ID3D11RenderTargetView *view : data->CubeSideRTV)
	{
		if (view)
			view->Release();
	}

	if (data->SRV)
		data->SRV->Release();

	memset(data, 0, sizeof(BSGraphics_CK::CubemapRenderTargetData));
}

void BSGraphicsRenderTargetManager_CK::CreateRenderTarget(uint32_t TargetIndex, const BSGraphics_CK::RenderTargetProperties *Properties)
{
	AssertMsg(TargetIndex < RENDER_TARGET_COUNT && TargetIndex != RENDER_TARGET_NONE, "Wrong target index");
	AssertMsg(TargetIndex != 0, "Framebuffer properties come from the renderer");

	BSGraphics_CK::Renderer::Instance.DestroyRenderTarget(TargetIndex);
	pRenderTargetDataA[TargetIndex] = *Properties;
	BSGraphics_CK::Renderer::Instance.CreateRenderTarget(TargetIndex, GetRenderTargetName(TargetIndex), Properties);
}

void BSGraphicsRenderTargetManager_CK::CreateDepthStencil(uint32_t TargetIndex, const BSGraphics_CK::DepthStencilTargetProperties *Properties)
{
	AssertMsg(TargetIndex < DEPTH_STENCIL_COUNT && TargetIndex != DEPTH_STENCIL_TARGET_NONE, "Wrong target index");

	BSGraphics_CK::Renderer::Instance.DestroyDepthStencil(TargetIndex);
	pDepthStencilTargetDataA[TargetIndex] = *Properties;
	BSGraphics_CK::Renderer::Instance.CreateDepthStencil(TargetIndex, GetDepthStencilName(TargetIndex), Properties);
}

void BSGraphicsRenderTargetManager_CK::CreateCubemapRenderTarget(uint32_t TargetIndex, const BSGraphics_CK::CubeMapRenderTargetProperties *Properties)
{
	AssertMsg(TargetIndex < RENDER_TARGET_CUBEMAP_COUNT, "Wrong target index");

	BSGraphics_CK::Renderer::Instance.DestroyCubemapRenderTarget(TargetIndex);
	pCubeMapRenderTargetDataA[TargetIndex] = *Properties;
	BSGraphics_CK::Renderer::Instance.CreateCubemapRenderTarget(TargetIndex, GetCubemapRenderTargetName(TargetIndex), Properties);
}

const char *BSGraphicsRenderTargetManager_CK::GetRenderTargetName(uint32_t Index)
{
	Assert(Index < RENDER_TARGET_COUNT);

	return BSShaderRenderTargets::GetRenderTargetName(Index);
}

const char *BSGraphicsRenderTargetManager_CK::GetDepthStencilName(uint32_t Index)
{
	Assert(Index < DEPTH_STENCIL_COUNT);

	return BSShaderRenderTargets::GetDepthStencilName(Index);
}

const char *BSGraphicsRenderTargetManager_CK::GetCubemapRenderTargetName(uint32_t Index)
{
	Assert(Index < RENDER_TARGET_CUBEMAP_COUNT);

	return BSShaderRenderTargets::GetCubemapName(Index);
}