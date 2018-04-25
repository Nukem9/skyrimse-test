#include <direct.h>
#include "../../../common.h"
#include "../../rendering/common.h"
#include "BSShaderManager.h"
#include "BSShaderAccumulator.h"

void /*BSShaderManager::*/SetCurrentAccumulator(BSShaderAccumulator *Accumulator)
{
	auto GraphicsGlobals = HACK_GetThreadedGlobals();

	// BSShaderManager::pCurrentShaderAccumulator
	uint64_t& qword_1431F5490 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3600);

	qword_1431F5490 = (uint64_t)Accumulator;
}

BSShaderAccumulator /*BSShaderManager::*/*GetCurrentAccumulator()
{
	auto GraphicsGlobals = HACK_GetThreadedGlobals();

	// BSShaderManager::pCurrentShaderAccumulator
	return *(BSShaderAccumulator **)((uintptr_t)GraphicsGlobals + 0x3600);
}

void /*BSShaderManager::*/ClearShaderAndTechnique()
{
	auto GraphicsGlobals = HACK_GetThreadedGlobals();
	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	if (qword_1432A8218)
	{
		((BSShader *)qword_1432A8218)->RestoreTechnique(dword_1432A8214);
		qword_1432A8218 = 0;// Shader
	}

	dword_1432A8214 = 0;// Technique
	qword_1434B5220 = 0;
}

bool /*BSShaderManager::*/SetupShaderAndTechnique(BSShader *Shader, uint32_t Technique)
{
	ClearShaderAndTechnique();

	auto GraphicsGlobals = HACK_GetThreadedGlobals();
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);

	if (Shader->SetupTechnique(Technique))
	{
		qword_1432A8218 = (uint64_t)Shader;
		dword_1432A8214 = Technique;
		return true;
	}

	qword_1432A8218 = 0;
	dword_1432A8214 = 0;
	return false;
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

		const BSShaderMappings::Entry *remapData = nullptr;

		switch (m_CodeType)
		{
		case BSSM_SHADER_TYPE::VERTEX: remapData = BSShaderMappings::GetEntryForVertexShader(m_Type, i); break;
		case BSSM_SHADER_TYPE::PIXEL: remapData = BSShaderMappings::GetEntryForPixelShader(m_Type, i); break;
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
			case BSSM_GROUP_TYPE::PER_GEO:
				geoIndexes.push_back({ i, name, remapData });
				break;
			case BSSM_GROUP_TYPE::PER_MAT:
				matIndexes.push_back({ i, name, remapData });
				break;
			case BSSM_GROUP_TYPE::PER_TEC:
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

void ShaderDecoder::DumpCBuffer(FILE *File, BSConstantGroup *Buffer, std::vector<ParamIndexPair> Params, int GroupIndex)
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
					sprintf_s(varName, "%.*s %s%s", (int)(strlen(start) - strlen(end)), start, entry.Name, end);
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
			return BSShaderInfo::BSBloodSplatterShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "DistantTree"))
			return BSShaderInfo::BSDistantTreeShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "RunGrass"))
			return BSShaderInfo::BSGrassShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Particle"))
			return BSShaderInfo::BSParticleShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Sky"))
			return BSShaderInfo::BSSkyShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Effect"))
			return BSShaderInfo::BSXShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Lighting"))
			return BSShaderInfo::BSLightingShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Utility"))
			return BSShaderInfo::BSUtilityShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_Type, "Water"))
			return BSShaderInfo::BSWaterShader::Techniques::GetString(Technique, Buffer, BufferSize);

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
		return BSShaderInfo::BSBloodSplatterShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "DistantTree"))
		return BSShaderInfo::BSDistantTreeShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "RunGrass"))
		return BSShaderInfo::BSGrassShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Particle"))
		return BSShaderInfo::BSParticleShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Sky"))
		return BSShaderInfo::BSSkyShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Effect"))
		return BSShaderInfo::BSXShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Lighting"))
		return BSShaderInfo::BSLightingShader::Samplers::GetString(Index, Technique);
	else if (!_stricmp(m_Type, "Utility"))
		return BSShaderInfo::BSUtilityShader::Samplers::GetString(Index);
	else if (!_stricmp(m_Type, "Water"))
		return BSShaderInfo::BSWaterShader::Samplers::GetString(Index);

	// TODO: Compute
	// TODO: ImageSpace
	return nullptr;
}

