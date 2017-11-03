#include "../../stdafx.h"
#include <direct.h>
#include <D3D11Shader.h>
#include <D3Dcompiler.h>

struct RemapEntry
{
	const char *Type;
	int Group;
	int Index;
	const char *ParamTypeOverride;
};

const RemapEntry VertexEntries[] =
{
#define REMAP_VERTEX_UNUSED(ShaderType, GroupType)
#define REMAP_VERTEX(ShaderType, GroupType, ParameterIndex, ParamType) { ShaderType, GroupType, ParameterIndex, ParamType },
#include "BSShaderConstants.inl"
};

const RemapEntry PixelEntries[] =
{
#define REMAP_PIXEL_UNUSED(ShaderType, GroupType)
#define REMAP_PIXEL(ShaderType, GroupType, ParameterIndex, ParamType) { ShaderType, GroupType, ParameterIndex, ParamType },
#include "BSShaderConstants.inl"
};

void GetInputLayoutString(uint64_t InputLayoutFlags, char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "");

	auto isSet = [InputLayoutFlags](uint64_t Bits)
	{
		return (InputLayoutFlags & Bits) == Bits;
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

const RemapEntry *GetEntryForVertexShader(const char *Type, int ParamIndex)
{
	for (int i = 0; i < ARRAYSIZE(VertexEntries); i++)
	{
		if (_stricmp(VertexEntries[i].Type, Type) != 0)
			continue;

		if (VertexEntries[i].Index != ParamIndex)
			continue;

		return &VertexEntries[i];
	}

	return nullptr;
}

const RemapEntry *GetEntryForPixelShader(const char *Type, int ParamIndex)
{
	for (int i = 0; i < ARRAYSIZE(PixelEntries); i++)
	{
		if (_stricmp(PixelEntries[i].Type, Type) != 0)
			continue;

		if (PixelEntries[i].Index != ParamIndex)
			continue;

		return &PixelEntries[i];
	}

	return nullptr;
}

const char *GetShaderConstantName(const char *ShaderType, BSSM_SHADER_TYPE CodeType, int ConstantIndex);
void GetShaderTechniqueName(const char *ShaderType, BSSM_SHADER_TYPE CodeType, char *Buffer, size_t BufferSize, uint32_t Technique);
const char *GetShaderSamplerName(const char *ShaderType, int ConstantIndex);

class ShaderDecoder
{
public:
	~ShaderDecoder()
	{
		if (m_ShaderData)
			delete[] m_ShaderData;
	}

	void SetShaderData(void *Buffer, size_t BufferSize)
	{
		m_ShaderData = new char[BufferSize];
		m_ShaderDataLen = BufferSize;

		memcpy(m_ShaderData, Buffer, BufferSize);
	}

protected:
	void *m_ShaderData;
	size_t m_ShaderDataLen;

	char m_Type[256];
	BSSM_SHADER_TYPE m_CodeType;

	struct ParamIndexPair
	{
		int Index;
		const char *Name;
		const RemapEntry *Remap;
	};

	ShaderDecoder(const char *Type, BSSM_SHADER_TYPE CodeType)
	{
		m_ShaderData = nullptr;
		m_ShaderDataLen = 0;
		strcpy_s(m_Type, Type);
		m_CodeType = CodeType;
	}

	const char *GetGroupName(int Index)
	{
		switch (Index)
		{
		case 0:return "PerGeometry";
		case 1:return "PerMaterial";
		case 2:return "PerTechnique";
		}

		return nullptr;
	}

	const char *GetGroupRegister(int Index)
	{
		switch (Index)
		{
		case 0:return "b0";
		case 1:return "b1";
		case 2:return "b2";
		}

		return nullptr;
	}

	const char *GetConstantName(int Index)
	{
		return GetShaderConstantName(m_Type, m_CodeType, Index);
	}

	int GetConstantArraySize()
	{
		switch (m_CodeType)
		{
		case BSSM_SHADER_TYPE::VERTEX: return ARRAYSIZE(BSVertexShader::m_ConstantOffsets);
		case BSSM_SHADER_TYPE::PIXEL:  return ARRAYSIZE(BSPixelShader::m_ConstantOffsets);
		case BSSM_SHADER_TYPE::COMPUTE:return 0;
		}

		return 0;
	}
};

class VertexShaderDecoder : public ShaderDecoder
{
public:

private:
	BSVertexShader *m_Shader;
	FILE *m_File;

public:
	VertexShaderDecoder(const char *Type, BSVertexShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::VERTEX)
	{
		m_Shader = Shader;

		// Guarantee that the folder exists
		char buf[1024];
		sprintf_s(buf, "C:\\myfolder\\%s\\", Type);
		_mkdir(buf);
	}

	~VertexShaderDecoder()
	{
	}

	void DumpShaderInfo()
	{
		// Build a list of all constants used here
		std::vector<ParamIndexPair> geoIndexes;
		std::vector<ParamIndexPair> matIndexes;
		std::vector<ParamIndexPair> tecIndexes;
		std::vector<ParamIndexPair> undefinedIndexes;

		for (int i = 0; i < GetConstantArraySize(); i++)
		{
			const char *name = GetConstantName(i);

			if (strstr(name, "Add-your-"))
				break;

			auto remapData = GetEntryForVertexShader(m_Type, i);

			if (!remapData)
			{
				// Indicates some kind of error here
				undefinedIndexes.push_back({ i, name, nullptr });
			}
			else
			{
				switch (remapData->Group)
				{
				case 0:
					geoIndexes.push_back({ i, name, remapData });
					break;
				case 1:
					matIndexes.push_back({ i, name, remapData });
					break;
				case 2:
					tecIndexes.push_back({ i, name, remapData });
					break;
				}
			}
		}

		char technique[1024];
		GetShaderTechniqueName(m_Type, m_CodeType, technique, ARRAYSIZE(technique), m_Shader->m_TechniqueID);

		for (int i = 0;; i++)
		{
			if (technique[i] == '\0')
				break;

			if (technique[i] == ' ')
				technique[i] = '_';
		}

		char buf1[1024];
		sprintf_s(buf1, "C:\\myfolder\\%s\\%s_%s_%X.vs.txt", m_Type, m_Type, technique, m_Shader->m_TechniqueID);

		// Something went really wrong if the shader exists already
		if (GetFileAttributesA(buf1) != INVALID_FILE_ATTRIBUTES)
			__debugbreak();

		m_File = fopen(buf1, "w");

		//char inputLayout[1024];
		//GetInputLayoutString(m_Shader->m_InputLayoutFlags, inputLayout, ARRAYSIZE(inputLayout));

		fprintf(m_File, "// %s\n", m_Type);
		fprintf(m_File, "// TechniqueID: 0x%X\n", m_Shader->m_TechniqueID);
		fprintf(m_File, "// Input flags: 0x%llX\n//\n", m_Shader->m_InputLayoutFlags);
		fprintf(m_File, "// Technique: %s\n\n", technique);
		//fprintf(m_File, "// Input layout: %s\n\n", inputLayout);

		DumpCBuffer(&m_Shader->m_PerGeometry, geoIndexes, 0);// Constant buffer 0 : register(b0)
		DumpCBuffer(&m_Shader->m_PerMaterial, matIndexes, 1);// Constant buffer 1 : register(b1)
		DumpCBuffer(&m_Shader->m_PerTechnique, tecIndexes, 2);// Constant buffer 2 : register(b2)

		// Dump undefined variables
		for (auto& entry : undefinedIndexes)
			fprintf(m_File, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);

		fclose(m_File);

		// Now write raw HLSL
		char buf[1024];
		sprintf_s(buf, "C:\\myfolder\\%s\\%s_%s_%X.vs.hlsl", m_Type, m_Type, technique, m_Shader->m_TechniqueID);
		FILE *f = fopen(buf, "wb");

		if (m_ShaderData && f)
		{
			fwrite(m_ShaderData, 1, m_ShaderDataLen, f);
			fclose(f);
		}
	}

private:
	void DumpCBuffer(BSConstantBufferInfo *Buffer, std::vector<ParamIndexPair> Params, int GroupIndex)
	{
		D3D11_BUFFER_DESC desc;

		if (Buffer->m_Buffer)
		{
			Buffer->m_Buffer->GetDesc(&desc);
			fprintf(m_File, "// Dynamic buffer: Size = %d (0x%X)\n", desc.ByteWidth, desc.ByteWidth);
		}

		if (Buffer->m_Data)
			fprintf(m_File, "// Unmapped\n");

		fprintf(m_File, "cbuffer %s : register(%s)\n{\n", GetGroupName(GroupIndex), GetGroupRegister(GroupIndex));

		// Don't print any variables if this buffer is undefined
		if (Buffer->m_Buffer)
		{
			// Sort each variable by offset
			std::sort(Params.begin(), Params.end(),
				[this](ParamIndexPair& a1, ParamIndexPair& a2)
			{
				return m_Shader->m_ConstantOffsets[a1.Index] < m_Shader->m_ConstantOffsets[a2.Index];
			});

			for (auto& entry : Params)
			{
				// Generate the variable and type name
				char varName[256];

				if (entry.Remap->ParamTypeOverride)
				{
					int index;
					if (sscanf_s(entry.Remap->ParamTypeOverride, "float4[%d]", &index) == 1)
						sprintf_s(varName, "float4 %s[%d]", entry.Name, index);
					else
						sprintf_s(varName, "%s %s", entry.Remap->ParamTypeOverride, entry.Name);
				}
				else
				{
					// Undefined variable type; default to float4
					sprintf_s(varName, "float4 %s", entry.Name);
				}

				// Convert cbOffset to packoffset(c0) or packoffset(c0.x) or packoffset(c0.y) or .... to enforce compiler
				// buffer ordering
				uint8_t cbOffset = m_Shader->m_ConstantOffsets[entry.Index];
				char packOffset[64];

				switch (cbOffset % 4)
				{
				case 0:sprintf_s(packOffset, "packoffset(c%d)", cbOffset / 4); break;  // Normal register on 16 byte boundary
				case 1:sprintf_s(packOffset, "packoffset(c%d.y)", cbOffset / 4); break;// Requires swizzle index
				case 2:sprintf_s(packOffset, "packoffset(c%d.z)", cbOffset / 4); break;// Requires swizzle index
				case 3:sprintf_s(packOffset, "packoffset(c%d.w)", cbOffset / 4); break;// Requires swizzle index
				default:__debugbreak(); break;
				}

				fprintf(m_File, "\t%s", varName);

				// Add space alignment
				for (int i = 45 - max(0, strlen(varName)); i > 0; i--)
					fprintf(m_File, " ");

				fprintf(m_File, ": %s;", packOffset);

				// Add space alignment
				for (int i = 20 - max(0, strlen(packOffset)); i > 0; i--)
					fprintf(m_File, " ");

				fprintf(m_File, "// @ %d - 0x%04X\n", cbOffset, cbOffset * 4);
			}
		}

		fprintf(m_File, "}\n\n");
	}
};

class PixelShaderDecoder : public ShaderDecoder
{
public:

private:
	BSPixelShader *m_Shader;
	FILE *m_File;

public:
	PixelShaderDecoder(const char *Type, BSPixelShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::PIXEL)
	{
		m_Shader = Shader;

		// Guarantee that the folder exists
		char buf[1024];
		sprintf_s(buf, "C:\\myfolder\\%s\\", Type);
		_mkdir(buf);
	}

	~PixelShaderDecoder()
	{
	}

	void DumpShaderInfo()
	{
		// Build a list of all constants used here
		std::vector<ParamIndexPair> geoIndexes;
		std::vector<ParamIndexPair> matIndexes;
		std::vector<ParamIndexPair> tecIndexes;
		std::vector<ParamIndexPair> undefinedIndexes;

		for (int i = 0; i < GetConstantArraySize(); i++)
		{
			const char *name = GetConstantName(i);

			if (strstr(name, "Add-your-"))
				break;

			auto remapData = GetEntryForPixelShader(m_Type, i);

			if (!remapData)
			{
				// Indicates some kind of error here
				undefinedIndexes.push_back({ i, name, nullptr });
			}
			else
			{
				switch (remapData->Group)
				{
				case 0:
					geoIndexes.push_back({ i, name, remapData });
					break;
				case 1:
					matIndexes.push_back({ i, name, remapData });
					break;
				case 2:
					tecIndexes.push_back({ i, name, remapData });
					break;
				}
			}
		}

		char technique[1024];
		GetShaderTechniqueName(m_Type, m_CodeType, technique, ARRAYSIZE(technique), m_Shader->m_TechniqueID);

		for (int i = 0;; i++)
		{
			if (technique[i] == '\0')
				break;

			if (technique[i] == ' ')
				technique[i] = '_';
		}

		char buf1[1024];
		sprintf_s(buf1, "C:\\myfolder\\%s\\%s_%s_%X.ps.txt", m_Type, m_Type, technique, m_Shader->m_TechniqueID);

		// Something went really wrong if the shader exists already
		if (GetFileAttributesA(buf1) != INVALID_FILE_ATTRIBUTES)
			__debugbreak();

		m_File = fopen(buf1, "w");

		fprintf(m_File, "// %s\n", m_Type);
		fprintf(m_File, "// TechniqueID: 0x%X\n//\n", m_Shader->m_TechniqueID);
		fprintf(m_File, "// Technique: %s\n\n", technique);

		// Dump samplers
		for (int i = 0;; i++)
		{
			const char *name = GetShaderSamplerName(m_Type, i);

			if (strstr(name, "Add-your-"))
				break;

			fprintf(m_File, "// Sampler[%d]: %s\n", i, name);
		}

		fprintf(m_File, "\n");

		DumpCBuffer(&m_Shader->m_PerGeometry, geoIndexes, 0); // Constant buffer 0 : register(b0)
		DumpCBuffer(&m_Shader->m_PerMaterial, matIndexes, 1); // Constant buffer 1 : register(b1)
		DumpCBuffer(&m_Shader->m_PerTechnique, tecIndexes, 2);// Constant buffer 2 : register(b2)

		// Dump undefined variables
		for (auto& entry : undefinedIndexes)
			fprintf(m_File, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);

		fclose(m_File);

		// Now write raw HLSL
		char buf[1024];
		sprintf_s(buf, "C:\\myfolder\\%s\\%s_%s_%X.ps.hlsl", m_Type, m_Type, technique, m_Shader->m_TechniqueID);
		FILE *f = fopen(buf, "wb");

		if (m_ShaderData && f)
		{
			fwrite(m_ShaderData, 1, m_ShaderDataLen, f);
			fclose(f);
		}
	}

private:
	void DumpCBuffer(BSConstantBufferInfo *Buffer, std::vector<ParamIndexPair> Params, int GroupIndex)
	{
		D3D11_BUFFER_DESC desc;

		if (Buffer->m_Buffer)
		{
			Buffer->m_Buffer->GetDesc(&desc);
			fprintf(m_File, "// Dynamic buffer: Size = %d (0x%X)\n", desc.ByteWidth, desc.ByteWidth);
		}

		if (Buffer->m_Data)
			fprintf(m_File, "// Unmapped\n");

		fprintf(m_File, "cbuffer %s : register(%s)\n{\n", GetGroupName(GroupIndex), GetGroupRegister(GroupIndex));

		// Don't print any variables if this buffer is undefined
		if (Buffer->m_Buffer)
		{
			// Sort each variable by offset
			std::sort(Params.begin(), Params.end(),
				[this](ParamIndexPair& a1, ParamIndexPair& a2)
			{
				return m_Shader->m_ConstantOffsets[a1.Index] < m_Shader->m_ConstantOffsets[a2.Index];
			});

			for (auto& entry : Params)
			{
				// Generate the variable and type name
				char varName[256];

				if (entry.Remap->ParamTypeOverride)
				{
					int index;
					if (sscanf_s(entry.Remap->ParamTypeOverride, "float4[%d]", &index) == 1)
						sprintf_s(varName, "float4 %s[%d]", entry.Name, index);
					else
						sprintf_s(varName, "%s %s", entry.Remap->ParamTypeOverride, entry.Name);
				}
				else
				{
					// Undefined variable type; default to float4
					sprintf_s(varName, "float4 %s", entry.Name);
				}

				// Convert cbOffset to packoffset(c0) or packoffset(c0.x) or packoffset(c0.y) or .... to enforce compiler
				// buffer ordering
				uint8_t cbOffset = m_Shader->m_ConstantOffsets[entry.Index];
				char packOffset[64];

				switch (cbOffset % 4)
				{
				case 0:sprintf_s(packOffset, "packoffset(c%d)", cbOffset / 4); break;  // Normal register on 16 byte boundary
				case 1:sprintf_s(packOffset, "packoffset(c%d.y)", cbOffset / 4); break;// Requires swizzle index
				case 2:sprintf_s(packOffset, "packoffset(c%d.z)", cbOffset / 4); break;// Requires swizzle index
				case 3:sprintf_s(packOffset, "packoffset(c%d.w)", cbOffset / 4); break;// Requires swizzle index
				default:__debugbreak(); break;
				}

				fprintf(m_File, "\t%s", varName);

				// Add space alignment
				for (int i = 45 - max(0, strlen(varName)); i > 0; i--)
					fprintf(m_File, " ");

				fprintf(m_File, ": %s;", packOffset);

				// Add space alignment
				for (int i = 20 - max(0, strlen(packOffset)); i > 0; i--)
					fprintf(m_File, " ");

				fprintf(m_File, "// @ %d - 0x%04X\n", cbOffset, cbOffset * 4);
			}
		}

		fprintf(m_File, "}\n\n");
	}
};

void DumpVertexShader(BSVertexShader *Shader, const char *Type)
{
	if (!_stricmp(Type, "BloodSplatter"))
	{
	}
	else if (!_stricmp(Type, "DistantTree"))
	{
	}
	else if (!_stricmp(Type, "RunGrass"))
	{
	}
	else if (!_stricmp(Type, "Particle"))
	{
	}
	else if (!_stricmp(Type, "Sky"))
	{
	}
	else if (!_stricmp(Type, "Effect"))
	{
	}
	else if (!_stricmp(Type, "Lighting"))
	{
	}
	else if (!_stricmp(Type, "Utility"))
	{
	}
	else if (!_stricmp(Type, "Water"))
	{
	}
	else
		return;

	VertexShaderDecoder decoder(Type, Shader);
	decoder.SetShaderData((void *)((uintptr_t)Shader + sizeof(BSVertexShader)), Shader->m_ShaderLength);
	decoder.DumpShaderInfo();
}

void DumpPixelShader(BSPixelShader *Shader, const char *Type, void *Buffer, size_t BufferLen)
{
	if (!_stricmp(Type, "BloodSplatter"))
	{
	}
	else if (!_stricmp(Type, "DistantTree"))
	{
	}
	else if (!_stricmp(Type, "RunGrass"))
	{
	}
	else if (!_stricmp(Type, "Particle"))
	{
	}
	else if (!_stricmp(Type, "Sky"))
	{
	}
	else if (!_stricmp(Type, "Effect"))
	{
	}
	else if (!_stricmp(Type, "Lighting"))
	{
	}
	else if (!_stricmp(Type, "Utility"))
	{
	}
	else if (!_stricmp(Type, "Water"))
	{
	}
	else
		return;

	PixelShaderDecoder decoder(Type, Shader);
	decoder.SetShaderData(Buffer, BufferLen);
	decoder.DumpShaderInfo();
}

const char *GetShaderConstantName(const char *ShaderType, BSSM_SHADER_TYPE CodeType, int ConstantIndex)
{
	switch (CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX:
		if (!_stricmp(ShaderType, "BloodSplatter"))
			return BSBloodSplatterShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "DistantTree"))
			return BSDistantTreeShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "RunGrass"))
			return BSGrassShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Particle"))
			return BSParticleShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Sky"))
			return BSSkyShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Effect"))
			return BSXShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Lighting"))
			return BSLightingShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Utility"))
			return BSUtilityShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Water"))
			return BSWaterShader::VSConstants::GetString(ConstantIndex);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::PIXEL:
		if (!_stricmp(ShaderType, "BloodSplatter"))
			return BSBloodSplatterShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "DistantTree"))
			return BSDistantTreeShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "RunGrass"))
			return BSGrassShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Particle"))
			return BSParticleShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Sky"))
			return BSSkyShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Effect"))
			return BSXShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Lighting"))
			return BSLightingShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Utility"))
			return BSUtilityShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Water"))
			return BSWaterShader::PSConstants::GetString(ConstantIndex);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::COMPUTE:
		// TODO
		break;
	}

	return nullptr;
}

