//
//
// DistantTree
//
// Possible technique defines:
// - RENDER_DEPTH   Unique
// - DO_ALPHA_TEST
//
//
#include "ShaderCommon.h"

struct VS_INPUT
{
	float3 Position : POSITION0;
	float2 TexCoord0 : TEXCOORD0;// InstDataX
	float4 TexCoord4 : TEXCOORD4;// InstDataX
	float4 TexCoord5 : TEXCOORD5;// InstDataX
	float4 TexCoord6 : TEXCOORD6;// InstDataX
	float4 TexCoord7 : TEXCOORD7;// InstDataX
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION0;
	float3 TexCoord : TEXCOORD0;

#if defined(RENDER_DEPTH)
	float4 Depth : TEXCOORD3;
#else
	float4 WorldPosition : POSITION1;
	float4 PreviousWorldPosition : POSITION2;
#endif
};

typedef VS_OUTPUT PS_INPUT;

struct PS_OUTPUT
{
	float4 Albedo : SV_Target0;

#if !defined(RENDER_DEPTH)
	float2 MotionVector : SV_Target1;
	float4 Normal : SV_Target2;
#endif
};

//
// Vertex shader code
//
#ifdef VSHADER
cbuffer PerTechnique : register(b0)
{
	float4 FogParam;
	float4 FogNearColor;
	float4 FogFarColor;
	float4 DiffuseDir;
};

cbuffer PerGeometry : register(b2)
{
	float InstanceData;
	row_major float4x4 WorldViewProj;
	row_major float4x4 World;
	row_major float4x4 PreviousWorld;
	float IndexScale;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT vsout;

	float4 r0, r1;
	r0.xy = float2(1, -1) * input.TexCoord5.xy;
	r1.xyz = input.TexCoord4.www * input.Position.xyz;
	r0.x = dot(r0.xy, r1.xy);
	r0.y = dot(input.TexCoord5.yx, r1.xy);
	r0.z = r1.z;
	r0 = float4(input.TexCoord4.xyz + r0.xyz, 1.0);
	float4 worldProjection = mul(WorldViewProj, r0);

#ifdef RENDER_DEPTH
	vsout.Depth.xy = worldProjection.zw;
	vsout.Depth.zw = input.TexCoord5.zw;
#else
	vsout.WorldPosition = mul(World, r0);
	vsout.PreviousWorldPosition = mul(PreviousWorld, r0);
#endif

	vsout.Position = worldProjection;
	vsout.TexCoord = float3(input.TexCoord0.xy, FogParam.z);

	return vsout;
}
#endif

//
// Pixel shader code
//
#ifdef PSHADER
SamplerState SampDiffuse : register(s0);

Texture2D<float4> TexDiffuse : register(t0);

cbuffer PerTechnique : register(b0)
{
	float4 DiffuseColor;
	float4 AmbientColor;
};

cbuffer AlphaTestRefCB : register(b11)
{
	float AlphaTestRefRS;
}

cbuffer PerFrame : register(b12)
{
	float4 cb12[20];
}

const static float DepthOffsets[16] =
{
	0.003921568,
	0.533333361,
	0.133333340,
	0.666666687,
	0.800000000,
	0.266666681,
	0.933333337,
	0.400000000,
	0.200000000,
	0.733333349,
	0.066666670,
	0.600000000,
	0.996078432,
	0.466666669,
	0.866666675,
	0.333333343
};

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT psout;

	float4 r0, r1;

#if defined(RENDER_DEPTH)
	// Weird code to restrict the XY values between 0 and 15 (branchless)
	uint2 temp = uint2(input.Position.xy);
	uint index = ((temp.x << 2) & 12) | (temp.y & 3);

	float depthOffset = DepthOffsets[index] - 0.5;
	float depthModifier = (input.Depth.w * depthOffset) + input.Depth.z - 0.5;

	if (depthModifier < 0)
		discard;

	float alpha = TexDiffuse.Sample(SampDiffuse, input.TexCoord.xy).w;

	if ((alpha - AlphaTestRefRS) < 0)
		discard;

	// Depth
	psout.Albedo.xyz = input.Depth.xxx / input.Depth.yyy;
	psout.Albedo.w = 0;
#else
	float4 baseColor = TexDiffuse.Sample(SampDiffuse, input.TexCoord.xy);

	#if defined(DO_ALPHA_TEST)
	if ((baseColor.w - AlphaTestRefRS) < 0)
		discard;
	#endif

	// Albedo
	r1.xyz = input.TexCoord.zzz * DiffuseColor.xyz + AmbientColor.xyz;
	psout.Albedo = float4(r1.xyz * baseColor.xyz, 1.0);

	// Motion vectors
	r0.x = dot(cb12[12].xyzw, input.WorldPosition);
	r0.y = dot(cb12[13].xyzw, input.WorldPosition);
	r0.z = dot(cb12[15].xyzw, input.WorldPosition);
	r0.xy = r0.xy / r0.zz;
	r1.x = dot(cb12[16].xyzw, input.PreviousWorldPosition);
	r1.y = dot(cb12[17].xyzw, input.PreviousWorldPosition);
	r0.z = dot(cb12[19].xyzw, input.PreviousWorldPosition);
	r0.zw = r1.xy / r0.zz;
	r0.xy = r0.xy + -r0.zw;
	psout.MotionVector = float2(-0.5, 0.5) * r0.xy;

	// Normals (tangent space, constant no normals)
	psout.Normal = float4(0.5, 0.5, 0, 0);
#endif

	return psout;
}
#endif