#include <direct.h>
#include "../../../common.h"
#include "BSShaderManager.h"
#include "BSShader_Dumper.h"
#include "BSShader.h"

#define SHADER_DUMP_PATH "C:\\ShaderDump"

ShaderDecoder::ShaderDecoder(const char *Type, BSSM_SHADER_TYPE CodeType)
{
	m_HlslData = nullptr;
	m_HlslDataLen = 0;
	m_Type = BSShaderManager::BSSM_SHADER_INVALID;
	strcpy_s(m_LoaderType, Type);
	m_CodeType = CodeType;

	// Convert string to enum
	if (!_stricmp(m_LoaderType, "RunGrass"))
		m_Type = BSShaderManager::BSSM_SHADER_RUNGRASS;
	else if (!_stricmp(m_LoaderType, "Sky"))
		m_Type = BSShaderManager::BSSM_SHADER_SKY;
	else if (!_stricmp(m_LoaderType, "Water"))
		m_Type = BSShaderManager::BSSM_SHADER_WATER;
	else if (!_stricmp(m_LoaderType, "BloodSplatter"))
		m_Type = BSShaderManager::BSSM_SHADER_BLOODSPLATTER;
	else if (!_stricmp(m_LoaderType, "ImageSpace-FIXME"))
		m_Type = BSShaderManager::BSSM_SHADER_IMAGESPACE;
	else if (!_stricmp(m_LoaderType, "Lighting"))
		m_Type = BSShaderManager::BSSM_SHADER_LIGHTING;
	else if (!_stricmp(m_LoaderType, "Effect"))
		m_Type = BSShaderManager::BSSM_SHADER_EFFECT;
	else if (!_stricmp(m_LoaderType, "Utility"))
		m_Type = BSShaderManager::BSSM_SHADER_UTILITY;
	else if (!_stricmp(m_LoaderType, "DistantTree"))
		m_Type = BSShaderManager::BSSM_SHADER_DISTANTTREE;
	else if (!_stricmp(m_LoaderType, "Particle"))
		m_Type = BSShaderManager::BSSM_SHADER_PARTICLE;

	// Guarantee that the sub-folder exists
	if (m_Type != BSShaderManager::BSSM_SHADER_INVALID)
	{
		char buf[1024];
		sprintf_s(buf, "%s\\%s\\", SHADER_DUMP_PATH, m_LoaderType);

		_mkdir(buf);
	}
}

ShaderDecoder::~ShaderDecoder()
{
	delete[] m_HlslData;
}

void ShaderDecoder::SetShaderData(void *Buffer, size_t BufferSize)
{
	m_HlslData = new uint8_t[BufferSize];
	m_HlslDataLen = BufferSize;

	memcpy(m_HlslData, Buffer, BufferSize);
}

