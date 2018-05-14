string ParamID = "0x000";

/////////////////////////////////////////
//#include "brdf.fxh"
#ifndef __ILLUM_H__
#define __ILLUM_H__

/**
 *  Copyright (C) 2011 by Morten S. Mikkelsen
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */


/// IMPORTANT!!!! ///
/// IMPORTANT!!!! ///
// All these are implemented such that n_dot_l is built into the BRDF
// and thus the shader using them should NOT multiply by this factor.
// The diffuse term is also built into the BRDF.

// Another important observation is that most implementations of the Torrance-Sparrow
// or Cook-Torrance model neglect to deal with division by zero issues.
// Most often those that do don't do it well such that the right limit value is determined
// giving a smooth/continuous behavior.

// A clear derivation of the Torrance-Sparrow model is given in section 2.4
// of my paper --> http://jbit.net/~sparky/academic/mm_brdf.pdf
// Note the error in the cook-torrance model pointed out at the end of this section.
// Another important observation is that the bechmann distribution can be replaced
// with a normalized phong distribution (which is cheaper) using: float toNPhong(const float m)


#ifndef M_PI
	#define M_PI 3.1415926535897932384626433832795
#endif

float3 FixNormal(float3 vN, float3 vV);
float toBeckmannParam(const float n);
float toNPhong(const float m);
float3 FixNormal(float3 vN, float3 vV);

// Schlick's Fresnel approximation
float fresnelReflectance( float VdotH, float F0 )
{
	float base = 1-VdotH;
	float exponential = pow(base, 5.0);	// replace this by 3 muls for C/C++
	return saturate(exponential + F0 * (1 - exponential));
}

#define FLT_EPSILON     1.192092896e-07F        // smallest such that 1.0+FLT_EPSILON != 1.0
#define FLT_MAX         3.402823466e+38F        // max value
#define FLT_MIN         1.175494351e-38F        // min positive value


// The Torrance-Sparrow visibility factor, G,
// as described by Jim Blinn but divided by VdotN
// Note that this was carefully implemented such that division
// by zero problems and awkward discontinuities are avoided.
float VisibDiv(float LdotN, float VdotN, float VdotH, float HdotN)
{	
	// VdotH should never be zero. Only possible if
	// L and V end up in the same plane (unlikely).
	const float denom = max( VdotH, FLT_EPSILON );	
										
	float numL = min(VdotN, LdotN);
	const float numR = 2*HdotN;
	if((numL*numR)<=denom)	// min(x,1) = x
	{
		numL = numL == VdotN ? 1.0 : (LdotN / VdotN);	// VdotN is > 0 if this division is used
		return (numL*numR) / denom;
	}
	else					// min(x,1) = 1				this branch is taken when H and N are "close" (see fig. 3)
		return 1.0 / VdotN;
		// VdotN >= HdotN*VdotN >= HdotN*min(VdotN, LdotN) >= FLT_EPSILON/2
}


// this is a normalized Phong model used in the Torrance-Sparrow model
float3 BRDF_ts_nphong(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32, float F0=0.2)
{
	// reflect hack when view vector is occluded
	// (doesn't seem to be needed)
	//vN = FixNormal(vN, vV);

	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	float fDiff = LdotN;

	// D is a surface distribution function and obeys:
	// D(vH)*HdotN is normalized (over half-spere)
	// Specifically, this is the normalized phong model
	const float D = ((n+2)/(2*M_PI))*pow(HdotN, n);

	// torrance-sparrow visibility term divided by VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	
	// Schlick's approximation
	const float fFres = fresnelReflectance(VdotH, F0);

	// torrance-sparrow:
	// (F * G * D) / (4 * LdotN * VdotN)
	// Division by VdotN is done in VisibDiv()
	// and division by LdotN is removed since 
	// outgoing radiance is determined by:
	// BRDF * LdotN * L()
	float fSpec = (fFres * fVdivDots * D) / 4;
	
	// sum up: diff + spec
	// technically fDiff should be divided by pi.
	// Instead, we choose here to scale Cs by pi
	// which makes the final result scaled by pi.
	// We do this to keep the output intensity range
	// at a level which is more "familiar".
	float3 res = /*Cd * fDiff +*/ M_PI * Cs * fSpec;
	return res;
}

// optional variant (self shadowing factor)
// vN is the shade normal (from a bump/normal map)
// vN2 represents the normalized interpolated vertex normal
float3 BRDF2_ts_nphong(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32, float F0=0.2)
{
	float3 res = BRDF_ts_nphong(vN, vL, vV, Cd, Cs, n, F0);
	return saturate(4*dot(vL, vN2)) * res;
}

// this is the Torrance-Sparrow model but using the Beckmann distribution
float3 BRDF_ts_beckmann(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22, float F0=0.2)
{
	// reflect hack when view vector is occluded
	// (doesn't seem to be needed)
	//vN = FixNormal(vN, vV);

	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	float fDiff = LdotN;
	
	// D is a surface distribution function and obeys:
	// D(vH)*HdotN is normalized (over half-spere)
	// Specifically, this is the Beckmann surface distribution function
	// D = exp(-tan^2(\theta_h)/m^2) / (pi * m^2 * cos^4(\theta_h));
	// where \theta_h = acos(HdotN)
	const float fSqCSnh = HdotN*HdotN;
	const float fSqCSnh_m2 = fSqCSnh*m*m;
	//const float numerator = exp(-pow(tan(acos(HdotN))/m,2));
	const float numerator = exp(-((1-fSqCSnh)/max(fSqCSnh_m2, FLT_EPSILON)));		// faster than tan+acos
	const float D = numerator / (M_PI*max(fSqCSnh_m2*fSqCSnh, FLT_EPSILON));

	// torrance-sparrow visibility term divided by VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	
	// Schlick's approximation
	const float fFres = fresnelReflectance(VdotH, F0);

	// torrance-sparrow:
	// (F * G * D) / (4 * LdotN * VdotN)
	// Division by VdotN is done in VisibDiv()
	// and division by LdotN is removed since 
	// outgoing radiance is determined by:
	// BRDF * LdotN * L()
	float fSpec = (fFres * fVdivDots * D) / 4;
	
	// sum up: diff + spec
	// technically fDiff should be divided by pi.
	// Instead, we choose here to scale Cs by pi
	// which makes the final result scaled by pi.
	// We do this to keep the output intensity range
	// at a level which is more "familiar".
	float3 res = Cd * fDiff + M_PI * Cs * fSpec;
	return res;
}

