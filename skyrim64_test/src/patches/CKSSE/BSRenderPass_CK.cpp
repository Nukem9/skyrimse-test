#include "../../common.h"
#include "../TES/MemoryManager.h"
#include "BSRenderPass_CK.h"

void BSRenderPass_CK::InitSDM()
{
	// Intentionally left empty
}

void BSRenderPass_CK::KillSDM()
{
	// Intentionally left empty
}

BSRenderPass_CK *BSRenderPass_CK::AllocatePass(BSShader *Shader, BSShaderProperty *ShaderProperty, BSGeometry *Geometry, uint32_t PassEnum, uint8_t NumLights, BSLight **SceneLights)
{
	uint32_t size = sizeof(BSRenderPass_CK) + (sizeof(BSLight *) * MaxLightInArrayC);
	void *data = MemoryManager::Allocate(nullptr, size, 8, true);

	memset(data, 0, size);

	auto *pass = reinterpret_cast<BSRenderPass_CK *>(data);
	auto *lights = reinterpret_cast<BSLight **>(&pass[1]);

	pass->m_SceneLights = lights;
	pass->Set(Shader, ShaderProperty, Geometry, PassEnum, NumLights, SceneLights);
	pass->m_CachePoolId = 0xFEFEDEAD;

	return pass;
}

void BSRenderPass_CK::DeallocatePass(BSRenderPass_CK *Pass)
{
	MemoryManager::Deallocate(nullptr, Pass, true);
}