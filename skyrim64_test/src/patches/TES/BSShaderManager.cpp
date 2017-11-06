#include "../../stdafx.h"
#include <direct.h>
#include <D3D11Shader.h>

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

ShaderDecoder::ShaderDecoder(const char *Type, BSSM_SHADER_TYPE CodeType)
{
	m_HlslData = nullptr;
	m_HlslDataLen = 0;
	strcpy_s(m_Type, Type);
	m_CodeType = CodeType;

	// Guarantee that the sub-folder exists
	char buf[1024];
	sprintf_s(buf, "C:\\myfolder\\%s\\", Type);
	_mkdir(buf);
}

ShaderDecoder::~ShaderDecoder()
{
	if (m_HlslData)
		delete[] m_HlslData;
}

void ShaderDecoder::SetShaderData(void *Buffer, size_t BufferSize)
{
	m_HlslData = new char[BufferSize];
	m_HlslDataLen = BufferSize;

	memcpy(m_HlslData, Buffer, BufferSize);
}

void ShaderDecoder::DumpShader()
{
	DumpShaderInfo();
}

void ShaderDecoder::DumpShaderInfo()
{
	// Build a list of all constants used
	std::vector<ParamIndexPair> geoIndexes;
	std::vector<ParamIndexPair> matIndexes;
	std::vector<ParamIndexPair> tecIndexes;
	std::vector<ParamIndexPair> undefinedIndexes;

	for (int i = 0; i < GetConstantArraySize(); i++)
	{
		const char *name = GetConstantName(i);

		if (strstr(name, "Add-your-"))
			break;

		const RemapEntry *remapData = nullptr;

		switch (m_CodeType)
		{
		case BSSM_SHADER_TYPE::VERTEX: remapData = GetEntryForVertexShader(m_Type, i); break;
		case BSSM_SHADER_TYPE::PIXEL: remapData = GetEntryForPixelShader(m_Type, i); break;
		default: remapData = nullptr; break;
		}

		if (!remapData)
		{
			// Indicates some kind of error
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

	// Technique name string (delimited by underscores)
	char technique[1024];
	GetTechniqueName(technique, ARRAYSIZE(technique), GetTechnique());

	for (int i = 0;; i++)
	{
		if (technique[i] == '\0')
			break;

		if (technique[i] == ' ')
			technique[i] = '_';
	}

	DumpShaderSpecific(technique, geoIndexes, matIndexes, tecIndexes, undefinedIndexes);
}

void ShaderDecoder::DumpCBuffer(FILE *File, BSConstantBufferInfo *Buffer, std::vector<ParamIndexPair> Params, int GroupIndex)
{
	D3D11_BUFFER_DESC desc;

	if (Buffer->m_Buffer)
	{
		Buffer->m_Buffer->GetDesc(&desc);
		fprintf(File, "// Dynamic buffer: Size = %d (0x%X)\n", desc.ByteWidth, desc.ByteWidth);
	}

	if (Buffer->m_Data)
		fprintf(File, "// Unmapped\n");

	fprintf(File, "cbuffer %s : register(%s)\n{\n", GetGroupName(GroupIndex), GetGroupRegister(GroupIndex));

	// Buffer undefined? Don't print anything
	if (Buffer->m_Buffer)
	{
		// Sort each variable by offset
		std::sort(Params.begin(), Params.end(),
			[this](ParamIndexPair& a1, ParamIndexPair& a2)
		{
			return GetConstantArray()[a1.Index] < GetConstantArray()[a2.Index];
		});

		for (auto& entry : Params)
		{
			// Generate var and type names
			char varName[256];

			if (entry.Remap->ParamTypeOverride)
			{
				const char *start = entry.Remap->ParamTypeOverride;
				const char *end = strchr(start, '[');

				if (end)
					sprintf_s(varName, "%.*s %s%s", strlen(start) - strlen(end), start, entry.Name, end);
				else
					sprintf_s(varName, "%s %s", start, entry.Name);
			}
			else
			{
				// Undefined type; default to float4
				sprintf_s(varName, "float4 %s", entry.Name);
			}

			// Convert cbOffset to packoffset(c0) or packoffset(c0.x) or packoffset(c0.y) or .... to enforce compiler
			// offset ordering
			uint8_t cbOffset = GetConstantArray()[entry.Index];
			char packOffset[64];

			switch (cbOffset % 4)
			{
			case 0:sprintf_s(packOffset, "packoffset(c%d)", cbOffset / 4); break;  // Normal register on 16 byte boundary
			case 1:sprintf_s(packOffset, "packoffset(c%d.y)", cbOffset / 4); break;// Requires swizzle index
			case 2:sprintf_s(packOffset, "packoffset(c%d.z)", cbOffset / 4); break;// Requires swizzle index
			case 3:sprintf_s(packOffset, "packoffset(c%d.w)", cbOffset / 4); break;// Requires swizzle index
			default:__debugbreak(); break;
			}

			fprintf(File, "\t%s", varName);

			// Add space alignment
			for (size_t i = 45 - max(0, strlen(varName)); i > 0; i--)
				fprintf(File, " ");

			fprintf(File, ": %s;", packOffset);

			// Add space alignment
			for (size_t i = 20 - max(0, strlen(packOffset)); i > 0; i--)
				fprintf(File, " ");

			fprintf(File, "// @ %d - 0x%04X\n", cbOffset, cbOffset * 4);
		}
	}

	fprintf(File, "}\n\n");
}

const char *ShaderDecoder::GetGroupName(int Index)
{
	switch (Index)
	{
	case 0:return "PerGeometry";
	case 1:return "PerMaterial";
	case 2:return "PerTechnique";
	}

	return nullptr;
}

const char *ShaderDecoder::GetGroupRegister(int Index)
{
	switch (Index)
	{
	case 0:return "b0";
	case 1:return "b1";
	case 2:return "b2";
	}

	return nullptr;
}

const char *ShaderDecoder::GetConstantName(int Index)
{
	return GetShaderConstantName(m_Type, m_CodeType, Index);
}

void ShaderDecoder::GetTechniqueName(char *Buffer, size_t BufferSize, uint32_t Technique)
{
	switch (m_CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX:
	case BSSM_SHADER_TYPE::PIXEL:
		if (!_stricmp(m_Type, "BloodSplatter"))
			return BSBloodSplatterShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "DistantTree"))
			return BSDistantTreeShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "RunGrass"))
			return BSGrassShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Particle"))
			return BSParticleShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Sky"))
			return BSSkyShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Effect"))
			return BSXShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Lighting"))
			return BSLightingShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Utility"))
			return BSUtilityShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Water"))
			return BSWaterShader::Techniques::GetString(Technique, Buffer, BufferSize);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::COMPUTE:
		// TODO
		break;
	}
}

