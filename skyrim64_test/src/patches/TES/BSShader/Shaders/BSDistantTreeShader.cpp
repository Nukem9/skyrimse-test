#include "../../../../common.h"
#include "../../BSGraphicsState.h"
#include "../../NiMain/NiSourceTexture.h"
#include "../../NiMain/NiDirectionalLight.h"
#include "../BSLight.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSDistantTreeShader.h"

DEFINE_SHADER_DESCRIPTOR(
	"DistantTree",

	// Vertex
	CONFIG_ENTRY(VS, PER_GEO, 0, undefined,				InstanceData)
	CONFIG_ENTRY(VS, PER_GEO, 1, row_major float4x4,	WorldViewProj)
	CONFIG_ENTRY(VS, PER_GEO, 2, row_major float4x4,	World)
	CONFIG_ENTRY(VS, PER_GEO, 3, row_major float4x4,	PreviousWorld)
	CONFIG_ENTRY(VS, PER_TEC, 4, float4,				FogParam)
	CONFIG_ENTRY(VS, PER_TEC, 5, float4,				FogNearColor)
	CONFIG_ENTRY(VS, PER_TEC, 6, float4,				FogFarColor)
	CONFIG_ENTRY(VS, PER_TEC, 7, float4,				DiffuseDir)
	CONFIG_ENTRY(VS, PER_GEO, 8, float,					IndexScale)

	// Pixel
	CONFIG_ENTRY(PS, PER_TEC, 0, float4,				DiffuseColor)
	CONFIG_ENTRY(PS, PER_TEC, 1, float4,				AmbientColor)

	CONFIG_ENTRY(PS, SAMPLER, 0, SamplerState,			SampDiffuse)

	CONFIG_ENTRY(PS, TEXTURE, 0, Texture2D<float4>,		TexDiffuse)
);

//
// Shader notes:
//
// - VS Parameter 0 is never set (InstanceData)
// - VS Parameter 7 is never set (DiffuseDir)
// - VS Parameter 8 is never set (IndexScale)
// - A global variable update was manually removed in SetupGeometry()
// - SetupTechnique() has an unknown pixel shader parameter 7 (TODO)
//
using namespace DirectX;

AutoPtr(NiSourceTexture *, WorldTreeLODAtlas, 0x32A7F58);
AutoPtr(BYTE, byte_141E32FE0, 0x1E32FE0);
AutoPtr(float, dword_141E32FBC, 0x1E32FBC);
AutoPtr(__int64, qword_141E32F20, 0x1E32F20);

BSDistantTreeShader::BSDistantTreeShader() : BSShader(ShaderConfig.Type)
{
	ShaderMetadata[BSShaderManager::BSSM_SHADER_DISTANTTREE] = &ShaderConfig;
	m_Type = BSShaderManager::BSSM_SHADER_DISTANTTREE;
	pInstance = this;
}

BSDistantTreeShader::~BSDistantTreeShader()
{
	pInstance = nullptr;
}

bool BSDistantTreeShader::SetupTechnique(uint32_t Technique)
{
	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTechnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTechnique, false))
		return false;

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

	// fogParams is of type NiFogProperty *
	uintptr_t fogParams = (uintptr_t)BSShaderManager::GetFogProperty(byte_141E32FE0);

	if (fogParams)
	{
		XMVECTOR& vs_fogParam = vertexCG.ParamVS<XMVECTOR, 4>();	// VS: p4 float4 FogParam
		XMVECTOR& vs_fogNearColor = vertexCG.ParamVS<XMVECTOR, 5>();// VS: p5 float4 FogNearColor
		XMVECTOR& vs_fogFarColor = vertexCG.ParamVS<XMVECTOR, 6>();	// VS: p6 float4 FogFarColor

		float v19 = *(float *)(fogParams + 84);
		float v20 = *(float *)(fogParams + 80);
		float v21 = *(float *)(fogParams + 132);
		float v22 = *(float *)(fogParams + 136);

		if (v19 != 0.0f || v20 != 0.0f)
		{
			BSGraphics::Utility::CopyNiColorAToFloat(&vs_fogParam,
				NiColorA((1.0f / (v19 - v20)) * v20, 1.0f / (v19 - v20), v21, v22));

			// NiFogProperty::GetFogNearColor(v6);
			BSGraphics::Utility::CopyNiColorAToFloat(&vs_fogNearColor,
				NiColorA(*(float *)(fogParams + 56), *(float *)(fogParams + 60), *(float *)(fogParams + 64), dword_141E32FBC));

			// NiFogProperty::GetFogFarColor(v6);
			BSGraphics::Utility::CopyNiColorAToFloat(&vs_fogFarColor,
				NiColorA(*(float *)(fogParams + 68), *(float *)(fogParams + 72), *(float *)(fogParams + 76), 1.0f));
		}
		else
		{
			BSGraphics::Utility::CopyNiColorAToFloat(&vs_fogParam, NiColorA(500000.0f, 0.0f, v21, 0.0f));
			BSGraphics::Utility::CopyNiColorAToFloat(&vs_fogNearColor, NiColorA::BLACK);
			BSGraphics::Utility::CopyNiColorAToFloat(&vs_fogFarColor, NiColorA::BLACK);
		}
	}

	// Sun is always of type NiDirectionalLight *
	NiDirectionalLight *sunLight = static_cast<NiDirectionalLight *>((*(BSLight **)(qword_141E32F20 + 512))->GetLight());

	if (sunLight)
	{
		XMVECTOR& ps_DiffuseColor = pixelCG.ParamPS<XMVECTOR, 0>();	// PS: p0 float4 DiffuseColor
		XMVECTOR& ps_AmbientColor = pixelCG.ParamPS<XMVECTOR, 1>();	// PS: p1 float4 AmbientColor
		XMVECTOR& ps_Unknown = pixelCG.ParamPS<XMVECTOR, 7>();		// TODO: WHAT IS THIS PARAM??? It should be for vertex instead of pixel?

		BSGraphics::Utility::CopyNiColorAToFloat(&ps_DiffuseColor, NiColorA(sunLight->GetDiffuseColor(), 1.0f));
		BSGraphics::Utility::CopyNiColorAToFloat(&ps_AmbientColor, NiColorA(sunLight->GetAmbientColor(), dword_141E32FBC));

		// NiPoint3 normalizedDir = NiDirectionalLight::GetWorldDirection().Unitize();
		NiPoint3 normalizedDir = sunLight->GetWorldDirection();
		normalizedDir.Unitize();

		BSGraphics::Utility::CopyNiColorAToFloat(&ps_Unknown,
			NiColorA(-normalizedDir.x, -normalizedDir.y, -normalizedDir.z, 1.0f));
	}

	AssertMsg(WorldTreeLODAtlas, "LOD tree texture atlas for current worldspace not found.");

	renderer->SetTexture(0, WorldTreeLODAtlas->QRendererTexture());// Diffuse
	renderer->SetTextureAddressMode(0, 0);

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	return true;
}

