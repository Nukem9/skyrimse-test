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
#include "../../NiMain/NiSourceTexture.h"
#include "../BSShaderAccumulator.h"
#include "BSGrassShader.h"

//
// Shader notes:
//
// - Destructor is not implemented
// - UpdateFogParameters() was modified to remove the use of global variables (TLS_FogNearColor)
// - SetupGeometry() same as above
//
using namespace DirectX;

thread_local XMVECTOR TLS_FogNearColor;

AutoPtr(bool, bUseEarlyZ, 0x30528E5);
AutoPtr(BYTE, byte_14304E4C5, 0x304E4C5);
AutoPtr(BYTE, byte_141E32E9D, 0x1E32E9D);
AutoPtr(BYTE, byte_141E32F65, 0x1E32F65);
AutoPtr(uint32_t, dword_141E338A0, 0x1E338A0);
AutoPtr(uintptr_t, qword_14304F260, 0x304F260);
AutoPtr(NiSourceTexture *, qword_1430528F0, 0x30528F0);
AutoPtr(uintptr_t, qword_141E32F20, 0x1E32F20);
AutoPtr(BYTE, byte_141E32FE0, 0x1E32FE0);

AutoPtr(float, flt_1431F6198, 0x31F6198);
AutoPtr(float, flt_1431F619C, 0x31F619C);
AutoPtr(float, flt_141E32F50, 0x1E32F50);
AutoPtr(float, flt_1431F63E8, 0x31F63E8);
AutoPtr(float, flt_141E33358, 0x1E33358);
AutoPtr(float, flt_141E33370, 0x1E33370);
AutoPtr(float, flt_141E32FBC, 0x1E32FBC);

void TestHook4()
{
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E4C10), &BSGrassShader::__ctor__);
}

BSGrassShader::BSGrassShader() : BSShader("RunGrass")
{
	m_Type = 1;
	pInstance = this;
}

BSGrassShader::~BSGrassShader()
{
	__debugbreak();
}

bool BSGrassShader::SetupTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSGrassShader::SetupTechnique, Technique);

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTechnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTechnique, false))
		return false;

	// Fog params get stored in TLS, read in SetupGeometry()
	UpdateFogParameters();

	BSGraphics::Renderer::SetTextureFilterMode(0, 2);

	if (rawTechnique != RAW_TECHNIQUE_RENDERDEPTH && byte_141E32E9D)
	{
		BSGraphics::Renderer::SetShaderResource(1, (ID3D11ShaderResourceView *)qword_14304F260);

		BSGraphics::Renderer::SetTextureAddressMode(1, 0);
		BSGraphics::Renderer::SetTextureFilterMode(1, (dword_141E338A0 != 4) ? 1 : 0);
	}
	else
	{
		BSGraphics::Renderer::SetTexture(1, qword_1430528F0->QRendererTexture());// ShadowMaskSampler
		BSGraphics::Renderer::SetTextureMode(1, 0, 0);
	}

	return true;
}

void BSGrassShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSGrassShader::RestoreTechnique, Technique);
}

void BSGrassShader::SetupMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSGrassShader::SetupMaterial, Material);

	NiSourceTexture *baseTexture = *(NiSourceTexture **)((uintptr_t)Material + 72);

	BSGraphics::Renderer::SetTexture(0, baseTexture->QRendererTexture());// BaseSampler
	BSGraphics::Renderer::SetTextureAddressMode(0, 0);
}

void BSGrassShader::RestoreMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSGrassShader::RestoreMaterial, Material);
}

void BSGrassShader::SetupGeometry(BSRenderPass *Pass, uint32_t Flags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSGrassShader::SetupGeometry, Pass, Flags);

	auto *renderer = GetThreadedGlobals();
	uintptr_t geometry = (uintptr_t)Pass->m_Geometry;
	uintptr_t property = (uintptr_t)Pass->m_Property;

	auto vertexCG = BSGraphics::Renderer::GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto data = (VertexConstantData *)vertexCG.RawData();

#if BSSHADER_FORWARD_DEBUG
	// Copy original game data to local buffer
	memcpy(data, (void *)(g_ModuleBase + 0x31F6400), sizeof(VertexConstantData));
#else
	// Sanity check: the original global struct should have zero data
	const static char zeroData[sizeof(VertexConstantData)] = { 0 };

	if (memcmp(&zeroData, (void *)(g_ModuleBase + 0x31F6400), sizeof(zeroData)) != 0)
		throw "BUG: This structure MUST be zero";
