#include "../../../rendering/common.h"
#include "../../../../common.h"
#include "../../NiMain/NiSourceTexture.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSSkyShaderProperty.h"
#include "BSSkyShader.h"

DEFINE_SHADER_DESCRIPTOR(
	"Sky",

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

AutoPtr(NiSourceTexture *, BSShader_DefHeightMap, 0x3052900);
AutoPtr(NiSourceTexture *, BSShader_DefNormalMap, 0x3052920);
AutoPtr(NiSourceTexture *, BSShader_DitheringNoise, 0x3052928);
AutoPtr(__int64, qword_1431F5810, 0x31F5810);
AutoPtr(float, dword_141E32FBC, 0x1E32FBC);
AutoPtr(__int64, qword_1431F55F8, 0x31F55F8);
AutoPtr(float, qword_143257D80, 0x3257D80);

BSSkyShader::BSSkyShader() : BSShader(ShaderConfig.Type)
{
	m_Type = BSShaderManager::BSSM_SHADER_SKY;
	pInstance = this;

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
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSSkyShader::SetupTechnique, Technique);

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTechnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTechnique, false))
		return false;

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_TEST);

	GAME_TLS(uint32_t, 0x9F0) = rawTechnique;

	switch (rawTechnique)
	{
	case RAW_TECHNIQUE_SUNOCCLUDE:
		renderer->AlphaBlendStateSetUnknown2(0);
		break;

	case RAW_TECHNIQUE_SUNGLARE:
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DISABLED);
		renderer->AlphaBlendStateSetMode(2);
		renderer->AlphaBlendStateSetUnknown2(11);
		break;

	case RAW_TECHNIQUE_MOONANDSTARSMASK:
		renderer->SetUseAlphaTestRef(true);
		renderer->AlphaBlendStateSetUnknown2(0);
		renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_TEST_WRITE);
		renderer->RasterStateSetUnknown1(3);
		break;

	case RAW_TECHNIQUE_STARS:
		renderer->AlphaBlendStateSetMode(2);
		break;

	case RAW_TECHNIQUE_CLOUDS:
	case RAW_TECHNIQUE_CLOUDSLERP:
	case RAW_TECHNIQUE_CLOUDSFADE:
	case RAW_TECHNIQUE_TEXTURE:
		renderer->AlphaBlendStateSetMode(1);
		renderer->RasterStateSetUnknown1(2);
		break;

	case RAW_TECHNIQUE_SKY:
		renderer->AlphaBlendStateSetMode(1);
		break;
	}

	renderer->SetTexture(2, BSShader_DitheringNoise->QRendererTexture());// NoiseGradSampler
	renderer->SetTextureMode(2, 3, 0);
	return true;
}

void BSSkyShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSSkyShader::RestoreTechnique, Technique);

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	renderer->AlphaBlendStateSetMode(0);
	renderer->AlphaBlendStateSetUnknown2(1);
	renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DEFAULT);
	renderer->SetUseAlphaTestRef(false);
	renderer->RasterStateSetUnknown1(0);
	EndTechnique();
}

void BSSkyShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSSkyShader::SetupGeometry, Pass, RenderFlags);

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto property = static_cast<const BSSkyShaderProperty *>(Pass->m_Property);

	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	uint32_t rawTechnique = GAME_TLS(uint32_t, 0x9F0);
	NiTransform geoTransform = Pass->m_Geometry->GetWorldTransform();

	if (*(bool *)((uintptr_t)BSShaderManager::GetCurrentAccumulator() + 0x128))
	{
		float v13 = geoTransform.m_Translate.x - *(float *)(qword_1431F55F8 + 160);
		float v14 = geoTransform.m_Translate.y - *(float *)(qword_1431F55F8 + 164);
		float v15 = geoTransform.m_Translate.z - *(float *)(qword_1431F55F8 + 168);
		float *v16 = *(float **)&BSShaderManager::GetCurrentAccumulator()->m_pkCamera;

		if (v16)
		{
			v13 += v16[40];
			v14 += v16[41];
			v15 += v16[42];
		}

		geoTransform.m_Translate.x = v13;
		geoTransform.m_Translate.y = v14;
		geoTransform.m_Translate.z = v15;
	}

	//
	// VS: p0 float4x4 WorldViewProj
	// VS: p1 float4x4 World
	//
	XMMATRIX xmmGeoTransform = BSShaderUtil::GetXMFromNi(geoTransform);
	vertexCG.ParamVS<XMMATRIX, 0>() = XMMatrixMultiplyTranspose(xmmGeoTransform, renderer->m_ViewProjMat);
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

		float *v27 = (float *)BSShaderManager::GetCurrentAccumulator();
		eyePos.x = v27[91] - renderer->m_CurrentPosAdjust.x;
		eyePos.y = v27[92] - renderer->m_CurrentPosAdjust.y;
		eyePos.z = v27[93] - renderer->m_CurrentPosAdjust.z;
	}

	//
	// PS: p0 float2 PParams
	//
	{
		XMFLOAT2& pparams = pixelCG.ParamPS<XMFLOAT2, 0>();

		if (property->uiSkyObjectType == 3)
		{
			if (rawTechnique == RAW_TECHNIQUE_CLOUDSFADE && !property->bUnknown)
				pparams.x = 1.0f - property->fBlendValue;
			else
				pparams.x = property->fBlendValue;

			pparams.y = dword_141E32FBC * *(float *)(qword_1431F5810 + 228);

			// VS: p5 float2 TexCoordOff
			vertexCG.ParamVS<XMFLOAT2, 5>() = *((XMFLOAT2 *)&qword_143257D80 + property->usUnknown);
		}
		else if (property->uiSkyObjectType != 6 && property->uiSkyObjectType != 1)
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
	case 0:
	case 1:
	case 3:
	case 5:
	case 6:
	case 7:
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[0], property->kBlendColor);
		//blendColor[1] = _mm_loadzero_ps(); -- Already zeroed
		//blendColor[2] = _mm_loadzero_ps(); -- Already zeroed
		break;

	case 2:
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
			baseSamplerTex = BSShader_DefHeightMap;

		renderer->SetTexture(0, baseSamplerTex->QRendererTexture());// BaseSampler
		renderer->SetTextureMode(0, 3, 1);
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
			baseSamplerTex = BSShader_DefNormalMap;

		renderer->SetTexture(0, baseSamplerTex->QRendererTexture());// BaseSampler
		renderer->SetTextureMode(0, 3, 1);

		if (rawTechnique == RAW_TECHNIQUE_CLOUDSLERP)
		{
			NiSourceTexture *blendSamplerTex = property->pBlendTexture;

			if (!blendSamplerTex)
			{
				blendSamplerTex = property->pBaseTexture;

				if (!blendSamplerTex)
					break;
			}

			renderer->SetTexture(1, blendSamplerTex->QRendererTexture());// BlendSampler
			renderer->SetTextureMode(1, 3, 1);
		}
	}
	break;

	case RAW_TECHNIQUE_CLOUDSFADE:
	{
		NiSourceTexture *baseSamplerTex = property->pBaseTexture;

		if (property->bUnknown)
			baseSamplerTex = property->pBlendTexture;

		if (!baseSamplerTex)
			baseSamplerTex = BSShader_DefNormalMap;

		renderer->SetTexture(0, baseSamplerTex->QRendererTexture());// BaseSampler
		renderer->SetTextureMode(0, 3, 1);
	}
	break;
	}

	if (property->uiSkyObjectType == 0 || property->uiSkyObjectType == 6)
		renderer->AlphaBlendStateSetMode(2);
}

void BSSkyShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSSkyShader::RestoreGeometry, Pass, RenderFlags);

	uint32_t skyObjectType = static_cast<const BSSkyShaderProperty *>(Pass->m_Property)->uiSkyObjectType;

	if (skyObjectType == 0 || skyObjectType == 6)
		BSGraphics::Renderer::GetGlobals()->AlphaBlendStateSetMode(1);
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
	auto getDefines = BSShaderInfo::BSSkyShader::Defines::GetArray(Technique);
	auto getConstant = [](int i) { return ShaderConfig.ByConstantIndexVS.count(i) ? ShaderConfig.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfig.Type, getDefines, getConstant);
}

void BSSkyShader::CreatePixelShader(uint32_t Technique)
{
	auto getDefines = BSShaderInfo::BSSkyShader::Defines::GetArray(Technique);
	auto getSampler = [](int i) { return ShaderConfig.BySamplerIndex.count(i) ? ShaderConfig.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfig.ByConstantIndexPS.count(i) ? ShaderConfig.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfig.Type, getDefines, getSampler, getConstant);
}

uint32_t BSSkyShader::GetRawTechnique(uint32_t Technique)
{
	switch (Technique)
	{
	case BSSM_SKYBASEPRE:return RAW_TECHNIQUE_SUNOCCLUDE;
	case BSSM_SKY:return RAW_TECHNIQUE_SKY;
	case BSSM_SKY_MOON_STARS_MASK:return RAW_TECHNIQUE_MOONANDSTARSMASK;
	case BSSM_SKY_STARS:return RAW_TECHNIQUE_STARS;
	case BSSM_SKY_TEXTURE:return RAW_TECHNIQUE_TEXTURE;
	case BSSM_SKY_CLOUDS:return RAW_TECHNIQUE_CLOUDS;
	case BSSM_SKY_CLOUDSLERP:return RAW_TECHNIQUE_CLOUDSLERP;
	case BSSM_SKY_CLOUDSFADE:return RAW_TECHNIQUE_CLOUDSFADE;
	case BSSM_SKY_SUNGLARE:return RAW_TECHNIQUE_SUNGLARE;

	// The game calls BSSkyShader with AO for some reason. It defaults to 0.
	case BSSM_AMBIENT_OCCLUSION:return 0;
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