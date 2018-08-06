#pragma once

#include "../BSShader.h"

class BSLightingShaderMaterial;
class BSLightingShaderProperty;

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
		RAW_TECHNIQUE_SNOW = 10,
		RAW_TECHNIQUE_MULTILAYERPARALLAX = 11,
		RAW_TECHNIQUE_TREE = 12,
		RAW_TECHNIQUE_LODOBJ = 13,
		RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW = 14,
		RAW_TECHNIQUE_LODOBJHD = 15,
		RAW_TECHNIQUE_EYE = 16,
		RAW_TECHNIQUE_CLOUD = 17,
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

	const static uintptr_t OriginalVTableBase = 0x187FBD8;

public:
	inline AutoPtr(BSLightingShader *, pInstance, 0x3257C48);

	DECLARE_CONSTRUCTOR_HOOK(BSLightingShader);

	BSLightingShader();
	virtual ~BSLightingShader();

	char _pad0[0x4];
	uint32_t m_CurrentRawTechnique;
	char _pad1[0x60];

	virtual bool SetupTechnique(uint32_t Technique) override;						// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;						// Implemented
	virtual void SetupMaterial(BSShaderMaterial const *Material) override;			// Implemented
	virtual void RestoreMaterial(BSShaderMaterial const *Material) override;		// Nullsub
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;	// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;// Implemented

	static uint32_t GetRawTechnique(uint32_t Technique);
	static uint32_t GetVertexTechnique(uint32_t RawTechnique);
	static uint32_t GetPixelTechnique(uint32_t RawTechnique);

private:
	static void TechUpdateHighDetailRangeConstants(BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG);
	static void TechUpdateFogConstants(BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG);

	static void sub_14130C470(__int64 a1, __int64 a2);
	static void sub_14130C4D0(__int64 a1, __int64 a2);
	static void sub_14130C220(int a1, __int64 a2, __int64 a3);
	static void MatSetMultiTextureLandOverrides(__int64 a1);
	static __int64 sub_141314170(__int64 a1);

	static void GeometrySetupViewProjection(BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, const NiTransform& Transform, bool IsPreviousWorld, const NiPoint3 *PosAdjust);
	static void GeometrySetupMTLandExtraConstants(const BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, const NiPoint3& Translate, float a3, float a4);
	static void GeometrySetupTreeAnimConstants(const BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, BSLightingShaderProperty *Property);
	static void GeometrySetupDirectionalLights(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, const BSRenderPass *Pass, DirectX::XMMATRIX& World, bool WorldSpace);
	static void GeometrySetupAmbientLights(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, const NiTransform& Transform, bool WorldSpace);
	static void GeometrySetupEmitColorConstants(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, BSLightingShaderProperty *Property);
	static void GeometrySetupConstantPointLights(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, BSRenderPass *Pass, DirectX::XMMATRIX& Transform, uint32_t LightCount, uint32_t ShadowLightCount, float Scale, bool WorldSpace);
	static void GeometrySetupProjectedUv(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, BSGeometry *Geometry, BSLightingShaderProperty *Property, bool EnableProjectedNormals);
	static void sub_14130C8A0(const NiTransform& Transform, DirectX::XMMATRIX& OutMatrix, bool DontMultiply);
};
static_assert(sizeof(BSLightingShader) == 0xF8);
static_assert_offset(BSLightingShader, m_CurrentRawTechnique, 0x94);