// optional variant (self shadowing factor)
// vN is the shade normal (from a bump/normal map)
// vN2 represents the normalized interpolated vertex normal
float3 BRDF2_ts_beckmann(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22, float F0=0.2)
{
	float3 res = BRDF_ts_beckmann(vN, vL, vV, Cd, Cs, m, F0);
	return saturate(4*dot(vL, vN2)) * res;
}


float toBeckmannParam(const float n)
{
	// remap to beckmann roughness parameter by matching
	// the normalization constants in the surface distribution functions.
	float m = sqrt(2 / (n+2));
	return m;
}

float toNPhong(const float m)
{
	// remap to normalized phong roughness parameter by matching
	// the normalization constants in the surface distribution functions.
	float n = (2 / (m*m)) - 2;
	return n;
}


//-------------------------- Alternative Tilt BRDF

float CalcTiltNormalization(const float brdfAng, const int n);

// this is an alternative to the normalized Phong model used in the Torrance-Sparrow model.
// This model supports a tilted peak as opposed to the default nphong case where brdfAng=0
float3 BRDF_ts_nphong_tilt(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32, float F0=0.2)
{	
	float3 vH = normalize(vV+vL);

	// reflect hack when view vector is occluded
	// (doesn't seem to be needed)
	//vN = FixNormal(vN, vV);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	float fDiff = LdotN;

	// D is a surface distribution function and obeys:
	// p(vH) = D(vH)*HdotN where p is normalized (over half-spere)
	// Specifically, this is a variant of the normalized phong model.
	// In this version p reaches its peak at the angle: brdfAng
	// if Vh is the angle between vN and vH then
	// p(vH) = fMultNorm * cos(Vh-brdfAng)^n
	// note that the integration domain remains the half-sphere
	// relative to the normal vN and that when
	// brdfAng is zero the result is identical to the regular nphong.
	
	const int ninc = n + 1;		// this increment is to match nphong which
								// has the division by n_dot_h built into it
	
	// hopefully this is resolved at compilation time
	float fMultNorm = CalcTiltNormalization(brdfAng, ninc);
	const float co2 = cos((brdfAng*M_PI)/180);
	const float si2 = sin((brdfAng*M_PI)/180);
	
	// Evaluate pdf and D
	const float co1 = dot(vH, vN);
	const float si1 = sqrt(1-co1*co1);
	const float H2dotN = saturate(co2*co1 + si2*si1);
	
	const float p = fMultNorm*pow(H2dotN, ninc);
	const float D = p / max(HdotN, FLT_EPSILON);

	// torrance-sparrow visibility term over VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	
	// Schlick's approximation
	const float fFres = fresnelReflectance(VdotH, F0);

	// torrance-sparrow
	float fSpec = (fFres * fVdivDots * D) / 4;
	
	// sum up: diff + spec
	// technically fDiff should be divided by pi.
	// Instead, we choose here to scale Cs by pi
	// which makes the final result scaled by pi.
	// We do this to keep the output intensity range
	// at a level which is more "familiar".
	float3 res = Cd * fDiff + M_PI * Cs * fSpec;
	return res;
}

// optional variant (self shadowing factor)
// vN is the shade normal (from a bump/normal map)
// vN2 represents the normalized interpolated vertex normal
float3 BRDF2_ts_nphong_tilt(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32, float F0=0.2)
{
	float3 res = BRDF_ts_nphong_tilt(vN, vL, vV, Cd, Cs, brdfAng, n, F0);
	return saturate(4*dot(vL, vN2)) * res;
}



////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Same BRDF functions but with Fresnel disabled /////////////////////
////////////////////////////////////////////////////////////////////////////////////////



float3 BRDF_ts_nphong_nofr(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32)
{
	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	float fDiff = LdotN;
	const float D = ((n+2)/(2*M_PI))*pow(HdotN, n);
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	float fSpec = (fVdivDots * D) / 4;

	return Cd * fDiff + M_PI * Cs * fSpec;
}

float3 BRDF2_ts_nphong_nofr(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32)
{
	float3 res = BRDF_ts_nphong_nofr(vN, vL, vV, Cd, Cs, n);
	return saturate(4*dot(vL, vN2)) * res;
}


float3 BRDF_ts_beckmann_nofr(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22)
{
	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	float fDiff = LdotN;
	const float fSqCSnh = HdotN*HdotN;
	const float fSqCSnh_m2 = fSqCSnh*m*m;
	const float numerator = exp(-((1-fSqCSnh)/max(fSqCSnh_m2, FLT_EPSILON)));		// faster than tan+acos
	const float D = numerator / (M_PI*max(fSqCSnh_m2*fSqCSnh, FLT_EPSILON));

	// torrance-sparrow visibility term divided by VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	float fSpec = (fVdivDots * D) / 4;
	
	return Cd * fDiff + M_PI * Cs * fSpec;
}

