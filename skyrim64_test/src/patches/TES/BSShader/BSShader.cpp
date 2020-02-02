#include "../../../common.h"
#include "../MemoryContextTracker.h"
#include "../BSGraphicsRenderer.h"
#include "BSShaderManager.h"
#include "BSShader.h"
#include "BSShader_Dumper.h"
#include "Shaders/BSBloodSplatterShader.h"
#include "Shaders/BSDistantTreeShader.h"
#include "Shaders/BSGrassShader.h"
#include "Shaders/BSSkyShader.h"

std::unordered_map<uint32_t, BSGraphics::HullShader *> HullShaders;
std::unordered_map<uint32_t, BSGraphics::DomainShader *> DomainShaders;

bool BSShader::g_ShaderToggles[16][3];
const ShaderDescriptor *BSShader::ShaderMetadata[BSShaderManager::BSSM_SHADER_COUNT];

BSShader::BSShader(const char *LoaderType)
{
	m_LoaderType = LoaderType;
}

BSShader::~BSShader()
{
	AssertMsg(false, "Destructor not implemented");
}

void BSShader::SetupMaterial(BSShaderMaterial const *Material)
{
}

void BSShader::RestoreMaterial(BSShaderMaterial const *Material)
{
}

void BSShader::GetTechniqueName(uint32_t Technique, char *Buffer, uint32_t BufferSize)
{
}

void BSShader::ReloadShaders(bool Unknown)
{
	// Calls BSShader::DeleteShaders down the line, but it's not a virtual function
	AutoFunc(void(__fastcall *)(bool), sub_141336480, 0x1336490);
	sub_141336480(Unknown);
}

void BSShader::ReloadShaders(BSIStream *Stream)
{
	AutoFunc(void(__fastcall *)(BSIStream *), sub_141336490, 0x1336490);
	sub_141336490(Stream);
}

void BSShader::SetBoneMatrix(NiSkinInstance *SkinInstance, NiSkinPartition::Partition *Partition, const NiTransform *Transform)
{
	MemoryContextTracker tracker(MemoryContextTracker::RENDER_SYSTEM, "BSShader.cpp");

	if (!Partition || Partition->m_usBones == 0)
		return;

	auto *renderer = BSGraphics::Renderer::GetGlobals();

	if (GAME_TLS(NiSkinInstance *, 0x2A00) == SkinInstance)
		return;

	GAME_TLS(NiSkinInstance *, 0x2A00) = SkinInstance;

	// WARNING: Contains a global variable edit
	AutoFunc(void(__fastcall *)(NiSkinInstance *, const NiTransform *), sub_140D74600, 0x0D74600);
	sub_140D74600(SkinInstance, Transform);

	uint32_t v11 = (unsigned int)(3 * *(uint32_t *)((uintptr_t)SkinInstance->m_spSkinData + 88i64)) * 16;

	auto boneDataConstants = renderer->GetShaderConstantGroup(v11, BSGraphics::CONSTANT_GROUP_LEVEL_BONES);
	auto prevBoneDataConstants = renderer->GetShaderConstantGroup(v11, BSGraphics::CONSTANT_GROUP_LEVEL_PREVIOUS_BONES);

	memcpy_s(boneDataConstants.RawData(), v11, SkinInstance->m_pvBoneMatrices, v11);
	memcpy_s(prevBoneDataConstants.RawData(), v11, SkinInstance->m_pvPrevBoneMatrices, v11);

	renderer->FlushConstantGroup(&boneDataConstants);
	renderer->FlushConstantGroup(&prevBoneDataConstants);
	renderer->ApplyConstantGroupVS(&boneDataConstants, BSGraphics::CONSTANT_GROUP_LEVEL_BONES);
	renderer->ApplyConstantGroupVS(&prevBoneDataConstants, BSGraphics::CONSTANT_GROUP_LEVEL_PREVIOUS_BONES);
}

void BSShader::CreateVertexShader(uint32_t Technique, const char *SourceFile, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetConstant)
{
	// Build source disk path, hand off to D3D11
	wchar_t fxpPath[MAX_PATH];
	swprintf_s(fxpPath, L"C:\\myshaders\\%S.hlsl", SourceFile);

	BSGraphics::VertexShader *vertexShader = BSGraphics::Renderer::GetGlobals()->CompileVertexShader(fxpPath, Defines, GetConstant);

	auto e = m_VertexShaderTable.find(Technique);

	Assert(e != m_VertexShaderTable.end());

	if (!strstr(SourceFile, "DistantTree"))
	{
		BSGraphics::Renderer::GetGlobals()->ValidateShaderReplacement(e->m_Shader, vertexShader->m_Shader);

		for (int i = 0; i < 20; i++)
		{
			if (vertexShader->m_ConstantOffsets[i] == BSGraphics::INVALID_CONSTANT_BUFFER_OFFSET)
				continue;

			//if (vertexShader->m_ConstantOffsets[i] != e->m_ConstantOffsets[i])
			//	Assert(false);
		}
	}

	vertexShader->m_TechniqueID = e->m_TechniqueID;
	vertexShader->m_VertexDescription = e->m_VertexDescription;
	e.temphack(vertexShader);
}

