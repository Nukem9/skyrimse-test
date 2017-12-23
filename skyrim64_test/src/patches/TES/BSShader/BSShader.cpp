#include "../../../common.h"
#include "BSVertexShader.h"
#include "BSPixelShader.h"
#include "BSShader.h"
#include "../BSGraphicsRenderer.h"

BSShader::BSShader(const char *LoaderType)
{
	__debugbreak();
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
	__debugbreak();
}

void BSShader::ReloadShaders(BSIStream *Stream)
{
	__debugbreak();
}

void BSShader::SetBoneMatrix()
{
	__debugbreak();
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

	BSGraphics::Renderer::SetVertexShader(vertexShader);
	BSGraphics::Renderer::SetPixelShader(pixelShader);
	return true;
}

void BSShader::EndTechnique()
{
}