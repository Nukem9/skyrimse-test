#pragma once

#include "../BSShader.h"

class BSGrassShader : public BSShader
{
public:
	// static BSGrassShader *pInstance;

	BSGrassShader();
	virtual ~BSGrassShader();

	virtual bool SetupTechnique(uint32_t Technique) override;
	virtual void SetupMaterial(BSShaderMaterial const *Material) override;
	virtual void SetupGeometry(BSRenderPass *Pass) override;
};
static_assert(sizeof(BSGrassShader) == 0x90, "");