float3 BRDF2_ts_beckmann_nofr(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22)
{
	float3 res = BRDF_ts_beckmann_nofr(vN, vL, vV, Cd, Cs, m);
	return saturate(4*dot(vL, vN2)) * res;
}



float3 BRDF_ts_nphong_tilt_nofr(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32)
{	
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	float fDiff = LdotN;
	const int ninc = n + 1;
	
	// hopefully this is resolved at compilation time
	float fMultNorm = CalcTiltNormalization(brdfAng, ninc);
	const float co2 = cos((brdfAng*M_PI)/180);
	const float si2 = sin((brdfAng*M_PI)/180);
	
	const float co1 = dot(vH, vN);
	const float si1 = sqrt(1-co1*co1);
	const float H2dotN = saturate(co2*co1 + si2*si1);
	const float p = fMultNorm*pow(H2dotN, ninc);
	const float D = p / max(HdotN, FLT_EPSILON);

	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	float fSpec = (fVdivDots * D) / 4;
	
	return Cd * fDiff + M_PI * Cs * fSpec;
}

float3 BRDF2_ts_nphong_tilt_nofr(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32)
{
	float3 res = BRDF_ts_nphong_tilt_nofr(vN, vL, vV, Cd, Cs, brdfAng, n);
	return saturate(4*dot(vL, vN2)) * res;
}






////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Utility Functions ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


float CalcTiltNormalization(const float brdfAng, const int n)
{
	// if \theta_h is the angle between vN and vH then the pdf is
	// p(vH) = fMultNorm * cos(\theta_h-brdfAng)^n
	// note that when brdfAng is zero this is the regular nphong
	
	// this function computes fMultNorm as the
	// reciprocal value of p integrated over the hemisphere
	// oriented relative to vN.
	const float v0 = M_PI - ((brdfAng*M_PI)/180);
	
	// After substitution the integral can be expressed as:
	// area = 2 pi * ( sin(v0)* Int cos(u)^{n+1} du - cos(v0)* Int cos(u)^n * sin(u) du )
	// Both integrals are integrated over the domain: ]v0-pi; v0-(pi/2)[
	float fLterm = 0;
	int m = n+1;
	float factor = 1;
	
	const float t1 = v0 - (M_PI/2);
	const float t0 = v0 - M_PI;
	while(m>0)
	{	
		const float R1 = pow( cos(t1), m-1)*sin(t1);
		const float R0 = pow( cos(t0), m-1)*sin(t0);
		fLterm += (factor/m)*(R1-R0);	
		factor *= ((float) m-1) / m;
		m -= 2;
	}
	if(m==0) fLterm += factor*(t1-t0);
	
	float fRterm = (-1.0/(n+1)) * ( pow( cos(t1), n+1) - pow( cos(t0), n+1) );
	const float fNorm = (2*M_PI) * (sin(v0)*fLterm - cos(v0)*fRterm);
	
	return 1/fNorm;
}

float3 FixNormal(float3 vN, float3 vV)
{
	const float VdotN = dot(vV,vN);
	if(VdotN<=0)
	{
		vN = vN - 2*vV * VdotN;
	}
	return vN;
}

#endif
//////////////////////////////
//// UN-TWEAKABLES - AUTOMATICALLY-TRACKED TRANSFORMS ////////////////

float4x4 World      : 		WORLD;
float4x4 WorldI		:		WORLDI;
float4x4 View       : 		VIEW;
float4x4 ViewI		:		VIEWI;
float4x4 Projection : 		PROJECTION;
float4x4 WorldViewProj : 	WORLDVIEWPROJ;
float4x4 WorldView 	: 		WORLDVIEW;
float4x4 WorldIT 	: 		WorldInverseTranspose < string UIWidget="None"; >;

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

int texcoord3 : Texcoord
<
	int Texcoord = 3;
	int MapChannel = -1;
	string UIWidget = "None";
>;

//// TWEAKABLE PARAMETERS ////////////////////

/// Point Lamp 0 ////////////
float3 Lamp0Pos : POSITION <
    string Object = "PointLight0";
    string UIName =  "Light Position";
    string Space = "World";
	int refID = 0;
> = {-0.5f,2.0f,1.25f};

float3 Lamp0Color <string UIName = "Diffuse";> = float3(1.0f, 1.0f, 1.0f);
bool bShowAmbient <string UIName = "Show Ambient";> = true;
float4 ambientColor <string UIName = "Ambient";> = float4( 0.0f, 0.0f, 0.0f, 1.0f );

bool g_AlphaStandard <string UIName = "Alpha Blend - Standard";> =false;
bool g_AlphaAdd <string UIName = "Alpha Blend - Additive";> =false;
bool g_AlphaMult <string UIName = "Alpha Blend - Multiplicative";> =false;

