#pragma once

#include "BSShaderMaterial.h"

class BSShaderProperty
{
private:
	static const uint32_t UniqueMaterialFlags[15];
	static const char *UniqueMaterialNames[15];
	static const char *MaterialBitNames[64];

public:
	static void GetMaterialString(uint64_t Flags, char *Buffer, size_t BufferSize);

	char _pad0[0x30];
	float fAlpha;// Might be part of a parent class
	char _pad1[0x4];// int iLastRenderPassState?
	uint64_t ulFlags;
	char _pad2[0x20];
	class BSFadeNode *pFadeNode;
	char _pad3[0x10];
	BSShaderMaterial *pMaterial;
	char _pad4[0x8];

	float GetAlpha()
	{
		return fAlpha;
	}

	class BSFadeNode *QFadeNode()
	{
		return pFadeNode;
	}

	uint64_t QFlags()
	{
		return ulFlags;
	}
};
static_assert(sizeof(BSShaderProperty) == 0x88);
static_assert_offset(BSShaderProperty, fAlpha, 0x30);
static_assert_offset(BSShaderProperty, ulFlags, 0x38);
static_assert_offset(BSShaderProperty, pFadeNode, 0x60);
static_assert_offset(BSShaderProperty, pMaterial, 0x78);