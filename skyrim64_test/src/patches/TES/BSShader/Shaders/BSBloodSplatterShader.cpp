#include "../../../rendering/common.h"
#include "../../../../common.h"
#include "../../NiMain/BSGeometry.h"
#include "../../BSGraphicsRenderer.h"
#include "../BSShaderManager.h"
#include "BSBloodSplatterShader.h"
#include "../BSShaderUtil.h"
#include "../../NiMain/NiTexture.h"

//
// Shader notes:
//
// - m_CurrentRawTechnique is an implicit global variable (TODO)
// - ~BSBloodSplatterShader() not yet implemented
//
using namespace DirectX;

AutoPtr(uintptr_t, qword_143052900, 0x52900);
AutoPtr(__int64, qword_14304EF00, 0x4EF00);

void TestHook1()
{
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12EF750), &BSBloodSplatterShader::__ctor__);
}

BSBloodSplatterShader::BSBloodSplatterShader() : BSShader("BloodSplatter")
{
	m_Type = 4;

	// Added in FO4:
	// BSShaderManager::GetTexture("Textures\\Blood\\FXBloodFlare.dds", 1, &spDefaultFlareTexture, 0, 0, 0);
	// if (!spDefaultFlareTexture && BSShaderManager::pShaderErrorCallback)
	//	BSShaderManager::pShaderErrorCallback("BSBloodSplatterShader: Unable to load Textures\\Blood\\FXBloodFlare.dds", 0);

	LightLoc.Set(0.9f, 0.9f, 0.0f, 0.1f);
	pInstance = this;
}

BSBloodSplatterShader::~BSBloodSplatterShader()
{
	__debugbreak();
}

bool BSBloodSplatterShader::SetupTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSBloodSplatterShader::SetupTechnique, Technique);

	// Converts technique with runtime flags to actual technique flags during shader load
	uint32_t rawTechnique;

	if (Technique == BSSM_BLOOD_SPLATTER_FLARE)
		rawTechnique = RAW_TECHNIQUE_FLARE;
	else
		rawTechnique = RAW_TECHNIQUE_SPLATTER;

	// Check if shaders exist
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTecnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTecnique, false))
		return false;

	if (rawTechnique == RAW_TECHNIQUE_FLARE)
	{
		// Use the sun or nearest light source to draw a water-like "shine reflection" from blood
		if (iAdaptedLightRenderTarget <= 0)
		{
			NiTexture *flareHDRTexture = *(NiTexture **)(qword_143052900 + 72);

			BSGraphics::Renderer::SetShaderResource(3, flareHDRTexture ? flareHDRTexture->QRendererTexture() : nullptr);
		}
		else
		{
			uintptr_t v9 = *(&qword_14304EF00 + 6 * iAdaptedLightRenderTarget);// BSGraphics::RenderTargetManager::SetTextureRenderTarget

			BSGraphics::Renderer::SetShaderResource(3, (ID3D11ShaderResourceView *)v9);
		}

		BSGraphics::Renderer::SetTextureMode(3, 0, 1);
		BSGraphics::Renderer::AlphaBlendStateSetMode(5);
		BSGraphics::Renderer::DepthStencilStateSetDepthMode(0);
	}
	else if (rawTechnique == RAW_TECHNIQUE_SPLATTER)
	{
		BSGraphics::Renderer::AlphaBlendStateSetMode(4);
		BSGraphics::Renderer::DepthStencilStateSetDepthMode(0);
	}

	m_CurrentRawTechnique = rawTechnique;
	return true;
}

void BSBloodSplatterShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSBloodSplatterShader::RestoreTechnique, Technique);

	BSGraphics::Renderer::AlphaBlendStateSetMode(0);
	BSGraphics::Renderer::DepthStencilStateSetDepthMode(3);
	EndTechnique();
}

void BSBloodSplatterShader::SetupGeometry(BSRenderPass *Pass, uint32_t Flags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSBloodSplatterShader::SetupGeometry, Pass, Flags);

	auto *renderer = GetThreadedGlobals();

	BSVertexShader *vs = renderer->m_CurrentVertexShader;
	BSPixelShader *ps = renderer->m_CurrentPixelShader;

	BSGraphics::ConstantGroup vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(vs, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	BSGraphics::ConstantGroup pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(ps, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	XMMATRIX geoTransform = BSShaderUtil::GetXMFromNi(Pass->m_Geometry->GetWorldTransform());
	XMMATRIX worldViewProj = XMMatrixMultiplyTranspose(geoTransform, *(XMMATRIX *)&renderer->__zz2[240]);

	uintptr_t v12 = (uintptr_t)Pass->m_Property;

	// BSBloodSplatterShaderProperty::GetAlpha() * BSShaderProperty::GetAlpha() * fGlobalAlpha?
	float alpha = (**(float **)(v12 + 168) * *(float *)(v12 + 48)) * fGlobalAlpha;

	if (m_CurrentRawTechnique == RAW_TECHNIQUE_FLARE)
		alpha *= fFlareMult * fAlpha;

	//
	// PS: p0 float Alpha
	//
	pixelCG.Param<float, 0>(ps) = alpha;

	//
	// VS: p0 float4x4 WorldViewProj
	// VS: p1 float4 LightLoc
	// VS: p2 float Ctrl
	//
	vertexCG.Param<XMMATRIX, 0>(vs)	= worldViewProj;
	vertexCG.Param<XMVECTOR, 1>(vs)	= LightLoc.XmmVector();
	vertexCG.Param<float, 2>(vs)	= fFlareOffsetScale;

	BSGraphics::Renderer::FlushConstantGroup(&vertexCG);
	BSGraphics::Renderer::FlushConstantGroup(&pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	if (m_CurrentRawTechnique == RAW_TECHNIQUE_FLARE)
	{
		NiTexture *flareColorTexture = *(NiTexture **)(*(uintptr_t *)(v12 + 152) + 72i64);

		BSGraphics::Renderer::SetShaderResource(2, flareColorTexture ? flareColorTexture->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureMode(2, 0, 1);
	}
	else
	{
		NiTexture *bloodColorTexture = *(NiTexture **)(*(uintptr_t *)(v12 + 136) + 72i64);

		BSGraphics::Renderer::SetShaderResource(0, bloodColorTexture ? bloodColorTexture->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureMode(0, 0, 1);

		NiTexture *bloodAlphaTexture = *(NiTexture **)(*(uintptr_t *)(v12 + 144) + 72i64);

		BSGraphics::Renderer::SetShaderResource(1, bloodAlphaTexture ? bloodAlphaTexture->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureMode(1, 0, 1);
	}
}

void BSBloodSplatterShader::RestoreGeometry(BSRenderPass *Pass)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSBloodSplatterShader::RestoreGeometry, Pass);
}

uint32_t BSBloodSplatterShader::GetVertexTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}

uint32_t BSBloodSplatterShader::GetPixelTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}