#endif

	UpdateGeometryProjections(data, Pass->m_Geometry->GetWorldTransform());
	data->FogNearColor = TLS_FogNearColor;

	if (byte_141E32F65)
	{
		data->AmbientColor[0] = 1.0f;
		data->AmbientColor[1] = 1.0f;
		data->AmbientColor[2] = 1.0f;

		data->DirLightColor[0] = 0.0f;
		data->DirLightColor[1] = 0.0f;
		data->DirLightColor[2] = 0.0f;

		data->DirLightDirection[0] = 1.0f;
		data->DirLightDirection[1] = 0.0f;
		data->DirLightDirection[2] = 0.0f;
	}
	else
	{
		uintptr_t v9 = *(uintptr_t *)(*(uintptr_t *)(qword_141E32F20 + 512) + 72i64);
		uintptr_t v13 = v9 + 0x11C;

		float v10 = *(float *)(v9 + 320);
		float v11 = *(float *)(v9 + 324);
		float v12 = *(float *)(v9 + 328);

		if (!v9)
		{
			data->AmbientColor[0] = 0.0f;
			data->AmbientColor[1] = 0.0f;
			data->AmbientColor[2] = 0.0f;

			data->DirLightColor[0] = 0.0f;
			data->DirLightColor[1] = 0.0f;
			data->DirLightColor[2] = 0.0f;

			throw std::invalid_argument("v9 must be non-null");
		}

		data->AmbientColor[0] = *(float *)(v9 + 272);
		data->AmbientColor[1] = *(float *)(v9 + 276);
		data->AmbientColor[2] = *(float *)(v9 + 280);

		data->DirLightColor[0] = *(float *)(v13 + 0);
		data->DirLightColor[1] = *(float *)(v13 + 4);
		data->DirLightColor[2] = *(float *)(v13 + 8);

		data->DirLightDirection[0] = -v10;
		data->DirLightDirection[1] = -v11;
		data->DirLightDirection[2] = -v12;
	}

	data->AlphaParam1 = flt_1431F6198;
	data->AlphaParam2 = flt_1431F619C;

	float windTimer = ((flt_141E32F50 / 600.0f) * DirectX::XM_2PI) * *(float *)(property + 388);
	float windDirZ = min(60.0f, flt_1431F63E8);

	// NiTransform::XMLoad()?
	alignas(16) float v25[4][4];
	v25[0][0] = *(float *)(geometry + 124) * *(float *)(geometry + 172);
	v25[0][1] = *(float *)(geometry + 136) * *(float *)(geometry + 172);
	v25[0][2] = *(float *)(geometry + 148) * *(float *)(geometry + 172);
	v25[1][0] = *(float *)(geometry + 128) * *(float *)(geometry + 172);
	v25[1][1] = *(float *)(geometry + 140) * *(float *)(geometry + 172);
	v25[1][2] = *(float *)(geometry + 152) * *(float *)(geometry + 172);
	v25[2][0] = *(float *)(geometry + 132) * *(float *)(geometry + 172);
	v25[2][1] = *(float *)(geometry + 144) * *(float *)(geometry + 172);
	v25[2][2] = *(float *)(geometry + 156) * *(float *)(geometry + 172);
	v25[3][0] = *(float *)(geometry + 160);
	v25[3][1] = *(float *)(geometry + 164);
	v25[3][2] = *(float *)(geometry + 168);
	v25[0][3] = 0.0f;
	v25[1][3] = 0.0f;
	v25[2][3] = 0.0f;
	v25[3][3] = 1.0f;

	DirectX::XMVECTORF32 windVecNormals;
	windVecNormals.f[0] = 0.0f;
	windVecNormals.f[1] = 1.0f;
	windVecNormals.f[2] = 0.0f;
	windVecNormals.v = XMVector3Normalize(XMVector3TransformNormal(windVecNormals, XMMatrixInverse(nullptr, *(XMMATRIX *)&v25)));

	data->WindVector[0] = windVecNormals.f[0];
	data->WindVector[1] = windVecNormals.f[1];
	data->WindVector[2] = windDirZ * flt_141E33358;
	data->WindTimer = windTimer;

	if (!byte_14304E4C5)
	{
		if (Flags & 0x10)
		{
			data->PreviousWindTimer = windTimer;
		}
		else
		{
			//
			// This code was originally a simple assignment. It's an atomic swap now:
			//
			// float temp = *(float *)(property + 392);
			// *(float *)(property + 392) = windTimer;
			// data.PreviousWindTimer = temp;
			//
			uint32_t oldTimer		= InterlockedExchange((volatile LONG *)(property + 392), *(LONG *)&windTimer);
			data->PreviousWindTimer	= *(float *)&oldTimer;
		}
	}

	if ((*(uintptr_t *)(property + 56) & 0x80000000000i64) == 0)
	{
		data->ScaleMask[0] = 0.0f;
		data->ScaleMask[1] = 0.0f;
		data->ScaleMask[2] = 1.0f;
	}
	else
	{
		data->ScaleMask[0] = 1.0f;
		data->ScaleMask[1] = 1.0f;
		data->ScaleMask[2] = 1.0f;
	}

	// Wtf? flt_141E33370 is 0.3 and not zero...?
	data->padding = flt_141E33370;

	BSGraphics::Renderer::FlushConstantGroupVSPS(&vertexCG, nullptr);
	BSGraphics::Renderer::ApplyConstantGroupVSPS(&vertexCG, nullptr, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	// Update constant buffer #8 containing InstanceData
	UpdateGeometryInstanceData(Pass->m_Geometry, Pass->m_Property);
}

