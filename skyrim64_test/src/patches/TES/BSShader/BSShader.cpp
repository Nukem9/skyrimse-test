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
	__debugbreak();
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
	//auto sub_1413362C0 = (void(__fastcall *)(BSShader*, NiSkinInstance *SkinInstance, Data *Parameters, const NiTransform *Transform))(g_ModuleBase + 0x13362C0);
	//sub_1413362C0(this, SkinInstance, Parameters, Transform);

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

	{
		D3D11_MAPPED_SUBRESOURCE map;
		renderer->m_DeviceContext->Map(renderer->m_ConstantBuffers1[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		memcpy_s(map.pData, 16 * v11, v9, 16 * v11);

		renderer->m_DeviceContext->Unmap(renderer->m_ConstantBuffers1[0], 0);
		renderer->m_DeviceContext->VSSetConstantBuffers(10, 1, &renderer->m_ConstantBuffers1[0]);
	}

	{
		D3D11_MAPPED_SUBRESOURCE map;
		renderer->m_DeviceContext->Map(renderer->m_ConstantBuffers1[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		memcpy_s(map.pData, 16 * v11, v10, 16 * v11);

		renderer->m_DeviceContext->Unmap(renderer->m_ConstantBuffers1[1], 0);
		renderer->m_DeviceContext->VSSetConstantBuffers(9, 1, &renderer->m_ConstantBuffers1[1]);
	}
}

bool BSShader::BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader)
{
	bool hasVertexShader = false;
	BSVertexShader *vertexShader = nullptr;

	if (m_VertexShaderTable.get(VertexShaderID, vertexShader))
		hasVertexShader = true;
	
	bool hasPixelShader = false;
	BSPixelShader *pixelShader = nullptr;

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