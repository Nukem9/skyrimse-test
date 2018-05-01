#include "../../../common.h"
#include "../../rendering/common.h"
#include "BSShaderManager.h"
#include "BSShaderAccumulator.h"

void BSShaderManager::SetRenderMode(uint32_t RenderMode)
{
	usRenderMode = RenderMode;
	BSShaderAccumulator::SetRenderMode(RenderMode);
}

void BSShaderManager::SetCurrentAccumulator(BSShaderAccumulator *Accumulator)
{
	// BSShaderManager::pCurrentShaderAccumulator
	uint64_t& qword_1431F5490 = *(uint64_t *)((uintptr_t)HACK_GetThreadedGlobals() + 0x3600);

	qword_1431F5490 = (uint64_t)Accumulator;
}

BSShaderAccumulator *BSShaderManager::GetCurrentAccumulator()
{
	// BSShaderManager::pCurrentShaderAccumulator
	return *(BSShaderAccumulator **)((uintptr_t)HACK_GetThreadedGlobals() + 0x3600);
}