bool bShowEmit <string UIName = "Show Emit";> = true;
bool bShowDiffuse <string UIName = "Show Diffuse";> = true;
bool bShowTexture <string UIName = "Show Texture";> = true;
bool bSpecularEnabled < string UIName = "Show Specular"; > = true;
bool bVertexColorsEnabled <string UIName = "Show Vertex Color";> = true;
bool bUseNormalMap <string UIName = "Use Normal Map";> = true;
bool bShowAlpha <string UIName = "Show Alpha";> = true;
bool bShowLeafWeights <string UIName = "Show Leaf Weights";> = false;
bool bLODObjects <string UIName = "LOD Object";> = false;
bool bLODObjectsHD <string UIName = "LOD Object HD";> = false;
bool bProjectedUV <string UIName = "Projected UV";> = false;
float fProjUVNoiseFalloffScale <string UIName = "Proj UV Noise Falloff Scale";> = 1.0f;
float fProjUVNoiseFalloffBias <string UIName = "Proj UV Noise Falloff Bias";> = 0.0f;
float fProjUVNoiseTextureScale <string UIName = "Proj UV Noise Texture Scale";> = 1.0f;
float fProjUVAngleRange <string UIName = "Proj UV Angle Range";> = 0.5f;
float3 ProjUVColor <string UIName = "Proj UV Color";> = float3(0.0f, 0.0f, 1.0f);
bool g_bTwoSided <string UIName = "Two Sided";> =false;
bool bModelSpaceNormals <string UIName = "Model Space Normals";> =false;
bool bRemapTexturesEnabled <string UIName = "Remappable Textures";> =true;
bool bDecal <string UIName = "Decal";> =false;
bool bDecalNoFade <string UIName = "Decal NoFade";> =false;
bool bExternalEmittance <string UIName = "External Emittance";> =false;
bool bHideSecret <string UIName = "Hide Secret";> =false;
bool bEnvmapLightFade <string UIName = "Envmap LightFade";> =true;
bool bZTestEnabled <string UIName = "Z Test";> =true;
bool bZWriteEnabled <string UIName = "Z Write";> =true;
bool bTreeAnim <string UIName = "Tree";> =false;
bool bRefractionEnabled <string UIName = "Refraction";> =false;
float fRefractionPower<
	string UIName = "Refraction Power";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.001f;
>  = 0.0f;
bool bRefractionFalloffEnabled <string UIName = "Refraction Falloff";> =false;

bool bParallaxEnabled <string UIName = "Parallax";> =false;
bool bParallaxOccEnabled <string UIName = "Parallax Occlusion";> =false;
bool bMultiLayerParallaxEnabled <string UIName = "Multi-Layer Parallax";> =false;
float ParallaxHeightScale<

	string UIName = "Parallax Height Scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 10.0f;	
	float UIStep = 0.1f;
>  = 1.0f;
int ParallaxNumSteps<
	string UIName = "Parallax Num Steps";
	string UIType = "IntSpinner";
	float UIMin = 1;
	float UIMax = 320;
>  = 4;
float fParallaxLayerThickness<
	string UIName = "Layer Thickness";
	string UIType = "FloatSpinner";
	float UIMin = 5.0f;
	float UIMax = 500.0f;
	float UIStep = 0.1f;
>  = 10.0f;
float fParallaxRefractionScale<
	string UIName = "Refraction Scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;
	float UIStep = 0.001f;
>  = 1.0f;
float fParallaxInnerLayerUScale<
	string UIName = "Inner Layer U Scale";
	string UIType = "FloatSpinner";
	float UIMin = -100.0f;
	float UIMax = 100.0f;	
	float UIStep = 0.01f;
>  = 1.0f;
float fParallaxInnerLayerVScale<
	string UIName = "Inner Layer V Scale";
	string UIType = "FloatSpinner";
	float UIMin = -100.0f;
	float UIMax = 100.0f;	
	float UIStep = 0.01f;
>  = 1.0f;

bool bFacegenEnabled <string UIName = "Facegen";> = false;
bool bAnisoLightingEnabled <string UIName = "Anisotropic Lighting";> =false;
bool bHairEnabled <string UIName = "Hair";> =false;
float3 HairTintColor <string UIName = "Hair Tint Color";> = 0.5f;
bool bBackLightingEnabled <string UIName = "Back Lighting";> =false;

bool bSubSurfaceLightingEnabled <string UIName = "Sub-Surface Lighting";> = false;
float fSubSurfaceLightRolloff<
	string UIName = "Sub Surface Rolloff";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 10.0f;	
	float UIStep = 0.01f;
>  = 0.3f;
bool bRimLightingEnabled <string UIName = "Rim Lighting";> =false;
float fRimLightPower<
	string UIName = "Rim Power";
	string UIType = "FloatSpinner";
	float UIMin = 0.1f;
	float UIMax = 10.0f;
	float UIStep = 0.1f;
>  = 2.0f;
float fBackLightPower<
	string UIName = "Back Light Power";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1000.0f;
	float UIStep = 0.01f;
>  = 0.0f;
float GetBackLightPower() { return fBackLightPower; }
bool bEnvmapEnabled <string UIName = "Envmap";> =false;
bool bEnvmapEye <string UIName = "Envmap Eye";> =false;
bool bEnvmapWindow <string UIName = "Envmap Window";> =false;
float fEnvmapScale<
	string UIName = "Envmap Scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 10.0f;	
	float UIStep = 0.01f;
>  = 1.0f;

float3 EmitColor <string UIName = "Emit Color";> = float4( 0.0f, 0.0f, 0.0f, 1.0f );
float fEmitScale<
	string UIName = "Emit Scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 10.0f;	
	float UIStep = 0.01f;
>  = 1.0f;

float fSmoothness<
	string UIName = "Smoothness";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
>  = 1.0f;

float fSpecularScale<
	string UIName = "Specular Scale";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 10.0f;	
	float UIStep = 0.01f;
>  = 1.0f;

float3 SpecularColor <string UIName = "Specular Color";> = float3( 1.0f, 1.0f, 1.0f );

float MatAlpha<
	string UIName = "Alpha";
	string UIType = "FloatSpinner";
	float UIMin = 0.0f;
	float UIMax = 1.0f;	
	float UIStep = 0.01f;
>  = 1.0f;

bool g_AlphaVertex <
	string UIName = "Vertex Alpha";
> = false;

