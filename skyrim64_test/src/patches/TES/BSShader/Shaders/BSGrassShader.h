#pragma once

#include "../BSShader.h"

class BSGrassShader : public BSShader
{
public:
	// static BSGrassShader *pInstance;

	BSGrassShader();
	virtual ~BSGrassShader();

	virtual bool SetupTechnique(uint32_t Technique) override;				// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;				// Nullsub
	virtual void SetupMaterial(BSShaderMaterial const *Material) override;	// Implemented
	virtual void RestoreMaterial(BSShaderMaterial const *Material) override;// Nullsub
	virtual void SetupGeometry(BSRenderPass *Pass) override;				// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass) override;				// Nullsub

	// NiBoneMatrixSetterI/BSReloadShaderI unknown
};
static_assert(sizeof(BSGrassShader) == 0x90, "");