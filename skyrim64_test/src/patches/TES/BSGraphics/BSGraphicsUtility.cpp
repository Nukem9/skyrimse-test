#include "../../../common.h"
#include "BSGraphicsUtility.h"

namespace BSGraphics::Utility
{
	void CopyNiColorAToFloat(float *Floats, const NiColorA& Color)
	{
		Floats[0] = Color.r;
		Floats[1] = Color.g;
		Floats[2] = Color.b;
		Floats[3] = Color.a;
	}

	void CopyNiColorAToFloat(DirectX::XMVECTOR *Floats, const NiColorA& Color)
	{
		*Floats = Color.AsXmm();
	}

	void PackDynamicParticleData(uint32_t ParticleCount, class NiParticles *Particles, void *Buffer)
	{
		AutoFunc(decltype(&PackDynamicParticleData), sub_140D75710, 0xD75710);
		sub_140D75710(ParticleCount, Particles, Buffer);
	}
}