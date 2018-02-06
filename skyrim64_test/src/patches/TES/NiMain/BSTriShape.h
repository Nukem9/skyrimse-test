#pragma once

#include "BSGeometry.h"

class BSTriShape : public BSGeometry
{
public:
	virtual ~BSTriShape();

	uint16_t m_TriangleCount;
	uint16_t m_VertexCount;
};
static_assert(sizeof(BSTriShape) == 0x160);