#pragma once

#include "NiAVObject.h"
#include "NiFrustum.h"

class NiCamera;
class NiVisibleArray;

class NiFrustumPlanes
{
public:
	char _pad[0x60];				// NiPlane m_akCullingPlanes[6];
	uint32_t m_uiActivePlanes;
	uint32_t m_uiBasePlaneStates;
	char _pad1[0x8];				// Intentional padding

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
static_assert(sizeof(NiFrustumPlanes) == 0x70);
static_assert_offset(NiFrustumPlanes, m_uiActivePlanes, 0x60);
static_assert_offset(NiFrustumPlanes, m_uiBasePlaneStates, 0x64);

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

	const bool m_bUseVirtualAppend;
	NiVisibleArray *m_pkVisibleSet;
	NiCamera *m_pkCamera;
	NiFrustum m_kFrustum;
	NiFrustumPlanes m_kPlanes;
	NiFrustumPlanes kCustomCullPlanes;
	bool m_bCameraRelatedUpdates;// Unverified
	bool bUpdateAccumulateFlag;
	bool bIgnorePreprocess;
	bool bCustomCullPlanes;// Unverified
	bool bUnknownBool1;// Unverified
	bool bUnknownBool2;// Unverified

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
static_assert(sizeof(NiCullingProcess) == 0x128);
static_assert_offset(NiCullingProcess, m_bUseVirtualAppend, 0x8);
static_assert_offset(NiCullingProcess, m_pkVisibleSet, 0x10);
static_assert_offset(NiCullingProcess, m_pkCamera, 0x18);
static_assert_offset(NiCullingProcess, m_kFrustum, 0x20);
static_assert_offset(NiCullingProcess, m_kPlanes, 0x3C);
static_assert_offset(NiCullingProcess, kCustomCullPlanes, 0xAC);
static_assert_offset(NiCullingProcess, m_bCameraRelatedUpdates, 0x11C);
static_assert_offset(NiCullingProcess, bUpdateAccumulateFlag, 0x11D);
static_assert_offset(NiCullingProcess, bIgnorePreprocess, 0x11E);
static_assert_offset(NiCullingProcess, bCustomCullPlanes, 0x11F);
static_assert_offset(NiCullingProcess, bUnknownBool1, 0x120);
static_assert_offset(NiCullingProcess, bUnknownBool2, 0x121);

STATIC_CONSTRUCTOR(CheckNiCullingProcess, []
{
	assert_vtable_index(&NiCullingProcess::IsBhkLimitedHingeConstraint, 20);
	//assert_vtable_index(&NiCullingProcess::~NiCullingProcess, 21);
	assert_vtable_index(&NiCullingProcess::AppendVirtual, 24);
});