#include "../rendering/common.h"
#include "../../common.h"
#include "BSGraphicsRenderer.h"
#include "MemoryContextTracker.h"
#include "BSSpinLock.h"
#include "BSBatchRenderer.h"
#include "BSShader/Shaders/BSSkyShader.h"
#include "BSShader/Shaders/BSLightingShader.h"
#include "MTRenderer.h"

AutoPtr(BYTE, byte_1431F54CD, 0x31F54CD);
AutoPtr(DWORD, dword_141E32FDC, 0x1E32FDC);

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
	if (!*a1)
		return;

	MTRenderer::ClearShaderAndTechnique();

	bool mtrContext = MTRenderer::IsGeneratingGameCommandList();
	int lockType = ((BSRenderPass *)*a1)->m_Shader->m_Type;

	MTRenderer::LockShader(lockType);

	for (BSRenderPass *i = (BSRenderPass *)*a1; i; i = i->m_Next)
	{
		if (!i->m_Geometry)
			continue;

		// This render pass function doesn't always use one shader type
		if (lockType != i->m_Shader->m_Type)
		{
			MTRenderer::UnlockShader(lockType);
			lockType = i->m_Shader->m_Type;
			MTRenderer::LockShader(lockType);
		}

		if ((a2 & 0x108) == 0)
		{
			if (i->m_Property->QFlags() & 0x1000000000i64)
				MTRenderer::RasterStateSetCullMode(0);
			else
				MTRenderer::RasterStateSetCullMode(1);
		}

		__int64 v9 = *(uint64_t *)((uintptr_t)i->m_Geometry + 288i64);// BSGeometry::GetModelBound?
		bool v10 = v9 && (*(WORD *)(v9 + 48) >> 9) & 1;

		if (mtrContext)
			MTRenderer::InsertCommand<MTRenderer::DrawGeometryRenderCommand>(i, i->m_TechniqueID, v10, a2);
		else
			BSBatchRenderer::SetupAndDrawPass(i, i->m_TechniqueID, v10, a2);
	}

	if ((a2 & 0x108) == 0)
		MTRenderer::RasterStateSetCullMode(1);

	a1[0] = 0i64;
	a1[1] = 0i64;

	MTRenderer::ClearShaderAndTechnique();
	MTRenderer::UnlockShader(lockType);
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
			AssertMsg(false, "Pass still has passes");

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

	if (*(BYTE *)(v6 + 108))
	{
		AssertMsg(m_EndingTech >= m_StartingTech, "RenderPasses in active lists are out of order, passes will probably be leaked");

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

BSShaderAccumulator *GetCurrentAccumulator();

extern int commandThreshold;
extern int commandThreshold2;

bool BSBatchRenderer::sub_14131E960(uint32_t& Technique, uint32_t& SubPassIndex, __int64 a4, unsigned int a5)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	bool unknownFlag2;
	bool unknownFlag;

	// Set pass render state
	{
		int cullMode = -1;
		int alphaBlendUnknown = -1;
		bool useScrapConstant = false;

		unknownFlag2 = false;
		unknownFlag = (a5 & 0x108) != 0;

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
			MTRenderer::RasterStateSetCullMode(cullMode);

		if (alphaBlendUnknown != -1)
			MTRenderer::AlphaBlendStateSetUnknown1(0);

		renderer->SetUseScrapConstantValue(useScrapConstant);
	}

	// Render this group with a specific render pass list
	int shaderType = -1;
	RenderPassArray *passArray = &m_RenderArrays[m_TechToArrayMap.get(Technique)];
	BSRenderPass *currentPass = passArray->m_Pass[SubPassIndex];

	if (currentPass)
		shaderType = currentPass->m_Shader->m_Type;

	MTRenderer::LockShader(shaderType);

	// If we can, submit it to the command list queue instead of running it directly
	if (MTRenderer::IsGeneratingGameCommandList())
	{
		// Combine 3 draw command packets into 1 when possible
		BSRenderPass *temp[3];

		for (int count = 0;; count = 0)
		{
			for (; currentPass && count < ARRAYSIZE(temp); currentPass = currentPass->m_Next)
				temp[count++] = currentPass;

			if (count == 0)
				break;

			if (count == ARRAYSIZE(temp))
			{
				// 3 x BSRenderPass, 1 packet
				MTRenderer::InsertCommand<MTRenderer::DrawGeometryMultiRenderCommand>(temp, Technique, unknownFlag2, a5);
			}
			else
			{
				for (int i = 0; i < count; i++)
					MTRenderer::InsertCommand<MTRenderer::DrawGeometryRenderCommand>(temp[i], Technique, unknownFlag2, a5);
			}
		}
	}
	else
	{
		for (; currentPass; currentPass = currentPass->m_Next)
			SetupAndDrawPass(currentPass, Technique, unknownFlag2, a5);
	}

	// a1+108 is probably a "remove list" flag after it's rendered, but the memory is not freed yet
	if (*(BYTE *)((uintptr_t)this + 108))
	{
		Assert(SubPassIndex >= 0 && SubPassIndex < ARRAYSIZE(passArray->m_Pass));

		passArray->m_PassIndexBits &= ~(1 << SubPassIndex);
		passArray->m_Pass[SubPassIndex] = nullptr;
	}

	MTRenderer::ClearShaderAndTechnique();
	MTRenderer::AlphaBlendStateSetUnknown1(0);
	MTRenderer::UnlockShader(shaderType);

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
	auto *renderer = BSGraphics::Renderer::GetGlobals();

	renderer->m_DeviceContext->Unmap(renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], 0);
}

