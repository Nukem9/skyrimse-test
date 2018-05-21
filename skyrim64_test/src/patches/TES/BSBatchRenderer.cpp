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

bool BSBatchRenderer::SetupShaderAndTechnique(BSShader *Shader, uint32_t Technique)
{
	ClearShaderAndTechnique();

	auto GraphicsGlobals = HACK_GetThreadedGlobals();
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);

	if (Shader->SetupTechnique(Technique))
	{
		qword_1432A8218 = (uint64_t)Shader;
		dword_1432A8214 = Technique;
		return true;
	}

	qword_1432A8218 = 0;
	dword_1432A8214 = 0;
	return false;
}

void BSBatchRenderer::ClearShaderAndTechnique()
{
	auto GraphicsGlobals = HACK_GetThreadedGlobals();
	//uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	if (qword_1432A8218)
	{
		((BSShader *)qword_1432A8218)->RestoreTechnique(dword_1432A8214);
		qword_1432A8218 = 0;// Shader
	}

	dword_1432A8214 = 0;// Technique
	qword_1434B5220 = 0;
}

void sub_14131F910(BSSimpleList<uint32_t> *Node, void *UserData)
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	if (UserData)
	{
		BSSpinLock& lock = *(BSSpinLock *)((uintptr_t)UserData + 8);

		lock.Acquire();
		Node->m_pkNext = *(BSSimpleList<uint32_t> **)UserData;
		*(uintptr_t *)UserData = (uintptr_t)Node;
		lock.Release();
	}
	else if (Node)
	{
		delete Node;
	}
}

void sub_14131F9F0(BSRenderPass *RenderPasses[2], uint32_t RenderFlags)
{
	if (!RenderPasses[0])
		return;

	MTRenderer::ClearShaderAndTechnique();

	bool mtrContext = MTRenderer::IsGeneratingGameCommandList();
	int lockType = RenderPasses[0]->m_Shader->m_Type;

	MTRenderer::LockShader(lockType);

	for (BSRenderPass *i = RenderPasses[0]; i; i = i->m_Next)
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

		if ((RenderFlags & 0x108) == 0)
		{
			if (i->m_Property->GetFlag(BSShaderProperty::BSSP_FLAG_TWO_SIDED))
				MTRenderer::RasterStateSetCullMode(0);
			else
				MTRenderer::RasterStateSetCullMode(1);
		}

		bool alphaTest = i->m_Geometry->QAlphaProperty() && i->m_Geometry->QAlphaProperty()->GetAlphaTesting();

		if (mtrContext)
			MTRenderer::InsertCommand<MTRenderer::DrawGeometryRenderCommand>(i, i->m_TechniqueID, alphaTest, RenderFlags);
		else
			BSBatchRenderer::SetupAndDrawPass(i, i->m_TechniqueID, alphaTest, RenderFlags);
	}

	if ((RenderFlags & 0x108) == 0)
		MTRenderer::RasterStateSetCullMode(1);

	RenderPasses[0] = nullptr;
	RenderPasses[1] = nullptr;

	MTRenderer::ClearShaderAndTechnique();
	MTRenderer::UnlockShader(lockType);
}

void BSBatchRenderer::RenderGroup::Render(uint32_t RenderFlags)
{
	static_assert(ARRAYSIZE(m_UnkPtrs) == 2);

	if (this->UnkByte1 & 1)
	{
		sub_14131F9F0(m_UnkPtrs, RenderFlags);
	}
	else if(m_BatchRenderer)
	{
		AssertMsg(false, "This is never called...?");

		m_BatchRenderer->VFunc03(1, BSSM_BLOOD_SPLATTER, RenderFlags);
	}

	if (m_BatchRenderer)
	{
		if (!m_BatchRenderer->m_DiscardPassesAfterRender)
			return;

		m_BatchRenderer->m_UnknownList.RemoveAllNodes(sub_14131F910, (void *)(g_ModuleBase + 0x34B5230));
	}

	this->UnkWord1 = 0;
}

void BSBatchRenderer::RenderGroup::Unregister()
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	m_UnkPtrs[0] = nullptr;
	m_UnkPtrs[1] = nullptr;

	if (m_BatchRenderer)
		m_BatchRenderer->sub_14131D6E0();

	this->UnkWord1 = 0;
}

void BSBatchRenderer::AlphaGroupPass::Clear(bool Validate)
{
	for (int i = 0; i < ARRAYSIZE(m_Pass); i++)
	{
		if (Validate && m_Pass[i])
			AssertMsg(false, "Pass still has passes");

		// This is removed in public builds? Sets the bool to indicate pass is no longer registered
		// for (result = *(_QWORD *)(v4 + 8 * v3); result; result = *(_QWORD *)(result + 48))
		//	*(_BYTE *)(result + 33) = 0;

		m_Pass[i] = nullptr;
	}

	m_PassIndexBits = 0;
}

