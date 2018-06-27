#pragma once

#include "NiTransform.h"
#include "NiObjectNET.h"

struct NiBound
{
	NiPoint3 m_kCenter;
	union
	{
		float m_fRadius;
		int m_iRadiusAsInt;
	};
};

class NiAVObject : public NiObjectNET
{
public:
	virtual ~NiAVObject();

	char _pad0[0x4C];
	NiTransform m_kWorld;
	NiTransform m_kPreviousWorld;
	NiBound m_kWorldBound;
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

	bool QAppCulled() const
	{
		return (m_uFlags & 1) != 0;
	}

	bool QAlwaysDraw() const
	{
		return (m_uFlags & (1 << 11)) != 0;
	}

	bool QPreProcessedNode() const
	{
		return (m_uFlags & (1 << 12)) != 0;
	}

	void SetAppCulled(bool Culled)
	{
		if (Culled)
			m_uFlags |= 1;
		else
			m_uFlags &= ~1;
	}

	int IsVisualObjectI() const
	{
		return m_kWorldBound.m_iRadiusAsInt;
	}
};
static_assert(sizeof(NiAVObject) == 0x110);
static_assert(offsetof(NiAVObject, m_kWorld) == 0x7C);
static_assert(offsetof(NiAVObject, m_kPreviousWorld) == 0xB0);
static_assert(offsetof(NiAVObject, m_kWorldBound) == 0xE4);
static_assert(offsetof(NiAVObject, m_uFlags) == 0xF4);