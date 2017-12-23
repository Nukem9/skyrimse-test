#pragma once

#include "NiObject.h"

class NiObjectNET : public NiObject
{
public:
	virtual ~NiObjectNET();

	char _pad[0x20];
};
static_assert(sizeof(NiObjectNET) == 0x30);