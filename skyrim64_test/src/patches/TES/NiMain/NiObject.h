#pragma once

#include "NiRefObject.h"

class NiStream;
class NiCloningProcess;
class NiObjectGroup;

struct NiRTTI
{
};

class NiObject : public NiRefObject
{
public:
	// Verified
	virtual const NiRTTI *GetRTTI();
	virtual const void *IsNode();		// const NiNode *
	virtual void *IsSwitchNode();		// NiSwitchNode *
	virtual void *IsFadeNode();			// BSFadeNode *
	virtual void *IsMultiBoundNode();	// BSMultiBoundNode *
	virtual void *IsGeometry();			// BSGeometry *
	virtual void *IsTriStrips();		// NiTriStrips *
	virtual void *IsTriShape();			// BSTriShape *
	virtual void *IsSegmentedTriShape();// BSSegmentedTriShape *
	virtual void *IsSubIndexTriShape();	// BSSubIndexTriShape *
	virtual void *IsDynamicTriShape();	// BSDynamicTriShape *
	virtual void *IsNiGeometry();		// NiGeometry *
	virtual void *IsNiTriBasedGeom();	// NiTriBasedGeom *
	virtual void *IsNiTriShape();		// NiTriShape *
	virtual void *IsParticlesGeom();	// NiParticles*
	virtual void *IsLinesGeom();		// BSLines *
	virtual void *IsBhkNiCollisionObject();// bhkNiCollisionObject *
	virtual void *IsBhkBlendCollisionObject();// bhkBlendCollisionObject *
	virtual void *unknown2();
	virtual void *IsBhkRigidBody();		// bhkRigidBody *
	virtual void *IsBhkLimitedHingeConstraint();// bhkLimitedHingeConstraint *

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
	virtual void *IsNiControllerManager();// NiControllerManager *
};
static_assert(sizeof(NiObject) == 0x10);