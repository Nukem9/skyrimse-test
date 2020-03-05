#pragma once

#include "../BSGraphics/BSGraphicsTypes.h"
#include "BSTriShape.h"
#include "NiColor.h"

namespace BSGraphics
{
	struct IndexBuffer;
}

class BSMultiIndexTriShape : public BSTriShape
{
public:
	virtual ~BSMultiIndexTriShape();

	BSGraphics::IndexBuffer *pAltIndexBuffer;
	uint32_t uiAltPrimCount;
	float MaterialProjection[4][4];
	NiPointer<BSShaderProperty> spAdditionalShaderProperty;
	bool bUseAdditionalTriList;
	NiColorA fMaterialParams;
	float fMaterialScale;
	float fNormalDampener;
};
static_assert(sizeof(BSMultiIndexTriShape) == 0x1D8);
static_assert_offset(BSMultiIndexTriShape, pAltIndexBuffer, 0x160);
static_assert_offset(BSMultiIndexTriShape, uiAltPrimCount, 0x168);
static_assert_offset(BSMultiIndexTriShape, MaterialProjection, 0x16C);
static_assert_offset(BSMultiIndexTriShape, spAdditionalShaderProperty, 0x1B0);
static_assert_offset(BSMultiIndexTriShape, bUseAdditionalTriList, 0x1B8);
static_assert_offset(BSMultiIndexTriShape, fMaterialParams, 0x1BC);
static_assert_offset(BSMultiIndexTriShape, fMaterialScale, 0x1CC);
static_assert_offset(BSMultiIndexTriShape, fNormalDampener, 0x1D0);