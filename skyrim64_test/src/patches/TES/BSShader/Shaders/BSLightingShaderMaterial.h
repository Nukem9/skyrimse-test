#pragma once

#include "../BSShaderMaterial.h"

class BSLightingShaderMaterialBase : public BSShaderMaterial
{
public:
	virtual ~BSLightingShaderMaterialBase();

	NiColor kSpecularColor;
	NiPointer<NiSourceTexture> spDiffuseTexture;
	int iDiffuseRenderTargetSourceIndex;
	NiPointer<NiSourceTexture> spNormalTexture;
	NiPointer<NiSourceTexture> spRimSoftLightingTexture;		// Rim lighting OR soft lighting
	NiPointer<NiSourceTexture> spSpecularBackLightingTexture;	// Specular OR back lighting
	int eTextureClampMode;										// BSGraphics::TextureAddressMode
	void *TextureSet;											// BSTextureSet @ 0x78
	float fMaterialAlpha;
	float fRefractionPower;
	float fSpecularPower;
	float fSpecularColorScale;
	float fSubSurfaceLightRolloff;
	float fRimLightPower;
	char _pad1[0x8];
};
static_assert(sizeof(BSLightingShaderMaterialBase) == 0xA0);
static_assert_offset(BSLightingShaderMaterialBase, kSpecularColor, 0x38);
static_assert_offset(BSLightingShaderMaterialBase, spDiffuseTexture, 0x48);
static_assert_offset(BSLightingShaderMaterialBase, iDiffuseRenderTargetSourceIndex, 0x50);
static_assert_offset(BSLightingShaderMaterialBase, spNormalTexture, 0x58);
static_assert_offset(BSLightingShaderMaterialBase, spRimSoftLightingTexture, 0x60);
static_assert_offset(BSLightingShaderMaterialBase, spSpecularBackLightingTexture, 0x68);
static_assert_offset(BSLightingShaderMaterialBase, eTextureClampMode, 0x70);
static_assert_offset(BSLightingShaderMaterialBase, fMaterialAlpha, 0x80);
static_assert_offset(BSLightingShaderMaterialBase, fRefractionPower, 0x84);
static_assert_offset(BSLightingShaderMaterialBase, fSpecularPower, 0x88);
static_assert_offset(BSLightingShaderMaterialBase, fSpecularColorScale, 0x8C);
static_assert_offset(BSLightingShaderMaterialBase, fSubSurfaceLightRolloff, 0x90);
static_assert_offset(BSLightingShaderMaterialBase, fRimLightPower, 0x94);

class BSLightingShaderMaterial : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterial();
};
static_assert(sizeof(BSLightingShaderMaterial) == 0xA0);

class BSLightingShaderMaterialEnvmap : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialEnvmap();

	NiPointer<NiSourceTexture> spEnvTexture;
	NiPointer<NiSourceTexture> spEnvMaskTexture;
	float fEnvmapScale;
};
static_assert(sizeof(BSLightingShaderMaterialEnvmap) == 0xB8);
static_assert_offset(BSLightingShaderMaterialEnvmap, spEnvTexture, 0xA0);
static_assert_offset(BSLightingShaderMaterialEnvmap, spEnvMaskTexture, 0xA8);
static_assert_offset(BSLightingShaderMaterialEnvmap, fEnvmapScale, 0xB0);

class BSLightingShaderMaterialGlowmap : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialGlowmap();

	NiPointer<NiSourceTexture> spGlowTexture;
};
static_assert(sizeof(BSLightingShaderMaterialGlowmap) == 0xA8);
static_assert_offset(BSLightingShaderMaterialGlowmap, spGlowTexture, 0xA0);

class BSLightingShaderMaterialParallax : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialParallax();

	NiPointer<NiSourceTexture> spHeightTexture;
};
static_assert(sizeof(BSLightingShaderMaterialParallax) == 0xA8);
static_assert_offset(BSLightingShaderMaterialParallax, spHeightTexture, 0xA0);

class BSLightingShaderMaterialFacegen : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialFacegen();

	NiPointer<NiSourceTexture> spTintTexture;
	NiPointer<NiSourceTexture> spDetailTexture;
	NiPointer<NiSourceTexture> spSubsurfaceTexture;
};
static_assert(sizeof(BSLightingShaderMaterialFacegen) == 0xB8);
static_assert_offset(BSLightingShaderMaterialFacegen, spTintTexture, 0xA0);
static_assert_offset(BSLightingShaderMaterialFacegen, spDetailTexture, 0xA8);
static_assert_offset(BSLightingShaderMaterialFacegen, spSubsurfaceTexture, 0xB0);

class BSLightingShaderMaterialFacegenTint : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialFacegenTint();

	NiColor kTintColor;
};
static_assert(sizeof(BSLightingShaderMaterialFacegenTint) == 0xB0);
static_assert_offset(BSLightingShaderMaterialFacegenTint, kTintColor, 0xA0);

class BSLightingShaderMaterialHairTint : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialHairTint();

	NiColor kHairTintColor;
};
static_assert(sizeof(BSLightingShaderMaterialHairTint) == 0xB0);
static_assert_offset(BSLightingShaderMaterialHairTint, kHairTintColor, 0xA0);

class BSLightingShaderMaterialParallaxOcc : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialParallaxOcc();

	NiPointer<NiSourceTexture> spHeightTexture;
	float fParallaxOccMaxPasses;
	float fParallaxOccScale;
};
static_assert(sizeof(BSLightingShaderMaterialParallaxOcc) == 0xB0);
static_assert_offset(BSLightingShaderMaterialParallaxOcc, spHeightTexture, 0xA0);
static_assert_offset(BSLightingShaderMaterialParallaxOcc, fParallaxOccMaxPasses, 0xA8);
static_assert_offset(BSLightingShaderMaterialParallaxOcc, fParallaxOccScale, 0xAC);

