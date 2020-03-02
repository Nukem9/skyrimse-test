#include "../../../rendering/common.h"
#include "../../../../common.h"
#include "../../BSGraphics/BSGraphicsUtility.h"
#include "../../BSGraphicsState.h"
#include "../../NiMain/NiNode.h"
#include "../../NiMain/NiCamera.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSSkyShaderProperty.h"
#include "BSSkyShader.h"

DEFINE_SHADER_DESCRIPTOR(
	Sky,

	// Vertex
	CONFIG_ENTRY(VS, PER_GEO, 0, row_major float4x4,	WorldViewProj)
	CONFIG_ENTRY(VS, PER_GEO, 1, row_major float4x4,	World)
	CONFIG_ENTRY(VS, PER_GEO, 2, row_major float4x4,	PreviousWorld)
	CONFIG_ENTRY(VS, PER_GEO, 3, float4[3],				BlendColor)
	CONFIG_ENTRY(VS, PER_GEO, 4, float3,				EyePosition)
	CONFIG_ENTRY(VS, PER_GEO, 5, float2,				TexCoordOff)
	CONFIG_ENTRY(VS, PER_GEO, 6, float,					VParams)

	// Pixel
	CONFIG_ENTRY(PS, PER_GEO, 0, float2,				PParams)

	CONFIG_ENTRY(PS, SAMPLER, 0, SamplerState,			SampBaseSampler)
	CONFIG_ENTRY(PS, SAMPLER, 1, SamplerState,			SampBlendSampler)
	CONFIG_ENTRY(PS, SAMPLER, 2, SamplerState,			SampNoiseGradSampler)

	CONFIG_ENTRY(PS, TEXTURE, 0, Texture2D<float4>,		TexBaseSampler)
	CONFIG_ENTRY(PS, TEXTURE, 1, Texture2D<float4>,		TexBlendSampler)
	CONFIG_ENTRY(PS, TEXTURE, 2, Texture2D<float4>,		TexNoiseGradSampler)
);

//
// Shader notes:
//
// - A global variable update was manually removed in SetupGeometry()
//
using namespace DirectX;
using namespace BSGraphics;

AutoPtr(__int64, qword_1431F5810, 0x31F5810);
AutoPtr(float, dword_141E32FBC, 0x1E32FBC);
AutoPtr(NiNode *, qword_1431F55F8, 0x31F55F8);// Points to "World" node in main SceneGraph
AutoPtr(float, qword_143257D80, 0x3257D80);

BSSkyShader::BSSkyShader() : BSShader(ShaderConfigSky.Type)
{
	ShaderMetadata[BSShaderManager::BSSM_SHADER_SKY] = &ShaderConfigSky;

	pInstance = this;
	m_Type = BSShaderManager::BSSM_SHADER_SKY;

	NightBlendColor0 = NiColorA::BLACK;
	NightBlendColor1 = NiColorA::BLACK;
	NightBlendColor2 = NiColorA::BLACK;
}

BSSkyShader::~BSSkyShader()
{
	pInstance = nullptr;
}

bool BSSkyShader::SetupTechnique(uint32_t Technique)
{
	auto renderer = BSGraphics::Renderer::QInstance();

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);

	if (!BeginTechnique(GetVertexTechnique(rawTechnique), GetPixelTechnique(rawTechnique), false))
		return false;

	renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_TEST);

	GAME_TLS(uint32_t, 0x9F0) = rawTechnique;

	switch (rawTechnique)
	{
	case RAW_TECHNIQUE_SUNOCCLUDE:
		renderer->AlphaBlendStateSetWriteMode(0);
		break;

	case RAW_TECHNIQUE_SUNGLARE:
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DISABLED);
		renderer->AlphaBlendStateSetMode(2);
		renderer->AlphaBlendStateSetWriteMode(11);
		break;

	case RAW_TECHNIQUE_MOONANDSTARSMASK:
		renderer->SetUseAlphaTestRef(true);
		renderer->AlphaBlendStateSetWriteMode(0);
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_TEST_WRITE);
		renderer->RasterStateSetDepthBias(3);
		break;

	case RAW_TECHNIQUE_STARS:
		renderer->AlphaBlendStateSetMode(2);
		break;

	case RAW_TECHNIQUE_CLOUDS:
	case RAW_TECHNIQUE_CLOUDSLERP:
	case RAW_TECHNIQUE_CLOUDSFADE:
	case RAW_TECHNIQUE_TEXTURE:
		renderer->AlphaBlendStateSetMode(1);
		renderer->RasterStateSetDepthBias(2);
		break;

	case RAW_TECHNIQUE_SKY:
		renderer->AlphaBlendStateSetMode(1);
		break;
	}

	renderer->SetTexture(TexSlot::NoiseGrad, BSGraphics::gState.pDefaultTextureDitherNoiseMap->QRendererTexture());
	renderer->SetTextureMode(TexSlot::NoiseGrad, 3, 0);
	return true;
}

