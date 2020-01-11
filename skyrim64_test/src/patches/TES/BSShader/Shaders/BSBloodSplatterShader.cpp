#include "../../../../common.h"
#include "../../BSGraphicsState.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSBloodSplatterShaderProperty.h"
#include "BSBloodSplatterShader.h"

DEFINE_SHADER_DESCRIPTOR(
	BloodSplatter,

	// Vertex
	CONFIG_ENTRY(VS, PER_GEO, 0, row_major float4x4,	WorldViewProj)
	CONFIG_ENTRY(VS, PER_GEO, 1, float4,				LightLoc)
	CONFIG_ENTRY(VS, PER_GEO, 2, float,					Ctrl)

	// Pixel
	CONFIG_ENTRY(PS, PER_GEO, 0, float,					Alpha)

	CONFIG_ENTRY(PS, SAMPLER, 0, SamplerState,			SampBloodColor)
	CONFIG_ENTRY(PS, SAMPLER, 1, SamplerState,			SampBloodAlpha)
	CONFIG_ENTRY(PS, SAMPLER, 2, SamplerState,			SampFlareColor)
	CONFIG_ENTRY(PS, SAMPLER, 3, SamplerState,			SampFlareHDR)

	CONFIG_ENTRY(PS, TEXTURE, 0, Texture2D<float4>,		TexBloodColor)
	CONFIG_ENTRY(PS, TEXTURE, 1, Texture2D<float4>,		TexBloodAlpha)
	CONFIG_ENTRY(PS, TEXTURE, 2, Texture2D<float4>,		TexFlareColor)
	CONFIG_ENTRY(PS, TEXTURE, 3, Texture2D<float4>,		TexFlareHDR)
);

//
// Shader notes:
//
// - m_CurrentRawTechnique is an implicit global variable (TODO)
//
using namespace DirectX;
using namespace BSGraphics;

AutoPtr(__int64, qword_14304EF00, 0x304EF00);

BSBloodSplatterShader::BSBloodSplatterShader() : BSShader(ShaderConfigBloodSplatter.Type)
{
	ShaderMetadata[BSShaderManager::BSSM_SHADER_BLOODSPLATTER] = &ShaderConfigBloodSplatter;
	m_Type = BSShaderManager::BSSM_SHADER_BLOODSPLATTER;
	pInstance = this;

	// Added in FO4:
	// BSShaderManager::GetTexture("Textures\\Blood\\FXBloodFlare.dds", 1, &spDefaultFlareTexture, 0, 0, 0);
	// if (!spDefaultFlareTexture && BSShaderManager::pShaderErrorCallback)
	//	BSShaderManager::pShaderErrorCallback("BSBloodSplatterShader: Unable to load Textures\\Blood\\FXBloodFlare.dds", 0);

	LightLoc.Set(0.9f, 0.9f, 0.0f, 0.1f);
}

BSBloodSplatterShader::~BSBloodSplatterShader()
{
	pInstance = nullptr;
}

bool BSBloodSplatterShader::SetupTechnique(uint32_t Technique)
{
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
			renderer->SetTexture(3, BSGraphics::gState.pDefaultHeightMap->QRendererTexture());// FlareHDR
		}
		else
		{
			uintptr_t v9 = *(&qword_14304EF00 + 6 * iAdaptedLightRenderTarget);// BSGraphics::RenderTargetManager::SetTextureRenderTarget

			renderer->SetShaderResource(3, (ID3D11ShaderResourceView *)v9);// FlareHDR
		}

		renderer->SetTextureMode(3, 0, 1);
		renderer->AlphaBlendStateSetMode(5);
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DISABLED);
	} 
	else if (rawTechnique == RAW_TECHNIQUE_SPLATTER)
	{
		renderer->AlphaBlendStateSetMode(4);
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DISABLED);
	}

	m_CurrentRawTechnique = rawTechnique;
	return true;
}