void BSShader::CreatePixelShader(uint32_t Technique, const char *SourceFile, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetSampler, std::function<const char *(int Index)> GetConstant)
{
	// Build source disk path, hand off to D3D11
	wchar_t fxpPath[MAX_PATH];
	swprintf_s(fxpPath, L"C:\\myshaders\\%S.hlsl", SourceFile);

	BSGraphics::PixelShader *pixelShader = BSGraphics::Renderer::GetGlobals()->CompilePixelShader(fxpPath, Defines, GetSampler, GetConstant);

	auto e = m_PixelShaderTable.find(Technique);

	Assert(e != m_PixelShaderTable.end());

	if (!strstr(SourceFile, "DistantTree"))
	{
		BSGraphics::Renderer::GetGlobals()->ValidateShaderReplacement(e->m_Shader, pixelShader->m_Shader);

		for (int i = 0; i < 64; i++)
		{
			if (pixelShader->m_ConstantOffsets[i] == BSGraphics::INVALID_CONSTANT_BUFFER_OFFSET)
				continue;

			Assert(pixelShader->m_ConstantOffsets[i] == e->m_ConstantOffsets[i]);
		}
	}

	pixelShader->m_TechniqueID = e->m_TechniqueID;
	e.temphack(pixelShader);
}

void BSShader::CreateHullShader(uint32_t Technique, const char *SourceFile, const std::vector<std::pair<const char *, const char *>>& Defines)
{
	// Build source disk path, hand off to D3D11
	wchar_t fxpPath[MAX_PATH];
	swprintf_s(fxpPath, L"C:\\myshaders\\%S.hlsl", SourceFile);

	BSGraphics::HullShader *hullShader = BSGraphics::Renderer::GetGlobals()->CompileHullShader(fxpPath, Defines);

	HullShaders[Technique] = hullShader;
}

void BSShader::CreateDomainShader(uint32_t Technique, const char *SourceFile, const std::vector<std::pair<const char *, const char *>>& Defines)
{
	// Build source disk path, hand off to D3D11
	wchar_t fxpPath[MAX_PATH];
	swprintf_s(fxpPath, L"C:\\myshaders\\%S.hlsl", SourceFile);

	BSGraphics::DomainShader *domainShader = BSGraphics::Renderer::GetGlobals()->CompileDomainShader(fxpPath, Defines);

	DomainShaders[Technique] = domainShader;
}

void BSShader::hk_Load(BSIStream *Stream)
{
	// Load original shaders first
	(this->*Load)(Stream);
	/*
	// Dump everything for debugging
	for (auto itr = m_VertexShaderTable.begin(); itr != m_VertexShaderTable.end(); itr++)
	{
		auto bytecode = BSGraphics::Renderer::GetGlobals()->GetShaderBytecode(itr->m_Shader);

		VertexShaderDecoder d(m_LoaderType, *itr);
		d.SetShaderData(bytecode.first, bytecode.second);
		d.DumpShader();
	}

	for (auto itr = m_PixelShaderTable.begin(); itr != m_PixelShaderTable.end(); itr++)
	{
		auto bytecode = BSGraphics::Renderer::GetGlobals()->GetShaderBytecode(itr->m_Shader);

		PixelShaderDecoder d(m_LoaderType, *itr);
		d.SetShaderData(bytecode.first, bytecode.second);
		d.DumpShader();
	}

	// ...and then replace with custom ones
	if (this == BSBloodSplatterShader::pInstance)
		BSBloodSplatterShader::pInstance->CreateAllShaders();

	if (this == BSDistantTreeShader::pInstance)
		BSDistantTreeShader::pInstance->CreateAllShaders();

	if (this == BSGrassShader::pInstance)
		BSGrassShader::pInstance->CreateAllShaders();

	if (this == BSSkyShader::pInstance)
		BSSkyShader::pInstance->CreateAllShaders();
	*/

	if (strstr(m_LoaderType, "Lighting"))
	{
		for (auto itr = m_VertexShaderTable.begin(); itr != m_VertexShaderTable.end(); itr++)
		{
			// Apply to parallax shaders only
			if (((itr->m_TechniqueID >> 24) & 0x3F) != 3)
				continue;

			CreateHullShader(itr->m_TechniqueID, "Lighting", BSShaderInfo::BSLightingShader::Defines::GetArray(itr->m_TechniqueID));
			CreateDomainShader(itr->m_TechniqueID, "Lighting", BSShaderInfo::BSLightingShader::Defines::GetArray(itr->m_TechniqueID));
		}
	}
}

