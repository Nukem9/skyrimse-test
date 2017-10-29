#pragma once

#define CONSTANT_BUFFER_PER_GEOMETRY  0
#define CONSTANT_BUFFER_PER_MATERIAL  1
#define CONSTANT_BUFFER_PER_TECHNIQUE 2

#define SHADER_TYPE_VERTEX 0
#define SHADER_TYPE_PIXEL 1
#define SHADER_TYPE_COMPUTE 2

#pragma pack(push, 8)
struct BSConstantBufferInfo
{
	ID3D11Buffer *m_Buffer;	// Selected from pool in Load*ShaderFromFile()
	void *m_Data;			// m_ConstantBufferData = DeviceContext->Map(m_ConstantBuffer)

	// Based on shader load flags, these **CAN BE NULL**. At least one of the
	// pointers is guaranteed to be non-null.
};

struct BSComputeBufferInfo
{
	ID3D11Buffer *m_Buffer;	// Always a nullptr
	void *m_Data;			// Static buffer inside the exe .data section. Can be null.
	uint32_t m_UnknownIndex;// Compute shaders do something special but I don't remember
							// off the top of my head. Optional buffer index?
};

struct BSVertexShader
{
	uint32_t m_Unknown;					// Probably technique index
	ID3D11VertexShader *m_Shader;
	uint32_t m_ShaderLength;			// Raw bytecode length
	BSConstantBufferInfo m_PerGeometry;
	BSConstantBufferInfo m_PerMaterial;
	BSConstantBufferInfo m_PerTechnique;
	uint64_t m_ShaderInputFlags;		// Flags for VSMain() inputs
	uint8_t m_ConstantOffsets[20];		// Actual offset is multiplied by 4
	// Raw bytecode appended after this
};

static_assert(offsetof(BSVertexShader, m_Shader) == 0x8, "");
static_assert(offsetof(BSVertexShader, m_ShaderLength) == 0x10, "");
static_assert(offsetof(BSVertexShader, m_PerGeometry) == 0x18, "");
static_assert(offsetof(BSVertexShader, m_PerMaterial) == 0x28, "");
static_assert(offsetof(BSVertexShader, m_PerTechnique) == 0x38, "");
static_assert(offsetof(BSVertexShader, m_ShaderInputFlags) == 0x48, "");
static_assert(offsetof(BSVertexShader, m_ConstantOffsets) == 0x50, "");
static_assert(sizeof(BSVertexShader) == 0x68, "");

struct BSPixelShader
{
	uint32_t m_Unknown;					// Probably technique index
	ID3D11PixelShader *m_Shader;
	BSConstantBufferInfo m_PerGeometry;
	BSConstantBufferInfo m_PerMaterial;
	BSConstantBufferInfo m_PerTechnique;
	uint8_t m_ConstantOffsets[64];		// Actual offset is multiplied by 4
	// Bytecode is *not* appended
};
static_assert(offsetof(BSPixelShader, m_Shader) == 0x8, "");
static_assert(offsetof(BSPixelShader, m_PerGeometry) == 0x10, "");
static_assert(offsetof(BSPixelShader, m_PerMaterial) == 0x20, "");
static_assert(offsetof(BSPixelShader, m_PerTechnique) == 0x30, "");
static_assert(offsetof(BSPixelShader, m_ConstantOffsets) == 0x40, "");
static_assert(sizeof(BSPixelShader) == 0x80, "");

