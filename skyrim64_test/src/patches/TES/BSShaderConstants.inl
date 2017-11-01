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
// BSBloodSplatterShader
//
REMAP_VERTEX_UNUSED("BloodSplatter", PER_GEO)
REMAP_VERTEX_UNUSED("BloodSplatter", PER_MAT)
REMAP_VERTEX("BloodSplatter", PER_TEC, 0, "float4x4")
REMAP_VERTEX("BloodSplatter", PER_TEC, 1, "float4")
REMAP_VERTEX("BloodSplatter", PER_TEC, 2, "float")

REMAP_PIXEL_UNUSED("BloodSplatter", PER_GEO)
REMAP_PIXEL_UNUSED("BloodSplatter", PER_MAT)
REMAP_PIXEL("BloodSplatter", PER_TEC, 0, "float")

//
// BSDistantTreeShader
//
// WARNING/FIXME: In the geometry render pass function, they write to the vertex cbuffer
// with a non-existent pixel constant (7).
//
REMAP_VERTEX("DistantTree", PER_GEO, 4, "float4")
REMAP_VERTEX("DistantTree", PER_GEO, 5, "float4")
REMAP_VERTEX("DistantTree", PER_GEO, 6, "float4")
REMAP_VERTEX("DistantTree", PER_GEO, 7, 0)// Guessed based on cbuffer offset. "Offset: 0x0000 DiffuseDir"
REMAP_VERTEX_UNUSED("DistantTree", PER_MAT)
REMAP_VERTEX("DistantTree", PER_TEC, 1, "float4x4")
REMAP_VERTEX("DistantTree", PER_TEC, 2, "float4x4")
REMAP_VERTEX("DistantTree", PER_TEC, 3, "float4x4")
REMAP_VERTEX("DistantTree", PER_TEC, 8, 0)// Guessed based on cbuffer offset. "Offset: 0x00C0 IndexScale"

REMAP_PIXEL("DistantTree", PER_GEO, 0, "float4")
REMAP_PIXEL("DistantTree", PER_GEO, 1, "float4")
REMAP_PIXEL("DistantTree", PER_GEO, 7, "float4")
REMAP_PIXEL_UNUSED("DistantTree", PER_MAT)
REMAP_PIXEL_UNUSED("DistantTree", PER_TEC)

//
// BSGrassShader
//
// NOTE: They do a special Map() in the technique pass, copying a static buffer all at once
// instead of using each parameter. Luckily, PerTechnique is the only constant buffer.
//
REMAP_VERTEX_UNUSED("RunGrass", PER_GEO)
REMAP_VERTEX_UNUSED("RunGrass", PER_MAT)
REMAP_VERTEX("RunGrass", PER_TEC, 0, "float4x4")	// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 1, "float4x4")	// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 2, "float4x4")	// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 3, "float4x4")	// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 4, "float4")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 5, "float3")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 6, "float")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 7, "float3")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 8, "float")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 9, "float3")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 10, "float")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 11, "float3")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 12, "float")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 13, "float3")		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 14, 0)			// Unverified

REMAP_PIXEL_UNUSED("RunGrass", PER_GEO)
REMAP_PIXEL_UNUSED("RunGrass", PER_MAT)
REMAP_PIXEL_UNUSED("RunGrass", PER_TEC)

//
// BSParticleShader
//
REMAP_VERTEX("Particle", PER_GEO, 13, 0)
REMAP_VERTEX_UNUSED("Particle", PER_MAT)
REMAP_VERTEX("Particle", PER_TEC, 0, "float4x4")
REMAP_VERTEX("Particle", PER_TEC, 1, "float4x4")// Guessed based on cbuffer offset. "Offset: 0x0040 PrevWorldViewProj"
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
// BSSkyShader
//
REMAP_VERTEX_UNUSED("Sky", PER_GEO)
REMAP_VERTEX_UNUSED("Sky", PER_MAT)
REMAP_VERTEX("Sky", PER_TEC, 0, "float4x4")
REMAP_VERTEX("Sky", PER_TEC, 1, "float4x4")
REMAP_VERTEX("Sky", PER_TEC, 2, "float4x4")
REMAP_VERTEX("Sky", PER_TEC, 3, "float4[3]")
REMAP_VERTEX("Sky", PER_TEC, 4, "float3")
REMAP_VERTEX("Sky", PER_TEC, 5, "float2")// At least float2
REMAP_VERTEX("Sky", PER_TEC, 6, "float")

