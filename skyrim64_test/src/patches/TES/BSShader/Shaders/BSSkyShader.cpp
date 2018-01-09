#include "../../../rendering/common.h"
#include "../../../../common.h"
#include "../BSVertexShader.h"
#include "../BSPixelShader.h"
#include "../BSShader.h"
#include "../../BSGraphicsRenderer.h"
#include "BSSkyShader.h"
#include "../../NiMain/NiColor.h"
#include "../../NiMain/NiTransform.h"
#include "../BSShaderUtil.h"
#include "../BSShaderManager.h"
#include "../../NiMain/BSGeometry.h"
#include "../../NiMain/NiSourceTexture.h"
#include "../BSShaderAccumulator.h"

//
// Shader notes:
//
// - Destructor is not implemented
// - A global variable update was manually removed in SetupGeometry()
//
using namespace DirectX;

AutoPtr(float, dword_141E32FBC, 0x1E32FBC);
AutoPtr(__int64, qword_1431F5810, 0x31F5810);
AutoPtr(NiSourceTexture *, qword_143052900, 0x3052900);
AutoPtr(NiSourceTexture *, qword_143052920, 0x3052920);
AutoPtr(NiSourceTexture *, qword_143052928, 0x3052928);
AutoPtr(__int64, qword_1431F55F8, 0x31F55F8);
AutoPtr(float, qword_143257D80, 0x3257D80);

AutoPtr(unsigned int, gameTlsIndex, 0x34BBA78);

BSShaderAccumulator *GetCurrentAccumulator();

void TestHook3()
{
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x13113F0), &BSSkyShader::__ctor__);
}

BSSkyShader::BSSkyShader() : BSShader("Sky")
{
	m_Type = 2;
	pInstance = this;

	xmmword_143257D48 = NiColorA::BLACK;
	xmmword_143257D58 = NiColorA::BLACK;
	xmmword_143257D68 = NiColorA::BLACK;
}

BSSkyShader::~BSSkyShader()
{
	__debugbreak();
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

	*(uint32_t *)(*(uintptr_t *)(__readgsqword(0x58u) + 8i64 * gameTlsIndex) + 2544i64) = rawTechnique;

	BSGraphics::Renderer::DepthStencilStateSetDepthMode(1);

	switch (rawTechnique)
	{
	case RAW_TECHNIQUE_SUNOCCLUDE:
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(0);
		break;

	case RAW_TECHNIQUE_SUNGLARE:
		BSGraphics::Renderer::DepthStencilStateSetDepthMode(0);
		BSGraphics::Renderer::AlphaBlendStateSetMode(2);
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(11);
		break;

	case RAW_TECHNIQUE_MOONANDSTARSMASK:
		BSGraphics::Renderer::SetUseScrapConstantValue(true);
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(0);
		BSGraphics::Renderer::DepthStencilStateSetDepthMode(3);
		BSGraphics::Renderer::RasterStateSetUnknown1(3);
		break;

	case RAW_TECHNIQUE_STARS:
		BSGraphics::Renderer::AlphaBlendStateSetMode(2);
		break;

	case RAW_TECHNIQUE_CLOUDS:
	case RAW_TECHNIQUE_CLOUDSLERP:
	case RAW_TECHNIQUE_CLOUDSFADE:
	case RAW_TECHNIQUE_TEXTURE:
		BSGraphics::Renderer::AlphaBlendStateSetMode(1);
		BSGraphics::Renderer::RasterStateSetUnknown1(2);
		break;

	case RAW_TECHNIQUE_SKY:
		BSGraphics::Renderer::AlphaBlendStateSetMode(1);
		break;
	}

	BSGraphics::Renderer::SetTexture(2, qword_143052928->QRendererTexture());// NoiseGradSampler
	BSGraphics::Renderer::SetTextureMode(2, 3, 0);
	return true;
}

void BSSkyShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSSkyShader::RestoreTechnique, Technique);

	BSGraphics::Renderer::AlphaBlendStateSetMode(0);
	BSGraphics::Renderer::AlphaBlendStateSetUnknown2(1);
	BSGraphics::Renderer::DepthStencilStateSetDepthMode(3);
	BSGraphics::Renderer::SetUseScrapConstantValue(false);
	BSGraphics::Renderer::RasterStateSetUnknown1(0);
	EndTechnique();
}

void BSSkyShader::SetupGeometry(BSRenderPass *Pass, uint32_t Flags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSSkyShader::SetupGeometry, Pass, Flags);

	auto *renderer = GetThreadedGlobals();
	auto vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	uintptr_t shaderProperty = (uintptr_t)Pass->m_Property;
	uint32_t propertyType = *(uint32_t *)(shaderProperty + 192);
	uint32_t tlsRawTechnique = *(uint32_t *)(*(uintptr_t *)(__readgsqword(0x58u) + 8i64 * gameTlsIndex) + 2544i64);

	NiTransform geoTransform = Pass->m_Geometry->GetWorldTransform();

	if (*(bool *)((uintptr_t)GetCurrentAccumulator() + 0x128))
	{
		float v13 = geoTransform.m_Translate.x - *(float *)(qword_1431F55F8 + 160);
		float v14 = geoTransform.m_Translate.y - *(float *)(qword_1431F55F8 + 164);
		float v15 = geoTransform.m_Translate.z - *(float *)(qword_1431F55F8 + 168);
		float *v16 = *(float **)&GetCurrentAccumulator()->m_pkCamera;

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
	vertexCG.ParamVS<XMMATRIX, 0>() = XMMatrixMultiplyTranspose(xmmGeoTransform, *(XMMATRIX *)&renderer->__zz2[240]);
	vertexCG.ParamVS<XMMATRIX, 1>() = XMMatrixTranspose(xmmGeoTransform);

	//
	// VS: p2 float4x4 PreviousWorld
	//
	// NOTE: Unlike BSDistantTreeShader and BSGrassShader, this uses GetPreviousWorldTransform() instead
	// of GetWorldTransform()...?
	//
	vertexCG.ParamVS<XMMATRIX, 2>() = XMMatrixTranspose(BSShaderUtil::GetXMFromNiPosAdjust(Pass->m_Geometry->GetPreviousWorldTransform(), *(NiPoint3 *)&renderer->__zz2[40]));

	//
	// VS: p4 float3 EyePosition (adjusted to relative coordinates, not world)
	//
	{
		XMFLOAT3& eyePos = vertexCG.ParamVS<XMFLOAT3, 4>();

		float *v27 = (float *)GetCurrentAccumulator();
		eyePos.x = v27[91] - *(float *)&renderer->__zz2[28];
		eyePos.y = v27[92] - *(float *)&renderer->__zz2[32];
		eyePos.z = v27[93] - *(float *)&renderer->__zz2[36];
	}

	//
	// PS: p0 float2 PParams
	//
	{
		XMFLOAT2& pparams = pixelCG.ParamPS<XMFLOAT2, 0>();

		if (propertyType == 3)
		{
			if (tlsRawTechnique == RAW_TECHNIQUE_CLOUDSFADE && !*(bool *)(shaderProperty + 190))
				pparams.x = 1.0f - *(float *)(shaderProperty + 184);
			else
				pparams.x = *(float *)(shaderProperty + 184);

			pparams.y = dword_141E32FBC * *(float *)(qword_1431F5810 + 228);

			// VS: p5 float2 TexCoordOff
			vertexCG.ParamVS<XMFLOAT2, 5>() = *((XMFLOAT2 *)&qword_143257D80 + *(unsigned __int16 *)(shaderProperty + 0xBC));
		}
		else if (propertyType != 6 && propertyType != 1)
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

	switch (propertyType)
	{
	case 0:
	case 1:
	case 3:
	case 5:
	case 6:
	case 7:
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[0], *(const NiColorA *)(shaderProperty + 136));
		//blendColor[1] = _mm_loadzero_ps(); -- Already zeroed
		//blendColor[2] = _mm_loadzero_ps(); -- Already zeroed
		break;

	case 2:
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[0], xmmword_143257D48);
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[1], xmmword_143257D58);
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[2], xmmword_143257D68);
		break;

	default:
		BSGraphics::Utility::CopyNiColorAToFloat(&blendColor[0], NiColorA::BLACK);
		//blendColor[1] = _mm_loadzero_ps(); -- Already zeroed
		//blendColor[2] = _mm_loadzero_ps(); -- Already zeroed
		break;
	}

	BSGraphics::Renderer::FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	switch (tlsRawTechnique)
	{
	case RAW_TECHNIQUE_SUNOCCLUDE:
	case RAW_TECHNIQUE_STARS:
	{
		NiSourceTexture *baseSamplerTex = *(NiSourceTexture **)(shaderProperty + 152);

		if (!baseSamplerTex)
			baseSamplerTex = qword_143052900;

		BSGraphics::Renderer::SetTexture(0, baseSamplerTex->QRendererTexture());// BaseSampler
		BSGraphics::Renderer::SetTextureMode(0, 3, 1);
	}
	break;

	case RAW_TECHNIQUE_SUNGLARE:
	case RAW_TECHNIQUE_MOONANDSTARSMASK:
	case RAW_TECHNIQUE_CLOUDS:
	case RAW_TECHNIQUE_CLOUDSLERP:
	case RAW_TECHNIQUE_TEXTURE:
	{
		NiSourceTexture *baseSamplerTex = *(NiSourceTexture **)(shaderProperty + 152);

		if (!baseSamplerTex)
			baseSamplerTex = qword_143052920;

		BSGraphics::Renderer::SetTexture(0, baseSamplerTex->QRendererTexture());// BaseSampler
		BSGraphics::Renderer::SetTextureMode(0, 3, 1);

		if (tlsRawTechnique == RAW_TECHNIQUE_CLOUDSLERP)
		{
			NiSourceTexture *blendSamplerTex = *(NiSourceTexture **)(shaderProperty + 160);

			if (!blendSamplerTex)
			{
				blendSamplerTex = *(NiSourceTexture **)(shaderProperty + 152);

				if (!blendSamplerTex)
					break;
			}

			BSGraphics::Renderer::SetTexture(1, blendSamplerTex->QRendererTexture());// BlendSampler
			BSGraphics::Renderer::SetTextureMode(1, 3, 1);
		}
	}
	break;

	case RAW_TECHNIQUE_CLOUDSFADE:
	{
		NiSourceTexture *baseSamplerTex = *(NiSourceTexture **)(shaderProperty + 152);

		if (*(bool *)(shaderProperty + 190))
			baseSamplerTex = *(NiSourceTexture **)(shaderProperty + 160);

		if (!baseSamplerTex)
			baseSamplerTex = qword_143052920;

		BSGraphics::Renderer::SetTexture(0, baseSamplerTex->QRendererTexture());// BaseSampler
		BSGraphics::Renderer::SetTextureMode(0, 3, 1);
	}
	break;
	}

	if (propertyType == 0 || propertyType == 6)
		BSGraphics::Renderer::AlphaBlendStateSetMode(2);
}

void BSSkyShader::RestoreGeometry(BSRenderPass *Pass)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSSkyShader::RestoreGeometry, Pass);

	uint32_t propertyType = *(uint32_t *)((uintptr_t)Pass->m_Property + 0xC0);

	if (propertyType == 0 || propertyType == 6)
		BSGraphics::Renderer::AlphaBlendStateSetMode(1);
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
	}

	// bAssert("BSSkyShader: bad technique ID");
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