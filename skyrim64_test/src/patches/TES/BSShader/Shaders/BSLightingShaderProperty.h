#pragma once

#include "../../NiMain/NiColor.h"
#include "../BSShaderProperty.h"

class BSLightingShaderProperty : public BSShaderProperty
{
public:
	char _pad0[0x68];
	NiColor *pEmitColor;
	float fEmitColorScale;
	char _pad1[0x4];
	float fSpecularLODFade;
	float fEnvmapLODFade;
	char _pad2[0x14];
	NiColorA kProjectedUVColor;
	char _pad3[0x34];

	const NiColorA& QProjectedUVColor() const
	{
		return kProjectedUVColor;
	}
};
static_assert(sizeof(BSLightingShaderProperty) == 0x160);
static_assert_offset(BSLightingShaderProperty, pEmitColor, 0xF0);
static_assert_offset(BSLightingShaderProperty, fEmitColorScale, 0xF8);
static_assert_offset(BSLightingShaderProperty, fSpecularLODFade, 0x100);
static_assert_offset(BSLightingShaderProperty, fEnvmapLODFade, 0x104);
static_assert_offset(BSLightingShaderProperty, kProjectedUVColor, 0x11C);