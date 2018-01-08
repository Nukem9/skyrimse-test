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
// - Constructor is not implemented
// - Destructor is not implemented
// - Global variables eliminated in each Setup/Restore function
// - A lock is held in GeoUpdateMTLandExtraConstants()
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

AutoPtr(float, flt_141E34C70, 0x1E34C70);
AutoPtr(float, flt_141E34C88, 0x1E34C88);
AutoPtr(float, flt_143257C40, 0x3257C40);
AutoPtr(uint32_t, dword_141E35280, 0x1E35280);
AutoPtr(NiColorA, dword_1431F5540, 0x31F5540);
AutoPtr(NiColorA, dword_1431F5550, 0x31F5550);
AutoPtr(uintptr_t, qword_141E32F90, 0x1E32F90);
AutoPtr(uintptr_t, qword_141E32F98, 0x1E32F98);
AutoPtr(uintptr_t, qword_143052890, 0x3052890);
AutoPtr(uintptr_t, qword_143052898, 0x3052898);
AutoPtr(uintptr_t, qword_1430528A8, 0x30528A8);
AutoPtr(BYTE, byte_141E35308, 0x1E35308);
AutoPtr(BYTE, byte_141E35320, 0x1E35320);
AutoPtr(uint32_t, dword_141E3527C, 0x1E3527C);
AutoPtr(float, xmmword_141880020, 0x1880020);
AutoPtr(float, flt_141E32F40, 0x1E32F40);
AutoPtr(float, flt_141E32FD8, 0x1E32FD8);
AutoPtr(float, flt_141E32FB8, 0x1E32FB8);
AutoPtr(BYTE, byte_1431F547C, 0x31F547C);
AutoPtr(XMFLOAT4, xmmword_141E32FC8, 0x1E32FC8);
AutoPtr(float, flt_141E35350, 0x1E35350);
AutoPtr(float, flt_141E35368, 0x1E35368);

BSShaderAccumulator *GetCurrentAccumulator();

char hookbuffer[50];

thread_local uint32_t TLS_m_CurrentRawTechnique;
thread_local uint32_t TLS_dword_141E35280;
thread_local uint32_t TLS_dword_141E3527C;

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
}