struct BSComputeShader
{
	char _pad0[0x8];
	BSComputeBufferInfo CBuffer1;
	char _pad1[0x8];
	BSComputeBufferInfo CBuffer2;
	char _pad2[0x8];
	BSComputeBufferInfo CBuffer3;
	ID3D11ComputeShader *m_Shader;
	uint32_t m_Unknown;					// Probably technique dword
	uint32_t m_ShaderLength;			// Raw bytecode length
	uint8_t m_ConstantOffsets[32];		// Actual offset is multiplied by 4
	// Raw bytecode appended after this
};
static_assert(offsetof(BSComputeShader, CBuffer1) == 0x8, "");
static_assert(offsetof(BSComputeShader, CBuffer2) == 0x28, "");
static_assert(offsetof(BSComputeShader, CBuffer3) == 0x48, "");
static_assert(offsetof(BSComputeShader, m_Shader) == 0x60, "");
static_assert(offsetof(BSComputeShader, m_Unknown) == 0x68, "");
static_assert(offsetof(BSComputeShader, m_ShaderLength) == 0x6C, "");
static_assert(offsetof(BSComputeShader, m_ConstantOffsets) == 0x70, "");
static_assert(sizeof(BSComputeShader) == 0x90, "");
#pragma pack(pop)

//
// *** SHADER CONSTANTS ***
//
// Everything ripped directly from CreationKit.exe.
//
// GetString() returns the name used in the HLSL source code.
// GetSize() is multiplied by sizeof(float) to get the real CBuffer size.
//
// NOTE: In the creation kit, there is a copy-paste error with the placeholder
// saying "BSLightingShaderX" instead of the real function name.
//

#define BSSM_PLACEHOLDER "Add-your-constant-to-" __FUNCTION__

// BSBloodSplatterShader
// BSDistantTreeShader
// BSGrassShader
// BSParticleShader
// BSSkyShader
// BSEffectShader
// BSLightingShader
// BSUtilityShader
// BSWaterShader

#pragma region BloodSplatterShader
namespace BSBloodSplatterShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "WorldViewProj";
		case 1:return "LightLoc";
		case 2:return "Ctrl";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
			return 4;
		case 1:
		case 2:// This one needs to be checked
			return 1;
		}

		return 0;
	}
}

namespace BSBloodSplatterShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "Alpha";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		return Index == 0;
	}
}
#pragma endregion

#pragma region DistantTreeShader
namespace BSDistantTreeShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "InstanceData";
		case 1:return "WorldViewProj";
		case 2:return "World";
		case 3:return "PreviousWorld";
		case 4:return "FogParam";
		case 5:return "FogNearColor";
		case 6:return "FogFarColor";
		case 7:return "DiffuseDir";
		case 8:return "IndexScale";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
			return 150;
		case 1:
		case 2:
		case 3:
			return 4;
		case 4:
		case 5:
		case 6:
		case 7:
			return 1;
		}

		return 0;
	}
}

namespace BSDistantTreeShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "DiffuseColor";
		case 1:return "AmbientColor";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		return !Index || Index == 1;
	}
}
#pragma endregion

#pragma region GrassShader
namespace BSGrassShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "WorldViewProj";
		case 1:return "WorldView";
		case 2:return "World";
		case 3:return "PreviousWorld";
		case 4:return "FogNearColor";
		case 5:return "WindVector";
		case 6:return "WindTimer";
		case 7:return "DirLightDirection";
		case 8:return "PreviousWindTimer";
		case 9:return "DirLightColor";
		case 10:return "AlphaParam1";
		case 11:return "AmbientColor";
		case 12:return "AlphaParam2";
		case 13:return "ScaleMask";
		case 14:return "padding";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			return 4;
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
			return 1;
		}

		return 0;
	}
}

namespace BSGrassShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		return 0;
	}
}
#pragma endregion

#pragma region ParticleShader
namespace BSParticleShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "WorldViewProj";
		case 1:return "PrevWorldViewProj";
		case 2:return "PrecipitationOcclusionWorldViewProj";
		case 3:return "fVars0";
		case 4:return "fVars1";
		case 5:return "fVars2";
		case 6:return "fVars3";
		case 7:return "fVars4";
		case 8:return "Color1";
		case 9:return "Color2";
		case 10:return "Color3";
		case 11:return "Velocity";
		case 12:return "Acceleration";
		case 13:return "ScaleAdjust";
		case 14:return "Wind";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
		case 1:
		case 2:
			return 4;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
			return 1;
		}

		return 0;
	}
}

namespace BSParticleShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "ColorScale";
		case 1:return "TextureSize";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		return !Index || Index == 1;
	}
}
#pragma endregion