REMAP_PIXEL_UNUSED("Sky", PER_GEO)
REMAP_PIXEL_UNUSED("Sky", PER_MAT)
REMAP_PIXEL("Sky", PER_TEC, 0, "float2")// At least float2

//
// BSEffectShader
//
REMAP_VERTEX("Effect", PER_GEO, 4, 0)
REMAP_VERTEX("Effect", PER_GEO, 5, 0)
REMAP_VERTEX("Effect", PER_GEO, 6, 0)
REMAP_VERTEX("Effect", PER_MAT, 7, 0)
REMAP_VERTEX("Effect", PER_MAT, 8, 0)
REMAP_VERTEX("Effect", PER_MAT, 9, 0)
REMAP_VERTEX("Effect", PER_TEC, 0, 0)// float3x4 // TODO/WARNING: The game code actually writes a float4x4 here if certain flags are set...
REMAP_VERTEX("Effect", PER_TEC, 1, 0)// float3x4 // TODO/WARNING: The game code actually writes a float4x4 here if certain flags are set...
REMAP_VERTEX("Effect", PER_TEC, 3, 0)// float3
REMAP_VERTEX("Effect", PER_TEC, 10, 0)// float4
REMAP_VERTEX("Effect", PER_TEC, 12, 0)// float2
REMAP_VERTEX("Effect", PER_TEC, 13, 0)// float4x4

REMAP_PIXEL("Effect", PER_GEO, 12, 0)
REMAP_PIXEL("Effect", PER_GEO, 13, 0)
REMAP_PIXEL("Effect", PER_GEO, 14, 0)
REMAP_PIXEL("Effect", PER_MAT, 15, 0)
REMAP_PIXEL("Effect", PER_MAT, 16, 0)
REMAP_PIXEL("Effect", PER_MAT, 17, 0)
REMAP_PIXEL("Effect", PER_TEC, 0, 0)
REMAP_PIXEL("Effect", PER_TEC, 1, 0)
REMAP_PIXEL("Effect", PER_TEC, 2, 0)
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
// BSLightingShader
//
REMAP_VERTEX("Lighting", PER_GEO, 12, 0)
REMAP_VERTEX("Lighting", PER_GEO, 13, 0)
REMAP_VERTEX("Lighting", PER_GEO, 14, 0)
REMAP_VERTEX("Lighting", PER_GEO, 15, 0)// WARNING/FIXME: There's another parameter after Wind?...Name can't be found anywhere. Index 15.
REMAP_VERTEX("Lighting", PER_MAT, 9, 0)
REMAP_VERTEX("Lighting", PER_MAT, 10, 0)
REMAP_VERTEX("Lighting", PER_MAT, 11, 0)
REMAP_VERTEX("Lighting", PER_TEC, 0, 0)
REMAP_VERTEX("Lighting", PER_TEC, 1, 0)// Guessed based on cbuffer offset. "Offset: 0x0030 PrevWorldViewProj"
REMAP_VERTEX("Lighting", PER_TEC, 2, 0)
REMAP_VERTEX("Lighting", PER_TEC, 3, 0)
REMAP_VERTEX("Lighting", PER_TEC, 4, 0)
REMAP_VERTEX("Lighting", PER_TEC, 5, 0)
REMAP_VERTEX("Lighting", PER_TEC, 6, 0)
REMAP_VERTEX("Lighting", PER_TEC, 7, 0)// Guessed based on cbuffer offset. "Offset: 0x00D0 fVars4"
REMAP_VERTEX("Lighting", PER_TEC, 8, 0)