AutoPtr(bool, zbUseEarlyZ, 0x30528E5);

void BSBatchRenderer::SetupAndDrawPass(BSRenderPass *Pass, uint32_t Technique, bool AlphaTest, uint32_t RenderFlags)
{
	auto *GraphicsGlobals = BSGraphics::Renderer::GetGlobals();
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	bool techniqueIsSetup = false;

	// SetupShaderAndTechnique doesn't need to be called again if we used this shader previously
	if (dword_1432A8214 == Technique && Technique != 0x5C006076 && (uint64_t)Pass->m_Shader == qword_1432A8218)
		techniqueIsSetup = true;

	if (!techniqueIsSetup)
	{
		dword_141E32FDC = Technique;// This is written but never read anywhere?
		techniqueIsSetup = SetupShaderAndTechnique(Pass->m_Shader, Technique);
	}

	if (techniqueIsSetup)
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

		*(BYTE *)((uintptr_t)Pass->m_Geometry + 264) = Pass->Byte1E;

		if (Pass->m_Geometry->QSkinInstance())
			DrawPassSkinned(Pass, AlphaTest, RenderFlags);
		else if (*(BYTE *)((uintptr_t)Pass->m_Geometry + 265) & 8)// BSGeometry::NeedsCustomRender()?
			DrawPassCustom(Pass, AlphaTest, RenderFlags);
		else
			DrawPass(Pass, AlphaTest, RenderFlags);
	}
}

