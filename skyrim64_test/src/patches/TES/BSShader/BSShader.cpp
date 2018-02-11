#include "../../../common.h"
#include "../BSGraphicsRenderer.h"
#include "BSVertexShader.h"
#include "BSPixelShader.h"
#include "BSShader.h"
#include "../BSReadWriteLock.h"

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

AutoPtr(unsigned int, gTlsIndex, 0x34BBA78);

void BSShader::SetBoneMatrix(NiSkinInstance *SkinInstance, Data *Parameters, const NiTransform *Transform)
{
	//MemoryContextTracker tracker(26, "BSShader.cpp");

	if (!Parameters || Parameters->m_Flags == 0)
		return;

	auto *renderer = BSGraphics::Renderer::GetGlobals();

	uintptr_t v4 = (uintptr_t)SkinInstance;
	uintptr_t v5 = *(uintptr_t *)(__readgsqword(0x58u) + 8i64 * gTlsIndex);

	if (*(NiSkinInstance **)(v5 + 0x2A00) == SkinInstance)
		return;

	*(NiSkinInstance **)(v5 + 0x2A00) = SkinInstance;

	auto sub_140D74600 = (void(__fastcall *)(NiSkinInstance *, const NiTransform *))(g_ModuleBase + 0x0D74600);
	sub_140D74600(SkinInstance, Transform);

	const void *v9 = *(const void **)(v4 + 72);
	const void *v10 = *(const void **)(v4 + 80);
	uint32_t v11 = (unsigned int)(3 * *(uint32_t *)(*(uintptr_t *)(v4 + 16) + 88i64));

	auto boneDataConstants = renderer->GetShaderConstantGroup(16 * v11, BSGraphics::CONSTANT_GROUP_LEVEL_BONES);
	auto prevBoneDataConstants = renderer->GetShaderConstantGroup(16 * v11, BSGraphics::CONSTANT_GROUP_LEVEL_PREVIOUS_BONES);

	memcpy_s(boneDataConstants.RawData(), 16 * v11, v9, 16 * v11);
	memcpy_s(prevBoneDataConstants.RawData(), 16 * v11, v10, 16 * v11);

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

	pixelShader->m_TechniqueID = e->m_TechniqueID;
	e.temphack(pixelShader);
}

bool BSShader::BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader)
{
	bool hasVertexShader = false;
	bool hasPixelShader = false;
	BSVertexShader *vertexShader = nullptr;
	BSPixelShader *pixelShader = nullptr;

	if (m_VertexShaderTable.get(VertexShaderID, vertexShader))
		hasVertexShader = true;

	if (IgnorePixelShader || m_PixelShaderTable.get(PixelShaderID, pixelShader))
		hasPixelShader = true;

	if (!hasVertexShader || !hasPixelShader)
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
	uintptr_t a3 = (uintptr_t)ShaderProperty;

	// NiAlphaProperty::GetTestRef() * BSShaderProperty::GetAlpha()
	int rounded = (int)(((float)*(unsigned __int8 *)(a2 + 50)) * *(float *)(a3 + 48));

	BSGraphics::Renderer::GetGlobals()->SetScrapConstantValue((float)rounded * 0.0039215689f);
}