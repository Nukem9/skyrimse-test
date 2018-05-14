// This is used by 3dsmax to load the correct parser
string ParamID = "0x000";

#define CAT(a, b) a ## b

#define BSConstantBuffer(x, i)
#define End

#define DeclareTex2D(name, i) sampler2D name : register(CAT(s, i));
#define ParamTex2D(name) sampler2D name
#define PassTex2D(name) name
#define SampleTex2D(name, u) tex2D(name, u)
#define SampleTex2DGrad(name, u, ddx, ddy) tex2D(name, u, ddx, ddy)
#define SampleTex2DProj(name, u) tex2Dproj(name, u)


#define DeclareTex3D(name, i) sampler3D name : register(CAT(s, i));
#define ParamTex3D(name) sampler3D name
#define PassTex3D(name) name
#define SampleTex3D(name, u) tex3D(name, u)

#define DeclareTexCube(name, i) samplerCUBE name : register(CAT(s, i));
#define ParamTexCube(name) samplerCUBE name
#define PassTexCube(name) name
#define SampleTexCube(name, u) texCUBE(name, u)

#define OutputPosition POSITION
#define OutputColor COLOR
#define OutputColor1 COLOR1
#define OutputColor2 COLOR2
#define OutputColor3 COLOR3
#define OutputDepth DEPTH

struct a2v {
	float4 Position : POSITION; //in object space
	float3 Normal : NORMAL; //in object space
	float2 TexCoord : TEXCOORD0;
	float3 Color		: TEXCOORD1;
	float Alpha		: TEXCOORD2;
};

float2 GetVertexTexCoord(a2v IN)
{
	return IN.TexCoord+float2(0, 1);
}

float GetVertexTextureIndex(a2v IN)
{
	return 0.0f;
}

float4 GetVertexColor(a2v IN)
{
	return float4(abs(IN.Color.rgb), IN.Alpha);
}

float3 GetVertexNormal(a2v IN)
{
	return IN.Normal;
}

float4 GetVertexBlendWeight(a2v IN)
{
	return 1;
}

float4 GetVertexBlendIndices(a2v IN)
{
	return 1;
}

float2 GetIndexedTextureCoord(float2 texcoord, float textureindex)
{
	return texcoord;
}

bool bShowBase <string UIName = "Show Base";> = true;
float3 baseColor <string UIName = "Base";> = float3( 1.0f, 1.0f, 1.0f);
float fBaseColorScale <
	string UIName = "Base Color Scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 100.0f;	
	float UIStep = 0.01f;
>  = 1.0f;
bool bVertexColorsEnabled <string UIName = "Show Vertex Color";> = false;
bool g_bTwoSided <string UIName = "Two Sided";> =false;
bool bDecal <string UIName = "Decal";> =false;
bool bDecalNoFade <string UIName = "Decal NoFade";> =false;
bool bLightingEnabled <string UIName = "Lighting";> =false;

float fLightingInfluence <
	string UIName = "Lighting Influence";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.01f;
>  = 1.0f;

float GetLightInfluence()
{
	return fLightingInfluence;
}

bool bSoft <string UIName = "Soft";> = false;
float fSoftDepth<
	string UIName = "Soft Depth";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 9999.0f;	
	float UIStep = 1.0f;
>  = 100.0f;

bool bAlphaFalloffEnabled <string UIName = "Falloff";> =false;
bool bColorFalloffEnabled <string UIName = "Color Falloff";> =false;
float fFalloffStartAngle<
	string UIName = "Falloff Start Angle";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 360.0f;	
	float UIStep = 0.1f;
>  = 0.0f;
float fFalloffStopAngle<
	string UIName = "Falloff Stop Angle";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 360.0f;	
	float UIStep = 0.1f;
>  = 0.0f;
float fFalloffStartOpacity<
	string UIName = "Falloff Start Opacity";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.01f;
