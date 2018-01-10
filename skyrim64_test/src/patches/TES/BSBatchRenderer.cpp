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

AutoPtr(BYTE, byte_1431F54CD, 0x31F54CD);
AutoPtr(DWORD, dword_141E32FDC, 0x1E32FDC);

extern BSReadWriteLock testLocks[32];

void operator_delete(__int64 a1, __int64 a2)
{
	((void(__fastcall *)(void *))(g_ModuleBase + 0x1026F0))((void *)a1);
}

void sub_1401C49C0(__int64 a1, __int64(__fastcall *a2)(uint64_t, __int64), __int64 a3)
{
	__int64 v3; // r9
	__int64(__fastcall *v4)(uint64_t, __int64); // r8
	__int64 v5; // rbx
	__int64 v6; // rax
	__int64 v7; // rsi
	__int64 v8; // rdi
	__int64(__fastcall *v9)(uint64_t, __int64); // [rsp+58h] [rbp+10h]
	__int64 v10; // [rsp+60h] [rbp+18h]

	v10 = a3;
	v9 = a2;
	v3 = a3;
	v4 = a2;
	v5 = a1;
	if (*(uint64_t *)(a1 + 8))
	{
		while (1)
		{
			v6 = *(uint64_t *)(v5 + 8);
			v7 = *(uint64_t *)(v6 + 8);
			*(uint64_t *)(v6 + 8) = 0i64;
			if (v4)
				break;
			v8 = *(uint64_t *)(v5 + 8);
			if (v8)
			{
				sub_1401C49C0(*(uint64_t *)(v5 + 8), 0i64, 0i64);
				operator_delete(v8, 16i64);
				goto LABEL_6;
			}
		LABEL_7:
			*(uint64_t *)(v5 + 8) = v7;
			if (!v7)
			{
				*(DWORD *)v5 = 0;
				return;
			}
		}
		v4(*(uint64_t *)(v5 + 8), v3);
	LABEL_6:
		v3 = v10;
		v4 = v9;
		goto LABEL_7;
	}
	*(DWORD *)a1 = 0;
}

void sub_14131F910(__int64 a1, __int64 a2)
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	if (a2)
	{
		BSSpinLock& lock = *(BSSpinLock *)(a2 + 8);

		lock.Acquire();
		*(uint64_t *)(a1 + 8) = *(uint64_t *)a2;
		*(uint64_t *)a2 = a1;
		lock.Release();
	}
	else if (a1)
	{
		sub_1401C49C0(a1, 0i64, 0i64);
		operator_delete(a1, (unsigned int)(a2 + 16));
	}
}

void sub_14131F9F0(__int64 *a1, unsigned int a2)
{
	__int64 *v5; // rdi

	v5 = a1;
	if (*a1)
	{
		sub_14131F090(true);

		BSRenderPass *i = (BSRenderPass *)*v5;

		int tempIndex = -1;
		if (i)
			tempIndex = i->m_Shader->m_Type;

		if (tempIndex != -1)
		{
			if (tempIndex != 6 && tempIndex != 1 && tempIndex != 9)
			{
				if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, true))
					testLocks[tempIndex].AcquireWrite();
				//AcquireSRWLockExclusive(&testLocks[tempIndex]);
			}
		}

		int firstType = -1;
		for (; i; i = i->m_Next)
		{
			if (!i->m_Geometry)
				continue;

			if (firstType == -1)
				firstType = i->m_Shader->m_Type;

			if (firstType != i->m_Shader->m_Type)
				__debugbreak();

			if ((a2 & 0x108) == 0)
			{
				if (i->m_Property->QFlags() & 0x1000000000i64)
					BSGraphics::Renderer::RasterStateSetCullMode(0);
				else
					BSGraphics::Renderer::RasterStateSetCullMode(1);
			}

			__int64 v9 = *(uint64_t *)((uintptr_t)i->m_Geometry + 288i64);// BSGeometry::GetModelBound?
			bool v10 = v9 && (*(WORD *)(v9 + 48) >> 9) & 1;
			BSBatchRenderer::DrawPassGeometry(i, i->m_TechniqueID, v10, a2);
		}

		if ((a2 & 0x108) == 0)
			BSGraphics::Renderer::RasterStateSetCullMode(1);

		v5[0] = 0i64;
		v5[1] = 0i64;

		sub_14131F090();

		if (tempIndex != -1)
		{
			if (tempIndex != 6 && tempIndex != 1 && tempIndex != 9)
			{
				if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, false))
					testLocks[tempIndex].ReleaseWrite();
				//ReleaseSRWLockExclusive(&testLocks[tempIndex]);
			}
		}
	}
}