REMAP_PIXEL("Lighting", PER_GEO, 11, 0)// This is doing something weird....
REMAP_PIXEL("Lighting", PER_GEO, 19, 0)
REMAP_PIXEL("Lighting", PER_GEO, 20, 0)
REMAP_PIXEL("Lighting", PER_MAT, 6, 0)
REMAP_PIXEL("Lighting", PER_MAT, 21, 0)
REMAP_PIXEL("Lighting", PER_MAT, 22, 0)
REMAP_PIXEL("Lighting", PER_MAT, 23, 0)
REMAP_PIXEL("Lighting", PER_MAT, 24, 0)
REMAP_PIXEL("Lighting", PER_MAT, 25, 0)
REMAP_PIXEL("Lighting", PER_MAT, 26, 0)
REMAP_PIXEL("Lighting", PER_MAT, 27, 0)
REMAP_PIXEL("Lighting", PER_MAT, 28, 0)
REMAP_PIXEL("Lighting", PER_MAT, 29, 0)
REMAP_PIXEL("Lighting", PER_MAT, 30, 0)
REMAP_PIXEL("Lighting", PER_MAT, 31, 0)
REMAP_PIXEL("Lighting", PER_MAT, 32, 0)
REMAP_PIXEL("Lighting", PER_MAT, 33, 0)
REMAP_PIXEL("Lighting", PER_MAT, 34, 0)
REMAP_PIXEL("Lighting", PER_TEC, 0, 0)
REMAP_PIXEL("Lighting", PER_TEC, 1, 0)
REMAP_PIXEL("Lighting", PER_TEC, 2, 0)
REMAP_PIXEL("Lighting", PER_TEC, 3, 0)
REMAP_PIXEL("Lighting", PER_TEC, 4, 0)
REMAP_PIXEL("Lighting", PER_TEC, 5, 0)// float3x4
REMAP_PIXEL("Lighting", PER_TEC, 7, 0)
REMAP_PIXEL("Lighting", PER_TEC, 8, 0)
REMAP_PIXEL("Lighting", PER_TEC, 9, 0)// float       // Guessed based on cbuffer offset. "Offset: 0x0040 AlphaTestRef"
REMAP_PIXEL("Lighting", PER_TEC, 10, 0)
REMAP_PIXEL("Lighting", PER_TEC, 12, 0)
REMAP_PIXEL("Lighting", PER_TEC, 13, 0)// Guessed based on cbuffer offset. "Offset: 0x0090 ProjectedUVParams2"
REMAP_PIXEL("Lighting", PER_TEC, 14, 0)// Guessed based on cbuffer offset. "Offset: 0x00A0 ProjectedUVParams3"
REMAP_PIXEL("Lighting", PER_TEC, 16, 0)
REMAP_PIXEL("Lighting", PER_TEC, 17, 0)
REMAP_PIXEL("Lighting", PER_TEC, 35, 0)// Guessed based on cbuffer offset. "Offset: 0x00E0 CharacterLightParams"

//
// BSUtilityShader
//
REMAP_VERTEX("Utility", PER_GEO, 3, 0)
REMAP_VERTEX("Utility", PER_GEO, 4, 0)
REMAP_VERTEX("Utility", PER_MAT, 1, 0)
REMAP_VERTEX("Utility", PER_TEC, 0, 0)// float4x4
REMAP_VERTEX("Utility", PER_TEC, 2, 0)
REMAP_VERTEX("Utility", PER_TEC, 5, 0)
REMAP_VERTEX("Utility", PER_TEC, 6, 0)
REMAP_VERTEX("Utility", PER_TEC, 7, 0)

REMAP_PIXEL("Utility", PER_GEO, 7, 0)
REMAP_PIXEL("Utility", PER_GEO, 10, 0)
REMAP_PIXEL("Utility", PER_GEO, 11, 0)
REMAP_PIXEL("Utility", PER_GEO, 12, 0)
REMAP_PIXEL("Utility", PER_MAT, 1, 0)
REMAP_PIXEL("Utility", PER_MAT, 3, 0)
REMAP_PIXEL("Utility", PER_TEC, 0, 0)
REMAP_PIXEL("Utility", PER_TEC, 2, 0)
REMAP_PIXEL("Utility", PER_TEC, 4, 0)
REMAP_PIXEL("Utility", PER_TEC, 5, 0)
REMAP_PIXEL("Utility", PER_TEC, 6, 0)
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