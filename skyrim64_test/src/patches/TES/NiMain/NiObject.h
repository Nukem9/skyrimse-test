#pragma once

#include "NiRefObject.h"
#include "NiRTTI.h"

class NiNode;
class NiStream;
class NiCloningProcess;
class NiObjectGroup;

class NiObject : public NiRefObject
{
public:
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

	void GetViewerStrings(void(*Callback)(const char *, ...), bool Recursive) const
	{
		if (Recursive)
			__super::GetViewerStrings(Callback, Recursive);

		Callback("-- NiObject --\n");
		Callback("NiRTTI = %s\n", GetRTTI()->GetName());
	}

	bool IsExactKindOf(NiRTTI *RTTI) const
	{
		return GetRTTI() == RTTI;
	}
};
static_assert(sizeof(NiObject) == 0x10);