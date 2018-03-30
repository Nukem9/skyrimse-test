#pragma once

#include "NiObject.h"

class NiBoneMatrixSetterI;
class BSGeometry;

class NiSkinInstance : public NiObject
{
public:
	struct UnknownData
	{
		NiBoneMatrixSetterI *m_BoneSetter;
		BSGeometry *m_Geometry;
		void *m_UnkPtr;
		uint32_t m_UnkDword1;
		uint32_t m_UnkDword2;
		uint32_t m_UnkDword3;
		uint32_t m_UnkDword4;
	};

	char _pad0[0x38];
	void *m_pvBoneMatrices;
	void *m_pvPrevBoneMatrices;
	char _pad1[0x8];
	CRITICAL_SECTION m_csLock;

	virtual ~NiSkinInstance();
	virtual void VFunc37(UnknownData *Data) = 0;
};
static_assert(sizeof(NiSkinInstance) == 0x88);
static_assert_offset(NiSkinInstance, m_pvBoneMatrices, 0x48);
static_assert_offset(NiSkinInstance, m_pvPrevBoneMatrices, 0x50);
static_assert_offset(NiSkinInstance, m_csLock, 0x60);

static_assert_offset(NiSkinInstance::UnknownData, m_BoneSetter, 0x0);
static_assert_offset(NiSkinInstance::UnknownData, m_Geometry, 0x8);
static_assert_offset(NiSkinInstance::UnknownData, m_UnkPtr, 0x10);
static_assert_offset(NiSkinInstance::UnknownData, m_UnkDword1, 0x18);
static_assert_offset(NiSkinInstance::UnknownData, m_UnkDword2, 0x1C);
static_assert_offset(NiSkinInstance::UnknownData, m_UnkDword3, 0x20);
static_assert_offset(NiSkinInstance::UnknownData, m_UnkDword4, 0x24);

STATIC_CONSTRUCTOR(__CheckNiSkinInstance, []
{
	assert_vtable_index(&NiSkinInstance::VFunc37, 37);
});