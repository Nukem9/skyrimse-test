#include "../../../../common.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSParticleShader.h"

using namespace DirectX;
using namespace BSGraphics;

DEFINE_SHADER_DESCRIPTOR(
	Particle,

	// Vertex
	CONFIG_ENTRY(VS, PER_TEC, 13, float4,				ScaleAdjust)

	CONFIG_ENTRY(VS, PER_GEO, 0, row_major float4x4,	WorldViewProj)
	CONFIG_ENTRY(VS, PER_GEO, 1, row_major float4x4,	PrevWorldViewProj)
	CONFIG_ENTRY(VS, PER_GEO, 2, row_major float4x4,	PrecipitationOcclusionWorldViewProj)
	CONFIG_ENTRY(VS, PER_GEO, 3, float4,				fVars0)
	CONFIG_ENTRY(VS, PER_GEO, 4, float4,				fVars1)
	CONFIG_ENTRY(VS, PER_GEO, 5, float4,				fVars2)
	CONFIG_ENTRY(VS, PER_GEO, 6, float4,				fVars3)
	CONFIG_ENTRY(VS, PER_GEO, 7, float4,				fVars4)
	CONFIG_ENTRY(VS, PER_GEO, 8, float4,				Color1)
	CONFIG_ENTRY(VS, PER_GEO, 9, float4,				Color2)
	CONFIG_ENTRY(VS, PER_GEO, 10, float4,				Color3)
	CONFIG_ENTRY(VS, PER_GEO, 11, float4,				Velocity)
	CONFIG_ENTRY(VS, PER_GEO, 12, float4,				Acceleration)
	CONFIG_ENTRY(VS, PER_GEO, 14, float4,				Wind)

	// Pixel
	CONFIG_ENTRY(PS, PER_GEO, 0, float4,				ColorScale)
	CONFIG_ENTRY(PS, PER_GEO, 1, float4,				TextureSize)

	CONFIG_ENTRY(PS, SAMPLER, 0, SamplerState,			SampSourceTexture)
	CONFIG_ENTRY(PS, SAMPLER, 1, SamplerState,			SampGrayscaleTexture)
	CONFIG_ENTRY(PS, SAMPLER, 2, SamplerState,			SampPrecipitationOcclusionTexture)
	CONFIG_ENTRY(PS, SAMPLER, 3, SamplerState,			SampUnderwaterMask)

	CONFIG_ENTRY(PS, TEXTURE, 0, Texture2D<float4>,		TexSourceTexture)
	CONFIG_ENTRY(PS, TEXTURE, 1, Texture2D<float4>,		TexGrayscaleTexture)
	CONFIG_ENTRY(PS, TEXTURE, 2, Texture2D<float4>,		TexPrecipitationOcclusionTexture)
	CONFIG_ENTRY(PS, TEXTURE, 3, Texture2D<float4>,		TexUnderwaterMask)
);

//
// Shader notes:
//
// - SetupGeometry is unimplemented
//
AutoPtr(float, flt_141E357A0, 0x1E357A0);
AutoPtr(uint32_t, dword_143051B3C, 0x3051B3C);
AutoPtr(uint32_t, dword_143051B40, 0x3051B40);
AutoPtr(uint32_t, dword_1434BA458, 0x34BA458);
AutoPtr(uint32_t, dword_141E33048, 0x1E33048);

BSParticleShader::BSParticleShader() : BSShader(ShaderConfigParticle.Type)
{
	ShaderMetadata[BSShaderManager::BSSM_SHADER_PARTICLE] = &ShaderConfigParticle;
	m_Type = BSShaderManager::BSSM_SHADER_PARTICLE;
	pInstance = this;
}

BSParticleShader::~BSParticleShader()
{
	pInstance = nullptr;
}

bool BSParticleShader::SetupTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSParticleShader::SetupTechnique, Technique);

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTechnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTechnique, false))
		return false;

	auto *renderer = Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

	dword_1434BA458 = rawTechnique;

	if (rawTechnique == RAW_TECHNIQUE_ENVCUBESNOW || rawTechnique == RAW_TECHNIQUE_ENVCUBERAIN)
	{
		//sub_140D74380((__int64)renderTargetManager, 1u, 85, 3, 1);// RENDER_TARGET_TEMPORAL_AA_MASK
	}
	else
	{
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_TEST);
	}

	// VS: p13 float4 ScaleAdjust
	{
		XMVECTORF32& scaleAdjust = vertexCG.ParamVS<XMVECTORF32, 13>();

		scaleAdjust.f[0] = 1.0f / flt_141E357A0;
		scaleAdjust.f[1] = ((float)dword_143051B3C / (float)dword_143051B40) / flt_141E357A0;
		scaleAdjust.f[2] = 1.0f;
		scaleAdjust.f[3] = 1.0f;
	}

	renderer->FlushConstantGroupVSPS(&vertexCG, nullptr);
	renderer->ApplyConstantGroupVSPS(&vertexCG, nullptr, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	return true;
}

void BSParticleShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSParticleShader::RestoreTechnique, Technique);

	if (dword_1434BA458 == RAW_TECHNIQUE_ENVCUBESNOW || dword_1434BA458 == RAW_TECHNIQUE_ENVCUBERAIN)
	{
		//sub_140D74380((__int64)renderTargetManager, 1u, 85, 3, 1);// RENDER_TARGET_TEMPORAL_AA_MASK
	}
	else
		Renderer::GetGlobals()->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_TEST_WRITE);

	BSShader::EndTechnique();
}

void BSParticleShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL_ALWAYS(GEOMETRY, &BSParticleShader::SetupGeometry, Pass, RenderFlags);
}

void BSParticleShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSParticleShader::RestoreGeometry, Pass, RenderFlags);

	auto *renderer = Renderer::GetGlobals();
	BSShaderProperty *property = Pass->m_Property;

	if (!property->GetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_TEST))
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DEFAULT);
	
	if (*(uintptr_t *)((uintptr_t)property + 0x190))
	{
		renderer->SetUseAlphaTestRef(false);
		renderer->AlphaBlendStateSetMode(0);
		renderer->RasterStateSetCullMode(RASTER_STATE_CULL_MODE_DEFAULT);
	}

	if ((dword_1434BA458 == RAW_TECHNIQUE_ENVCUBESNOW || dword_1434BA458 == RAW_TECHNIQUE_ENVCUBERAIN) && dword_141E33048 == 2)
		renderer->DepthStencilStateSetStencilMode(DEPTH_STENCIL_STENCIL_MODE_DEFAULT, 255);
}

void BSParticleShader::CreateAllShaders()
{
	CreatePixelShader(RAW_TECHNIQUE_PARTICLES);
	CreateVertexShader(RAW_TECHNIQUE_PARTICLES);
	CreatePixelShader(RAW_TECHNIQUE_PARTICLES_GRYCOLOR);
	CreateVertexShader(RAW_TECHNIQUE_PARTICLES_GRYCOLOR);
	CreatePixelShader(RAW_TECHNIQUE_PARTICLES_GRYALPHA);
	CreateVertexShader(RAW_TECHNIQUE_PARTICLES_GRYALPHA);
	CreatePixelShader(RAW_TECHNIQUE_PARTICLES_GRYCOLORALPHA);
	CreateVertexShader(RAW_TECHNIQUE_PARTICLES_GRYCOLORALPHA);
	CreatePixelShader(RAW_TECHNIQUE_ENVCUBESNOW);
	CreateVertexShader(RAW_TECHNIQUE_ENVCUBESNOW);
	CreatePixelShader(RAW_TECHNIQUE_ENVCUBERAIN);
	CreateVertexShader(RAW_TECHNIQUE_ENVCUBERAIN);
}

void BSParticleShader::CreateVertexShader(uint32_t Technique)
{
	auto getDefines = BSShaderInfo::BSParticleShader::Defines::GetArray(Technique);
	auto getConstant = [](int i) { return ShaderConfigParticle.ByConstantIndexVS.count(i) ? ShaderConfigParticle.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfigParticle.Type, getDefines, getConstant);
}

void BSParticleShader::CreatePixelShader(uint32_t Technique)
{
	auto getDefines = BSShaderInfo::BSParticleShader::Defines::GetArray(Technique);
	auto getSampler = [](int i) { return ShaderConfigParticle.BySamplerIndex.count(i) ? ShaderConfigParticle.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfigParticle.ByConstantIndexPS.count(i) ? ShaderConfigParticle.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfigParticle.Type, getDefines, getSampler, getConstant);
}

uint32_t BSParticleShader::GetRawTechnique(uint32_t Technique)
{
	switch (Technique)
	{
	case BSSM_PARTICLE:return RAW_TECHNIQUE_PARTICLES;
	case BSSM_PARTICLE_GRYCOLORALPHA:return RAW_TECHNIQUE_PARTICLES_GRYCOLORALPHA;
	case BSSM_PARTICLE_GRYCOLOR:return RAW_TECHNIQUE_PARTICLES_GRYCOLOR;
	case BSSM_PARTICLE_GRYALPHA:return RAW_TECHNIQUE_PARTICLES_GRYALPHA;
	case BSSM_ENVCUBESNOWPARTICLE:return RAW_TECHNIQUE_ENVCUBESNOW;
	case BSSM_ENVCUBERAINPARTICLE:return RAW_TECHNIQUE_ENVCUBERAIN;
	}

	AssertMsg(false, "BSParticleShader: bad technique ID");
	return 0;
}

uint32_t BSParticleShader::GetVertexTechnique(uint32_t RawTechnique)
{
	if (RawTechnique == RAW_TECHNIQUE_PARTICLES_GRYCOLOR ||
		RawTechnique == RAW_TECHNIQUE_PARTICLES_GRYALPHA ||
		RawTechnique == RAW_TECHNIQUE_PARTICLES_GRYCOLORALPHA)
		return 0;

	return RawTechnique;
}

uint32_t BSParticleShader::GetPixelTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}