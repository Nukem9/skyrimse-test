#pragma once

#include "../TES/BSShader/BSShaderRenderTargets.h"

class BSGraphics_CK
{
public:
	struct RenderTargetProperties
	{
		uint32_t uiWidth;
		uint32_t uiHeight;
		uint32_t eFormat;
		bool bCopyable;
		bool bSupportUnorderedAccess;
		bool bAllowMipGeneration;
		int iMipLevel;
		uint32_t uiTextureTarget;
		uint32_t uiUnknown;
	};
	static_assert(sizeof(RenderTargetProperties) == 0x1C);
	static_assert_offset(RenderTargetProperties, uiWidth, 0x0);
	static_assert_offset(RenderTargetProperties, uiHeight, 0x4);
	static_assert_offset(RenderTargetProperties, eFormat, 0x8);
	static_assert_offset(RenderTargetProperties, bCopyable, 0xC);
	static_assert_offset(RenderTargetProperties, bSupportUnorderedAccess, 0xD);
	static_assert_offset(RenderTargetProperties, bAllowMipGeneration, 0xE);
	static_assert_offset(RenderTargetProperties, iMipLevel, 0x10);
	static_assert_offset(RenderTargetProperties, uiTextureTarget, 0x14);

	struct DepthStencilTargetProperties
	{
		uint32_t uiWidth;
		uint32_t uiHeight;
		uint32_t uiArraySize;
		bool Unknown1;
		bool Stencil;
		bool Use16BitsDepth;
	};
	static_assert(sizeof(DepthStencilTargetProperties) == 0x10);
	static_assert_offset(DepthStencilTargetProperties, uiWidth, 0x0);
	static_assert_offset(DepthStencilTargetProperties, uiHeight, 0x4);
	static_assert_offset(DepthStencilTargetProperties, uiArraySize, 0x8);
	static_assert_offset(DepthStencilTargetProperties, Stencil, 0xD);
	static_assert_offset(DepthStencilTargetProperties, Use16BitsDepth, 0xE);

	struct CubeMapRenderTargetProperties
	{
		uint32_t uiWidth;
		uint32_t uiHeight;
		uint32_t eFormat;
	};
	static_assert(sizeof(CubeMapRenderTargetProperties) == 0xC);
	static_assert_offset(CubeMapRenderTargetProperties, uiWidth, 0x0);
	static_assert_offset(CubeMapRenderTargetProperties, uiHeight, 0x4);
	static_assert_offset(CubeMapRenderTargetProperties, eFormat, 0x8);

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

	struct DepthStencilData
	{
		ID3D11Texture2D *Texture;
		ID3D11DepthStencilView *PrimaryViews[8];
		ID3D11DepthStencilView *SecondaryViews[8];
		ID3D11ShaderResourceView *DepthSRV;
		ID3D11ShaderResourceView *StencilSRV;
	};
	static_assert(sizeof(DepthStencilData) == 0x98);
	static_assert_offset(DepthStencilData, Texture, 0x0);
	static_assert_offset(DepthStencilData, PrimaryViews, 0x8);
	static_assert_offset(DepthStencilData, SecondaryViews, 0x48);
	static_assert_offset(DepthStencilData, DepthSRV, 0x88);
	static_assert_offset(DepthStencilData, StencilSRV, 0x90);

	struct CubemapRenderTargetData
	{
		ID3D11Texture2D *Texture;
		ID3D11RenderTargetView *CubeSideRTV[6];
		ID3D11ShaderResourceView *SRV;
	};
	static_assert(sizeof(CubemapRenderTargetData) == 0x40);
	static_assert_offset(CubemapRenderTargetData, Texture, 0x0);
	static_assert_offset(CubemapRenderTargetData, CubeSideRTV, 0x8);
	static_assert_offset(CubemapRenderTargetData, SRV, 0x38);

	class Renderer
	{
	public:
		inline AutoPtr(Renderer, Instance, 0x56B73A0);

		char _pad0[0xAF0];
		RenderTargetData pRenderTargets[RENDER_TARGET_COUNT];
		DepthStencilData pDepthStencils[DEPTH_STENCIL_COUNT];
		CubemapRenderTargetData pCubemapRenderTargets[RENDER_TARGET_CUBEMAP_COUNT];

		void SetResourceName(ID3D11DeviceChild *Resource, const char *Format, ...);
		void CreateRenderTarget(uint32_t TargetIndex, const char *Name, const RenderTargetProperties *Properties);
		void CreateDepthStencil(uint32_t TargetIndex, const char *Name, const DepthStencilTargetProperties *Properties);
		void CreateCubemapRenderTarget(uint32_t TargetIndex, const char *Name, const CubeMapRenderTargetProperties *Properties);

		void DestroyRenderTarget(uint32_t TargetIndex);
		void DestroyDepthStencil(uint32_t TargetIndex);
		void DestroyCubemapRenderTarget(uint32_t TargetIndex);
	};
	static_assert_offset(Renderer, pRenderTargets, 0xAF0);
	static_assert_offset(Renderer, pDepthStencils, 0x2050);
	static_assert_offset(Renderer, pCubemapRenderTargets, 0x2770);
};

class BSGraphicsRenderTargetManager_CK
{
public:
	BSGraphics_CK::RenderTargetProperties pRenderTargetDataA[RENDER_TARGET_COUNT];
	BSGraphics_CK::DepthStencilTargetProperties pDepthStencilTargetDataA[DEPTH_STENCIL_COUNT];
	BSGraphics_CK::CubeMapRenderTargetProperties pCubeMapRenderTargetDataA[RENDER_TARGET_CUBEMAP_COUNT];

	void CreateRenderTarget(uint32_t TargetIndex, const BSGraphics_CK::RenderTargetProperties *Properties);
	void CreateDepthStencil(uint32_t TargetIndex, const BSGraphics_CK::DepthStencilTargetProperties *Properties);
	void CreateCubemapRenderTarget(uint32_t TargetIndex, const BSGraphics_CK::CubeMapRenderTargetProperties *Properties);

	const char *GetRenderTargetName(uint32_t Index);
	const char *GetDepthStencilName(uint32_t Index);
	const char *GetCubemapRenderTargetName(uint32_t Index);
};
static_assert_offset(BSGraphicsRenderTargetManager_CK, pRenderTargetDataA, 0x0);
static_assert_offset(BSGraphicsRenderTargetManager_CK, pDepthStencilTargetDataA, 0xC78);
static_assert_offset(BSGraphicsRenderTargetManager_CK, pCubeMapRenderTargetDataA, 0xD38);