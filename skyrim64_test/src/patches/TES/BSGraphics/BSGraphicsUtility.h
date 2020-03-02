#pragma once

#include <DirectXMath.h>
#include "../NiMain/NiColor.h"

namespace BSGraphics::Utility
{
	void CopyNiColorAToFloat(float *Floats, const NiColorA& Color);
	void CopyNiColorAToFloat(DirectX::XMVECTOR *Floats, const NiColorA& Color);
	void PackDynamicParticleData(uint32_t ParticleCount, class NiParticles *Particles, void *Buffer);
}