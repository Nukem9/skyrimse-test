#pragma once

#include "BSGeometry.h"

class BSTriShape : public BSGeometry
{
public:
	virtual ~BSTriShape();

	uint16_t m_TriangleCount;
	uint16_t m_VertexCount;

	void GetViewerStrings(void(*Callback)(const char *, ...), bool Recursive) const
	{
		if (Recursive)
			__super::GetViewerStrings(Callback, Recursive);

		Callback("-- BSTriShape --\n");
		Callback("Tri Count = %u\n", (uint32_t)m_TriangleCount);
		Callback("Vert Count = %u\n", (uint32_t)m_VertexCount);
	}
};
static_assert(sizeof(BSTriShape) == 0x160);
static_assert_offset(BSTriShape, m_TriangleCount, 0x158);
static_assert_offset(BSTriShape, m_VertexCount, 0x15A);
