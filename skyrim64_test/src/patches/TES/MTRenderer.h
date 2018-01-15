#pragma once

#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSBatchRenderer.h"
#include "BSReadWriteLock.h"

extern uintptr_t commandDataStart[6];
extern uintptr_t commandData[6];
extern thread_local int ThreadUsingCommandList;

namespace MTRenderer
{
	bool IsInMultithreadedContext();
	void ExecuteCommandList(int Index);

	void ClearShaderAndTechnique();
	void RasterStateSetCullMode(uint32_t CullMode);
	void AlphaBlendStateSetUnknown1(uint32_t Value);

	struct RenderCommand
	{
		int m_Type;
		int m_Size;

		RenderCommand(int Type, int Size)
		{
			m_Type = Type;
			m_Size = Size;
		}
	};

	struct EndListRenderCommand : RenderCommand
	{
		// Terminates execution sequence
		EndListRenderCommand()
			: RenderCommand(0, sizeof(EndListRenderCommand))
		{
		}
	};

	struct ClearStateRenderCommand : RenderCommand
	{
		// Equivalent to calling ClearShaderAndTechnique();
		ClearStateRenderCommand()
			: RenderCommand(1, sizeof(ClearStateRenderCommand))
		{
		}

		void Run()
		{
			ClearShaderAndTechnique();
		}
	};

	struct SetStateRenderCommand : RenderCommand
	{
		// Equivalent to BSGraphics::Renderer::SetXYZ functions
		enum StateVar
		{
			RasterStateCullMode,
			AlphaBlendStateMode,
			AlphaBlendStateUnknown1,
			AlphaBlendStateUnknown2,
			DepthStencilStateStencilMode,
			DepthStencilStateDepthMode,
			UseScrapConstantValue_1,
			UseScrapConstantValue_2,
		};

		union
		{
			struct
			{
				uint32_t part1;
				uint32_t part2;
			};

			__int64 all;
		} Data;

		StateVar m_StateType;

		SetStateRenderCommand(StateVar Type, uint32_t Arg1 = 0, uint32_t Arg2 = 0)
			: RenderCommand(2, sizeof(SetStateRenderCommand))
		{
			m_StateType = Type;
			Data.part1 = Arg1;
			Data.part2 = Arg2;

			switch (m_StateType)
			{
			case RasterStateCullMode:
			case AlphaBlendStateMode:
			case AlphaBlendStateUnknown1:
			case AlphaBlendStateUnknown2:
			case DepthStencilStateStencilMode:
			case DepthStencilStateDepthMode:
			case UseScrapConstantValue_1:
			case UseScrapConstantValue_2:
				break;

			default:
				__debugbreak();
			}
		}

		void Run()
		{
			auto *r = BSGraphics::Renderer::GetGlobals();

			switch (m_StateType)
			{
			case RasterStateCullMode:
				r->RasterStateSetCullMode(Data.part1);
				break;

			case AlphaBlendStateMode:
				r->AlphaBlendStateSetMode(Data.part1);
				break;

			case AlphaBlendStateUnknown1:
				r->AlphaBlendStateSetUnknown1(Data.part1);
				break;

			case AlphaBlendStateUnknown2:
				r->AlphaBlendStateSetUnknown2(Data.part1);
				break;

			case DepthStencilStateStencilMode:
				r->DepthStencilStateSetStencilMode(Data.part1, Data.part2);
				break;

			case DepthStencilStateDepthMode:
				r->DepthStencilStateSetDepthMode(Data.part1);
				break;

			case UseScrapConstantValue_1:
				r->SetUseScrapConstantValue((bool)Data.part1);
				break;

			case UseScrapConstantValue_2:
				r->SetUseScrapConstantValue(*(float *)&Data.part1);
				break;

			default:
				__debugbreak();
			}
		}
	};

	struct DrawGeometryRenderCommand : RenderCommand
	{
		// Equivalent to calling DrawPassGeometry();
		BSRenderPass Pass;
		uint32_t Technique;
		unsigned __int8 a3;
		unsigned int a4;

		DrawGeometryRenderCommand(BSRenderPass *Arg1, uint32_t Arg2, unsigned __int8 Arg3, uint32_t Arg4)
			: RenderCommand(3, sizeof(DrawGeometryRenderCommand))
		{
			memcpy(&Pass, Arg1, sizeof(BSRenderPass));
			Technique = Arg2;
			a3 = Arg3;
			a4 = Arg4;
		}

