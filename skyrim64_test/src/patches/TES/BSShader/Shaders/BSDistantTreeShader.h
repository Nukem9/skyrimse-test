#pragma once

#include "../BSShader.h"

class BSDistantTreeShader : public BSShader
{
private:
	enum
	{
		RAW_TECHNIQUE_BLOCK = 0,
		RAW_TECHNIQUE_DEPTH = 1,
	};

	inline AutoPtr(BSDistantTreeShader *, pInstance, 0x32A7F50);

public:
	DECLARE_CONSTRUCTOR_HOOK(BSDistantTreeShader);

	BSDistantTreeShader();
	virtual ~BSDistantTreeShader();

	virtual bool SetupTechnique(uint32_t Technique) override;	// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;	// Nullsub
	virtual void SetupGeometry(BSRenderPass *Pass) override;	// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass) override;	// Nullsub

	uint32_t GetVertexTechnique(uint32_t RawTechnique);
	uint32_t GetPixelTechnique(uint32_t RawTechnique);
};
static_assert(sizeof(BSDistantTreeShader) == 0x90);