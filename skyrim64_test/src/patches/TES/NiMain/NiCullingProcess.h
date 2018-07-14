#pragma once

#include "NiAVObject.h"

class NiCamera;

class NiFrustumPlanes
{
public:
	char _pad[0x60];
	uint32_t m_uiActivePlanes;

	uint32_t GetActivePlaneState() const
	{
		return m_uiActivePlanes;
	}

	void SetActivePlaneState(uint32_t State)
	{
		m_uiActivePlanes = State;
	}

	bool IsAnyPlaneActive() const
	{
		return m_uiActivePlanes != 0;
	}
};
static_assert(sizeof(NiFrustumPlanes) == 0x64);
static_assert_offset(NiFrustumPlanes, m_uiActivePlanes, 0x60);

class BSCompoundFrustum
{
public:
	char _pad0[0xBC];
	uint32_t dwordBC;
	char _pad1[0x4];
	bool byteC4;

	void GetActivePlaneState(int *__restrict States) const
	{
		((void(*)(const BSCompoundFrustum *, int *))(g_ModuleBase + 0xD5B110))(this, States);
	}

	void SetActivePlaneState(int *States)
	{
		((void(*)(BSCompoundFrustum *, int *))(g_ModuleBase + 0xD5B160))(this, States);
	}

	bool Process(NiAVObject *Object)
	{
		return ((bool(*)(BSCompoundFrustum *, NiAVObject *))(g_ModuleBase + 0xD59940))(this, Object);
	}
};
static_assert_offset(BSCompoundFrustum, dwordBC, 0xBC);
static_assert_offset(BSCompoundFrustum, byteC4, 0xC4);

class NiCullingProcess
{
public:
	// Yes, they really did copy these by hand. Yes, the hierarchy is fucked.
	virtual const NiRTTI *GetRTTI() const;

	virtual const NiNode						*IsNode() const;
	virtual class NiSwitchNode					*IsSwitchNode() const;
	virtual class BSFadeNode					*IsFadeNode() const;
	virtual class BSMultiBoundNode				*IsMultiBoundNode() const;
	virtual class BSGeometry					*IsGeometry() const;
	virtual class NiTriStrips					*IsTriStrips() const;
	virtual class BSTriShape					*IsTriShape() const;
	virtual class BSSegmentedTriShape			*IsSegmentedTriShape() const;
	virtual class BSSubIndexTriShape			*IsSubIndexTriShape() const;
	virtual class BSDynamicTriShape				*IsDynamicTriShape() const;
	virtual class NiGeometry					*IsNiGeometry() const;
	virtual class NiTriBasedGeom				*IsNiTriBasedGeom() const;
	virtual class NiTriShape					*IsNiTriShape() const;
	virtual class NiParticles					*IsParticlesGeom() const;
	virtual void								*IsLinesGeom() const;
	virtual class bhkNiCollisionObject			*IsBhkNiCollisionObject() const;
	virtual class bhkBlendCollisionObject		*IsBhkBlendCollisionObject() const;
	virtual void								*IsUnknown1() const;
	virtual class bhkRigidBody					*IsBhkRigidBody() const;
	virtual class bhkLimitedHingeConstraint		*IsBhkLimitedHingeConstraint() const;

	virtual ~NiCullingProcess();
	virtual void Process(NiAVObject *Object, uint32_t Unknown);
	virtual void Process(const NiCamera *Camera, NiAVObject *Object, class NiVisibleArray *Array);// nullsub
	virtual void AppendVirtual(BSGeometry *Geometry, uint32_t Unknown);// Calls debugbreak

	char _pad0[0x34];
	NiFrustumPlanes m_kPlanes;
	char _pad1[0x7D];
	bool bUpdateAccumulateFlag;
	bool bIgnorePreprocess;
	char _pad2[0x30078];
	int kCullMode;
	BSCompoundFrustum *pCompoundFrustum;
	char _pad3[0x2C];
	bool m_bDontUseVirtualAppend;

	void DoCulling(NiAVObject *Object, uint32_t Unknown)
	{
		((void(*)(NiCullingProcess *, NiAVObject *, uint32_t))(g_ModuleBase + 0xC78AA0))(this, Object, Unknown);
	}

	void SetAccumulated(NiAVObject *Object, bool Accumulated)
	{
		if (bUpdateAccumulateFlag)
			Object->SetAccumulated(Accumulated);
	}
};
static_assert_offset(NiCullingProcess, m_kPlanes, 0x3C);
static_assert_offset(NiCullingProcess, bUpdateAccumulateFlag, 0x11D);
static_assert_offset(NiCullingProcess, bIgnorePreprocess, 0x11E);
static_assert_offset(NiCullingProcess, kCullMode, 0x30198);
static_assert_offset(NiCullingProcess, pCompoundFrustum, 0x301A0);
static_assert_offset(NiCullingProcess, m_bDontUseVirtualAppend, 0x301D4);

STATIC_CONSTRUCTOR(CheckNiCullingProcess, []
{
	assert_vtable_index(&NiCullingProcess::IsBhkLimitedHingeConstraint, 20);
	//assert_vtable_index(&NiCullingProcess::~NiCullingProcess, 21);
	assert_vtable_index(&NiCullingProcess::AppendVirtual, 24);
});