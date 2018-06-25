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
	char _pad1[0x10];
	uint32_t m_uFlags;
	char _pad2[0x18];

	const NiTransform& GetWorldTransform() const
	{
		return m_kWorld;
	}

	const NiTransform& GetPreviousWorldTransform() const
	{
		return m_kPreviousWorld;
	}

	const NiPoint3& GetWorldTranslate() const
	{
		return m_kWorld.m_Translate;
	}

	bool GetBit(uint32_t Bit) const
	{
		return (m_uFlags & Bit) != 0;
	}

	bool GetAppCulled() const
	{
		return GetBit(1);
	}

	void SetAppCulled(bool Culled)
	{
		if (Culled)
			m_uFlags |= 1;
		else
			m_uFlags &= ~1;
	}
};
static_assert(sizeof(NiAVObject) == 0x110);
static_assert(offsetof(NiAVObject, m_kWorld) == 0x7C);
static_assert(offsetof(NiAVObject, m_kPreviousWorld) == 0xB0);
static_assert(offsetof(NiAVObject, m_uFlags) == 0xF4);