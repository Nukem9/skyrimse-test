#pragma once

#include "../BSShader.h"

class BSDistantTreeShader : public BSShader
{
private:
	enum Techniques
	{
		RAW_TECHNIQUE_BLOCK = 0,
		RAW_TECHNIQUE_DEPTH = 1,
	};

	enum
	{
		RAW_FLAG_DO_ALPHA = 0x10000,
	};

	const static uintptr_t OriginalVTableBase = 0x1881808;

public:
	inline AutoPtr(BSDistantTreeShader *, pInstance, 0x32A7F50);

	DECLARE_CONSTRUCTOR_HOOK(BSDistantTreeShader);

	BSDistantTreeShader();
	virtual ~BSDistantTreeShader();

	virtual bool SetupTechnique(uint32_t Technique) override;						// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;						// Nullsub
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;	// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;// Nullsub

	void CreateAllShaders();
	void CreateVertexShader(uint32_t Technique);
	void CreatePixelShader(uint32_t Technique);

	static uint32_t GetRawTechnique(uint32_t Technique);
	static uint32_t GetVertexTechnique(uint32_t RawTechnique);
	static uint32_t GetPixelTechnique(uint32_t RawTechnique);
};
static_assert(sizeof(BSDistantTreeShader) == 0x90);