bool g_AlphaTest <string UIName = "Alpha Test";> = false;
int g_AlphaRef<
	string UIName = "Alpha Test Ref";
	string UIType = "IntSpinner";
	float UIMin = 0.0f;
	float UIMax = 255.0f;	
>  = 128;

bool g_UseParallax <
	string UIName = "Normal Map Parallax";
> =false;

float g_ParallaxScale <
	string UIName = "Parallax Scale";
	string UIWidget = "slider";
	float UIMin = -0.50f;
	float UIMax = 0.5f;
	float UIStep = 0.01f;
>   = 0.02f;
									
float g_ParallaxBias <
	string UIName = "Parallax Bias";
	string UIWidget = "slider";
	float UIMin = -0.5f;
	float UIMax = 0.5f;
	float UIStep = 0.01f;
>   = 0.0f;

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

bool bGrayscaleToColor <string UIName = "Grayscale to Palette";> = false;

//////// COLOR & TEXTURE /////////////////////

bool bTileU <string UIName = "Tile U";> = true;
bool bTileV <string UIName = "Tile V";> = true;

bool bDiffuseTexEnabled <string UIName = "Diffuse Texture Enabled";> =false;
texture diffuseTexture : DiffuseMap< 
	string UIName = "Diffuse Map ";
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bNormalTexEnabled <string UIName = "Normal Texture Enabled";> =false;
texture normalTexture < 
	string UIName = "Normal Texture"; 
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bSmoothSpecMaskTexEnabled <string UIName = "Smooth/SpecMask Enabled";> =false;
texture smoothspecmaskTexture < 
	string UIName = "Smooth/SpecMask Texture"; 
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bHeightTexEnabled <string UIName = "Height Texture Enabled";> =false;
texture heightTexture < 
	string UIName = "Height Texture"; 
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bEnvTexEnabled <string UIName = "Envmap Texture Enabled";> =false;
texture envTexture  <
	string UIName = "Envmap Texture";
	string ResourceType = "3D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bGlowTexEnabled <string UIName = "Glow Texture Enabled";> =false;
texture glowTexture < 
	string UIName = "Glow Texture";
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bWrinkleTexEnabled <string UIName = "Wrinkle Texture Enabled";> =false;
texture wrinklesTexture < 
	string UIName = "Wrinkle Texture"; 
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bInnerLayerTexEnabled <string UIName = "Inner Layer Texture Enabled";> =false;
texture innerLayerTexture < 
	string UIName = "Inner Layer Texture"; 
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bBackLightMaskTexEnabled <string UIName = "Back Light Mask Texture Enabled";> =false;
texture backlightMask < 
	string UIName = "Back Light Mask Texture"; 
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

bool bSubSurfaceTexEnabled <string UIName = "SubSurface Tint Texture Enabled";> =false;
texture subsurfacetintTexture < 
	string UIName = "Subsurface Tint Texture"; 
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;
texture projectedNoiseTexture <
	string UIName = "Projected Noise Texture";
	string ResourceType = "2D";
	int Texcoord = 0;
	int MapChannel = 1;
>;

sampler2D DiffuseSampler = sampler_state { Texture = <diffuseTexture>;};
sampler2D NormalSampler = sampler_state { Texture = <normalTexture>;};
sampler2D WrinkleSampler = sampler_state { Texture = <wrinklesTexture>;};
sampler2D SmoothSpecMaskSampler = sampler_state { Texture = <smoothspecmaskTexture>;};
sampler2D HeightSampler = sampler_state { Texture = <heightTexture>; AddressU = Clamp; AddressV = Clamp; MAGFILTER = LINEAR; MINFILTER = LINEAR; };
samplerCUBE EnvSampler = sampler_state { Texture = <envTexture>;};
sampler2D GlowSampler = sampler_state { Texture = <glowTexture>;};
sampler2D SubSurfaceSampler = sampler_state { Texture = <subsurfacetintTexture>;};
sampler2D InnerLayerSampler = sampler_state { Texture = <innerLayerTexture>;};
sampler2D BackLightMaskSampler = sampler_state { Texture = <backlightMask>;};
sampler2D ProjectedNoiseSampler = sampler_state { Texture = <projectedNoiseTexture>; AddressU = Wrap; AddressV = Wrap; MAGFILTER = LINEAR; MINFILTER = LINEAR; MIPFILTER = NONE; };

float3 GetGrayscaleToPaletteColor(float2 aTexcoord, float afOffset)
{
	float gray = tex2D(DiffuseSampler,aTexcoord).g;
	float2 coord = float2(gray, afOffset);
	return tex2D(HeightSampler, coord).rgb;
}

/* data from application vertex buffer */
struct a2v 
{
	float4 Position	: POSITION;
	float3 Normal	: NORMAL;
	float3 Tangent	: TANGENT;
	float3 Binormal	: BINORMAL;
	float2 UV0		: TEXCOORD0;
	float3 Color	: TEXCOORD1;
	float3 Alpha	: TEXCOORD2;
	float3 Illum	: TEXCOORD3;
	float3 UV1		: TEXCOORD4;
	float3 UV2		: TEXCOORD5;
	float3 UV3		: TEXCOORD6;
	float3 UV4		: TEXCOORD7;
};

/* data passed from vertex shader to pixel shader */
struct v2fOut 
{
    float4 HPosition	: POSITION;
    float4 UV0			: TEXCOORD0;
    // The following values are passed in "World" coordinates since
    //   it tends to be the most flexible and easy for handling
    //   reflections, sky lighting, and other "global" effects.
    float3 LightVec		: TEXCOORD1;

