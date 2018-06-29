#pragma once

#include "../BSSpinLock.h"
#include "../BSGraphicsTypes.h"
#include "BSTriShape.h"

class BSDynamicTriShape : public BSTriShape
{
public:
	virtual ~BSDynamicTriShape();

	void *pDynamicData;
	BSSpinLock DynamicDataAccessSpinLock;
	uint32_t DynamicDataSize;
	uint32_t uiFrameCount;
	BSGraphics::DynamicTriShapeDrawData DrawData;
	char _pad0[0x4];

	void *LockDynamicData()
	{
		DynamicDataAccessSpinLock.Acquire();
		return pDynamicData;
	}

	const void *LockDynamicDataForRead()
	{
		return LockDynamicData();
	}

	void UnlockDynamicData()
	{
		DynamicDataAccessSpinLock.Release();
	}

	uint32_t QDynamicDataSize() const
	{
		return DynamicDataSize;
	}
};
static_assert(sizeof(BSDynamicTriShape) == 0x180);
static_assert_offset(BSDynamicTriShape, pDynamicData, 0x160);
static_assert_offset(BSDynamicTriShape, DynamicDataAccessSpinLock, 0x168);
static_assert_offset(BSDynamicTriShape, DynamicDataSize, 0x170);
static_assert_offset(BSDynamicTriShape, uiFrameCount, 0x174);
static_assert_offset(BSDynamicTriShape, DrawData, 0x178);