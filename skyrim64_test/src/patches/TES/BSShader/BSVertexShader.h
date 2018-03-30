#pragma once

#include "../../../common.h"
#include "BSShaderManager.h"

#define MAX_VS_CONSTANTS 20

struct BSVertexShader
{
	uint32_t m_TechniqueID;			// Bit flags
	ID3D11VertexShader *m_Shader;	// DirectX handle
	uint32_t m_ShaderLength;		// Raw bytecode length

	// This must match the BSGraphics::ConstantGroupLevel enum
	union
	{
		struct
		{
			BSConstantGroup m_PerTechnique;
			BSConstantGroup m_PerMaterial;
			BSConstantGroup m_PerGeometry;
		};

		BSConstantGroup m_ConstantGroups[3];
	};

	uint64_t m_VertexDescription;				// ID3D11Device::CreateInputLayout (for VSMain())
	uint8_t m_ConstantOffsets[MAX_VS_CONSTANTS];// Actual offset is multiplied by 4
	uint8_t __padding[4];						//
	uint8_t m_RawBytecode[0];					// Raw bytecode
};
static_assert(offsetof(BSVertexShader, m_TechniqueID) == 0x0, "");
static_assert(offsetof(BSVertexShader, m_Shader) == 0x8, "");
static_assert(offsetof(BSVertexShader, m_ShaderLength) == 0x10, "");
static_assert(offsetof(BSVertexShader, m_PerTechnique) == 0x18, "");
static_assert(offsetof(BSVertexShader, m_PerMaterial) == 0x28, "");
static_assert(offsetof(BSVertexShader, m_PerGeometry) == 0x38, "");
static_assert(offsetof(BSVertexShader, m_VertexDescription) == 0x48, "");
static_assert(offsetof(BSVertexShader, m_ConstantOffsets) == 0x50, "");
static_assert_offset(BSVertexShader, m_RawBytecode, 0x68);
static_assert(sizeof(BSVertexShader) == 0x68, "");

class VertexShaderDecoder : public ShaderDecoder
{
private:
	BSVertexShader *m_Shader;

public:
	VertexShaderDecoder(const char *Type, BSVertexShader *Shader);

private:
	virtual uint32_t GetTechnique() override;
	virtual const uint8_t *GetConstantArray() override;
	virtual size_t GetConstantArraySize() override;
	virtual void DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined) override;

	void GetInputLayoutString(char *Buffer, size_t BufferSize);
};