>  = 0.0f;
float fFalloffStopOpacity<
	string UIName = "Falloff Stop Opacity";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.01f;
>  = 0.0f;

float GetFalloffStartAngle() { return cos(fFalloffStartAngle * (3.14159265/180.0f)); }
float GetFalloffStopAngle() { return cos(fFalloffStopAngle * (3.14159265/180.0f)); }
float GetFalloffStartOpacity() { return fFalloffStartOpacity; }
float GetFalloffStopOpacity() { return fFalloffStopOpacity; }

float fBaseAlpha<
	string UIName = "Base Alpha";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.01f;
>  = 1.0f;

float fEnvmapScale<
	string UIName = "Envmap scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.01f;
> = 1.0f;

int iEnvmapMinLOD<
	string UIName = "Envmap Min LOD";
	string UIType = "IntSpinner";
	float UIMin = 0.0f;
	float UIMax = 16.0f;	
>  = 0;

float4x4 World      : 		WORLD;
float4x4 WorldI		:		WORLDI;
float4x4 View       : 		VIEW;
float4x4 ViewI		:		VIEWI;
float4x4 ViewProj   :		VIEWPROJ;
float4x4 Projection : 		PROJECTION;
float4x4 WorldViewProj : 	WORLDVIEWPROJ;
float4x4 WorldView : 		WORLDVIEW;

float4 g_LightPos : POSITION 
<  
	string UIName = "Light Position"; 
	string Object = "PointLight";
	string Space = "Object";
	int refID = 0;
> = {-0.577, -0.577, 0.577,0.0};

float4 g_LightCol : LIGHTCOLOR
<
	int LightRef = 0;
	string UIWidget = "None";
> = float4(0.0f, 0.0f, 0.0f, 0.0f);

float4 GetPLightPositionX()
{
	return mul(g_LightPos, WorldI).xxxx;
}
float4 GetPLightPositionY()
{
	return mul(g_LightPos, WorldI).yyyy;
}
float4 GetPLightPositionZ()
{
	return mul(g_LightPos, WorldI).zzzz;
}
float4 GetPLightingRadiusInverseSquared()
{
	return 1.0f/(500.0f*500.0f);
}
float4 GetPLightColorR()
{
	return float4(g_LightCol.r, 0, 0, 0);
}
float4 GetPLightColorG()
{
	return float4(g_LightCol.g, 0, 0, 0);
}
float4 GetPLightColorB()
{
	return float4(g_LightCol.b, 0, 0, 0);
}
float3 GetDLightColor()
{
	return 1;
}

bool bZTestEnabled <string UIName = "Z Test";> =true;
bool bZWriteEnabled <string UIName = "Z Write";> =true;

bool g_AlphaTest <string UIName = "Alpha Test";> =false;
int g_AlphaRef<
	string UIName = "Alpha Test Ref";
	string UIType = "IntSpinner";
	float UIMin = 0.0f;
	float UIMax = 255.0f;	
>  = 128;

bool g_AlphaStandard <string UIName = "Alpha Blend - Standard";> =false;
bool g_AlphaAdd <string UIName = "Alpha Blend - Additive";> =false;
bool g_AlphaMult <string UIName = "Alpha Blend - Multiplicative";> =false;

bool bBloodEnabled <string UIName = "Blood Enabled";> =false;
float fBloodFalloffThresh<
	string UIName = "Blood Falloff Thresh";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
>  = 0.3f;
float fBloodAlphaToRGBScale<
	string UIName = "Blood Alpha To RGB Scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
>  = 0.85f;

float4 GetAlphaTestRef()
{
	return g_AlphaRef/255.0f;
}
float4 GetBloodFalloffThresh()
{
	return fBloodFalloffThresh;
}
float4 GetBloodAlphaToRGBScale()
{
	return fBloodAlphaToRGBScale;
}

float3 GetEyePosition()
{
	return ViewI[3];
}

