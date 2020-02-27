#include "../../../../common.h"
#include "../../NiMain/NiDirectionalLight.h"
#include "../../BSGraphicsState.h"
#include "../../TES.h"
#include "../../BSTArray.h"
#include "../../Setting.h"
#include "../BSLight.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSGrassShader.h"

DEFINE_SHADER_DESCRIPTOR(
	RunGrass,

	// Vertex
	CONFIG_ENTRY(VS, PER_GEO, 0, row_major float4x4,	WorldViewProj)
	CONFIG_ENTRY(VS, PER_GEO, 1, row_major float4x4,	WorldView)
	CONFIG_ENTRY(VS, PER_GEO, 2, row_major float4x4,	World)
	CONFIG_ENTRY(VS, PER_GEO, 3, row_major float4x4,	PreviousWorld)
	CONFIG_ENTRY(VS, PER_GEO, 4, float4,				FogNearColor)
	CONFIG_ENTRY(VS, PER_GEO, 5, float3,				WindVector)
	CONFIG_ENTRY(VS, PER_GEO, 6, float,					WindTimer)
	CONFIG_ENTRY(VS, PER_GEO, 7, float3,				DirLightDirection)
	CONFIG_ENTRY(VS, PER_GEO, 8, float,					PreviousWindTimer)
	CONFIG_ENTRY(VS, PER_GEO, 9, float3,				DirLightColor)
	CONFIG_ENTRY(VS, PER_GEO, 10, float,				AlphaParam1)
	CONFIG_ENTRY(VS, PER_GEO, 11, float3,				AmbientColor)
	CONFIG_ENTRY(VS, PER_GEO, 12, float,				AlphaParam2)
	CONFIG_ENTRY(VS, PER_GEO, 13, float3,				ScaleMask)
	CONFIG_ENTRY(VS, PER_GEO, 14, float,				ShadowClampValue)

	// Pixel
	CONFIG_ENTRY(PS, SAMPLER, 0, SamplerState,			SampBaseSampler)
	CONFIG_ENTRY(PS, SAMPLER, 1, SamplerState,			SampShadowMaskSampler)

	CONFIG_ENTRY(PS, TEXTURE, 0, Texture2D<float4>,		TexBaseSampler)
	CONFIG_ENTRY(PS, TEXTURE, 1, Texture2D<float4>,		TexShadowMaskSampler)
);

//
// Shader notes:
//
// - UpdateFogParameters() was modified to remove the use of global variables (TLS_FogNearColor)
// - SetupGeometry() same as above
//
using namespace DirectX;

AutoPtr(BYTE, byte_14304E4C5, 0x304E4C5);
AutoPtr(BYTE, byte_141E32E9D, 0x1E32E9D);// bShadowsOnGrass_Display
AutoPtr(BYTE, byte_141E32F65, 0x1E32F65);// BSShaderManager::bLiteBrite
AutoPtr(uintptr_t, qword_14304F260, 0x304F260);
AutoPtr(float, flt_1431F6198, 0x31F6198);// Fade parameter
AutoPtr(float, flt_1431F619C, 0x31F619C);// Fade parameter
AutoPtr(float, flt_141E32F50, 0x1E32F50);// Part of BSShaderManager timer array
AutoPtr(float, flt_1431F63E8, 0x31F63E8);

DefineIniSetting(iShadowMaskQuarter, Display);
DefineIniSetting(fShadowClampValue, Display);
DefineIniSetting(fWindGrassMultiplier, Display);

thread_local XMVECTOR TLS_FogNearColor;

BSGrassShader::BSGrassShader() : BSShader(ShaderConfigRunGrass.Type)
{
	ShaderMetadata[BSShaderManager::BSSM_SHADER_RUNGRASS] = &ShaderConfigRunGrass;

	pInstance = this;
	m_Type = BSShaderManager::BSSM_SHADER_RUNGRASS;
}

BSGrassShader::~BSGrassShader()
{
	pInstance = nullptr;
}

bool BSGrassShader::SetupTechnique(uint32_t Technique)
{
	auto renderer = BSGraphics::Renderer::GetGlobals();

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);

	if (!BeginTechnique(GetVertexTechnique(rawTechnique), GetPixelTechnique(rawTechnique), false))
		return false;

	// Fog params get stored in TLS, read in SetupGeometry()
	UpdateFogParameters();

	renderer->SetTextureFilterMode(TexSlot::Base, 2);

	if (rawTechnique != RAW_TECHNIQUE_RENDERDEPTH && byte_141E32E9D)
	{
		renderer->SetShaderResource(TexSlot::ShadowMask, (ID3D11ShaderResourceView *)qword_14304F260);

		renderer->SetTextureAddressMode(TexSlot::ShadowMask, 0);
		renderer->SetTextureFilterMode(TexSlot::ShadowMask, (iShadowMaskQuarter->uValue.i != 4) ? 1 : 0);
	}
	else
	{
		renderer->SetTexture(TexSlot::ShadowMask, BSGraphics::gState.pDefaultTextureWhite->QRendererTexture());
		renderer->SetTextureMode(TexSlot::ShadowMask, 0, 0);
	}

	return true;
}

