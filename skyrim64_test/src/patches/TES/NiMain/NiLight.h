#pragma once

#include "NiColor.h"
#include "NiAVObject.h"

class NiLight : public NiAVObject
{
public:
	NiColor m_kAmb;
	NiColor m_kDiff;
	NiColor m_kSpec;
	float m_fDimmer;
	char _pad2[0x4];

public:
	const NiColor& GetAmbientColor() const
	{
		return m_kAmb;
	}

	const NiColor& GetDiffuseColor() const
	{
		return m_kDiff;
	}

	const NiColor& GetSpecularColor() const
	{
		return m_kSpec;
	}

	float GetDimmer() const
	{
		return m_fDimmer;
	}
};
static_assert(sizeof(NiLight) == 0x140);
static_assert_offset(NiLight, m_kAmb, 0x110);
static_assert_offset(NiLight, m_kDiff, 0x11C);
static_assert_offset(NiLight, m_kSpec, 0x128);
static_assert_offset(NiLight, m_fDimmer, 0x134);