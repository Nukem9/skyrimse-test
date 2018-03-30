#include "BSPixelShader.h"

PixelShaderDecoder::PixelShaderDecoder(const char *Type, BSPixelShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::PIXEL)
{
	m_Shader = Shader;
}

uint32_t PixelShaderDecoder::GetTechnique()
{
	return m_Shader->m_TechniqueID;
}

const uint8_t *PixelShaderDecoder::GetConstantArray()
{
	return m_Shader->m_ConstantOffsets;
}

size_t PixelShaderDecoder::GetConstantArraySize()
{
	return ARRAYSIZE(BSPixelShader::m_ConstantOffsets);
}

void PixelShaderDecoder::DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined)
{
	char buf1[1024];
	sprintf_s(buf1, "C:\\myfolder\\%s\\%s_%s_%X.ps.txt", m_Type, m_Type, TechName, m_Shader->m_TechniqueID);

	// Something went really wrong if the shader exists already
	AssertMsg(GetFileAttributesA(buf1) == INVALID_FILE_ATTRIBUTES, "Trying to overwrite a shader that already exists!");

	FILE *file = fopen(buf1, "w");

	fprintf(file, "// %s\n", m_Type);
	fprintf(file, "// TechniqueID: 0x%X\n//\n", m_Shader->m_TechniqueID);
	fprintf(file, "// Technique: %s\n\n", TechName);

	// Defines
	if (auto& defs = GetDefineArray(m_Shader->m_TechniqueID); defs.size() > 0)
	{
		for (auto& define : defs)
			fprintf(file, "#define %s %s\n", define.first, define.second);

		fprintf(file, "\n");
	}

	// Samplers
	for (int i = 0;; i++)
	{
		const char *name = GetSamplerName(i, m_Shader->m_TechniqueID);

		if (strstr(name, "Add-your-"))
			break;

		fprintf(file, "// Sampler[%d]: %s\n", i, name);
	}

	fprintf(file, "\n");

	DumpCBuffer(file, &m_Shader->m_PerGeometry, PerGeo, 0); // Constant buffer 0 : register(b0)
	DumpCBuffer(file, &m_Shader->m_PerMaterial, PerMat, 1); // Constant buffer 1 : register(b1)
	DumpCBuffer(file, &m_Shader->m_PerTechnique, PerTec, 2);// Constant buffer 2 : register(b2)

	// Dump undefined variables
	for (auto& entry : Undefined)
		fprintf(file, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);

	fclose(file);

	// Now write raw HLSL
	char buf[1024];
	sprintf_s(buf, "C:\\myfolder\\%s\\%s_%s_%X.ps.hlsl", m_Type, m_Type, TechName, m_Shader->m_TechniqueID);
	FILE *f = fopen(buf, "wb");

	if (m_HlslData && f)
	{
		fwrite(m_HlslData, 1, m_HlslDataLen, f);
		fclose(f);
	}
}