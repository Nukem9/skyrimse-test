//
//
// RunGrass
//
// Possible technique defines:
// - VERTLIT
// - SLOPE
// - BILLBOARD
// - RENDER_DEPTH
// - DO_ALPHA_TEST
//
//
#include "ShaderCommon.h"

struct VS_INPUT
{
	float4 Position : POSITION0;
	float2 TexCoord : TEXCOORD0;
	float4 Normal : NORMAL0;
	float4 Color : COLOR0;
	float4 InstanceData1 : TEXCOORD4;
	float4 InstanceData2 : TEXCOORD5;
	float4 InstanceData3 : TEXCOORD6;
	float4 InstanceData4 : TEXCOORD7;
};

struct VS_OUTPUT
{
	float4 HPosition : SV_POSITION0;
	float4 DiffuseColor : COLOR0;
	float3 TexCoord : TEXCOORD0;
	float4 AmbientColor : TEXCOORD1;
	float3 ViewSpacePosition : TEXCOORD2;
#if defined(RENDER_DEPTH)
	float2 Depth : TEXCOORD3;
#endif
	float4 CurrentPosition : POSITION1;
	float4 PreviousPosition : POSITION2;
};

typedef VS_OUTPUT PS_INPUT;

struct PS_OUTPUT
{
#if defined(RENDER_DEPTH)
	float4 PS : SV_Target0;
#else
	float4 Albedo : SV_Target0;
	float2 MotionVectors : SV_Target1;
	float4 Normal : SV_Target2;
#endif
};

//
// Vertex shader code
//
#ifdef VSHADER
cbuffer PerGeometry : register(b2)
{
	row_major float4x4 WorldViewProj             : packoffset(c0);      // @ 0 - 0x0000
	row_major float4x4 WorldView                 : packoffset(c4);      // @ 16 - 0x0040
	row_major float4x4 World                     : packoffset(c8);      // @ 32 - 0x0080
	row_major float4x4 PreviousWorld             : packoffset(c12);     // @ 48 - 0x00C0
	float4 FogNearColor                          : packoffset(c16);     // @ 64 - 0x0100
	float3 WindVector                            : packoffset(c17);     // @ 68 - 0x0110
	float WindTimer                              : packoffset(c17.w);   // @ 71 - 0x011C
	float3 DirLightDirection                     : packoffset(c18);     // @ 72 - 0x0120
	float PreviousWindTimer                      : packoffset(c18.w);   // @ 75 - 0x012C
	float3 DirLightColor                         : packoffset(c19);     // @ 76 - 0x0130
	float AlphaParam1                            : packoffset(c19.w);   // @ 79 - 0x013C
	float3 AmbientColor                          : packoffset(c20);     // @ 80 - 0x0140
	float AlphaParam2                            : packoffset(c20.w);   // @ 83 - 0x014C
	float3 ScaleMask                             : packoffset(c21);     // @ 84 - 0x0150
	float ShadowClampValue                       : packoffset(c21.w);   // @ 0 - 0x0000
}

cbuffer cb8 : register(b8)
{
  float4 cb8[240];
}

cbuffer cb7 : register(b7)
{
  float4 cb7[1];
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT vsout;

	float4 r0,r1,r2,r3,r4,r5;
	r0.x = input.InstanceData1.x + input.InstanceData1.y;
	r0.y = -r0.x * 0.0078125 + WindTimer;
	r0.x = -r0.x * 0.0078125 + PreviousWindTimer;
	r0.xy = float2(0.4, 0.4) * r0.xy;
	sincos(r0.x, r0.x, r1.x);
	sincos(r0.y, r2.x, r3.x);
	r0.yz = float2(M_PI, M_2PI) * r2.xx;
	r0.w = cos(M_PI * r3.x);
	r0.w = 0.2 * r0.w;
	r0.yz = sin(r0.yz);
	r0.y = r0.y + r0.z;
	r0.y = r0.y * 0.3 + r0.w;
	r0.z = input.Color.w * input.Color.w;
	r0.z = 0.5 * r0.z;
	r0.y = r0.y * r0.z;
	r0.y = WindVector.z * r0.y;
	r2.x = input.InstanceData4.x;
	r2.y = input.InstanceData2.w;
	r2.z = input.InstanceData3.w;
	r1.yzw = input.InstanceData4.yyy * ScaleMask.xyz + float3(1,1,1);
	r1.yzw = input.Position.xyz * r1.yzw;
	r2.z = dot(r2.xyz, r1.yzw);
	r2.x = dot(input.InstanceData2.xyz, r1.yzw);
	r2.y = dot(input.InstanceData3.xyz, r1.yzw);
	r3.xy = WindVector.xy;
	r3.z = 0;
	r1.yzw = r3.xyz * r0.yyy + r2.xyz;
	r4.xyz = input.InstanceData1.xyz + r1.yzw;
	r4.w = 1;

	float4 projSpacePosition = mul(WorldViewProj, r4);
	vsout.HPosition = projSpacePosition;

#if defined(RENDER_DEPTH)
	vsout.Depth = projSpacePosition.zw;
#endif

	r5.x = input.InstanceData2.z;
	r5.yz = input.InstanceData3.zw;
	r0.w = saturate(dot(DirLightDirection.xyz, r5.xyz));
	r1.yzw = input.InstanceData1.www * input.Color.xyz;
	r1.yzw = r1.yzw * r0.www;

	r0.w = dot(cb8[(asuint(cb7[0].x) >> 2)].xyzw, M_IdentityMatrix[(asint(cb7[0].x) & 3)].xyzw);
	r0.y = 1 - saturate((length(projSpacePosition.xyz) - AlphaParam1) / AlphaParam2);

	vsout.DiffuseColor.xyz = DirLightColor.xyz * r1.yzw;
	vsout.DiffuseColor.w = r0.y * r0.w;// DistanceFade * PerInstanceFade?

	vsout.TexCoord.xy = input.TexCoord.xy;
	vsout.TexCoord.z = FogNearColor.w;

	r1.yzw = AmbientColor.xyz * input.Color.xyz;
	vsout.AmbientColor.xyz = input.InstanceData1.www * r1.yzw;
	vsout.AmbientColor.w = ShadowClampValue;

	vsout.ViewSpacePosition = mul(WorldView, r4).xyz;
	vsout.CurrentPosition = mul(World, r4);

	r0.w = cos(M_PI * r1.x);
	r0.xyw = float3(M_PI, M_2PI, 0.2) * r0.xxw;
	r0.xy = sin(r0.xy);
	r0.x = r0.x + r0.y;
	r0.x = r0.x * 0.3 + r0.w;
	r0.x = r0.x * r0.z;
	r0.x = WindVector.z * r0.x;
	r0.xyz = r3.xyz * r0.xxx + r2.xyz;
	r0.xyz = input.InstanceData1.xyz + r0.xyz;
	r0.w = 1;
	vsout.PreviousPosition = mul(PreviousWorld, r0);

	return vsout;
}
#endif