void GetShaderTechniqueName(const char *ShaderType, BSSM_SHADER_TYPE CodeType, char *Buffer, size_t BufferSize, uint32_t Technique)
{
	switch (CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX:
	case BSSM_SHADER_TYPE::PIXEL:
		if (!_stricmp(ShaderType, "BloodSplatter"))
			return BSBloodSplatterShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "DistantTree"))
			return BSDistantTreeShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "RunGrass"))
			return BSGrassShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "Particle"))
			return BSParticleShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "Sky"))
			return BSSkyShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "Effect"))
			return BSXShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "Lighting"))
			return BSLightingShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "Utility"))
			return BSUtilityShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(ShaderType, "Water"))
			return BSWaterShader::Techniques::GetString(Technique, Buffer, BufferSize);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::COMPUTE:
		// TODO
		break;
	}
}

const char *GetShaderSamplerName(const char *ShaderType, int ConstantIndex)
{
	if (!_stricmp(ShaderType, "BloodSplatter"))
		return BSBloodSplatterShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "DistantTree"))
		return BSDistantTreeShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "RunGrass"))
		return BSGrassShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "Particle"))
		return BSParticleShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "Sky"))
		return BSSkyShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "Effect"))
		return BSXShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "Lighting"))
		return BSLightingShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "Utility"))
		return BSUtilityShader::Samplers::GetString(ConstantIndex);
	else if (!_stricmp(ShaderType, "Water"))
		return BSWaterShader::Samplers::GetString(ConstantIndex);

	// TODO: Compute?
	// TODO: ImageSpace
	return nullptr;
}

