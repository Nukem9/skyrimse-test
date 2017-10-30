#define REMAP_VERTEX_UNUSED(ShaderType, GroupType)
#define REMAP_VERTEX(ShaderType, GroupType, ParameterIndex)

#define REMAP_PIXEL_UNUSED(ShaderType, GroupType)
#define REMAP_PIXEL(ShaderType, GroupType, ParameterIndex)

#define PER_GEO 0	// PerGeometry
#define PER_MAT 1	// PerMaterial
#define PER_TEC 2	// PerTechnique

// Vertex shader base: 80
// Pixel shader base: 64

//
// BSBloodSplatterShader
//
REMAP_VERTEX_UNUSED("BloodSplatter", PER_GEO);
REMAP_VERTEX_UNUSED("BloodSplatter", PER_MAT);
REMAP_VERTEX("BloodSplatter", PER_TEC, 0);
REMAP_VERTEX("BloodSplatter", PER_TEC, 1);
REMAP_VERTEX("BloodSplatter", PER_TEC, 2);

REMAP_PIXEL_UNUSED("BloodSplatter", PER_GEO);
REMAP_PIXEL_UNUSED("BloodSplatter", PER_MAT);
REMAP_PIXEL("BloodSplatter", PER_TEC, 0);

//
// BSDistantTreeShader
//
// WARNING/FIXME: In the geometry render pass function, they write to the vertex cbuffer
// with a non-existent pixel constant (7).
//
REMAP_VERTEX("DistantTree", PER_GEO, 4);
REMAP_VERTEX("DistantTree", PER_GEO, 5);
REMAP_VERTEX("DistantTree", PER_GEO, 6);
REMAP_VERTEX_UNUSED("DistantTree", PER_MAT);
REMAP_VERTEX("DistantTree", PER_TEC, 1);
REMAP_VERTEX("DistantTree", PER_TEC, 2);
REMAP_VERTEX("DistantTree", PER_TEC, 3);

REMAP_PIXEL("DistantTree", PER_GEO, 0);
REMAP_PIXEL("DistantTree", PER_GEO, 1);
REMAP_PIXEL("DistantTree", PER_GEO, 7);
REMAP_PIXEL_UNUSED("DistantTree", PER_MAT);
REMAP_PIXEL_UNUSED("DistantTree", PER_TEC);

//
// BSGrassShader
//
// NOTE: They do a special Map() in the technique pass, copying a static buffer all at once
// instead of using each parameter. Luckily, PerTechnique is the only constant buffer.
//
REMAP_VERTEX_UNUSED("RunGrass", PER_GEO);
REMAP_VERTEX_UNUSED("RunGrass", PER_MAT);
REMAP_VERTEX("RunGrass", PER_TEC, 0);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 1);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 2);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 3);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 4);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 5);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 6);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 7);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 8);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 9);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 10);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 11);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 12);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 13);		// Unverified
REMAP_VERTEX("RunGrass", PER_TEC, 14);		// Unverified

REMAP_PIXEL_UNUSED("RunGrass", PER_GEO);
REMAP_PIXEL_UNUSED("RunGrass", PER_MAT);
REMAP_PIXEL_UNUSED("RunGrass", PER_TEC);

//
// BSParticleShader
//
REMAP_VERTEX("Particle", PER_GEO, 13);
REMAP_VERTEX_UNUSED("Particle", PER_MAT);
REMAP_VERTEX("Particle", PER_TEC, 0);
REMAP_VERTEX("Particle", PER_TEC, 2);
REMAP_VERTEX("Particle", PER_TEC, 3);
REMAP_VERTEX("Particle", PER_TEC, 4);
REMAP_VERTEX("Particle", PER_TEC, 5);
REMAP_VERTEX("Particle", PER_TEC, 6);
REMAP_VERTEX("Particle", PER_TEC, 7);
REMAP_VERTEX("Particle", PER_TEC, 8);
REMAP_VERTEX("Particle", PER_TEC, 9);
REMAP_VERTEX("Particle", PER_TEC, 10);
REMAP_VERTEX("Particle", PER_TEC, 11);
REMAP_VERTEX("Particle", PER_TEC, 12);
REMAP_VERTEX("Particle", PER_TEC, 14);

REMAP_PIXEL_UNUSED("Particle", PER_GEO);
REMAP_PIXEL_UNUSED("Particle", PER_MAT);
REMAP_PIXEL("Particle", PER_TEC, 0);
REMAP_PIXEL("Particle", PER_TEC, 1);

