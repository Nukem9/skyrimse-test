#include "BSVertexShader.h"

/*
VertexShaderDecoder::VertexShaderDecoder(const char *Type, BSVertexShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::VERTEX)
{
	m_Shader = Shader;
}

uint32_t VertexShaderDecoder::GetTechnique()
{
	return m_Shader->m_TechniqueID;
}

const uint8_t *VertexShaderDecoder::GetConstantArray()
{
	return m_Shader->m_ConstantOffsets;
}

size_t VertexShaderDecoder::GetConstantArraySize()
{
	return ARRAYSIZE(BSVertexShader::m_ConstantOffsets);
}

void VertexShaderDecoder::DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined)
{
	char buf1[1024];
	sprintf_s(buf1, "C:\\myfolder\\%s\\%s_%s_%X.vs.txt", m_Type, m_Type, TechName, m_Shader->m_TechniqueID);

	// Something went really wrong if the shader exists already
	AssertMsg(GetFileAttributesA(buf1) == INVALID_FILE_ATTRIBUTES, "Trying to overwrite a shader that already exists!");

	FILE *file = fopen(buf1, "w");

	//char inputLayout[1024];
	//GetInputLayoutString(m_Shader->m_VertexDescription, inputLayout, ARRAYSIZE(inputLayout));

	fprintf(file, "// %s\n", m_Type);
	fprintf(file, "// TechniqueID: 0x%X\n", m_Shader->m_TechniqueID);
	fprintf(file, "// Vertex description: 0x%llX\n//\n", m_Shader->m_VertexDescription);
	fprintf(file, "// Technique: %s\n\n", TechName);
	//fprintf(file, "// Input layout: %s\n\n", inputLayout);

	// Defines
	if (auto& defs = GetDefineArray(m_Shader->m_TechniqueID); defs.size() > 0)
	{
		for (const auto& define : defs)
			fprintf(file, "#define %s %s\n", define.first, define.second);

		fprintf(file, "\n");
	}

	DumpCBuffer(file, &m_Shader->m_PerGeometry, PerGeo, 0); // Constant buffer 0 : register(b0)
	DumpCBuffer(file, &m_Shader->m_PerMaterial, PerMat, 1); // Constant buffer 1 : register(b1)
	DumpCBuffer(file, &m_Shader->m_PerTechnique, PerTec, 2);// Constant buffer 2 : register(b2)

	// Dump undefined variables
	for (auto& entry : Undefined)
		fprintf(file, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);

	fclose(file);

	// Now write raw HLSL
	char buf[1024];
	sprintf_s(buf, "C:\\myfolder\\%s\\%s_%s_%X.vs.hlsl", m_Type, m_Type, TechName, m_Shader->m_TechniqueID);
	FILE *f = fopen(buf, "wb");

	if (m_HlslData && f)
	{
		fwrite(m_HlslData, 1, m_HlslDataLen, f);
		fclose(f);
	}
}

void VertexShaderDecoder::GetInputLayoutString(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "");

	auto isSet = [this](uint64_t Bits)
	{
		return (m_Shader->m_VertexDescription & Bits) == Bits;
	};

	// Skyrim always uses some default layout params, these ones are optional
	if (isSet(0x401000000000FFull))
		strcat_s(Buffer, BufferSize, "0:POSITION, ");
	if (isSet(0x80200000000F00ull))
		strcat_s(Buffer, BufferSize, "0:TEXCOORD, ");
	if (isSet(0x10040000000F000ull))
		strcat_s(Buffer, BufferSize, "1:TEXCOORD2, ");
	if (isSet(0x2008000000F0000ull))
		strcat_s(Buffer, BufferSize, "0:NORMAL, ");
	if (isSet(0x401000000F00000ull))
		strcat_s(Buffer, BufferSize, "0:BINORMAL, ");
	if (isSet(0x80200000F000000ull))
		strcat_s(Buffer, BufferSize, "0:COLOR, ");
	if (isSet(0x10040000F0000000ull))
		strcat_s(Buffer, BufferSize, "0:BLENDWEIGHT, ");
	if (isSet(0x2008000F00000000ull))
		strcat_s(Buffer, BufferSize, "3:TEXCOORD, ");
	if (isSet(0x401000F000000000ull))
		strcat_s(Buffer, BufferSize, "2:TEXCOORD, ");
	if (isSet(0x80200F0000000000ull))
		strcat_s(Buffer, BufferSize, "4:TEXCOORD, ");

	Trim(Buffer, ' ');
	Trim(Buffer, ',');
}
*/