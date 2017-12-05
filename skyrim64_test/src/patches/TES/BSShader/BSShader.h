#pragma once

#include "../NiObject.h"
#include "BSShaderMaterial.h"
#include "BSShaderProperty.h"
#include "BSShaderManager.h"

class NiBoneMatrixSetterI
{
public:
	virtual ~NiBoneMatrixSetterI();

	virtual void SetBoneMatrix() = 0;
};

class BSReloadShaderI
{
public:
	virtual void ReloadShaders(/*BSIStream **/) = 0;
};

class BSShader : public NiRefObject, public NiBoneMatrixSetterI, public BSReloadShaderI
{
public:
	BSShader(const char *LoaderType);
	virtual ~BSShader();

	virtual bool SetupTechnique(uint32_t Technique) = 0;
	virtual void RestoreTechnique(uint32_t Technique) = 0;
	virtual void SetupMaterial(BSShaderMaterial const *Material);
	virtual void RestoreMaterial(BSShaderMaterial const *Material);
	virtual void SetupGeometry(BSRenderPass *Pass) = 0;
	virtual void RestoreGeometry(BSRenderPass *Pass) = 0;
	virtual void GetTechniqueName(uint32_t Technique, char *Buffer, uint32_t BufferSize);
	virtual void ReloadShaders(bool);

	virtual void ReloadShaders(/*BSIStream **/) override;
	virtual void SetBoneMatrix() override;

	bool BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader);
	void EndTechnique();

	uint32_t m_Type;
	char _pad[0x6C];
	uint32_t m_ActiveTechnique;
	const char *m_LoaderType;
};
static_assert(sizeof(BSShader) == 0xA0, "");
static_assert(offsetof(BSShader, m_Type) == 0x20, "");
static_assert(offsetof(BSShader, m_ActiveTechnique) == 0x90, "");
static_assert(offsetof(BSShader, m_LoaderType) == 0x98, "");

STATIC_CONSTRUCTOR(__CheckBSShaderVtable, []
{
	if (VTABLE_FUNCTION_INDEX(BSShader::SetupTechnique) != 2) MessageBoxA(nullptr, "Shit's broken", "", 0);
	if (VTABLE_FUNCTION_INDEX(BSShader::RestoreTechnique) != 3) MessageBoxA(nullptr, "Shit's broken", "", 0);
	if (VTABLE_FUNCTION_INDEX(BSShader::SetupMaterial) != 4) MessageBoxA(nullptr, "Shit's broken", "", 0);
	if (VTABLE_FUNCTION_INDEX(BSShader::RestoreMaterial) != 5) MessageBoxA(nullptr, "Shit's broken", "", 0);
	if (VTABLE_FUNCTION_INDEX(BSShader::SetupGeometry) != 6) MessageBoxA(nullptr, "Shit's broken", "", 0);
	if (VTABLE_FUNCTION_INDEX(BSShader::RestoreGeometry) != 7) MessageBoxA(nullptr, "Shit's broken", "", 0);
	if (VTABLE_FUNCTION_INDEX(BSShader::GetTechniqueName) != 8) MessageBoxA(nullptr, "Shit's broken", "", 0);
	if (vtable_index_util::getIndexOf(static_cast<void(BSShader::*)(bool)>(&BSShader::ReloadShaders)) != 9) MessageBoxA(nullptr, "Shit's broken", "", 0);
});