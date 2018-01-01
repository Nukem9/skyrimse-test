#include "../../../rendering/common.h"
#include "../../../../common.h"
#include "../BSVertexShader.h"
#include "../BSPixelShader.h"
#include "../BSShader.h"
#include "../../BSGraphicsRenderer.h"
#include "../../NiMain/NiColor.h"
#include "../../NiMain/NiTransform.h"
#include "../BSShaderUtil.h"
#include "../BSShaderManager.h"
#include "../../NiMain/BSGeometry.h"
#include "../../NiMain/NiTexture.h"
#include "../BSShaderAccumulator.h"
#include "BSLightingShader.h"

//
// Shader notes:
//
// - None
//
using namespace DirectX;

AutoPtr(uintptr_t, qword_143052900, 0x3052900);
AutoPtr(uintptr_t, qword_1430528A0, 0x30528A0);
AutoPtr(int, dword_143051B3C, 0x3051B3C);
AutoPtr(int, dword_143051B40, 0x3051B40);
AutoPtr(int, dword_141E338A0, 0x1E338A0);
AutoPtr(uintptr_t, qword_14304F260, 0x304F260);
AutoPtr(float, flt_143257C50, 0x3257C50);
AutoPtr(float, flt_143257C54, 0x3257C54);
AutoPtr(float, flt_143257C58, 0x3257C58);
AutoPtr(float, flt_141E32F54, 0x1E32F54);
AutoPtr(float, flt_141E32F58, 0x1E32F58);
AutoPtr(float, flt_141E32F5C, 0x1E32F5C);
AutoPtr(float, flt_141E32F60, 0x1E32F60);
AutoPtr(BYTE, byte_141E32FE0, 0x1E32FE0);
AutoPtr(int, dword_141E33BA0, 0x1E33BA0);
AutoPtr(BYTE, byte_141E32F66, 0x1E32F66);
AutoPtr(float, xmmword_141E3302C, 0x1E3302C);// This is really XMVECTORF32
AutoPtr(float, flt_141E35380, 0x1E35380);
AutoPtr(float, flt_141E35398, 0x1E35398);
AutoPtr(float, flt_141E353B0, 0x1E353B0);
AutoPtr(BYTE, byte_141E353C8, 0x1E353C8);
AutoPtr(XMVECTORF32, xmmword_141E3301C, 0x1E3301C);
AutoPtr(int, dword_141E33040, 0x1E33040);
AutoPtr(uintptr_t, qword_1431F5810, 0x31F5810);
AutoPtr(uintptr_t, qword_143052920, 0x3052920);
AutoPtr(float, flt_141E32FBC, 0x1E32FBC);
AutoPtr(XMVECTORF32, xmmword_14187D940, 0x187D940);
AutoPtr(BYTE, byte_141E32E88, 0x1E32E88);
AutoPtr(BYTE, byte_141E35338, 0x1E35338);
AutoPtr(int, dword_141E353E0, 0x1E353E0);
AutoPtr(uintptr_t, qword_14304EF00, 0x304EF00);
AutoPtr(BYTE, byte_141E32E89, 0x1E32E89);
AutoPtr(BYTE, byte_141E352F0, 0x1E352F0);

char hookbuffer[50];

void TestHook5()
{
	memcpy(hookbuffer, (PBYTE)(g_ModuleBase + 0x1307BD0), 50);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1307BD0), &BSLightingShader::__ctor__);
}

BSLightingShader::BSLightingShader() : BSShader("Lighting")
{
	uintptr_t v1 = *(uintptr_t *)((uintptr_t)this + 0x0);
	uintptr_t v2 = *(uintptr_t *)((uintptr_t)this + 0x10);
	uintptr_t v3 = *(uintptr_t *)((uintptr_t)this + 0x18);

	PatchMemory((g_ModuleBase + 0x1307BD0), (PBYTE)&hookbuffer, 50);

	auto sub_141307BD0 = (uintptr_t(__fastcall *)(BSLightingShader *))(g_ModuleBase + 0x1307BD0);
	sub_141307BD0(this);

	m_Type = 6;
	pInstance = this;

	*(uintptr_t *)((uintptr_t)this + 0x0) = v1;
	*(uintptr_t *)((uintptr_t)this + 0x10) = v2;
	*(uintptr_t *)((uintptr_t)this + 0x18) = v3;

	g_ShaderToggles[6][2] = true;
}

BSLightingShader::~BSLightingShader()
{
	__debugbreak();
}

void UpdateAccelerationConstants(BSVertexShader *Shader, BSGraphics::ConstantGroup& VertexCG)
{
	auto *renderer = GetThreadedGlobals();

	// VS: p12 float4 Acceleration
	BSGraphics::Utility::CopyNiColorAToFloat(&VertexCG.Param<XMVECTOR, 12>(Shader),
		NiColorA(
			flt_141E32F54 - *(float *)&renderer->__zz2[28],
			flt_141E32F58 - *(float *)&renderer->__zz2[32],
			flt_141E32F5C - 15.0f,
			flt_141E32F60 - 15.0f));
}

void UpdateFogWindConstants(BSVertexShader *VertexShader, BSGraphics::ConstantGroup& VertexCG, BSPixelShader *PixelShader, BSGraphics::ConstantGroup& PixelCG)
{
	auto sub_1412AC860 = (uintptr_t(__fastcall *)(BYTE))(g_ModuleBase + 0x12AC860);
	uintptr_t fogParams = sub_1412AC860(byte_141E32FE0);

	if (!fogParams)
		return;

	// Set both vertex & pixel here (incorrect names???)
	{
		XMVECTORF32 wind;
		wind.f[0] = *(float *)(fogParams + 56);
		wind.f[1] = *(float *)(fogParams + 60);
		wind.f[2] = *(float *)(fogParams + 64);
		wind.f[3] = flt_141E32FBC;

		// VS: p14 float4 Wind
		VertexCG.Param<XMVECTORF32, 14>(VertexShader) = wind;

		// PS: p19 float4 FogColor
		PixelCG.Param<XMVECTORF32, 19>(PixelShader) = wind;
	}

	// VS: p15 float4 UNKNOWN_NAME
	XMVECTORF32& UNKNOWN_PARAM = VertexCG.Param<XMVECTORF32, 15>(VertexShader);
	UNKNOWN_PARAM.f[0] = *(float *)(fogParams + 68);
	UNKNOWN_PARAM.f[1] = *(float *)(fogParams + 72);
	UNKNOWN_PARAM.f[2] = *(float *)(fogParams + 76);
	UNKNOWN_PARAM.f[3] = 0.0f;

	// VS: p13 float4 ScaleAdjust
	XMVECTORF32& scaleAdjust = VertexCG.Param<XMVECTORF32, 13>(VertexShader);

	float v5 = *(float *)(fogParams + 84);
	float v6 = *(float *)(fogParams + 80);

	if (v5 != 0.0f || v6 != 0.0f)
	{
		float fogMultiplier = 1.0f / (v5 - v6);

		scaleAdjust.f[0] = fogMultiplier * v6;
		scaleAdjust.f[1] = fogMultiplier;
		scaleAdjust.f[2] = *(float *)(fogParams + 132);
		scaleAdjust.f[3] = *(float *)(fogParams + 136);
	}
	else
	{
		scaleAdjust = xmmword_14187D940;
	}
}

