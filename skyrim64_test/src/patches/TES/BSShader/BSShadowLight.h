#pragma once

#include "BSLight.h"

class BSShadowLight : public BSLight
{
public:
	char _pad[0x400];
	uint32_t UnkDword540;
};
static_assert_offset(BSShadowLight, UnkDword540, 0x540);