#pragma region SkyShader
namespace BSSkyShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "WorldViewProj";
		case 1:return "World";
		case 2:return "PreviousWorld";
		case 3:return "BlendColor";
		case 4:return "EyePosition";
		case 5:return "TexCoordOff";
		case 6:return "VParams";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
		case 1:
		case 2:
			return 4;
		case 3:
			return 3;
		case 4:
		case 5:
		case 6:
			return 1;
		}

		return 0;
	}
}

namespace BSSkyShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "PParams";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		return Index == 0;
	}
}
#pragma endregion

#pragma region EffectShader
namespace BSXShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "World";
		case 1:return "PreviousWorld";
		case 2:return "Bones";
		case 3:return "EyePosition";
		case 4:return "FogParam";
		case 5:return "FogNearColor";
		case 6:return "FogFarColor";
		case 7:return "FalloffData";
		case 8:return "SoftMaterialVSParams";
		case 9:return "TexcoordOffset";
		case 10:return "TexcoordOffsetMembrane";
		case 11:return "SubTexOffset";
		case 12:return "PosAdjust";
		case 13:return "MatProj";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
		case 1:
		case 13:
			return 3;
		case 3:
		case 12:
			return 1;
		}

		return 0;
	}
}

namespace BSXShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "PropertyColor";
		case 1:return "AlphaTestRef";
		case 2:return "MembraneRimColor";
		case 3:return "MembraneVars";
		case 4:return "PLightPositionX";
		case 5:return "PLightPositionY";
		case 6:return "PLightPositionZ";
		case 7:return "PLightingRadiusInverseSquared";
		case 8:return "PLightColorR";
		case 9:return "PLightColorG";
		case 10:return "PLightColorB";
		case 11:return "DLightColor";
		case 12:return "VPOSOffset";
		case 13:return "CameraData";
		case 14:return "FilteringParam";
		case 15:return "BaseColor";
		case 16:return "BaseColorScale";
		case 17:return "LightingInfluence";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
			return 1;
		}

		return 0;
	}
}
#pragma endregion

#pragma region LightingShader
namespace BSLightingShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "WorldViewProj";
		case 1:return "PrevWorldViewProj";
		case 2:return "PrecipitationOcclusionWorldViewProj";
		case 3:return "fVars0";
		case 4:return "fVars1";
		case 5:return "fVars2";
		case 6:return "fVars3";
		case 7:return "fVars4";
		case 8:return "Color1";
		case 9:return "Color2";
		case 10:return "Color3";
		case 11:return "Velocity";
		case 12:return "Acceleration";
		case 13:return "ScaleAdjust";
		case 14:return "Wind";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
		case 1:
		case 6:
			return 3;
		case 2:
		case 3:
		case 4:
		case 5:
			return 1;
		}

		return 0;
	}
}

namespace BSLightingShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "NumLightNumShadowLight";
		case 1:return "PointLightPosition";
		case 2:return "PointLightColor";
		case 3:return "DirLightDirection";
		case 4:return "DirLightColor";
		case 5:return "DirectionalAmbient";
		case 6:return "AmbientSpecularTintAndFresnelPower";
		case 7:return "MaterialData";
		case 8:return "EmitColor";
		case 9:return "AlphaTestRef";
		case 10:return "ShadowLightMaskSelect";
		case 11:return "VPOSOffset";
		case 12:return "ProjectedUVParams";
		case 13:return "ProjectedUVParams2";
		case 14:return "ProjectedUVParams3";
		case 15:return "SplitDistance";
		case 16:return "SSRParams";
		case 17:return "WorldMapOverlayParametersPS";
		case 18:return "AmbientColor";
		case 19:return "FogColor";
		case 20:return "ColourOutputClamp";
		case 21:return "EnvmapData";
		case 22:return "ParallaxOccData";
		case 23:return "TintColor";
		case 24:return "LODTexParams";
		case 25:return "SpecularColor";
		case 26:return "SparkleParams";
		case 27:return "MultiLayerParallaxData";
		case 28:return "LightingEffectParams";
		case 29:return "IBLParams";
		case 30:return "LandscapeTexture1to4IsSnow";
		case 31:return "LandscapeTexture5to6IsSnow";
		case 32:return "LandscapeTexture1to4IsSpecPower";
		case 33:return "LandscapeTexture5to6IsSpecPower";
		case 34:return "SnowRimLightParameters";
		case 35:return "CharacterLightParams";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		int v2 = 1;

		if ((unsigned int)(Index - 1) <= 1)
			v2 = (Flags >> 3) & 7;

		switch (Index)
		{
		case 5:
			return 3;
		case 6:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
			return 0;
		}

		return v2;
	}
}
#pragma endregion