void BSGrassShader::RestoreTechnique(uint32_t Technique)
{
}

void BSGrassShader::SetupMaterial(BSShaderMaterial const *Material)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	NiSourceTexture *baseTexture = *(NiSourceTexture **)((uintptr_t)Material + 72);

	renderer->SetTexture(TexSlot::Base, baseTexture->QRendererTexture());
	renderer->SetTextureAddressMode(TexSlot::Base, 0);
}

void BSGrassShader::RestoreMaterial(BSShaderMaterial const *Material)
{
}

void BSGrassShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	uintptr_t geometry = (uintptr_t)Pass->m_Geometry;
	uintptr_t property = (uintptr_t)Pass->m_ShaderProperty;

	auto renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto data = (VertexConstantData *)vertexCG.RawData();

	UpdateGeometryProjections(data, Pass->m_Geometry->GetWorldTransform());
	data->FogNearColor = TLS_FogNearColor;

	if (byte_141E32F65)
	{
		data->AmbientColor = NiColor::WHITE;
		data->DirLightColor = NiColor::BLACK;
		data->DirLightDirection = NiPoint3(1.0f, 0.0f, 0.0f);
	}
	else
	{
		NiDirectionalLight *sunLight = static_cast<NiDirectionalLight *>((*(BSLight **)(TES::qword_141E32F20 + 512))->GetLight());

		data->AmbientColor = sunLight->GetAmbientColor();
		data->DirLightColor = sunLight->GetDiffuseColor();
		data->DirLightDirection = -sunLight->GetWorldDirection();
	}

	data->AlphaParam1 = flt_1431F6198;
	data->AlphaParam2 = flt_1431F619C;

	float windTimer = ((flt_141E32F50 / 600.0f) * DirectX::XM_2PI) * *(float *)(property + 388);
	float windDirZ = std::min(60.0f, flt_1431F63E8);

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

	if (!Pass->m_ShaderProperty->GetFlag(BSShaderProperty::BSSP_FLAG_UNIFORM_SCALE))
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

	data->ShadowClampValue = fShadowClampValue->uValue.f;

	renderer->FlushConstantGroupVSPS(&vertexCG, nullptr);
	renderer->ApplyConstantGroupVSPS(&vertexCG, nullptr, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	// Update constant buffer #8 containing InstanceData
	UpdateGeometryInstanceData(Pass->m_Geometry, Pass->m_ShaderProperty);
}

void BSGrassShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
}

void BSGrassShader::UpdateFogParameters()
{
	uintptr_t fogParams = (uintptr_t)BSShaderManager::GetFogProperty(TES::byte_141E32FE0);

	if (!fogParams)
		return;

	// Default to black when no fog is active
	NiColorA color = NiColorA::BLACK;

	if (*(float *)(fogParams + 80) != 0.0f || *(float *)(fogParams + 84) != 0.0f)
		color = NiColorA(*(float *)(fogParams + 56), *(float *)(fogParams + 60), *(float *)(fogParams + 64), TES::flt_141E32FBC);

	BSGraphics::Utility::CopyNiColorAToFloat(&TLS_FogNearColor, color);
}

void BSGrassShader::UpdateGeometryProjections(VertexConstantData *Data, const NiTransform& GeoTransform)
{
	auto renderer = BSGraphics::Renderer::GetGlobals();
	XMMATRIX xmmGeoTransform = BSShaderUtil::GetXMFromNi(GeoTransform);

	Data->WorldViewProj = XMMatrixMultiplyTranspose(xmmGeoTransform, renderer->m_CameraData.m_ViewProjMat);
	Data->WorldView = XMMatrixMultiplyTranspose(xmmGeoTransform, renderer->m_CameraData.m_ViewMat);
	Data->World = XMMatrixTranspose(xmmGeoTransform);
	Data->PreviousWorld = XMMatrixTranspose(BSShaderUtil::GetXMFromNiPosAdjust(GeoTransform, renderer->m_PreviousPosAdjust));
}

void BSGrassShader::UpdateGeometryInstanceData(const BSGeometry *Geometry, BSShaderProperty *Property)
{
	auto renderer = BSGraphics::Renderer::GetGlobals();

	BSTArray<float> *propertyInstanceData = (BSTArray<float> *)((uintptr_t)Property + 0x160);
	uint32_t instanceDataCount = propertyInstanceData->QSize();

	AssertMsg(instanceDataCount <= (3840 / 4), "Grass instance group count is too large. It does not fit in register size.");

	// TODO/WARNING: There is another data race hazard in this function. Properties are supposed to be unique though?
	AutoFunc(void(__fastcall *)(BSShaderProperty *, uint32_t), sub_1412E0810, 0x12E0810);
	sub_1412E0810(Property, instanceDataCount);

	uint32_t neededSize = instanceDataCount * sizeof(float);
	auto constantGroup = renderer->GetShaderConstantGroup(neededSize, BSGraphics::CONSTANT_GROUP_LEVEL_INSTANCE);

	// Already zero initialized, just copy
	if (instanceDataCount > 0)
		memcpy(constantGroup.RawData(), propertyInstanceData->QBuffer(), neededSize);

	renderer->FlushConstantGroup(&constantGroup);
	renderer->ApplyConstantGroupVS(&constantGroup, BSGraphics::CONSTANT_GROUP_LEVEL_INSTANCE);
}

