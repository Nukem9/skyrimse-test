#pragma once

#include "../BSShader.h"

class BSGrassShader : public BSShader
{
private:
	enum Techniques
	{
		RAW_TECHNIQUE_VERTEXL = 0,
		RAW_TECHNIQUE_FLATL = 1,
		RAW_TECHNIQUE_FLATL_SLOPE = 2,
		RAW_TECHNIQUE_VERTEXL_SLOPE = 3,
		RAW_TECHNIQUE_VERTEXL_BILLBOARD = 4,
		RAW_TECHNIQUE_FLATL_BILLBOARD = 5,
		RAW_TECHNIQUE_FLATL_SLOPE_BILLBOARD = 6,
		RAW_TECHNIQUE_VERTEXL_SLOPE_BILLBOARD = 7,
		RAW_TECHNIQUE_RENDERDEPTH = 8,
	};

	enum
	{
		RAW_FLAG_DO_ALPHA = 0x10000,
	};

#pragma pack(push, 16)
	struct VertexConstantData
	{
		DirectX::XMMATRIX WorldViewProj;
		DirectX::XMMATRIX WorldView;
		DirectX::XMMATRIX World;
		DirectX::XMMATRIX PreviousWorld;
		DirectX::XMVECTOR FogNearColor;
		float WindVector[3];
		float WindTimer;
		NiPoint3 DirLightDirection;
		float PreviousWindTimer;
		NiColor DirLightColor;
		float AlphaParam1;
		NiColor AmbientColor;
		float AlphaParam2;
		float ScaleMask[3];
		float ShadowClampValue;
	};
	static_assert(sizeof(VertexConstantData) == 0x160);
#pragma pack(pop)

	const static uintptr_t OriginalVTableBase = 0x1878260;

public:
	inline AutoPtr(BSGrassShader *, pInstance, 0x31F63E0);

	DECLARE_CONSTRUCTOR_HOOK(BSGrassShader);

	BSGrassShader();
	virtual ~BSGrassShader();

	virtual bool SetupTechnique(uint32_t Technique) override;						// Implemented
	virtual void RestoreTechnique(uint32_t Technique) override;						// Nullsub
	virtual void SetupMaterial(BSShaderMaterial const *Material) override;			// Implemented
	virtual void RestoreMaterial(BSShaderMaterial const *Material) override;		// Nullsub
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;	// Implemented
	virtual void RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags) override;// Nullsub

	void UpdateFogParameters();
	void UpdateGeometryProjections(VertexConstantData *Data, const NiTransform& GeoTransform);
	void UpdateGeometryInstanceData(const BSGeometry *Geometry, BSShaderProperty *Property);

	void CreateAllShaders();
	void CreateVertexShader(uint32_t Technique);
	void CreatePixelShader(uint32_t Technique);

	static uint32_t GetRawTechnique(uint32_t Technique);
	static uint32_t GetVertexTechnique(uint32_t RawTechnique);
	static uint32_t GetPixelTechnique(uint32_t RawTechnique);

	static std::vector<std::pair<const char *, const char *>> GetSourceDefines(uint32_t Technique);
};
static_assert(sizeof(BSGrassShader) == 0x90);