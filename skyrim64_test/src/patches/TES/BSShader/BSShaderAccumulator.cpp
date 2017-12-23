#include "../../rendering/common.h"
#include "../../../common.h"
#include "../BSGraphicsRenderer.h"
#include "../BSBatchRenderer.h"
#include "BSShaderManager.h"
#include "BSShaderAccumulator.h"
#include "../BSReadWriteLock.h"

extern ID3DUserDefinedAnnotation *annotation;

uintptr_t commandDataStart[6];
uintptr_t commandData[6];
thread_local int ThreadUsingCommandList;

void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int), int Index);
void DC_WaitDeferred(int Index);

BSReadWriteLock testLocks[32];

STATIC_CONSTRUCTOR(__Testing,
[]{
	//for (int i = 0; i < 32; i++)
	//	InitializeSRWLock(&testLocks[i]);

	for (int i = 0; i < ARRAYSIZE(commandData); i++)
	{
		commandData[i] = (uintptr_t)VirtualAlloc(nullptr, 1 * 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		commandDataStart[i] = commandData[i];

		if (!commandData[i])
			__debugbreak();
	}
})

static void /*BSShaderManager::*/SetCurrentAccumulator(BSShaderAccumulator *Accumulator)
{
	// We always run the full function because I'm not sure if the structure
	// is used anywhere else important.
	InsertRenderCommand<SetAccumulatorRenderCommand>(Accumulator);

	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	// BSShaderManager::pCurrentShaderAccumulator
	uint64_t& qword_1431F5490 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3600);

	qword_1431F5490 = (uint64_t)Accumulator;
}

void sub_14131F090(bool RequireZero)
{
	if (InsertRenderCommand<ClearStateRenderCommand>())
		return;

	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	if (qword_1432A8218)
	{
		if (RequireZero)
			__debugbreak();

		(*(void(__fastcall **)(__int64, uint64_t))(*(uint64_t *)qword_1432A8218 + 24i64))(
			qword_1432A8218,
			(unsigned int)dword_1432A8214);
	}

	qword_1432A8218 = 0i64;
	dword_1432A8214 = 0;
	qword_1434B5220 = 0i64;
}

void DoRenderCommands(int Index)
{
	DC_RenderDeferred(0, Index, [](long long, unsigned int arg2)
	{
		ProfileTimer("Generate Lists");

		// Run everything in the command list...
		bool endOfList = false;

		for (uintptr_t ptr = commandDataStart[arg2]; !endOfList;)
		{
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
					testLocks[b->m_LockIndex].AcquireWrite();
					//AcquireSRWLockExclusive(&testLocks[b->m_LockIndex]);
				else
					testLocks[b->m_LockIndex].ReleaseWrite();
					//ReleaseSRWLockExclusive(&testLocks[b->m_LockIndex]);
			}
			break;

			case 5:
				static_cast<SetAccumulatorRenderCommand *>(cmd)->Run();
				break;

			default:
				__debugbreak();
				break;
			}
		}
	}, Index);
}

void DoRenderCommandsNOW(int Index)
{
	// Run everything in the command list...
	bool endOfList = false;

	for (uintptr_t ptr = commandDataStart[Index]; !endOfList;)
	{
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
				testLocks[b->m_LockIndex].AcquireWrite();
			//AcquireSRWLockExclusive(&testLocks[b->m_LockIndex]);
			else
				testLocks[b->m_LockIndex].ReleaseWrite();
			//ReleaseSRWLockExclusive(&testLocks[b->m_LockIndex]);
		}
		break;

		case 5:
			static_cast<SetAccumulatorRenderCommand *>(cmd)->Run();
			break;

		default:
			__debugbreak();
			break;
		}
	}
}

#include <functional>

class GameCommandList
{
protected:
	int m_Index;

public:
	GameCommandList(int Index, std::function<void()> ListBuildFunction) : m_Index(Index)
	{
		ThreadUsingCommandList = m_Index + 1;
		commandData[m_Index] = commandDataStart[m_Index];

		if (ListBuildFunction)
			ListBuildFunction();

		InsertRenderCommand<EndListRenderCommand>();
		ThreadUsingCommandList = 0;
	}

	void Wait()
	{
		DoRenderCommandsNOW(m_Index);
	}
};

class DeferredCommandList : public GameCommandList
{
public:
	DeferredCommandList(int Index, std::function<void()> ListBuildFunction) : GameCommandList(Index, ListBuildFunction)
	{
		DoRenderCommands(m_Index);
	}

	void Wait()
	{
		DC_WaitDeferred(m_Index);
	}
};

