#include "../../../common.h"
#include "BSVertexShader.h"
#include "BSPixelShader.h"
#include "BSShader.h"
#include "../BSGraphicsRenderer.h"

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

void BSShader::SetBoneMatrix()
{
	auto sub_1413362C0 = (void(__fastcall *)())(g_ModuleBase + 0x13362C0);
	sub_1413362C0();
}

bool BSShader::BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader)
{
	auto sub_141336860 = (bool(__fastcall *)(BSShader *, uint32_t, uint32_t, bool))(g_ModuleBase + 0x1336860);

	return sub_141336860(this, VertexShaderID, PixelShaderID, IgnorePixelShader);

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

	BSGraphics::Renderer::SetVertexShader(vertexShader);
	BSGraphics::Renderer::SetPixelShader(pixelShader);
	return true;
}

void BSShader::EndTechnique()
{
}