void BSBatchRenderer::PassInfo::Render(unsigned int a2)
{
	//auto Render = (void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x131CB70);

	__int64 a1 = (__int64)this;

	__int64 v4; // r14
	__int64 v6; // rbx
	__int64 v7; // rax
	__int64 v8; // rsi
	__int64 v9; // rdi

	v4 = a1;
	if (this->UnkByte1 & 1)
	{
		sub_14131F9F0((__int64 *)(a1 + 8), a2);
	}
	else
	{
		if (!this->m_BatchRenderer)
			goto LABEL_14;

		__debugbreak();

		uint64_t v5 = (uint64_t)this->m_BatchRenderer;

		(*(void(__fastcall **)(__int64, signed __int64, signed __int64, uint64_t, signed __int64))(*(uint64_t *)v5 + 24i64))(
			v5,
			1i64,
			BSSM_BLOOD_SPLATTER,
			a2,
			-2i64);
	}

	v6 = *(uint64_t *)v4;

	if (*(uint64_t *)v4)
	{
		if (!*(BYTE *)(v6 + 108))
			return;
		if (*(uint64_t *)(v6 + 96))
		{
			do
			{
				v7 = *(uint64_t *)(v6 + 96);
				v8 = *(uint64_t *)(v7 + 8);
				*(uint64_t *)(v7 + 8) = 0i64;
				if (true /*sub_14131F910*/)
				{
					sub_14131F910(*(uint64_t *)(v6 + 96), g_ModuleBase + 0x34B5230);
				}
				else
				{
					v9 = *(uint64_t *)(v6 + 96);
					if (v9)
					{
						sub_1401C49C0(*(uint64_t *)(v6 + 96), 0i64, 0i64);
						operator_delete(v9, 16i64);
					}
				}
				*(uint64_t *)(v6 + 96) = v8;
			} while (v8);
		}
		*(DWORD *)(v6 + 88) = 0;
	}

LABEL_14:
	this->UnkWord1 = 0;
}

void BSBatchRenderer::PassInfo::Unregister()
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	this->UnkPtr2 = 0;
	this->UnkPtr3 = 0;

	if (m_BatchRenderer)
		m_BatchRenderer->sub_14131D6E0();

	this->UnkWord1 = 0;
}

void BSBatchRenderer::RenderPassArray::Clear(bool Validate)
{
	for (int i = 0; i < ARRAYSIZE(this->m_Pass); i++)
	{
		if (Validate && this->m_Pass[i])
		{
			//sub_14002288B((__int64)&a1, 255i64, (__int64)"Pass still has passes");
			//if (qword_14468B560)
			//	qword_14468B560(&a1, 0i64);
		}

		// This is removed in public builds? Sets the bool to indicate pass is no longer registered
		// for (result = *(_QWORD *)(v4 + 8 * v3); result; result = *(_QWORD *)(result + 48))
		//	*(_BYTE *)(result + 33) = 0;

		this->m_Pass[i] = nullptr;
	}

	this->m_PassIndexBits = 0;
}

bool BSBatchRenderer::HasTechniquePasses(uint32_t StartTech, uint32_t EndTech)
{
	auto sub_14131F100 = (char(__fastcall *)(__int64 a1, unsigned int a2, unsigned int a3))(g_ModuleBase + 0x131F460);
	return sub_14131F100((__int64)this, StartTech, EndTech);
}