bool BSLightingShader::SetupTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSLightingShader::SetupTechnique, Technique);

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTechnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTechnique, false))
		return false;

	m_CurrentRawTechnique = rawTechnique;

	auto *renderer = GetThreadedGlobals();

	BSVertexShader *vs = renderer->m_CurrentVertexShader;
	BSPixelShader *ps = renderer->m_CurrentPixelShader;

	BSGraphics::ConstantGroup vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(vs, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	BSGraphics::ConstantGroup pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(ps, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

	BSGraphics::Renderer::SetTextureFilterMode(0, 3);
	BSGraphics::Renderer::SetTextureFilterMode(1, 3);

	switch ((rawTechnique >> 24) & 0x3F)
	{
	case RAW_TECHNIQUE_ENVMAP:
	case RAW_TECHNIQUE_EYE:
		BSGraphics::Renderer::SetTextureFilterMode(4, 3);
		BSGraphics::Renderer::SetTextureFilterMode(5, 3);
		break;

	case RAW_TECHNIQUE_GLOWMAP:
		BSGraphics::Renderer::SetTextureFilterMode(6, 3);
		break;

	case RAW_TECHNIQUE_PARALLAX:
	case RAW_TECHNIQUE_PARALLAXOCC:
		BSGraphics::Renderer::SetTextureFilterMode(3, 3);
		break;

	case RAW_TECHNIQUE_FACEGEN:
		BSGraphics::Renderer::SetTextureFilterMode(3, 3);
		BSGraphics::Renderer::SetTextureFilterMode(4, 3);
		BSGraphics::Renderer::SetTextureFilterMode(12, 3);
		break;

	case RAW_TECHNIQUE_MTLAND:
	case RAW_TECHNIQUE_MTLANDLODBLEND:
	{
		// Override all 16 samplers
		for (int i = 0; i < 16; i++)
		{
			NiTexture *v21 = *(NiTexture **)(qword_143052900 + 72);

			BSGraphics::Renderer::SetShaderResource(i, v21 ? v21->QRendererTexture() : nullptr);
			BSGraphics::Renderer::SetTextureMode(i, 3, 3);
		}

		BSGraphics::Renderer::SetTextureMode(15, 3, 1);
	}
	break;

	case RAW_TECHNIQUE_LODLAND:
	case RAW_TECHNIQUE_LODLANDNOISE:
		BSGraphics::Renderer::SetTextureMode(0, 3, 1);
		BSGraphics::Renderer::SetTextureMode(1, 3, 1);
		UpdateAccelerationConstants(vs, vertexCG);
		break;

	case RAW_TECHNIQUE_MULTILAYERPARALLAX:
		BSGraphics::Renderer::SetTextureFilterMode(4, 3);
		BSGraphics::Renderer::SetTextureFilterMode(5, 3);
		BSGraphics::Renderer::SetTextureFilterMode(8, 3);
		break;

	case RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW:
		NiTexture *v15 = *(NiTexture **)(qword_1430528A0 + 72);

		BSGraphics::Renderer::SetShaderResource(10, v15 ? v15->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureMode(10, 3, 0);
		break;
	}

	UpdateFogWindConstants(vs, vertexCG, ps, pixelCG);

	// PS: p20 float4 ColourOutputClamp
	{
		XMVECTORF32& colourOutputClamp = pixelCG.Param<XMVECTORF32, 20>(ps);

		colourOutputClamp.f[0] = flt_143257C50;// fLightingOutputColourClampPostLit?
		colourOutputClamp.f[1] = flt_143257C54;// fLightingOutputColourClampPostEnv?
		colourOutputClamp.f[2] = flt_143257C58;// fLightingOutputColourClampPostSpec?
		colourOutputClamp.f[3] = 0.0f;
	}

	BSGraphics::Renderer::FlushConstantGroup(&vertexCG);
	BSGraphics::Renderer::FlushConstantGroup(&pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

	bool shadowed = (rawTechnique & RAW_FLAG_SHADOW_DIR) || (rawTechnique & (RAW_FLAG_UNKNOWN6 | RAW_FLAG_UNKNOWN5 | RAW_FLAG_UNKNOWN4));
	bool defShadow = (rawTechnique & RAW_FLAG_DEFSHADOW);

	// WARNING: Amazing use-after-free code right hereeeeee.............
	if (shadowed && defShadow)
	{
		BSGraphics::Renderer::SetShaderResource(14, (ID3D11ShaderResourceView *)qword_14304F260);
		BSGraphics::Renderer::SetTextureMode(14, 0, (dword_141E338A0 != 4) ? 1 : 0);

		// PS: p11 float4 VPOSOffset
		XMVECTORF32& vposOffset = pixelCG.Param<XMVECTORF32, 11>(ps);

		vposOffset.f[0] = 1.0f / (float)dword_143051B3C;
		vposOffset.f[1] = 1.0f / (float)dword_143051B40;
		vposOffset.f[2] = 0.0f;
		vposOffset.f[3] = 0.0f;
	}

	return true;
}

void BSLightingShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSLightingShader::RestoreTechnique, Technique);

	if (m_CurrentRawTechnique & RAW_FLAG_DEFSHADOW)
		BSGraphics::Renderer::SetShaderResource(14, nullptr);

	uintptr_t v2 = *(uintptr_t *)(qword_1431F5810 + 448);

	if (v2 && *(bool *)(v2 + 16) && *(bool *)(v2 + 17))
		BSGraphics::Renderer::SetShaderResource(15, nullptr);

	EndTechnique();
}

void sub_14130C470(__int64 a1, __int64 a2)
{
	NiTexture *v2 = *(NiTexture **)(a1 + 72);
	uint32_t v3 = *(uint32_t *)(a2 + 112);

	BSGraphics::Renderer::SetShaderResource(4, v2 ? v2->QRendererTexture() : nullptr);
	BSGraphics::Renderer::SetTextureAddressMode(4, v3);
}

void sub_14130C4D0(__int64 a1, __int64 a2)
{
	NiTexture *v2 = nullptr;
	uint32_t v3 = *(uint32_t *)(a2 + 112);

	if (a1)
		v2 = *(NiTexture **)(a1 + 72);
	else
		v2 = *(NiTexture **)(qword_143052920 + 72);

	BSGraphics::Renderer::SetShaderResource(5, v2 ? v2->QRendererTexture() : nullptr);
	BSGraphics::Renderer::SetTextureAddressMode(5, v3);
}

void sub_14130C220(int a1, __int64 a2, __int64 a3)
{
	NiTexture *v3 = *(NiTexture **)(a2 + 72);
	uint32_t v4 = *(uint32_t *)(a3 + 112);

	BSGraphics::Renderer::SetShaderResource(a1, v3 ? v3->QRendererTexture() : nullptr);
	BSGraphics::Renderer::SetTextureAddressMode(a1, v4);
}

void SetMultiTextureLandOverrides(__int64 a1)
{
	// This overrides all 16 samplers/input resources for land parameters if set in the property
	NiTexture *v2 = *(NiTexture **)(*(uintptr_t *)(a1 + 72) + 72i64);
	NiTexture *v3 = *(NiTexture **)(*(uintptr_t *)(a1 + 88) + 72i64);

	BSGraphics::Renderer::SetShaderResource(0, v2 ? v2->QRendererTexture() : nullptr);
	BSGraphics::Renderer::SetShaderResource(7, v3 ? v3->QRendererTexture() : nullptr);

	if (*(uint32_t *)(a1 + 160))
	{
		int v4 = 8;
		do
		{
			NiTexture *v5 = *(NiTexture **)(*(uintptr_t *)(a1 + 8i64 * (unsigned int)(v4 - 8) + 0xA8) + 72i64);
			uint32_t v6 = (unsigned int)(v4 - 7);

			BSGraphics::Renderer::SetShaderResource(v6, v5 ? v5->QRendererTexture() : nullptr);
			BSGraphics::Renderer::SetTextureAddressMode(v6, 3);

			NiTexture *v8 = *(NiTexture **)(*(uintptr_t *)(a1 + 8i64 * (unsigned int)(v4 - 8) + 208) + 72i64);

			BSGraphics::Renderer::SetShaderResource(v4, v8 ? v8->QRendererTexture() : nullptr);
			BSGraphics::Renderer::SetTextureAddressMode(v4, 3);

			++v4;
		} while ((unsigned int)(v4 - 8) < *(uint32_t *)(a1 + 160));
	}

	if (*(uintptr_t *)(a1 + 248))
	{
		NiTexture *result = *(NiTexture **)(*(uintptr_t *)(a1 + 248) + 72);

		BSGraphics::Renderer::SetShaderResource(13, result ? result->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(13, 0);
	}
}

__int64 __fastcall sub_141314170(__int64 a1)
{
	return *(unsigned int *)(a1 + 8);
}

void BSLightingShader::SetupMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSLightingShader::SetupMaterial, Material);

	auto *renderer = GetThreadedGlobals();

	BSVertexShader *vs = renderer->m_CurrentVertexShader;
	BSPixelShader *ps = renderer->m_CurrentPixelShader;

	BSGraphics::ConstantGroup vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(vs, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);
	BSGraphics::ConstantGroup pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(ps, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);

	const uint32_t rawTechnique = m_CurrentRawTechnique;
	const uint32_t baseTechniqueID = (rawTechnique >> 24) & 0x3F;
	bool setDiffuseNormalSamplers = true;

	const uintptr_t v5 = (uintptr_t)this;
	const uintptr_t v3 = (uintptr_t)Material;

	switch (baseTechniqueID)
	{
	case RAW_TECHNIQUE_ENVMAP:
	{
		sub_14130C470(*(uintptr_t *)(v3 + 160), v3);
		sub_14130C4D0(*(uintptr_t *)(v3 + 168), v3);

		// PS: p21 float4 EnvmapData
		XMVECTORF32& envmapData = pixelCG.Param<XMVECTORF32, 21>(ps);

		envmapData.f[0] = *(float *)(v3 + 176);
		envmapData.f[1] = (*(uintptr_t *)(v3 + 168)) ? 1.0f : 0.0f;
	}
	break;

	case RAW_TECHNIQUE_GLOWMAP:
		sub_14130C220(6, *(uintptr_t *)(v3 + 160), v3);
		break;

	case RAW_TECHNIQUE_PARALLAX:
		sub_14130C220(3, *(uintptr_t *)(v3 + 160), v3);
		break;

	case RAW_TECHNIQUE_FACEGEN:
		sub_14130C220(3, *(uintptr_t *)(v3 + 160), v3);
		sub_14130C220(4, *(uintptr_t *)(v3 + 168), v3);
		sub_14130C220(12, *(uintptr_t *)(v3 + 176), v3);
		break;

	case RAW_TECHNIQUE_FACEGENRGBTINT:
	case RAW_TECHNIQUE_HAIR:
	{
		// PS: p23 float4 TintColor
		XMVECTORF32& tintColor = pixelCG.Param<XMVECTORF32, 23>(ps);

		tintColor.f[0] = *(float *)(v3 + 160);
		tintColor.f[1] = *(float *)(v3 + 164);
		tintColor.f[2] = *(float *)(v3 + 168);
	}
	break;

	case RAW_TECHNIQUE_PARALLAXOCC:
	{
		// PS: p22 float4 ParallaxOccData
		XMVECTORF32& parallaxOccData = pixelCG.Param<XMVECTORF32, 22>(ps);

		sub_14130C220(3, *(uintptr_t *)(v3 + 160), v3);

		parallaxOccData.f[0] = *(float *)(v3 + 172);// Reversed on purpose?
		parallaxOccData.f[1] = *(float *)(v3 + 168);// Reversed on purpose?
	}
	break;

	case RAW_TECHNIQUE_MTLAND:
	case RAW_TECHNIQUE_MTLANDLODBLEND:
	{
		// PS: p24 float4 LODTexParams
		{
			XMVECTORF32& lodTexParams = pixelCG.Param<XMVECTORF32, 24>(ps);

			lodTexParams.f[0] = *(float *)(v3 + 328);
			lodTexParams.f[1] = *(float *)(v3 + 332);
			lodTexParams.f[2] = (byte_141E32E88) ? 1.0f : 0.0f;
			lodTexParams.f[3] = *(float *)(v3 + 336);
		}

		SetMultiTextureLandOverrides(v3);

		if (*(uintptr_t *)(v3 + 0x100))
			sub_14130C220(15, *(uintptr_t *)(v3 + 0x100), v3);

		setDiffuseNormalSamplers = false;

		// PS: p32 float4 LandscapeTexture1to4IsSpecPower
		{
			XMVECTORF32& landscapeTexture1to4IsSpecPower = pixelCG.Param<XMVECTORF32, 32>(ps);

			landscapeTexture1to4IsSpecPower.f[0] = *(float *)(v3 + 304);
			landscapeTexture1to4IsSpecPower.f[1] = *(float *)(v3 + 308);
			landscapeTexture1to4IsSpecPower.f[2] = *(float *)(v3 + 312);
			landscapeTexture1to4IsSpecPower.f[3] = *(float *)(v3 + 316);
		}

		// PS: p33 float4 LandscapeTexture5to6IsSpecPower
		{
			XMVECTORF32& landscapeTexture5to6IsSpecPower = pixelCG.Param<XMVECTORF32, 33>(ps);

			landscapeTexture5to6IsSpecPower.f[0] = *(float *)(v3 + 320);
			landscapeTexture5to6IsSpecPower.f[1] = *(float *)(v3 + 324);
			landscapeTexture5to6IsSpecPower.f[2] = 0.0f;
			landscapeTexture5to6IsSpecPower.f[3] = 0.0f;
		}

		if (rawTechnique & RAW_FLAG_SNOW)
		{
			// PS: p30 float4 LandscapeTexture1to4IsSnow
			XMVECTORF32& landscapeTexture1to4IsSnow = pixelCG.Param<XMVECTORF32, 30>(ps);

			landscapeTexture1to4IsSnow.f[0] = *(float *)(v3 + 280);
			landscapeTexture1to4IsSnow.f[1] = *(float *)(v3 + 284);
			landscapeTexture1to4IsSnow.f[2] = *(float *)(v3 + 288);
			landscapeTexture1to4IsSnow.f[3] = *(float *)(v3 + 292);

			// PS: p31 float4 LandscapeTexture5to6IsSnow
			XMVECTORF32& LandscapeTexture5to6IsSnow = pixelCG.Param<XMVECTORF32, 31>(ps);

			LandscapeTexture5to6IsSnow.f[0] = *(float *)(v3 + 296);
			LandscapeTexture5to6IsSnow.f[1] = *(float *)(v3 + 300);
			LandscapeTexture5to6IsSnow.f[2] = (byte_141E35338) ? 1.0f : 0.0f;
			LandscapeTexture5to6IsSnow.f[3] = 1.0f / (float)dword_141E353E0;// iLandscapeMultiNormalTilingFactor
		}
	}
	break;

	case RAW_TECHNIQUE_LODLAND:
	case RAW_TECHNIQUE_LODLANDNOISE:
	{
		// PS: p24 float4 LODTexParams
		{
			XMVECTORF32& lodTexParams = pixelCG.Param<XMVECTORF32, 24>(ps);

			lodTexParams.f[0] = *(float *)(v3 + 184);
			lodTexParams.f[1] = *(float *)(v3 + 188);
			lodTexParams.f[2] = (byte_141E32E88) ? 1.0f : 0.0f;
			lodTexParams.f[3] = *(float *)(v3 + 192);
		}

		if (baseTechniqueID == RAW_TECHNIQUE_LODLANDNOISE)
		{
			if (*(uintptr_t *)(v3 + 176))
				sub_14130C220(15, *(uintptr_t *)(v3 + 176), v3);

			BSGraphics::Renderer::SetTextureMode(15, 3, 1);
		}
	}
	break;

	case RAW_TECHNIQUE_MULTILAYERPARALLAX:
	{
		sub_14130C220(8, *(uintptr_t *)(v3 + 160), v3);

		// PS: p27 float4 MultiLayerParallaxData
		{
			XMVECTORF32& multiLayerParallaxData = pixelCG.Param<XMVECTORF32, 27>(ps);

			multiLayerParallaxData.f[0] = *(float *)(v3 + 184);
			multiLayerParallaxData.f[1] = *(float *)(v3 + 188);
			multiLayerParallaxData.f[2] = *(float *)(v3 + 192);
			multiLayerParallaxData.f[3] = *(float *)(v3 + 196);
		}

		sub_14130C470(*(uintptr_t *)(v3 + 168), v3);
		sub_14130C4D0(*(uintptr_t *)(v3 + 176), v3);

		// PS: p21 float4 EnvmapData
		{
			XMVECTORF32& envmapData = pixelCG.Param<XMVECTORF32, 21>(ps);

			envmapData.f[0] = *(float *)(v3 + 200);
			envmapData.f[1] = (*(uintptr_t *)(v3 + 176)) ? 1.0f : 0.0f;
		}
	}
	break;

	case RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW:
	{
		// PS: p26 float4 SparkleParams
		XMVECTORF32& sparkleParams = pixelCG.Param<XMVECTORF32, 26>(ps);

		sparkleParams.f[0] = *(float *)(v3 + 160);
		sparkleParams.f[1] = *(float *)(v3 + 164);
		sparkleParams.f[2] = *(float *)(v3 + 168);
		sparkleParams.f[3] = *(float *)(v3 + 172);
	}
	break;

	case RAW_TECHNIQUE_EYE:
	{
		sub_14130C470(*(uintptr_t *)(v3 + 160), v3);
		sub_14130C4D0(*(uintptr_t *)(v3 + 168), v3);

		// VS: p9 float4 Color2
		{
			XMVECTORF32& color2 = vertexCG.Param<XMVECTORF32, 9>(vs);

			color2.f[0] = *(float *)(v3 + 180);
			color2.f[1] = *(float *)(v3 + 184);
			color2.f[2] = *(float *)(v3 + 188);
		}

		// VS: p10 float4 Color3
		{
			XMVECTORF32& color3 = vertexCG.Param<XMVECTORF32, 10>(vs);

			color3.f[0] = *(float *)(v3 + 192);
			color3.f[1] = *(float *)(v3 + 196);
			color3.f[2] = *(float *)(v3 + 200);
		}

		// PS: p21 float4 EnvmapData
		{
			XMVECTORF32& envmapData = pixelCG.Param<XMVECTORF32, 21>(ps);

			envmapData.f[0] = *(float *)(v3 + 176);
			envmapData.f[1] = (*(uintptr_t *)(v3 + 168)) ? 1.0f : 0.0f;
		}
	}
	break;
	}

	// VS: p11 float4 Velocity
	{
		XMVECTORF32& velocity = vertexCG.Param<XMVECTORF32, 11>(vs);

		velocity.f[0] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 12);
		velocity.f[1] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 16);
		velocity.f[2] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 28);
		velocity.f[3] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 32);
	}

	if (rawTechnique & RAW_FLAG_SPECULAR)
	{
		// PS: p25 float4 SpecularColor
		XMVECTORF32& specularColor = pixelCG.Param<XMVECTORF32, 25>(ps);

		specularColor.f[0] = *(float *)(v3 + 56) * *(float *)(v3 + 0x8C);
		specularColor.f[1] = *(float *)(v3 + 60) * *(float *)(v3 + 0x8C);
		specularColor.f[2] = *(float *)(v3 + 64) * *(float *)(v3 + 0x8C);
		specularColor.f[3] = *(float *)(v3 + 136);

		if (rawTechnique & RAW_FLAG_MODELSPACENORMALS)
		{
			sub_14130C220(2, *(uintptr_t *)(v3 + 104), v3);

			uint32_t v68 = *(uint32_t *)(v3 + 112);
			NiTexture *v69 = *(NiTexture **)(*(uintptr_t *)(v3 + 104) + 72i64);

			BSGraphics::Renderer::SetShaderResource(2, v69 ? v69->QRendererTexture() : nullptr);
			BSGraphics::Renderer::SetTextureMode(2, v68, 3);
		}
	}

	if (rawTechnique & RAW_FLAG_AMBIENT_SPECULAR)
	{
		// PS: p6 float4 AmbientSpecularTintAndFresnelPower
		pixelCG.Param<XMVECTORF32, 6>(ps) = xmmword_141E3301C;
	}

	if (rawTechnique & RAW_FLAG_SOFT_LIGHTING)
	{
		uint32_t v71 = *(uint32_t *)(v3 + 112);
		NiTexture *v72 = *(NiTexture **)(*(uintptr_t *)(v3 + 96) + 72i64);

		BSGraphics::Renderer::SetShaderResource(12, v72 ? v72->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(12, v71);

		// PS: p28 float4 LightingEffectParams
		XMVECTORF32& lightingEffectParams = pixelCG.Param<XMVECTORF32, 28>(ps);

		lightingEffectParams.f[0] = *(float *)(v3 + 144);
		lightingEffectParams.f[1] = *(float *)(v3 + 148);
	}

	if (rawTechnique & RAW_FLAG_RIM_LIGHTING)
	{
		// I guess this is identical to RAW_FLAG_SOFT_LIGHTING above
		uint32_t v76 = *(uint32_t *)(v3 + 112);
		NiTexture *v77 = *(NiTexture **)(*(uintptr_t *)(v3 + 96) + 72i64);

		BSGraphics::Renderer::SetShaderResource(12, v77 ? v77->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(12, v76);

		// PS: p28 float4 LightingEffectParams
		XMVECTORF32& lightingEffectParams = pixelCG.Param<XMVECTORF32, 28>(ps);

		lightingEffectParams.f[0] = *(float *)(v3 + 144);
		lightingEffectParams.f[1] = *(float *)(v3 + 148);
	}

	if (rawTechnique & RAW_FLAG_BACK_LIGHTING)
	{
		uint32_t v81 = *(uint32_t *)(v3 + 112);
		NiTexture *v82 = *(NiTexture **)(*(uintptr_t *)(v3 + 104) + 72i64);

		BSGraphics::Renderer::SetShaderResource(9, v82 ? v82->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(9, v81);
	}

	if (rawTechnique & RAW_FLAG_SNOW)
	{
		// PS: p34 float4 SnowRimLightParameters
		XMVECTORF32& snowRimLightParameters = pixelCG.Param<XMVECTORF32, 34>(ps);

		snowRimLightParameters.f[0] = flt_141E35380;// fSnowRimLightIntensity
		snowRimLightParameters.f[1] = flt_141E35398;// fSnowGeometrySpecPower
		snowRimLightParameters.f[2] = flt_141E353B0;// fSnowNormalSpecPower
		snowRimLightParameters.f[3] = (!byte_141E353C8) ? 0.0f : 1.0f;// bEnableSnowRimLighting
	}

	if (setDiffuseNormalSamplers)
	{
		int v88 = *(int *)(v3 + 80);

		if (v88 == -1)
		{
			sub_14130C220(0, *(uintptr_t *)(v3 + 72), v3);
		}
		else
		{
			uint32_t v89 = *(uint32_t *)(v3 + 112);
			auto v90 = (ID3D11ShaderResourceView *)*(&qword_14304EF00 + 6 * v88);

			BSGraphics::Renderer::SetShaderResource(0, v90);
			BSGraphics::Renderer::SetTextureAddressMode(0, v89);
		}

		uint32_t v91 = *(uint32_t *)(v3 + 112);
		NiTexture *v92 = *(NiTexture **)(*(uintptr_t *)(v3 + 88) + 72i64);

		BSGraphics::Renderer::SetShaderResource(1, v92 ? v92->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(1, v91);
	}

	// PS: p29 float4 IBLParams
	{
		XMVECTORF32& iblParams = pixelCG.Param<XMVECTORF32, 29>(ps);

		XMVECTORF32 thing;

		if (*(bool *)(v5 + 240))
			thing = *(XMVECTORF32 *)(v5 + 208);
		else
			thing = *(XMVECTORF32 *)(v5 + 224);

		iblParams.f[0] = *(float *)(v5 + 204);
		iblParams.f[1] = thing.f[0];
		iblParams.f[2] = thing.f[1];
		iblParams.f[3] = thing.f[2];
	}

	if (rawTechnique & RAW_FLAG_CHARACTER_LIGHT)
	{
		if (dword_141E33BA0 >= 0)
		{
			auto v99 = (ID3D11ShaderResourceView *)*(&qword_14304EF00 + 6 * (signed int)sub_141314170(g_ModuleBase + 0x1E33B98));

			BSGraphics::Renderer::SetShaderResource(11, v99);
			BSGraphics::Renderer::SetTextureAddressMode(11, 0);
		}

		// PS: p35 float4 CharacterLightParams
		XMVECTORF32& characterLightParams = pixelCG.Param<XMVECTORF32, 35>(ps);

		// if (bEnableCharacterRimLighting)
		if (byte_141E32F66)
			characterLightParams.v = _mm_loadu_ps(&xmmword_141E3302C);
	}

	BSGraphics::Renderer::FlushConstantGroup(&vertexCG);
	BSGraphics::Renderer::FlushConstantGroup(&pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);
}

void BSLightingShader::RestoreMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSLightingShader::RestoreMaterial, Material);
}

void UpdateViewProjectionConstants(BSGraphics::ConstantGroup& VertexCG, const NiTransform& Transform, bool IsPreviousWorld, const NiPoint3 *PosAdjust)
{
	//
	// Instead of using the typical 4x4 matrix like everywhere else, someone decided that the
	// lighting shader is going to use a 4x3 matrix. The missing 4th column is assumed to be
	// { 0, 0, 0, 1 } in row-major form.
	//
	XMMATRIX projMatrix;
	
	if (PosAdjust)
		projMatrix = BSShaderUtil::GetXMFromNiPosAdjust(Transform, *PosAdjust);
	else
		projMatrix = BSShaderUtil::GetXMFromNi(Transform);

	//
	// VS: p0 float4x3 WorldViewProj
	// -- or --
	// VS: p1 float4x3 PrevWorldViewProj
	//
	if (!IsPreviousWorld)
		XMStoreFloat4x3(&VertexCG.Param<XMFLOAT4X3, 0>(GetThreadedGlobals()->m_CurrentVertexShader), projMatrix);
	else
		XMStoreFloat4x3(&VertexCG.Param<XMFLOAT4X3, 1>(GetThreadedGlobals()->m_CurrentVertexShader), projMatrix);
}

void BSLightingShader::SetupGeometry(BSRenderPass *Pass, uint32_t Flags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSLightingShader::SetupGeometry, Pass, Flags);

	/*
	v2 = a1;
	v3 = *(_QWORD *)(a2 + 8);
	v4 = *(_QWORD *)&renderer.__zz2[16];
	v5 = *(_QWORD *)(a2 + 16);
	v109 = (_DWORD *)v5;
	retaddr = 0;
	v104 = v5 + 124;
	v6 = v5 + 176;
	v105 = *(unsigned __int8 **)&renderer.__zz2[16];
	v102 = ~(unsigned __int8)(*(_DWORD *)(a1 + 148) >> 1) & 1;
	v7 = *(_QWORD *)(*(_QWORD *)&renderer.__zz2[8] + 56i64);
	v8 = (_QWORD *)(*(_QWORD *)&renderer.__zz2[8] + 56i64);
	v110 = *(_QWORD *)&renderer.__zz2[8];

	auto *renderer = GetThreadedGlobals();

	BSVertexShader *vs = renderer->m_CurrentVertexShader;
	BSPixelShader *ps = renderer->m_CurrentPixelShader;

	BSGraphics::ConstantGroup vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(vs, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	BSGraphics::ConstantGroup pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(ps, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	v11 = 0;
	v12 = *(_BYTE *)(v120 + 28);
	v103 = (unsigned __int8)(v12 - 2) <= 1u;

	if (v12 == 3 && *(_QWORD *)(v3 + 56) & 0x100000000i64)
	{
		dword_141E3527C = *(_DWORD *)&renderer.__zz0[72];
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(1);
	}

	v14 = *(_DWORD *)(v2 + 148);
	v15 = (*(_DWORD *)(v2 + 148) >> 24) & 0x3F;

	switch (v15)
	{
	case RAW_TECHNIQUE_ENVMAP:
	case RAW_TECHNIQUE_MULTILAYERPARALLAX:
	case RAW_TECHNIQUE_EYE:
		if (!(v14 & RAW_FLAG_SKINNED))
			UpdateViewProjectionConstants(vertexCG, Pass->m_Geometry->GetWorldTransform(), false, nullptr);

		v16 = 0;
		v102 = 0;
		retaddr = 1;
		*(_DWORD *)(v10[1] + 4i64 * v105[71]) = *(_DWORD *)(v3 + 260);
		goto LABEL_20;

	case RAW_TECHNIQUE_MTLAND:
	case RAW_TECHNIQUE_MTLANDLODBLEND:
		v17 = *(_QWORD *)(v3 + 120);
		v18 = *(unsigned int *)(v17 + 268);
		v19 = *(unsigned int *)(v17 + 264);
		sub_14130BAB0(v8, v5 + 160);
		break;

	case RAW_TECHNIQUE_LODLAND:
	case RAW_TECHNIQUE_LODOBJ:
	case RAW_TECHNIQUE_LODOBJHD:
		UpdateViewProjectionConstants(vertexCG, Pass->m_Geometry->GetWorldTransform(), false, nullptr);
		UpdateViewProjectionConstants(vertexCG, Pass->m_Geometry->GetWorldTransform(), true, (NiPoint3 *)&renderer->__zz2[40]);
		v11 = 1;
		break;

	case RAW_TECHNIQUE_TREE:
		sub_14130BC60(v8, v3);
		break;
	}

	v16 = v102;
LABEL_20:
	if (*(_BYTE *)(v119 + 148) & RAW_FLAG_SKINNED)
	{
		v24 = v104;
	}
	else
	{
		v23 = v11 == 0;
		v24 = v104;
		if (v23)
		{
			UpdateViewProjectionConstants(vertexCG, Pass->m_Geometry->GetWorldTransform(), false, nullptr);

			if (vars0 & 0x10)
				v6 = v104;

			UpdateViewProjectionConstants(vertexCG, v6, true, (NiPoint3 *)&renderer->__zz2[40]);
		}
	}
	sub_140D422E0(v24, 0i64, (__int64)&v118);
	sub_14130B190((__int64)v10, v120, (__int64)&v118, v16);
	sub_14130B2A0((__int64)v10, v24, v16);
	v28 = v120;
	v29 = v10[1];
	v30 = *(float *)(v3 + 48);
	v31 = *(unsigned __int8 *)(*(_QWORD *)&renderer.__zz2[16] + 71i64);
	v32 = *(_BYTE *)(v120 + 30) < 0;
	*(float *)(v29 + 4 * v31 + 8) = v30;
	if (v32)
		*(float *)(v29 + 4 * v31 + 8) = v30 * *(float *)(*(_QWORD *)(v3 + 96) + 332i64);
	v33 = *(_QWORD *)(v3 + 240);
	v34 = *(float *)(v3 + 248);
	v35 = v10[1];
	v36 = v119;
	v37 = *(_QWORD *)v33;
	LODWORD(v33) = *(_DWORD *)(v33 + 8);
	v106 = v37;
	v38 = (__m128)(unsigned int)v37;
	v39 = (__m128)HIDWORD(v37);
	v107 = *(float *)&v33;
	v40 = *(unsigned __int8 *)(*(_QWORD *)&renderer.__zz2[16] + 72i64);
	v38.m128_f32[0] = *(float *)&v37 * v34;
	v107 = *(float *)&v33 * v34;
	v39.m128_f32[0] = *((float *)&v37 + 1) * v34;
	*(_QWORD *)(v35 + 4 * v40) = (unsigned __int128)_mm_unpacklo_ps(v38, v39);
	*(float *)(v35 + 4 * v40 + 8) = *(float *)&v33 * v34;
	v41 = (*(_DWORD *)(v36 + 148) >> 3) & 7;
	v42 = (*(_DWORD *)(v36 + 148) >> 6) & 7;
	v43 = (float *)(v10[1] + 4i64 * v105[64]);
	if (v43)
	{
		*v43 = (float)v41;
		v43[1] = (float)v42;
	}
	v44 = v10[1];
	v45 = (void *)(v44 + 4i64 * v105[66]);
	v46 = (void *)(v44 + 4i64 * v105[65]);
	if (v45 && v46)
	{
		memset(v45, 0, 0x70ui64);
		memset(v46, 0, 0x70ui64);
		v36 = v119;
		v28 = v120;
	}
	if (v41 <= 0)
	{
		v49 = v102;
	}
	else
	{
		v47 = *(float *)(v104 + 48);
		v48 = *(_BYTE *)(v36 + 151) & 0x3F;
		if (v48 == 1 || v48 == 16)
			v47 = 1.0;
		v49 = v102;
		sub_14130B390((__int64)v10, v28, (__int64)&v118, v41, v42, v47, v102);
	}
	v50 = v119;
	if (*(_DWORD *)(v119 + 148) & RAW_FLAG_SPECULAR)
	{
		retaddr = 1;
		*(_DWORD *)(v10[1] + 4i64 * v105[71] + 4) = *(_DWORD *)(v3 + 256);
	}
	v51 = *(_DWORD *)(v50 + 148);
	v52 = retaddr;
	if (*(_DWORD *)(v50 + 148) & 0x21C00)
		v52 = 1;
	LODWORD(v119) = v52;
	v53 = byte_141E35308 && (!(vars0 & 8) || !byte_141E35320);

	if (_bittest(&v51, 0xFu) && v15 != 6)
	{
		v54 = *(_QWORD *)(qword_143052890 + 72);
		if (v54)
			v54 = *(_QWORD *)(v54 + 16);

		BSGraphics::Renderer::SetShaderResource(11, v54);
		BSGraphics::Renderer::SetTextureMode(11, 3, 1);

		if (v53 && qword_143052898)
		{
			v57 = *(_QWORD *)(qword_143052898 + 72);
			if (v57)
				v57 = *(_QWORD *)(v57 + 16);

			BSGraphics::Renderer::SetShaderResource(3, v57);
			BSGraphics::Renderer::SetTextureMode(3, 3, 1);

			v58 = *(_QWORD *)(qword_1430528A0 + 72);

			if (v58)
				v58 = *(_QWORD *)(v58 + 16);

			BSGraphics::Renderer::SetShaderResource(8, v58);
			BSGraphics::Renderer::SetTextureMode(8, 3, 1);

			v59 = *(_QWORD *)(qword_1430528A8 + 72);

			if (v59)
				v59 = *(_QWORD *)(v59 + 16);

			BSGraphics::Renderer::SetShaderResource(10, v59);
			BSGraphics::Renderer::SetTextureMode(10, 3, 1);
		}

		v60 = v109;
		if ((*(__int64(__fastcall **)(_DWORD *, __int64))(*(_QWORD *)v109 + 424i64))(v109, v55))
		{
			v61 = (__int64)v60;
			v62 = v8[1];
			v63 = *(unsigned __int8 *)(*(_QWORD *)&renderer.__zz2[8] + 86i64);
			*(_DWORD *)(v62 + 4 * v63) = v60[91];
			*(_DWORD *)(v62 + 4 * v63 + 4) = v60[95];
			*(_DWORD *)(v62 + 4 * v63 + 8) = v60[99];
			*(_DWORD *)(v62 + 4 * v63 + 12) = v60[103];
			*(_DWORD *)(v62 + 4 * v63 + 16) = v60[92];
			*(_DWORD *)(v62 + 4 * v63 + 20) = v60[96];
			*(_DWORD *)(v62 + 4 * v63 + 24) = v60[100];
			*(_DWORD *)(v62 + 4 * v63 + 28) = v60[104];
			*(_DWORD *)(v62 + 4 * v63 + 32) = v60[93];
			*(_DWORD *)(v62 + 4 * v63 + 36) = v60[97];
			*(_DWORD *)(v62 + 4 * v63 + 40) = v60[101];
			*(_DWORD *)(v62 + 4 * v63 + 44) = v60[105];
		}
		else
		{
			sub_14130C8A0((__int64)(v60 + 31), &v107, (*(_BYTE *)(v50 + 151) & 0x3F) == 1);
			v61 = 0i64;
			v64 = v110;
			v65 = *(unsigned __int8 *)(*(_QWORD *)&renderer.__zz2[8] + 86i64);
			v66 = v8[1];
			*(float *)(v66 + 4 * v65) = v107;
			v67 = v112;
			*(_DWORD *)(v66 + 4 * v65 + 4) = v64;
			v68 = v115;
			*(_DWORD *)(v66 + 4 * v65 + 8) = v67;
			v69 = v108;
			*(_DWORD *)(v66 + 4 * v65 + 12) = v68;
			v70 = HIDWORD(v110);
			*(_DWORD *)(v66 + 4 * v65 + 16) = v69;
			v71 = v113;
			*(_DWORD *)(v66 + 4 * v65 + 20) = v70;
			v72 = v116;
			*(_DWORD *)(v66 + 4 * v65 + 24) = v71;
			v73 = (signed int)v109;
			*(_DWORD *)(v66 + 4 * v65 + 28) = v72;
			v74 = v111;
			*(_DWORD *)(v66 + 4 * v65 + 32) = v73;
			v75 = v114;
			*(_DWORD *)(v66 + 4 * v65 + 36) = v74;
			v76 = v117;
			*(_DWORD *)(v66 + 4 * v65 + 40) = v75;
			*(_DWORD *)(v66 + 4 * v65 + 44) = v76;
		}
		sub_14130BE70((__int64)v10, v61, (_DWORD *)v3, v53);
	}

	if (*(_DWORD *)(v50 + 148) & RAW_FLAG_WORLD_MAP)
	{
		v77 = *(_QWORD *)(qword_141E32F90 + 72);
		if (v77)
			v77 = *(_QWORD *)(v77 + 16);

		BSGraphics::Renderer::SetShaderResource(12, v77);
		BSGraphics::Renderer::SetTextureAddressMode(12, 3);

		v80 = *(_QWORD *)(qword_141E32F98 + 72);

		if (v80)
			v80 = *(_QWORD *)(v80 + 16);

		BSGraphics::Renderer::SetShaderResource(13, v80);
		BSGraphics::Renderer::SetTextureAddressMode(13, 3);

		v81 = (_DWORD *)(v10[1] + 4i64 * v105[81]);

		BSGraphics::Utility::CopyNiColorAToFloat(
			(_DWORD *)(v8[1] + 4i64 * *(unsigned __int8 *)(v110 + 88)),
			&dword_1431F5540);

		BSGraphics::Utility::CopyNiColorAToFloat(v81, &dword_1431F5550);
	}

	if ((_BYTE)v119)
	{
		if (v49 == 1)
		{
			v82 = v8[1] + 4i64 * *(unsigned __int8 *)(*(_QWORD *)&renderer.__zz2[8] + 82i64);
			v83 = sub_1412AD330();
			D3DXVec3TransformCoord(v82, v83 + 364, &v118);
		}
		else
		{
			v84 = *(unsigned __int8 *)(*(_QWORD *)&renderer.__zz2[8] + 82i64);
			v85 = v8[1];
			v86 = (float *)sub_1412AD330();
			v87 = v86[92] - *(float *)&renderer.__zz2[32];
			v88 = v86[93] - *(float *)&renderer.__zz2[36];
			*(float *)(v85 + 4 * v84) = v86[91] - *(float *)&renderer.__zz2[28];
			*(float *)(v85 + 4 * v84 + 4) = v87;
			*(float *)(v85 + 4 * v84 + 8) = v88;
		}
	}
	if (*(_BYTE *)(v120 + 28) != 10
		|| ((v89 = *(_QWORD *)(v3 + 96), *(_BYTE *)(v120 + 30) >= 0) ? (v90 = *(float *)(v89 + 304)) : (v90 = *(float *)(v89 + 332)),
			*(_DWORD *)&renderer.__zz0[40] == 11
			&& *(_DWORD *)&renderer.__zz0[44] == (unsigned __int8)(signed int)(float)(v90 * 31.0)))
	{
		v91 = renderer.dword_14304DEB0;
	}
	else
	{
		*(_DWORD *)&renderer.__zz0[44] = (unsigned __int8)(signed int)(float)(v90 * 31.0);
		v91 = renderer.dword_14304DEB0 | 8;
		*(_DWORD *)&renderer.__zz0[40] = 11;
		renderer.dword_14304DEB0 |= 8u;
	}
	if (!v103)
	{
		v92 = *(_DWORD *)&renderer.__zz0[32];
		if (!(*(_QWORD *)(v3 + 56) & 0x100000000i64))
		{
			dword_141E35280 = *(_DWORD *)&renderer.__zz0[32];
			if (*(_DWORD *)&renderer.__zz0[32] != 1)
			{
				v92 = 1;
				*(_DWORD *)&renderer.__zz0[32] = 1;
				v93 = v91;
				v91 &= 0xFFFFFFFB;
				v94 = v93 | 4;
				if (*(_DWORD *)&renderer.__zz0[36] != 1)
					v91 = v94;
				renderer.dword_14304DEB0 = v91;
			}
		}
		if (!(*(_DWORD *)(v3 + 56) & 0x80000000))
		{
			dword_141E35280 = v92;
			if (v92)
			{
				*(_DWORD *)&renderer.__zz0[32] = 0;
				v95 = v91 | 4;
				v96 = v91 & 0xFFFFFFFB;
				if (*(_DWORD *)&renderer.__zz0[36])
					v96 = v95;
				renderer.dword_14304DEB0 = v96;
			}
		}
	}
	v97 = v10[1] + 4i64 * v105[80];
	if (v97)
	{
		*(_DWORD *)v97 = dword_141E34C70;
		*(float *)(v97 + 4) = *(float *)&dword_141E34C88 + *(float *)&dword_141E34C70;
		v98 = 0.0;
		*(_DWORD *)(v97 + 8) = dword_143257C40;
		if (*(_DWORD *)(v50 + 148) & 0x200)
			v99 = *(float *)(v3 + 256);
		else
			v99 = 0.0;
		if (!(vars0 & 2))
			v98 = 1.0;
		*(float *)(v97 + 12) = v98 * v99;
	}

	BSGraphics::Renderer::FlushConstantGroup(&vertexCG);
	BSGraphics::Renderer::FlushConstantGroup(&pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);*/
}

