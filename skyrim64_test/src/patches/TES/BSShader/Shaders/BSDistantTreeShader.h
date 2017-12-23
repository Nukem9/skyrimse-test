#pragma once

#include "../BSShader.h"

class BSDistantTreeShader : public BSShader
{
public:
	// static BSDistantTreeShader *pInstance;

	BSDistantTreeShader();
	virtual ~BSDistantTreeShader();

	virtual bool SetupTechnique(uint32_t Technique) override;	// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;	// Nullsub
	virtual void SetupGeometry(BSRenderPass *Pass) override;	// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass) override;	// Nullsub

	// NiBoneMatrixSetterI/BSReloadShaderI unknown
};
static_assert(sizeof(BSDistantTreeShader) == 0x90, "");