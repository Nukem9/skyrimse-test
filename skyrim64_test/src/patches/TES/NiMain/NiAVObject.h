#pragma once

#include "NiTransform.h"
#include "NiObjectNET.h"

class NiAVObject : public NiObjectNET
{
public:
	virtual ~NiAVObject();

	char _pad0[0x4C];
	NiTransform m_kWorld;
	NiTransform m_kPreviousWorld;
	char _pad1[0x2C];

	inline const NiTransform& GetWorldTransform() const
	{
		return m_kWorld;
	}

	inline const NiTransform& GetPreviousWorldTransform() const
	{
		return m_kPreviousWorld;
	}

	inline const NiPoint3& GetWorldTranslate() const
	{
		return m_kWorld.m_Translate;
	}
};
static_assert(sizeof(NiAVObject) == 0x110);
static_assert(offsetof(NiAVObject, m_kWorld) == 0x7C);
static_assert(offsetof(NiAVObject, m_kPreviousWorld) == 0xB0);