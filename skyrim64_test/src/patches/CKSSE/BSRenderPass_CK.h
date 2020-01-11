#pragma once

#include "../TES/BSRenderPass.h"

class BSRenderPass_CK : BSRenderPass
{
public:
	static void InitSDM();
	static void KillSDM();

	static BSRenderPass_CK *AllocatePass(class BSShader *Shader, class BSShaderProperty *ShaderProperty, class BSGeometry *Geometry, uint32_t PassEnum, uint8_t NumLights, BSLight **SceneLights);
	static void DeallocatePass(BSRenderPass_CK *Pass);
};
static_assert(sizeof(BSRenderPass_CK) == 0x48);