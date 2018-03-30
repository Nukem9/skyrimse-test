#pragma once

#include "../../NiMain/NiColor.h"
#include "../BSShaderProperty.h"

class BSSkyShaderProperty : public BSShaderProperty
{
public:
	NiColorA kBlendColor;
	NiSourceTexture *pBaseTexture;
	NiSourceTexture *pBlendTexture;
	char _pad0[0x10];
	float fBlendValue;
	uint16_t usUnknown;
	bool bUnknown;
	uint32_t uiSkyObjectType;
};
static_assert(sizeof(BSSkyShaderProperty) == 0xC8);
static_assert_offset(BSSkyShaderProperty, kBlendColor, 0x88);
static_assert_offset(BSSkyShaderProperty, pBaseTexture, 0x98);
static_assert_offset(BSSkyShaderProperty, pBlendTexture, 0xA0);
static_assert_offset(BSSkyShaderProperty, fBlendValue, 0xB8);
static_assert_offset(BSSkyShaderProperty, usUnknown, 0xBC);
static_assert_offset(BSSkyShaderProperty, bUnknown, 0xBE);
static_assert_offset(BSSkyShaderProperty, uiSkyObjectType, 0xC0);