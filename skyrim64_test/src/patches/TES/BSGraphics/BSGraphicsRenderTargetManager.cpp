#include "../../../common.h"
#include "BSGraphicsRenderTargetManager.h"
#include "BSGraphicsRenderer.h"

namespace BSGraphics
{
	void RenderTargetManager::CreateRenderTarget(uint32_t TargetIndex, const RenderTargetProperties *Properties)
	{
		AssertMsg(TargetIndex < RENDER_TARGET_COUNT && TargetIndex != RENDER_TARGET_NONE, "Wrong target index");
		AssertMsg(TargetIndex != RENDER_TARGET_FRAMEBUFFER, "Framebuffer properties come from the renderer");

		BSGraphics::Renderer::QInstance()->DestroyRenderTarget(TargetIndex);
		pRenderTargetDataA[TargetIndex] = *Properties;
		BSGraphics::Renderer::QInstance()->CreateRenderTarget(TargetIndex, GetRenderTargetName(TargetIndex), Properties);
	}

	void RenderTargetManager::CreateDepthStencil(uint32_t TargetIndex, const DepthStencilTargetProperties *Properties)
	{
		AssertMsg(TargetIndex < DEPTH_STENCIL_COUNT && TargetIndex != DEPTH_STENCIL_TARGET_NONE, "Wrong target index");

		BSGraphics::Renderer::QInstance()->DestroyDepthStencil(TargetIndex);
		pDepthStencilTargetDataA[TargetIndex] = *Properties;
		BSGraphics::Renderer::QInstance()->CreateDepthStencil(TargetIndex, GetDepthStencilName(TargetIndex), Properties);
	}

	void RenderTargetManager::CreateCubemapRenderTarget(uint32_t TargetIndex, const CubeMapRenderTargetProperties *Properties)
	{
		AssertMsg(TargetIndex < RENDER_TARGET_CUBEMAP_COUNT && TargetIndex != RENDER_TARGET_CUBEMAP_NONE, "Wrong target index");

		BSGraphics::Renderer::QInstance()->DestroyCubemapRenderTarget(TargetIndex);
		pCubeMapRenderTargetDataA[TargetIndex] = *Properties;
		BSGraphics::Renderer::QInstance()->CreateCubemapRenderTarget(TargetIndex, GetCubemapRenderTargetName(TargetIndex), Properties);
	}

	void RenderTargetManager::SetCurrentRenderTarget(uint32_t Slot, uint32_t TargetIndex, SetRenderTargetMode Mode, bool UpdateViewport)
	{
		Renderer::QInstance()->SetRenderTarget(Slot, TargetIndex, Mode, UpdateViewport);
	}

	void RenderTargetManager::SetCurrentDepthStencilTarget(uint32_t TargetIndex, SetRenderTargetMode Mode, uint32_t Slice)
	{
		Renderer::QInstance()->SetDepthStencilTarget(TargetIndex, Mode, Slice);
	}

	void RenderTargetManager::SetCurrentCubeMapRenderTarget(uint32_t TargetIndex, SetRenderTargetMode Mode, uint32_t View, bool UpdateViewport)
	{
		Renderer::QInstance()->SetCubeMapRenderTarget(TargetIndex, Mode, View, UpdateViewport);
	}

	uint32_t RenderTargetManager::QCurrentRenderTargetWidth() const
	{
		auto s = Renderer::QInstance()->GetRendererShadowState();

		if (s->m_RenderTargets[0] != RENDER_TARGET_NONE)
			return pRenderTargetDataA[s->m_RenderTargets[0]].uiWidth;

		if (s->m_DepthStencil != DEPTH_STENCIL_TARGET_NONE)
			return pDepthStencilTargetDataA[s->m_DepthStencil].uiWidth;

		return 0;
	}

	uint32_t RenderTargetManager::QCurrentRenderTargetHeight() const
	{
		auto s = Renderer::QInstance()->GetRendererShadowState();

		if (s->m_RenderTargets[0] != RENDER_TARGET_NONE)
			return pRenderTargetDataA[s->m_RenderTargets[0]].uiHeight;

		if (s->m_DepthStencil != DEPTH_STENCIL_TARGET_NONE)
			return pDepthStencilTargetDataA[s->m_DepthStencil].uiHeight;

		return 0;
	}

	uint32_t RenderTargetManager::QCurrentCubeMapRenderTargetWidth() const
	{
		auto s = Renderer::QInstance()->GetRendererShadowState();

		// Game doesn't check for invalid indexes
		if (s->m_CubeMapRenderTarget != RENDER_TARGET_CUBEMAP_NONE)
			return pCubeMapRenderTargetDataA[s->m_CubeMapRenderTarget].uiWidth;

		return 0;
	}

	uint32_t RenderTargetManager::QCurrentCubeMapRenderTargetHeight() const
	{
		auto s = Renderer::QInstance()->GetRendererShadowState();

		// Game doesn't check for invalid indexes
		if (s->m_CubeMapRenderTarget != RENDER_TARGET_CUBEMAP_NONE)
			return pCubeMapRenderTargetDataA[s->m_CubeMapRenderTarget].uiHeight;

		return 0;
	}

	uint32_t RenderTargetManager::QCurrentPlatformRenderTarget() const
	{
		return Renderer::QInstance()->GetRendererShadowState()->m_RenderTargets[0];
	}

	const char *RenderTargetManager::GetRenderTargetName(uint32_t Index)
	{
		Assert(Index < RENDER_TARGET_COUNT);

		return BSShaderRenderTargets::GetRenderTargetName(Index);
	}

	const char *RenderTargetManager::GetDepthStencilName(uint32_t Index)
	{
		Assert(Index < DEPTH_STENCIL_COUNT);

		return BSShaderRenderTargets::GetDepthStencilName(Index);
	}

	const char *RenderTargetManager::GetCubemapRenderTargetName(uint32_t Index)
	{
		Assert(Index < RENDER_TARGET_CUBEMAP_COUNT);

		return BSShaderRenderTargets::GetCubemapName(Index);
	}
}