#pragma once

#include "../../NiMain/NiColor.h"
#include "../BSShader.h"

class BSSkyShader : public BSShader
{
private:
	enum Techniques
	{
		RAW_TECHNIQUE_SUNOCCLUDE = 0,
		RAW_TECHNIQUE_SUNGLARE = 1,
		RAW_TECHNIQUE_MOONANDSTARSMASK = 2,
		RAW_TECHNIQUE_STARS = 3,
		RAW_TECHNIQUE_CLOUDS = 4,
		RAW_TECHNIQUE_CLOUDSLERP = 5,
		RAW_TECHNIQUE_CLOUDSFADE = 6,
		RAW_TECHNIQUE_TEXTURE = 7,
		RAW_TECHNIQUE_SKY = 8,
	};

	inline AutoPtr(NiColorA, xmmword_143257D48, 0x3257D48);
	inline AutoPtr(NiColorA, xmmword_143257D58, 0x3257D58);
	inline AutoPtr(NiColorA, xmmword_143257D68, 0x3257D68);

	const static uintptr_t OriginalVTableBase = 0x1880A60;

public:
	inline AutoPtr(BSSkyShader *, pInstance, 0x3257D30);

	DECLARE_CONSTRUCTOR_HOOK(BSSkyShader);

	BSSkyShader();
	virtual ~BSSkyShader();

	virtual bool SetupTechnique(uint32_t Technique) override;						// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;						// Implemented
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;	// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;// Implemented

	static uint32_t GetRawTechnique(uint32_t Technique);
	static uint32_t GetVertexTechnique(uint32_t RawTechnique);
	static uint32_t GetPixelTechnique(uint32_t RawTechnique);
};
static_assert(sizeof(BSSkyShader) == 0x90);