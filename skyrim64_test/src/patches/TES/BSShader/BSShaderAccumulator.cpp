#include "../../rendering/common.h"
#include "../../../common.h"
#include "../BSBatchRenderer.h"
#include "BSShaderManager.h"
#include "BSShaderAccumulator.h"

extern ID3DUserDefinedAnnotation *annotation;
SRWLOCK srwtest = SRWLOCK_INIT;

uintptr_t commandDataStart[3];
uintptr_t commandData[3];
thread_local int ThreadUsingCommandList;

void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int), int Index);
void DC_WaitDeferred(int Index);

SRWLOCK testLocks[64];

STATIC_CONSTRUCTOR(__Testing,
[]{
	for (int i = 0; i < 64; i++)
		InitializeSRWLock(&testLocks[i]);

	commandData[0] = (uintptr_t)VirtualAlloc(nullptr, 1 * 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	commandDataStart[0] = commandData[0];

	commandData[1] = (uintptr_t)VirtualAlloc(nullptr, 1 * 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	commandDataStart[1] = commandData[1];

	commandData[2] = (uintptr_t)VirtualAlloc(nullptr, 1 * 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	commandDataStart[2] = commandData[2];

	if (!commandData[0] || !commandData[1] || !commandData[2])
		__debugbreak();
})

static void /*BSShaderManager::*/SetCurrentAccumulator(BSShaderAccumulator *Accumulator)
{
	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	// BSShaderManager::pCurrentShaderAccumulator
	uint64_t& qword_1431F5490 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3600);

	qword_1431F5490 = (uint64_t)Accumulator;
}

void sub_14131F090()
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
		(*(void(__fastcall **)(__int64, uint64_t))(*(uint64_t *)qword_1432A8218 + 24i64))(
			qword_1432A8218,
			(unsigned int)dword_1432A8214);
	}

	qword_1432A8218 = 0i64;
	dword_1432A8214 = 0;
	qword_1434B5220 = 0i64;
}

void BSGraphics__Renderer__RasterStateSetCullMode(uint32_t CullMode)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::RasterStateCullMode, CullMode))
		return;

	auto *renderer = GetThreadedGlobals();

	if (*(DWORD *)&renderer->__zz0[52] != CullMode)
	{
		*(DWORD *)&renderer->__zz0[52] = CullMode;
		renderer->dword_14304DEB0 |= 0x20;
	}
}

void BSGraphics__Renderer__AlphaBlendStateSetMode(uint32_t Mode)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateMode, Mode))
		return;

	auto *renderer = GetThreadedGlobals();

	if (*(DWORD *)&renderer->__zz0[64] != Mode)
	{
		*(DWORD *)&renderer->__zz0[64] = Mode;
		renderer->dword_14304DEB0 |= 0x80;
	}
}

void BSGraphics__Renderer__AlphaBlendStateSetUnknown1(uint32_t Value)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateUnknown1, Value))
		return;

	auto *renderer = GetThreadedGlobals();

	if (*(DWORD *)&renderer->__zz0[68] != Value)
	{
		*(DWORD *)&renderer->__zz0[68] = Value;
		renderer->dword_14304DEB0 |= 0x80;
	}
}

void BSGraphics__Renderer__AlphaBlendStateSetUnknown2(uint32_t Value)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateUnknown2, Value))
		return;

	auto *renderer = GetThreadedGlobals();

	if (*(DWORD *)&renderer->__zz0[72] != Value)
	{
		*(DWORD *)&renderer->__zz0[72] = Value;
		renderer->dword_14304DEB0 |= 0x80;
	}
}

void BSGraphics__Renderer__DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::DepthStencilStateStencilMode, Mode, StencilRef))
		return;

	auto *renderer = GetThreadedGlobals();

	if (*(DWORD *)&renderer->__zz0[40] != Mode || *(DWORD *)&renderer->__zz0[44] != StencilRef)
	{
		*(DWORD *)&renderer->__zz0[40] = Mode;
		*(DWORD *)&renderer->__zz0[44] = StencilRef;
		renderer->dword_14304DEB0 |= 0x8;
	}
}