#pragma region UtilityShader
namespace BSUtilityShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "World";
		case 1:return "TexcoordOffset";
		case 2:return "EyePos";
		case 3:return "HighDetailRange";
		case 4:return "ParabolaParam";
		case 5:return "ShadowFadeParam";
		case 6:return "TreeParams";
		case 7:return "WaterParams";
		case 8:return "Bones";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
			return 3;
		case 2:
		case 5:
		case 6:
		case 7:
			return 1;
		}

		return 0;
	}
}

namespace BSUtilityShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "AlphaTestRef";
		case 1:return "RefractionPower";
		case 2:return "DebugColor";
		case 3:return "BaseColor";
		case 4:return "PropertyColor";
		case 5:return "FocusShadowMapProj";
		case 6:return "ShadowMapProj";
		case 7:return "ShadowSampleParam";
		case 8:return "ShadowLightParam";
		case 9:return "ShadowFadeParam";
		case 10:return "VPOSOffset";
		case 11:return "EndSplitDistances";
		case 12:return "StartSplitDistances";
		case 13:return "FocusShadowFadeParam";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		if (Index == 6 && Flags & (1u << 21))
			return 9;

		switch (Index)
		{
		case 0:
		case 2:
		case 3:
		case 4:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			return 1;
		case 5:
			return 12;
		case 6:
			return 4;
		}

		return 0;
	}
}
#pragma endregion

#pragma region WaterShader
namespace BSWaterShaderVertexConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "WorldViewProj";
		case 1:return "World";
		case 2:return "PreviousWorld";
		case 3:return "QPosAdjust";
		case 4:return "ObjectUV";
		case 5:return "NormalsScroll0";
		case 6:return "NormalsScroll1";
		case 7:return "NormalsScale";
		case 8:return "VSFogParam";
		case 9:return "VSFogNearColor";
		case 10:return "VSFogFarColor";
		case 11:return "CellTexCoordOffset";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
		case 1:
		case 2:
			return 4;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			return 1;
		}

		return 0;
	}
}

namespace BSWaterShaderPixelConstants
{
	static const char *GetString(int Index)
	{
		switch (Index)
		{
		case 0:return "TextureProj";
		case 1:return "ShallowColor";
		case 2:return "DeepColor";
		case 3:return "ReflectionColor";
		case 4:return "FresnelRI";
		case 5:return "BlendRadius";
		case 6:return "PosAdjust";
		case 7:return "ReflectPlane";
		case 8:return "CameraData";
		case 9:return "ProjData";
		case 10:return "VarAmounts";
		case 11:return "FogParam";
		case 12:return "FogNearColor";
		case 13:return "FogFarColor";
		case 14:return "SunDir";
		case 15:return "SunColor";
		case 16:return "NumLights";
		case 17:return "LightPos";
		case 18:return "LightColor";
		case 19:return "WaterParams";
		case 20:return "DepthControl";
		case 21:return "SSRParams";
		case 22:return "SSRParams2";
		case 23:return "NormalsAmplitude";
		case 24:return "VPOSOffset";
		}

		return BSSM_PLACEHOLDER;
	}

	static int GetSize(int Index, unsigned int Flags = 0)
	{
		switch (Index)
		{
		case 0:
			return 4;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
			return 1;
		case 17:
		case 18:
			return 8;
		}

		return 0;
	}
}
#pragma endregion