void ShaderDecoder::DumpShader()
{
	if (m_Type == BSShaderManager::BSSM_SHADER_INVALID)
		return;

	// Build a list of all constants used
	std::vector<ParamIndexPair> tecIndexes;
	std::vector<ParamIndexPair> matIndexes;
	std::vector<ParamIndexPair> geoIndexes;
	std::vector<ParamIndexPair> undefinedIndexes;

	for (int i = 0; i < GetConstantArraySize(); i++)
	{
		const char *name = GetConstantName(i);

		if (!name || strstr(name, "Add-your-"))
			break;

		switch (GetVariableCategory(i))
		{
		case ShaderDescriptor::DeclType::PER_TEC:
			tecIndexes.push_back({ i, name });
			break;
		case ShaderDescriptor::DeclType::PER_MAT:
			matIndexes.push_back({ i, name });
			break;
		case ShaderDescriptor::DeclType::PER_GEO:
			geoIndexes.push_back({ i, name });
			break;
		default:
			// Indicates some kind of error
			undefinedIndexes.push_back({ i, name });
			break;
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

void ShaderDecoder::DumpCBuffer(FILE *File, BSGraphics::Buffer *Buffer, std::vector<ParamIndexPair> Params, int GroupIndex)
{
	// NOTE: Some buffers might be undefined (= unused in shader) but offsets are still valid
	if (Buffer->m_Buffer)
	{
		D3D11_BUFFER_DESC desc;
		Buffer->m_Buffer->GetDesc(&desc);

		fprintf(File, "// Dynamic buffer: sizeof() = %d (0x%X)\n", desc.ByteWidth, desc.ByteWidth);
	}

	fprintf(File, "cbuffer %s : register(%s)\n{\n", GetGroupName(GroupIndex), GetGroupRegister(GroupIndex));

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

		const char *paramType = GetVariableType(entry.Index);

		if (paramType)
		{
			const char *start = paramType;
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
		for (int64_t i = 45 - std::max<int64_t>(0, strlen(varName)); i > 0; i--)
			fprintf(File, " ");

		fprintf(File, ": %s;", packOffset);

		// Add space alignment
		for (int64_t i = 20 - std::max<int64_t>(0, strlen(packOffset)); i > 0; i--)
			fprintf(File, " ");

		fprintf(File, "// @ %d - 0x%04X\n", cbOffset, cbOffset * 4);
	}

	fprintf(File, "}\n\n");
}

const char *ShaderDecoder::GetGroupName(int Index)
{
	switch (Index)
	{
	case 0:return "PerTechnique";
	case 1:return "PerMaterial";
	case 2:return "PerGeometry";
	case 11:return "AlphaTestRefCB";
	case 12:return "PerFrame";
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
	case 3:return "b3";
	case 4:return "b4";
	case 5:return "b5";
	case 6:return "b6";
	case 7:return "b7";
	case 8:return "b8";
	case 9:return "b9";
	case 10:return "b10";
	case 11:return "b11";
	case 12:return "b12";
	}

	return nullptr;
}

const char *ShaderDecoder::GetConstantName(int Index)
{
	switch (m_CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX:
		return BSShader::GetVSConstantName(m_Type, Index);

	case BSSM_SHADER_TYPE::PIXEL:
		return BSShader::GetPSConstantName(m_Type, Index);

	case BSSM_SHADER_TYPE::COMPUTE:
		Assert(false);
		break;
	}

	return nullptr;
}

ShaderDescriptor::DeclType ShaderDecoder::GetVariableCategory(int Index)
{
	auto category = BSShader::GetVariableCategory(m_Type, GetConstantName(Index));

	if (category != ShaderDescriptor::INVALID_DECL_TYPE)
		return category;

	const BSShaderMappings::Entry *remapData = nullptr;

	switch (m_CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX: remapData = BSShaderMappings::GetEntryForVertexShader(m_LoaderType, Index); break;
	case BSSM_SHADER_TYPE::PIXEL: remapData = BSShaderMappings::GetEntryForPixelShader(m_LoaderType, Index); break;
	}

	if (remapData)
	{
		switch (remapData->Group)
		{
		case BSSM_GROUP_TYPE::PER_TEC: return ShaderDescriptor::PER_TEC;
		case BSSM_GROUP_TYPE::PER_MAT: return ShaderDescriptor::PER_MAT;
		case BSSM_GROUP_TYPE::PER_GEO: return ShaderDescriptor::PER_GEO;
		}
	}

	return ShaderDescriptor::INVALID_DECL_TYPE;
}

const char *ShaderDecoder::GetVariableType(int Index)
{
	const char *paramType = BSShader::GetVariableType(m_Type, GetConstantName(Index));

	if (paramType)
		return paramType;

	const BSShaderMappings::Entry *remapData = nullptr;
	
	switch (m_CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX: remapData = BSShaderMappings::GetEntryForVertexShader(m_LoaderType, Index); break;
	case BSSM_SHADER_TYPE::PIXEL: remapData = BSShaderMappings::GetEntryForPixelShader(m_LoaderType, Index); break;
	}

	if (remapData)
		return remapData->ParamTypeOverride;

	return nullptr;
}

void ShaderDecoder::GetTechniqueName(char *Buffer, size_t BufferSize, uint32_t Technique)
{
	switch (m_CodeType)
	{
	case BSSM_SHADER_TYPE::VERTEX:
	case BSSM_SHADER_TYPE::PIXEL:
		if (!_stricmp(m_LoaderType, "BloodSplatter"))
			return BSShaderInfo::BSBloodSplatterShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "DistantTree"))
			return BSShaderInfo::BSDistantTreeShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "RunGrass"))
			return BSShaderInfo::BSGrassShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "Particle"))
			return BSShaderInfo::BSParticleShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "Sky"))
			return BSShaderInfo::BSSkyShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "Effect"))
			return BSShaderInfo::BSXShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "Lighting"))
			return BSShaderInfo::BSLightingShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "Utility"))
			return BSShaderInfo::BSUtilityShader::Techniques::GetString(Technique, Buffer, BufferSize);
		else if (!_stricmp(m_LoaderType, "Water"))
			return BSShaderInfo::BSWaterShader::Techniques::GetString(Technique, Buffer, BufferSize);

		// TODO: ImageSpace
		break;

	case BSSM_SHADER_TYPE::COMPUTE:
		Assert(false);
		break;
	}
}

/*
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
*/

//
// VertexShaderDecoder
//
VertexShaderDecoder::VertexShaderDecoder(const char *Type, BSGraphics::VertexShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::VERTEX)
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
	return ARRAYSIZE(BSGraphics::VertexShader::m_ConstantOffsets);
}

