//
//
// Sky
//
// Possible technique defines:
// - OCCLUSION      Unique
// - MOONMASK       Unique
// - HORIZFADE      Unique
// - TEXLERP        Unique
// - TEXFADE        Unique
// - TEX
// - DITHER
// - CLOUDS
//
//
#include "ShaderCommon.h"

struct VS_INPUT
{
	float4 Position : POSITION0;

#if defined(TEX) || defined(HORIZFADE)
	float2 TexCoord : TEXCOORD0;
#endif

	float4 Color : COLOR0;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION0;

#if defined(DITHER) && defined(TEX)
	float4 TexCoord0 : TEXCOORD0;
#elif defined(DITHER)
	float2 TexCoord0 : TEXCOORD3;// !
#elif defined(TEX) || defined(HORIZFADE)
	float2 TexCoord0 : TEXCOORD0;
#endif

#if defined(TEXLERP)
	float2 TexCoord1 : TEXCOORD1;
#endif

#if defined(HORIZFADE)
	float TexCoord2 : TEXCOORD2;
#endif

#if defined(TEX) || defined(DITHER) || defined(HORIZFADE)
	float4 Color : COLOR0;
#endif

	float4 WorldPosition : POSITION1;
	float4 PreviousWorldPosition : POSITION2;
};

typedef VS_OUTPUT PS_INPUT;

struct PS_OUTPUT
{
	float4 Color : SV_Target0;
	float2 MotionVectors : SV_Target1;
	float4 Normal : SV_Target2;
};

//
// Vertex shader code
//
#ifdef VSHADER
cbuffer PerGeometry : register(b2)
{
	row_major float4x4 WorldViewProj;
	row_major float4x4 World;
	row_major float4x4 PreviousWorld;
	float3 EyePosition;
	float VParams;
	float4 BlendColor[3];
	float2 TexCoordOff;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT vsout;

	float4 r0 = float4(input.Position.xyz, 1.0);
	float3 r1;

#if defined(OCCLUSION)

	// Intentionally left blank

#elif defined(MOONMASK)

	vsout.TexCoord0 = input.TexCoord;
	vsout.Color = float4(VParams.xxx, 1.0);

#elif defined(HORIZFADE)

	float worldHeight = mul(World, r0).z;
	float eyeHeightDelta = -EyePosition.z + worldHeight;

	vsout.TexCoord0.xy = input.TexCoord;
	vsout.TexCoord2.x = saturate((1.0 / 17.0) * eyeHeightDelta);
	vsout.Color.xyz = BlendColor[0].xyz * VParams;
	vsout.Color.w = BlendColor[0].w;

#else // MOONMASK HORIZFADE

#if defined(DITHER)

	#if defined(TEX)
	vsout.TexCoord0.xyzw = input.TexCoord.xyxy * float4(1.0, 1.0, 501.0, 501.0);
	#else
	r1 = normalize(input.Position.xyz);
	r1.y = r1.y + r1.z;

	vsout.TexCoord0.x = 501 * acos(r1.x);
	vsout.TexCoord0.y = 501 * asin(r1.y);
	#endif // TEX

#elif defined(CLOUDS)
	vsout.TexCoord0.xy = TexCoordOff + input.TexCoord;
#else
	vsout.TexCoord0.xy = input.TexCoord;
#endif // DITHER CLOUDS

#ifdef TEXLERP
	vsout.TexCoord1.xy = TexCoordOff + input.TexCoord;
#endif // TEXLERP

	r1 = BlendColor[1].xyz * input.Color.yyy;
	r1 = BlendColor[0].xyz * input.Color.xxx + r1;
	r1 = BlendColor[2].xyz * input.Color.zzz + r1;

	vsout.Color.xyz = VParams * r1;
	vsout.Color.w = BlendColor[0].w * input.Color.w;

#endif // OCCLUSION MOONMASK HORIZFADE

	vsout.Position = mul(WorldViewProj, r0).xyww;
	vsout.WorldPosition = mul(World, r0);
	vsout.PreviousWorldPosition = mul(PreviousWorld, r0);

	return vsout;
}
#endif