void BSDistantTreeShader::RestoreTechnique(uint32_t Technique)
{
}

void BSDistantTreeShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	//
	// GetXMFromNiPosAdjust is a custom function to remove the original global variable
	// references...I'm copying what FO4 did and passing in another local argument.
	//
	// The original code copies a global Point3 (and restores it afterwards):
	//		flt_14304E20C = flt_14304E218;
	//		flt_14304E210 = flt_14304E21C;
	//		flt_14304E214 = flt_14304E220;
	//
	XMMATRIX geoTransform = BSShaderUtil::GetXMFromNi(Pass->m_Geometry->GetWorldTransform());
	XMMATRIX prevGeoTransform = BSShaderUtil::GetXMFromNiPosAdjust(Pass->m_Geometry->GetWorldTransform(), renderer->m_PreviousPosAdjust);

	//
	// VS: p1 float4x4 WorldViewProj
	// VS: p2 float4x4 World
	// VS: p3 float4x4 PreviousWorld
	//
	vertexCG.ParamVS<XMMATRIX, 1>() = XMMatrixMultiplyTranspose(geoTransform, renderer->m_ViewProjMat);
	vertexCG.ParamVS<XMMATRIX, 2>() = XMMatrixTranspose(geoTransform);
	vertexCG.ParamVS<XMMATRIX, 3>() = XMMatrixTranspose(prevGeoTransform);

	renderer->FlushConstantGroupVSPS(&vertexCG, nullptr);
	renderer->ApplyConstantGroupVSPS(&vertexCG, nullptr, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
}

void BSDistantTreeShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
}

void BSDistantTreeShader::CreateAllShaders()
{
	CreatePixelShader(RAW_TECHNIQUE_BLOCK);
	CreateVertexShader(RAW_TECHNIQUE_BLOCK);
	CreatePixelShader(RAW_TECHNIQUE_BLOCK | RAW_FLAG_DO_ALPHA);
	CreateVertexShader(RAW_TECHNIQUE_BLOCK | RAW_FLAG_DO_ALPHA);

	CreatePixelShader(RAW_TECHNIQUE_DEPTH);
	CreateVertexShader(RAW_TECHNIQUE_DEPTH);
	CreatePixelShader(RAW_TECHNIQUE_DEPTH | RAW_FLAG_DO_ALPHA);
	CreateVertexShader(RAW_TECHNIQUE_DEPTH | RAW_FLAG_DO_ALPHA);
}

void BSDistantTreeShader::CreateVertexShader(uint32_t Technique)
{
	auto getDefines = BSShaderInfo::BSDistantTreeShader::Defines::GetArray(Technique);
	auto getConstant = [](int i) { return ShaderConfig.ByConstantIndexVS.count(i) ? ShaderConfig.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfig.Type, getDefines, getConstant);
}

void BSDistantTreeShader::CreatePixelShader(uint32_t Technique)
{
	auto getDefines = BSShaderInfo::BSDistantTreeShader::Defines::GetArray(Technique);
	auto getSampler = [](int i) { return ShaderConfig.BySamplerIndex.count(i) ? ShaderConfig.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfig.ByConstantIndexPS.count(i) ? ShaderConfig.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfig.Type, getDefines, getSampler, getConstant);
}

uint32_t BSDistantTreeShader::GetRawTechnique(uint32_t Technique)
{
	uint32_t outputTech = 0;

	switch (Technique)
	{
	case BSSM_DISTANTTREE:outputTech = RAW_TECHNIQUE_BLOCK; break;
	case BSSM_DISTANTTREE_DEPTH:outputTech = RAW_TECHNIQUE_DEPTH; break;
	default:AssertMsg(false, "BSDistantTreeShader: bad technique ID"); break;
	}

	if (BSGraphics::gState.bUseEarlyZ)
		outputTech |= RAW_FLAG_DO_ALPHA;
	
	return outputTech;
}

uint32_t BSDistantTreeShader::GetVertexTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}

uint32_t BSDistantTreeShader::GetPixelTechnique(uint32_t RawTechnique)
{
	return RawTechnique;
}

std::vector<std::pair<const char *, const char *>> BSDistantTreeShader::GetSourceDefines(uint32_t Technique)
{
	// FIXME
	return BSShaderInfo::BSDistantTreeShader::Defines::GetArray(Technique);
}