	//This is a transform to put all vectors/points into the same space in the pixel shader
	//Normally this is the model to tangent space
	//For EnvMapping this is tangent to world space (or model to world space for model space normals)
	float3 CommonSpaceTransform0 : TEXCOORD2;
	float3 CommonSpaceTransform1 : TEXCOORD3;
	float3 CommonSpaceTransform2 : TEXCOORD4;

    float3 WorldView	: TEXCOORD5;
	float4 UV1			: TEXCOORD6;
	float4 UV2			: TEXCOORD7;
	float4 wPos			: TEXCOORD8;

	float Weight		: TEXCOORD9;
};

struct v2fIn
{
    float4 HPosition	: POSITION;
    float4 UV0			: TEXCOORD0;
    // The following values are passed in "World" coordinates since
    //   it tends to be the most flexible and easy for handling
    //   reflections, sky lighting, and other "global" effects.
    float3 LightVec		: TEXCOORD1;

	//This is a transform to put all vectors/points into the same space in the pixel shader
	//Normally this is the model to tangent space
	//For EnvMapping this is tangent to world space (or model to world space for model space normals)
	float3 CommonSpaceTransform0 : TEXCOORD2;
	float3 CommonSpaceTransform1 : TEXCOORD3;
	float3 CommonSpaceTransform2 : TEXCOORD4;

    float3 WorldView	: TEXCOORD5;
	float4 UV1			: TEXCOORD6;
	float4 UV2			: TEXCOORD7;
	float4 wPos			: TEXCOORD8;

	float Weight		: TEXCOORD9;

	float vface			: VFACE;
};
 
///////// VERTEX SHADING /////////////////////

v2fOut VS(a2v IN) 
{
    v2fOut OUT = (v2fOut)0;
 
	float3 normal = mul(IN.Normal,WorldIT).xyz;
    float3 binormal = mul(IN.Tangent,WorldIT).xyz;
    float3 tangent = mul(IN.Binormal,WorldIT).xyz;

	if( bModelSpaceNormals )
	{
		//model to world space
		OUT.CommonSpaceTransform0 = World[0].xyz;
		OUT.CommonSpaceTransform1 = World[1].xyz;
		OUT.CommonSpaceTransform2 = World[2].xyz;
	}
	else
	{
		//model to tangent space
		OUT.CommonSpaceTransform0 = tangent;
		OUT.CommonSpaceTransform1 = binormal;
		OUT.CommonSpaceTransform2 = normal;
	}

	float4 position = float4(IN.Position.xyz, 1);
    float4 Pw = mul(position, World);
    OUT.LightVec = (Lamp0Pos - Pw);
    OUT.WorldView = normalize(ViewI[3].xyz - Pw);
    OUT.HPosition = mul(position,WorldViewProj);
	OUT.wPos = mul(IN.Position, World);
	
	// UV bindings
	// Encode the color data
 	float4 color;
   	color.rgb = IN.Color;
   	color.a = IN.Alpha.x;
   	OUT.UV0.z = color.r;
   	OUT.UV0.a = color.g;
  	OUT.UV1.z = color.b;
   	OUT.UV1.a = color.a;

	// Pass through the UVs
	OUT.UV0.xy = (IN.UV0.xy + float2(0,1)) * float2(UScale,VScale) + float2(UOffset,VOffset);		// apparently max has Vs offset by -1
   	OUT.UV1.xy = IN.UV1.xy;
   	OUT.UV2.xyz = IN.UV2.xyz;

	// Pass through illumniation red channel for wrinkle map weight
	OUT.Weight = IN.Illum.r;

    return OUT;
}

///////// PIXEL SHADING //////////////////////

float GetAttenuation(float3 vect, float afRadius)
{
	return (1 - pow( saturate( (length( vect ) / afRadius) ), 2 ));
}

float3 DirectionalLightSpecular(
	float3 vectEye,
	float3 vectEyeDir,
	float3 vectLightDir,
	float3 vectNormal,
	float3 colLight,
	float fSpecularPower,
	float3 vectModelSpacePos,
	float2 texcoord )
{
	float3 vectHalfAngle = normalize( vectEyeDir + vectLightDir );
	float specBase = saturate( dot( vectHalfAngle, vectNormal ) );
	float3 finalSpecular = 0.0f;

	if( bSpecularEnabled )
		finalSpecular = pow( specBase, fSpecularPower );
	
	return colLight * ( finalSpecular );
}

float3 PointLightSpecular(
	float3 vectEye,	
	float3 vectEyeDir,
	float3 vectLight,
	float afRadius,
	float3 vectNormal,
	float3 colLight,
	float fSpecularPower,
	float3 vectModelSpacePos,
	float2 texcoord )
{
	return DirectionalLightSpecular(vectEye, vectEyeDir, normalize(vectLight), vectNormal, colLight, fSpecularPower, vectModelSpacePos, texcoord );
}

float3 DirectionalLightDiffuse(
	float3 vectLightDir,
	float3 vectNormal,
	float3 colLight)
{
	float3 diffuse = dot( vectNormal, vectLightDir );
	diffuse = saturate( diffuse );
	return colLight * diffuse;
}

float3 DirectionalLightDiffuseTranslucent(
	float3 vectLightDir,
	float3 vectNormal,
	float3 colLight,
	float3 colMask)
{
	float3 diffuse = saturate( -dot( vectNormal, vectLightDir ) ) * colMask.rgb;
	return colLight * diffuse;
}

float3 SubSurfaceIllumination(
	float3 vectLightDir,
	float3 vectNormal,
	float afRolloff,
	float3 colLight,
	float3 colSubSurface)
{
	float flightdotnormal = dot( vectNormal, vectLightDir );
	float fsubill = saturate(smoothstep( -afRolloff, 1.0, flightdotnormal ) - smoothstep( 0.0, 1.0, flightdotnormal ));
	return fsubill * colLight * colSubSurface;
}
	