void BSShaderAccumulator::sub_1412E1600(__int64 a1, unsigned int a2, float a3)
{
	auto accumulator = (BSShaderAccumulator *)a1;
	auto graphicsGlobals = GetThreadedGlobals();

	if (!accumulator->m_pkCamera)
		return;

	if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5))
		BSGraphics::Renderer::DepthStencilStateSetDepthMode(4);

	// v7 = RenderDepthOnly()? RenderAlphaOnly()?
	bool v7 = (a2 & 0xA) != 0;

	if (!v7)
		((void(__fastcall *)())(g_ModuleBase + 0x12F8B10))();

	//
	// Original draw order:
	//
	// RenderBatches
	// LowAniso
	// RenderGrass
	// RenderNoShadowGroup
	// RenderLODObjects
	// RenderLODLand
	// RenderSky
	// RenderSkyClouds
	// ???
	// BlendedDecals
	// RenderWaterStencil
	//

	// RenderBatches
	GameCommandList renderBatches(0, [accumulator, a2]
	{
		accumulator->RenderTechniques(1, BSSM_DISTANTTREE_DEPTH, a2, -1);
	});

	// LowAniso
	DeferredCommandList lowAniso(1, [accumulator, a2]
	{
		ProfileTimer("LowAniso");

		auto pass = accumulator->m_MainBatch->m_Passes[9];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(a2);
			else
				accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 9);
		}
	});

	// RenderGrass
	DeferredCommandList renderGrass(2, [accumulator, a2]
	{
		ProfileTimer("RenderGrass");

		accumulator->RenderTechniques(BSSM_GRASS_DIRONLY_LF, 0x5C00005C, a2, -1);
	});

	// RenderNoShadowGroup
	DeferredCommandList renderNoShadowGroup(3, [accumulator, a2]
	{
		ProfileTimer("RenderNoShadowGroup");

		auto pass = accumulator->m_MainBatch->m_Passes[8];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(a2);
			else
				accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 8);
		}
	});

	// RenderLODObjects
	DeferredCommandList renderLODObjects(4, [accumulator, a1, a2]
	{
		ProfileTimer("LOD");

		auto pass = accumulator->m_MainBatch->m_Passes[1];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(a2);
			else
				accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 1);
		}

		if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5))
			BSGraphics::Renderer::DepthStencilStateSetDepthMode(3);
	});

	{
		ProfileTimer("RenderBatches");
		renderBatches.Wait();
	}

	{
		ProfileTimer("DC_WaitDeferred");
		lowAniso.Wait();
		renderGrass.Wait();
		renderNoShadowGroup.Wait();
		renderLODObjects.Wait();
	}

	// RenderLODLand
	annotation->BeginEvent(L"RenderLODLand");
	{
		ProfileTimer("LOD");

		auto pass = accumulator->m_MainBatch->m_Passes[0];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(a2);
			else
				accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 0);
		}

		if (!v7)
			((void(__fastcall *)())(g_ModuleBase + 0x12F8C70))();
	}
	annotation->EndEvent();

	// RenderSky
	annotation->BeginEvent(L"RenderSky");
	{
		BSGraphics::Renderer::SetUseScrapConstantValue(true, 0.50196081f);
		accumulator->RenderTechniques(BSSM_SKYBASEPRE, BSSM_SKY_CLOUDSFADE, a2, -1);
	}
	annotation->EndEvent();

	// RenderSkyClouds
	annotation->BeginEvent(L"RenderSkyClouds");
	{
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(11);

		auto pass = accumulator->m_MainBatch->m_Passes[13];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(a2);
			else
				accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 13);
		}

		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(1);
	}
	annotation->EndEvent();

	if (!v7)
		((void(__fastcall *)())(g_ModuleBase + 0x12F8B10))();

	// NormalDecals?...CK doesn't have a specific name for this
	{
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(10);
		((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E27B0))(a1, a2);
	}

	// BlendedDecals
	annotation->BeginEvent(L"BlendedDecals");
	{
		BSGraphics::Renderer::AlphaBlendStateSetUnknown2(11);
		((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E2950))(a1, a2);
	}
	annotation->EndEvent();

	BSGraphics::Renderer::AlphaBlendStateSetMode(0);
	BSGraphics::Renderer::AlphaBlendStateSetUnknown2(1);

	auto sub_140D744B0 = (int(__fastcall *)())(g_ModuleBase + 0xD744E0);
	auto sub_140D69E70 = (__int64(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0xD69EA0);
	auto sub_140D69D30 = (__int64(__fastcall *)(float *a1, float a2, float a3, float a4, int a5))(g_ModuleBase + 0xD69D60);
	auto sub_1412FD120 = (signed __int64(__fastcall *)())(g_ModuleBase + 0x12FD480);
	auto sub_140D74350 = (__int64(__fastcall *)(__int64 a1, unsigned int a2, int a3, int a4, char a5))(g_ModuleBase + 0xD74380);
	auto sub_140D74370 = (void(__fastcall *)(__int64 a1, uint32_t a2, int a3, uint32_t a4))(g_ModuleBase + 0xD743A0);
	auto sub_140D69990 = (void(__fastcall *)(__int64 a1, char a2))(g_ModuleBase + 0xD699C0);
	auto sub_1412FADA0 = (__int64(__fastcall *)())(g_ModuleBase + 0x12FB100);
	auto sub_140D69DA0 = (void(__fastcall *)(DWORD *a1))(g_ModuleBase + 0xD69DD0);

	DWORD *flt_14304E490 = (DWORD *)(g_ModuleBase + 0x304E490);

	if ((a2 & 0x80u) != 0 && accumulator->m_MainBatch->HasTechniquePasses(0x5C000071, 0x5C006071))
	{
		int aiSource = sub_140D744B0();

		//	bAssert(aiSource != aiTarget &&
		//		aiSource < DEPTH_STENCIL_COUNT &&
		//		aiTarget < DEPTH_STENCIL_COUNT &&
		//		aiSource != DEPTH_STENCIL_TARGET_NONE &&
		//		aiTarget != DEPTH_STENCIL_TARGET_NONE);

		graphicsGlobals->m_DeviceContext->CopyResource(*(ID3D11Resource **)(g_ModuleBase + 0x3050870), *((ID3D11Resource **)flt_14304E490 + 19 * aiSource + 1015));
	}

	// RenderWaterStencil
	annotation->BeginEvent(L"RenderWaterStencil");
	{
		if (accumulator->m_MainBatch->HasTechniquePasses(BSSM_WATER_STENCIL, BSSM_WATER_DISPLACEMENT_STENCIL_Vc))
		{
			sub_140D69E70((__int64)flt_14304E490, 2u);
			sub_140D69D30((float *)flt_14304E490, 0.0, 0.0, 0.0, 0);
			__int64 v20 = sub_1412FD120();
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 0, v20, 3, 1);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 1u, 7, 3, 1);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 2u, -1, 3, 1);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 3u, -1, 3, 1);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 4u, -1, 3, 1);
			accumulator->RenderTechniques(BSSM_WATER_STENCIL, BSSM_WATER_DISPLACEMENT_STENCIL_Vc, a2, -1);
			sub_140D69DA0((DWORD *)flt_14304E490);
			*(DWORD *)(*(uint64_t *)((*(uint64_t*)(g_ModuleBase + 0x31F5810)) + 496) + 44i64) = 1;
		}
	}
	annotation->EndEvent();

	if (a2 & 0x40)
	{
		sub_140D74370((__int64)(g_ModuleBase + 0x3051B20), 0xFFFFFFFF, 3, 0);
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 1u, -1, 3, 1);
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 2u, -1, 3, 1);
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 3u, -1, 3, 1);
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 4u, -1, 3, 1);
		sub_140D69990((__int64)flt_14304E490, 1);
		sub_1412FADA0();
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 0, 1, 3, 1);
		int v21 = sub_140D744B0();
		sub_140D74370((__int64)(g_ModuleBase + 0x3051B20), v21, 3, 0);

		BSGraphics::Renderer::DepthStencilStateSetDepthMode(1);
	}

	if (!v7)
		((void(__fastcall *)())(g_ModuleBase + 0x12F8C70))();
}

