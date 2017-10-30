#include "../../stdafx.h"
#include <direct.h>
#include <D3D11Shader.h>
#include <D3Dcompiler.h>

struct RemapEntry
{
	const char *Type;
	int Group;
	int Index;
};

const RemapEntry VertexEntries[] =
{
#define REMAP_VERTEX_UNUSED(ShaderType, GroupType)
#define REMAP_VERTEX(ShaderType, GroupType, ParameterIndex) { ShaderType, GroupType, ParameterIndex },
#include "BSShaderConstants.inl"
};

const RemapEntry PixelEntries[] =
{
#define REMAP_PIXEL_UNUSED(ShaderType, GroupType)
#define REMAP_PIXEL(ShaderType, GroupType, ParameterIndex) { ShaderType, GroupType, ParameterIndex },
#include "BSShaderConstants.inl"
};

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

class ShaderDecoder
{
protected:
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

		char buf[1024];
		sprintf_s(buf, "C:\\myfolder\\%s\\", Type);
		mkdir(buf);

		sprintf_s(buf, "C:\\myfolder\\%s\\vs_%llX.txt", Type, Shader);
		m_File = fopen(buf, "w");
	}

	~VertexShaderDecoder()
	{
		if (m_File)
			fclose(m_File);
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

		DumpCBuffer(&m_Shader->m_PerGeometry, geoIndexes, 0);// Constant buffer 0 : register(b0)
		DumpCBuffer(&m_Shader->m_PerMaterial, matIndexes, 1);// Constant buffer 1 : register(b1)
		DumpCBuffer(&m_Shader->m_PerTechnique, tecIndexes, 2);// Constant buffer 2 : register(b2)

		// Dump undefined variables
		for (auto& entry : undefinedIndexes)
			fprintf(m_File, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);

		// Now write raw HLSL
		char buf[1024];
		sprintf_s(buf, "C:\\myfolder\\%s\\vs_%llX.hlsl", m_Type, m_Shader);
		FILE *f = fopen(buf, "wb");

		if (f)
		{
			fwrite((void *)((uintptr_t)m_Shader + sizeof(BSVertexShader)), 1, m_Shader->m_ShaderLength, f);
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
			fprintf(m_File, "// Static buffer: Unmapped\n");

		fprintf(m_File, "cbuffer %s : register(%s)\n{\n", GetGroupName(GroupIndex), GetGroupRegister(GroupIndex));

		// Don't print any variables if this buffer is undefined
		if (Buffer->m_Buffer)
		{
			// Sort each variable by their offset in the buffer
			std::sort(Params.begin(), Params.end(),
				[this](ParamIndexPair& a1, ParamIndexPair& a2)
			{
				return m_Shader->m_ConstantOffsets[a1.Index] < m_Shader->m_ConstantOffsets[a2.Index];
			});

			// Now dump it to the file
			for (auto& entry : Params)
			{
				uint8_t cbOffset = m_Shader->m_ConstantOffsets[entry.Index];
				int padding = max(40 - strlen(entry.Name), 0);

				fprintf(m_File, "\tfloat4 %s;", entry.Name);
				for (int i = 0; i < padding; i++) fprintf(m_File, " ");
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

		char buf[1024];
		sprintf_s(buf, "C:\\myfolder\\%s\\", Type);
		mkdir(buf);

		sprintf_s(buf, "C:\\myfolder\\%s\\ps_%llX.txt", Type, Shader);
		m_File = fopen(buf, "w");
	}

	~PixelShaderDecoder()
	{
		if (m_File)
			fclose(m_File);
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

		DumpCBuffer(&m_Shader->m_PerGeometry, geoIndexes, 0); // Constant buffer 0 : register(b0)
		DumpCBuffer(&m_Shader->m_PerMaterial, matIndexes, 1); // Constant buffer 1 : register(b1)
		DumpCBuffer(&m_Shader->m_PerTechnique, tecIndexes, 2);// Constant buffer 2 : register(b2)

		// Dump undefined variables
		for (auto& entry : undefinedIndexes)
			fprintf(m_File, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);
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
			fprintf(m_File, "// Static buffer: Unmapped\n");

		fprintf(m_File, "cbuffer %s : register(%s)\n{\n", GetGroupName(GroupIndex), GetGroupRegister(GroupIndex));

		// Don't print any variables if this buffer is undefined
		if (Buffer->m_Buffer)
		{
			// Sort each variable by their offset in the buffer
			std::sort(Params.begin(), Params.end(),
				[this](ParamIndexPair& a1, ParamIndexPair& a2)
			{
				return m_Shader->m_ConstantOffsets[a1.Index] < m_Shader->m_ConstantOffsets[a2.Index];
			});

			// Now dump it to the file
			for (auto& entry : Params)
			{
				uint8_t cbOffset = m_Shader->m_ConstantOffsets[entry.Index];
				int padding = max(40 - strlen(entry.Name), 0);

				fprintf(m_File, "\tfloat4 %s;", entry.Name);
				for (int i = 0; i < padding; i++) fprintf(m_File, " ");
				fprintf(m_File, "// @ %d - 0x%04X\n", cbOffset, cbOffset * 4);
			}
		}

		fprintf(m_File, "}\n\n");
	}
};

void DumpVertexShader(BSVertexShader *Shader, const char *Type, std::function<const char *(int)> GetConstantFunc, std::function<int(int, int)> GetSizeFunc)
{
	VertexShaderDecoder decoder(Type, Shader);
	decoder.DumpShaderInfo();
}

void DumpVertexShader(BSVertexShader *Shader, const char *Type)
{
	std::function<const char *(int)> constantFunc = [](int) { return "UNKNOWN VERTEX SHADER TYPE"; };
	std::function<int(int,int)> sizeFunc = [](int, int) { return 0; };

	if (!_stricmp(Type, "BloodSplatter"))
	{
		constantFunc = BSBloodSplatterShaderVertexConstants::GetString;
		sizeFunc = BSBloodSplatterShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "DistantTree"))
	{
		constantFunc = BSDistantTreeShaderVertexConstants::GetString;
		sizeFunc = BSDistantTreeShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "RunGrass"))
	{
		constantFunc = BSGrassShaderVertexConstants::GetString;
		sizeFunc = BSGrassShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "Particle"))
	{
		constantFunc = BSParticleShaderVertexConstants::GetString;
		sizeFunc = BSParticleShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "Sky"))
	{
		constantFunc = BSSkyShaderVertexConstants::GetString;
		sizeFunc = BSSkyShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "Effect"))
	{
		constantFunc = BSXShaderVertexConstants::GetString;
		sizeFunc = BSXShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "Lighting"))
	{
		constantFunc = BSLightingShaderVertexConstants::GetString;
		sizeFunc = BSLightingShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "Utility"))
	{
		constantFunc = BSUtilityShaderVertexConstants::GetString;
		sizeFunc = BSUtilityShaderVertexConstants::GetSize;
	}
	else if (!_stricmp(Type, "Water"))
	{
		constantFunc = BSWaterShaderVertexConstants::GetString;
		sizeFunc = BSWaterShaderVertexConstants::GetSize;
	}
	else
		return;

	DumpVertexShader(Shader, Type, constantFunc, sizeFunc);
}

