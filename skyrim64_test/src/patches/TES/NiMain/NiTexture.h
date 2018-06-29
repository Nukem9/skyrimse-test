#pragma once

#include "NiObjectNET.h"

class NiTexture : public NiObject
{
public:
	NiTexture();
	virtual ~NiTexture();

	// BSFixedString m_kName @ 0x20
	// NiTexture *m_pkPrev; @ 0x30
	// NiTexture *m_pkNext; @ 0x38

	char _pad[0x30];
};
static_assert(sizeof(NiTexture) == 0x40);