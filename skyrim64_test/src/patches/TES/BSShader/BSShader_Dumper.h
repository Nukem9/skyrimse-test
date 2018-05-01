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
		const struct BSShaderMappings::Entry *Remap;
	};

	void *m_HlslData;
	size_t m_HlslDataLen;

	char m_Type[256];
	BSSM_SHADER_TYPE m_CodeType;

public:
	ShaderDecoder(const char *Type, BSSM_SHADER_TYPE CodeType);
	~ShaderDecoder();

	void SetShaderData(void *Buffer, size_t BufferSize);
	void DumpShader();

protected:
	void DumpShaderInfo();
	void DumpCBuffer(FILE *File, BSConstantGroup *Buffer, std::vector<ParamIndexPair> Params, int GroupIndex);

	virtual uint32_t GetTechnique() = 0;
	virtual const uint8_t *GetConstantArray() = 0;
	virtual size_t GetConstantArraySize() = 0;
	virtual void DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined) = 0;

	const char *GetGroupName(int Index);
	const char *GetGroupRegister(int Index);
	const char *GetConstantName(int Index);
	void GetTechniqueName(char *Buffer, size_t BufferSize, uint32_t Technique);
	const char *GetSamplerName(int Index, uint32_t Technique);
	std::vector<std::pair<const char *, const char *>> GetDefineArray(uint32_t Technique);
};