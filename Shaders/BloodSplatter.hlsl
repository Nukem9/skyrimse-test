//
//
// BloodSplatter
//
// Possible technique defines:
// - SPLATTER       Unique
// - FLARE          Unique
//
//
#include "ShaderCommon.h"

struct VS_INPUT
{
	float4 Position : POSITION0;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION0;
	float3 TexCoordColor : TEXCOORD0;

#if defined(SPLATTER)
	float2 TexCoordAlpha : TEXCOORD1;
#endif
};

typedef VS_OUTPUT PS_INPUT;

struct PS_OUTPUT
{
	float4 Color : SV_Target0;
};

//
// Vertex shader code
//
#ifdef VSHADER
cbuffer PerGeometry : register(b2)
{
	row_major float4x4 WorldViewProj;
	float4 LightLoc;
	float Ctrl;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT vsout;

	float4 projected = mul(WorldViewProj, float4(input.Position.xy, 0.0, 1.0));
	float2 r1 = (-LightLoc.xy + projected.xy) * LightLoc.ww;

#if defined(SPLATTER)
	vsout.TexCoordAlpha = -(r1.xy * float2(-0.5, 0.5)) + input.TexCoord;
#elif defined(FLARE)
	projected.xy += r1 * Ctrl.xx;
#endif

	vsout.Position = projected;
	vsout.TexCoordColor = float3(input.TexCoord, input.Position.z);

	return vsout;
}
#endif

//
// Pixel shader code
//
#ifdef PSHADER
SamplerState SampBloodColor : register(s0);
SamplerState SampBloodAlpha : register(s1);
SamplerState SampFlareColor : register(s2);
SamplerState SampFlareHDR : register(s3);

Texture2D<float4> TexBloodColor : register(t0);
Texture2D<float4> TexBloodAlpha : register(t1);
Texture2D<float4> TexFlareColor : register(t2);
Texture2D<float4> TexFlareHDR : register(t3);

cbuffer PerGeometry : register(b2)
{
	float Alpha;
};

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT psout;

#if defined(SPLATTER)
	float4 r0,r1;
	r0.xyz = TexBloodAlpha.Sample(SampBloodAlpha, input.TexCoordAlpha.xy).xyz;
	r1.xyzw = TexBloodColor.Sample(SampBloodColor, input.TexCoordColor.xy).xyzw;
	r1.xyz = float3(-1,-1,-1) + r1.xyz;

	r0.w = Alpha * input.TexCoordColor.z;
	r0.w = r1.w * r0.w;
	r1.xyz = r0.www * r1.xyz + float3(1,1,1);
	r0.xyz = -r1.xyz + r0.xyz;

	psout.Color = float4(r0.www * r0.xyz + r1.xyz, 1.0);
#elif defined(FLARE)
	float flareMult = TexFlareColor.Sample(SampFlareColor, input.TexCoordColor.xy).x;
	float4 colorHDR = TexFlareHDR.Sample(SampFlareHDR, input.TexCoordColor.xy).xyzw;

	psout.Color = (colorHDR * flareMult) * Alpha;
#endif

	return psout;
}
#endif