const char *ShaderDecoder::GetSamplerName(int Index, uint32_t Technique)
{
	if (!_stricmp(m_Type, "BloodSplatter"))
		return BSBloodSplatterShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "DistantTree"))
		return BSDistantTreeShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "RunGrass"))
		return BSGrassShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Particle"))
		return BSParticleShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Sky"))
		return BSSkyShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Effect"))
		return BSXShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Lighting"))
		return BSLightingShader::Samplers::GetString(Index, Technique);
	else if (!_stricmp(m_Type, "Utility"))
		return BSUtilityShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Water"))
		return BSWaterShader::Samplers::GetString(Index);

	// TODO: Compute
	// TODO: ImageSpace
	return nullptr;
}

std::vector<std::pair<const char *, const char *>> ShaderDecoder::GetDefineArray(uint32_t Technique)
{
	if (!_stricmp(m_Type, "BloodSplatter"))
		return BSBloodSplatterShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "DistantTree"))
		return BSDistantTreeShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "RunGrass"))
		return BSGrassShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Particle"))
		return BSParticleShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Sky"))
		return BSSkyShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Effect"))
		return BSXShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Lighting"))
		return BSLightingShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Utility"))
		return BSUtilityShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Water"))
		return BSWaterShader::Defines::GetArray(Technique);

	// TODO: Compute
	// TODO: ImageSpace
	return std::vector<std::pair<const char *, const char *>>();
}

VertexShaderDecoder::VertexShaderDecoder(const char *Type, BSVertexShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::VERTEX)
{
	m_Shader = Shader;
}

void VertexShaderDecoder::DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined)
{
	char buf1[1024];
	sprintf_s(buf1, "C:\\myfolder\\%s\\%s_%s_%X.vs.txt", m_Type, m_Type, TechName, m_Shader->m_TechniqueID);

	// Something went really wrong if the shader exists already
	if (GetFileAttributesA(buf1) != INVALID_FILE_ATTRIBUTES)
		__debugbreak();

	FILE *file = fopen(buf1, "w");

	//char inputLayout[1024];
	//GetInputLayoutString(m_Shader->m_InputLayoutFlags, inputLayout, ARRAYSIZE(inputLayout));

	fprintf(file, "// %s\n", m_Type);
	fprintf(file, "// TechniqueID: 0x%X\n", m_Shader->m_TechniqueID);
	fprintf(file, "// Input flags: 0x%llX\n//\n", m_Shader->m_InputLayoutFlags);
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
		return (m_Shader->m_InputLayoutFlags & Bits) == Bits;
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

PixelShaderDecoder::PixelShaderDecoder(const char *Type, BSPixelShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::PIXEL)
{
	m_Shader = Shader;
}

void PixelShaderDecoder::DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined)
{
	char buf1[1024];
	sprintf_s(buf1, "C:\\myfolder\\%s\\%s_%s_%X.ps.txt", m_Type, m_Type, TechName, m_Shader->m_TechniqueID);

	// Something went really wrong if the shader exists already
	if (GetFileAttributesA(buf1) != INVALID_FILE_ATTRIBUTES)
		__debugbreak();

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
	decoder.DumpShader();
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
	decoder.DumpShader();
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