void BSLightingShader::RestoreGeometry(BSRenderPass *Pass)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSLightingShader::RestoreGeometry, Pass);
}

uint32_t BSLightingShader::GetRawTechnique(uint32_t Technique)
{
	uint32_t outputTech = Technique - 0x4800002D;
	uint32_t subIndex = (outputTech >> 24) & 0x3F;

	if (subIndex == 18 && !byte_141E32E89)
	{
		outputTech = outputTech & 0xC9FFFFFF | 0x9000000;
	}
	else if (subIndex == 7 && !byte_141E352F0)
	{
		outputTech &= 0xC0FFFFFF;
	}

	return outputTech;
}

uint32_t BSLightingShader::GetVertexTechnique(uint32_t RawTechnique)
{
	uint32_t flags = RawTechnique & (
		RAW_FLAG_VC |
		RAW_FLAG_SKINNED |
		RAW_FLAG_MODELSPACENORMALS |
		RAW_FLAG_PROJECTED_UV |
		RAW_FLAG_WORLD_MAP);

	if (RawTechnique & (RAW_FLAG_SPECULAR | RAW_FLAG_RIM_LIGHTING | RAW_FLAG_AMBIENT_SPECULAR))
		flags |= RAW_FLAG_SPECULAR;

	return (RawTechnique & 0x3F000000) | flags;
}

uint32_t BSLightingShader::GetPixelTechnique(uint32_t RawTechnique)
{
	uint32_t flags = RawTechnique & ~(
		RAW_FLAG_UNKNOWN1 |
		RAW_FLAG_UNKNOWN2 |
		RAW_FLAG_UNKNOWN3 |
		RAW_FLAG_UNKNOWN4 |
		RAW_FLAG_UNKNOWN5 |
		RAW_FLAG_UNKNOWN6);

	if ((flags & RAW_FLAG_MODELSPACENORMALS) == 0)
		flags &= ~RAW_FLAG_SKINNED;

	return flags | RAW_FLAG_VC;
}