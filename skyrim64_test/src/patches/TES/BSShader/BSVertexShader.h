#pragma once

#include "../../../common.h"
#include "BSShaderManager.h"
#include "../BSGraphicsRenderer.h"

/*
class VertexShaderDecoder : public ShaderDecoder
{
private:
	BSGraphics::VertexShader *m_Shader;

public:
	VertexShaderDecoder(const char *Type, BSGraphics::VertexShader *Shader);

private:
	virtual uint32_t GetTechnique() override;
	virtual const uint8_t *GetConstantArray() override;
	virtual size_t GetConstantArraySize() override;
	virtual void DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined) override;

	void GetInputLayoutString(char *Buffer, size_t BufferSize);
};
*/