BSLightingShader::~BSLightingShader()
{
	__debugbreak();
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

	//m_CurrentRawTechnique = rawTechnique;
	TLS_m_CurrentRawTechnique = rawTechnique;

	auto *renderer = GetThreadedGlobals();
	auto vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	auto pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

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
		TechUpdateAccelerationConstants(vertexCG);
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

	TechUpdateFogWindConstants(vertexCG, pixelCG);

	// PS: p20 float4 ColourOutputClamp
	{
		XMVECTORF32& colourOutputClamp = pixelCG.ParamPS<XMVECTORF32, 20>();

		colourOutputClamp.f[0] = flt_143257C50;// fLightingOutputColourClampPostLit?
		colourOutputClamp.f[1] = flt_143257C54;// fLightingOutputColourClampPostEnv?
		colourOutputClamp.f[2] = flt_143257C58;// fLightingOutputColourClampPostSpec?
		colourOutputClamp.f[3] = 0.0f;
	}

	BSGraphics::Renderer::FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

	bool shadowed = (rawTechnique & RAW_FLAG_SHADOW_DIR) || (rawTechnique & (RAW_FLAG_UNKNOWN6 | RAW_FLAG_UNKNOWN5 | RAW_FLAG_UNKNOWN4));
	bool defShadow = (rawTechnique & RAW_FLAG_DEFSHADOW);

	// WARNING: Amazing use-after-free code right hereeeeee.............
	if (shadowed && defShadow)
	{
		BSGraphics::Renderer::SetShaderResource(14, (ID3D11ShaderResourceView *)qword_14304F260);
		BSGraphics::Renderer::SetTextureMode(14, 0, (dword_141E338A0 != 4) ? 1 : 0);

		// PS: p11 float4 VPOSOffset
		XMVECTORF32& vposOffset = pixelCG.ParamPS<XMVECTORF32, 11>();

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

	if (TLS_m_CurrentRawTechnique & RAW_FLAG_DEFSHADOW)
		BSGraphics::Renderer::SetShaderResource(14, nullptr);

	uintptr_t v2 = *(uintptr_t *)(qword_1431F5810 + 448);

	if (v2 && *(bool *)(v2 + 16) && *(bool *)(v2 + 17))
		BSGraphics::Renderer::SetShaderResource(15, nullptr);

	EndTechnique();
}

void BSLightingShader::SetupMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSLightingShader::SetupMaterial, Material);

	auto *renderer = GetThreadedGlobals();
	auto vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);
	auto pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);

	const uint32_t rawTechnique = TLS_m_CurrentRawTechnique;
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
		XMVECTORF32& envmapData = pixelCG.ParamPS<XMVECTORF32, 21>();

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
		XMVECTORF32& tintColor = pixelCG.ParamPS<XMVECTORF32, 23>();

		tintColor.f[0] = *(float *)(v3 + 160);
		tintColor.f[1] = *(float *)(v3 + 164);
		tintColor.f[2] = *(float *)(v3 + 168);
	}
	break;

	case RAW_TECHNIQUE_PARALLAXOCC:
	{
		// PS: p22 float4 ParallaxOccData
		XMVECTORF32& parallaxOccData = pixelCG.ParamPS<XMVECTORF32, 22>();

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
			XMVECTORF32& lodTexParams = pixelCG.ParamPS<XMVECTORF32, 24>();

			lodTexParams.f[0] = *(float *)(v3 + 328);
			lodTexParams.f[1] = *(float *)(v3 + 332);
			lodTexParams.f[2] = (byte_141E32E88) ? 1.0f : 0.0f;
			lodTexParams.f[3] = *(float *)(v3 + 336);
		}

		MatSetMultiTextureLandOverrides(v3);

		if (*(uintptr_t *)(v3 + 0x100))
			sub_14130C220(15, *(uintptr_t *)(v3 + 0x100), v3);

		setDiffuseNormalSamplers = false;

		// PS: p32 float4 LandscapeTexture1to4IsSpecPower
		{
			XMVECTORF32& landscapeTexture1to4IsSpecPower = pixelCG.ParamPS<XMVECTORF32, 32>();

			landscapeTexture1to4IsSpecPower.f[0] = *(float *)(v3 + 304);
			landscapeTexture1to4IsSpecPower.f[1] = *(float *)(v3 + 308);
			landscapeTexture1to4IsSpecPower.f[2] = *(float *)(v3 + 312);
			landscapeTexture1to4IsSpecPower.f[3] = *(float *)(v3 + 316);
		}

		// PS: p33 float4 LandscapeTexture5to6IsSpecPower
		{
			XMVECTORF32& landscapeTexture5to6IsSpecPower = pixelCG.ParamPS<XMVECTORF32, 33>();

			landscapeTexture5to6IsSpecPower.f[0] = *(float *)(v3 + 320);
			landscapeTexture5to6IsSpecPower.f[1] = *(float *)(v3 + 324);
			landscapeTexture5to6IsSpecPower.f[2] = 0.0f;
			landscapeTexture5to6IsSpecPower.f[3] = 0.0f;
		}

		if (rawTechnique & RAW_FLAG_SNOW)
		{
			// PS: p30 float4 LandscapeTexture1to4IsSnow
			XMVECTORF32& landscapeTexture1to4IsSnow = pixelCG.ParamPS<XMVECTORF32, 30>();

			landscapeTexture1to4IsSnow.f[0] = *(float *)(v3 + 280);
			landscapeTexture1to4IsSnow.f[1] = *(float *)(v3 + 284);
			landscapeTexture1to4IsSnow.f[2] = *(float *)(v3 + 288);
			landscapeTexture1to4IsSnow.f[3] = *(float *)(v3 + 292);

			// PS: p31 float4 LandscapeTexture5to6IsSnow
			XMVECTORF32& LandscapeTexture5to6IsSnow = pixelCG.ParamPS<XMVECTORF32, 31>();

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
			XMVECTORF32& lodTexParams = pixelCG.ParamPS<XMVECTORF32, 24>();

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
			XMVECTORF32& multiLayerParallaxData = pixelCG.ParamPS<XMVECTORF32, 27>();

			multiLayerParallaxData.f[0] = *(float *)(v3 + 184);
			multiLayerParallaxData.f[1] = *(float *)(v3 + 188);
			multiLayerParallaxData.f[2] = *(float *)(v3 + 192);
			multiLayerParallaxData.f[3] = *(float *)(v3 + 196);
		}

		sub_14130C470(*(uintptr_t *)(v3 + 168), v3);
		sub_14130C4D0(*(uintptr_t *)(v3 + 176), v3);

		// PS: p21 float4 EnvmapData
		{
			XMVECTORF32& envmapData = pixelCG.ParamPS<XMVECTORF32, 21>();

			envmapData.f[0] = *(float *)(v3 + 200);
			envmapData.f[1] = (*(uintptr_t *)(v3 + 176)) ? 1.0f : 0.0f;
		}
	}
	break;

	case RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW:
	{
		// PS: p26 float4 SparkleParams
		XMVECTORF32& sparkleParams = pixelCG.ParamPS<XMVECTORF32, 26>();

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
			XMVECTORF32& color2 = vertexCG.ParamVS<XMVECTORF32, 9>();

			color2.f[0] = *(float *)(v3 + 180);
			color2.f[1] = *(float *)(v3 + 184);
			color2.f[2] = *(float *)(v3 + 188);
		}

		// VS: p10 float4 Color3
		{
			XMVECTORF32& color3 = vertexCG.ParamVS<XMVECTORF32, 10>();

			color3.f[0] = *(float *)(v3 + 192);
			color3.f[1] = *(float *)(v3 + 196);
			color3.f[2] = *(float *)(v3 + 200);
		}

		// PS: p21 float4 EnvmapData
		{
			XMVECTORF32& envmapData = pixelCG.ParamPS<XMVECTORF32, 21>();

			envmapData.f[0] = *(float *)(v3 + 176);
			envmapData.f[1] = (*(uintptr_t *)(v3 + 168)) ? 1.0f : 0.0f;
		}
	}
	break;
	}

	// VS: p11 float4 Velocity
	{
		XMVECTORF32& velocity = vertexCG.ParamVS<XMVECTORF32, 11>();

		velocity.f[0] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 12);
		velocity.f[1] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 16);
		velocity.f[2] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 28);
		velocity.f[3] = *(float *)(v3 + 8i64 * (unsigned int)dword_141E33040 + 32);
	}

	if (rawTechnique & RAW_FLAG_SPECULAR)
	{
		// PS: p25 float4 SpecularColor
		XMVECTORF32& specularColor = pixelCG.ParamPS<XMVECTORF32, 25>();

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
		pixelCG.ParamPS<XMVECTORF32, 6>() = xmmword_141E3301C;
	}

	if (rawTechnique & RAW_FLAG_SOFT_LIGHTING)
	{
		uint32_t v71 = *(uint32_t *)(v3 + 112);
		NiTexture *v72 = *(NiTexture **)(*(uintptr_t *)(v3 + 96) + 72i64);

		BSGraphics::Renderer::SetShaderResource(12, v72 ? v72->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(12, v71);

		// PS: p28 float4 LightingEffectParams
		XMVECTORF32& lightingEffectParams = pixelCG.ParamPS<XMVECTORF32, 28>();

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
		XMVECTORF32& lightingEffectParams = pixelCG.ParamPS<XMVECTORF32, 28>();

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
		XMVECTORF32& snowRimLightParameters = pixelCG.ParamPS<XMVECTORF32, 34>();

		snowRimLightParameters.f[0] = flt_141E35380;// fSnowRimLightIntensity
		snowRimLightParameters.f[1] = flt_141E35398;// fSnowGeometrySpecPower
		snowRimLightParameters.f[2] = flt_141E353B0;// fSnowNormalSpecPower
		snowRimLightParameters.f[3] = (byte_141E353C8) ? 1.0f : 0.0f;// bEnableSnowRimLighting
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
		XMVECTORF32& iblParams = pixelCG.ParamPS<XMVECTORF32, 29>();

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
		XMVECTORF32& characterLightParams = pixelCG.ParamPS<XMVECTORF32, 35>();

		// if (bEnableCharacterRimLighting)
		if (byte_141E32F66)
			characterLightParams.v = _mm_loadu_ps(&xmmword_141E3302C);
	}

	BSGraphics::Renderer::FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);
}