bool BSBatchRenderer::sub_14131E8F0(unsigned int a2, uint32_t& SubPassIndex)
{
	if (SubPassIndex > 4)
		SubPassIndex = 0;

	uint32_t v4 = SubPassIndex;
	bool v5 = SubPassIndex == 5;

	if (SubPassIndex < 5)
	{
		uint32_t v6 = SubPassIndex;
		do
		{
			if (this->m_RenderArrays[a2].m_Pass[v6])
			{
				SubPassIndex = v4;
				v4 = 5;
				v6 = 5i64;
			}
			++v4;
			++v6;
			v5 = v4 == 5;
		} while (v4 < 5);
	}
	if (v5)
		SubPassIndex = 0;
	return v4 == 6;
}

bool BSBatchRenderer::sub_14131E700(uint32_t& Technique, uint32_t& SubPassIndex, __int64 a4)
{
	if (!Technique)
		return sub_14131E7B0(Technique, SubPassIndex, (__int64 *)a4);

	uint32_t passArray = m_TechToArrayMap.get(Technique);

	if (!sub_14131E8F0(passArray, SubPassIndex))
		return sub_14131E7B0(Technique, SubPassIndex, (__int64 *)a4);

	return true;
}

bool BSBatchRenderer::sub_14131ECE0(uint32_t& Technique, uint32_t& SubPassIndex, __int64 a4)
{
	uint32_t passArray = m_TechToArrayMap.get(Technique);

	if (*(BYTE *)((uintptr_t)this + 108))
		m_RenderArrays[passArray].Clear(true);

	return sub_14131E700(Technique, SubPassIndex, a4);
}

bool BSBatchRenderer::sub_14131E7B0(uint32_t& Technique, uint32_t& SubPassIndex, __int64 *a4)
{
	__int64 v6; // rbx
	__int64 v7; // rax
	unsigned int v8; // ecx
	__int64 v10; // rdi

	__int64 a1 = (__int64)this;

	v6 = a1;
	v7 = *a4;

	if (!*a4 || !*(uint64_t *)(v7 + 8) && !*(DWORD *)v7)
		return false;

	v8 = *(DWORD *)v7;
	for (Technique = *(DWORD *)v7; v8 < *(DWORD *)(v6 + 80); Technique = *(DWORD *)v7)
	{
		v7 = *(uint64_t *)(v7 + 8);
		if (!v7)
			return false;
		v8 = *(DWORD *)v7;
	}

	if (v8 > *(DWORD *)(v6 + 84))
		return false;

	// "RenderPasses in active lists are out of order, passes will probably be leaked"
	if (*(BYTE *)(v6 + 108))
	{
		v10 = *(uint64_t *)(v6 + 96);
		if (v10)
		{
			*(uint64_t *)(v6 + 96) = *(uint64_t *)(v10 + 8);
			*(DWORD *)(v6 + 88) = *(DWORD *)v10;
			*(uint64_t *)(v10 + 8) = 0i64;
			if (true /*sub_14131F910*/)
			{
				sub_14131F910(v10, g_ModuleBase + 0x34B5230);
			}
			else
			{
				sub_1401C49C0(v10, 0i64, 0i64);
				operator_delete(v10, 16i64);
			}
		}
		else
		{
			*(DWORD *)(v6 + 88) = 0;
		}
	}
	else
	{
		*a4 = *(uint64_t *)(*a4 + 8);
	}

	return sub_14131E8F0(m_TechToArrayMap.get(Technique), SubPassIndex);
}

