#pragma once

#include "NiObjectNET.h"

class NiProperty : public NiObjectNET
{
};
static_assert(sizeof(NiProperty) == 0x30);

class NiAlphaProperty : public NiProperty
{
private:
	union
	{
		uint16_t Value;
		struct
		{
			uint16_t DoBlending : 1;
			uint16_t SrcBlend : 4;	// BlendFunction
			uint16_t DstBlend : 4;	// BlendFunction
			uint16_t DoTesting : 1;
			uint16_t TestFunc : 3;	// BlendFunction
			uint16_t NoSorter : 1;
		};
	} AlphaFlags;
	uint8_t m_ucAlphaTestRef;

public:
	bool GetAlphaBlending() const
	{
		return AlphaFlags.DoBlending;
	}

	bool GetAlphaTesting() const
	{
		return AlphaFlags.DoTesting;
	}

	uint8_t GetTestRef() const
	{
		return m_ucAlphaTestRef;
	}
};
static_assert(sizeof(NiAlphaProperty) == 0x38);
//static_assert_offset(NiAlphaProperty, m_uFlags, 0x30);
//static_assert_offset(NiAlphaProperty, m_ucAlphaTestRef, 0x32);