bool bGrayscaleToColor <string UIName = "Grayscale to Color";> = false;
bool bGrayscaleToAlpha <string UIName = "Grayscale to Alpha";> = false;

bool bRefractionEnabled <string UIName = "Refraction";> =false;
float fRefractionPower<
	string UIName = "Refraction Power";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.001f;
>  = 0.0f;
bool bRefractionFalloffEnabled <string UIName = "Refraction Falloff";> =false;

bool bTileU <string UIName = "Tile U";> = true;
bool bTileV <string UIName = "Tile V";> = true;

bool bBaseTextureEnabled <string UIName = "Base Texture Enabled";> =false;
texture baseTexture : DiffuseMap< string UIName = "Base Texture"; >;
sampler2D BaseSampler = sampler_state { Texture = <baseTexture>; };
float4 SampleBase(float2 atexcoord) { return bBaseTextureEnabled ? tex2D( BaseSampler, atexcoord ) : 1; }

bool bGrayscaleTextureEnabled <string UIName = "Grayscale Texture Enabled";> =false;
texture grayscaleTexture < string UIName = "Grayscale Texture"; >;
sampler2D GrayscaleSampler = sampler_state { Texture = <grayscaleTexture>;
	AddressU = Clamp;
	AddressV = Clamp; 
};
float4 SampleGrayscale(float2 atexcoord) { return bGrayscaleTextureEnabled ? tex2D( GrayscaleSampler, atexcoord ) : 1; }

bool bEnvmapTextureEnabled <string UIName = "Envmap Texture Enabled";> =false;
texture envmapTexture : DiffuseMap< string UIName = "Envmap Texture"; >;
sampler2D EnvmapSampler = sampler_state { Texture = <envmapTexture>; };
float4 SampleEnvmap(float2 atexcoord) { return bEnvmapTextureEnabled ? tex2D( EnvmapSampler, atexcoord ) : 1; }

bool bNormalTextureEnabled <string UIName = "Normal Texture Enabled";> =false;
texture normalTexture : DiffuseMap< string UIName = "Normal Texture"; >;
sampler2D NormalSampler = sampler_state { Texture = <normalTexture>; };
float4 SampleNormal(float2 atexcoord) { return bNormalTextureEnabled ? tex2D( NormalSampler, atexcoord ) : 1; }

bool bEnvmapMaskTextureEnabled <string UIName = "Envmap Mask Texture Enabled";> =false;
texture envmapMaskTexture : DiffuseMap< string UIName = "Envmap Mask texture"; >;
sampler2D EnvmapMaskSampler = sampler_state { Texture = <envmapMaskTexture>; };
float4 SampleEnvmapMask(float2 atexcoord) { return bEnvmapMaskTextureEnabled ? tex2D( EnvmapMaskSampler, atexcoord ) : 1; }

float4 SampleNoise(float2 atexcoord) { return 1; }

int texcoord0 : Texcoord
<
	int Texcoord = 0;
	int MapChannel = 1;
	string UIWidget = "None";
>;

int texcoord1 : Texcoord
<
	int Texcoord = 1;
	int MapChannel = 0;
	string UIWidget = "None";
>;

int texcoord2 : Texcoord
<
	int Texcoord = 2;
	int MapChannel = -2;
	string UIWidget = "None";
>;

float UOffset<
	string UIName = "U Offset";
	string UIType = "FloatSpinner";
	float UIMin = -100.0f;
	float UIMax = 100.0f;	
	float UIStep = 0.01f;
>  = 0.0f;
float VOffset<
	string UIName = "V Offset";
	string UIType = "FloatSpinner";
	float UIMin = -100.0f;
	float UIMax = 100.0f;	
	float UIStep = 0.01f;
>  = 0.0f;
float UScale<
	string UIName = "U Scale";
	string UIType = "FloatSpinner";
	float UIMin = -100.0f;
	float UIMax = 100.0f;	
	float UIStep = 0.01f;
