#pragma once

#include "NiRefObject.h"

class NiNode;
class NiStream;
class NiCloningProcess;
class NiObjectGroup;

class NiRTTI
{
private:
	const char *m_pcName;
	NiRTTI *m_pkBaseRTTI;

public:
	const char *GetName() const
	{
		return m_pcName;
	}

	const NiRTTI *GetBaseRTTI() const
	{
		return m_pkBaseRTTI;
	}
};
static_assert(sizeof(NiRTTI) == 0x10);
//static_assert(offsetof(NiRTTI, m_pcName) == 0x0);
//static_assert(offsetof(NiRTTI, m_pkBaseRTTI) == 0x10);

class NiObject : public NiRefObject
{
public:
	virtual const NiRTTI *GetRTTI();

	virtual const NiNode						*IsNode();
	virtual class NiSwitchNode					*IsSwitchNode();
	virtual class BSFadeNode					*IsFadeNode();
	virtual class BSMultiBoundNode				*IsMultiBoundNode();
	virtual class BSGeometry					*IsGeometry();
	virtual class NiTriStrips					*IsTriStrips();
	virtual class BSTriShape					*IsTriShape();
	virtual class BSSegmentedTriShape			*IsSegmentedTriShape();
	virtual class BSSubIndexTriShape			*IsSubIndexTriShape();
	virtual class BSDynamicTriShape				*IsDynamicTriShape();
	virtual class NiGeometry					*IsNiGeometry();
	virtual class NiTriBasedGeom				*IsNiTriBasedGeom();
	virtual class NiTriShape					*IsNiTriShape();
	virtual class NiParticles					*IsParticlesGeom();
	virtual void								*IsLinesGeom();
	virtual class bhkNiCollisionObject			*IsBhkNiCollisionObject();
	virtual class bhkBlendCollisionObject		*IsBhkBlendCollisionObject();
	virtual void								*IsUnknown1();
	virtual class bhkRigidBody					*IsBhkRigidBody();
	virtual class bhkLimitedHingeConstraint		*IsBhkLimitedHingeConstraint();

	virtual NiObject *CreateClone(NiCloningProcess &);
	virtual void LoadBinary(NiStream &);
	virtual void LinkObject(NiStream &);
	virtual bool RegisterStreamables(NiStream &);
	virtual void SaveBinary(NiStream &);
	virtual bool IsEqual(NiObject *Other);
	virtual void ProcessClone(NiCloningProcess &);
	virtual void PostLinkObject(NiStream &);
	virtual bool StreamCanSkip();
	virtual const NiRTTI *GetStreamableRTTI();
	virtual unsigned int GetBlockAllocationSize();
	virtual NiObjectGroup *GetGroup();
	virtual void *SetGroup(NiObjectGroup *);
	virtual class NiControllerManager *IsNiControllerManager();
};
static_assert(sizeof(NiObject) == 0x10);