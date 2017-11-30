#include "../rendering/common.h"
#include "../../common.h"
#include "BSShaderManager.h"
#include "BSBatchRenderer.h"
#include "BSShaderAccumulator.h"

extern ID3DUserDefinedAnnotation *annotation;
SRWLOCK srwtest = SRWLOCK_INIT;

void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int));
void DC_WaitDeferred();

static void sub_1412ACFE0(__int64 a1)
{
	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint64_t& qword_1431F5490 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3600);

	qword_1431F5490 = a1;
}

static void sub_14131F090()
{
	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	if (qword_1432A8218)
		(*(void(__fastcall **)(__int64, uint64_t))(*(uint64_t *)qword_1432A8218 + 24i64))(
			qword_1432A8218,
			(unsigned int)dword_1432A8214);

	qword_1432A8218 = 0i64;
	dword_1432A8214 = 0;
	qword_1434B5220 = 0i64;
}

void BSShaderAccumulator::sub_1412E1600(__int64 a1, unsigned int a2, float a3)
{
	// a1 = BSShaderAccumulator
	auto accumulator = (BSShaderAccumulator *)a1;

	uint32_t v6; // ecx
	bool v7; // si
	uint32_t v18; // eax
	int v20; // eax

	auto graphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	if (*(uint64_t *)(a1 + 16))
	{
		if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5) && *(DWORD *)&graphicsGlobals->__zz0[32] != 4)
		{
			*(DWORD *)&graphicsGlobals->__zz0[32] = 4;
			v6 = graphicsGlobals->dword_14304DEB0 & 0xFFFFFFFB;
			if (*(DWORD *)&graphicsGlobals->__zz0[36] != 4)
				v6 = graphicsGlobals->dword_14304DEB0 | 4;
			graphicsGlobals->dword_14304DEB0 = v6;
		}
		v7 = (a2 & 0xA) != 0;

		if (!(a2 & 0xA))
			((void(__fastcall *)())(g_ModuleBase + 0x12F8B10))();

		// RenderBatches
		annotation->BeginEvent(L"RenderBatches");
		DC_RenderDeferred(a1, a2, [](__int64 a1, unsigned int a2)
		{
			((BSShaderAccumulator *)a1)->RenderTechniques(1, BSSM_DISTANTTREE_DEPTH, a2, -1);
		});
		/*{
		RenderTechniques(a1, 1, BSSM_DISTANTTREE_DEPTH, a2, -1);
		}*/
		DC_WaitDeferred();
		annotation->EndEvent();

		// LowAniso
		annotation->BeginEvent(L"LowAniso");
		{
			auto pass = accumulator->m_MainBatch->m_Passes[9];

			if (pass)
			{
				if (pass->UnkByte1 & 1)
					pass->Render(a2);
				else
					accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 9);
			}
		}
		annotation->EndEvent();

		// RenderGrass
		annotation->BeginEvent(L"RenderGrass");
		{
			accumulator->RenderTechniques(BSSM_GRASS_DIRONLY_LF, 0x5C00005C, a2, -1);
		}
		annotation->EndEvent();

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
			auto pass = accumulator->m_MainBatch->m_Passes[1];

			if (pass)
			{
				if (pass->UnkByte1 & 1)
					pass->Render(a2);
				else
					accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 1);
			}

			if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5) && *(DWORD *)&graphicsGlobals->__zz0[32] != 3)
			{
				*(DWORD *)&graphicsGlobals->__zz0[32] = 3;
				uint32_t v13 = graphicsGlobals->dword_14304DEB0 & 0xFFFFFFFB;
				if (*(DWORD *)&graphicsGlobals->__zz0[36] != 3)
					v13 = graphicsGlobals->dword_14304DEB0 | 4;
				graphicsGlobals->dword_14304DEB0 = v13;
			}
		}
		annotation->EndEvent();

		// RenderLODLand
		annotation->BeginEvent(L"RenderLODLand");
		{
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
		/*
		annotation->BeginEvent(L"RenderSky");
		{
		DC_RenderDeferred(a1, a2, [](__int64 a1, unsigned int a2) {
		auto graphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

		if (graphicsGlobals->__zz0[76] != 1)
		{
		graphicsGlobals->dword_14304DEB0 |= 0x100u;
		graphicsGlobals->__zz0[76] = 1;
		}
		float v15 = graphicsGlobals->float_14304DF68;
		if (graphicsGlobals->float_14304DF68 != 0.50196081)
		{
		graphicsGlobals->dword_14304DEB0 |= 0x200u;
		graphicsGlobals->float_14304DF68 = 0.50196081f;
		}
		rbt1(a1, BSSM_SKYBASEPRE, BSSM_SKY_CLOUDSFADE, a2, -1);
		});
		}
		DC_WaitDeferred();
		annotation->EndEvent();*/
		annotation->BeginEvent(L"RenderSky");
		if (graphicsGlobals->__zz0[76] != 1)
		{
			graphicsGlobals->dword_14304DEB0 |= 0x100u;
			graphicsGlobals->__zz0[76] = 1;
		}
		float v15 = graphicsGlobals->float_14304DF68;
		if (graphicsGlobals->float_14304DF68 != 0.50196081f)
		{
			graphicsGlobals->dword_14304DEB0 |= 0x200u;
			graphicsGlobals->float_14304DF68 = 0.50196081f;
		}
		accumulator->RenderTechniques(BSSM_SKYBASEPRE, BSSM_SKY_CLOUDSFADE, a2, -1);
		annotation->EndEvent();

		// RenderSkyClouds
		annotation->BeginEvent(L"RenderSkyClouds");
		{
			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 11)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 11;
			}

			auto pass = accumulator->m_MainBatch->m_Passes[13];

			if (pass)
			{
				if (pass->UnkByte1 & 1)
					pass->Render(a2);
				else
					accumulator->RenderTechniques(1, BSSM_BLOOD_SPLATTER, a2, 13);
			}

			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 1)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 1;
			}
		}
		annotation->EndEvent();

		if (!v7)
			((void(__fastcall *)())(g_ModuleBase + 0x12F8B10))();

		if (*(DWORD *)&graphicsGlobals->__zz0[72] != 10)
		{
			graphicsGlobals->dword_14304DEB0 |= 0x80u;
			*(DWORD *)&graphicsGlobals->__zz0[72] = 10;
		}

		((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E27B0))(a1, a2);

		// BlendedDecals
		annotation->BeginEvent(L"BlendedDecals");
		{
			// WARNING: Nvidia NSight causes a bug (?) with the water texture somewhere. It gets drawn here.
			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 11)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 11;
			}

			((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E2950))(a1, a2);
		}
		annotation->EndEvent();

		v18 = graphicsGlobals->dword_14304DEB0;
		if (*(DWORD *)&graphicsGlobals->__zz0[64])
		{
			v18 = graphicsGlobals->dword_14304DEB0 | 0x80;
			*(DWORD *)&graphicsGlobals->__zz0[64] = 0;
			graphicsGlobals->dword_14304DEB0 |= 0x80u;
		}
		if (*(DWORD *)&graphicsGlobals->__zz0[72] != 1)
		{
			*(DWORD *)&graphicsGlobals->__zz0[72] = 1;
			graphicsGlobals->dword_14304DEB0 = v18 | 0x80;
		}

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
				v20 = sub_1412FD120();
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
			if (*(DWORD *)&graphicsGlobals->__zz0[32] != 1)
			{
				*(DWORD *)&graphicsGlobals->__zz0[32] = 1;
				DWORD v22 = graphicsGlobals->dword_14304DEB0 & 0xFFFFFFFB;
				if (*(DWORD *)&graphicsGlobals->__zz0[36] != 1)
					v22 = graphicsGlobals->dword_14304DEB0 | 4;
				graphicsGlobals->dword_14304DEB0 = v22;
			}
		}

		if (!v7)
			((void(__fastcall *)())(g_ModuleBase + 0x12F8C70))();
	}
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
		*(BYTE *)(a1 + 320) = batch->sub_14131E700(&this->m_CurrentTech, a1 + 316, (__int64)&v14);
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
				v12 = batch->sub_14131ECE0(&this->m_CurrentTech, a1 + 316, (__int64)&v14);
			else
				v12 = batch->sub_14131E960(&this->m_CurrentTech, (unsigned int *)(a1 + 316), (__int64)&v14, a4);

			*(BYTE *)(a1 + 320) = v12;
		} while (v12);
	}

	if (subPass)
		subPass->Unregister();

	sub_14131F090();
	ReleaseSRWLockExclusive(&srwtest);
}