>  = 1.0f;
float VScale<
	string UIName = "V Scale";
	string UIType = "FloatSpinner";
	float UIMin = -100.0f;
	float UIMax = 100.0f;	
	float UIStep = 0.01f;
>  = 1.0f;
float2 GetUVOffset()
{
	return float2(UOffset, VOffset);
}
float2 GetUVScale()
{
	return float2(UScale, VScale);
}

float4 GetBaseColor()
{
	float3 base = baseColor.rgb;
	if (!bGrayscaleToColor)
		base *= fBaseColorScale;
	return float4(base.rgb, fBaseAlpha);
}
float GetBaseColorScale()
{
	if (bGrayscaleToColor)
		return fBaseColorScale;
	else
		return 1.0f;
}
float4 GetPropertyColor()
{ 
	return 1;
}

float4 Bones[180];
float4 FogNearColor = float4(1, 0, 0, 1);
float4 FogFarColor = float4(1, 0, 0, 1);
float4 FogParam = float4(10000000, 10000000-1, 1, 0);

#define VC
#define LIGHTING

// fog blend value based
float GetFogAmount(float3 apPosition, float afFar, float afFarMinusNear, float afPower)
{
	float dist = length( apPosition );
	return pow(1.0 - saturate( ( (afFar - dist) / (afFarMinusNear) ) ), afPower);
}

// fog blend value based
float4 GetFog(float3 apPosition, float afNearInverseFarMinusNear, float afInverseFarMinusNear, float afPower, float afClamp, float3 NearColor, float3 FarColor)
{
	float4 ret;
	float dist = length( apPosition );
	ret.a = min(pow(saturate( dist*afInverseFarMinusNear-afNearInverseFarMinusNear ), afPower), afClamp);
	ret.rgb = lerp(NearColor, FarColor, ret.a);
	
	return ret;
}

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

// transform a float4 vector by 4 bone skin matrix
// after call, avPosition is the final modelspace position
float4 TransformSkinned (  float4x4 matViewProj, inout float4 avPosition, float4 avWeights, float4 avBlendIndices, float4 Bones[180] )
{

  float4 tmpPos;
  tmpPos.xyz = avPosition.xyz;
  tmpPos.w = 1.0;

  // multiply blendindices to get offset into bonematrix array
  float4 boneOffsets = avBlendIndices * 765.01;
  
  float3x4 LocalToWorldMatrix = 0;
	
  LocalToWorldMatrix += avWeights.x * float3x4(Bones[boneOffsets.x], Bones[boneOffsets.x+1], Bones[boneOffsets.x+2]);
  LocalToWorldMatrix += avWeights.y * float3x4(Bones[boneOffsets.y], Bones[boneOffsets.y+1], Bones[boneOffsets.y+2]);
  LocalToWorldMatrix += avWeights.z * float3x4(Bones[boneOffsets.z], Bones[boneOffsets.z+1], Bones[boneOffsets.z+2]);
  LocalToWorldMatrix += avWeights.w * float3x4(Bones[boneOffsets.w], Bones[boneOffsets.w+1], Bones[boneOffsets.w+2]);

  //// determine final skinned values
  float4 finalPos;
  finalPos.xyz = float3( dot(tmpPos, LocalToWorldMatrix[0]), dot(tmpPos, LocalToWorldMatrix[1]), dot(tmpPos, LocalToWorldMatrix[2]) );
  finalPos.w = 1.0;

  avPosition = finalPos;

  // view projection
  return Transform(finalPos, matViewProj);
}

