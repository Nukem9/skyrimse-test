#pragma once

#include "../BSShader.h"

class BSParticleShader : public BSShader
{
private:
	enum Techniques
	{
		RAW_TECHNIQUE_PARTICLES = 0,
		RAW_TECHNIQUE_PARTICLES_GRYCOLOR = 1,
		RAW_TECHNIQUE_PARTICLES_GRYALPHA = 2,
		RAW_TECHNIQUE_PARTICLES_GRYCOLORALPHA = 3,
		RAW_TECHNIQUE_ENVCUBESNOW = 4,
		RAW_TECHNIQUE_ENVCUBERAIN = 5,
	};

	const static uintptr_t OriginalVTableBase = 0x18852A8;

public:
	inline AutoPtr(BSParticleShader *, pInstance, 0x34BA460);

	DECLARE_CONSTRUCTOR_HOOK(BSParticleShader);

	BSParticleShader();
	virtual ~BSParticleShader();

	virtual bool SetupTechnique(uint32_t Technique) override;						// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;						// Implemented
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;	// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;// Implemented

	void CreateAllShaders();
	void CreateVertexShader(uint32_t Technique);
	void CreatePixelShader(uint32_t Technique);

	static uint32_t GetRawTechnique(uint32_t Technique);
	static uint32_t GetVertexTechnique(uint32_t RawTechnique);
	static uint32_t GetPixelTechnique(uint32_t RawTechnique);
};
static_assert(sizeof(BSParticleShader) == 0x90);