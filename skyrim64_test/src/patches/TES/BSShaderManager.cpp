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
	auto remapParam = [](const char *, const char *, const char *)
	{
	};

	// BSBloodSplatterShader
	// PerGeometry is unused

	// PerMaterial is unused

	remapParam("BloodSplatter", "VS", "PerTechnique", "WorldViewProj");
	remapParam("BloodSplatter", "VS", "PerTechnique", "LightLoc");
	remapParam("BloodSplatter", "VS", "PerTechnique", "Ctrl");

	// BSDistantTreeShader
	// WARNING/FIXME: In the geometry render pass function, they write to the vertex cbuffer with a pixel constant
	remapParam("DistantTree", "VS", "PerGeometry", "FogParam");
	remapParam("DistantTree", "VS", "PerGeometry", "FogNearColor");
	remapParam("DistantTree", "VS", "PerGeometry", "FogFarColor");

	// PerMaterial is unused

	remapParam("DistantTree", "VS", "PerTechnique", "WorldViewProj");
	remapParam("DistantTree", "VS", "PerTechnique", "World");
	remapParam("DistantTree", "VS", "PerTechnique", "PreviousWorld");

	// remapParam("DistantTree", "InstanceData", "???");
	// remapParam("DistantTree", "DiffuseDir", "???");
	// remapParam("DistantTree", "IndexScale", "???");

	// BSGrassShader
	// PerGeometry is unused

	// PerMaterial is unused

	remapParam("RunGrass", "VS", "PerTechnique", "WorldViewProj");
	remapParam("RunGrass", "VS", "PerTechnique", "WorldView");
	remapParam("RunGrass", "VS", "PerTechnique", "World");
	remapParam("RunGrass", "VS", "PerTechnique", "PreviousWorld");
	remapParam("RunGrass", "VS", "PerTechnique", "FogNearColor");
	remapParam("RunGrass", "VS", "PerTechnique", "WindVector");
	remapParam("RunGrass", "VS", "PerTechnique", "WindTimer");
	remapParam("RunGrass", "VS", "PerTechnique", "DirLightDirection");
	remapParam("RunGrass", "VS", "PerTechnique", "PreviousWindTimer");
	remapParam("RunGrass", "VS", "PerTechnique", "DirLightColor");
	remapParam("RunGrass", "VS", "PerTechnique", "AlphaParam1");
	remapParam("RunGrass", "VS", "PerTechnique", "AmbientColor");
	remapParam("RunGrass", "VS", "PerTechnique", "AlphaParam2");
	remapParam("RunGrass", "VS", "PerTechnique", "ScaleMask");
	remapParam("RunGrass", "VS", "PerTechnique", "padding");// Guessed. Probably never used on purpose.

	// BSParticleShader (Vertex)
	remapParam("Particle", "VS", "PerGeometry", "ScaleAdjust");

	// PerMaterial is unused

	remapParam("Particle", "VS", "PerTechnique", "WorldViewProj");
	remapParam("Particle", "VS", "PerTechnique", "PrevWorldViewProj");// Guessed. Based on offset in cbuffer.
	remapParam("Particle", "VS", "PerTechnique", "PrecipitationOcclusionWorldViewProj");
	remapParam("Particle", "VS", "PerTechnique", "fVars0");
	remapParam("Particle", "VS", "PerTechnique", "fVars1");
	remapParam("Particle", "VS", "PerTechnique", "fVars2");
	remapParam("Particle", "VS", "PerTechnique", "fVars3");
	remapParam("Particle", "VS", "PerTechnique", "fVars4");
	remapParam("Particle", "VS", "PerTechnique", "Color1");
	remapParam("Particle", "VS", "PerTechnique", "Color2");
	remapParam("Particle", "VS", "PerTechnique", "Color3");
	remapParam("Particle", "VS", "PerTechnique", "Velocity");
	remapParam("Particle", "VS", "PerTechnique", "Acceleration");

	// BSSkyShader (Vertex)
	// PerGeometry is unused

	// PerMaterial is unused

	remapParam("Sky", "VS", "PerTechnique", "WorldViewProj");
	remapParam("Sky", "VS", "PerTechnique", "World");
	remapParam("Sky", "VS", "PerTechnique", "PreviousWorld");
	remapParam("Sky", "VS", "PerTechnique", "BlendColor");
	remapParam("Sky", "VS", "PerTechnique", "EyePosition");
	remapParam("Sky", "VS", "PerTechnique", "TexCoordOff");
	remapParam("Sky", "VS", "PerTechnique", "VParams");

	// BSEffectShader (Vertex)
	remapParam("Effect", "VS", "PerGeometry", "FogParam");
	remapParam("Effect", "VS", "PerGeometry", "FogNearColor");
	remapParam("Effect", "VS", "PerGeometry", "FogFarColor");

	remapParam("Effect", "VS", "PerMaterial", "FalloffData");
	remapParam("Effect", "VS", "PerMaterial", "SoftMaterialVSParams");
	remapParam("Effect", "VS", "PerMaterial", "TexcoordOffset");

	remapParam("Effect", "VS", "PerTechnique", "World");
	remapParam("Effect", "VS", "PerTechnique", "PreviousWorld");
	remapParam("Effect", "VS", "PerTechnique", "EyePosition");
	remapParam("Effect", "VS", "PerTechnique", "TexcoordOffsetMembrane");
	remapParam("Effect", "VS", "PerTechnique", "PosAdjust");
	remapParam("Effect", "VS", "PerTechnique", "MatProj");

	// remapParam("Effect", "Bones", "???");
	// remapParam("Effect", "SubTexOffset", "???");

	// BSLightingShader (Vertex)
	remapParam("Lighting", "VS", "PerGeometry", "Acceleration");
	remapParam("Lighting", "VS", "PerGeometry", "ScaleAdjust");
	remapParam("Lighting", "VS", "PerGeometry", "Wind");
	// WARNING/FIXME: There's another parameter after Wind?...Name can't be found anywhere. Index 15.

	remapParam("Lighting", "VS", "PerMaterial", "Color2");
	remapParam("Lighting", "VS", "PerMaterial", "Color3");
	remapParam("Lighting", "VS", "PerMaterial", "Velocity");

	remapParam("Lighting", "VS", "PerTechnique", "WorldViewProj");
	remapParam("Lighting", "VS", "PerTechnique", "PrevWorldViewProj");// Guessed. Based on offset in cbuffer.
	remapParam("Lighting", "VS", "PerTechnique", "PrecipitationOcclusionWorldViewProj");
	remapParam("Lighting", "VS", "PerTechnique", "fVars0");// Guessed. Based on offset in cbuffer.
	remapParam("Lighting", "VS", "PerTechnique", "fVars1");
	remapParam("Lighting", "VS", "PerTechnique", "fVars2");
	remapParam("Lighting", "VS", "PerTechnique", "fVars3");
	remapParam("Lighting", "VS", "PerTechnique", "fVars4");// Guessed. Based on offset in cbuffer.
	remapParam("Lighting", "VS", "PerTechnique", "Color1");

	// BSUtilityShader (Vertex)
	remapParam("Utility", "VS", "PerGeometry", "HighDetailRange");
	remapParam("Utility", "VS", "PerGeometry", "ParabolaParam");

	remapParam("Utility", "VS", "PerMaterial", "TexcoordOffset");

	remapParam("Utility", "VS", "PerTechnique", "World");
	remapParam("Utility", "VS", "PerTechnique", "EyePos");
	remapParam("Utility", "VS", "PerTechnique", "ShadowFadeParam");
	remapParam("Utility", "VS", "PerTechnique", "TreeParams");
	remapParam("Utility", "VS", "PerTechnique", "WaterParams");

	// remapParam("Effect", "Bones", "???");

	// BSWaterShader (Vertex)
	remapParam("Water", "VS", "PerGeometry", "QPosAdjust");

	remapParam("Water", "VS", "PerMaterial", "NormalsScroll0");
	remapParam("Water", "VS", "PerMaterial", "NormalsScroll1");
	remapParam("Water", "VS", "PerMaterial", "NormalsScale");
	remapParam("Water", "VS", "PerMaterial", "VSFogParam");
	remapParam("Water", "VS", "PerMaterial", "VSFogNearColor");
	remapParam("Water", "VS", "PerMaterial", "VSFogFarColor");

	remapParam("Water", "VS", "PerTechnique", "WorldViewProj");
	remapParam("Water", "VS", "PerTechnique", "World");
	remapParam("Water", "VS", "PerTechnique", "PreviousWorld");
	remapParam("Water", "VS", "PerTechnique", "CellTexCoordOffset");
}