// This is used by 3dsmax to load the correct parser
string ParamID = "0x000";

// Vertex Attributes
struct a2v 
{
	float4 Position		: POSITION;
#if defined(VC)
	float4 Color		: COLOR;
#endif
	float4 TexCoord0	: TEXCOORD0;
};

// Vertex constants
float4x4 WorldViewProj		:		WORLDVIEWPROJ;
float4x4 World				: 		WORLD;

bool bReflectionsEnabled <string UIName = "Reflections";>  = false;
bool bRefractionsEnabled <string UIName = "Refractions";>  = false;
bool bDepthEnabled <string UIName = "Depth";> = false;
bool bVertexUVEnabled <string UIName = "Vertex UV";> = false;
bool bVertexAlphaDepthEnabled <string UIName = "Vertex Alpha";> = false;
bool bFoggingEnabled <string UIName = "Fog";> = false;
bool bSilhouetteReflectionsEnabled <string UIName = "Silhouette Reflections";> = false;


//performs vec*mat where mat is a column major matrix so that the matrix multiply can be done as a series of dot products.
float4 Transform(float4 vec, float4x4 mat)
{
	return mul(vec, mat);
}

//performs vec*mat where mat is a column major matrix so that the matrix multiply can be done as a series of dot products.
float3 Transform(float4 vec, float3x4 mat)
{
	return mul(mat, vec);
}

// Pixel Attributes
struct v2fWater
{
	float4 HPosition	: POSITION;
};

v2fWater VS(a2v IN)
{
	v2fWater OUT = (v2fWater)0;

	float4 inPos = IN.Position;

	// first multiply vertex position by view and projection matrices to get it to clip space
	float4 outPos = Transform(inPos, WorldViewProj);
	
	// then set the output position and texture coordinate...
	OUT.HPosition = outPos;

	return OUT;
}

float4 PS(v2fWater IN) : COLOR
{
	return float4( 0.0f, 0.0f, 1.0f, 1.0f );
}

//////// techniques ////////////////////////////

technique Default
{

	pass p0
	{
		VertexShader = compile vs_3_0 VS();
		PixelShader = compile ps_3_0 PS();
	}
}
