#include "../../../../common.h"
#include "../../NiMain/NiSourceTexture.h"
#include "../../BSTArray.h"
#include "../../Setting.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSGrassShader.h"

//
// Shader notes:
//
// - Destructor is not implemented
// - UpdateFogParameters() was modified to remove the use of global variables (TLS_FogNearColor)
// - SetupGeometry() same as above
//
using namespace DirectX;

AutoPtr(NiSourceTexture *, DefaultWhiteMap, 0x30528F0);
AutoPtr(bool, bUseEarlyZ, 0x30528E5);
AutoPtr(BYTE, byte_14304E4C5, 0x304E4C5);
AutoPtr(BYTE, byte_141E32E9D, 0x1E32E9D);
AutoPtr(BYTE, byte_141E32F65, 0x1E32F65);
AutoPtr(uintptr_t, qword_14304F260, 0x304F260);
AutoPtr(uintptr_t, qword_141E32F20, 0x1E32F20);
AutoPtr(BYTE, byte_141E32FE0, 0x1E32FE0);
AutoPtr(float, flt_1431F6198, 0x31F6198);
AutoPtr(float, flt_1431F619C, 0x31F619C);
AutoPtr(float, flt_141E32F50, 0x1E32F50);
AutoPtr(float, flt_1431F63E8, 0x31F63E8);
AutoPtr(float, flt_141E32FBC, 0x1E32FBC);

DefineIniSetting(iShadowMaskQuarter, Display);
DefineIniSetting(fShadowClampValue, Display);
DefineIniSetting(fWindGrassMultiplier, Display);

thread_local XMVECTOR TLS_FogNearColor;

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
	Assert(false);
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

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	renderer->SetTextureFilterMode(0, 2);

	if (rawTechnique != RAW_TECHNIQUE_RENDERDEPTH && byte_141E32E9D)
	{
		renderer->SetShaderResource(1, (ID3D11ShaderResourceView *)qword_14304F260);// ShadowMaskSampler

		renderer->SetTextureAddressMode(1, 0);
		renderer->SetTextureFilterMode(1, (iShadowMaskQuarter->uValue.i != 4) ? 1 : 0);
	}
	else
	{
		renderer->SetTexture(1, DefaultWhiteMap->QRendererTexture());// ShadowMaskSampler
		renderer->SetTextureMode(1, 0, 0);
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

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	NiSourceTexture *baseTexture = *(NiSourceTexture **)((uintptr_t)Material + 72);

	renderer->SetTexture(0, baseTexture->QRendererTexture());// BaseSampler
	renderer->SetTextureAddressMode(0, 0);
}

void BSGrassShader::RestoreMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSGrassShader::RestoreMaterial, Material);
}

void BSGrassShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSGrassShader::SetupGeometry, Pass, RenderFlags);

	uintptr_t geometry = (uintptr_t)Pass->m_Geometry;
	uintptr_t property = (uintptr_t)Pass->m_Property;

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto data = (VertexConstantData *)vertexCG.RawData();

#if BSSHADER_FORWARD_DEBUG
	// Copy original game data to local buffer
	memcpy(data, (void *)(g_ModuleBase + 0x31F6400), sizeof(VertexConstantData));
#else
	// Sanity check
	const static char zeroData[sizeof(VertexConstantData)] = { 0 };

	AssertMsg(memcmp(&zeroData, (void *)(g_ModuleBase + 0x31F6400), sizeof(zeroData)) == 0, "BUG: This structure MUST be zeroed. Someone wrote data to it.");
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

		AssertMsg(v9, "Game expects v9 to be non-null");

		float v10 = *(float *)(v9 + 320);
		float v11 = *(float *)(v9 + 324);
		float v12 = *(float *)(v9 + 328);

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
	data->WindVector[2] = windDirZ * fWindGrassMultiplier->uValue.f;
	data->WindTimer = windTimer;

	if (!byte_14304E4C5)
	{
		if (RenderFlags & 0x10)
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

	if (!Pass->m_Property->GetFlag(BSShaderProperty::BSSP_FLAG_UNIFORM_SCALE))
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

	// Wtf? padding is 0.3 and not zero...?
	data->padding = fShadowClampValue->uValue.f;

	renderer->FlushConstantGroupVSPS(&vertexCG, nullptr);
	renderer->ApplyConstantGroupVSPS(&vertexCG, nullptr, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	// Update constant buffer #8 containing InstanceData
	UpdateGeometryInstanceData(Pass->m_Geometry, Pass->m_Property);
}

void BSGrassShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSGrassShader::RestoreGeometry, Pass, RenderFlags);
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
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	XMMATRIX xmmGeoTransform = BSShaderUtil::GetXMFromNi(GeoTransform);

	Data->WorldViewProj = XMMatrixMultiplyTranspose(xmmGeoTransform, *(XMMATRIX *)&renderer->__zz2[240]);
	Data->WorldView = XMMatrixMultiplyTranspose(xmmGeoTransform, *(XMMATRIX *)&renderer->__zz2[112]);
	Data->World = XMMatrixTranspose(xmmGeoTransform);
	Data->PreviousWorld = XMMatrixTranspose(BSShaderUtil::GetXMFromNiPosAdjust(GeoTransform, renderer->m_PreviousPosAdjust));
}

void BSGrassShader::UpdateGeometryInstanceData(const BSGeometry *Geometry, BSShaderProperty *Property)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();

	BSTArray<float> *propertyInstanceData = (BSTArray<float> *)((uintptr_t)Property + 0x160);
	uint32_t instanceDataCount = propertyInstanceData->QSize();

	AssertMsg(instanceDataCount <= (3840 / 4), "Grass instance group count is too large. It does not fit in register size.");

	// TODO/WARNING: There is another data race hazard in this function. Properties are supposed to be unique though?
	auto sub_1412E0810 = (void(__fastcall *)(BSShaderProperty *, uint32_t))(g_ModuleBase + 0x12E0810);
	sub_1412E0810(Property, instanceDataCount);

	uint32_t neededSize = instanceDataCount * sizeof(float);
	auto constantGroup = renderer->GetShaderConstantGroup(neededSize, BSGraphics::CONSTANT_GROUP_LEVEL_INSTANCE);

	// Already zero initialized, just copy
	if (instanceDataCount > 0)
		memcpy(constantGroup.RawData(), propertyInstanceData->QBuffer(), neededSize);

	renderer->FlushConstantGroup(&constantGroup);
	renderer->ApplyConstantGroupVS(&constantGroup, BSGraphics::CONSTANT_GROUP_LEVEL_INSTANCE);
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
		AssertMsg(false, "BSGrassShader: bad technique ID");
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