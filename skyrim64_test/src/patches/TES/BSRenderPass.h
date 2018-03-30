#pragma once

#include "BSShader/BSShader.h"

struct BSRenderPass
{
	BSShader *m_Shader;
	BSShaderProperty *m_Property;
	BSGeometry *m_Geometry;
	uint32_t m_TechniqueID;
	uint8_t Byte1C;
	uint8_t Byte1D;
	uint8_t Byte1E;
	char _pad[17];
	BSRenderPass *m_Next;
	void *unkPtr1;// Pointer to an array of lights (directional only?)

	NiAlphaProperty *QAlphaProperty()
	{
		return m_Geometry->QAlphaProperty();
	}
};
// Size unknown
static_assert(offsetof(BSRenderPass, m_Shader) == 0x0, "");
static_assert(offsetof(BSRenderPass, m_Property) == 0x8, "");
static_assert(offsetof(BSRenderPass, m_Geometry) == 0x10, "");
static_assert(offsetof(BSRenderPass, m_TechniqueID) == 0x18, "");
static_assert(offsetof(BSRenderPass, Byte1C) == 0x1C, "");
static_assert(offsetof(BSRenderPass, Byte1D) == 0x1D, "");
static_assert(offsetof(BSRenderPass, Byte1E) == 0x1E, "");
static_assert(offsetof(BSRenderPass, m_Next) == 0x30, "");