#include "../rendering/common.h"
#include "../../common.h"
#include "BSGraphicsRenderer.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "MemoryContextTracker.h"
#include "BSSpinLock.h"
#include "BSBatchRenderer.h"
#include "BSReadWriteLock.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/Shaders/BSSkyShader.h"
#include "BSShader/Shaders/BSLightingShader.h"
#include "MTRenderer.h"

BSReadWriteLock testLocks[32];
uintptr_t commandDataStart[6];
uintptr_t commandData[6];
thread_local class GameCommandList *ActiveManager;

STATIC_CONSTRUCTOR(__Testing, []
{
	for (int i = 0; i < ARRAYSIZE(commandData); i++)
	{
		commandData[i] = (uintptr_t)VirtualAlloc(nullptr, 1 * 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		commandDataStart[i] = commandData[i];

		if (!commandData[i])
			__debugbreak();
	}
})

namespace MTRenderer
{
	thread_local bool testmtr;

	bool IsGeneratingGameCommandList()
	{
		if (!ActiveManager)
			return false;

		return true;
	}

	bool IsRenderingMultithreaded()
	{
		return testmtr;
	}

	void ClearShaderAndTechnique()
	{
		if (IsGeneratingGameCommandList())
			InsertCommand<ClearStateRenderCommand>();
		else
			::ClearShaderAndTechnique();
	}

	void RasterStateSetCullMode(uint32_t CullMode)
	{
		if (IsGeneratingGameCommandList())
			InsertCommand<SetStateRenderCommand>(SetStateRenderCommand::RasterStateCullMode, CullMode);
		else
			BSGraphics::Renderer::GetGlobals()->RasterStateSetCullMode(CullMode);
	}

	void AlphaBlendStateSetUnknown1(uint32_t Value)
	{
		if (IsGeneratingGameCommandList())
			InsertCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateUnknown1, Value);
		else
			BSGraphics::Renderer::GetGlobals()->AlphaBlendStateSetUnknown1(Value);
	}

	void LockShader(int ShaderType)
	{
		Assert(ShaderType != -1, "Invalid shader type supplied - should've never reached this");

		switch (ShaderType)
		{
		case 1:
		case 6:
		case 9:
			break;

		default:
			if (IsGeneratingGameCommandList())
				InsertCommand<LockShaderTypeRenderCommand>(ShaderType, true);
			else
				testLocks[ShaderType].AcquireWrite();
			break;
		}
	}

	void UnlockShader(int ShaderType)
	{
		Assert(ShaderType != -1, "Invalid shader type supplied - should've never reached this");

		switch (ShaderType)
		{
		case 1:
		case 6:
		case 9:
			break;

		default:
			if (IsGeneratingGameCommandList())
				InsertCommand<LockShaderTypeRenderCommand>(ShaderType, false);
			else
				testLocks[ShaderType].ReleaseWrite();
			break;
		}
	}

}