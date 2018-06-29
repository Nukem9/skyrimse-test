#pragma once

#include "BSNiNode.h"

class BSMultiBoundShape : public NiObject
{
public:
	uint32_t kCullResult;
};
static_assert(sizeof(BSMultiBoundShape) == 0x18);
static_assert(offsetof(BSMultiBoundShape, kCullResult) == 0x10);

class BSMultiBoundAABB : public BSMultiBoundShape
{
public:
	char _pad0[0x4];
	NiPoint3 m_kCenter;
	char _pad1[0x4];
	NiPoint3 m_kHalfExtents;
	char _pad2[0x8];

	void GetViewerStrings(void(*Callback)(const char *, ...), bool Recursive) const
	{
		if (Recursive)
			__super::GetViewerStrings(Callback, Recursive);

		Callback("-- BSMultiBoundAABB --\n");
		Callback("Center = (%g, %g, %g)\n",
			m_kCenter.x,
			m_kCenter.y,
			m_kCenter.z);
		Callback("Half Extents = (%g, %g, %g)\n",
			m_kHalfExtents.x,
			m_kHalfExtents.y,
			m_kHalfExtents.z);
	}
};
static_assert(sizeof(BSMultiBoundAABB) == 0x40);
static_assert(offsetof(BSMultiBoundAABB, m_kCenter) == 0x1C);
static_assert(offsetof(BSMultiBoundAABB, m_kHalfExtents) == 0x2C);

class BSMultiBound : public NiObject
{
public:
	uint32_t uiBoundFrameCount;
	BSMultiBoundShape *spShape;
};
static_assert(sizeof(BSMultiBound) == 0x20);
static_assert(offsetof(BSMultiBound, uiBoundFrameCount) == 0x10);
static_assert(offsetof(BSMultiBound, spShape) == 0x18);

class BSMultiBoundNode : public BSNiNode
{
public:
	BSMultiBound *spMultiBound;
	char _pad[0x8];
};
static_assert(sizeof(BSMultiBoundNode) == 0x138);