void BSGrassShader::RestoreGeometry(BSRenderPass *Pass)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSGrassShader::RestoreGeometry, Pass);
}

void BSGrassShader::UpdateFogParameters()
{
	auto sub_1412AC860 = (uintptr_t(__fastcall *)(BYTE))(g_ModuleBase + 0x12AC860);
	uintptr_t fogParams = sub_1412AC860(byte_141E32FE0);

	if (!fogParams)
		return;

	// Default to black when no fog is active
	NiColorA color = NiColorA::BLACK;

	if (*(float *)(fogParams + 80) != 0.0f || *(float *)(fogParams + 84) != 0.0f)
		color = NiColorA(*(float *)(fogParams + 56), *(float *)(fogParams + 60), *(float *)(fogParams + 64), flt_141E32FBC);

	BSGraphics::Utility::CopyNiColorAToFloat(&TLS_FogNearColor, color);
}

void BSGrassShader::UpdateGeometryProjections(VertexConstantData *Data, const NiTransform& GeoTransform)
{
	auto *renderer = GetThreadedGlobals();
	XMMATRIX xmmGeoTransform = BSShaderUtil::GetXMFromNi(GeoTransform);

	Data->WorldViewProj = XMMatrixMultiplyTranspose(xmmGeoTransform, *(XMMATRIX *)&renderer->__zz2[240]);
	Data->WorldView = XMMatrixMultiplyTranspose(xmmGeoTransform, *(XMMATRIX *)&renderer->__zz2[112]);
	Data->World = XMMatrixTranspose(xmmGeoTransform);
	Data->PreviousWorld = XMMatrixTranspose(BSShaderUtil::GetXMFromNiPosAdjust(GeoTransform, *(NiPoint3 *)&renderer->__zz2[40]));
}

void BSGrassShader::UpdateGeometryInstanceData(const BSGeometry *Geometry, BSShaderProperty *Property)
{
	auto sub_1412E5820 = (void(__fastcall *)(BSGrassShader *, const BSGeometry *, BSShaderProperty *))(g_ModuleBase + 0x12E5820);
	sub_1412E5820(this, Geometry, Property);
}

uint32_t BSGrassShader::GetRawTechnique(uint32_t Technique)
{
	uint32_t outputTech = 0;

	switch (Technique)
	{
	case BSSM_GRASS_DIRONLY_LF:
	case BSSM_GRASS_NOALPHA_DIRONLY_LF:
		outputTech = RAW_TECHNIQUE_FLATL;
		break;

	case BSSM_GRASS_DIRONLY_LFS:
	case BSSM_GRASS_NOALPHA_DIRONLY_LFS:
		outputTech = RAW_TECHNIQUE_FLATL_SLOPE;
		break;

	case BSSM_GRASS_DIRONLY_LVS:
	case BSSM_GRASS_NOALPHA_DIRONLY_LVS:
		outputTech = RAW_TECHNIQUE_VERTEXL_SLOPE;
		break;

	case BSSM_GRASS_DIRONLY_LFB:
	case BSSM_GRASS_NOALPHA_DIRONLY_LFB:
		outputTech = RAW_TECHNIQUE_FLATL_BILLBOARD;
		break;

	case BSSM_GRASS_DIRONLY_LFSB:
	case BSSM_GRASS_NOALPHA_DIRONLY_LFSB:
		outputTech = RAW_TECHNIQUE_FLATL_SLOPE_BILLBOARD;
		break;

	case 0x5C00005C:
		outputTech = RAW_TECHNIQUE_RENDERDEPTH;
		break;

	default:
		// bAssert("BSGrassShader: bad technique ID");
		break;
	}

	if (bUseEarlyZ)
		outputTech |= RAW_FLAG_DO_ALPHA;

	return outputTech;
}

uint32_t BSGrassShader::GetVertexTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}

uint32_t BSGrassShader::GetPixelTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}