void BSLightingShader::RestoreMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSLightingShader::RestoreMaterial, Material);
}

// BSUtilities::GetInverseWorldMatrix(const NiTransform& Transform, bool UseInputTransform, D3DMATRIX& Matrix)
void GetInverseWorldMatrix(const NiTransform& Transform, bool UseWorldPosition, XMMATRIX& OutMatrix)
{
	auto *renderer = GetThreadedGlobals();

	if (UseWorldPosition)
	{
		// XMMatrixIdentity(), row[3] = { world.x, world.y, world.z, 1.0f }
		XMMATRIX worldTranslation = XMMatrixTranslation(
			*(float *)&renderer->__zz2[28],
			*(float *)&renderer->__zz2[32],
			*(float *)&renderer->__zz2[36]);

		OutMatrix = XMMatrixInverse(nullptr, worldTranslation);
	}
	else
	{
		NiTransform inverted;
		Transform.Invert(inverted);

		OutMatrix = BSShaderUtil::GetXMFromNiPosAdjust(inverted, NiPoint3(0.0f, 0.0f, 0.0f));
	}
}

void BSLightingShader::SetupGeometry(BSRenderPass *Pass, uint32_t Flags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSLightingShader::SetupGeometry, Pass, Flags);

	auto *renderer = GetThreadedGlobals();
	auto vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	const uint32_t rawTechnique = TLS_m_CurrentRawTechnique;
	const uint32_t baseTechniqueID = (rawTechnique >> 24) & 0x3F;

	bool doPrecipitationOcclusion = false;
	bool isLOD = false;

	uintptr_t v3 = (uintptr_t)Pass->m_Property;

	uint8_t v102 = ~(unsigned __int8)(rawTechnique >> 1) & 1;
	int v16 = 0;
	uint8_t v12 = Pass->Byte1C;
	uint8_t v103 = (unsigned __int8)(v12 - 2) <= 1u;

	if (v12 == 3 && *(uintptr_t *)(v3 + 56) & 0x100000000i64)
	{
		TLS_dword_141E35280 = *(uint32_t *)&renderer->__zz0[72];
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(1);
	}

	switch (baseTechniqueID)
	{
	case RAW_TECHNIQUE_ENVMAP:
	case RAW_TECHNIQUE_MULTILAYERPARALLAX:
	case RAW_TECHNIQUE_EYE:
	{
		// NOTE: The game updates view projection twice...? See the if() after this switch.
		if ((rawTechnique & RAW_FLAG_SKINNED) == 0)
			GeoUpdateViewProjectionConstants(vertexCG, Pass->m_Geometry->GetWorldTransform(), false, nullptr);

		v16 = 0;
		v102 = 0;
		doPrecipitationOcclusion = true;

		// PS: p7 float4 MaterialData (NOTE: This is written AGAIN)
		auto& var = pixelCG.ParamPS<XMVECTORF32, 7>();
		var.f[0] = *(float *)(v3 + 260);
	}
	break;

	case RAW_TECHNIQUE_MTLAND:
	case RAW_TECHNIQUE_MTLANDLODBLEND:
		GeoUpdateMTLandExtraConstants(
			vertexCG,
			Pass->m_Geometry->GetWorldTranslate(),
			*(float *)(*(uintptr_t *)(v3 + 120) + 264i64),
			*(float *)(*(uintptr_t *)(v3 + 120) + 268i64));
		v16 = v102;
		break;

	case RAW_TECHNIQUE_LODLAND:
	case RAW_TECHNIQUE_LODOBJ:
	case RAW_TECHNIQUE_LODOBJHD:
		GeoUpdateViewProjectionConstants(vertexCG, Pass->m_Geometry->GetWorldTransform(), false, nullptr);
		GeoUpdateViewProjectionConstants(vertexCG, Pass->m_Geometry->GetWorldTransform(), true, (NiPoint3 *)&renderer->__zz2[40]);
		isLOD = true;
		v16 = v102;
		break;

	case RAW_TECHNIQUE_TREE:
		sub_14130BC60(vertexCG, Pass->m_Property);
		v16 = v102;
		break;

	default:
		v16 = v102;
		break;
	}

	if ((rawTechnique & RAW_FLAG_SKINNED) == 0 && !isLOD)
	{
		// Unknown renderer flag: Determines if previous world projection is used
		const NiTransform& world = Pass->m_Geometry->GetWorldTransform();
		const NiTransform& temp = (Flags & 0x10) ? world : Pass->m_Geometry->GetPreviousWorldTransform();

		GeoUpdateViewProjectionConstants(vertexCG, world, false, nullptr);
		GeoUpdateViewProjectionConstants(vertexCG, temp, true, (NiPoint3 *)&renderer->__zz2[40]);// BSGraphics::Renderer::QViewData(BSGraphics::gRenderer)->kViewProjMat?
	}

	XMMATRIX inverseWorldMatrix;
	GetInverseWorldMatrix(Pass->m_Geometry->GetWorldTransform(), false, inverseWorldMatrix);

	GeoUpdateDirectionalLightConstants(pixelCG, Pass, inverseWorldMatrix, v16);
	GeoUpdateAmbientLightConstants(pixelCG, Pass->m_Geometry->GetWorldTransform(), v16);

	// PS: p7 float4 MaterialData
	{
		XMVECTORF32& materialData = pixelCG.ParamPS<XMVECTORF32, 7>();

		float v30 = *(float *)(v3 + 48);
		bool wtf = (signed char)Pass->Byte1E >= 0;

		wtf = !wtf;

		if (!(wtf == false))
			materialData.f[2] = v30 * *(float *)(*(uintptr_t *)(v3 + 96) + 332i64);
		else
			materialData.f[2] = v30;
	}

	GeoUpdateEmitColorConstants(pixelCG, Pass->m_Property);

	uint32_t lightCount = (rawTechnique >> 3) & 7;
	uint32_t shadowLightCount = (rawTechnique >> 6) & 7;

	// PS: p0 float2 NumLightNumShadowLight
	{
		XMFLOAT2& numLightNumShadowLight = pixelCG.ParamPS<XMFLOAT2, 0>();

		numLightNumShadowLight.x = (float)lightCount;
		numLightNumShadowLight.y = (float)shadowLightCount;
	}

	//
	// PS: p1 float4 PointLightPosition[7]
	// PS: p2 float4 PointLightColor[7]
	//
	// Original code just memset()'s these but it's already zeroed now. The values get
	// set up in the relevant function (GeoUpdatePointLightConstants).
	//
	//{
	//	auto& pointLightPosition = pixelCG.Param<XMVECTOR[7], 1>(ps);
	//	auto& pointLightColor = pixelCG.Param<XMVECTOR[7], 2>(ps);
	//
	//	memset(&pointLightPosition, 0, sizeof(XMVECTOR) * 7);
	//	memset(&pointLightColor, 0, sizeof(XMVECTOR) * 7);
	//}

	if (lightCount > 0)
	{
		float scale = Pass->m_Geometry->GetWorldTransform().m_fScale;

		if (baseTechniqueID == RAW_TECHNIQUE_ENVMAP || baseTechniqueID == RAW_TECHNIQUE_EYE)
			scale = 1.0f;

		GeoUpdatePointLightConstants(pixelCG, Pass, inverseWorldMatrix, lightCount, shadowLightCount, scale, v102);
	}

	if (rawTechnique & RAW_FLAG_SPECULAR)
	{
		// PS: p7 float4 MaterialData (NOTE: This is written TWICE)
		auto& var = pixelCG.ParamPS<XMVECTORF32, 7>();
		var.f[1] = *(float *)(v3 + 256);

		doPrecipitationOcclusion = true;
	}

	if (rawTechnique & (RAW_FLAG_SOFT_LIGHTING | RAW_FLAG_RIM_LIGHTING | RAW_FLAG_BACK_LIGHTING | RAW_FLAG_AMBIENT_SPECULAR))
		doPrecipitationOcclusion = true;

	bool enableProjectedUvNormals = byte_141E35308 && (!(Flags & 0x8) || !byte_141E35320);

	if ((rawTechnique & RAW_FLAG_PROJECTED_UV) && (baseTechniqueID != RAW_TECHNIQUE_HAIR))
	{
		NiTexture *v54 = *(NiTexture **)(qword_143052890 + 72);

		BSGraphics::Renderer::SetShaderResource(11, v54 ? v54->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureMode(11, 3, 1);

		if (enableProjectedUvNormals && qword_143052898)
		{
			NiTexture *v57 = *(NiTexture **)(qword_143052898 + 72);

			BSGraphics::Renderer::SetShaderResource(3, v57 ? v57->QRendererTexture() : nullptr);
			BSGraphics::Renderer::SetTextureMode(3, 3, 1);

			NiTexture *v58 = *(NiTexture **)(qword_1430528A0 + 72);

			BSGraphics::Renderer::SetShaderResource(8, v58 ? v58->QRendererTexture() : nullptr);
			BSGraphics::Renderer::SetTextureMode(8, 3, 1);

			NiTexture *v59 = *(NiTexture **)(qword_1430528A8 + 72);

			BSGraphics::Renderer::SetShaderResource(10, v59 ? v59->QRendererTexture() : nullptr);
			BSGraphics::Renderer::SetTextureMode(10, 3, 1);
		}

		// IDA says there are 2 args to this virtual function, it's probably wrong
		if ((*(__int64(__fastcall **)(BSGeometry *))(*(uintptr_t *)Pass->m_Geometry + 424i64))(Pass->m_Geometry))
		{
			float *v60 = (float *)Pass->m_Geometry;

			// VS: p6 float3x4 fVars3
			float *fVars3 = &vertexCG.ParamVS<float, 6>();

			fVars3[0] = v60[91];
			fVars3[1] = v60[95];
			fVars3[2] = v60[99];
			fVars3[3] = v60[103];

			fVars3[4] = v60[92];
			fVars3[5] = v60[96];
			fVars3[6] = v60[100];
			fVars3[7] = v60[104];

			fVars3[8] = v60[93];
			fVars3[9] = v60[97];
			fVars3[10] = v60[101];
			fVars3[11] = v60[105];

			GeoUpdateProjectedUvConstants(pixelCG, Pass->m_Geometry, Pass->m_Property, enableProjectedUvNormals);
		}
		else
		{
			XMMATRIX outputTemp;
			sub_14130C8A0(Pass->m_Geometry->GetWorldTransform(), outputTemp, baseTechniqueID == RAW_TECHNIQUE_ENVMAP);

			// VS: p6 float3x4 fVars3
			BSShaderUtil::TransposeStoreMatrix3x4(&vertexCG.ParamVS<float, 6>(), outputTemp);

			GeoUpdateProjectedUvConstants(pixelCG, nullptr, Pass->m_Property, enableProjectedUvNormals);
		}
	}

	if (rawTechnique & RAW_FLAG_WORLD_MAP)
	{
		NiTexture *v77 = *(NiTexture **)(qword_141E32F90 + 72);

		BSGraphics::Renderer::SetShaderResource(12, v77 ? v77->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(12, 3);

		NiTexture *v80 = *(NiTexture **)(qword_141E32F98 + 72);

		BSGraphics::Renderer::SetShaderResource(13, v80 ? v80->QRendererTexture() : nullptr);
		BSGraphics::Renderer::SetTextureAddressMode(13, 3);

		// VS: p8 float4 Color1
		BSGraphics::Utility::CopyNiColorAToFloat(&vertexCG.ParamVS<XMVECTOR, 8>(), dword_1431F5540);

		// PS: p17 float4 WorldMapOverlayParametersPS
		BSGraphics::Utility::CopyNiColorAToFloat(&pixelCG.ParamPS<XMVECTOR, 17>(), dword_1431F5550);
	}

	// This block is an inlined function
	if (doPrecipitationOcclusion)
	{
		// VS: p2 float3 PrecipitationOcclusionWorldViewProj
		XMFLOAT3& precipitationOcclusionWorldViewProj = vertexCG.ParamVS<XMFLOAT3, 2>();

		if (v102 == 1)
		{
			float *v83 = (float *)GetCurrentAccumulator();
			XMVECTOR coord = XMVectorSet(v83[91], v83[92], v83[93], 0.0f);

			XMStoreFloat3(&precipitationOcclusionWorldViewProj, XMVector3TransformCoord(coord, inverseWorldMatrix));
		}
		else
		{
			float *v86 = (float *)GetCurrentAccumulator();

			// Equivalent to XMMatrixTranslation(x, y, z) -- missing rows/cols are multiplied in shader code
			precipitationOcclusionWorldViewProj.x = v86[91] - *(float *)&renderer->__zz2[28];
			precipitationOcclusionWorldViewProj.y = v86[92] - *(float *)&renderer->__zz2[32];
			precipitationOcclusionWorldViewProj.z = v86[93] - *(float *)&renderer->__zz2[36];
		}
	}

	if (Pass->Byte1C == 10)
	{
		uintptr_t v89 = *(uintptr_t *)(v3 + 96);
		float v90;

		if ((signed char)Pass->Byte1E >= 0)
			v90 = *(float *)(v89 + 304) * 31.0f;
		else
			v90 = *(float *)(v89 + 332) * 31.0f;

		BSGraphics::Renderer::DepthStencilStateSetStencilMode(11, ((uint32_t)v90) & 0xFF);
	}

	if (!v103)
	{
		uint32_t oldDepthMode = *(uint32_t *)&renderer->__zz0[32];

		if (!(*(uint64_t *)(v3 + 56) & 0x100000000i64))
		{
			TLS_dword_141E35280 = oldDepthMode;
			BSGraphics::Renderer::DepthStencilStateSetDepthMode(1);
		}

		if (!(*(uint64_t *)(v3 + 56) & 0x80000000))
		{
			TLS_dword_141E35280 = oldDepthMode;
			BSGraphics::Renderer::DepthStencilStateSetDepthMode(0);
		}

	}

	// PS: p16 float4 SSRParams
	{
		XMVECTORF32& ssrParams = pixelCG.ParamPS<XMVECTORF32, 16>();

		ssrParams.f[0] = flt_141E34C70;
		ssrParams.f[1] = flt_141E34C88 + flt_141E34C70;
		ssrParams.f[2] = flt_143257C40;

		float v98 = 0.0f;
		float v99 = 0.0f;

		if (rawTechnique & RAW_FLAG_SPECULAR)
			v99 = *(float *)(v3 + 256);

		if ((Flags & 2) == 0)
			v98 = 1.0f;

		ssrParams.f[3] = v98 * v99;
	}

	BSGraphics::Renderer::FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
}

void BSLightingShader::RestoreGeometry(BSRenderPass *Pass)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSLightingShader::RestoreGeometry, Pass);

	if (Pass->Byte1C == 10)
		BSGraphics::Renderer::DepthStencilStateSetStencilMode(0, 255);

	if (TLS_dword_141E35280 != 6)
	{
		BSGraphics::Renderer::DepthStencilStateSetDepthMode(TLS_dword_141E35280);
		TLS_dword_141E35280 = 6;
	}

	if (TLS_dword_141E3527C != 13)
	{
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(TLS_dword_141E3527C);
		TLS_dword_141E3527C = 13;
	}
}

