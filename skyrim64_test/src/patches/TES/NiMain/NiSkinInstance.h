#pragma once

#include "NiObject.h"

class NiBoneMatrixSetterI;
class BSGeometry;

struct SkinRenderData
{
	NiBoneMatrixSetterI *m_BoneSetter;
	BSGeometry *m_Geometry;
	void *m_Decl;					// Don't know what this means
	uint32_t m_SingleLevelLOD;		// Guessed from FO4
	uint32_t m_LODIndex;			// Guessed from FO4
	float m_SkinLODLevel;			// Guessed from FO4
	uint32_t m_VertexBufferOffset;

	SkinRenderData(NiBoneMatrixSetterI *Setter, BSGeometry *Geometry, void *Decl, uint32_t SingleLevelLOD, uint32_t LODIndex)
	{
		m_BoneSetter = Setter;
		m_Geometry = Geometry;
		m_Decl = Decl;
		m_SingleLevelLOD = SingleLevelLOD;
		m_LODIndex = LODIndex;
		m_SkinLODLevel = 0.0f;
		m_VertexBufferOffset = -1;
	}
};
static_assert_offset(SkinRenderData, m_BoneSetter, 0x0);
static_assert_offset(SkinRenderData, m_Geometry, 0x8);
static_assert_offset(SkinRenderData, m_Decl, 0x10);
static_assert_offset(SkinRenderData, m_SingleLevelLOD, 0x18);
static_assert_offset(SkinRenderData, m_LODIndex, 0x1C);
static_assert_offset(SkinRenderData, m_SkinLODLevel, 0x20);
static_assert_offset(SkinRenderData, m_VertexBufferOffset, 0x24);

class NiSkinInstance : public NiObject
{
public:
	char _pad0[0x38];
	void *m_pvBoneMatrices;
	void *m_pvPrevBoneMatrices;
	char _pad1[0x8];
	CRITICAL_SECTION m_csLock;

	virtual ~NiSkinInstance();
	virtual void VFunc37(SkinRenderData *RenderData) = 0;
};
static_assert(sizeof(NiSkinInstance) == 0x88);
static_assert_offset(NiSkinInstance, m_pvBoneMatrices, 0x48);
static_assert_offset(NiSkinInstance, m_pvPrevBoneMatrices, 0x50);
static_assert_offset(NiSkinInstance, m_csLock, 0x60);

STATIC_CONSTRUCTOR(__CheckNiSkinInstance, []
{
	assert_vtable_index(&NiSkinInstance::VFunc37, 37);
});