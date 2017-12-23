#pragma once

class NiMemObject
{
};

class NiMatrix3
{
public:
	float m_pEntry[3][3];
};

class NiPoint3
{
public:
	float x;
	float y;
	float z;
};

class NiTransform// : public NiMemObject
{
public:
	NiMatrix3 m_Rotate;
	NiPoint3 m_Translate;
	float m_fScale;
};
static_assert(sizeof(NiTransform) == 0x34, "");
//static_assert(offsetof(NiTransform, m_Rotate) == 0x0, "");
//static_assert(offsetof(NiTransform, m_Translate) == 0x24, "");
//static_assert(offsetof(NiTransform, m_fScale) == 0x30, "");