void BSSkyShader::RestoreTechnique(uint32_t Technique)
{
	auto renderer = BSGraphics::Renderer::QInstance();

	renderer->AlphaBlendStateSetMode(0);
	renderer->AlphaBlendStateSetWriteMode(1);
	renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DEFAULT);
	renderer->SetUseAlphaTestRef(false);
	renderer->RasterStateSetDepthBias(0);
	EndTechnique();
}

void BSSkyShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	auto renderer = BSGraphics::Renderer::QInstance();
	auto property = static_cast<const BSSkyShaderProperty *>(Pass->m_ShaderProperty);

	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	uint32_t rawTechnique = GAME_TLS(uint32_t, 0x9F0);
	NiTransform geoTransform = Pass->m_Geometry->GetWorldTransform();

	if (auto *accumulator = BSShaderManager::GetCurrentAccumulator(); accumulator->m_1stPerson)
	{
		NiPoint3 adjusted = geoTransform.m_Translate - qword_1431F55F8->GetWorldTranslate();

		if (accumulator->m_pkCamera)
			adjusted += accumulator->m_pkCamera->GetWorldTranslate();

		geoTransform.m_Translate = adjusted;
	}

	//
	// VS: p0 float4x4 WorldViewProj
	// VS: p1 float4x4 World
	//
	XMMATRIX xmmGeoTransform = BSShaderUtil::GetXMFromNi(geoTransform);
	vertexCG.ParamVS<XMMATRIX, 0>() = XMMatrixMultiplyTranspose(xmmGeoTransform, renderer->m_CameraData.m_ViewProjMat);
	vertexCG.ParamVS<XMMATRIX, 1>() = XMMatrixTranspose(xmmGeoTransform);

	//
	// VS: p2 float4x4 PreviousWorld
	//
	// NOTE: Unlike BSDistantTreeShader and BSGrassShader, this uses GetPreviousWorldTransform() instead
	// of GetWorldTransform()...?
	//
	vertexCG.ParamVS<XMMATRIX, 2>() = XMMatrixTranspose(BSShaderUtil::GetXMFromNiPosAdjust(Pass->m_Geometry->GetPreviousWorldTransform(), renderer->m_PreviousPosAdjust));

	//
	// VS: p4 float3 EyePosition (adjusted to relative coordinates, not world)
	//
	{
		XMFLOAT3& eyePos = vertexCG.ParamVS<XMFLOAT3, 4>();

		auto& position = BSShaderManager::GetCurrentAccumulator()->m_EyePosition;
		eyePos.x = position.x - renderer->m_PosAdjust.x;
		eyePos.y = position.y - renderer->m_PosAdjust.y;
		eyePos.z = position.z - renderer->m_PosAdjust.z;
	}

	//
	// PS: p0 float2 PParams
	//
	{
		XMFLOAT2& pparams = pixelCG.ParamPS<XMFLOAT2, 0>();

		if (property->uiSkyObjectType == BSSkyShaderProperty::SO_CLOUDS)
		{
			if (rawTechnique == RAW_TECHNIQUE_CLOUDSFADE && !property->bFadeSecondTexture)
				pparams.x = 1.0f - property->fBlendValue;
			else
				pparams.x = property->fBlendValue;

			pparams.y = dword_141E32FBC * *(float *)(qword_1431F5810 + 228);

			// VS: p5 float2 TexCoordOff
			vertexCG.ParamVS<XMFLOAT2, 5>() = *((XMFLOAT2 *)&qword_143257D80 + property->usCloudLayer);
		}
		else if (property->uiSkyObjectType != BSSkyShaderProperty::SO_MOON && property->uiSkyObjectType != BSSkyShaderProperty::SO_SUN_GLARE)
		{
			pparams.y = dword_141E32FBC * *(float *)(qword_1431F5810 + 228);
		}
		else
		{
			pparams.y = 0.0f;
		}
	}

	//
	// VS: p6 float VParams
	//
	vertexCG.ParamVS<float, 6>() = dword_141E32FBC;

	//
	// VS: p3 float4 BlendColor[3]
	//
	auto& blendColor = vertexCG.ParamVS<XMVECTOR[3], 3>();

	switch (property->uiSkyObjectType)
	{
	case BSSkyShaderProperty::SO_SUN:
	case BSSkyShaderProperty::SO_SUN_GLARE:
	case BSSkyShaderProperty::SO_CLOUDS:
	case BSSkyShaderProperty::SO_STARS:
	case BSSkyShaderProperty::SO_MOON:
	case BSSkyShaderProperty::SO_MOON_SHADOW:
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[0], property->kBlendColor);
		//blendColor[1] = _mm_loadzero_ps(); -- Already zeroed
		//blendColor[2] = _mm_loadzero_ps(); -- Already zeroed
		break;

	case BSSkyShaderProperty::SO_ATMOSPHERE:
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[0], NightBlendColor0);
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[1], NightBlendColor1);
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[2], NightBlendColor2);
		break;

	default:
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[0], NiColorA::BLACK);
		//blendColor[1] = _mm_loadzero_ps(); -- Already zeroed
		//blendColor[2] = _mm_loadzero_ps(); -- Already zeroed
		break;
	}

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	switch (rawTechnique)
	{
	case RAW_TECHNIQUE_SUNOCCLUDE:
	case RAW_TECHNIQUE_STARS:
	{
		NiSourceTexture *baseSamplerTex = property->pBaseTexture;

		if (!baseSamplerTex)
			baseSamplerTex = BSGraphics::gState.pDefaultHeightMap;

		renderer->SetTexture(TexSlot::Base, baseSamplerTex->QRendererTexture());
		renderer->SetTextureMode(TexSlot::Base, 3, 1);
	}
	break;

	case RAW_TECHNIQUE_SUNGLARE:
	case RAW_TECHNIQUE_MOONANDSTARSMASK:
	case RAW_TECHNIQUE_CLOUDS:
	case RAW_TECHNIQUE_CLOUDSLERP:
	case RAW_TECHNIQUE_TEXTURE:
	{
		NiSourceTexture *baseSamplerTex = property->pBaseTexture;

		if (!baseSamplerTex)
			baseSamplerTex = BSGraphics::gState.pDefaultTextureNormalMap;

		renderer->SetTexture(TexSlot::Base, baseSamplerTex->QRendererTexture());
		renderer->SetTextureMode(TexSlot::Base, 3, 1);

		if (rawTechnique == RAW_TECHNIQUE_CLOUDSLERP)
		{
			NiSourceTexture *blendSamplerTex = property->pBlendTexture;

			if (!blendSamplerTex)
				blendSamplerTex = property->pBaseTexture;

			if (blendSamplerTex)
			{
				renderer->SetTexture(TexSlot::Blend, blendSamplerTex->QRendererTexture());
				renderer->SetTextureMode(TexSlot::Blend, 3, 1);
			}
		}
	}
	break;

	case RAW_TECHNIQUE_CLOUDSFADE:
	{
		NiSourceTexture *baseSamplerTex = property->pBaseTexture;

		if (property->bFadeSecondTexture)
			baseSamplerTex = property->pBlendTexture;

		if (!baseSamplerTex)
			baseSamplerTex = BSGraphics::gState.pDefaultTextureNormalMap;

		renderer->SetTexture(TexSlot::Base, baseSamplerTex->QRendererTexture());
		renderer->SetTextureMode(TexSlot::Base, 3, 1);
	}
	break;
	}

	if (property->uiSkyObjectType == BSSkyShaderProperty::SO_SUN || property->uiSkyObjectType == BSSkyShaderProperty::SO_MOON)
		renderer->AlphaBlendStateSetMode(2);
}

void BSSkyShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	auto skyObjectType = static_cast<const BSSkyShaderProperty *>(Pass->m_ShaderProperty)->uiSkyObjectType;

	if (skyObjectType == BSSkyShaderProperty::SO_SUN || skyObjectType == BSSkyShaderProperty::SO_MOON)
		BSGraphics::Renderer::QInstance()->AlphaBlendStateSetMode(1);
}

void BSSkyShader::CreateAllShaders()
{
	static_assert(RAW_TECHNIQUE_SUNOCCLUDE == 0, "Please update this function to match the enum");
	static_assert(RAW_TECHNIQUE_SKY == 8, "Please update this function to match the enum");

	for (int i = RAW_TECHNIQUE_SUNOCCLUDE; i <= RAW_TECHNIQUE_SKY; i++)
	{
		CreateVertexShader(i);
		CreatePixelShader(i);
	}
}

void BSSkyShader::CreateVertexShader(uint32_t Technique)
{
	auto defines = GetSourceDefines(Technique);
	auto getConstant = [](int i) { return ShaderConfigSky.ByConstantIndexVS.count(i) ? ShaderConfigSky.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfigSky.Type, defines, getConstant);
}

void BSSkyShader::CreatePixelShader(uint32_t Technique)
{
	auto defines = GetSourceDefines(Technique);
	auto getSampler = [](int i) { return ShaderConfigSky.BySamplerIndex.count(i) ? ShaderConfigSky.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfigSky.ByConstantIndexPS.count(i) ? ShaderConfigSky.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfigSky.Type, defines, getSampler, getConstant);
}

