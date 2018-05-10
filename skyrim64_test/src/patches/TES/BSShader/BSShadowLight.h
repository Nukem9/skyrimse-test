#pragma once

#include "BSLight.h"

class BSShadowLight : public BSLight
{
public:
	char _pad[0x3E0];
	uint32_t UnkDword520;
};
static_assert_offset(BSShadowLight, UnkDword520, 0x520);