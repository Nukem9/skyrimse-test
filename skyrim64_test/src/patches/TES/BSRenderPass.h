#pragma once

#include "BSShader/BSShader.h"

class BSShader;
class BSShaderProperty;
class BSGeometry;
class BSLight;

class BSRenderPass
{
public:
	const static int MaxLightInArrayC = 16;

	BSShader *m_Shader;
	BSShaderProperty *m_ShaderProperty;
	BSGeometry *m_Geometry;
	uint32_t m_PassEnum;
	uint8_t m_AccumulationHint;
	uint8_t m_ExtraParam;
	struct
	{
		uint8_t Index : 7;				// Also referred to as "texture degrade level"
		bool SingleLevel : 1;
	} m_LODMode;
	uint8_t m_NumLights;
	uint16_t m_Word20;					// Not used?
	BSRenderPass *m_Next;
	BSRenderPass *m_PassGroupNext;
	BSLight **m_SceneLights;
	uint32_t m_CachePoolId;				// Maximum of 2

	void Set(BSShader *Shader, BSShaderProperty *ShaderProperty, BSGeometry *Geometry, uint32_t PassEnum, uint8_t NumLights, BSLight **SceneLights);
	void SetLights(uint8_t NumLights, BSLight **SceneLights);

	NiAlphaProperty *QAlphaProperty() const;
	BSLight **QLights() const;
};
static_assert(sizeof(BSRenderPass) == 0x48);
static_assert_offset(BSRenderPass, m_Shader, 0x0);
static_assert_offset(BSRenderPass, m_ShaderProperty, 0x8);
static_assert_offset(BSRenderPass, m_Geometry, 0x10);
static_assert_offset(BSRenderPass, m_PassEnum, 0x18);
static_assert_offset(BSRenderPass, m_AccumulationHint, 0x1C);
static_assert_offset(BSRenderPass, m_ExtraParam, 0x1D);
static_assert_offset(BSRenderPass, m_LODMode, 0x1E);
static_assert_offset(BSRenderPass, m_NumLights, 0x1F);
static_assert_offset(BSRenderPass, m_Word20, 0x20);
static_assert_offset(BSRenderPass, m_Next, 0x28);
static_assert_offset(BSRenderPass, m_PassGroupNext, 0x30);
static_assert_offset(BSRenderPass, m_SceneLights, 0x38);
static_assert_offset(BSRenderPass, m_CachePoolId, 0x40);