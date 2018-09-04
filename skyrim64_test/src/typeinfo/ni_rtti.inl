//
// NiRefObject
//
NI_START_LIST(NiRefObject)
	NI_LIST_ENTRY(void *{{__vecDelDtor}}({{TYPE}} *this, unsigned int))
	NI_LIST_ENTRY(void {{DeleteThis}}({{TYPE}} *this))
NI_END_LIST()

NI_START_LIST_CK(NiRefObject)
NI_END_LIST_CK()

//
// NiObject
//
NI_START_LIST(NiObject)
	NI_LIST_ENTRY(NiRTTI *{{GetRTTI}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiNode *{{IsNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiSwitchNode *{{IsSwitchNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSFadeNode *{{IsFadeNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSMultiBoundNode *{{IsMultiBoundNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSGeometry *{{IsGeometry}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiTriStrips *{{IsTriStrips}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSTriShape *{{IsTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSSegmentedTriShape *{{IsSegmentedTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSSubIndexTriShape *{{IsSubIndexTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSDynamicTriShape *{{IsDynamicTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiGeometry *{{IsNiGeometry}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiTriBasedGeom *{{IsNiTriBasedGeom}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiTriShape *{{IsNiTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiParticles *{{IsParticlesGeom}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSLines *{{IsLinesGeom}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkNiCollisionObject *{{IsBhkNiCollisionObject}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkBlendCollisionObject *{{IsBhkBlendCollisionObject}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkAttachmentCollisionObject *{{IsBhkAttachmentCollisionObject}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkRigidBody *{{IsBhkRigidBody}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkLimitedHingeConstraint *{{IsBhkLimitedHingeConstraint}}({{TYPE}} *this))

	NI_LIST_ENTRY(NiObject *{{CreateClone}}({{TYPE}} *this, NiCloningProcess *Process))
	NI_LIST_ENTRY(void {{LoadBinary}}({{TYPE}} *this, NiStream *))
	NI_LIST_ENTRY(void {{LinkObject}}({{TYPE}} *this, NiStream *))
	NI_LIST_ENTRY(bool {{RegisterStreamables}}({{TYPE}} *this, NiStream *Stream))
	NI_LIST_ENTRY(void {{SaveBinary}}({{TYPE}} *this, NiStream *Stream))
	NI_LIST_ENTRY(bool {{IsEqual}}({{TYPE}} *this, NiObject *Other))
	NI_LIST_ENTRY(void {{GetViewerStrings}}({{TYPE}} *this, NiTPrimitiveArray<char const *> *Strings)) // CREATION KIT ONLY
	NI_LIST_ENTRY(void {{ProcessClone}}({{TYPE}} *this, NiCloningProcess *Process))
	NI_LIST_ENTRY(void {{PostLinkObject}}({{TYPE}} *this, NiStream *Stream))
	NI_LIST_ENTRY(bool {{StreamCanSkip}}({{TYPE}} *this))
	NI_LIST_ENTRY(const NiRTTI *{{GetStreamableRTTI}}({{TYPE}} *this))
	NI_LIST_ENTRY(unsigned int {{GetBlockAllocationSize}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiObjectGroup *{{GetGroup}}({{TYPE}} *this))
	NI_LIST_ENTRY(void *{{SetGroup}}({{TYPE}} *this, NiObjectGroup *Group))
	NI_LIST_ENTRY(NiControllerManager *{{IsNiControllerManager}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{AddViewerStrings}}({{TYPE}} *this, NiTPrimitiveArray<char const *> *Strings)) // CREATION KIT ONLY
NI_END_LIST()

NI_START_LIST_CK(NiObject)
	NI_LIST_ENTRY_CK(27)
	NI_LIST_ENTRY_CK(36)
NI_END_LIST_CK()

//
// NiObjectNET
//
NI_START_LIST(NiObjectNET)
NI_END_LIST()

NI_START_LIST_CK(NiObjectNET)
NI_END_LIST_CK()

//
// NiAVObject
//
NI_START_LIST(NiAVObject)
	NI_LIST_ENTRY(void {{UpdateControllers}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{PerformOp}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk0}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk1}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk2}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk3}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{SetSelectiveUpdateFlags}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{UpdateDownwardPass}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{UpdateSelectedDownwardPass}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{UpdateRigidDownwardPass}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{UpdateWorldBound}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{UpdateWorldData}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk4}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk5}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk6}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{OnVisible}}({{TYPE}} *this, NiCullingProcess *Process, uint32_t Unknown))
NI_END_LIST()

NI_START_LIST_CK(NiAVObject)
NI_END_LIST_CK()

//
// NiObject
//
NI_START_LIST(NiCullingProcess)
	NI_LIST_ENTRY(NiRTTI *{{GetRTTI}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiNode *{{IsNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiSwitchNode *{{IsSwitchNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSFadeNode *{{IsFadeNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSMultiBoundNode *{{IsMultiBoundNode}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSGeometry *{{IsGeometry}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiTriStrips *{{IsTriStrips}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSTriShape *{{IsTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSSegmentedTriShape *{{IsSegmentedTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSSubIndexTriShape *{{IsSubIndexTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSDynamicTriShape *{{IsDynamicTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiGeometry *{{IsNiGeometry}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiTriBasedGeom *{{IsNiTriBasedGeom}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiTriShape *{{IsNiTriShape}}({{TYPE}} *this))
	NI_LIST_ENTRY(NiParticles *{{IsParticlesGeom}}({{TYPE}} *this))
	NI_LIST_ENTRY(BSLines *{{IsLinesGeom}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkNiCollisionObject *{{IsBhkNiCollisionObject}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkBlendCollisionObject *{{IsBhkBlendCollisionObject}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkAttachmentCollisionObject *{{IsBhkAttachmentCollisionObject}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkRigidBody *{{IsBhkRigidBody}}({{TYPE}} *this))
	NI_LIST_ENTRY(bhkLimitedHingeConstraint *{{IsBhkLimitedHingeConstraint}}({{TYPE}} *this))
	NI_LIST_ENTRY(void *{{__vecDelDtor}}({{TYPE}} *this, unsigned int))
	NI_LIST_ENTRY(void *{{Process1}}({{TYPE}} *this, NiAVObject *Object, uint32_t Unknown))
	NI_LIST_ENTRY(void *{{Process2}}({{TYPE}} *this, const NiCamera *Camera, NiAVObject *Object, class NiVisibleArray *Array))
	NI_LIST_ENTRY(void *{{AppendVirtual}}({{TYPE}} *this, BSGeometry *Geometry, uint32_t Unknown))
NI_END_LIST()

NI_START_LIST_CK(NiCullingProcess)
NI_END_LIST_CK()

//
// BSCullingProcess
//
NI_START_LIST(BSCullingProcess)
	NI_LIST_ENTRY(void {{AppendNonAccum}}({{TYPE}} *this, NiAVObject *Object))
	NI_LIST_ENTRY(bool {{TestBaseVisibility1}}({{TYPE}} *this, BSMultiBound *Bound))
	NI_LIST_ENTRY(bool {{TestBaseVisibility2}}({{TYPE}} *this, BSOcclusionPlane *Bound))
	NI_LIST_ENTRY(bool {{TestBaseVisibility3}}({{TYPE}} *this, NiBound *Bound))
NI_END_LIST()

NI_START_LIST_CK(BSCullingProcess)
NI_END_LIST_CK()

//
// bhkSerializable
//
NI_START_LIST(bhkSerializable)
	NI_LIST_ENTRY(void {{Unk7}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk8}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk9}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk10}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk11}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{Unk12}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{KillCinfo}}({{TYPE}} *this, bool Created))
	NI_LIST_ENTRY(unsigned int {{QCinfoSize}}({{TYPE}} *this))
	NI_LIST_ENTRY(int {{LoadCInfo}}({{TYPE}} *this, NiStream *Stream))
	NI_LIST_ENTRY(void {{Init}}({{TYPE}} *this, const hkSerializableCinfo *Info))
	NI_LIST_ENTRY(hkSerializableCinfo *{{CreateCinfo}}({{TYPE}} *this, bool *Created))
	NI_LIST_ENTRY(void {{PostInit1}}({{TYPE}} *this))
	NI_LIST_ENTRY(void {{PostInit2}}({{TYPE}} *this))
NI_END_LIST()

NI_START_LIST_CK(bhkSerializable)
NI_END_LIST_CK()