#pragma once

#include <stdint.h>
#include "../NiMain/NiRefObject.h"
#include "../BSTScatterTable.h"

class BSShaderMaterial;
struct BSRenderPass;
struct BSVertexShader;
struct BSPixelShader;
struct BSIStream;

#define BSSHADER_FORWARD_DEBUG 0

#define BSSHADER_FORWARD_CALL_ALWAYS(OptionIndex, Func, ...) \
{ \
	static uint32_t vtableIndex = vtable_index_util::getIndexOf(Func); \
	auto realFunc = Func; \
	*(uintptr_t *)&realFunc = *(uintptr_t*)(g_ModuleBase + OriginalVTableBase + (8 * vtableIndex)); \
	return (this->*realFunc)(__VA_ARGS__); \
}

#define BSSHADER_FORWARD_CALL(OptionIndex, Func, ...) \
if (g_ShaderToggles[m_Type][BSGraphics::CONSTANT_GROUP_LEVEL_##OptionIndex]) { BSSHADER_FORWARD_CALL_ALWAYS(OptionIndex, Func, __VA_ARGS__) }

class NiBoneMatrixSetterI
{
public:
	virtual ~NiBoneMatrixSetterI()
	{
		__debugbreak();
	}

	virtual void SetBoneMatrix() = 0;
};

class BSReloadShaderI
{
public:
	virtual void ReloadShaders(BSIStream *Stream) = 0;
};

class BSShader : public NiRefObject, public NiBoneMatrixSetterI, public BSReloadShaderI
{
public:
	static bool g_ShaderToggles[16][3];

	BSShader(const char *LoaderType);
	virtual ~BSShader();

	virtual bool SetupTechnique(uint32_t Technique) = 0;
	virtual void RestoreTechnique(uint32_t Technique) = 0;
	virtual void SetupMaterial(BSShaderMaterial const *Material);
	virtual void RestoreMaterial(BSShaderMaterial const *Material);
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t Flags) = 0;
	virtual void RestoreGeometry(BSRenderPass *Pass) = 0;
	virtual void GetTechniqueName(uint32_t Technique, char *Buffer, uint32_t BufferSize);
	virtual void ReloadShaders(bool Unknown);

	virtual void ReloadShaders(BSIStream *Stream) override;
	virtual void SetBoneMatrix() override;

	bool BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader);
	void EndTechnique();

	uint32_t m_Type;
	BSTScatterTable<uint32_t, BSVertexShader *> m_VertexShaderTable;
	BSTScatterTable<uint32_t, BSPixelShader *> m_PixelShaderTable;
	const char *m_LoaderType;
};
static_assert(sizeof(BSShader) == 0x90, "");
static_assert(offsetof(BSShader, m_Type) == 0x20, "");
static_assert(offsetof(BSShader, m_VertexShaderTable) == 0x28, "");
static_assert(offsetof(BSShader, m_PixelShaderTable) == 0x58, "");
static_assert(offsetof(BSShader, m_LoaderType) == 0x88, "");

STATIC_CONSTRUCTOR(__CheckBSShaderVtable, []
{
	assert_vtable_index(&BSShader::~BSShader, 0);
	assert_vtable_index(&BSShader::DeleteThis, 1);
	assert_vtable_index(&BSShader::SetupTechnique, 2);
	assert_vtable_index(&BSShader::RestoreTechnique, 3);
	assert_vtable_index(&BSShader::SetupMaterial, 4);
	assert_vtable_index(&BSShader::RestoreMaterial, 5);
	assert_vtable_index(&BSShader::SetupGeometry, 6);
	assert_vtable_index(&BSShader::RestoreGeometry, 7);
	assert_vtable_index(&BSShader::GetTechniqueName, 8);
	assert_vtable_index(static_cast<void(BSShader::*)(bool)>(&BSShader::ReloadShaders), 9);
});