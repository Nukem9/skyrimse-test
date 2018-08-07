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

class NiSkinPartition : public NiObject
{
public:
	class Partition
	{
	public:
		char _pad0[0x3A];
		uint16_t m_usTriangles;
		uint16_t m_usBones;
		char _pad1[0xA];
		void *pRendererData;	// Usually casted to BSGraphics::TriShape or BSGraphics::DynamicTriShape

		Partition()
		{
			auto sub_140C7B160 = (void(__fastcall *)(Partition *))(g_ModuleBase + 0xC7B160);
			sub_140C7B160(this);
		}

		~Partition()
		{
			auto sub_140C7B1A0 = (void(__fastcall *)(Partition *))(g_ModuleBase + 0xC7B1A0);
			sub_140C7B1A0(this);
		}
	};

	uint32_t m_uiPartitions;
	Partition *m_pkPartitions;
	uint32_t uiVertexCount;

	virtual bool RenderPartition(SkinRenderData *RenderData, uint32_t Index) = 0;
	/*{
		Partition *partition = &m_pkPartitions[Index];
		BSGeometry *geometry = RenderData->m_Geometry;

		if (!byte_141E2B650[3 * (RenderData->m_LODIndex + 4i64 * RenderData->m_SingleLevelLOD) + *(unsigned __int8 *)(partition + 0x42)])
			return false;

		RenderData->m_BoneSetter->SetBoneMatrix(geometry->QSkinInstance(), partition, &geometry->GetWorldTransform());

		if (BSDynamicTriShape *dynamicTriShape = geometry->IsDynamicTriShape())
		{
			(IRendererResourceManager::pInstance + 0x30)(
				static_cast<BSGraphics::DynamicTriShape *>(partition->pRendererData),
				dynamicTriShape->DrawData,
				0, // IndexStartOffset
				partition->m_usTriangles,
				RenderData->m_VertexBufferOffset);
		}
		else
		{
			(IRendererResourceManager::pInstance + 0x38)(
				static_cast<BSGraphics::TriShape *>(partition->pRendererData),
				0, // StartOffset
				partition->m_usTriangles);
		}

		return true;
	}*/

	void Render(SkinRenderData *RenderData)
	{
		for (uint32_t i = 0; i < m_uiPartitions; i++)
			RenderPartition(RenderData, i);
	}
};
static_assert(sizeof(NiSkinPartition::Partition) == 0x50);
static_assert_offset(NiSkinPartition::Partition, m_usTriangles, 0x3A);
static_assert_offset(NiSkinPartition::Partition, m_usBones, 0x3C);
static_assert_offset(NiSkinPartition::Partition, pRendererData, 0x48);

static_assert(sizeof(NiSkinPartition) == 0x28);
static_assert_offset(NiSkinPartition, m_uiPartitions, 0x10);
static_assert_offset(NiSkinPartition, m_pkPartitions, 0x18);
static_assert_offset(NiSkinPartition, uiVertexCount, 0x20);

class NiSkinInstance : public NiObject
{
public:
	class NiSkinData *m_spSkinData;
	NiSkinPartition *m_spSkinPartition;
	char _pad0[0x28];
	void *m_pvBoneMatrices;
	void *m_pvPrevBoneMatrices;
	char _pad1[0x8];
	CRITICAL_SECTION m_csLock;

	virtual ~NiSkinInstance();

	virtual void Render(SkinRenderData *RenderData)
	{
		m_spSkinPartition->Render(RenderData);
	}
};
static_assert(sizeof(NiSkinInstance) == 0x88);
static_assert_offset(NiSkinInstance, m_spSkinData, 0x10);
static_assert_offset(NiSkinInstance, m_spSkinPartition, 0x18);
static_assert_offset(NiSkinInstance, m_pvBoneMatrices, 0x48);
static_assert_offset(NiSkinInstance, m_pvPrevBoneMatrices, 0x50);
static_assert_offset(NiSkinInstance, m_csLock, 0x60);

STATIC_CONSTRUCTOR(__CheckNiSkinInstance, []
{
	assert_vtable_index(&NiSkinPartition::RenderPartition, 37);
	assert_vtable_index(&NiSkinInstance::Render, 37);
});