//
// Pixel shader code
//
#ifdef PSHADER
SamplerState SampBaseSampler : register(s0);
SamplerState SampShadowMaskSampler : register(s1);

Texture2D<float4> TexBaseSampler : register(t0);
Texture2D<float4> TexShadowMaskSampler : register(t1);

cbuffer AlphaTestRefCB : register(b11)
{
	float AlphaTestRefRS;
}

cbuffer PerFrame : register(b12)
{
	float4 cb12[20];
}

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT psout;

	float4 r0;
	float4 r1;
	float4 r2;

	float4 baseColor = TexBaseSampler.Sample(SampBaseSampler, input.TexCoord.xy);

#if defined(RENDER_DEPTH) || defined(DO_ALPHA_TEST)
	float diffuseAlpha = input.DiffuseColor.w * baseColor.w;

	if ((diffuseAlpha - AlphaTestRefRS) < 0)
		discard;
#endif

#if defined(RENDER_DEPTH)
	// Depth
	psout.PS.xyz = input.Depth.xxx / input.Depth.yyy;
	psout.PS.w = diffuseAlpha;
#else
	float sunShadowMask = TexShadowMaskSampler.Load(int3(input.HPosition.xy, 0)).x;

	// Albedo
	r0.x = lerp(sunShadowMask, 1, input.AmbientColor.w);
	r0.yzw = baseColor.xyz;
	r1.xyz = input.DiffuseColor.xyz * r0.yzw;
	r0.yzw = input.AmbientColor.xyz * r0.yzw;
	r0.xyz = r1.xyz * r0.xxx + r0.yzw;
	psout.Albedo.xyz = input.TexCoord.zzz * r0.xyz;
	psout.Albedo.w = 1;

	// Motion vectors
	r0.x = dot(cb12[12].xyzw, input.CurrentPosition);
	r0.y = dot(cb12[13].xyzw, input.CurrentPosition);
	r0.z = dot(cb12[15].xyzw, input.CurrentPosition);
	r0.xy = r0.xy / r0.zz;
	r1.x = dot(cb12[16].xyzw, input.PreviousPosition);
	r1.y = dot(cb12[17].xyzw, input.PreviousPosition);
	r0.z = dot(cb12[19].xyzw, input.PreviousPosition);
	r0.zw = r1.xy / r0.zz;
	r0.xy = r0.xy - r0.zw;
	psout.MotionVectors = float2(-0.5,0.5) * r0.xy;
	
	// Generated normals (tangent space)
	r0.xyz = ddx_coarse(input.ViewSpacePosition.zxy);
	r1.xyz = ddy_coarse(input.ViewSpacePosition.yzx);
	r2.xyz = r1.xyz * r0.xyz;
	r0.xyz = normalize(r0.zxy * r1.yzx - r2.xyz);// This is actually normalize(cross(r0.yzx, r1.zxy));
	r0.z = r0.z * -8 + 8;
	r0.z = max(1.0 / 1000.0, sqrt(r0.z));
	r0.xy = r0.xy / r0.zz;
	psout.Normal.xy = float2(0.5,0.5) + r0.xy;
	psout.Normal.zw = float2(0,0);
#endif

	return psout;
}
#endif