bool BSBatchRenderer::sub_14131E960(uint32_t& Technique, uint32_t& SubPassIndex, __int64 a4, unsigned int a5)
{
	auto& GraphicsGlobals = *(BSGraphicsRendererGlobals *)GetThreadedGlobals();

	bool unknownFlag2 = false;
	bool unknownFlag = false;

	// Set pass render state
	{
		unknownFlag2 = 0;
		unknownFlag = (a5 & 0x108) != 0;

		int cullMode = -1;
		int alphaBlendUnknown = -1;
		bool useScrapConstant = false;

		switch (SubPassIndex)
		{
		case 0:
			if (!unknownFlag)
				cullMode = 1;

			useScrapConstant = false;
			alphaBlendUnknown = 0;
			break;

		case 1:
			if (!unknownFlag)
				cullMode = 1;

			useScrapConstant = true;
			unknownFlag2 = 1;

			if (byte_1431F54CD)
				alphaBlendUnknown = 1;
			break;

		case 2:
			if (!unknownFlag)
				cullMode = 0;

			useScrapConstant = false;
			alphaBlendUnknown = 0;
			break;

		case 3:
			if (!unknownFlag)
				cullMode = 0;

			useScrapConstant = true;
			unknownFlag2 = 1;

			if (byte_1431F54CD)
				alphaBlendUnknown = 1;
			break;

		case 4:
			if (!unknownFlag)
				cullMode = 1;

			useScrapConstant = true;
			unknownFlag2 = 1;
			alphaBlendUnknown = 0;
			break;
		}

		if (cullMode != -1)
			BSGraphics::Renderer::RasterStateSetCullMode(cullMode);

		if (alphaBlendUnknown != -1)
			BSGraphics::Renderer::AlphaBlendStateSetUnknown1(alphaBlendUnknown);

		BSGraphics::Renderer::SetUseScrapConstantValue(useScrapConstant);
	}

	// Render this group with a specific render pass list
	RenderPassArray *passArray = &m_RenderArrays[m_TechToArrayMap.get(Technique)];

	int tempIndex = -1;
	if (passArray->m_Pass[SubPassIndex])
		tempIndex = passArray->m_Pass[SubPassIndex]->m_Shader->m_Type;

	if (tempIndex != -1)
	{
		if (tempIndex != 6 && tempIndex != 1 && tempIndex != 9)
		{
			if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, true))
				testLocks[tempIndex].AcquireWrite();
			//AcquireSRWLockExclusive(&testLocks[tempIndex]);
		}
	}

	int firstType = -1;
	for (BSRenderPass *pass = passArray->m_Pass[SubPassIndex]; pass; pass = pass->m_Next)
	{
		if (firstType == -1)
			firstType = pass->m_Shader->m_Type;

		if (pass->m_Shader->m_Type != firstType)
			__debugbreak();

		DrawPassGeometry(pass, Technique, unknownFlag2, a5);
	}

	// a1+108 is probably a "remove list" flag after it's rendered, but the memory is not freed yet
	if (*(BYTE *)((uintptr_t)this + 108))
	{
		if (SubPassIndex < 0 || SubPassIndex >= ARRAYSIZE(passArray->m_Pass))
			__debugbreak();

		passArray->m_PassIndexBits &= ~(1 << SubPassIndex);
		passArray->m_Pass[SubPassIndex] = nullptr;
	}

	sub_14131F090();
	BSGraphics::Renderer::AlphaBlendStateSetUnknown1(0);

	if (tempIndex != -1)
	{
		if (tempIndex != 6 && tempIndex != 1 && tempIndex != 9)
		{
			if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, false))
				testLocks[tempIndex].ReleaseWrite();
			//ReleaseSRWLockExclusive(&testLocks[tempIndex]);
		}
	}

	SubPassIndex++;
	return sub_14131E700(Technique, SubPassIndex, a4);
}