void BSGraphics__Renderer__DepthStencilStateSetDepthMode(uint32_t Mode)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::DepthStencilStateDepthMode, Mode))
		return;

	auto *renderer = GetThreadedGlobals();

	if (*(DWORD *)&renderer->__zz0[32] != Mode)
	{
		*(DWORD *)&renderer->__zz0[32] = Mode;

		// Temp var to prevent duplicate state setting? Don't know where this gets set.
		if (*(DWORD *)&renderer->__zz0[36] != Mode)
			renderer->dword_14304DEB0 |= 0x4;
		else
			renderer->dword_14304DEB0 &= ~0x4;
	}
}

void BSGraphics__Renderer__SetTextureFilterMode(uint32_t Index, uint32_t Mode)
{
	auto *renderer = GetThreadedGlobals();

	if (renderer->m_PSSamplerSetting2[Index] != Mode)
	{
		renderer->m_PSSamplerSetting2[Index] = Mode;
		renderer->m_PSSamplerModifiedBits |= 1 << Index;
	}
}

void BSGraphics__Renderer__SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode)
{
	auto *renderer = GetThreadedGlobals();

	if (renderer->m_PSSamplerSetting1[Index] != AddressMode)
	{
		renderer->m_PSSamplerSetting1[Index] = AddressMode;
		renderer->m_PSSamplerModifiedBits |= 1 << Index;
	}

	if (renderer->m_PSSamplerSetting2[Index] != FilterMode)
	{
		renderer->m_PSSamplerSetting2[Index] = FilterMode;
		renderer->m_PSSamplerModifiedBits |= 1 << Index;
	}
}

void BSGraphics__Renderer__SetUseScrapConstantValue(bool UseStoredValue)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::UseScrapConstantValue_1, UseStoredValue))
		return;

	auto *renderer = GetThreadedGlobals();

	// When UseStoredValue is false, the constant buffer data is zeroed, but float_14304DF68 is saved
	if (renderer->__zz0[76] != UseStoredValue)
	{
		renderer->__zz0[76] = UseStoredValue;
		renderer->dword_14304DEB0 |= 0x100u;
	}
}

void BSGraphics__Renderer__SetUseScrapConstantValue(bool UseStoredValue, float Value)
{
	if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::UseScrapConstantValue_2, UseStoredValue, *(uint32_t *)&Value))
		return;

	auto *renderer = GetThreadedGlobals();

	if (renderer->__zz0[76] != UseStoredValue)
	{
		renderer->__zz0[76] = UseStoredValue;
		renderer->dword_14304DEB0 |= 0x100u;
	}

	if (renderer->float_14304DF68 != Value)
	{
		renderer->float_14304DF68 = Value;
		renderer->dword_14304DEB0 |= 0x200u;
	}
}

void DoRenderCommands(int Index)
{
	DC_RenderDeferred(0, Index, [](long long, unsigned int arg2)
	{
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
					AcquireSRWLockExclusive(&testLocks[b->m_LockIndex]);
				else
					ReleaseSRWLockExclusive(&testLocks[b->m_LockIndex]);
			}
			break;

			default:
				__debugbreak();
				break;
			}
		}
	}, Index);
}

