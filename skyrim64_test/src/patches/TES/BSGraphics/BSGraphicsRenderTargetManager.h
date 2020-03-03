#pragma once

#include "BSGraphicsTypes.h"
#include "../BSShader/BSShaderRenderTargets.h"

namespace BSGraphics
{
	class RenderTargetManager
	{
	public:
		RenderTargetProperties pRenderTargetDataA[RENDER_TARGET_COUNT];
		DepthStencilTargetProperties pDepthStencilTargetDataA[DEPTH_STENCIL_COUNT];
		CubeMapRenderTargetProperties pCubeMapRenderTargetDataA[RENDER_TARGET_CUBEMAP_COUNT];

		void CreateRenderTarget(uint32_t TargetIndex, const RenderTargetProperties *Properties);
		void CreateDepthStencil(uint32_t TargetIndex, const DepthStencilTargetProperties *Properties);
		void CreateCubemapRenderTarget(uint32_t TargetIndex, const CubeMapRenderTargetProperties *Properties);

		void SetCurrentRenderTarget(uint32_t Slot, uint32_t TargetIndex, SetRenderTargetMode Mode, bool UpdateViewport);
		void SetCurrentDepthStencilTarget(uint32_t TargetIndex, SetRenderTargetMode Mode, uint32_t Slice);
		void SetCurrentCubeMapRenderTarget(uint32_t TargetIndex, SetRenderTargetMode Mode, uint32_t View, bool UpdateViewport);

		uint32_t QCurrentRenderTargetWidth() const;
		uint32_t QCurrentRenderTargetHeight() const;
		uint32_t QCurrentCubeMapRenderTargetWidth() const;
		uint32_t QCurrentCubeMapRenderTargetHeight() const;
		uint32_t QCurrentPlatformRenderTarget() const;

		static const char *GetRenderTargetName(uint32_t Index);
		static const char *GetDepthStencilName(uint32_t Index);
		static const char *GetCubemapRenderTargetName(uint32_t Index);
	};
	static_assert_offset(RenderTargetManager, pRenderTargetDataA, 0x0);
	static_assert_offset(RenderTargetManager, pDepthStencilTargetDataA, 0xC78);
	static_assert_offset(RenderTargetManager, pCubeMapRenderTargetDataA, 0xD38);

	AutoPtr(RenderTargetManager, gRenderTargetManager, 0x3051B20);
}