void DumpPixelShader(BSPixelShader *Shader, const char *Type, std::function<const char *(int)> GetConstantFunc, std::function<int(int, int)> GetSizeFunc)
{
	PixelShaderDecoder decoder(Type, Shader);
	decoder.DumpShaderInfo();
}

void DumpPixelShader(BSPixelShader *Shader, const char *Type)
{
	std::function<const char *(int)> constantFunc = [](int) { return "UNKNOWN PIXEL SHADER TYPE"; };
	std::function<int(int, int)> sizeFunc = [](int, int) { return 0; };

	if (!_stricmp(Type, "BloodSplatter"))
	{
		constantFunc = BSBloodSplatterShaderPixelConstants::GetString;
		sizeFunc = BSBloodSplatterShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "DistantTree"))
	{
		constantFunc = BSDistantTreeShaderPixelConstants::GetString;
		sizeFunc = BSDistantTreeShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "RunGrass"))
	{
		constantFunc = BSGrassShaderPixelConstants::GetString;
		sizeFunc = BSGrassShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "Particle"))
	{
		constantFunc = BSParticleShaderPixelConstants::GetString;
		sizeFunc = BSParticleShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "Sky"))
	{
		constantFunc = BSSkyShaderPixelConstants::GetString;
		sizeFunc = BSSkyShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "Effect"))
	{
		constantFunc = BSXShaderPixelConstants::GetString;
		sizeFunc = BSXShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "Lighting"))
	{
		constantFunc = BSLightingShaderPixelConstants::GetString;
		sizeFunc = BSLightingShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "Utility"))
	{
		constantFunc = BSUtilityShaderPixelConstants::GetString;
		sizeFunc = BSUtilityShaderPixelConstants::GetSize;
	}
	else if (!_stricmp(Type, "Water"))
	{
		constantFunc = BSWaterShaderPixelConstants::GetString;
		sizeFunc = BSWaterShaderPixelConstants::GetSize;
	}
	else
		return;

	DumpPixelShader(Shader, Type, constantFunc, sizeFunc);
}

const char *GetShaderConstantName(const char *ShaderType, BSSM_SHADER_TYPE CodeType, int ConstantIndex)
{
	switch (CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX:
		if (!_stricmp(ShaderType, "BloodSplatter"))
			return BSBloodSplatterShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "DistantTree"))
			return BSDistantTreeShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "RunGrass"))
			return BSGrassShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Particle"))
			return BSParticleShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Sky"))
			return BSSkyShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Effect"))
			return BSXShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Lighting"))
			return BSLightingShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Utility"))
			return BSUtilityShaderVertexConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Water"))
			return BSWaterShaderVertexConstants::GetString(ConstantIndex);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::PIXEL:
		if (!_stricmp(ShaderType, "BloodSplatter"))
			return BSBloodSplatterShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "DistantTree"))
			return BSDistantTreeShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "RunGrass"))
			return BSGrassShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Particle"))
			return BSParticleShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Sky"))
			return BSSkyShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Effect"))
			return BSXShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Lighting"))
			return BSLightingShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Utility"))
			return BSUtilityShaderPixelConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Water"))
			return BSWaterShaderPixelConstants::GetString(ConstantIndex);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::COMPUTE:
		// TODO
		break;
	}

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