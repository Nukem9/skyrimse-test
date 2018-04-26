#pragma once

#include "NiObjectNET.h"

class NiProperty : public NiObjectNET
{
};
static_assert(sizeof(NiProperty) == 0x30);

class NiAlphaProperty : public NiProperty
{
private:
	uint16_t m_uFlags;
	uint8_t m_ucAlphaTestRef;

public:
	bool GetAlphaTesting() const
	{
		return (m_uFlags >> 9) & 1;
	}

	uint8_t GetTestRef() const
	{
		return m_ucAlphaTestRef;
	}
};
static_assert(sizeof(NiAlphaProperty) == 0x38);
//static_assert_offset(NiAlphaProperty, m_uFlags, 0x30);
//static_assert_offset(NiAlphaProperty, m_ucAlphaTestRef, 0x32);