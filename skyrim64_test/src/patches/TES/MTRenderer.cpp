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

uintptr_t commandDataStart[6];
uintptr_t commandData[6];
thread_local int ThreadUsingCommandList;

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
		if (!ThreadUsingCommandList)
			return false;

		return true;
	}

	bool IsRenderingMultithreaded()
	{
		return testmtr;
	}

	void ExecuteCommandList(int Index, bool MTWorker)
	{
		ProfileTimer("GameCommandListToD3D");

		testmtr = MTWorker;

		// Run everything in the command list...
		bool endOfList = false;
		int cmdCount = 0;

		for (uintptr_t ptr = commandDataStart[Index]; !endOfList;)
		{
			cmdCount++;
			RenderCommand *cmd = (RenderCommand *)ptr;
			ptr += cmd->m_Size;

			switch (cmd->m_Type)
			{
			case 0:
				endOfList = true;
				break;

			case 1:
				static_cast<ClearStateRenderCommand *>(cmd)->Run();
				break;

			case 2:
				static_cast<SetStateRenderCommand *>(cmd)->Run();
				break;

			case 3:
				static_cast<DrawGeometryRenderCommand *>(cmd)->Run();
				break;

			case 4:
			{
				auto b = static_cast<LockShaderTypeRenderCommand *>(cmd);

				if (b->m_Lock)
					BSShader::LockShader(b->m_LockIndex);
				else
					BSShader::UnlockShader(b->m_LockIndex);
			}
			break;

			case 5:
				static_cast<SetAccumulatorRenderCommand *>(cmd)->Run();
				break;

			case 6:
				//static_cast<Setsub_14131E960 *>(cmd)->Run();
				break;

			case 7:
				static_cast<DrawGeometryMultiRenderCommand *>(cmd)->Run();
				break;

			default:
				__debugbreak();
				break;
			}

			ProfileCounterAdd("Command Count", cmdCount);
		}

		testmtr = false;
	}

	void ClearShaderAndTechnique()
	{
		if (IsGeneratingGameCommandList())
			MTRenderer::InsertCommand<MTRenderer::ClearStateRenderCommand>();
		else
			::ClearShaderAndTechnique();
	}

	void RasterStateSetCullMode(uint32_t CullMode)
	{
		if (IsGeneratingGameCommandList())
			MTRenderer::InsertCommand<MTRenderer::SetStateRenderCommand>(MTRenderer::SetStateRenderCommand::RasterStateCullMode, CullMode);
		else
			BSGraphics::Renderer::GetGlobals()->RasterStateSetCullMode(CullMode);
	}

	void AlphaBlendStateSetUnknown1(uint32_t Value)
	{
		if (IsGeneratingGameCommandList())
			MTRenderer::InsertCommand<MTRenderer::SetStateRenderCommand>(MTRenderer::SetStateRenderCommand::AlphaBlendStateUnknown1, Value);
		else
			BSGraphics::Renderer::GetGlobals()->AlphaBlendStateSetUnknown1(Value);
	}
}