#pragma once

#include "BSShaderManager.h"
#include "BSShaderAccumulator.h"

class ShaderDecoder
{
protected:
	struct ParamIndexPair
	{
		int Index;
		const char *Name;
	};

	void *m_HlslData;
	size_t m_HlslDataLen;

	BSShaderManager::ShaderEnum m_Type;
	char m_LoaderType[256];
	BSSM_SHADER_TYPE m_CodeType;

public:
	ShaderDecoder(const char *Type, BSSM_SHADER_TYPE CodeType);
	~ShaderDecoder();

	void SetShaderData(void *Buffer, size_t BufferSize);
	void DumpShader();

protected:
	void DumpCBuffer(FILE *File, BSGraphics::Buffer *Buffer, std::vector<ParamIndexPair> Params, int GroupIndex);

	virtual uint32_t GetTechnique() = 0;
	virtual const uint8_t *GetConstantArray() = 0;
	virtual size_t GetConstantArraySize() = 0;
	virtual void DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined) = 0;

	const char *GetGroupName(int Index);
	const char *GetGroupRegister(int Index);
	const char *GetConstantName(int Index);
	ShaderDescriptor::DeclType GetVariableCategory(int Index);
	const char *GetVariableType(int Index);
	void GetTechniqueName(char *Buffer, size_t BufferSize, uint32_t Technique);
};

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
};

class PixelShaderDecoder : public ShaderDecoder
{
private:
	BSGraphics::PixelShader *m_Shader;

public:
	PixelShaderDecoder(const char *Type, BSGraphics::PixelShader *Shader);

private:
	virtual uint32_t GetTechnique() override;
	virtual const uint8_t *GetConstantArray() override;
	virtual size_t GetConstantArraySize() override;
	virtual void DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined) override;
};