void VertexShaderDecoder::DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined)
{
	char buf[1024];
	sprintf_s(buf, "%s\\%s\\%s_%s_%X.vs.txt", SHADER_DUMP_PATH, m_LoaderType, m_LoaderType, TechName, m_Shader->m_TechniqueID);

	// Something went really wrong if the shader exists already
	AssertMsg(GetFileAttributesA(buf) == INVALID_FILE_ATTRIBUTES, "Trying to overwrite a shader that already exists!");

	if (FILE *file; fopen_s(&file, buf, "w") == 0)
	{
		fprintf(file, "// %s\n", m_LoaderType);
		fprintf(file, "// TechniqueID: 0x%X\n", m_Shader->m_TechniqueID);
		fprintf(file, "// Vertex description: 0x%llX\n//\n", m_Shader->m_VertexDescription);
		fprintf(file, "// Technique: %s\n\n", TechName);

		// Defines
		if (auto& defs = BSShader::GetSourceDefines(m_Type, m_Shader->m_TechniqueID); defs.size() > 0)
		{
			for (const auto& define : defs)
				fprintf(file, "#define %s %s\n", define.first, define.second);

			fprintf(file, "\n");
		}

		DumpCBuffer(file, &m_Shader->m_PerTechnique, PerTec, 0);// Constant buffer 0 : register(b0)
		DumpCBuffer(file, &m_Shader->m_PerMaterial, PerMat, 1); // Constant buffer 1 : register(b1)
		DumpCBuffer(file, &m_Shader->m_PerGeometry, PerGeo, 2); // Constant buffer 2 : register(b2)

		// Dump undefined variables
		for (auto& entry : Undefined)
			fprintf(file, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);

		fclose(file);
	}

	// Now write raw HLSL
	if (m_HlslData)
	{
		sprintf_s(buf, "%s\\%s\\%s_%s_%X.vs.bin", SHADER_DUMP_PATH, m_LoaderType, m_LoaderType, TechName, m_Shader->m_TechniqueID);

		if (FILE *file; fopen_s(&file, buf, "wb") == 0)
		{
			fwrite(m_HlslData, 1, m_HlslDataLen, file);
			fclose(file);
		}
	}
}

//
// PixelShaderDecoder
//
PixelShaderDecoder::PixelShaderDecoder(const char *Type, BSGraphics::PixelShader *Shader) : ShaderDecoder(Type, BSSM_SHADER_TYPE::PIXEL)
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
	return ARRAYSIZE(BSGraphics::PixelShader::m_ConstantOffsets);
}

void PixelShaderDecoder::DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined)
{
	char buf[1024];
	sprintf_s(buf, "%s\\%s\\%s_%s_%X.ps.txt", SHADER_DUMP_PATH, m_LoaderType, m_LoaderType, TechName, m_Shader->m_TechniqueID);

	// Something went really wrong if the shader exists already
	AssertMsg(GetFileAttributesA(buf) == INVALID_FILE_ATTRIBUTES, "Trying to overwrite a shader that already exists!");

	if (FILE *file; fopen_s(&file, buf, "w") == 0)
	{
		fprintf(file, "// %s\n", m_LoaderType);
		fprintf(file, "// TechniqueID: 0x%X\n//\n", m_Shader->m_TechniqueID);
		fprintf(file, "// Technique: %s\n\n", TechName);

		// Defines
		if (auto& defs = BSShader::GetSourceDefines(m_Type, m_Shader->m_TechniqueID); defs.size() > 0)
		{
			for (auto& define : defs)
				fprintf(file, "#define %s %s\n", define.first, define.second);

			fprintf(file, "\n");
		}

		// Samplers
		for (int i = 0;; i++)
		{
			const char *name = BSShader::GetPSSamplerName(m_Type, i/*, m_Shader->m_TechniqueID*/);

			if (!name || strstr(name, "Add-your-"))
				break;

			fprintf(file, "// Sampler[%d]: %s\n", i, name);
		}

		fprintf(file, "\n");

		DumpCBuffer(file, &m_Shader->m_PerTechnique, PerTec, 0);// Constant buffer 0 : register(b0)
		DumpCBuffer(file, &m_Shader->m_PerMaterial, PerMat, 1); // Constant buffer 1 : register(b1)
		DumpCBuffer(file, &m_Shader->m_PerGeometry, PerGeo, 2); // Constant buffer 2 : register(b2)

		// Dump undefined variables
		for (auto& entry : Undefined)
			fprintf(file, "// UNDEFINED PARAMETER: Index: %02d Offset: 0x%04X Name: %s\n", entry.Index, m_Shader->m_ConstantOffsets[entry.Index] * 4, entry.Name);

		fclose(file);
	}

	// Now write raw HLSL
	if (m_HlslData)
	{
		sprintf_s(buf, "%s\\%s\\%s_%s_%X.ps.bin", SHADER_DUMP_PATH, m_LoaderType, m_LoaderType, TechName, m_Shader->m_TechniqueID);

		if (FILE *file; fopen_s(&file, buf, "wb") == 0)
		{
			fwrite(m_HlslData, 1, m_HlslDataLen, file);
			fclose(file);
		}
	}
}