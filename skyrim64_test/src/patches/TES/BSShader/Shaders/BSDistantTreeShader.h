#pragma once

#include "../BSShader.h"

class BSDistantTreeShader : public BSShader
{
public:
	// static BSDistantTreeShader *pInstance;

	BSDistantTreeShader();
	virtual ~BSDistantTreeShader();

	virtual bool SetupTechnique(uint32_t Technique) override;
	virtual void SetupGeometry(BSRenderPass *Pass) override;
};
static_assert(sizeof(BSDistantTreeShader) == 0x90, "");