void BSBloodSplatterShader::RestoreTechnique(uint32_t Technique)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	renderer->AlphaBlendStateSetMode(0);
	renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DEFAULT);
	EndTechnique();
}

void BSBloodSplatterShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto property = static_cast<const BSBloodSplatterShaderProperty *>(Pass->m_ShaderProperty);

	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	//
	// PS: p0 float Alpha
	//
	float alpha = property->GetAlpha() * fGlobalAlpha;

	if (m_CurrentRawTechnique == RAW_TECHNIQUE_FLARE)
		alpha *= fFlareMult * fAlpha;

	pixelCG.ParamPS<float, 0>() = alpha;

	//
	// VS: p0 float4x4 WorldViewProj
	//
	XMMATRIX geoTransform = BSShaderUtil::GetXMFromNi(Pass->m_Geometry->GetWorldTransform());
	XMMATRIX worldViewProj = XMMatrixMultiplyTranspose(geoTransform, renderer->m_ViewProjMat);

	vertexCG.ParamVS<XMMATRIX, 0>() = worldViewProj;

	//
	// VS: p1 float4 LightLoc
	// VS: p2 float Ctrl
	//
	BSGraphics::Utility::CopyNiColorAToFloat(&vertexCG.ParamVS<XMVECTOR, 1>(), LightLoc);
	vertexCG.ParamVS<float, 2>() = fFlareOffsetScale;

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	if (m_CurrentRawTechnique == RAW_TECHNIQUE_FLARE)
	{
		renderer->SetTexture(2, property->pFlareColorTexture->QRendererTexture());// FlareColor
		renderer->SetTextureMode(2, 0, 1);
	}
	else if (m_CurrentRawTechnique == RAW_TECHNIQUE_SPLATTER)
	{
		AssertMsgDebug(property->pBloodColorTexture, "Missing texture");
		AssertMsgDebug(property->pBloodAlphaTexture, "Missing texture");

		renderer->SetTexture(0, property->pBloodColorTexture->QRendererTexture());// BloodColor
		renderer->SetTextureMode(0, 0, 1);

		renderer->SetTexture(1, property->pBloodAlphaTexture->QRendererTexture());// BloodAlpha
		renderer->SetTextureMode(1, 0, 1);
	}
}

void BSBloodSplatterShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
}

void BSBloodSplatterShader::CreateAllShaders()
{
	CreatePixelShader(RAW_TECHNIQUE_SPLATTER);
	CreateVertexShader(RAW_TECHNIQUE_SPLATTER);
	CreatePixelShader(RAW_TECHNIQUE_FLARE);
	CreateVertexShader(RAW_TECHNIQUE_FLARE);
}

void BSBloodSplatterShader::CreateVertexShader(uint32_t Technique)
{
	auto getDefines = BSShaderInfo::BSBloodSplatterShader::Defines::GetArray(Technique);
	auto getConstant = [](int i) { return ShaderConfigBloodSplatter.ByConstantIndexVS.count(i) ? ShaderConfigBloodSplatter.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfigBloodSplatter.Type, getDefines, getConstant);
}

void BSBloodSplatterShader::CreatePixelShader(uint32_t Technique)
{
	auto getDefines = BSShaderInfo::BSBloodSplatterShader::Defines::GetArray(Technique);
	auto getSampler = [](int i) { return ShaderConfigBloodSplatter.BySamplerIndex.count(i) ? ShaderConfigBloodSplatter.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfigBloodSplatter.ByConstantIndexPS.count(i) ? ShaderConfigBloodSplatter.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfigBloodSplatter.Type, getDefines, getSampler, getConstant);
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

std::vector<std::pair<const char *, const char *>> BSBloodSplatterShader::GetSourceDefines(uint32_t Technique)
{
	// FIXME
	return BSShaderInfo::BSBloodSplatterShader::Defines::GetArray(Technique);
}