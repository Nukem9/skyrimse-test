#pragma once

#include "NiLight.h"

class NiDirectionalLight : public NiLight
{
public:
	NiPoint3 m_kWorldDir;
	char _pad[0xC]; // NiColor m_kEffectColor?

	// The model direction of the light is (1,0,0). The world direction is
	// the first column of the world rotation matrix.
	inline const NiPoint3& GetWorldDirection() const
	{
		return m_kWorldDir;
	}
};
static_assert(sizeof(NiDirectionalLight) == 0x158);
static_assert_offset(NiDirectionalLight, m_kWorldDir, 0x140);