// transform a float4 vector and 3 vector tangent space by 4 bone skin matrix
//   normal, tangent, & binormal skinned values are returned through the same parameters as were passed
float4 TransformSkinnedTangent( float4x4 matViewProj,
					  inout float4 avPosition,
					  inout float3 normal,
					  inout float3 binormal,
					  inout float3 tangent,
					  float4 avWeights,
					  float4 avBlendIndices,
					  float4 Bones[180] )
{
  float4 tmpPos;
  tmpPos.xyz = avPosition.xyz;
  tmpPos.w = 1.0;
  
  float4 n;
  n.xyz = normal;
  n.w = 0;
  float4 b;
  b.xyz = binormal;
  b.w = 0;
  float4 t;
  t.xyz = tangent;
  t.w = 0;

  // multiply blendindices to get offset into bonematrix array
  float4 boneOffsets = avBlendIndices * 765.01;
  
  float3x4 LocalToWorldMatrix = 0;
	
  LocalToWorldMatrix += avWeights.x * float3x4(Bones[boneOffsets.x], Bones[boneOffsets.x+1], Bones[boneOffsets.x+2]);
  LocalToWorldMatrix += avWeights.y * float3x4(Bones[boneOffsets.y], Bones[boneOffsets.y+1], Bones[boneOffsets.y+2]);
  LocalToWorldMatrix += avWeights.z * float3x4(Bones[boneOffsets.z], Bones[boneOffsets.z+1], Bones[boneOffsets.z+2]);
  LocalToWorldMatrix += avWeights.w * float3x4(Bones[boneOffsets.w], Bones[boneOffsets.w+1], Bones[boneOffsets.w+2]);

  //// determine final skinned values
  float4 finalPos;
  finalPos.xyz = float3( dot(tmpPos, LocalToWorldMatrix[0]), dot(tmpPos, LocalToWorldMatrix[1]), dot(tmpPos, LocalToWorldMatrix[2]) );
  finalPos.w = 1.0;

  //// save the final worldspace position for lighting calculations
  avPosition = finalPos;

  //// combine weighted tangent space
  float3 skinnednormal = float3( dot(n, LocalToWorldMatrix[0]), dot(n, LocalToWorldMatrix[1]), dot(n, LocalToWorldMatrix[2]) );
  float3 skinnedtangent = float3( dot(t, LocalToWorldMatrix[0]), dot(t, LocalToWorldMatrix[1]), dot(t, LocalToWorldMatrix[2]) );
  float3 skinnedbinormal = float3( dot(b, LocalToWorldMatrix[0]), dot(b, LocalToWorldMatrix[1]), dot(b, LocalToWorldMatrix[2]) );

  normal = normalize( skinnednormal );
  tangent = normalize( skinnedtangent );
  binormal = normalize( skinnedbinormal );

  // view projection

  return Transform(finalPos,matViewProj);
}

float4 HighRangeToLowRange(float4 aColor)
{
	return aColor * float4(.5, .5, .5, 1);
}

float4 LowRangeToHighRange(float4 aColor)
{
	return aColor * float4(2, 2, 2, 1);
}


struct v2fOut
{
	float4 HPosition	: OutputPosition;

	float4 TexCoord0	: TEXCOORD0; //texcoord + fader value

#ifdef VC
	float4 Color		: COLOR;
#endif

	float4 FogColor		: COLOR1;
	float3 Normal		: TEXCOORD1;
	float4 EyeVec		: TEXCOORD2;

#ifdef LIGHTING
	float3 Position		: TEXCOORD3;
#endif
};


struct v2fIn
{
	float4 HPosition	: OutputPosition;

	float4 TexCoord0	: TEXCOORD0; //texcoord + fader value

#ifdef VC
	float4 Color		: COLOR;
#endif

	float4 FogColor		: COLOR1;
	float3 Normal		: TEXCOORD1;
	float4 EyeVec		: TEXCOORD2;

#ifdef LIGHTING
	float3 Position		: TEXCOORD3;
#endif

	float vface			: VFACE;
};