bool BSShader::BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader)
{
	BSGraphics::VertexShader *vertexShader = nullptr;
	BSGraphics::PixelShader *pixelShader = nullptr;
	BSGraphics::DomainShader *domainShader = nullptr;
	BSGraphics::HullShader *hullShader = nullptr;

	if (!m_VertexShaderTable.get(VertexShaderID, vertexShader))
		return false;

	if (!IgnorePixelShader && !m_PixelShaderTable.get(PixelShaderID, pixelShader))
		return false;

	if (HullShaders.count(VertexShaderID))
		hullShader = HullShaders[VertexShaderID];

	if (DomainShaders.count(VertexShaderID))
		domainShader = DomainShaders[VertexShaderID];

	// Vertex shader required, pixel shader optional (nullptr)
	auto globals = BSGraphics::Renderer::GetGlobals();

	globals->SetVertexShader(vertexShader);
	globals->SetPixelShader(pixelShader);
	globals->SetHullShader(hullShader);
	globals->SetDomainShader(domainShader);
	return true;
}

void BSShader::EndTechnique()
{
}

void BSShader::SetupGeometryAlphaBlending(const NiAlphaProperty *AlphaProperty, BSShaderProperty *ShaderProperty, bool a4)
{
	AutoFunc(void(__fastcall *)(BSShader *, const NiAlphaProperty *, BSShaderProperty *, bool), sub_1413360D0, 0x13360D0);
	sub_1413360D0(this, AlphaProperty, ShaderProperty, a4);
}

void BSShader::SetupAlphaTestRef(const NiAlphaProperty *AlphaProperty, BSShaderProperty *ShaderProperty)
{
	int alphaRef = (int)((float)AlphaProperty->GetTestRef() * ShaderProperty->GetAlpha());

	BSGraphics::Renderer::GetGlobals()->SetAlphaTestRef(alphaRef * (1.0f / 255.0f));
}

std::vector<std::pair<const char *, const char *>> BSShader::GetSourceDefines(uint32_t Type, uint32_t Technique)
{
	switch (Type)
	{
	case BSShaderManager::BSSM_SHADER_RUNGRASS:
		return BSGrassShader::GetSourceDefines(Technique);
	case BSShaderManager::BSSM_SHADER_SKY:
		return BSSkyShader::GetSourceDefines(Technique);
	case BSShaderManager::BSSM_SHADER_WATER:
		return BSShaderInfo::BSWaterShader::Defines::GetArray(Technique);
	case BSShaderManager::BSSM_SHADER_BLOODSPLATTER:
		return BSBloodSplatterShader::GetSourceDefines(Technique);
	//case BSShaderManager::BSSM_SHADER_IMAGESPACE:
	//	break;
	case BSShaderManager::BSSM_SHADER_LIGHTING:
		return BSShaderInfo::BSLightingShader::Defines::GetArray(Technique);
	case BSShaderManager::BSSM_SHADER_EFFECT:
		return BSShaderInfo::BSXShader::Defines::GetArray(Technique);
	case BSShaderManager::BSSM_SHADER_UTILITY:
		return BSShaderInfo::BSUtilityShader::Defines::GetArray(Technique);
	case BSShaderManager::BSSM_SHADER_DISTANTTREE:
		return BSDistantTreeShader::GetSourceDefines(Technique);
	case BSShaderManager::BSSM_SHADER_PARTICLE:
		return BSShaderInfo::BSParticleShader::Defines::GetArray(Technique);
	}

	// Return empty vector
	return std::vector<std::pair<const char *, const char *>>();
}

const char *BSShader::GetVariableType(uint32_t Type, const char *Name)
{
	auto *descriptor = ShaderMetadata[Type];

	if (descriptor)
	{
		for (const ShaderDescriptor::Entry& e : descriptor->AllEntries())
		{
			if (!strcmp(e.Name, Name))
				return e.DataType;
		}
	}

	return nullptr;
}

ShaderDescriptor::DeclType BSShader::GetVariableCategory(uint32_t Type, const char *Name)
{
	auto *descriptor = ShaderMetadata[Type];

	if (descriptor)
	{
		for (const ShaderDescriptor::Entry& e : descriptor->AllEntries())
		{
			if (!strcmp(e.Name, Name))
				return e.m_DeclType;
		}
	}

	return ShaderDescriptor::INVALID_DECL_TYPE;
}