void BSGrassShader::CreateAllShaders()
{
	static_assert(RAW_TECHNIQUE_VERTEXL == 0, "Please update this function to match the enum");
	static_assert(RAW_TECHNIQUE_RENDERDEPTH == 8, "Please update this function to match the enum");

	for (int i = RAW_TECHNIQUE_VERTEXL; i <= RAW_TECHNIQUE_RENDERDEPTH; i++)
	{
		CreateVertexShader(i);
		CreatePixelShader(i);

		CreateVertexShader(i | RAW_FLAG_DO_ALPHA);
		CreatePixelShader(i | RAW_FLAG_DO_ALPHA);
	}
}

void BSGrassShader::CreateVertexShader(uint32_t Technique)
{
	auto defines = GetSourceDefines(Technique);
	auto getConstant = [](int i) { return ShaderConfigRunGrass.ByConstantIndexVS.count(i) ? ShaderConfigRunGrass.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfigRunGrass.Type, defines, getConstant);
}

void BSGrassShader::CreatePixelShader(uint32_t Technique)
{
	auto defines = GetSourceDefines(Technique);
	auto getSampler = [](int i) { return ShaderConfigRunGrass.BySamplerIndex.count(i) ? ShaderConfigRunGrass.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfigRunGrass.ByConstantIndexPS.count(i) ? ShaderConfigRunGrass.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfigRunGrass.Type, defines, getSampler, getConstant);
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

	if (BSGraphics::gState.bUseEarlyZ)
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

std::vector<std::pair<const char *, const char *>> BSGrassShader::GetSourceDefines(uint32_t Technique)
{
	std::vector<std::pair<const char *, const char *>> defines;

	switch (Technique & ~RAW_FLAG_DO_ALPHA)
	{
	case RAW_TECHNIQUE_VERTEXL: defines.emplace_back("VERTLIT", ""); break;
	case RAW_TECHNIQUE_FLATL: break;
	case RAW_TECHNIQUE_FLATL_SLOPE: defines.emplace_back("SLOPE", ""); break;
	case RAW_TECHNIQUE_VERTEXL_SLOPE: defines.emplace_back("VERTLIT", ""); defines.emplace_back("SLOPE", ""); break;
	case RAW_TECHNIQUE_VERTEXL_BILLBOARD: defines.emplace_back("VERTLIT", ""); defines.emplace_back("SLOPE", ""); defines.emplace_back("BILLBOARD", ""); break;
	case RAW_TECHNIQUE_FLATL_BILLBOARD: defines.emplace_back("BILLBOARD", ""); break;
	case RAW_TECHNIQUE_FLATL_SLOPE_BILLBOARD: defines.emplace_back("SLOPE", ""); defines.emplace_back("BILLBOARD", ""); break;
	case RAW_TECHNIQUE_VERTEXL_SLOPE_BILLBOARD: defines.emplace_back("VERTLIT", ""); defines.emplace_back("SLOPE", ""); defines.emplace_back("BILLBOARD", ""); defines.emplace_back("RENDER_DEPTH", ""); break;
	case RAW_TECHNIQUE_RENDERDEPTH: defines.emplace_back("RENDER_DEPTH", ""); break;
	default: Assert(false); break;
	}

	if (Technique & RAW_FLAG_DO_ALPHA)
		defines.emplace_back("DO_ALPHA_TEST", "");

	return defines;
}

std::string BSGrassShader::GetTechniqueString(uint32_t Technique)
{
	std::string str;

	switch (Technique & ~RAW_FLAG_DO_ALPHA)
	{
	case RAW_TECHNIQUE_VERTEXL: str = "VertexL"; break;
	case RAW_TECHNIQUE_FLATL: str = "FlatL"; break;
	case RAW_TECHNIQUE_FLATL_SLOPE: str = "FlatL_Slope"; break;
	case RAW_TECHNIQUE_VERTEXL_SLOPE: str = "VertexL_Slope"; break;
	case RAW_TECHNIQUE_VERTEXL_BILLBOARD: str = "VertexL_Billboard"; break;
	case RAW_TECHNIQUE_FLATL_BILLBOARD: str = "FlatL_Billboard"; break;
	case RAW_TECHNIQUE_FLATL_SLOPE_BILLBOARD: str = "FlatL_Slope_Billboard"; break;
	case RAW_TECHNIQUE_VERTEXL_SLOPE_BILLBOARD: str = "VertexL_Slope_Billboard"; break;
	case RAW_TECHNIQUE_RENDERDEPTH: str = "RenderDepth"; break;
	default: Assert(false); break;
	}

	if (Technique & RAW_FLAG_DO_ALPHA)
		str += " AlphaTest";

	return str;
}