void BSShaderAccumulator::RenderTechniques(uint32_t StartTechnique, uint32_t EndTechnique, int a4, int PassType)
{
	//auto RenderTechniques = (__int64(__fastcall *)(__int64 a1, int a2, int a3, int a4, int a5))(g_ModuleBase + 0x12E3AD0);
	__int64 a1 = (__int64)this;

	BSBatchRenderer::PassInfo *subPass = nullptr;
	BSBatchRenderer *batch = nullptr;

	SetCurrentAccumulator(this);

	if (PassType <= -1)
	{
		// Render EVERYTHING with the given techniques
		subPass = nullptr;
		batch = this->m_MainBatch;
	}
	else
	{
		// Render a single sub-pass with given techniques
		subPass = this->m_MainBatch->m_Passes[PassType];
		batch = subPass->m_BatchRenderer;
	}

	__int64 v14;
	m_CurrentTech = 0;

	if (batch)
	{
		batch->m_StartingTech = StartTechnique;
		batch->m_EndingTech = EndTechnique;

		m_CurrentSubPass = 0;
		v14 = (__int64)batch + 88;

		*(BYTE *)(a1 + 320) = batch->sub_14131E700(m_CurrentTech, m_CurrentSubPass, (__int64)&v14);
	}
	else
	{
		*(BYTE *)(a1 + 320) = 0;
	}

	if (*(BYTE *)(a1 + 320))
	{
		bool v12;

		do
		{
			if ((unsigned int)(m_CurrentTech - BSSM_GRASS_SHADOW_L) <= 3 && (*(BYTE *)(a1 + 296) || *(BYTE *)(a1 + 297)))// if (is grass shadow) ???
				v12 = batch->sub_14131ECE0(m_CurrentTech, m_CurrentSubPass, (__int64)&v14);// Probably discards pass, returns true if there's remaining sub passes
			else
				v12 = batch->sub_14131E960(m_CurrentTech, m_CurrentSubPass, (__int64)&v14, a4);

			*(BYTE *)(a1 + 320) = v12;
		} while (v12);
	}

	if (subPass)
		subPass->Unregister();

	sub_14131F090(true);
}