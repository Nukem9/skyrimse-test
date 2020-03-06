#include "../../../../common.h"
#include "../../TES.h"
#include "../../BSGraphics/BSGraphicsRenderer.h"
#include "../../BSGraphics/BSGraphicsUtility.h"
#include "../../NiMain/NiSourceTexture.h"
#include "../../NiMain/NiDirectionalLight.h"
#include "../BSLight.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "BSDistantTreeShader.h"

DEFINE_SHADER_DESCRIPTOR(
	DistantTree,

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

BSDistantTreeShader::BSDistantTreeShader() : BSShader(ShaderConfigDistantTree.Type)
{
	ShaderMetadata[BSShaderManager::BSSM_SHADER_DISTANTTREE] = &ShaderConfigDistantTree;

	pInstance = this;
	m_Type = BSShaderManager::BSSM_SHADER_DISTANTTREE;
}

BSDistantTreeShader::~BSDistantTreeShader()
{
	pInstance = nullptr;
}

bool BSDistantTreeShader::SetupTechnique(uint32_t Technique)
{
	auto renderer = BSGraphics::Renderer::QInstance();
	auto state = renderer->GetRendererShadowState();

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);

	if (!BeginTechnique(GetVertexTechnique(rawTechnique), GetPixelTechnique(rawTechnique), false))
		return false;

	auto vertexCG = renderer->GetShaderConstantGroup(state->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	auto pixelCG = renderer->GetShaderConstantGroup(state->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

	// fogParams is of type NiFogProperty *
	uintptr_t fogParams = (uintptr_t)BSShaderManager::GetFogProperty(TES::byte_141E32FE0);

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
				NiColorA(*(float *)(fogParams + 56), *(float *)(fogParams + 60), *(float *)(fogParams + 64), BSShaderManager::St.fInvFrameBufferRange));

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
	NiDirectionalLight *sunLight = static_cast<NiDirectionalLight *>((*(BSLight **)((uintptr_t)BSShaderManager::St.pShadowSceneNode[0] + 512))->GetLight());

	if (sunLight)
	{
		XMVECTOR& ps_DiffuseColor = pixelCG.ParamPS<XMVECTOR, 0>();	// PS: p0 float4 DiffuseColor
		XMVECTOR& ps_AmbientColor = pixelCG.ParamPS<XMVECTOR, 1>();	// PS: p1 float4 AmbientColor
		XMVECTOR& ps_Unknown = pixelCG.ParamPS<XMVECTOR, 7>();		// TODO: WHAT IS THIS PARAM??? It should be for vertex instead of pixel?

		BSGraphics::Utility::CopyNiColorAToFloat(&ps_DiffuseColor, NiColorA(sunLight->GetDiffuseColor(), 1.0f));
		BSGraphics::Utility::CopyNiColorAToFloat(&ps_AmbientColor, NiColorA(sunLight->GetAmbientColor(), BSShaderManager::St.fInvFrameBufferRange));

		// NiPoint3 normalizedDir = NiDirectionalLight::GetWorldDirection().Unitize();
		NiPoint3 normalizedDir = sunLight->GetWorldDirection();
		normalizedDir.Unitize();

		BSGraphics::Utility::CopyNiColorAToFloat(&ps_Unknown,
			NiColorA(-normalizedDir.x, -normalizedDir.y, -normalizedDir.z, 1.0f));
	}

	AssertMsg(WorldTreeLODAtlas, "LOD tree texture atlas for current worldspace not found.");

	renderer->SetTexture(TexSlot::Diffuse, WorldTreeLODAtlas->QRendererTexture());
	renderer->SetTextureAddressMode(TexSlot::Diffuse, 0);

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	return true;
}

void BSDistantTreeShader::RestoreTechnique(uint32_t Technique)
{
}

void BSDistantTreeShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	auto renderer = BSGraphics::Renderer::QInstance();
	auto state = renderer->GetRendererShadowState();

	auto vertexCG = renderer->GetShaderConstantGroup(state->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

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
	XMMATRIX prevGeoTransform = BSShaderUtil::GetXMFromNiPosAdjust(Pass->m_Geometry->GetWorldTransform(), state->m_PreviousPosAdjust);

	//
	// VS: p1 float4x4 WorldViewProj
	// VS: p2 float4x4 World
	// VS: p3 float4x4 PreviousWorld
	//
	vertexCG.ParamVS<XMMATRIX, 1>() = XMMatrixMultiplyTranspose(geoTransform, state->m_CameraData.m_ViewProjMat);
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
	auto defines = GetSourceDefines(Technique);
	auto getConstant = [](int i) { return ShaderConfigDistantTree.ByConstantIndexVS.count(i) ? ShaderConfigDistantTree.ByConstantIndexVS.at(i)->Name : nullptr; };

	BSShader::CreateVertexShader(Technique, ShaderConfigDistantTree.Type, defines, getConstant);
}

void BSDistantTreeShader::CreatePixelShader(uint32_t Technique)
{
	auto defines = GetSourceDefines(Technique);
	auto getSampler = [](int i) { return ShaderConfigDistantTree.BySamplerIndex.count(i) ? ShaderConfigDistantTree.BySamplerIndex.at(i)->Name : nullptr; };
	auto getConstant = [](int i) { return ShaderConfigDistantTree.ByConstantIndexPS.count(i) ? ShaderConfigDistantTree.ByConstantIndexPS.at(i)->Name : nullptr; };

	BSShader::CreatePixelShader(Technique, ShaderConfigDistantTree.Type, defines, getSampler, getConstant);
}

uint32_t BSDistantTreeShader::GetRawTechnique(uint32_t Technique)
{
	uint32_t outputTech = 0;

	switch (Technique)
	{
	case BSSM_DISTANTTREE: outputTech = RAW_TECHNIQUE_BLOCK; break;
	case BSSM_DISTANTTREE_DEPTH: outputTech = RAW_TECHNIQUE_DEPTH; break;
	default: AssertMsg(false, "BSDistantTreeShader: bad technique ID"); break;
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
	std::vector<std::pair<const char *, const char *>> defines;

	switch (Technique & ~RAW_FLAG_DO_ALPHA)
	{
	case RAW_TECHNIQUE_BLOCK: break;
	case RAW_TECHNIQUE_DEPTH: defines.emplace_back("RENDER_DEPTH", ""); break;
	default: Assert(false); break;
	}

	if (Technique & RAW_FLAG_DO_ALPHA)
		defines.emplace_back("DO_ALPHA_TEST", "");

	return defines;
}

std::string BSDistantTreeShader::GetTechniqueString(uint32_t Technique)
{
	std::string str;

	switch (Technique & ~RAW_FLAG_DO_ALPHA)
	{
	case RAW_TECHNIQUE_BLOCK: str = "DistantTreeBlock"; break;
	case RAW_TECHNIQUE_DEPTH: str = "Depth"; break;
	default: Assert(false); break;
	}

	if (Technique & RAW_FLAG_DO_ALPHA)
		str += " AlphaTest";

	return str;
}