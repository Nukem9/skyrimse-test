#pragma once

#include "../BSShaderMaterial.h"

class BSEffectShaderMaterial : public BSShaderMaterial
{
public:
	virtual ~BSEffectShaderMaterial();
	virtual BSShaderMaterial *CreateNew() override;
	virtual void CopyMembers(BSShaderMaterial *Other) override;
	virtual bool DoIsCopy(const BSShaderMaterial *Other) override;
	virtual uint32_t ComputeCRC32(uint32_t Unknown) override;
	virtual BSShaderMaterial *GetDefault() override;
	// virtual uint32_t GetUnknown1(); -- Not an override
	virtual uint32_t GetUnknown2() override;
	// virtual void Print() override; -- Creation kit only

	float fFalloffStartAngle;
	float fFalloffStopAngle;
	float fFalloffStartOpacity;
	float fFalloffStopOpacity;
	NiColorA kBaseColor;
	char _pad[0x10];
	float fSoftFalloffDepth;
	float fBaseColorScale;
	NiPointer<NiSourceTexture> spTexture;
	NiPointer<NiSourceTexture> spGrayscaleTexture;
	uint8_t eEffectClampMode;
	uint8_t eUnknown;
};
static_assert(sizeof(BSEffectShaderMaterial) == 0x88);
static_assert_offset(BSEffectShaderMaterial, fFalloffStartAngle, 0x38);
static_assert_offset(BSEffectShaderMaterial, fFalloffStopAngle, 0x3C);
static_assert_offset(BSEffectShaderMaterial, fFalloffStartOpacity, 0x40);
static_assert_offset(BSEffectShaderMaterial, fFalloffStopOpacity, 0x44);
static_assert_offset(BSEffectShaderMaterial, kBaseColor, 0x48);
static_assert_offset(BSEffectShaderMaterial, fSoftFalloffDepth, 0x68);
static_assert_offset(BSEffectShaderMaterial, fBaseColorScale, 0x6C);
static_assert_offset(BSEffectShaderMaterial, spTexture, 0x70);
static_assert_offset(BSEffectShaderMaterial, spGrayscaleTexture, 0x78);
static_assert_offset(BSEffectShaderMaterial, eEffectClampMode, 0x80);
static_assert_offset(BSEffectShaderMaterial, eUnknown, 0x81);