void BSBatchRenderer::sub_14131D6E0()
{
	__int64 v1 = (__int64)this;

	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	for (auto itr = m_TechToArrayMap.begin(); itr != m_TechToArrayMap.end(); itr++)
	{
		if (itr)
			m_RenderArrays[*itr].Clear(true);
	}

	// This entire block is some inlined function
	if (*(uint64_t *)(v1 + 96))
	{
		uint64_t v11;

		do
		{
			uint64_t v10 = *(uint64_t *)(v1 + 96);
			v11 = *(uint64_t *)(v10 + 8);
			*(uint64_t *)(v10 + 8) = 0i64;

			if (true /*sub_14131F910*/)
			{
				sub_14131F910(*(uint64_t *)(v1 + 96), g_ModuleBase + 0x34B5230);
			}
			else
			{
				uint64_t v12 = *(uint64_t *)(v1 + 96);
				if (v12)
				{
					// BSGraphics::Renderer::CleanupCommandBuffer or similar name (????)
					sub_1401C49C0(*(uint64_t *)(v1 + 96), 0i64, 0i64);
					operator_delete(v12, 16i64);
				}
			}
			*(uint64_t *)(v1 + 96) = v11;
		} while (v11);
	}

	*(DWORD *)(v1 + 88) = 0;
}

bool __fastcall sub_1412E3AB0(int a1)
{
	return (unsigned int)(a1 - 0x5C000058) <= 3;
}

void *sub_140D6BF00(__int64 a1, int AllocationSize, uint32_t *AllocationOffset);

void UnmapDynamicData()
{
	auto *renderer = GetThreadedGlobals();

	renderer->m_DeviceContext->Unmap(renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], 0);
}

AutoPtr(bool, zbUseEarlyZ, 0x30528E5);
SRWLOCK testingLock = SRWLOCK_INIT;

void BSBatchRenderer::DrawPassGeometry(BSRenderPass *Pass, uint32_t Technique, unsigned __int8 a3, unsigned int a4)
{
	if (InsertRenderCommand<DrawGeometryRenderCommand>(Pass, Technique, a3, a4))
		return;

	__int64 v6; // r14
	__int64 result; // rax

	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	auto sub_14131EFF0 = (__int64(__fastcall *)(unsigned int a1, __int64 a2))(g_ModuleBase + 0x131F350);
	auto sub_1412ABF00 = (const char *(__fastcall *)(unsigned int a1))(g_ModuleBase + 0x12ABF00);

	v6 = (uint64_t)Pass->m_Shader;

	if (dword_1432A8214 == Technique && Technique != 0x5C006076 && v6 == qword_1432A8218
		|| (dword_141E32FDC = Technique, result = sub_14131EFF0(Technique, v6), (BYTE)result))
	{
		BSShaderProperty *property = Pass->m_Property;
		BSShaderMaterial *material = nullptr;

		if (property)
			material = property->pMaterial;

		if ((uintptr_t)material != qword_1434B5220)
		{
			if (material)
				Pass->m_Shader->SetupMaterial(material);

			qword_1434B5220 = (uintptr_t)material;
		}

		//AcquireSRWLockExclusive(&testingLock);
		*(BYTE *)((uintptr_t)Pass->m_Geometry + 264) = Pass->Byte1E;

		if (Pass->m_Geometry->QSkinInstance())
			DrawGeometrySkinned(Pass, a3, a4);
		else if (*(BYTE *)((uintptr_t)Pass->m_Geometry + 265) & 8)// BSGeometry::NeedsCustomRender()?
			DrawGeometryCustom(Pass, a3, a4);
		else
			DrawGeometryDefault(Pass, a3, a4);
		//ReleaseSRWLockExclusive(&testingLock);
	}
}