//
// BSSkyShader
//
REMAP_VERTEX_UNUSED("Sky", PER_GEO);
REMAP_VERTEX_UNUSED("Sky", PER_MAT);
REMAP_VERTEX("Sky", PER_TEC, 0);
REMAP_VERTEX("Sky", PER_TEC, 1);
REMAP_VERTEX("Sky", PER_TEC, 2);
REMAP_VERTEX("Sky", PER_TEC, 3);
REMAP_VERTEX("Sky", PER_TEC, 4);
REMAP_VERTEX("Sky", PER_TEC, 5);
REMAP_VERTEX("Sky", PER_TEC, 6);

REMAP_PIXEL_UNUSED("Sky", PER_GEO);
REMAP_PIXEL_UNUSED("Sky", PER_MAT);
REMAP_PIXEL("Sky", PER_TEC, 0);

//
// BSEffectShader
//
REMAP_VERTEX("Effect", PER_GEO, 4);
REMAP_VERTEX("Effect", PER_GEO, 5);
REMAP_VERTEX("Effect", PER_GEO, 6);
REMAP_VERTEX("Effect", PER_MAT, 7);
REMAP_VERTEX("Effect", PER_MAT, 8);
REMAP_VERTEX("Effect", PER_MAT, 9);
REMAP_VERTEX("Effect", PER_TEC, 0);
REMAP_VERTEX("Effect", PER_TEC, 1);
REMAP_VERTEX("Effect", PER_TEC, 3);
REMAP_VERTEX("Effect", PER_TEC, 10);
REMAP_VERTEX("Effect", PER_TEC, 12);
REMAP_VERTEX("Effect", PER_TEC, 13);

REMAP_PIXEL("Effect", PER_GEO, 12);
REMAP_PIXEL("Effect", PER_GEO, 13);
REMAP_PIXEL("Effect", PER_GEO, 14);
REMAP_PIXEL("Effect", PER_MAT, 15);
REMAP_PIXEL("Effect", PER_MAT, 16);
REMAP_PIXEL("Effect", PER_MAT, 17);
REMAP_PIXEL("Effect", PER_TEC, 0);
REMAP_PIXEL("Effect", PER_TEC, 1);
REMAP_PIXEL("Effect", PER_TEC, 2);
REMAP_PIXEL("Effect", PER_TEC, 3);
REMAP_PIXEL("Effect", PER_TEC, 4);
REMAP_PIXEL("Effect", PER_TEC, 5);
REMAP_PIXEL("Effect", PER_TEC, 6);
REMAP_PIXEL("Effect", PER_TEC, 7);
REMAP_PIXEL("Effect", PER_TEC, 8);
REMAP_PIXEL("Effect", PER_TEC, 9);
REMAP_PIXEL("Effect", PER_TEC, 10);
REMAP_PIXEL("Effect", PER_TEC, 11);

//
// BSLightingShader
//
REMAP_VERTEX("Lighting", PER_GEO, 12);
REMAP_VERTEX("Lighting", PER_GEO, 13);
REMAP_VERTEX("Lighting", PER_GEO, 14);
REMAP_VERTEX("Lighting", PER_GEO, 15);// WARNING/FIXME: There's another parameter after Wind?...Name can't be found anywhere. Index 15.
REMAP_VERTEX("Lighting", PER_MAT, 9);
REMAP_VERTEX("Lighting", PER_MAT, 10);
REMAP_VERTEX("Lighting", PER_MAT, 11);
REMAP_VERTEX("Lighting", PER_TEC, 0);
REMAP_VERTEX("Lighting", PER_TEC, 2);
REMAP_VERTEX("Lighting", PER_TEC, 3);
REMAP_VERTEX("Lighting", PER_TEC, 4);
REMAP_VERTEX("Lighting", PER_TEC, 5);
REMAP_VERTEX("Lighting", PER_TEC, 6);
REMAP_VERTEX("Lighting", PER_TEC, 8);

REMAP_PIXEL("Lighting", PER_GEO, 11);// This is doing something weird....
REMAP_PIXEL("Lighting", PER_GEO, 19);
REMAP_PIXEL("Lighting", PER_GEO, 20);
REMAP_PIXEL("Lighting", PER_MAT, 6);
REMAP_PIXEL("Lighting", PER_MAT, 21);
REMAP_PIXEL("Lighting", PER_MAT, 22);
REMAP_PIXEL("Lighting", PER_MAT, 23);
REMAP_PIXEL("Lighting", PER_MAT, 24);
REMAP_PIXEL("Lighting", PER_MAT, 25);
REMAP_PIXEL("Lighting", PER_MAT, 26);
REMAP_PIXEL("Lighting", PER_MAT, 27);
REMAP_PIXEL("Lighting", PER_MAT, 28);
REMAP_PIXEL("Lighting", PER_MAT, 29);
REMAP_PIXEL("Lighting", PER_MAT, 30);
REMAP_PIXEL("Lighting", PER_MAT, 31);
REMAP_PIXEL("Lighting", PER_MAT, 32);
REMAP_PIXEL("Lighting", PER_MAT, 33);
REMAP_PIXEL("Lighting", PER_MAT, 34);
REMAP_PIXEL("Lighting", PER_TEC, 0);
REMAP_PIXEL("Lighting", PER_TEC, 1);
REMAP_PIXEL("Lighting", PER_TEC, 2);
REMAP_PIXEL("Lighting", PER_TEC, 3);
REMAP_PIXEL("Lighting", PER_TEC, 4);
REMAP_PIXEL("Lighting", PER_TEC, 5);
REMAP_PIXEL("Lighting", PER_TEC, 7);
REMAP_PIXEL("Lighting", PER_TEC, 8);
REMAP_PIXEL("Lighting", PER_TEC, 10);
REMAP_PIXEL("Lighting", PER_TEC, 12);
REMAP_PIXEL("Lighting", PER_TEC, 16);
REMAP_PIXEL("Lighting", PER_TEC, 17);

