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

	char _pad[0x48];

	NiAlphaProperty *QAlphaProperty() const
	{
		return *(NiAlphaProperty **)((uintptr_t)this + 0x120);
	}

	NiSkinInstance *QSkinInstance() const
	{
		return *(NiSkinInstance **)((uintptr_t)this + 0x130);
	}

	void *QRendererData()
	{
		return *(void **)((uintptr_t)this + 0x138);
	}

	uint64_t GetVertexDesc()
	{
		return *(uint64_t *)((uintptr_t)this + 0x148);
	}

	uint32_t GetVertexSize1()
	{
		return (GetVertexDesc() >> 2) & 0x3C;
	}

	int QType() const
	{
		return *(uint8_t *)((uintptr_t)this + 0x150);
	}
};
static_assert(sizeof(BSGeometry) == 0x158);