bool BSBatchRenderer::HasTechniquePasses(uint32_t StartTech, uint32_t EndTech)
{
	BSSimpleList<uint32_t> *node = &m_UnknownList;

	if (!node->QNext() && node->QItem() == 0)
		return false;

	while (node && node->QItem() <= EndTech)
	{
		if (node->QItem() >= StartTech)
			return true;

		node = node->QNext();
	}

	return false;
}

bool BSBatchRenderer::sub_14131E8F0(unsigned int a2, uint32_t& GroupIndex)
{
	if (GroupIndex > 4)
		GroupIndex = 0;

	uint32_t v4 = GroupIndex;
	bool v5 = GroupIndex == 5;

	if (GroupIndex < 5)
	{
		uint32_t v6 = GroupIndex;
		do
		{
			if (this->m_RenderArrays[a2].m_Pass[v6])
			{
				GroupIndex = v4;
				v4 = 5;
				v6 = 5i64;
			}
			++v4;
			++v6;
			v5 = v4 == 5;
		} while (v4 < 5);
	}
	if (v5)
		GroupIndex = 0;
	return v4 == 6;
}

bool BSBatchRenderer::sub_14131E700(uint32_t& Technique, uint32_t& GroupIndex, __int64 a4)
{
	if (!Technique)
		return sub_14131E7B0(Technique, GroupIndex, (__int64 *)a4);

	uint32_t passArray = m_TechToArrayMap.get(Technique);

	if (!sub_14131E8F0(passArray, GroupIndex))
		return sub_14131E7B0(Technique, GroupIndex, (__int64 *)a4);

	return true;
}

bool BSBatchRenderer::sub_14131ECE0(uint32_t& Technique, uint32_t& GroupIndex, __int64 a4)
{
	uint32_t passArray = m_TechToArrayMap.get(Technique);

	if (m_DiscardPassesAfterRender)
		m_RenderArrays[passArray].Clear(true);

	return sub_14131E700(Technique, GroupIndex, a4);
}

bool BSBatchRenderer::sub_14131E7B0(uint32_t& Technique, uint32_t& GroupIndex, __int64 *a4)
{
	__int64 v7; // rax
	unsigned int v8; // ecx

	v7 = *a4;

	if (!*a4 || !*(uint64_t *)(v7 + 8) && !*(DWORD *)v7)
		return false;

	v8 = *(DWORD *)v7;
	for (Technique = *(DWORD *)v7; v8 < m_StartingTech; Technique = *(DWORD *)v7)
	{
		v7 = *(uint64_t *)(v7 + 8);

		if (!v7)
			return false;

		v8 = *(DWORD *)v7;
	}

	if (v8 > m_EndingTech)
		return false;

	if (m_DiscardPassesAfterRender)
	{
		AssertMsg(m_EndingTech >= m_StartingTech, "RenderPasses in active lists are out of order, passes will probably be leaked");

		m_UnknownList.RemoveNode(sub_14131F910, (void *)(g_ModuleBase + 0x34B5230));
	}
	else
	{
		*a4 = *(uint64_t *)(*a4 + 8);
	}

	return sub_14131E8F0(m_TechToArrayMap.get(Technique), GroupIndex);
}

bool BSBatchRenderer::sub_14131E960(uint32_t& Technique, uint32_t& GroupIndex, __int64 a4, uint32_t RenderFlags)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	bool alphaTest;
	bool unknownFlag;

	// Set pass render state
	{
		int cullMode = -1;
		int alphaBlendUnknown = -1;
		bool useScrapConstant = false;

		alphaTest = false;
		unknownFlag = (RenderFlags & 0x108) != 0;

		switch (GroupIndex)
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
			alphaTest = true;

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
			alphaTest = true;

			if (byte_1431F54CD)
				alphaBlendUnknown = 1;
			break;

		case 4:
			if (!unknownFlag)
				cullMode = 1;

			useScrapConstant = true;
			alphaTest = true;
			alphaBlendUnknown = 0;
			break;
		}

		if (cullMode != -1)
			MTRenderer::RasterStateSetCullMode(cullMode);

		if (alphaBlendUnknown != -1)
			MTRenderer::AlphaBlendStateSetUnknown1(0);

		renderer->SetUseAlphaTestRef(useScrapConstant);
	}

	// Render this group with a specific render pass list
	int shaderType = -1;
	AlphaGroupPass *group = &m_RenderArrays[m_TechToArrayMap.get(Technique)];
	BSRenderPass *currentPass = group->m_Pass[GroupIndex];

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
				MTRenderer::InsertCommand<MTRenderer::DrawGeometryMultiRenderCommand>(temp, Technique, alphaTest, RenderFlags);
			}
			else
			{
				for (int i = 0; i < count; i++)
					MTRenderer::InsertCommand<MTRenderer::DrawGeometryRenderCommand>(temp[i], Technique, alphaTest, RenderFlags);
			}
		}
	}
	else
	{
		for (; currentPass; currentPass = currentPass->m_Next)
			SetupAndDrawPass(currentPass, Technique, alphaTest, RenderFlags);
	}

	// Zero the pointers only - the memory is freed elsewhere
	if (m_DiscardPassesAfterRender)
	{
		Assert(GroupIndex >= 0 && GroupIndex < ARRAYSIZE(group->m_Pass));

		group->m_PassIndexBits &= ~(1 << GroupIndex);
		group->m_Pass[GroupIndex] = nullptr;
	}

	MTRenderer::ClearShaderAndTechnique();
	MTRenderer::AlphaBlendStateSetUnknown1(0);
	MTRenderer::UnlockShader(shaderType);

	GroupIndex++;
	return sub_14131E700(Technique, GroupIndex, a4);
}