//
// Pixel shader code
//
#ifdef PSHADER
SamplerState SampBaseSampler : register(s0);
SamplerState SampBlendSampler : register(s1);
SamplerState SampNoiseGradSampler : register(s2);

Texture2D<float4> TexBaseSampler : register(t0);
Texture2D<float4> TexBlendSampler : register(t1);
Texture2D<float4> TexNoiseGradSampler : register(t2);

cbuffer PerGeometry : register(b2)
{
	float2 PParams;
};

cbuffer AlphaTestRefCB : register(b11)
{
	float AlphaTestRefRS;
}

cbuffer cb12 : register(b12)
{
	float4 cb12[20];
}

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT psout;

	float4 r0,r1;

#ifndef OCCLUSION
#ifndef TEXLERP
	r0 = TexBaseSampler.Sample(SampBaseSampler, input.TexCoord0.xy);
#ifdef TEXFADE
	r0.w = r0.w * PParams.x;
#endif
#else
	r0 = TexBlendSampler.Sample(SampBlendSampler, input.TexCoord1.xy);
	r1 = TexBaseSampler.Sample(SampBaseSampler, input.TexCoord0.xy);
	r0.xyzw = -r1.xyzw + r0.xyzw;
	r0.xyzw = PParams.xxxx * r0.xyzw + r1.xyzw;
#endif

#if defined(DITHER)
	r0.xy = float2(0.125,0.125) * input.Position.xy;
	r0.x = TexNoiseGradSampler.Sample(SampNoiseGradSampler, r0.xy).x;
	r0.x = r0.x * 0.03125 + -0.0078125;

#ifdef TEX
	float4 baseTex = TexBaseSampler.Sample(SampBaseSampler, input.TexCoord0.xy);

	r0.yzw = input.Color.xyz * baseTex.xyz + PParams.yyy;
	psout.Color.xyz = r0.yzw + r0.xxx;
	psout.Color.w = baseTex.w * input.Color.w;
#else
	r0.yzw = PParams.yyy + input.Color.xyz;
	psout.Color.xyz = r0.yzw + r0.xxx;
	psout.Color.w = input.Color.w;
#endif // TEX
#elif defined(MOONMASK)
	r1.x = r0.w - AlphaTestRefRS.x;
	psout.Color.xyzw = r0;

	if (r1.x < 0)
		discard;

#elif defined(HORIZFADE)
	r0.w = input.Color.w * r0.w;
	r0.xyz = input.Color.xyz * r0.xyz + PParams.y;
	psout.Color.xyz = float3(1.5, 1.5, 1.5) * r0.xyz;
	psout.Color.w = input.TexCoord2.x * r0.w;
#else
	psout.Color.w = input.Color.w * r0.w;
	psout.Color.xyz = input.Color.xyz * r0.xyz + PParams.yyy;
#endif

#else
	psout.Color = float4(0, 0, 0, 1.0);
#endif // OCCLUSION

	r0.x = dot(cb12[12].xyzw, input.WorldPosition);
	r0.y = dot(cb12[13].xyzw, input.WorldPosition);
	r0.z = dot(cb12[15].xyzw, input.WorldPosition);
	r0.xy = r0.xy / r0.zz;
	r1.x = dot(cb12[16].xyzw, input.PreviousWorldPosition);
	r1.y = dot(cb12[17].xyzw, input.PreviousWorldPosition);
	r0.z = dot(cb12[19].xyzw, input.PreviousWorldPosition);
	r0.zw = r1.xy / r0.zz;
	r0.xy = r0.xy + -r0.zw;
	psout.MotionVectors = float2(-0.5, 0.5) * r0.xy;
	psout.Normal = float4(0.5, 0.5, 0, 0);

	return psout;
}
#endif