void BSBatchRenderer::SetupGeometryBlending(BSRenderPass *Pass, BSShader *Shader, bool AlphaTest, uint32_t RenderFlags)
{
	if (Shader != BSSkyShader::pInstance)
	{
		if ((RenderFlags & 4) && !sub_1412E3AB0(Pass->m_TechniqueID))
			Shader->SetupGeometryAlphaBlending(Pass->QAlphaProperty(), Pass->m_Property, AlphaTest);

		if (AlphaTest && Pass->QAlphaProperty())
			Shader->SetupAlphaTestRef(Pass->QAlphaProperty(), Pass->m_Property);
	}

	Shader->SetupGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawPass(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	MemoryContextTracker tracker(26, "BSBatchRenderer.cpp");

	AssertMsgDebug(Pass, "Render Error: Render pass is nullptr");
	AssertMsgDebug(Pass->m_Geometry, "Render Error: Render pass geometry is nullptr");
	AssertMsgDebug(Pass->m_Shader, "Render Error: There is no BSShader attached to the geometry");

	SetupGeometryBlending(Pass, Pass->m_Shader, (AlphaTest || zbUseEarlyZ) ? true : false, RenderFlags);
	DrawGeometry(Pass);
	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawPassSkinned(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	AssertMsgDebug(Pass, "Render Error: Render pass is nullptr");
	AssertMsgDebug(Pass->m_Geometry, "Render Error: Render pass geometry is nullptr");
	AssertMsgDebug(Pass->m_Shader, "Render Error: There is no BSShader attached to the geometry");

	// "Render Error : Skin instance is nullptr"
	// "Render Error : Skin partition is nullptr"
	// "Render Error : Skin partition array is nullptr"
	AssertMsgDebug(Pass->m_Property, "Don't have a shader property when we expected one.");

	auto sub_141336450 = (void(__fastcall *)())(g_ModuleBase + 0x1336450);
	sub_141336450();

	if ((*(__int64(__fastcall **)(BSGeometry *))(*(uintptr_t *)Pass->m_Geometry + 432i64))(Pass->m_Geometry))
	{
		SetupGeometryBlending(Pass, Pass->m_Shader, AlphaTest, RenderFlags);

		NiBoneMatrixSetterI::Data params;
		params.m_Flags = 1;

		Pass->m_Shader->SetBoneMatrix(Pass->m_Geometry->QSkinInstance(), &params, &Pass->m_Geometry->GetWorldTransform());
		DrawGeometry(Pass);
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

		// Runtime-updated vertices are sent to a GPU vertex buffer directly (non-static objects like trees/characters)
		BSDynamicTriShape *dynamicTri = Pass->m_Geometry->IsDynamicTriShape();

		if (dynamicTri)
		{
			uint32_t v16 = *(uint32_t *)((uintptr_t)dynamicTri + 0x170);
			void *vertexBuffer = sub_140D6BF00(0, v16, &params.m_UnkDword4);

			const void *data = dynamicTri->LockDynamicDataForRead();
			memcpy_s(vertexBuffer, v16, data, v16);
			dynamicTri->UnlockDynamicData();

			UnmapDynamicData();
		}

		// Renders multiple skinned instances (SetupTechnique, SetBoneMatrix)
		Pass->m_Geometry->QSkinInstance()->VFunc37(&params);
	}

	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawPassCustom(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	Pass->m_Shader->SetupGeometry(Pass, RenderFlags);
	Pass->m_Shader->SetupGeometryAlphaBlending(Pass->QAlphaProperty(), Pass->m_Property, true);

	if (Pass->QAlphaProperty())
		Pass->m_Shader->SetupAlphaTestRef(Pass->QAlphaProperty(), Pass->m_Property);

	DrawGeometry(Pass);
	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawGeometry(BSRenderPass *Pass)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	BSGeometry *geometry = Pass->m_Geometry;

	switch (geometry->QType())
	{
	case GEOMETRY_TYPE_PARTICLES:
	{
		auto sub_14131DDF0 = (void(__fastcall *)(BSRenderPass *))(g_ModuleBase + 0x131DDF0);
		sub_14131DDF0(Pass);
		return;

		// NiParticles::GetActiveVertexCount
		int particleCount = (*(unsigned __int16(**)(void))(**(uintptr_t **)((uintptr_t)geometry + 344) + 304i64))();

		AssertMsg(particleCount <= MAX_SHARED_PARTICLES_SIZE,
			"This emitter emits more particles than allowed in our rendering buffers."
			"Please investigate on emitter or increase MAX_SHARED_PARTICLES_SIZE");

		if (particleCount > 0)
		{
			BSGraphics::DynamicTriShape *triInfo = renderer->GetParticlesDynamicTriShape();
			void *map = renderer->MapDynamicTriShapeDynamicData(nullptr, triInfo, 4 * particleCount * geometry->GetVertexSize1());

			if (map)
			{
				BSGraphics::Utility::PackDynamicParticleData(particleCount, (class BSGraphics::Utility::NiParticles *)geometry, map);
				renderer->UnmapDynamicTriShapeDynamicData(triInfo);
			}

			renderer->DrawDynamicTriShape(triInfo, 0, 2 * particleCount);
		}
	}
	break;
	
	case GEOMETRY_TYPE_STRIP_PARTICLES:
	{
		// WARNING: Do not enable this function without fixing the input layout lookups first
		Assert(false);
	}
	break;
	
	case GEOMETRY_TYPE_TRISHAPE:
	{
		AssertDebug(geometry->IsTriShape());

		auto triShape = static_cast<BSTriShape *>(geometry);
		auto rendererData = reinterpret_cast<BSGraphics::TriShape *>(triShape->QRendererData());

		renderer->DrawTriShape(rendererData, 0, triShape->m_TriangleCount);
	}
	break;

	case GEOMETRY_TYPE_DYNAMIC_TRISHAPE:
	{
		AssertDebug(false);
	}
	break;

	case GEOMETRY_TYPE_MESHLOD_TRISHAPE:
	{
		AssertDebug(false);
	}
	break;

	case GEOMETRY_TYPE_LOD_MULTIINDEX_TRISHAPE:
	{
		AssertDebug(false);
	}
	break;

	case GEOMETRY_TYPE_MULTIINDEX_TRISHAPE:
	{
		AssertDebug(false);
	}
	break;

	case GEOMETRY_TYPE_SUBINDEX_TRISHAPE:
	{
		auto sub_14131DDF0 = (void(__fastcall *)(BSRenderPass *))(g_ModuleBase + 0x131DDF0);
		sub_14131DDF0(Pass);
	}
	break;

	case GEOMETRY_TYPE_SUBINDEX_LAND_TRISHAPE:
	{
		AssertDebug(false);
	}
	break;

	case GEOMETRY_TYPE_MULTISTREAMINSTANCE_TRISHAPE:
	{
		auto sub_14131DDF0 = (void(__fastcall *)(BSRenderPass *))(g_ModuleBase + 0x131DDF0);
		sub_14131DDF0(Pass);
	}
	break;

	case GEOMETRY_TYPE_PARTICLE_SHADER_DYNAMIC_TRISHAPE:
	{
		AssertDebug(geometry->IsDynamicTriShape());

		auto dynTriShape = static_cast<BSDynamicTriShape *>(geometry);
		auto rendererData = dynTriShape->LockDynamicDataForRead();

		renderer->DrawParticleShaderTriShape(rendererData, dynTriShape->m_VertexCount);
		dynTriShape->UnlockDynamicData();
	}
	break;

	case GEOMETRY_TYPE_LINES:
	{
		AssertDebug(false);

		auto lineShape = static_cast<BSTriShape *>(geometry);
		auto rendererData = reinterpret_cast<BSGraphics::LineShape *>(lineShape->QRendererData());

		renderer->DrawLineShape(rendererData, 0, lineShape->m_TriangleCount);
	}
	break;

	case GEOMETRY_TYPE_DYNAMIC_LINES:
	{
		AssertDebug(false);
	}
	break;

	case GEOMETRY_TYPE_INSTANCE_GROUP:
	{
		AssertDebug(false);
	}
	break;

	default:
		AssertMsgVa(false, "Unimplemented geometry type %d", geometry->QType());
		break;
	}
}