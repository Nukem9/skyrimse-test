#pragma once

#include "NiAVObject.h"
#include "NiAlphaProperty.h"
#include "NiSkinInstance.h"

enum
{
	GEOMETRY_TYPE_GEOMETRY = 0x0,
	GEOMETRY_TYPE_PARTICLES = 0x1,
	GEOMETRY_TYPE_STRIP_PARTICLES = 0x2,
	GEOMETRY_TYPE_TRISHAPE = 0x3,
	GEOMETRY_TYPE_DYNAMIC_TRISHAPE = 0x4,
	GEOMETRY_TYPE_MESHLOD_TRISHAPE = 0x5,
	GEOMETRY_TYPE_LOD_MULTIINDEX_TRISHAPE = 0x6,
	GEOMETRY_TYPE_MULTIINDEX_TRISHAPE = 0x7,
	GEOMETRY_TYPE_SUBINDEX_TRISHAPE = 0x8,
	GEOMETRY_TYPE_SUBINDEX_LAND_TRISHAPE = 0x9,
	GEOMETRY_TYPE_MULTISTREAMINSTANCE_TRISHAPE = 0xA,
	GEOMETRY_TYPE_PARTICLE_SHADER_DYNAMIC_TRISHAPE = 0xB,
	GEOMETRY_TYPE_LINES = 0xC,
	GEOMETRY_TYPE_DYNAMIC_LINES = 0xD,
	GEOMETRY_TYPE_INSTANCE_GROUP = 0xE,
};

class BSGeometry : public NiAVObject
{
public:
	virtual ~BSGeometry();

	char _pad[0x10];
	NiProperty *spProperties[2];	// NiPointer<NiProperty> spProperties[2];
	NiSkinInstance *spSkinInstance;	// NiPointer<NiSkinInstance>
	void *pRendererData;
	char _pad2[0x8];
	uint64_t uiVertexDesc;
	char ucType;
	// bool Registered?

	NiAlphaProperty *QAlphaProperty() const
	{
		return static_cast<NiAlphaProperty *>(spProperties[0]);
	}

	NiSkinInstance *QSkinInstance() const
	{
		return spSkinInstance;
	}

	void *QRendererData() const
	{
		return pRendererData;
	}

	uint64_t GetVertexDesc() const
	{
		return uiVertexDesc;
	}

	uint32_t GetVertexSize1() const
	{
		return (GetVertexDesc() >> 2) & 0x3C;
	}

	int QType() const
	{
		return ucType;
	}
};
static_assert(sizeof(BSGeometry) == 0x158);
static_assert_offset(BSGeometry, spProperties, 0x120);
static_assert_offset(BSGeometry, spSkinInstance, 0x130);
static_assert_offset(BSGeometry, pRendererData, 0x138);
static_assert_offset(BSGeometry, uiVertexDesc, 0x148);
static_assert_offset(BSGeometry, ucType, 0x150);