uint32_t BSLightingShader::GetRawTechnique(uint32_t Technique)
{
	uint32_t outputTech = Technique - 0x4800002D;
	uint32_t subIndex = (outputTech >> 24) & 0x3F;

	if (subIndex == RAW_TECHNIQUE_LODLANDNOISE && !byte_141E32E89)
	{
		outputTech = outputTech & 0xC9FFFFFF | 0x9000000;
	}
	else if (subIndex == RAW_TECHNIQUE_PARALLAXOCC && !byte_141E352F0)
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

void BSLightingShader::TechUpdateAccelerationConstants(BSGraphics::ConstantGroup<BSVertexShader>& VertexCG)
{
	auto *renderer = GetThreadedGlobals();

	// VS: p12 float4 Acceleration
	BSGraphics::Utility::CopyNiColorAToFloat(&VertexCG.ParamVS<XMVECTOR, 12>(),
		NiColorA(
			flt_141E32F54 - *(float *)&renderer->__zz2[28],
			flt_141E32F58 - *(float *)&renderer->__zz2[32],
			flt_141E32F5C - 15.0f,
			flt_141E32F60 - 15.0f));
}

void BSLightingShader::TechUpdateFogWindConstants(BSGraphics::ConstantGroup<BSVertexShader>& VertexCG, BSGraphics::ConstantGroup<BSPixelShader>& PixelCG)
{
	auto sub_1412AC860 = (uintptr_t(__fastcall *)(BYTE))(g_ModuleBase + 0x12AC860);
	uintptr_t fogParams = sub_1412AC860(byte_141E32FE0);

	if (!fogParams)
		return;

	// Set both vertex & pixel here (incorrect names?)
	{
		XMVECTORF32 wind;
		wind.f[0] = *(float *)(fogParams + 56);
		wind.f[1] = *(float *)(fogParams + 60);
		wind.f[2] = *(float *)(fogParams + 64);
		wind.f[3] = flt_141E32FBC;

		// VS: p14 float4 Wind
		VertexCG.ParamVS<XMVECTORF32, 14>() = wind;

		// PS: p19 float4 FogColor
		PixelCG.ParamPS<XMVECTORF32, 19>() = wind;
	}

	// VS: p15 float4 UNKNOWN_NAME
	XMVECTORF32& UNKNOWN_PARAM = VertexCG.ParamVS<XMVECTORF32, 15>();
	UNKNOWN_PARAM.f[0] = *(float *)(fogParams + 68);
	UNKNOWN_PARAM.f[1] = *(float *)(fogParams + 72);
	UNKNOWN_PARAM.f[2] = *(float *)(fogParams + 76);
	UNKNOWN_PARAM.f[3] = 0.0f;

	// VS: p13 float4 ScaleAdjust
	XMVECTORF32& scaleAdjust = VertexCG.ParamVS<XMVECTORF32, 13>();

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

void BSLightingShader::sub_14130C470(__int64 a1, __int64 a2)
{
	NiTexture *v2 = *(NiTexture **)(a1 + 72);
	uint32_t v3 = *(uint32_t *)(a2 + 112);

	BSGraphics::Renderer::SetShaderResource(4, v2 ? v2->QRendererTexture() : nullptr);
	BSGraphics::Renderer::SetTextureAddressMode(4, v3);
}

void BSLightingShader::sub_14130C4D0(__int64 a1, __int64 a2)
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

void BSLightingShader::sub_14130C220(int a1, __int64 a2, __int64 a3)
{
	NiTexture *v3 = *(NiTexture **)(a2 + 72);
	uint32_t v4 = *(uint32_t *)(a3 + 112);

	BSGraphics::Renderer::SetShaderResource(a1, v3 ? v3->QRendererTexture() : nullptr);
	BSGraphics::Renderer::SetTextureAddressMode(a1, v4);
}

void BSLightingShader::MatSetMultiTextureLandOverrides(__int64 a1)
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

__int64 BSLightingShader::sub_141314170(__int64 a1)
{
	return *(unsigned int *)(a1 + 8);
}

void BSLightingShader::GeoUpdateViewProjectionConstants(BSGraphics::ConstantGroup<BSVertexShader>& VertexCG, const NiTransform& Transform, bool IsPreviousWorld, const NiPoint3 *PosAdjust)
{
	//
	// Instead of using the typical 4x4 matrix like everywhere else, someone decided that the
	// lighting shader is going to use 3x4 matrices. The missing 4th column is assumed to be
	// { 0, 0, 0, 1 } in row-major form.
	//
	XMMATRIX projMatrix;

	if (PosAdjust)
		projMatrix = BSShaderUtil::GetXMFromNiPosAdjust(Transform, *PosAdjust);
	else
		projMatrix = BSShaderUtil::GetXMFromNi(Transform);

	//
	// VS: p0 float3x4 WorldViewProj
	// -- or --
	// VS: p1 float3x4 PrevWorldViewProj
	//
	if (!IsPreviousWorld)
		BSShaderUtil::TransposeStoreMatrix3x4(&VertexCG.ParamVS<float, 0>(), projMatrix);
	else
		BSShaderUtil::TransposeStoreMatrix3x4(&VertexCG.ParamVS<float, 1>(), projMatrix);
}

SRWLOCK asdf = SRWLOCK_INIT;

void BSLightingShader::GeoUpdateMTLandExtraConstants(const BSGraphics::ConstantGroup<BSVertexShader>& VertexCG, const NiPoint3& Translate, float a3, float a4)
{
	float v4 = 0.0f;
	float v6 = (flt_141E32F40 - flt_141E32FD8) / (flt_141E32FB8 * 5.0f);

	if (v6 >= 0.0f)
		v4 = fmin(1.0f, v6);

	/*
	FO4 code:

	v7 = NiPoint2::operator-(&unk_10194FD8, &v10, &unk_10194FD0, &v11, LODWORD(v6));
	v8 = NiPoint2::operator*(v7);
	NiPoint2::operator+(&unk_10194FD0, &v12, v8);
	*/

	// v11 might be wildly incorrect (swap?): v11 = _mm_unpacklo_ps(LODWORD(xmmword_141E32FC8[0]), LODWORD(xmmword_141E32FC8[1]));
	AcquireSRWLockExclusive(&asdf);
	XMFLOAT2 v11;
	v11.x = xmmword_141E32FC8.x;
	v11.y = xmmword_141E32FC8.y;

	float v9 = xmmword_141E32FC8.z - xmmword_141E32FC8.x;
	float v10 = xmmword_141E32FC8.w - xmmword_141E32FC8.y;

	*(XMFLOAT2 *)&xmmword_141E32FC8 = v11;
	ReleaseSRWLockExclusive(&asdf);

	v11.x += (v9 * v4);
	v11.y += (v10 * v4);

	if (v4 == 1.0f)
		byte_1431F547C = 0;

	// VS: p3 float4 fVars0
	XMVECTORF32& fVars0 = VertexCG.ParamVS<XMVECTORF32, 3>();

	fVars0.f[0] = a3;
	fVars0.f[1] = a4;
	fVars0.f[2] = v11.x - Translate.x;
	fVars0.f[3] = v11.y - Translate.y;
}

void BSLightingShader::sub_14130BC60(const BSGraphics::ConstantGroup<BSVertexShader>& VertexCG, BSShaderProperty *Property)
{
	// __int64 __fastcall sub_14130BC60(__int64 a1, __int64 a2)

	struct tempbufdata
	{
		char _pad[8];
		void *ptr;
	} temp;

	temp.ptr = VertexCG.m_Map.pData;

	auto sub_14130BC60 = (void(__fastcall *)(tempbufdata *, BSShaderProperty *))(g_ModuleBase + 0x130BC60);
	sub_14130BC60(&temp, Property);
}

void BSLightingShader::GeoUpdateDirectionalLightConstants(const BSGraphics::ConstantGroup<BSPixelShader>& PixelCG, const BSRenderPass *Pass, XMMATRIX& a3, int a4)
{
	uintptr_t v7 = 0;

	if (*(BYTE *)((uintptr_t)Pass + 31))
		v7 = *(uintptr_t *)((uintptr_t)Pass + 56);

	float *v10 = *(float **)(*(uintptr_t *)v7 + 72i64);
	float v12 = *(float *)(qword_1431F5810 + 224) * v10[77];

	// PS: p4 float3 DirLightColor
	XMFLOAT3& dirLightColor = PixelCG.ParamPS<XMFLOAT3, 4>();

	dirLightColor.x = v12 * v10[71];
	dirLightColor.y = v12 * v10[72];
	dirLightColor.z = v12 * v10[73];

	// PS: p3 float3 DirLightDirection
	XMFLOAT3& dirLightDirection = PixelCG.ParamPS<XMFLOAT3, 3>();
	XMVECTOR tempDir = XMVectorSet(-v10[80], -v10[81], -v10[82], 0.0f);

	if (a4 == 1)
		tempDir = XMVector3TransformNormal(tempDir, a3);

	XMStoreFloat3(&dirLightDirection, XMVector3Normalize(tempDir));
}

void BSLightingShader::GeoUpdateAmbientLightConstants(const BSGraphics::ConstantGroup<BSPixelShader>& PixelCG, const NiTransform& Transform, int a3)
{
	// __int64 __fastcall sub_14130B2A0(__int64 a1, __int64 a2, int a3)

	struct tempbufdata
	{
		char _pad[8];
		void *ptr;
	} temp;

	temp.ptr = PixelCG.m_Map.pData;

	auto GeoUpdateAmbientLightConstants = (void(__fastcall *)(tempbufdata *, const NiTransform&, int))(g_ModuleBase + 0x130B2A0);
	GeoUpdateAmbientLightConstants(&temp, Transform, a3);
}

void BSLightingShader::GeoUpdateEmitColorConstants(const BSGraphics::ConstantGroup<BSPixelShader>& PixelCG, BSShaderProperty *Property)
{
	// PS: p8 float4 EmitColor
	XMVECTORF32& emitColor = PixelCG.ParamPS<XMVECTORF32, 8>();

	float *v33 = (float *)((uintptr_t)Property + 240);
	float v31 = *(float *)((uintptr_t)Property + 248);

	emitColor.f[0] = v33[0] * v31;
	emitColor.f[1] = v33[1] * v31;
	emitColor.f[2] = v33[2] * v31;
}

void BSLightingShader::GeoUpdatePointLightConstants(const BSGraphics::ConstantGroup<BSPixelShader>& PixelCG, BSRenderPass *Pass, XMMATRIX& Transform, uint32_t LightCount, uint32_t ShadowLightCount, float Scale, int a7)
{
	// __int64 __fastcall sub_14130B390(__int64 a1, __int64 a2, __int64 a3, int a4, int a5, float a6, int a7)

	struct tempbufdata
	{
		char _pad[8];
		void *ptr;
	} temp;

	temp.ptr = PixelCG.m_Map.pData;

	auto GeoUpdatePointLightConstants = (void(__fastcall *)(tempbufdata *, BSRenderPass *Pass, XMMATRIX& Transform, uint32_t LightCount, uint32_t ShadowLightCount, float Scale, int a7))(g_ModuleBase + 0x130B390);
	GeoUpdatePointLightConstants(&temp, Pass, Transform, LightCount, ShadowLightCount, Scale, a7);
}

void BSLightingShader::GeoUpdateProjectedUvConstants(const BSGraphics::ConstantGroup<BSPixelShader>& PixelCG, BSGeometry *Geometry, BSShaderProperty *Property, bool EnableProjectedNormals)
{
	// PS: p12 float4 ProjectedUVParams
	{
		XMVECTORF32& projectedUVParams = PixelCG.ParamPS<XMVECTORF32, 12>();

		float *v6 = (float *)((uintptr_t)Geometry + 444);

		if (!Geometry)
			v6 = (float *)((uintptr_t)Property + 67);

		float v8 = 1.0f - v6[3];

		projectedUVParams.f[0] = v8 * v6[0];
		//projectedUVParams.f[1] = ;
		projectedUVParams.f[2] = v6[2];
		projectedUVParams.f[3] = (v8 * v6[1]) + v6[3];
	}

	// PS: p13 float4 ProjectedUVParams2
	{
		XMVECTORF32& projectedUVParams2 = PixelCG.ParamPS<XMVECTORF32, 13>();

		if (Geometry)
		{
			projectedUVParams2.f[0] = *(float *)((uintptr_t)Geometry + 464);// Reversed on purpose?
			projectedUVParams2.f[1] = *(float *)((uintptr_t)Geometry + 460);// Reversed on purpose?
		}
		else
		{
			projectedUVParams2.f[0] = *(float *)((uintptr_t)Property + 284);
			projectedUVParams2.f[1] = *(float *)((uintptr_t)Property + 288);
			projectedUVParams2.f[2] = *(float *)((uintptr_t)Property + 292);
			projectedUVParams2.f[3] = *(float *)((uintptr_t)Property + 296);
		}
	}

	// PS: p14 float4 ProjectedUVParams3
	{
		XMVECTORF32& projectedUVParams3 = PixelCG.ParamPS<XMVECTORF32, 14>();

		projectedUVParams3.f[0] = flt_141E35350;// fProjectedUVDiffuseNormalTilingScale
		projectedUVParams3.f[1] = flt_141E35368;// fProjectedUVNormalDetailTilingScale
		projectedUVParams3.f[2] = 0.0f;
		projectedUVParams3.f[3] = (EnableProjectedNormals) ? 1.0f : 0.0f;
	}
}

void BSLightingShader::sub_14130C8A0(const NiTransform& Transform, XMMATRIX& OutMatrix, bool DontMultiply)
{
	auto *renderer = GetThreadedGlobals();

	NiTransform temp;

	temp.m_Rotate.m_pEntry[0][0] = 0.0f;
	temp.m_Rotate.m_pEntry[0][1] = 1.0f;
	temp.m_Rotate.m_pEntry[0][2] = 0.0f;

	temp.m_Rotate.m_pEntry[1][0] = -1.0f;
	temp.m_Rotate.m_pEntry[1][1] = 0.0f;
	temp.m_Rotate.m_pEntry[1][2] = 0.0f;

	temp.m_Rotate.m_pEntry[2][0] = 0.0f;
	temp.m_Rotate.m_pEntry[2][1] = 0.0f;
	temp.m_Rotate.m_pEntry[2][2] = 1.0f;

	temp.m_Translate.x = *(float *)&renderer->__zz2[28];
	temp.m_Translate.y = *(float *)&renderer->__zz2[32];
	temp.m_Translate.z = *(float *)&renderer->__zz2[36];

	temp.m_fScale = 1.0f;

	if (DontMultiply)
	{
		OutMatrix = BSShaderUtil::GetXMFromNi(temp);
	}
	else
	{
		XMMATRIX m1 = BSShaderUtil::GetXMFromNi(temp);
		XMMATRIX m2 = BSShaderUtil::GetXMFromNi(Transform);

		// out = Translate(m2, (NiPoint3 *)&renderer.__zz2[28]) * m1; -- operator order DOES matter
		m2.r[3] = XMVectorAdd(m2.r[3], XMVectorSet(
			*(float *)&renderer->__zz2[28],
			*(float *)&renderer->__zz2[32],
			*(float *)&renderer->__zz2[36],
			0.0f));

		OutMatrix = XMMatrixMultiply(m2, m1);
	}
}