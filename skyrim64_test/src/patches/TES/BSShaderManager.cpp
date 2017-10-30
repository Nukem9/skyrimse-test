#include "../../stdafx.h"
#include <direct.h>

#include <D3D11Shader.h>
#include <D3Dcompiler.h>
void DumpVertexShader(BSVertexShader *Shader, const char *Type, std::function<const char *(int)> GetConstantFunc, std::function<int(int, int)> GetSizeFunc)
{
	char buf[1024];
	sprintf_s(buf, "C:\\myfolder\\%s\\", Type);
	mkdir(buf);

	sprintf_s(buf, "C:\\myfolder\\%s\\vs_%llX.txt", Type, Shader);

	FILE *f = fopen(buf, "w");

	// Constant buffer 0 : register(b0)
	if (Shader->m_PerGeometry.m_Buffer)
	{
		D3D11_BUFFER_DESC desc;
		Shader->m_PerGeometry.m_Buffer->GetDesc(&desc);
		fprintf(f, "// Dynamic buffer: Size = %d\n", desc.ByteWidth);
	}

	if (Shader->m_PerGeometry.m_Data)
		fprintf(f, "// Static buffer: Unmapped\n");

	fprintf(f, "cbuffer PerGeometry : register(b0)\n{\n");
	fprintf(f, "}\n\n");

	// Constant buffer 1 : register(b1)
	if (Shader->m_PerMaterial.m_Buffer)
	{
		D3D11_BUFFER_DESC desc;
		Shader->m_PerMaterial.m_Buffer->GetDesc(&desc);
		fprintf(f, "// Dynamic buffer: Size = %d\n", desc.ByteWidth);
	}

	if (Shader->m_PerMaterial.m_Data)
		fprintf(f, "// Static buffer: Unmapped\n");

	fprintf(f, "cbuffer PerMaterial : register(b1)\n{\n");
	fprintf(f, "}\n\n");

	// Constant buffer 2 : register(b2)
	if (Shader->m_PerTechnique.m_Buffer)
	{
		D3D11_BUFFER_DESC desc;
		Shader->m_PerTechnique.m_Buffer->GetDesc(&desc);
		fprintf(f, "// Dynamic buffer: Size = %d\n", desc.ByteWidth);
	}

	if (Shader->m_PerTechnique.m_Data)
		fprintf(f, "// Static buffer: Unmapped\n");

	fprintf(f, "cbuffer PerTechnique : register(b2)\n{\n");

	for (int i = 0; i < ARRAYSIZE(BSVertexShader::m_ConstantOffsets); i++)
	{
		uint8_t cbOffset = Shader->m_ConstantOffsets[i];
		const char *name = GetConstantFunc(i);

		if (strstr(name, "Add-your-"))
			break;

		fprintf(f, "\tfloat4 %s; \t\t// @ %d - 0x%04X Size: %d\n", name, cbOffset, cbOffset * 4, GetSizeFunc(i, 0));
	}

	fprintf(f, "}\n");
	fclose(f);

	// Now write raw HLSL
	sprintf_s(buf, "C:\\myfolder\\%s\\vs_%llX.hlsl", Type, Shader);
	f = fopen(buf, "wb");

	if (f)
	{
		fwrite((void *)((uintptr_t)Shader + sizeof(BSVertexShader)), 1, Shader->m_ShaderLength, f);
		fclose(f);
	}
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
	uint8_t cbOffset = 0;
	uint8_t previous = 0;

	char buf[1024];
	sprintf_s(buf, "C:\\myfolder\\%s\\", Type);
	mkdir(buf);

	sprintf_s(buf, "C:\\myfolder\\%s\\ps_%llX.txt", Type, Shader);

	FILE *f = fopen(buf, "w");

	// Constant buffer 0 : register(b0)
	if (Shader->m_PerGeometry.m_Buffer)
	{
		D3D11_BUFFER_DESC desc;
		Shader->m_PerGeometry.m_Buffer->GetDesc(&desc);
		fprintf(f, "// Dynamic buffer: Size = %d\n", desc.ByteWidth);
	}

	if (Shader->m_PerGeometry.m_Data)
		fprintf(f, "// Static buffer: Unmapped\n");

	fprintf(f, "cbuffer PerGeometry : register(b0)\n{\n");
	fprintf(f, "}\n\n");

	// Constant buffer 1 : register(b1)
	if (Shader->m_PerMaterial.m_Buffer)
	{
		D3D11_BUFFER_DESC desc;
		Shader->m_PerMaterial.m_Buffer->GetDesc(&desc);
		fprintf(f, "// Dynamic buffer: Size = %d\n", desc.ByteWidth);
	}

	if (Shader->m_PerMaterial.m_Data)
		fprintf(f, "// Static buffer: Unmapped\n");

	fprintf(f, "cbuffer PerMaterial : register(b1)\n{\n");
	fprintf(f, "}\n\n");

	// Constant buffer 2 : register(b2)
	if (Shader->m_PerTechnique.m_Buffer)
	{
		D3D11_BUFFER_DESC desc;
		Shader->m_PerTechnique.m_Buffer->GetDesc(&desc);
		fprintf(f, "// Dynamic buffer: Size = %d\n", desc.ByteWidth);
	}

	if (Shader->m_PerTechnique.m_Data)
		fprintf(f, "// Static buffer: Unmapped\n");

	fprintf(f, "cbuffer PerTechnique : register(b2)\n{\n");

	for (int i = 0; i < ARRAYSIZE(BSPixelShader::m_ConstantOffsets); i++)
	{
		uint8_t cbOffset = Shader->m_ConstantOffsets[i];

		//if (previous > cbOffset)
		//	break;

		const char *name = GetConstantFunc(i);

		if (strstr(name, "Add-your"))
			break;

		previous = cbOffset;

		fprintf(f, "\tfloat4 %s; \t\t// @ %d - 0x%04X Size: %d\n", name, cbOffset, cbOffset * 4, GetSizeFunc(i, 0));
	}

	fprintf(f, "}\n");
	fclose(f);
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

void DumpComputeShader(BSComputeShader *Shader)
{
}

void RemapShaderParams()
{
}