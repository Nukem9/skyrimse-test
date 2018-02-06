#pragma once

#include "../BSSpinLock.h"
#include "BSTriShape.h"

class BSDynamicTriShape : public BSTriShape
{
public:
	virtual ~BSDynamicTriShape();

	char _pad0[0x20];

	const void *LockDynamicDataForRead()
	{
		BSSpinLock *lock = (BSSpinLock *)((uintptr_t)this + 0x168);
		lock->Acquire();

		return *(void **)((uintptr_t)this + 0x160);
	}

	void UnlockDynamicData()
	{
		BSSpinLock *lock = (BSSpinLock *)((uintptr_t)this + 0x168);
		lock->Release();
	}
};
static_assert(sizeof(BSDynamicTriShape) == 0x180);