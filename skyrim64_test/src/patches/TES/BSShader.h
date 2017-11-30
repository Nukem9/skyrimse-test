#pragma once

#include "NiObject.h"
#include "BSShaderManager.h"

struct BSRenderPass;

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

	virtual void ReloadShaders(/*BSIStream **/) override;
	virtual void SetBoneMatrix() override;

	virtual bool SetupTechnique(uint32_t Technique) = 0;
	virtual void RestoreTechnique(uint32_t Technique) = 0;
	virtual void SetupMaterial();
	virtual void RestoreMaterial();
	virtual void SetupGeometry(BSRenderPass *Pass) = 0;
	virtual void RestoreGeometry(BSRenderPass *Pass) = 0;
	virtual void GetTechniqueName(uint32_t Technique, char *Buffer, uint32_t BufferSize);
	virtual void ReloadShaders(bool);

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