#pragma once

#include "../../NiMain/NiColor.h"
#include "../../NiMain/NiSourceTexture.h"
#include "../BSShaderProperty.h"

class BSBloodSplatterShaderProperty : public BSShaderProperty
{
public:
	NiSourceTexture *pBloodColorTexture;
	NiSourceTexture *pBloodAlphaTexture;
	NiSourceTexture *pFlareColorTexture;
	uint32_t eTextureClampMode;
	float *pfAlphaSource;

	float GetAlpha() const
	{
		AssertMsgDebug(pfAlphaSource, "BSBloodSplatterShaderProperty::GetAlpha() is missing an alpha source.");

		return *pfAlphaSource * BSShaderProperty::GetAlpha();
	}
};
static_assert(sizeof(BSBloodSplatterShaderProperty) == 0xB0);
static_assert_offset(BSBloodSplatterShaderProperty, pBloodColorTexture, 0x88);
static_assert_offset(BSBloodSplatterShaderProperty, pBloodAlphaTexture, 0x90);
static_assert_offset(BSBloodSplatterShaderProperty, pFlareColorTexture, 0x98);
static_assert_offset(BSBloodSplatterShaderProperty, eTextureClampMode, 0xA0);
static_assert_offset(BSBloodSplatterShaderProperty, pfAlphaSource, 0xA8);