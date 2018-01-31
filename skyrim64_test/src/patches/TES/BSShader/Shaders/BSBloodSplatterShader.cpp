#include "../../../../common.h"
#include "../../NiMain/BSGeometry.h"
#include "../../BSGraphicsRenderer.h"
#include "../BSShaderManager.h"
#include "BSBloodSplatterShader.h"
#include "../BSShaderUtil.h"
#include "../../NiMain/NiSourceTexture.h"

//
// Shader notes:
//
// - Destructor is not implemented
// - m_CurrentRawTechnique is an implicit global variable (TODO)
//
using namespace DirectX;

AutoPtr(NiSourceTexture *, qword_143052900, 0x52900);
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
	Assert(false);
}

bool BSBloodSplatterShader::SetupTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSBloodSplatterShader::SetupTechnique, Technique);

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTecnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTecnique, false))
		return false;

	auto *renderer = BSGraphics::Renderer::GetGlobals();

	if (rawTechnique == RAW_TECHNIQUE_FLARE)
	{
		// Use the sun or nearest light source to draw a water-like reflection from blood
		if (iAdaptedLightRenderTarget <= 0)
		{
			renderer->SetTexture(3, qword_143052900->QRendererTexture());// FlareHDR
		}
		else
		{
			uintptr_t v9 = *(&qword_14304EF00 + 6 * iAdaptedLightRenderTarget);// BSGraphics::RenderTargetManager::SetTextureRenderTarget

			renderer->SetShaderResource(3, (ID3D11ShaderResourceView *)v9);// FlareHDR
		}

		renderer->SetTextureMode(3, 0, 1);
		renderer->AlphaBlendStateSetMode(5);
		renderer->DepthStencilStateSetDepthMode(0);
	} 
	else if (rawTechnique == RAW_TECHNIQUE_SPLATTER)
	{
		renderer->AlphaBlendStateSetMode(4);
		renderer->DepthStencilStateSetDepthMode(0);
	}

	m_CurrentRawTechnique = rawTechnique;
	return true;
}

void BSBloodSplatterShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSBloodSplatterShader::RestoreTechnique, Technique);

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	renderer->AlphaBlendStateSetMode(0);
	renderer->DepthStencilStateSetDepthMode(3);
	EndTechnique();
}

void BSBloodSplatterShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSBloodSplatterShader::SetupGeometry, Pass, RenderFlags);

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

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
	pixelCG.ParamPS<float, 0>() = alpha;

	//
	// VS: p0 float4x4 WorldViewProj
	// VS: p1 float4 LightLoc
	// VS: p2 float Ctrl
	//
	vertexCG.ParamVS<XMMATRIX, 0>()	= worldViewProj;
	vertexCG.ParamVS<XMVECTOR, 1>()	= LightLoc.XmmVector();
	vertexCG.ParamVS<float, 2>()	= fFlareOffsetScale;

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	if (m_CurrentRawTechnique == RAW_TECHNIQUE_FLARE)
	{
		NiSourceTexture *flareColorTexture = *(NiSourceTexture **)(v12 + 152);

		renderer->SetTexture(2, flareColorTexture->QRendererTexture());// FlareColor
		renderer->SetTextureMode(2, 0, 1);
	}
	else
	{
		NiSourceTexture *bloodColorTexture = *(NiSourceTexture **)(v12 + 136);

		renderer->SetTexture(0, bloodColorTexture->QRendererTexture());// BloodColor
		renderer->SetTextureMode(0, 0, 1);

		NiSourceTexture *bloodAlphaTexture = *(NiSourceTexture **)(v12 + 144);

		renderer->SetTexture(1, bloodAlphaTexture->QRendererTexture());// BloodAlpha
		renderer->SetTextureMode(1, 0, 1);
	}
}

void BSBloodSplatterShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSBloodSplatterShader::RestoreGeometry, Pass, RenderFlags);
}

uint32_t BSBloodSplatterShader::GetRawTechnique(uint32_t Technique)
{
	switch (Technique)
	{
	case BSSM_BLOOD_SPLATTER_FLARE:return RAW_TECHNIQUE_FLARE;
	case BSSM_BLOOD_SPLATTER:return RAW_TECHNIQUE_SPLATTER;
	}

	AssertMsg(false, "BSBloodSplatterShader: bad technique ID");
	return 0;
}

uint32_t BSBloodSplatterShader::GetVertexTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}

uint32_t BSBloodSplatterShader::GetPixelTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}