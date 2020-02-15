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
// - m_CurrentTechniqueID is an implicit global variable (TODO)
//
using namespace DirectX;

AutoPtr(__int64, qword_14304EF00, 0x304EF00);

BSBloodSplatterShader::BSBloodSplatterShader() : BSShader(ShaderConfigBloodSplatter.Type)
{
	ShaderMetadata[BSShaderManager::BSSM_SHADER_BLOODSPLATTER] = &ShaderConfigBloodSplatter;

	pInstance = this;
	m_Type = BSShaderManager::BSSM_SHADER_BLOODSPLATTER;
	LightLoc.Set(0.9f, 0.9f, 0.0f, 0.1f);
}

BSBloodSplatterShader::~BSBloodSplatterShader()
{
	pInstance = nullptr;
}

bool BSBloodSplatterShader::SetupTechnique(uint32_t Technique)
{
	auto renderer = BSGraphics::Renderer::GetGlobals();

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);

	if (!BeginTechnique(GetVertexTechnique(rawTechnique), GetPixelTechnique(rawTechnique), false))
		return false;

	if (rawTechnique == RAW_TECHNIQUE_FLARE)
	{
		// Use the sun or nearest light source to draw a water-like reflection from blood
		if (iAdaptedLightRenderTarget <= 0)
		{
			renderer->SetTexture(TexSlot::FlareHDR, BSGraphics::gState.pDefaultHeightMap->QRendererTexture());
		}
		else
		{
			uintptr_t v9 = *(&qword_14304EF00 + 6 * iAdaptedLightRenderTarget);// BSGraphics::RenderTargetManager::SetTextureRenderTarget

			renderer->SetShaderResource(TexSlot::FlareHDR, (ID3D11ShaderResourceView *)v9);
		}

		renderer->SetTextureMode(TexSlot::FlareHDR, 0, 1);
		renderer->AlphaBlendStateSetMode(5);
		renderer->DepthStencilStateSetDepthMode(BSGraphics::DEPTH_STENCIL_DEPTH_MODE_DISABLED);
	} 
	else if (rawTechnique == RAW_TECHNIQUE_SPLATTER)
	{
		renderer->AlphaBlendStateSetMode(4);
		renderer->DepthStencilStateSetDepthMode(BSGraphics::DEPTH_STENCIL_DEPTH_MODE_DISABLED);
	}

	m_CurrentTechniqueID = rawTechnique;
	return true;
}

void BSBloodSplatterShader::RestoreTechnique(uint32_t Technique)
{
	auto renderer = BSGraphics::Renderer::GetGlobals();

	renderer->AlphaBlendStateSetMode(0);
	renderer->DepthStencilStateSetDepthMode(BSGraphics::DEPTH_STENCIL_DEPTH_MODE_DEFAULT);
	EndTechnique();
}

void BSBloodSplatterShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	auto renderer = BSGraphics::Renderer::GetGlobals();
	auto property = static_cast<const BSBloodSplatterShaderProperty *>(Pass->m_ShaderProperty);

	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	//
	// PS: p0 float Alpha
	//
	float alpha = property->GetAlpha() * fGlobalAlpha;

	if (m_CurrentTechniqueID == RAW_TECHNIQUE_FLARE)
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

	if (m_CurrentTechniqueID == RAW_TECHNIQUE_FLARE)
	{
		renderer->SetTexture(TexSlot::FlareColor, property->pFlareColorTexture->QRendererTexture());
		renderer->SetTextureMode(TexSlot::FlareColor, 0, 1);
	}
	else if (m_CurrentTechniqueID == RAW_TECHNIQUE_SPLATTER)
	{
		AssertMsgDebug(property->pBloodColorTexture, "Missing texture");
		AssertMsgDebug(property->pBloodAlphaTexture, "Missing texture");

		renderer->SetTexture(TexSlot::BloodColor, property->pBloodColorTexture->QRendererTexture());
		renderer->SetTextureMode(TexSlot::BloodColor, 0, 1);

		renderer->SetTexture(TexSlot::BloodAlpha, property->pBloodAlphaTexture->QRendererTexture());
		renderer->SetTextureMode(TexSlot::BloodAlpha, 0, 1);
	}

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
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
	auto defines = GetSourceDefines(Technique);
	auto getConstant = [](int i) { return ShaderConfigBloodSplatter.ByConstantIndexVS.count(i) ? ShaderConfigBloodSplatter.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfigBloodSplatter.Type, defines, getConstant);
}

void BSBloodSplatterShader::CreatePixelShader(uint32_t Technique)
{
	auto defines = GetSourceDefines(Technique);
	auto getSampler = [](int i) { return ShaderConfigBloodSplatter.BySamplerIndex.count(i) ? ShaderConfigBloodSplatter.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfigBloodSplatter.ByConstantIndexPS.count(i) ? ShaderConfigBloodSplatter.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfigBloodSplatter.Type, defines, getSampler, getConstant);
}

uint32_t BSBloodSplatterShader::GetRawTechnique(uint32_t Technique)
{
	switch (Technique)
	{
	case BSSM_BLOOD_SPLATTER_FLARE: return RAW_TECHNIQUE_FLARE;
	case BSSM_BLOOD_SPLATTER: return RAW_TECHNIQUE_SPLATTER;
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
	std::vector<std::pair<const char *, const char *>> defines;

	switch (Technique)
	{
	case RAW_TECHNIQUE_SPLATTER: defines.emplace_back("SPLATTER", ""); break;
	case RAW_TECHNIQUE_FLARE: defines.emplace_back("FLARE", ""); break;
	default:/* Apparently returns nothing */break;
	}

	return defines;
}

std::string BSBloodSplatterShader::GetTechniqueString(uint32_t Technique)
{
	switch (Technique)
	{
	case RAW_TECHNIQUE_SPLATTER: return "Splatter";
	case RAW_TECHNIQUE_FLARE: return "Flare";
	default: break;
	}

	Assert(false);
	return "";
}