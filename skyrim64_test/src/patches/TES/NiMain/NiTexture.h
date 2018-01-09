#pragma once

#include "NiObjectNET.h"

class NiTexture : public NiObject
{
public:
	NiTexture();
	virtual ~NiTexture();

	char _pad[0x30];
};
static_assert(sizeof(NiTexture) == 0x40);