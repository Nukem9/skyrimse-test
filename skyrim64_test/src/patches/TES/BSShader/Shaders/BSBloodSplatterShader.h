#pragma once

#include "../BSShader.h"

/*
class BSBloodSplatterShader : public BSShader
{
public:
	// static BSBloodSplatterShader *pInstance;

	BSBloodSplatterShader();
	virtual ~BSBloodSplatterShader();

	virtual bool SetupTechnique(uint32_t Technique) override;
	virtual void RestoreTechnique(uint32_t Technique) override;
	virtual void SetupGeometry(BSRenderPass *Pass) override;
	virtual void RestoreGeometry(BSRenderPass *Pass) override;

	uint32_t m_SpecialTechniqueFlag;// Is either 0 or 1, set in SetupTechnique(), used in SetupGeometry()
};
static_assert(sizeof(BSBloodSplatterShader) == 0x98, "");
static_assert(offsetof(BSBloodSplatterShader, m_SpecialTechniqueFlag) == 0x90, "");
*/