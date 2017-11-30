#pragma once

#include "../../../common.h"
#include "BSShaderManager.h"

struct BSPixelShader
{
	uint32_t m_TechniqueID;				// Bit flags
	ID3D11PixelShader *m_Shader;
	BSConstantGroup m_PerGeometry;
	BSConstantGroup m_PerMaterial;
	BSConstantGroup m_PerTechnique;
	uint8_t m_ConstantOffsets[64];		// Actual offset is multiplied by 4
										// Bytecode is not appended
};
static_assert(offsetof(BSPixelShader, m_TechniqueID) == 0x0, "");
static_assert(offsetof(BSPixelShader, m_Shader) == 0x8, "");
static_assert(offsetof(BSPixelShader, m_PerGeometry) == 0x10, "");
static_assert(offsetof(BSPixelShader, m_PerMaterial) == 0x20, "");
static_assert(offsetof(BSPixelShader, m_PerTechnique) == 0x30, "");
static_assert(offsetof(BSPixelShader, m_ConstantOffsets) == 0x40, "");
static_assert(sizeof(BSPixelShader) == 0x80, "");

class PixelShaderDecoder : public ShaderDecoder
{
private:
	BSPixelShader *m_Shader;

public:
	PixelShaderDecoder(const char *Type, BSPixelShader *Shader);

private:
	virtual uint32_t GetTechnique() override;
	virtual const uint8_t *GetConstantArray() override;
	virtual size_t GetConstantArraySize() override;
	virtual void DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined) override;
};