		void Run()
		{
			//
			// **** WARNING ****
			// The game code shouldn't really be using this pointer after the initial batching
			// stage, but just to make sure...
			//
			if (Pass.m_Next)
				Pass.m_Next = (BSRenderPass *)0xCCCCCCCCDEADBEEF;

			if (Pass.m_Shader->m_Type == 0 || Pass.m_Shader->m_Type == 5)
				__debugbreak();
			/*
			if (Pass.m_Shader->m_Type == 0 ||	// ???
			Pass.m_Shader->m_Type == 1 ||	// RunGrass
			Pass.m_Shader->m_Type == 2 ||	// Sky
			Pass.m_Shader->m_Type == 3 ||	// Water
			Pass.m_Shader->m_Type == 4 ||	// BloodSplatter
			Pass.m_Shader->m_Type == 5 ||	// ???
			Pass.m_Shader->m_Type == 6 ||	// Lighting
			Pass.m_Shader->m_Type == 7 ||	// Effect
			Pass.m_Shader->m_Type == 8 ||	// Utility
			Pass.m_Shader->m_Type == 9 ||	// DistantTree
			Pass.m_Shader->m_Type == 10)	// Particle
			return;
			*/

			BSBatchRenderer::DrawPassGeometry(&Pass, Technique, a3, a4);
		}
	};

	struct DrawGeometryMultiRenderCommand : RenderCommand
	{
		// Equivalent to calling DrawPassGeometry();
		BSRenderPass Pass[3];
		uint32_t Technique;
		unsigned __int8 a3;
		unsigned int a4;

		DrawGeometryMultiRenderCommand(BSRenderPass **Arg1, uint32_t Arg2, unsigned __int8 Arg3, uint32_t Arg4)
			: RenderCommand(7, sizeof(DrawGeometryMultiRenderCommand))
		{
			memcpy(&Pass[0], Arg1[0], sizeof(BSRenderPass));
			memcpy(&Pass[1], Arg1[1], sizeof(BSRenderPass));
			memcpy(&Pass[2], Arg1[2], sizeof(BSRenderPass));
			Technique = Arg2;
			a3 = Arg3;
			a4 = Arg4;
		}

		void Run()
		{
			//
			// **** WARNING ****
			// The game code shouldn't really be using this pointer after the initial batching
			// stage, but just to make sure...
			//
			Pass[0].m_Next = (BSRenderPass *)0xCCCCCCCCDEADBEEF;
			Pass[1].m_Next = (BSRenderPass *)0xCCCCCCCCDEADBEEF;
			Pass[2].m_Next = (BSRenderPass *)0xCCCCCCCCDEADBEEF;

			/*
			if (Pass.m_Shader->m_Type == 0 ||	// ???
			Pass.m_Shader->m_Type == 1 ||	// RunGrass
			Pass.m_Shader->m_Type == 2 ||	// Sky
			Pass.m_Shader->m_Type == 3 ||	// Water
			Pass.m_Shader->m_Type == 4 ||	// BloodSplatter
			Pass.m_Shader->m_Type == 5 ||	// ???
			Pass.m_Shader->m_Type == 6 ||	// Lighting
			Pass.m_Shader->m_Type == 7 ||	// Effect
			Pass.m_Shader->m_Type == 8 ||	// Utility
			Pass.m_Shader->m_Type == 9 ||	// DistantTree
			Pass.m_Shader->m_Type == 10)	// Particle
			return;
			*/

			BSBatchRenderer::DrawPassGeometry(&Pass[0], Technique, a3, a4);
			BSBatchRenderer::DrawPassGeometry(&Pass[1], Technique, a3, a4);
			BSBatchRenderer::DrawPassGeometry(&Pass[2], Technique, a3, a4);
		}
	};

	struct LockShaderTypeRenderCommand : RenderCommand
	{
		int m_LockIndex;
		bool m_Lock;

		LockShaderTypeRenderCommand(int LockIndex, bool Lock)
			: RenderCommand(4, sizeof(LockShaderTypeRenderCommand))
		{
			m_LockIndex = LockIndex;
			m_Lock = Lock;
		}
	};

	struct SetAccumulatorRenderCommand : RenderCommand
	{
		char data[sizeof(BSShaderAccumulator)];

		SetAccumulatorRenderCommand(BSShaderAccumulator *Accumulator)
			: RenderCommand(5, sizeof(SetAccumulatorRenderCommand))
		{
			memcpy(data, Accumulator, sizeof(BSShaderAccumulator));

			//
			// **** WARNING ****
			// The game code shouldn't really be using this pointer after the initial batching
			// stage, but just to make sure...
			//
			if (Accumulator->m_MainBatch)
				((BSShaderAccumulator *)&data)->m_MainBatch = (BSBatchRenderer *)0xCCCCCCCCDEADBEEF;
		}

		void Run()
		{
			auto GraphicsGlobals = HACK_GetThreadedGlobals();

			// BSShaderManager::pCurrentShaderAccumulator
			uint64_t& qword_1431F5490 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3600);

			qword_1431F5490 = (uint64_t)&data;
		}
	};

	template<typename T, class ... Types>
	bool InsertCommand(Types&& ...args)
	{
		static_assert(sizeof(T) % 8 == 0);

		if (!ThreadUsingCommandList)
			return false;

		// Utilize placement new, then increment to next command slot
		new ((void *)commandData[ThreadUsingCommandList - 1]) T(args...);
		commandData[ThreadUsingCommandList - 1] += sizeof(T);

		return true;
	}
}