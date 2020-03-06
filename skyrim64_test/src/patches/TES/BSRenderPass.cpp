#include "../../common.h"
#include "BSRenderPass.h"

void BSRenderPass::Set(BSShader *Shader, BSShaderProperty *ShaderProperty, BSGeometry *Geometry, uint32_t PassEnum, uint8_t NumLights, BSLight **SceneLights)
{
	m_PassEnum = PassEnum;
	m_Shader = Shader;
	m_ShaderProperty = ShaderProperty;
	m_Geometry = Geometry;
	m_AccumulationHint = 0;
	m_ExtraParam = 0;
	m_LODMode.Index = 3;
	m_LODMode.SingleLevel = false;
	m_NumLights = NumLights;
	m_Word20 = 0;
	m_Next = nullptr;
	m_PassGroupNext = nullptr;
	m_CachePoolId = 0;

	Assert(m_SceneLights);
	SetLights(NumLights, SceneLights);
}

void BSRenderPass::SetLights(uint8_t NumLights, BSLight **SceneLights)
{
	AssertMsg(NumLights <= MaxLightInArrayC, "MaxLightInArrayC is too small");

	m_NumLights = NumLights;

	for (uint32_t i = 0; i < MaxLightInArrayC; i++)
		m_SceneLights[i] = nullptr;

	for (uint32_t i = 0; i < NumLights; i++)
		m_SceneLights[i] = SceneLights[i];
}

NiAlphaProperty *BSRenderPass::QAlphaProperty() const
{
	return m_Geometry->QAlphaProperty();
}

BSLight **BSRenderPass::QLights() const
{
	if (m_NumLights > 0)
		return m_SceneLights;

	return nullptr;
}