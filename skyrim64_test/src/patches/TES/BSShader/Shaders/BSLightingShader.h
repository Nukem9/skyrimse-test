#pragma once

#pragma once

#include "../BSShader.h"

class BSLightingShader : public BSShader
{
private:
	enum Techniques
	{
		RAW_TECHNIQUE_NONE = 0,
		RAW_TECHNIQUE_ENVMAP = 1,
		RAW_TECHNIQUE_GLOWMAP = 2,
		RAW_TECHNIQUE_PARALLAX = 3,
		RAW_TECHNIQUE_FACEGEN = 4,
		RAW_TECHNIQUE_FACEGENRGBTINT = 5,
		RAW_TECHNIQUE_HAIR = 6,
		RAW_TECHNIQUE_PARALLAXOCC = 7,
		RAW_TECHNIQUE_MTLAND = 8,
		RAW_TECHNIQUE_LODLAND = 9,
		RAW_TECHNIQUE_UNKNOWN1 = 10,
		RAW_TECHNIQUE_MULTILAYERPARALLAX = 11,
		RAW_TECHNIQUE_TREE = 12,
		RAW_TECHNIQUE_LODOBJ = 13,
		RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW = 14,
		RAW_TECHNIQUE_LODOBJHD = 15,
		RAW_TECHNIQUE_EYE = 16,
		RAW_TECHNIQUE_UNKNOWN2 = 17,
		RAW_TECHNIQUE_LODLANDNOISE = 18,
		RAW_TECHNIQUE_MTLANDLODBLEND = 19,
	};

	enum
	{
		RAW_FLAG_VC = 1 << 0,
		RAW_FLAG_SKINNED = 1 << 1,
		RAW_FLAG_MODELSPACENORMALS = 1 << 2,
		RAW_FLAG_UNKNOWN1 = 1 << 3,
		RAW_FLAG_UNKNOWN2 = 1 << 4,
		RAW_FLAG_UNKNOWN3 = 1 << 5,
		RAW_FLAG_UNKNOWN4 = 1 << 6,
		RAW_FLAG_UNKNOWN5 = 1 << 7,
		RAW_FLAG_UNKNOWN6 = 1 << 8,
		RAW_FLAG_SPECULAR = 1 << 9,
		RAW_FLAG_SOFT_LIGHTING = 1 << 10,
		RAW_FLAG_RIM_LIGHTING = 1 << 11,
		RAW_FLAG_BACK_LIGHTING = 1 << 12,
		RAW_FLAG_SHADOW_DIR = 1 << 13,
		RAW_FLAG_DEFSHADOW = 1 << 14,
		RAW_FLAG_PROJECTED_UV = 1 << 15,
		RAW_FLAG_ANISO_LIGHTING = 1 << 16,
		RAW_FLAG_AMBIENT_SPECULAR = 1 << 17,
		RAW_FLAG_WORLD_MAP = 1 << 18,
		RAW_FLAG_BASE_OBJECT_IS_SNOW = 1 << 19,
		RAW_FLAG_DO_ALPHA_TEST = 1 << 20,
		RAW_FLAG_SNOW = 1 << 21,
		RAW_FLAG_CHARACTER_LIGHT = 1 << 22,
		RAW_FLAG_ADDITIONAL_ALPHA_MASK = 1 << 23,
	};

	inline AutoPtr(BSLightingShader *, pInstance, 0x3257C48);

	const static uintptr_t OriginalVTableBase = 0x187FBD8;

public:
	DECLARE_CONSTRUCTOR_HOOK(BSLightingShader);

	BSLightingShader();
	virtual ~BSLightingShader();

	char _pad0[0x4];
	uint32_t m_CurrentRawTechnique;
	char _pad1[0x60];

	virtual bool SetupTechnique(uint32_t Technique) override;				// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;				// Implemented
	virtual void SetupMaterial(BSShaderMaterial const *Material) override;	// Implemented
	virtual void RestoreMaterial(BSShaderMaterial const *Material) override;// Nullsub
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t Flags) override;// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass) override;				// Implemented

	uint32_t GetRawTechnique(uint32_t Technique);
	uint32_t GetVertexTechnique(uint32_t RawTechnique);
	uint32_t GetPixelTechnique(uint32_t RawTechnique);
};
static_assert(sizeof(BSLightingShader) == 0xF8, "");
static_assert(offsetof(BSLightingShader, m_CurrentRawTechnique) == 0x94, "");