std::vector<std::pair<const char *, const char *>> ShaderDecoder::GetDefineArray(uint32_t Technique)
{
	if (!_stricmp(m_Type, "BloodSplatter"))
		return BSShaderInfo::BSBloodSplatterShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "DistantTree"))
		return BSShaderInfo::BSDistantTreeShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "RunGrass"))
		return BSShaderInfo::BSGrassShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Particle"))
		return BSShaderInfo::BSParticleShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Sky"))
		return BSShaderInfo::BSSkyShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Effect"))
		return BSShaderInfo::BSXShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Lighting"))
		return BSShaderInfo::BSLightingShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Utility"))
		return BSShaderInfo::BSUtilityShader::Defines::GetArray(Technique);
	else if (!_stricmp(m_Type, "Water"))
		return BSShaderInfo::BSWaterShader::Defines::GetArray(Technique);

	// TODO: Compute
	// TODO: ImageSpace
	return std::vector<std::pair<const char *, const char *>>();
}

void DumpVertexShader(BSGraphics::VertexShader *Shader, const char *Type)
{
	/*
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
	decoder.DumpShader();*/
}

void DumpPixelShader(BSGraphics::PixelShader *Shader, const char *Type, void *Buffer, size_t BufferLen)
{
	/*
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
	decoder.DumpShader();*/
}

const char *GetShaderConstantName(const char *ShaderType, BSSM_SHADER_TYPE CodeType, int ConstantIndex)
{
	switch (CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX:
		if (!_stricmp(ShaderType, "BloodSplatter"))
			return BSShaderInfo::BSBloodSplatterShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "DistantTree"))
			return BSShaderInfo::BSDistantTreeShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "RunGrass"))
			return BSShaderInfo::BSGrassShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Particle"))
			return BSShaderInfo::BSParticleShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Sky"))
			return BSShaderInfo::BSSkyShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Effect"))
			return BSShaderInfo::BSXShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Lighting"))
			return BSShaderInfo::BSLightingShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Utility"))
			return BSShaderInfo::BSUtilityShader::VSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Water"))
			return BSShaderInfo::BSWaterShader::VSConstants::GetString(ConstantIndex);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::PIXEL:
		if (!_stricmp(ShaderType, "BloodSplatter"))
			return BSShaderInfo::BSBloodSplatterShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "DistantTree"))
			return BSShaderInfo::BSDistantTreeShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "RunGrass"))
			return BSShaderInfo::BSGrassShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Particle"))
			return BSShaderInfo::BSParticleShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Sky"))
			return BSShaderInfo::BSSkyShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Effect"))
			return BSShaderInfo::BSXShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Lighting"))
			return BSShaderInfo::BSLightingShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Utility"))
			return BSShaderInfo::BSUtilityShader::PSConstants::GetString(ConstantIndex);
		else if (!_stricmp(ShaderType, "Water"))
			return BSShaderInfo::BSWaterShader::PSConstants::GetString(ConstantIndex);

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
	auto doValidation = [](const BSShaderMappings::Entry *Entries, size_t EntryCount, BSSM_SHADER_TYPE CodeType)
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

			const BSSM_GROUP_TYPE group = Entries[i].Group;
			const int index = Entries[i].Index;

			// Group must be Geometry, Material, or Technique
			if (group != BSSM_GROUP_TYPE::PER_GEO &&
				group != BSSM_GROUP_TYPE::PER_MAT &&
				group != BSSM_GROUP_TYPE::PER_TEC)
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

	doValidation(BSShaderMappings::Vertex, ARRAYSIZE(BSShaderMappings::Vertex), BSSM_SHADER_TYPE::VERTEX);
	doValidation(BSShaderMappings::Pixel, ARRAYSIZE(BSShaderMappings::Pixel), BSSM_SHADER_TYPE::PIXEL);
}