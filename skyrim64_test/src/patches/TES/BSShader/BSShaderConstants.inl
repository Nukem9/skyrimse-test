// clang-format off

#ifndef REMAP_VERTEX_UNUSED
#define REMAP_VERTEX_UNUSED(ShaderType, GroupType)
#endif
#ifndef REMAP_VERTEX
#define REMAP_VERTEX(ShaderType, GroupType, ParameterIndex, ParamType)
#endif

#ifndef REMAP_PIXEL_UNUSED
#define REMAP_PIXEL_UNUSED(ShaderType, GroupType)
#endif
#ifndef REMAP_PIXEL
#define REMAP_PIXEL(ShaderType, GroupType, ParameterIndex, ParamType)
#endif

#define PER_GEO 0	// PerGeometry
#define PER_MAT 1	// PerMaterial
#define PER_TEC 2	// PerTechnique

//
// BSParticleShader
//
REMAP_VERTEX("Particle", PER_GEO, 13, 0)
REMAP_VERTEX_UNUSED("Particle", PER_MAT)
REMAP_VERTEX("Particle", PER_TEC, 0, "float4x4")
REMAP_VERTEX("Particle", PER_TEC, 1, "float4x4")	// Guessed based on cbuffer offset. "Offset: 0x0040 PrevWorldViewProj"
REMAP_VERTEX("Particle", PER_TEC, 2, "float4x4")
REMAP_VERTEX("Particle", PER_TEC, 3, 0)
REMAP_VERTEX("Particle", PER_TEC, 4, 0)
REMAP_VERTEX("Particle", PER_TEC, 5, 0)
REMAP_VERTEX("Particle", PER_TEC, 6, 0)
REMAP_VERTEX("Particle", PER_TEC, 7, 0)
REMAP_VERTEX("Particle", PER_TEC, 8, 0)
REMAP_VERTEX("Particle", PER_TEC, 9, 0)
REMAP_VERTEX("Particle", PER_TEC, 10, 0)
REMAP_VERTEX("Particle", PER_TEC, 11, 0)
REMAP_VERTEX("Particle", PER_TEC, 12, 0)
REMAP_VERTEX("Particle", PER_TEC, 14, 0)

REMAP_PIXEL_UNUSED("Particle", PER_GEO)
REMAP_PIXEL_UNUSED("Particle", PER_MAT)
REMAP_PIXEL("Particle", PER_TEC, 0, 0)
REMAP_PIXEL("Particle", PER_TEC, 1, 0)

//
// BSEffectShader
//
REMAP_VERTEX("Effect", PER_GEO, 4, "float4")
REMAP_VERTEX("Effect", PER_GEO, 5, "float4")
REMAP_VERTEX("Effect", PER_GEO, 6, "float4")
REMAP_VERTEX("Effect", PER_MAT, 7, "float4")
REMAP_VERTEX("Effect", PER_MAT, 8, 0)
REMAP_VERTEX("Effect", PER_MAT, 9, "float4")
REMAP_VERTEX("Effect", PER_TEC, 0, "float3x4")		// WARNING: The game code always writes a float4x4 here. Shader has a float3x4...
REMAP_VERTEX("Effect", PER_TEC, 1, "float3x4")		// WARNING: The game code always writes a float4x4 here. Shader has a float3x4...
REMAP_VERTEX("Effect", PER_TEC, 3, 0)
REMAP_VERTEX("Effect", PER_TEC, 10, "float4")
REMAP_VERTEX("Effect", PER_TEC, 12, 0)
REMAP_VERTEX("Effect", PER_TEC, 13, "float4x4")		// WARNING: The game code always writes a float4x4 here. Unknown shader type.

REMAP_PIXEL("Effect", PER_GEO, 12, "float2")
REMAP_PIXEL("Effect", PER_GEO, 13, "float4")
REMAP_PIXEL("Effect", PER_GEO, 14, "float2")
REMAP_PIXEL("Effect", PER_MAT, 15, "float4")
REMAP_PIXEL("Effect", PER_MAT, 16, 0)
REMAP_PIXEL("Effect", PER_MAT, 17, 0)
REMAP_PIXEL("Effect", PER_TEC, 0, 0)
REMAP_PIXEL("Effect", PER_TEC, 1, 0)
REMAP_PIXEL("Effect", PER_TEC, 2, "float4")
REMAP_PIXEL("Effect", PER_TEC, 3, 0)
REMAP_PIXEL("Effect", PER_TEC, 4, 0)
REMAP_PIXEL("Effect", PER_TEC, 5, 0)
REMAP_PIXEL("Effect", PER_TEC, 6, 0)
REMAP_PIXEL("Effect", PER_TEC, 7, 0)
REMAP_PIXEL("Effect", PER_TEC, 8, 0)
REMAP_PIXEL("Effect", PER_TEC, 9, 0)
REMAP_PIXEL("Effect", PER_TEC, 10, 0)
REMAP_PIXEL("Effect", PER_TEC, 11, 0)

