#include "../../../common.h"
#include "../MemoryContextTracker.h"
#include "../BSGraphicsRenderer.h"
#include "BSVertexShader.h"
#include "BSPixelShader.h"
#include "BSShader.h"

bool BSShader::g_ShaderToggles[16][3];

BSShader::BSShader(const char *LoaderType)
{
	m_LoaderType = LoaderType;
}

BSShader::~BSShader()
{
	Assert(false);
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
	auto sub_141336480 = (void(__fastcall *)(bool))(g_ModuleBase + 0x1336490);
	sub_141336480(Unknown);
}

void BSShader::ReloadShaders(BSIStream *Stream)
{
	auto sub_141336490 = (void(__fastcall *)(BSIStream *))(g_ModuleBase + 0x1336490);
	sub_141336490(Stream);
}

void BSShader::SetBoneMatrix(NiSkinInstance *SkinInstance, Data *Parameters, const NiTransform *Transform)
{
	MemoryContextTracker tracker(26, "BSShader.cpp");

	if (!Parameters || Parameters->m_Flags == 0)
		return;

	auto *renderer = BSGraphics::Renderer::GetGlobals();

	if (GAME_TLS(NiSkinInstance *, 0x2A00) == SkinInstance)
		return;

	GAME_TLS(NiSkinInstance *, 0x2A00) = SkinInstance;

	// WARNING: Contains a global variable edit
	auto sub_140D74600 = (void(__fastcall *)(NiSkinInstance *, const NiTransform *))(g_ModuleBase + 0x0D74600);
	sub_140D74600(SkinInstance, Transform);

	uint32_t v11 = (unsigned int)(3 * *(uint32_t *)(*(uintptr_t *)((uintptr_t)SkinInstance + 16) + 88i64)) * 16;

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
	swprintf_s(fxpPath, L"C:\\myshaders\\%S.fxp", SourceFile);

	BSVertexShader *vertexShader = BSGraphics::Renderer::GetGlobals()->CompileVertexShader(fxpPath, Defines, GetConstant);

	auto e = m_VertexShaderTable.find(Technique);

	Assert(e != m_VertexShaderTable.end());

	if (!strstr(SourceFile, "DistantTree"))
	{
		BSGraphics::Renderer::GetGlobals()->ValidateShaderReplacement(e->m_Shader, vertexShader->m_Shader);

		for (int i = 0; i < 20; i++)
		{
			if (vertexShader->m_ConstantOffsets[i] == BSGraphics::INVALID_CONSTANT_BUFFER_OFFSET)
				continue;

			if (vertexShader->m_ConstantOffsets[i] != e->m_ConstantOffsets[i])
				Assert(false);
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
	swprintf_s(fxpPath, L"C:\\myshaders\\%S.fxp", SourceFile);

	BSPixelShader *pixelShader = BSGraphics::Renderer::GetGlobals()->CompilePixelShader(fxpPath, Defines, GetSampler, GetConstant);

	auto e = m_PixelShaderTable.find(Technique);

	Assert(e != m_PixelShaderTable.end());

	if (!strstr(SourceFile, "DistantTree"))
	{
		BSGraphics::Renderer::GetGlobals()->ValidateShaderReplacement(e->m_Shader, pixelShader->m_Shader);

		for (int i = 0; i < 64; i++)
		{
			if (pixelShader->m_ConstantOffsets[i] == BSGraphics::INVALID_CONSTANT_BUFFER_OFFSET)
				continue;

			if (pixelShader->m_ConstantOffsets[i] != e->m_ConstantOffsets[i])
				Assert(false);
		}
	}

	pixelShader->m_TechniqueID = e->m_TechniqueID;
	e.temphack(pixelShader);
}

bool BSShader::BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader)
{
	BSVertexShader *vertexShader = nullptr;
	BSPixelShader *pixelShader = nullptr;

	if (!m_VertexShaderTable.get(VertexShaderID, vertexShader))
		return false;

	if (!IgnorePixelShader && !m_PixelShaderTable.get(PixelShaderID, pixelShader))
		return false;

	// Vertex shader required, pixel shader optional (nullptr)
	BSGraphics::Renderer::GetGlobals()->SetVertexShader(vertexShader);
	BSGraphics::Renderer::GetGlobals()->SetPixelShader(pixelShader);
	return true;
}

void BSShader::EndTechnique()
{
}

void BSShader::SetupGeometryAlphaBlending(const NiAlphaProperty *AlphaProperty, BSShaderProperty *ShaderProperty, bool a4)
{
	auto sub_1413360D0 = (void(__fastcall *)(BSShader *, const NiAlphaProperty *, BSShaderProperty *, bool))(g_ModuleBase + 0x13360D0);
	sub_1413360D0(this, AlphaProperty, ShaderProperty, a4);
}

void BSShader::SetupAlphaTestRef(const NiAlphaProperty *AlphaProperty, BSShaderProperty *ShaderProperty)
{
	uintptr_t a2 = (uintptr_t)AlphaProperty;

	// NiAlphaProperty::GetTestRef() * BSShaderProperty::GetAlpha()
	float alphaRef = trunc((float)*(unsigned __int8 *)(a2 + 50) * ShaderProperty->GetAlpha());

	BSGraphics::Renderer::GetGlobals()->SetAlphaTestRef(alphaRef * (1.0f / 255.0f));
}