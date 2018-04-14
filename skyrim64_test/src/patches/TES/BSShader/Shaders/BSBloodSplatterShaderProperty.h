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
	char _pad[0x8];
	void *pAlphaSource;

	float GetAlpha() const
	{
		AssertMsgDebug(pAlphaSource, "BSBloodSplatterShaderProperty::GetAlpha() is missing an alpha source.");

		return **(float **)&pAlphaSource * BSShaderProperty::GetAlpha();
	}
};
static_assert(sizeof(BSBloodSplatterShaderProperty) == 0xB0);
static_assert_offset(BSBloodSplatterShaderProperty, pBloodColorTexture, 0x88);
static_assert_offset(BSBloodSplatterShaderProperty, pBloodAlphaTexture, 0x90);
static_assert_offset(BSBloodSplatterShaderProperty, pFlareColorTexture, 0x98);
static_assert_offset(BSBloodSplatterShaderProperty, pAlphaSource, 0xA8);