v2fOut VS(a2v IN)
{
	v2fOut OUT;
	
	float4 position = float4(IN.Position.xyz, 1);
	float3 normal = GetVertexNormal(IN);
	float2 texcoord = GetVertexTexCoord(IN);
	float4 vertexcolor = GetVertexColor(IN);
	float textureindex = GetVertexTextureIndex(IN);
	float4 blendweight = GetVertexBlendWeight(IN);
	float4 blendindices = GetVertexBlendIndices(IN);

	float3 binormal = float3(0, 1, 0);
	float3 tangent = float3(1, 0, 0);	

	OUT.HPosition = Transform(position,WorldViewProj);
	
#ifdef LIGHTING
	OUT.Position = position.rgb;
#endif

	OUT.TexCoord0 = 0;
	
	if (bBaseTextureEnabled)
	{
		OUT.TexCoord0.xy = texcoord * GetUVScale() + GetUVOffset();
	}
	
	//the position and normal for skinned geometry is already in world space here, for non-skinned geometry they are in modelspace
	normal = normalize(Transform(float4(normal.xyz, 0), World));
	position.w = 1.0f;
	position.xyz = Transform(position, World);

	OUT.TexCoord0.z = 1.0f;
	if (bAlphaFalloffEnabled || bColorFalloffEnabled)
	{
		float3 posEye = GetEyePosition();
		float3 view_dir = normalize(posEye-position);
		float angle = abs(dot(normal, view_dir));
		float fade = smoothstep(GetFalloffStartAngle(), GetFalloffStopAngle(), angle);
		OUT.TexCoord0.z = lerp(GetFalloffStartOpacity(), GetFalloffStopOpacity(), fade);
	}

	OUT.EyeVec.w = OUT.TexCoord0.z;
	OUT.Normal = normalize(normal);

	// EyeVec and normal are in worldspace if skinned, otherwise they are in modelspace
	OUT.EyeVec.xyz = normalize(GetEyePosition() - IN.Position.xyz);

#ifdef VC
	OUT.Color = vertexcolor;
#endif

	OUT.FogColor = GetFog( OUT.HPosition.xyz, FogParam.x, FogParam.y, FogParam.z, FogParam.w, FogNearColor.rgb, FogFarColor.rgb );

	return OUT;
}

float4 GetPSVertexColor(v2fIn IN)
{
#ifdef VC
	return IN.Color;
#else
	return 1;
#endif	
}

float2 GetScreenPosition(v2fIn IN)
{
	return float2( 1, 1 );
}

float3 GetGrayscaleToPaletteColor(float2 aTexcoord, float afOffset)
{
	float gray = SampleBase(aTexcoord).g;
	float2 coord = float2(gray, afOffset);
	return SampleGrayscale(coord).rgb;
}

float GetGrayscaleToPaletteAlpha(float2 aTexcoord, float afOffset)
{
	float gray = SampleBase(aTexcoord).a;
	float2 coord = float2(gray, afOffset);
	return SampleGrayscale(coord).a;
}