//
// BSUtilityShader
//
REMAP_VERTEX("Utility", PER_GEO, 3, 0)
REMAP_VERTEX("Utility", PER_GEO, 4, 0)
REMAP_VERTEX("Utility", PER_MAT, 1, 0)
REMAP_VERTEX("Utility", PER_TEC, 0, "float4x4")
REMAP_VERTEX("Utility", PER_TEC, 2, 0)
REMAP_VERTEX("Utility", PER_TEC, 5, 0)
REMAP_VERTEX("Utility", PER_TEC, 6, 0)
REMAP_VERTEX("Utility", PER_TEC, 7, 0)

REMAP_PIXEL("Utility", PER_GEO, 7, 0)
REMAP_PIXEL("Utility", PER_GEO, 10, 0)
REMAP_PIXEL("Utility", PER_GEO, 11, 0)
REMAP_PIXEL("Utility", PER_GEO, 12, 0)
REMAP_PIXEL("Utility", PER_GEO, 13, 0)				// Guessed based on cbuffer offset. "Offset: 0x0040 FocusShadowFadeParam"
REMAP_PIXEL("Utility", PER_MAT, 1, 0)
REMAP_PIXEL("Utility", PER_MAT, 3, 0)
REMAP_PIXEL("Utility", PER_TEC, 0, 0)
REMAP_PIXEL("Utility", PER_TEC, 2, 0)
REMAP_PIXEL("Utility", PER_TEC, 4, 0)
REMAP_PIXEL("Utility", PER_TEC, 5, "float3x4[4]")
REMAP_PIXEL("Utility", PER_TEC, 6, "float3x4[3]")
REMAP_PIXEL("Utility", PER_TEC, 8, 0)

//
// BSWaterShader
//
REMAP_VERTEX("Water", PER_GEO, 3, 0)
REMAP_VERTEX("Water", PER_MAT, 5, 0)
REMAP_VERTEX("Water", PER_MAT, 6, 0)
REMAP_VERTEX("Water", PER_MAT, 7, 0)
REMAP_VERTEX("Water", PER_MAT, 8, 0)
REMAP_VERTEX("Water", PER_MAT, 9, 0)
REMAP_VERTEX("Water", PER_MAT, 10, 0)
REMAP_VERTEX("Water", PER_TEC, 0, "float4x4")
REMAP_VERTEX("Water", PER_TEC, 1, "float4x4")
REMAP_VERTEX("Water", PER_TEC, 2, "float4x4")
REMAP_VERTEX("Water", PER_TEC, 4, "float3")
REMAP_VERTEX("Water", PER_TEC, 11, "float4")

REMAP_PIXEL("Water", PER_GEO, 6, 0)
REMAP_PIXEL("Water", PER_GEO, 8, 0)
REMAP_PIXEL("Water", PER_GEO, 14, 0)
REMAP_PIXEL("Water", PER_GEO, 15, 0)
REMAP_PIXEL("Water", PER_GEO, 24, 0)
REMAP_PIXEL("Water", PER_MAT, 1, 0)
REMAP_PIXEL("Water", PER_MAT, 2, 0)
REMAP_PIXEL("Water", PER_MAT, 3, 0)
REMAP_PIXEL("Water", PER_MAT, 4, 0)
REMAP_PIXEL("Water", PER_MAT, 5, 0)
REMAP_PIXEL("Water", PER_MAT, 10, 0)
REMAP_PIXEL("Water", PER_MAT, 11, 0)
REMAP_PIXEL("Water", PER_MAT, 12, 0)
REMAP_PIXEL("Water", PER_MAT, 13, 0)
REMAP_PIXEL("Water", PER_MAT, 19, 0)
REMAP_PIXEL("Water", PER_MAT, 20, 0)
REMAP_PIXEL("Water", PER_MAT, 21, 0)
REMAP_PIXEL("Water", PER_MAT, 22, 0)
REMAP_PIXEL("Water", PER_MAT, 23, 0)
REMAP_PIXEL("Water", PER_TEC, 0, "float4x4")
REMAP_PIXEL("Water", PER_TEC, 7, "float4")
REMAP_PIXEL("Water", PER_TEC, 9, "float2")
REMAP_PIXEL("Water", PER_TEC, 16, "float")
REMAP_PIXEL("Water", PER_TEC, 17, "float4[8]")
REMAP_PIXEL("Water", PER_TEC, 18, "float4[8]")

#undef REMAP_VERTEX_UNUSED
#undef REMAP_VERTEX

#undef REMAP_PIXEL_UNUSED
#undef REMAP_PIXEL

#undef PER_GEO
#undef PER_MAT
#undef PER_TEC

// clang-format on