void DumpComputeShader(BSComputeShader *Shader)
{
}

void ValidateShaderParamTable()
{
	//
	// Validate everything to check for typos:
	//
	// Invalid shader types
	// Invalid group types
	// Duplicate/invalid param indexes
	// Duplicate/invalid param names
	//
	auto doValidation = [](const RemapEntry *Entries, size_t EntryCount, BSSM_SHADER_TYPE CodeType)
	{
		const char *lastType = "";
		std::vector<std::string> paramNames;
		std::vector<int> paramIndexes;

		for (int i = 0; i < EntryCount; i++)
		{
			if (_stricmp(Entries[i].Type, lastType) != 0)
			{
				lastType = Entries[i].Type;
				paramNames.clear();
				paramIndexes.clear();
			}

			const int group = Entries[i].Group;
			const int index = Entries[i].Index;

			// Group must be Geometry, Material, or Technique
			if (group != CONSTANT_BUFFER_PER_GEOMETRY &&
				group != CONSTANT_BUFFER_PER_MATERIAL &&
				group != CONSTANT_BUFFER_PER_TECHNIQUE)
				printf("VALIDATION FAILURE: Group type for [%s, param index %d] is %d\n", lastType, index, group);

			// Check for dupe indexes
			if (std::find(paramIndexes.begin(), paramIndexes.end(), index) != paramIndexes.end())
			{
				printf("VALIDATION FAILURE: Duplicate parameter index %d for %s\n", index, lastType);
				continue;
			}

			paramIndexes.push_back(index);

			// Check if index is valid
			const char *constName = GetShaderConstantName(lastType, CodeType, index);

			if (!constName || strstr(constName, "Add-your-"))
			{
				printf("VALIDATION FAILURE: Invalid parameter name for index %d in %s\n", index, lastType);
				continue;
			}

			// Check for dupe names
			if (std::find(paramNames.begin(), paramNames.end(), constName) != paramNames.end())
			{
				printf("VALIDATION FAILURE: Duplicate parameter name %s for %s\n", constName, lastType);
				continue;
			}

			paramNames.push_back(constName);
		}
	};

	doValidation(VertexEntries, ARRAYSIZE(VertexEntries), BSSM_SHADER_TYPE::VERTEX);
	doValidation(PixelEntries, ARRAYSIZE(PixelEntries), BSSM_SHADER_TYPE::PIXEL);
}