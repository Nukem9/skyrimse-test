#pragma once

#include "NiTransform.h"
#include "NiObjectNET.h"

class NiAVObject : public NiObjectNET
{
public:
	virtual ~NiAVObject();

	char _pad0[0x4C];
	NiTransform m_kWorld;
	char _pad1[0x60];

	inline const NiTransform& GetWorldTransform() const
	{
		return m_kWorld;
	}
};
static_assert(sizeof(NiAVObject) == 0x110);
static_assert(offsetof(NiAVObject, m_kWorld) == 0x7C);