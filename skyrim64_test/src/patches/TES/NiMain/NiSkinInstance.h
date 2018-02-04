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

	virtual ~NiSkinInstance();
	virtual void VFunc37(UnknownData *Data) = 0;
};
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