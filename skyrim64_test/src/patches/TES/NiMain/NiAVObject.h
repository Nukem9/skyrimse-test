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
	virtual void UpdateControllers();
	virtual void PerformOp();
	virtual void Unk0();
	virtual void Unk1();
	virtual void Unk2();
	virtual void Unk3();
	virtual void SetSelectiveUpdateFlags();
	virtual void UpdateDownwardPass();
	virtual void UpdateSelectedDownwardPass();
	virtual void UpdateRigidDownwardPass();
	virtual void UpdateWorldBound();
	virtual void UpdateWorldData();
	virtual void Unk4();
	virtual void Unk5();
	virtual void Unk6();
	virtual void OnVisible(class NiCullingProcess *Process, uint32_t Unknown);

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

	bool QNotVisible() const
	{
		return (m_uFlags & (1 << 20)) != 0;
	}

	void SetAppCulled(bool Culled)
	{
		if (Culled)
			m_uFlags |= 1;
		else
			m_uFlags &= ~1;
	}

	void SetAccumulated(bool Accumulated)
	{
		if (Accumulated)
			m_uFlags |= 0x4000000;
		else
			m_uFlags &= ~0x4000000;
	}

	int IsVisualObjectI() const
	{
		return m_kWorldBound.m_iRadiusAsInt;
	}

	void GetViewerStrings(void(*Callback)(const char *, ...), bool Recursive) const
	{
		if (Recursive)
			__super::GetViewerStrings(Callback, Recursive);

		Callback("-- NiAVObject --\n");
		Callback("Flags = 0x%X\n", m_uFlags);
		Callback("App Culled = %s\n", QAppCulled() ? "true" : "false");
		Callback("World Translate = (%g, %g, %g)\n",
			m_kWorld.m_Translate.x,
			m_kWorld.m_Translate.y,
			m_kWorld.m_Translate.z);
		Callback("World Bound = (%g, %g, %g) %g\n",
			m_kWorldBound.m_kCenter.x,
			m_kWorldBound.m_kCenter.y,
			m_kWorldBound.m_kCenter.z,
			m_kWorldBound.m_fRadius);
	}
};
static_assert(sizeof(NiAVObject) == 0x110);
static_assert_offset(NiAVObject, m_kWorld, 0x7C);
static_assert_offset(NiAVObject, m_kPreviousWorld, 0xB0);
static_assert_offset(NiAVObject, m_kWorldBound, 0xE4);
static_assert_offset(NiAVObject, m_uFlags, 0xF4);

STATIC_CONSTRUCTOR(CheckNiAVObject, []
{
	//assert_vtable_index(&NiAVObject::~NiAVObject, 0);
	assert_vtable_index(&NiAVObject::OnVisible, 52);
});