void BSBatchRenderer::sub_14131D6E0()
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	for (auto itr = m_TechToArrayMap.begin(); itr != m_TechToArrayMap.end(); itr++)
	{
		if (itr)
			m_RenderArrays[*itr].Clear(true);
	}

	m_UnknownList.RemoveAllNodes(sub_14131F910, (void *)(g_ModuleBase + 0x34B5230));
}

void *sub_140D6BF00(__int64 a1, int AllocationSize, uint32_t *AllocationOffset);

void UnmapDynamicData()
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();

	renderer->m_DeviceContext->Unmap(renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], 0);
}

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

		*(BYTE *)((uintptr_t)Pass->m_Geometry + 264) = *(BYTE *)(&Pass->m_Lod);// WARNING: MT data write hazard. ucCurrentMeshLODLevel?

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
		if ((RenderFlags & 4) && !BSShaderAccumulator::IsGrassShadowBlacklist(Pass->m_TechniqueID))
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

	SetupGeometryBlending(Pass, Pass->m_Shader, AlphaTest || BSShaderManager::bUseEarlyZ, RenderFlags);
	DrawGeometry(Pass);
	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::DrawPassSkinned(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	AssertMsgDebug(Pass, "Render Error: Render pass is nullptr");
	AssertMsgDebug(Pass->m_Geometry, "Render Error: Render pass geometry is nullptr");
	AssertMsgDebug(Pass->m_Shader, "Render Error: There is no BSShader attached to the geometry");

	AssertMsgDebug(Pass->m_Geometry->QSkinInstance(), "Render Error: Skin instance is nullptr");
	// "Render Error : Skin partition is nullptr"
	// "Render Error : Skin partition array is nullptr"
	AssertMsgDebug(Pass->m_Property, "Don't have a shader property when we expected one.");

	auto sub_141336450 = (void(__fastcall *)())(g_ModuleBase + 0x1336450);
	sub_141336450();

	// BSTriShape::IsBSSkinnedDecalTriShape?
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

		SkinRenderData skinData(static_cast<NiBoneMatrixSetterI *>(Pass->m_Shader), Pass->m_Geometry, nullptr, Pass->m_Lod.SingleLevel, Pass->m_Lod.Index);

		// Runtime-updated vertices are sent to a GPU vertex buffer directly (non-static objects like trees/characters)
		BSDynamicTriShape *dynamicTri = Pass->m_Geometry->IsDynamicTriShape();

		if (dynamicTri)
		{
			uint32_t size = dynamicTri->QDynamicDataSize();
			void *vertexBuffer = sub_140D6BF00(0, size, &skinData.m_VertexBufferOffset);

			const void *data = dynamicTri->LockDynamicDataForRead();
			memcpy_s(vertexBuffer, size, data, size);
			dynamicTri->UnlockDynamicData();

			UnmapDynamicData();
		}

		// Renders multiple skinned instances (SetupTechnique, SetBoneMatrix)
		Pass->m_Geometry->QSkinInstance()->VFunc37(&skinData);
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

		particleCount = min(particleCount, 2048);

		//AssertMsg(particleCount <= MAX_SHARED_PARTICLES_SIZE,
		//	"This emitter emits more particles than allowed in our rendering buffers. "
		//	"Please investigate emitter or increase MAX_SHARED_PARTICLES_SIZE");

		if (particleCount > 0)
		{
			BSGraphics::DynamicTriShape *triInfo = renderer->GetParticlesDynamicTriShape();
			void *map = renderer->MapDynamicTriShapeDynamicData(nullptr, triInfo, 4 * particleCount * geometry->GetDynamicVertexSize());

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
		auto sub_14131DDF0 = (void(__fastcall *)(BSRenderPass *))(g_ModuleBase + 0x131DDF0);
		sub_14131DDF0(Pass);
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