uint32_t BSSkyShader::GetRawTechnique(uint32_t Technique)
{
	switch (Technique)
	{
	case BSSM_SKYBASEPRE: return RAW_TECHNIQUE_SUNOCCLUDE;
	case BSSM_SKY: return RAW_TECHNIQUE_SKY;
	case BSSM_SKY_MOON_STARS_MASK: return RAW_TECHNIQUE_MOONANDSTARSMASK;
	case BSSM_SKY_STARS: return RAW_TECHNIQUE_STARS;
	case BSSM_SKY_TEXTURE: return RAW_TECHNIQUE_TEXTURE;
	case BSSM_SKY_CLOUDS: return RAW_TECHNIQUE_CLOUDS;
	case BSSM_SKY_CLOUDSLERP: return RAW_TECHNIQUE_CLOUDSLERP;
	case BSSM_SKY_CLOUDSFADE: return RAW_TECHNIQUE_CLOUDSFADE;
	case BSSM_SKY_SUNGLARE: return RAW_TECHNIQUE_SUNGLARE;

	// The game calls BSSkyShader with AO for some reason (lens flare occlusion test?). It defaults to tech 0.
	case BSSM_AMBIENT_OCCLUSION: return RAW_TECHNIQUE_SUNOCCLUDE;
	}

	AssertMsg(false, "BSSkyShader: bad technique ID");
	return 0;
}

uint32_t BSSkyShader::GetVertexTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}

uint32_t BSSkyShader::GetPixelTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}

std::vector<std::pair<const char *, const char *>> BSSkyShader::GetSourceDefines(uint32_t Technique)
{
	std::vector<std::pair<const char *, const char *>> defines;

	switch (Technique)
	{
	case RAW_TECHNIQUE_SUNOCCLUDE: defines.emplace_back("OCCLUSION", ""); break;
	case RAW_TECHNIQUE_SUNGLARE: defines.emplace_back("TEX", ""); defines.emplace_back("DITHER", ""); break;
	case RAW_TECHNIQUE_MOONANDSTARSMASK: defines.emplace_back("TEX", ""); defines.emplace_back("MOONMASK", ""); break;
	case RAW_TECHNIQUE_STARS: defines.emplace_back("HORIZFADE", ""); break;
	case RAW_TECHNIQUE_CLOUDS: defines.emplace_back("TEX", ""); defines.emplace_back("CLOUDS", ""); break;
	case RAW_TECHNIQUE_CLOUDSLERP: defines.emplace_back("TEX", ""); defines.emplace_back("CLOUDS", ""); defines.emplace_back("TEXLERP", ""); break;
	case RAW_TECHNIQUE_CLOUDSFADE: defines.emplace_back("TEX", ""); defines.emplace_back("CLOUDS", ""); defines.emplace_back("TEXFADE", ""); break;
	case RAW_TECHNIQUE_TEXTURE: defines.emplace_back("TEX", ""); break;
	case RAW_TECHNIQUE_SKY: defines.emplace_back("DITHER", ""); break;
	default: Assert(false); break;
	}

	return defines;
}

std::string BSSkyShader::GetTechniqueString(uint32_t Technique)
{
	switch (Technique)
	{
	case RAW_TECHNIQUE_SUNOCCLUDE: return "SunOcclude"; break;
	case RAW_TECHNIQUE_SUNGLARE: return "SunGlare"; break;
	case RAW_TECHNIQUE_MOONANDSTARSMASK: return "MoonAndStarsMask"; break;
	case RAW_TECHNIQUE_STARS: return "Stars"; break;
	case RAW_TECHNIQUE_CLOUDS: return "Clouds"; break;
	case RAW_TECHNIQUE_CLOUDSLERP: return "CloudsLerp"; break;
	case RAW_TECHNIQUE_CLOUDSFADE: return "CloudsFade"; break;
	case RAW_TECHNIQUE_TEXTURE: return "Texture"; break;
	case RAW_TECHNIQUE_SKY: return "Sky"; break;
	}

	Assert(false);
	return "";
}