const char *BSShader::GetVSConstantName(uint32_t Type, uint32_t Index)
{
	switch (Type)
	{
	case BSShaderManager::BSSM_SHADER_RUNGRASS:
		break;
	case BSShaderManager::BSSM_SHADER_SKY:
		break;
	case BSShaderManager::BSSM_SHADER_WATER:
		return BSShaderInfo::BSWaterShader::VSConstants::GetString(Index);
	case BSShaderManager::BSSM_SHADER_BLOODSPLATTER:
		break;
	//case BSShaderManager::BSSM_SHADER_IMAGESPACE:
	//	break;
	case BSShaderManager::BSSM_SHADER_LIGHTING:
		break;
	case BSShaderManager::BSSM_SHADER_EFFECT:
		return BSShaderInfo::BSXShader::VSConstants::GetString(Index);
	case BSShaderManager::BSSM_SHADER_UTILITY:
		return BSShaderInfo::BSUtilityShader::VSConstants::GetString(Index);
	case BSShaderManager::BSSM_SHADER_DISTANTTREE:
		break;
	case BSShaderManager::BSSM_SHADER_PARTICLE:
		return BSShaderInfo::BSParticleShader::VSConstants::GetString(Index);

	default:
		Assert(false);
		return nullptr;
	}

	if (ShaderMetadata[Type]->ByConstantIndexVS.count(Index))
		return ShaderMetadata[Type]->ByConstantIndexVS.at(Index)->Name;

	return nullptr;
}

const char *BSShader::GetPSConstantName(uint32_t Type, uint32_t Index)
{
	switch (Type)
	{
	case BSShaderManager::BSSM_SHADER_RUNGRASS:
		break;
	case BSShaderManager::BSSM_SHADER_SKY:
		break;
	case BSShaderManager::BSSM_SHADER_WATER:
		return BSShaderInfo::BSWaterShader::PSConstants::GetString(Index);
	case BSShaderManager::BSSM_SHADER_BLOODSPLATTER:
		break;
		//case BSShaderManager::BSSM_SHADER_IMAGESPACE:
		//	break;
	case BSShaderManager::BSSM_SHADER_LIGHTING:
		break;
	case BSShaderManager::BSSM_SHADER_EFFECT:
		return BSShaderInfo::BSXShader::PSConstants::GetString(Index);
	case BSShaderManager::BSSM_SHADER_UTILITY:
		return BSShaderInfo::BSUtilityShader::PSConstants::GetString(Index);
	case BSShaderManager::BSSM_SHADER_DISTANTTREE:
		break;
	case BSShaderManager::BSSM_SHADER_PARTICLE:
		return BSShaderInfo::BSParticleShader::PSConstants::GetString(Index);

	default:
		Assert(false);
		return nullptr;
	}

	if (ShaderMetadata[Type]->ByConstantIndexPS.count(Index))
		return ShaderMetadata[Type]->ByConstantIndexPS.at(Index)->Name;

	return nullptr;
}

const char *BSShader::GetPSSamplerName(uint32_t Type, uint32_t Index, uint32_t TechniqueID)
{
	switch (Type)
	{
	case BSShaderManager::BSSM_SHADER_RUNGRASS:
		break;
	case BSShaderManager::BSSM_SHADER_SKY:
		break;
	case BSShaderManager::BSSM_SHADER_WATER:
		return BSShaderInfo::BSWaterShader::Samplers::GetString(Index);
	case BSShaderManager::BSSM_SHADER_BLOODSPLATTER:
		break;
		//case BSShaderManager::BSSM_SHADER_IMAGESPACE:
		//	break;
	case BSShaderManager::BSSM_SHADER_LIGHTING:
		return BSShaderInfo::BSLightingShader::Samplers::GetString(Index, TechniqueID);
	case BSShaderManager::BSSM_SHADER_EFFECT:
		return BSShaderInfo::BSXShader::Samplers::GetString(Index);
	case BSShaderManager::BSSM_SHADER_UTILITY:
		return BSShaderInfo::BSUtilityShader::Samplers::GetString(Index);
	case BSShaderManager::BSSM_SHADER_DISTANTTREE:
		break;
	case BSShaderManager::BSSM_SHADER_PARTICLE:
		return BSShaderInfo::BSParticleShader::Samplers::GetString(Index);

	default:
		Assert(false);
		return nullptr;
	}

	if (ShaderMetadata[Type]->BySamplerIndex.count(Index))
		return ShaderMetadata[Type]->BySamplerIndex.at(Index)->Name;

	return nullptr;
}
