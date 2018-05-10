#pragma once

#include "..\NiMain\NiRefObject.h"
#include "..\NiMain\NiLight.h"

class BSLight : public NiRefObject
{
public:
	char _pad[0x4];
	float fLODDimmer;
	char _pad1[0x30];
	NiLight *spLight;
	char _pad2[0xF0];

public:
	float GetLODDimmer() const
	{
		return fLODDimmer;
	}

	NiLight *GetLight()
	{
		return spLight;
	}
};
static_assert(sizeof(BSLight) == 0x140);
static_assert_offset(BSLight, fLODDimmer, 0x14);
static_assert_offset(BSLight, spLight, 0x48);