//
// BSUtilityShader
//
REMAP_VERTEX("Utility", PER_GEO, 3);
REMAP_VERTEX("Utility", PER_GEO, 4);
REMAP_VERTEX("Utility", PER_MAT, 1);
REMAP_VERTEX("Utility", PER_TEC, 0);
REMAP_VERTEX("Utility", PER_TEC, 2);
REMAP_VERTEX("Utility", PER_TEC, 5);
REMAP_VERTEX("Utility", PER_TEC, 6);
REMAP_VERTEX("Utility", PER_TEC, 7);

REMAP_PIXEL("Utility", PER_GEO, 7);
REMAP_PIXEL("Utility", PER_GEO, 10);
REMAP_PIXEL("Utility", PER_GEO, 11);
REMAP_PIXEL("Utility", PER_GEO, 12);
REMAP_PIXEL("Utility", PER_MAT, 1);
REMAP_PIXEL("Utility", PER_MAT, 3);
REMAP_PIXEL("Utility", PER_TEC, 0);
REMAP_PIXEL("Utility", PER_TEC, 2);
REMAP_PIXEL("Utility", PER_TEC, 4);
REMAP_PIXEL("Utility", PER_TEC, 5);
REMAP_PIXEL("Utility", PER_TEC, 6);
REMAP_PIXEL("Utility", PER_TEC, 8);

//
// BSWaterShader
//
REMAP_VERTEX("Water", PER_GEO, 3);
REMAP_VERTEX("Water", PER_MAT, 5);
REMAP_VERTEX("Water", PER_MAT, 6);
REMAP_VERTEX("Water", PER_MAT, 7);
REMAP_VERTEX("Water", PER_MAT, 8);
REMAP_VERTEX("Water", PER_MAT, 9);
REMAP_VERTEX("Water", PER_MAT, 10);
REMAP_VERTEX("Water", PER_TEC, 0);
REMAP_VERTEX("Water", PER_TEC, 1);
REMAP_VERTEX("Water", PER_TEC, 2);
REMAP_VERTEX("Water", PER_TEC, 4);
REMAP_VERTEX("Water", PER_TEC, 11);

REMAP_PIXEL("Water", PER_GEO, 6);
REMAP_PIXEL("Water", PER_GEO, 8);
REMAP_PIXEL("Water", PER_GEO, 14);
REMAP_PIXEL("Water", PER_GEO, 15);
REMAP_PIXEL("Water", PER_GEO, 24);
REMAP_PIXEL("Water", PER_MAT, 1);
REMAP_PIXEL("Water", PER_MAT, 2);
REMAP_PIXEL("Water", PER_MAT, 3);
REMAP_PIXEL("Water", PER_MAT, 4);
REMAP_PIXEL("Water", PER_MAT, 5);
REMAP_PIXEL("Water", PER_MAT, 10);
REMAP_PIXEL("Water", PER_MAT, 11);
REMAP_PIXEL("Water", PER_MAT, 12);
REMAP_PIXEL("Water", PER_MAT, 13);
REMAP_PIXEL("Water", PER_MAT, 19);
REMAP_PIXEL("Water", PER_MAT, 20);
REMAP_PIXEL("Water", PER_MAT, 21);
REMAP_PIXEL("Water", PER_MAT, 22);
REMAP_PIXEL("Water", PER_MAT, 23);
REMAP_PIXEL("Water", PER_TEC, 0);
REMAP_PIXEL("Water", PER_TEC, 7);
REMAP_PIXEL("Water", PER_TEC, 9);

#undef REMAP_VERTEX_UNUSED
#undef REMAP_VERTEX

#undef REMAP_PIXEL_UNUSED
#undef REMAP_PIXEL

#undef PER_GEO
#undef PER_MAT
#undef PER_TEC