float3 RimIllumination(
	float3 vectEyeDir,
	float3 vectLightDir,
	float3 vectNormal,
	float3 colLight,
	float3 colSubSurface,
	float afRimPower)
{
	float frimill = pow((1.0f - saturate(dot( vectNormal, vectEyeDir ))), afRimPower) * saturate(dot( vectEyeDir, -vectLightDir));
	return frimill * colLight * colSubSurface;
}

float3 PointLightDiffuse(
	float3 vectLight,
	float afRadius,
	float3 vectNormal,
	float3 colLight)
{
	return DirectionalLightDiffuse(normalize(vectLight), vectNormal, colLight);
}

float3 PointLightDiffuseTranslucent(
	float3 vectLight,
	float afRadius,
	float3 vectNormal,
	float3 colLight,
	float3 colMask)
{
	return DirectionalLightDiffuseTranslucent(normalize(vectLight), vectNormal, colLight, colMask);
}

// per pixel anisotropic lighting
float3 AnisoLight(
	float3 vEyeDir,
	float3 vLightDir,
	float3 vLightColor,
	float3 vNormal,
	float3 vVertexColor,
	float fSpecularPower,
	row_major float3x3 matCommonSpaceTransform)
{
	float2 anisocoord;
	float3 halfangle = normalize( vEyeDir + vLightDir );
	float3 upFromSurface = float3( 0, 0, 1 );
	vNormal = normalize( upFromSurface + vNormal * 0.5 );
	anisocoord.x = dot( vNormal, vLightDir );
	anisocoord.y = dot( vNormal, halfangle );
	float aniso = pow( 1 - saturate( abs( anisocoord.x - anisocoord.y ) ), fSpecularPower );

	float3 tint = 0;
	float ndotl = max( 0.0, dot( float3(0,0,1), vLightDir ) );

	return vLightColor * (tint + aniso * 0.7 ) * ndotl;
}

//////
//  Lighting Model

float3 OrenNayarDiffuse(
	float3 light,
	float3 view,
	float3 norm,
	float roughness )
{
	float3 l = light;
	float3 v = view;
	float3 n = norm;
	float vdotn = dot( v, n );
	float ldotn = dot( l, n );
	float gamma = dot( v - n * vdotn, l - n * ldotn );
	float rough_sq = roughness * roughness;
	float A = 1.0 - 0.5 * (rough_sq / (rough_sq + 0.57f) );
	float B = 0.45 * (rough_sq / (rough_sq + 0.09));
	float C = sqrt( (1.0 - vdotn*vdotn) * (1.0 - ldotn*ldotn)) / max( vdotn, ldotn );
	return max(0,dot(light,norm)) * (A + B * max( 0.0f, gamma) * C);
}

float3 FresnelSchlick(float3 sc, float3 e, float3 h)
{
	return sc + (1.0f - sc) * pow( 1.0f - saturate(dot(e,h)), 5);
}