float4 PS(v2fIn IN) : OutputColor
{	
	float4 colBase = GetBaseColor();
	float4 colVertex = float4(1, 1, 1, 1);
	float4 colTexture = float4(1, 1, 1, 1);

	 if (!bTileU)
 		IN.TexCoord0.x = saturate(IN.TexCoord0.x);
 	if (!bTileV)
 		IN.TexCoord0.y = saturate(IN.TexCoord0.y);

	if (bVertexColorsEnabled)
		colVertex = GetPSVertexColor(IN);

	if (bBaseTextureEnabled)
		colTexture = SampleBase(IN.TexCoord0.xy);
		
	float ffalloff = IN.TexCoord0.z;

	if (bGrayscaleToAlpha)
		colTexture.a = 1;
	
	if (bAlphaFalloffEnabled)
		colTexture.a *= ffalloff; //apply the falloff fader
	if (bColorFalloffEnabled)
		colTexture.rgb *= ffalloff;

	if (!bShowBase)
		colBase = 1;
		
	float4 colFragout = colBase * colVertex * colTexture;

	if (bBloodEnabled)
	{
		float fthresh = GetBloodFalloffThresh();
	
		float falpha = colFragout.b * colVertex.a;
		float ffalloff = saturate(colFragout.g - GetAlphaTestRef());
		if (ffalloff  < fthresh)
			falpha *= (ffalloff/fthresh);
		float foutalpha = colFragout.g;
		colFragout.rgb = saturate(float3(1, 0, 0) + (1.0f - falpha)) * (1.0f - falpha*GetBloodAlphaToRGBScale());
		colFragout.a = foutalpha;
	}
	
	// Calculate the normal in worldspace for skinned geometry and modelspace otherwise
	float3 normal = IN.Normal;
	
	if (bGrayscaleToColor)
		colFragout.rgb = GetGrayscaleToPaletteColor(IN.TexCoord0.xy, colVertex.r*colBase.r * ffalloff) * GetBaseColorScale();
	if (bGrayscaleToAlpha)
		colFragout.a = GetGrayscaleToPaletteAlpha(IN.TexCoord0.xy, colFragout.a*GetPropertyColor().a);

	if (bLightingEnabled)
	{
		float4 plightpositionx = GetPLightPositionX();
		float4 plightpositiony = GetPLightPositionY();
		float4 plightpositionz = GetPLightPositionZ();
		float4 plightradiusinvsq = GetPLightingRadiusInverseSquared();
		float4 plightcolorr = GetPLightColorR();
		float4 plightcolorg = GetPLightColorG();
		float4 plightcolorb = GetPLightColorB();
		float3 dlightcolor = GetDLightColor();
#ifdef LIGHTING
		float3 position = IN.Position;
#else
		float3 position = 0;
#endif
		
		float4 plightvectorx = plightpositionx - position.xxxx;
		float4 plightvectory = plightpositiony - position.yyyy;
		float4 plightvectorz = plightpositionz - position.zzzz;
		float4 plightdistsq = plightvectorx*plightvectorx + plightvectory*plightvectory + plightvectorz*plightvectorz;
		float4 plightatten = 1.0f - saturate(plightdistsq*plightradiusinvsq);
		float3 lightcolor = dlightcolor;
		lightcolor.r += dot(plightatten * plightcolorr, float4(1, 1, 1, 1));
		lightcolor.g += dot(plightatten * plightcolorg, float4(1, 1, 1, 1));
		lightcolor.b += dot(plightatten * plightcolorb, float4(1, 1, 1, 1));
		colFragout.rgb = lerp(colFragout.rgb, lightcolor*colFragout.rgb, GetLightInfluence());
	}
	else
	{
		colFragout.rgb = lerp(colFragout.rgb, GetPropertyColor().rgb*colFragout.rgb, GetLightInfluence());
	}
	if (!bGrayscaleToAlpha)
		colFragout.a *= GetPropertyColor().a;

	float ffogAmount = IN.FogColor.a;
	colFragout.rgb = lerp(colFragout.rgb, IN.FogColor.rgb, ffogAmount);

	if( g_AlphaTest )
		clip(colFragout.a < g_AlphaRef/255.0f ? -1:1);

	if( !g_AlphaStandard && !g_AlphaAdd && !g_AlphaMult )
		colFragout.a = 0.0f;

	if( !g_bTwoSided )
		clip( IN.vface < 0.0f ? -1:1 );

	return colFragout;
}

//////// techniques ////////////////////////////

technique Default
{

	pass p0
	{
		ZEnable = bZTestEnabled;
		ZWriteEnable = bZWriteEnabled;
		AlphaBlendEnable = g_AlphaStandard || g_AlphaAdd || g_AlphaMult;
		SrcBlend = g_AlphaStandard ? 5: g_AlphaAdd ? 5 : 9;
		DestBlend = g_AlphaStandard ? 6: g_AlphaAdd ? 2 : 1;
		CullMode = NONE;
		VertexShader = compile vs_3_0 VS();
		PixelShader = compile ps_3_0 PS();
	}
}