void BSBatchRenderer::DrawGeometryDefault(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	MemoryContextTracker tracker(26, "BSBatchRenderer.cpp");

	// bAssert(Pass, "Render Error: Render pass is nullptr");
	// bAssert(Pass->m_Geometry, "Render Error: Render pass geometry is nullptr");
	// bAssert(Pass->m_Shader, "Render Error: There is no BSShader attached to the geometry");

	SetupGeometryBlending(Pass, Pass->m_Shader, (AlphaTest || zbUseEarlyZ) ? true : false, RenderFlags);
	DrawTriStrips(Pass);
	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawGeometrySkinned(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	// bAssert(Pass, "Render Error: Render pass is nullptr");
	// bAssert(Pass->m_Geometry, "Render Error: Render pass geometry is nullptr");
	// bAssert(Pass->m_Shader, "Render Error: There is no BSShader attached to the geometry");

	// "Render Error : Skin instance is nullptr"
	// "Render Error : Skin partition is nullptr"
	// "Render Error : Skin partition array is nullptr"
	// bAssert(Pass->m_Property, "Don't have a shader property when we expected one.");

	auto sub_140C71A50 = (const void *(__fastcall *)(uintptr_t))(g_ModuleBase + 0x0C71A50);
	auto sub_140C71AB0 = (void(__fastcall *)(uintptr_t))(g_ModuleBase + 0x0C71AB0);
	auto sub_141336450 = (void(__fastcall *)())(g_ModuleBase + 0x1336450);

	uintptr_t v7 = (uintptr_t)Pass->m_Geometry;

	sub_141336450();

	if ((*(__int64(__fastcall **)(__int64))(*(uintptr_t *)v7 + 432i64))(v7))
	{
		SetupGeometryBlending(Pass, Pass->m_Shader, AlphaTest, RenderFlags);

		NiBoneMatrixSetterI::Data params;
		params.m_Flags = 1;

		Pass->m_Shader->SetBoneMatrix(Pass->m_Geometry->QSkinInstance(), &params, &Pass->m_Geometry->GetWorldTransform());
		DrawTriStrips(Pass);
	}
	else
	{
		SetupGeometryBlending(Pass, Pass->m_Shader, AlphaTest, RenderFlags);

		uint32_t v10 = (Pass->Byte1E >> 7) & 1;
		uint32_t v11 = (Pass->Byte1E & 0x7F);

		NiSkinInstance::UnknownData params;
		params.m_BoneSetter = static_cast<NiBoneMatrixSetterI *>(Pass->m_Shader);
		params.m_Geometry = Pass->m_Geometry;
		params.m_UnkPtr = nullptr;
		params.m_UnkDword1 = v10;
		params.m_UnkDword2 = v11;
		params.m_UnkDword3 = 0;
		params.m_UnkDword4 = -1;

		// Skinned verts are uploaded to a GPU vertex buffer directly (non-static objects like characters)
		uintptr_t v12 = (uintptr_t)Pass->m_Geometry->IsDynamicTriShape();

		if (v12)
		{
			uint32_t v16 = *(uint32_t *)(v12 + 0x170);
			void *v15 = sub_140D6BF00(0, v16, &params.m_UnkDword4);

			const void *v17 = sub_140C71A50(v12);
			memcpy_s(v15, v16, v17, v16);
			sub_140C71AB0(v12);
			UnmapDynamicData();
		}

		Pass->m_Geometry->QSkinInstance()->VFunc37(&params);
	}

	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawGeometryCustom(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	Pass->m_Shader->SetupGeometry(Pass, RenderFlags);
	Pass->m_Shader->SetupGeometryAlphaBlending(Pass->QAlphaProperty(), Pass->m_Property, true);

	if (Pass->QAlphaProperty())
		Pass->m_Shader->SetupAlphaTestRef(Pass->QAlphaProperty(), Pass->m_Property);

	DrawTriStrips(Pass);
	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::SetupGeometryBlending(BSRenderPass *Pass, BSShader *Shader, bool AlphaTest, uint32_t RenderFlags)
{
	//if (Shader != BSSkyShader::pInstance)
	{
		if ((RenderFlags & 4) && !sub_1412E3AB0(Pass->m_TechniqueID))
			Shader->SetupGeometryAlphaBlending(Pass->QAlphaProperty(), Pass->m_Property, AlphaTest);

		if (AlphaTest && Pass->QAlphaProperty())
			Shader->SetupAlphaTestRef(Pass->QAlphaProperty(), Pass->m_Property);
	}

	Shader->SetupGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawTriStrips(BSRenderPass *Pass)
{
	auto sub_14131DDF0 = (void(__fastcall *)(BSRenderPass *))(g_ModuleBase + 0x131DDF0);
	sub_14131DDF0(Pass);
}