string ParamID = "0x000";

#define OutputPosition POSITION
#define OutputColor COLOR

// Vertex Attributes
struct a2v 
{
	float4 Position : POSITION;
#if defined(TEX) || defined(HORIZFADE)
	float3 TexCoord : TEXCOORD0;
#endif
	float4 Color	: COLOR;
};

float4x4 World      : 		WORLD;
float4x4 View       : 		VIEW;
float4x4 ViewI		:		VIEWI;
float4x4 Projection : 		PROJECTION;
float4x4 WorldViewProj : 	WORLDVIEWPROJ;
float4x4 WorldView : 		WORLDVIEW;

texture baseTexture : DiffuseMap< string UIName = "Base Texture"; >;
sampler2D BaseSampler = sampler_state { Texture = <baseTexture>;};
float3 BaseColor <string UIName = "BaseColor";> = float3( 1.0f, 1.0f, 1.0f);
int Type <string UIName = "Type";> = 0;

// Vertex constants
float4 BlendColor[3];
float3 EyePosition;
float  TexCoordYOff;

// Pixel constants
float3  Params;

//performs vec*mat where mat is a column major matrix so that the matrix multiply can be done as a series of dot products.
float4 Transform(float4 vec, float4x4 mat)
{
	return mul(vec, mat);
}

// Pixel Attributes
struct v2fSky
{
	float4 HPosition	: OutputPosition;

#if defined(TEX) && defined(DITHER)
	float4 TexCoord0	: TEXCOORD0;

#else //!(TEX && DITHER)

#if defined(TEX) || defined(HORIZFADE)
	float2 TexCoord0	: TEXCOORD0;
#endif

#if defined(DITHER)
	float2 NoiseCoord	: TEXCOORD3;
#endif

#endif //TEX && DITHER

#if defined(TEXLERP)
	float2 TexCoord1	: TEXCOORD1;
#endif

#if defined(HORIZFADE)
    float WorldZPos     : TEXCOORD2;
#endif
	
#if !defined(OCCLUSION)
	float4 Color	    : COLOR;
#endif
};

// Make the sky draw behind everything else
#ifndef DEPTH_VALUE
#define DEPTH_VALUE 1.0
#endif

#if defined(HORIZFADE)
#undef HDR
#endif

#if defined(DEPTH_VALUE)
#define DepthValue true
#else
#define DepthValue false
#endif

#if defined(OCCLUSION)
#define Occlusion true
#else
#define Occlusion false
#endif

#if defined(HORIZFADE)
#define HorizonFade true
#else
#define HorizonFade false
#endif

#if defined(TEX)
#define Texture true
#else
#define Texture false
#endif

#if defined(CLOUDS)
#define Clouds true
#else
#define Clouds false
#endif

#if defined(MOONMASK)
#define MoonMask true
#else
#define MoonMask false
#endif

struct PS_OUTPUT
{
	float4	Color		: OutputColor;
#if defined(OCCLUSION)
	float	Depth		: OutputDepth;
#endif //OCCLUSION
};

void InitializeVertexOut( inout v2fSky OUT )
{
	OUT.HPosition = 0.0f;
#if defined(TEX) || defined(HORIZFADE)
	OUT.TexCoord0 = 0.0f;
#endif

#if defined(TEXLERP)
	OUT.TexCoord1 = 0.0f;
#endif

#if defined(HORIZFADE)
    OUT.WorldZPos = 0.0f;
#endif

#if !defined(OCCLUSION)
	OUT.Color = 0.0f;
#endif

#if defined(DITHER) && !defined(TEX)
	OUT.NoiseCoord = 0.0f;
#endif
}

	
v2fSky VS(a2v IN)
{
	v2fSky OUT;
	InitializeVertexOut(OUT);

	float4 position = float4( IN.Position.xyz, 1 );
	float3 vec = normalize(position.xyz);

	OUT.HPosition = Transform( position, WorldViewProj );

	return OUT;
}


float4 PS(v2fSky IN) : OutputColor
{
	return float4(BaseColor, 1);
}


//////// techniques ////////////////////////////

technique Sky
{
	pass p0
	{
		VertexShader = compile vs_3_0 VS();
		PixelShader = compile ps_3_0 PS();
	}
}
