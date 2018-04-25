#pragma once

#include "../BSSpinLock.h"
#include "BSTriShape.h"

class BSDynamicTriShape : public BSTriShape
{
public:
	virtual ~BSDynamicTriShape();

	void *pDynamicData;
	BSSpinLock DynamicDataAccessSpinLock;
	char _pad0[0x10];

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
};
static_assert(sizeof(BSDynamicTriShape) == 0x180);
static_assert_offset(BSDynamicTriShape, pDynamicData, 0x160);
static_assert_offset(BSDynamicTriShape, DynamicDataAccessSpinLock, 0x168);