///////
float4 PS(v2fIn IN) : COLOR 
{
	float4 colFragout = 0;
	float3 colDiffuse = 0;
  	float3 colSpecular = 0;
  	float3 colEnvmap = 0;
  	float3 colSubSurface = 0;

	if (!bTileU)
 		IN.UV0.x = saturate(IN.UV0.x);
 	if (!bTileV)
 		IN.UV0.y = saturate(IN.UV0.y);

	float3x3 matCommonSpaceTransform;
	matCommonSpaceTransform[0] = IN.CommonSpaceTransform0;
	matCommonSpaceTransform[1] = IN.CommonSpaceTransform1;
	matCommonSpaceTransform[2] = IN.CommonSpaceTransform2;

	float3 vectEye = mul( matCommonSpaceTransform, IN.WorldView );
	float3 vectEyeDir = normalize( vectEye );

    float3 diffContrib;
    float3 specContrib;
    float3 vectLight = mul( matCommonSpaceTransform, IN.LightVec );
	float4 vertColor = float4(IN.UV0.z,IN.UV0.a,IN.UV1.z,IN.UV1.a);

    float3 Tn = normalize(IN.CommonSpaceTransform0);
    float3 Bn = normalize(IN.CommonSpaceTransform1);
    float3 Nn = normalize(IN.CommonSpaceTransform2);

	float fatten = GetAttenuation(vectLight, 99999999999999.0f);

	//
	//Sample the textures
	//
	float4 colBaseTexture = tex2D(DiffuseSampler,IN.UV0.xy);
	float4 snormal = 0;
	float4 wnormal = 0;
	float3 colTranslucentMask=0;

	if(bGrayscaleToColor)
	{
		colBaseTexture.rgb = GetGrayscaleToPaletteColor(IN.UV0.xy, saturate(HairTintColor.r - (1-vertColor.r)));
		vertColor.rgb = 1;
	}

	if(bUseNormalMap && bNormalTexEnabled)
	{
	    snormal = tex2D(NormalSampler,IN.UV0);

	    if(!bModelSpaceNormals)
	    {
			snormal.xy = 2.0 * snormal.xy - 1.0f;
			snormal.z = sqrt(1-dot(snormal.xy,snormal.xy));
		}
		else
			snormal.xyz = 2.0 * snormal.xyz - 1.0f;
	}
	else
		snormal = float4( 0.0f, 0.0f, 1.0f, 0.0f );

	if( bWrinkleTexEnabled )
	{
		wnormal = tex2D(WrinkleSampler,IN.UV0);

	    if(!bModelSpaceNormals)
	    {
			wnormal.xy = 2.0 * wnormal.xy - 1.0f;
			wnormal.z = sqrt(1-dot(wnormal.xy,wnormal.xy));
		}
		else
			wnormal.xyz = 2.0 * wnormal.xyz - 1.0f;

		snormal.xyz = lerp( snormal.xyz, wnormal.xyz, IN.Weight );
	}

	//backlighting mask
	if (bBackLightingEnabled && !bModelSpaceNormals)
	{
		colTranslucentMask = tex2D(BackLightMaskSampler,IN.UV0.xy);
	}

	if( bSubSurfaceLightingEnabled || bRimLightingEnabled )
	{
		colSubSurface = tex2D( SubSurfaceSampler, IN.UV0.xy );
	}

	//
	//Get the normal in the proper space
	//
	float3 vectNormal = snormal.xyz;
	if (bModelSpaceNormals)
		vectNormal = vectNormal.xzy; //modelspace normalmaps from zmapper have their y and z coordinates flipped

	float2 smoothspecmask = tex2D(SmoothSpecMaskSampler,IN.UV0.xy);
	float fSpecularMask = smoothspecmask.r * fSpecularScale;
	// scale, then convert to fullrange
	float smoothness = fSmoothness * smoothspecmask.g;
	float fSpecularPower = exp2(smoothness*10 + 1);
	float roughness = 1.0f - smoothness;

	if(g_UseParallax && bNormalTexEnabled)
	{
		float height = tex2D(NormalSampler, IN.UV0).a;
		float Altitude = height + g_ParallaxBias; 
		float3 normalUV  = Altitude * g_ParallaxScale*vectEye;
		normalUV +=  IN.UV0;		
		IN.UV0.xyz = normalUV;
	}	

	//
	//Lighting
	//
	float3 nvectlight = normalize(vectLight.xyz);
	float flambertdiffuse = saturate(dot(vectNormal.xyz,nvectlight.xyz));
	colDiffuse = OrenNayarDiffuse( nvectlight, vectEyeDir, vectNormal, roughness ) * Lamp0Color.rgb;

	colSpecular = BRDF_ts_nphong(vectNormal.xyz, nvectlight.xyz, vectEyeDir.xyz, 1.0, fSpecularMask, fSpecularPower) * flambertdiffuse * Lamp0Color.rgb;	

	//EnvMapping	
	if (bEnvmapEnabled && bEnvTexEnabled)
	{
		float3 refl = 2 * dot( vectNormal, vectEyeDir ) * vectNormal - vectEyeDir;
		colEnvmap = texCUBE( EnvSampler, refl.xyz ) * fSpecularMask;
	}

	//Glow Map
	float3 colGlow = EmitColor.rgb * fEmitScale;
	if( bGlowTexEnabled )
		colGlow *= tex2D(GlowSampler, IN.UV0);

	//
	//Combine the various components
	//
	// NOTE: 3DS2013 w/ Nitrous is negating the green channel of the vertex color,
	// so just take the absolute value of the entire thing to get correct vertex color...
	float4 colVertex = abs(vertColor);
	if (bShowAmbient)
		colFragout.rgb = ambientColor.rgb;
	if (bShowEmit)
	 	colFragout.rgb += colGlow;
	if (bShowDiffuse)
		colFragout.rgb += colDiffuse.rgb;
	float3 colLight = colFragout.rgb;
	if (bShowTexture)
		if (bShowAmbient || bShowDiffuse)
			colFragout.rgb *= colBaseTexture.rgb;
		else
			colFragout.rgb = colBaseTexture.rgb;
	if (bVertexColorsEnabled)
	{
		if (bShowAmbient || bShowDiffuse || bShowTexture)
			colFragout.rgb*= colVertex.rgb;
		else
			colFragout.rgb = colVertex.rgb;
	}
	if (bSpecularEnabled)
		colFragout.rgb += colSpecular.rgb * SpecularColor;
	if (bEnvmapEnabled)
		colFragout.rgb += colEnvmap;// * colLight;

	float alpha = 0.0f;
	if( bShowAlpha )
	{
		alpha = MatAlpha;
		if (bShowTexture)
			alpha *= colBaseTexture.a;
		if (bVertexColorsEnabled)
			alpha *= colVertex.a;

		if( g_AlphaTest )
			clip(alpha < g_AlphaRef/255.0f ? -1:1);
	}

	if( g_AlphaStandard || g_AlphaAdd || g_AlphaMult )
		colFragout.a = alpha;

	if( !g_bTwoSided )
		clip( IN.vface < 0.0f ? -1:1 );

	if( bWrinkleTexEnabled )
	{
		// Darken the diffuse color by an ambient occlusion map
		// leaving the red channel a bit brighter to simulate
		// subsurface scattering of light on the skin layer...
		float fambocc = (1-wnormal.a) * IN.Weight * 0.3f;
		float fdarken_r = 1-(fambocc*0.75f);
		float fdarken_gb = 1-fambocc;
		colFragout.rgb = float3( colFragout.r * fdarken_r, colFragout.gb * fdarken_gb );
	}

	return colFragout;
}

///// TECHNIQUES /////////////////////////////

technique Main
{
    pass p0
	{
		ZEnable = bZTestEnabled;
		ZWriteEnable = bZWriteEnabled;
		AlphaBlendEnable = bShowAlpha && (g_AlphaStandard || g_AlphaAdd || g_AlphaMult);
		SrcBlend = g_AlphaStandard ? 5: g_AlphaAdd ? 2 : 9;
		DestBlend = g_AlphaStandard ? 6: g_AlphaAdd ? 2 : 1;
		CullMode = NONE;
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS();
    }
}

/////////////////////////////////////// eof //
