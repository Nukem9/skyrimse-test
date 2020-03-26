#include "../../../common.h"
#include "BSShaderManager.h"
#include "BSShaderAccumulator.h"

thread_local BSShaderAccumulator *PerThreadAccumulator;

void BSShaderManager::SetRenderMode(uint32_t RenderMode)
{
	usRenderMode = RenderMode;
	BSShaderAccumulator::SetRenderMode(RenderMode);
}

uint32_t BSShaderManager::GetRenderMode()
{
	return usRenderMode;
}

void BSShaderManager::SetCurrentAccumulator(BSShaderAccumulator *Accumulator)
{
	// pCurrentShaderAccumulator = Accumulator;
	PerThreadAccumulator = Accumulator;
}

BSShaderAccumulator *BSShaderManager::GetCurrentAccumulator()
{
	// return pCurrentShaderAccumulator;
	return PerThreadAccumulator;
}

BSFogProperty *BSShaderManager::GetFogProperty(uint32_t ScenegraphIndex)
{
	AutoFunc(uintptr_t(__fastcall *)(BYTE), sub_1412AC860, 0x12AC860);
	uintptr_t fogParams = sub_1412AC860((BYTE)ScenegraphIndex);

	return (class BSFogProperty *)fogParams;
}

BSFogProperty *BSShaderManager::GetCurrentFogProperty()
{
	return GetFogProperty(St.cSceneGraph);
}