void BSShaderAccumulator::sub_1412E1600(__int64 a1, unsigned int a2, float a3)
{
	auto accumulator = (BSShaderAccumulator *)a1;
	auto graphicsGlobals = GetThreadedGlobals();

	if (!accumulator->m_pkCamera)
		return;

	if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5))
		BSGraphics__Renderer__DepthStencilStateSetDepthMode(4);

	// v7 = RenderDepthOnly()? RenderAlphaOnly()?
	bool v7 = (a2 & 0xA) != 0;

	if (!v7)
		((void(__fastcall *)())(g_ModuleBase + 0x12F8B10))();

	// RenderBatches
	{

		ThreadUsingCommandList = 1;
		commandData[0] = commandDataStart[0];

		//annotation->BeginEvent(L"RenderBatches");
		{
			ProfileTimer("RenderBatches");

			accumulator->RenderTechniques(1, BSSM_DISTANTTREE_DEPTH, a2, -1);
		}
		//annotation->EndEvent();

		InsertRenderCommand<EndListRenderCommand>();
		ThreadUsingCommandList = 0;

		SetCurrentAccumulator(accumulator);
		DoRenderCommands(0);
	}

	// LowAniso
	{
		ThreadUsingCommandList = 2;
		commandData[1] = commandDataStart[1];

		//annotation->BeginEvent(L"LowAniso");
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
		}
		//annotation->EndEvent();

		InsertRenderCommand<EndListRenderCommand>();
		ThreadUsingCommandList = 0;

		SetCurrentAccumulator(accumulator);
		DoRenderCommands(1);
	}

	// RenderGrass
	{
		ThreadUsingCommandList = 3;
		commandData[2] = commandDataStart[2];

		//annotation->BeginEvent(L"RenderGrass");
		{
			ProfileTimer("RenderGrass");

			accumulator->RenderTechniques(BSSM_GRASS_DIRONLY_LF, 0x5C00005C, a2, -1);
		}
		//annotation->EndEvent();

		InsertRenderCommand<EndListRenderCommand>();
		ThreadUsingCommandList = 0;

		SetCurrentAccumulator(accumulator);
		DoRenderCommands(2);
	}

	{
		ProfileTimer("DC_WaitDeferred");

		DC_WaitDeferred(0);
		DC_WaitDeferred(1);
		DC_WaitDeferred(2);
	}

	// RenderNoShadowGroup
	annotation->BeginEvent(L"RenderNoShadowGroup");
	{
		auto pass = accumulator->m_MainBatch->m_Passes[8];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(a2);
			else
				accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 8);
		}
	}
	annotation->EndEvent();

	// RenderLODObjects
	annotation->BeginEvent(L"RenderLODObjects");
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
			BSGraphics__Renderer__DepthStencilStateSetDepthMode(3);
	}
	annotation->EndEvent();

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
		BSGraphics__Renderer__SetUseScrapConstantValue(true, 0.50196081f);
		accumulator->RenderTechniques(BSSM_SKYBASEPRE, BSSM_SKY_CLOUDSFADE, a2, -1);
	}
	annotation->EndEvent();

	// RenderSkyClouds
	annotation->BeginEvent(L"RenderSkyClouds");
	{
		BSGraphics__Renderer__AlphaBlendStateSetUnknown2(11);

		auto pass = accumulator->m_MainBatch->m_Passes[13];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(a2);
			else
				accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 13);
		}

		BSGraphics__Renderer__AlphaBlendStateSetUnknown2(1);
	}
	annotation->EndEvent();

	if (!v7)
		((void(__fastcall *)())(g_ModuleBase + 0x12F8B10))();

	// NormalDecals?...CK doesn't have a specific name for this
	{
		BSGraphics__Renderer__AlphaBlendStateSetUnknown2(10);
		((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E27B0))(a1, a2);
	}

	// BlendedDecals
	annotation->BeginEvent(L"BlendedDecals");
	{
		BSGraphics__Renderer__AlphaBlendStateSetUnknown2(11);
		((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E2950))(a1, a2);
	}
	annotation->EndEvent();

	BSGraphics__Renderer__AlphaBlendStateSetMode(0);
	BSGraphics__Renderer__AlphaBlendStateSetUnknown2(1);

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

		BSGraphics__Renderer__DepthStencilStateSetDepthMode(1);
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

	AcquireSRWLockExclusive(&srwtest);
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
	this->m_CurrentTech = 0;

	if (batch)
	{
		batch->m_StartingTech = StartTechnique;
		batch->m_EndingTech = EndTechnique;

		*(DWORD *)(a1 + 316) = 0;
		v14 = (__int64)batch + 88;
		*(BYTE *)(a1 + 320) = batch->sub_14131E700(this->m_CurrentTech, *(uint32_t *)(a1 + 316), (__int64)&v14);
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
			if ((unsigned int)(this->m_CurrentTech - BSSM_GRASS_SHADOW_L) <= 3 && (*(BYTE *)(a1 + 296) || *(BYTE *)(a1 + 297)))// if (is grass shadow) ???
				v12 = batch->sub_14131ECE0(this->m_CurrentTech, *(uint32_t *)(a1 + 316), (__int64)&v14);// Probably discards pass, returns true if there's remaining sub passes
			else
				v12 = batch->sub_14131E960(this->m_CurrentTech, *(uint32_t *)(a1 + 316), (__int64)&v14, a4);

			*(BYTE *)(a1 + 320) = v12;
		} while (v12);
	}

	if (subPass)
		subPass->Unregister();

	sub_14131F090();
	ReleaseSRWLockExclusive(&srwtest);
}