class BSLightingShaderMaterialLandscape : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialLandscape();

	uint32_t uiNumLandscapeTextures;
	NiPointer<NiSourceTexture> spLandscapeDiffuseTexture[5];
	NiPointer<NiSourceTexture> spLandscapeNormalTexture[5];
	NiPointer<NiSourceTexture> spUnknownTexture1;
	NiPointer<NiSourceTexture> spUnknownTexture2;
	NiColorA kLandBlendParams;
	float fTextureIsSnow[6];
	float fTextureIsSpecPower[6];
	float fTerrainTexOffsetX;
	float fTerrainTexOffsetY;
	float fTerrainTexFade;
};
static_assert(sizeof(BSLightingShaderMaterialLandscape) == 0x158);
static_assert_offset(BSLightingShaderMaterialLandscape, uiNumLandscapeTextures, 0xA0);
static_assert_offset(BSLightingShaderMaterialLandscape, spLandscapeDiffuseTexture, 0xA8);
static_assert_offset(BSLightingShaderMaterialLandscape, spLandscapeNormalTexture, 0xD0);
static_assert_offset(BSLightingShaderMaterialLandscape, spUnknownTexture1, 0xF8);
static_assert_offset(BSLightingShaderMaterialLandscape, spUnknownTexture2, 0x100);
static_assert_offset(BSLightingShaderMaterialLandscape, kLandBlendParams, 0x108);
static_assert_offset(BSLightingShaderMaterialLandscape, fTextureIsSnow, 0x118);
static_assert_offset(BSLightingShaderMaterialLandscape, fTextureIsSpecPower, 0x130);
static_assert_offset(BSLightingShaderMaterialLandscape, fTerrainTexOffsetX, 0x148);
static_assert_offset(BSLightingShaderMaterialLandscape, fTerrainTexOffsetY, 0x14C);
static_assert_offset(BSLightingShaderMaterialLandscape, fTerrainTexFade, 0x150);

class BSLightingShaderMaterialLODLandscape : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialLODLandscape();

	NiPointer<NiSourceTexture> spParentDiffuseTexture;	// Never read
	NiPointer<NiSourceTexture> spParentNormalTexture;	// Never read
	NiPointer<NiSourceTexture> spLandscapeNoiseTexture;
	float fTerrainTexOffsetX;
	float fTerrainTexOffsetY;
	float fTerrainTexFade;
};
static_assert(sizeof(BSLightingShaderMaterialLODLandscape) == 0xC8);
static_assert_offset(BSLightingShaderMaterialLODLandscape, spParentDiffuseTexture, 0xA0);
static_assert_offset(BSLightingShaderMaterialLODLandscape, spParentNormalTexture, 0xA8);
static_assert_offset(BSLightingShaderMaterialLODLandscape, spLandscapeNoiseTexture, 0xB0);
static_assert_offset(BSLightingShaderMaterialLODLandscape, fTerrainTexOffsetX, 0xB8);
static_assert_offset(BSLightingShaderMaterialLODLandscape, fTerrainTexOffsetY, 0xBC);
static_assert_offset(BSLightingShaderMaterialLODLandscape, fTerrainTexFade, 0xC0);

class BSLightingShaderMaterialMultiLayerParallax : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialMultiLayerParallax();

	NiPointer<NiSourceTexture> spLayerTexture;
	NiPointer<NiSourceTexture> spEnvTexture;
	NiPointer<NiSourceTexture> spEnvMaskTexture;
	float fParallaxLayerThickness;
	float fParallaxRefractionScale;
	float fParallaxInnerLayerUScale;
	float fParallaxInnerLayerVScale;
	float fEnvmapScale;
};
static_assert(sizeof(BSLightingShaderMaterialMultiLayerParallax) == 0xD0);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, spLayerTexture, 0xA0);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, spEnvTexture, 0xA8);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, spEnvMaskTexture, 0xB0);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, fParallaxLayerThickness, 0xB8);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, fParallaxRefractionScale, 0xBC);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, fParallaxInnerLayerUScale, 0xC0);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, fParallaxInnerLayerVScale, 0xC4);
static_assert_offset(BSLightingShaderMaterialMultiLayerParallax, fEnvmapScale, 0xC8);

class BSLightingShaderMaterialSnow : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialSnow();

	NiColorA kSparkleParams;
};
static_assert(sizeof(BSLightingShaderMaterialSnow) == 0xB0);
static_assert_offset(BSLightingShaderMaterialSnow, kSparkleParams, 0xA0);

class BSLightingShaderMaterialEye : public BSLightingShaderMaterialBase
{
public:
	virtual ~BSLightingShaderMaterialEye();

	NiPointer<NiSourceTexture> spEnvTexture;
	NiPointer<NiSourceTexture> spEnvMaskTexture;
	float fEnvmapScale;
	NiPoint3 kEyeCenter[2];
};
static_assert(sizeof(BSLightingShaderMaterialEye) == 0xD0);
static_assert_offset(BSLightingShaderMaterialEye, spEnvTexture, 0xA0);
static_assert_offset(BSLightingShaderMaterialEye, spEnvMaskTexture, 0xA8);
static_assert_offset(BSLightingShaderMaterialEye, fEnvmapScale, 0xB0);
static_assert_offset(BSLightingShaderMaterialEye, kEyeCenter, 0xB4);