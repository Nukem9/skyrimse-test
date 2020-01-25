#include "../rendering/common.h"
#include "../../common.h"
#include "BSGraphicsState.h"
#include "BSGraphicsRenderer.h"
#include "MemoryContextTracker.h"
#include "BSSpinLock.h"
#include "BSBatchRenderer.h"
#include "BSShader/Shaders/BSSkyShader.h"
#include "MTRenderer.h"

AutoPtr(BYTE, byte_1431F54CD, 0x31F54CD);
AutoPtr(DWORD, dword_141E32FDC, 0x1E32FDC);

bool BSBatchRenderer::BeginPass(BSShader *Shader, uint32_t Technique)
{
	EndPass();

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

void BSBatchRenderer::EndPass()
{
	auto GraphicsGlobals = HACK_GetThreadedGlobals();
	//uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	BSShader*& qword_1432A8218 = *(BSShader **)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	if (qword_1432A8218)
	{
		qword_1432A8218->RestoreTechnique(dword_1432A8214);
		qword_1432A8218 = nullptr;
	}

	dword_1432A8214 = 0;// Technique
	qword_1434B5220 = 0;
}

void sub_14131F910(BSSimpleList<uint32_t> *Node, void *UserData)
{
	MemoryContextTracker tracker(MemoryContextTracker::RENDER_ACCUMULATOR, "BSBatchRenderer.cpp");

	if (UserData)
	{
		BSSpinLock& lock = *(BSSpinLock *)((uintptr_t)UserData + 8);

		lock.Acquire();
		Node->m_pkNext = *(BSSimpleList<uint32_t> **)UserData;
		*(uintptr_t *)UserData = (uintptr_t)Node;
		lock.Release();
	}
	else
	{
		delete Node;
	}
}

void BSBatchRenderer::PersistentPassList::Clear()
{
	m_Head = nullptr;
	m_Tail = nullptr;
}

void BSBatchRenderer::GeometryGroup::Render(uint32_t RenderFlags)
{
	if (m_Flags & 1)
	{
		RenderPersistentPassList(&m_PassList, RenderFlags);
	}
	else if(m_BatchRenderer)
	{
		AssertMsg(false, "This is never called...?");

		m_BatchRenderer->VFunc03(1, BSSM_BLOOD_SPLATTER, RenderFlags);
	}

	if (m_BatchRenderer)
	{
		if (!m_BatchRenderer->m_AutoClearPasses)
			return;

		m_BatchRenderer->m_ActivePassIndexList.RemoveAllNodes(sub_14131F910, (void *)(g_ModuleBase + 0x34B5230));
	}

	m_Count = 0;
}

void BSBatchRenderer::GeometryGroup::ClearAndFreePasses()
{
	MemoryContextTracker tracker(MemoryContextTracker::RENDER_ACCUMULATOR, "BSBatchRenderer.cpp");

	m_PassList.Clear();

	if (m_BatchRenderer)
		m_BatchRenderer->ClearRenderPasses();

	m_Count = 0;
}

void BSBatchRenderer::PassGroup::Clear(bool ReportNotEmpty, bool FreePasses)
{
	for (int i = 0; i < ARRAYSIZE(m_Passes); i++)
	{
		if (ReportNotEmpty && m_Passes[i])
			AssertMsg(false, "Pass still has passes");

		// This is removed in public builds? Sets the bool to indicate pass is no longer registered
		// for (result = *(_QWORD *)(v4 + 8 * v3); result; result = *(_QWORD *)(result + 48))
		//	*(_BYTE *)(result + 33) = 0;

		m_Passes[i] = nullptr;
	}

	m_ValidPassBits = 0;
}

bool BSBatchRenderer::QPassesWithinRange(uint32_t StartRange, uint32_t EndRange)
{
	BSSimpleList<uint32_t> *node = &m_ActivePassIndexList;

	if (!node->QNext() && node->QItem() == 0)
		return false;

	while (node && node->QItem() <= EndRange)
	{
		if (node->QItem() >= StartRange)
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
			if (m_RenderPass[a2].m_Passes[v6])
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

bool BSBatchRenderer::sub_14131E700(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList)
{
	if (!Technique)
		return sub_14131E7B0(Technique, GroupIndex, PassIndexList);

	uint32_t passGroup = m_RenderPassMap.get(Technique);

	if (!sub_14131E8F0(passGroup, GroupIndex))
		return sub_14131E7B0(Technique, GroupIndex, PassIndexList);

	return true;
}

bool BSBatchRenderer::DiscardBatches(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList)
{
	// Probably discards pass, returns true if there's remaining sub passes
	uint32_t passArray = m_RenderPassMap.get(Technique);

	if (m_AutoClearPasses)
		m_RenderPass[passArray].Clear(true, false);

	return sub_14131E700(Technique, GroupIndex, PassIndexList);
}

bool BSBatchRenderer::sub_14131E7B0(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList)
{
	BSSimpleList<uint32_t> *list = PassIndexList;

	if (!list || (!list->QNext() && !list->QItem()))
		return false;

	for (Technique = list->QItem(); Technique < m_CurrentFirstPass; Technique = list->QItem())
	{
		list = list->QNext();

		if (!list)
			return false;
	}

	if (Technique > m_CurrentLastPass)
		return false;

	if (m_AutoClearPasses)
	{
		AssertMsg(m_CurrentLastPass >= m_CurrentFirstPass, "RenderPasses in active lists are out of order, passes will probably be leaked");

		m_ActivePassIndexList.RemoveNode(sub_14131F910, (void *)(g_ModuleBase + 0x34B5230));
	}
	else
	{
		PassIndexList = PassIndexList->QNext();
	}

	return sub_14131E8F0(m_RenderPassMap.get(Technique), GroupIndex);
}

bool BSBatchRenderer::RenderBatches(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList, uint32_t RenderFlags)
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
	PassGroup *group = &m_RenderPass[m_RenderPassMap.get(Technique)];
	BSRenderPass *currentPass = group->m_Passes[GroupIndex];

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
			RenderPassImmediately(currentPass, Technique, alphaTest, RenderFlags);
	}

	// Zero the pointers only - the memory is freed elsewhere
	if (m_AutoClearPasses)
	{
		Assert(GroupIndex >= 0 && GroupIndex < ARRAYSIZE(group->m_Passes));

		group->m_ValidPassBits &= ~(1 << GroupIndex);
		group->m_Passes[GroupIndex] = nullptr;
	}

	MTRenderer::EndPass();
	MTRenderer::AlphaBlendStateSetUnknown1(0);
	MTRenderer::UnlockShader(shaderType);

	GroupIndex++;
	return sub_14131E700(Technique, GroupIndex, PassIndexList);
}

void BSBatchRenderer::ClearRenderPasses()
{
	MemoryContextTracker tracker(MemoryContextTracker::RENDER_ACCUMULATOR, "BSBatchRenderer.cpp");

	for (auto itr = m_RenderPassMap.begin(); itr != m_RenderPassMap.end(); itr++)
	{
		if (itr)
			m_RenderPass[*itr].Clear(true, false);
	}

	m_ActivePassIndexList.RemoveAllNodes(sub_14131F910, (void *)(g_ModuleBase + 0x34B5230));
}

void UnmapDynamicData()
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();

	renderer->m_DeviceContext->Unmap(renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], 0);
}

void BSBatchRenderer::RenderPersistentPassList(PersistentPassList *PassList, uint32_t RenderFlags)
{
	if (!PassList->m_Head)
		return;

	MTRenderer::EndPass();

	bool mtrContext = MTRenderer::IsGeneratingGameCommandList();
	int lockType = PassList->m_Head->m_Shader->m_Type;

	MTRenderer::LockShader(lockType);

	for (BSRenderPass *i = PassList->m_Head; i; i = i->m_Next)
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
			if (i->m_ShaderProperty->GetFlag(BSShaderProperty::BSSP_FLAG_TWO_SIDED))
				MTRenderer::RasterStateSetCullMode(0);
			else
				MTRenderer::RasterStateSetCullMode(1);
		}

		bool alphaTest = i->m_Geometry->QAlphaProperty() && i->m_Geometry->QAlphaProperty()->GetAlphaTesting();

		if (mtrContext)
			MTRenderer::InsertCommand<MTRenderer::DrawGeometryRenderCommand>(i, i->m_PassEnum, alphaTest, RenderFlags);
		else
			BSBatchRenderer::RenderPassImmediately(i, i->m_PassEnum, alphaTest, RenderFlags);
	}

	if ((RenderFlags & 0x108) == 0)
		MTRenderer::RasterStateSetCullMode(1);

	PassList->Clear();

	MTRenderer::EndPass();
	MTRenderer::UnlockShader(lockType);
}

void BSBatchRenderer::RenderPassImmediately(BSRenderPass *Pass, uint32_t Technique, bool AlphaTest, uint32_t RenderFlags)
{
	auto *GraphicsGlobals = BSGraphics::Renderer::GetGlobals();
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);// LastPass
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);// LastShader
	BSShaderMaterial*& qword_1434B5220 = *(BSShaderMaterial **)((uintptr_t)GraphicsGlobals + 0x3500);// LastMaterial

	bool techniqueIsSetup = false;

	// BeginPass doesn't need to be called again if we used this shader previously
	if (dword_1432A8214 == Technique && Technique != 0x5C006076 && (uint64_t)Pass->m_Shader == qword_1432A8218)
		techniqueIsSetup = true;

	if (!techniqueIsSetup)
	{
		dword_141E32FDC = Technique;// This is written but never read anywhere?
		techniqueIsSetup = BeginPass(Pass->m_Shader, Technique);
	}

	if (techniqueIsSetup)
	{
		BSShaderProperty *property = Pass->m_ShaderProperty;
		BSShaderMaterial *material = nullptr;

		if (property)
			material = property->pMaterial;

		if (material != qword_1434B5220)
		{
			if (material)
				Pass->m_Shader->SetupMaterial(material);

			qword_1434B5220 = material;
		}

		*(BYTE *)((uintptr_t)Pass->m_Geometry + 264) = *(BYTE *)(&Pass->m_LODMode);// WARNING: MT data write hazard. ucCurrentMeshLODLevel?

		if (Pass->m_Geometry->QSkinInstance())
			RenderPassImmediately_Skinned(Pass, AlphaTest, RenderFlags);
		else if (*(BYTE *)((uintptr_t)Pass->m_Geometry + 265) & 8)// BSGeometry::NeedsCustomRender()?
			RenderPassImmediately_Custom(Pass, AlphaTest, RenderFlags);
		else
			RenderPassImmediately_Standard(Pass, AlphaTest, RenderFlags);
	}
}

void BSBatchRenderer::ShaderSetup(BSRenderPass *Pass, BSShader *Shader, bool AlphaTest, uint32_t RenderFlags)
{
	if (Shader != BSSkyShader::pInstance)
	{
		if ((RenderFlags & 4) && !BSShaderAccumulator::IsGrassShadowBlacklist(Pass->m_PassEnum))
			Shader->SetupGeometryAlphaBlending(Pass->QAlphaProperty(), Pass->m_ShaderProperty, AlphaTest);

		if (AlphaTest && Pass->QAlphaProperty())
			Shader->SetupAlphaTestRef(Pass->QAlphaProperty(), Pass->m_ShaderProperty);
	}

	Shader->SetupGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::RenderPassImmediately_Standard(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	MemoryContextTracker tracker(MemoryContextTracker::RENDER_ACCUMULATOR, "BSBatchRenderer.cpp");

	AssertMsgDebug(Pass, "Render Error: Render pass is nullptr");
	AssertMsgDebug(Pass->m_Geometry, "Render Error: Render pass geometry is nullptr");
	AssertMsgDebug(Pass->m_Shader, "Render Error: There is no BSShader attached to the geometry");

	ShaderSetup(Pass, Pass->m_Shader, AlphaTest || BSGraphics::gState.bUseEarlyZ, RenderFlags);
	Draw(Pass);
	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::RenderPassImmediately_Skinned(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	AssertMsgDebug(Pass, "Render Error: Render pass is nullptr");
	AssertMsgDebug(Pass->m_Geometry, "Render Error: Render pass geometry is nullptr");
	AssertMsgDebug(Pass->m_Shader, "Render Error: There is no BSShader attached to the geometry");

	AssertMsgDebug(Pass->m_Geometry->QSkinInstance(), "Render Error: Skin instance is nullptr");
	// "Render Error : Skin partition is nullptr"
	// "Render Error : Skin partition array is nullptr"
	AssertMsgDebug(Pass->m_ShaderProperty, "Don't have a shader property when we expected one.");

	AutoFunc(void(__fastcall *)(), sub_141336450, 0x1336450);
	sub_141336450();

	// BSTriShape::IsBSSkinnedDecalTriShape?
	if ((*(__int64(__fastcall **)(BSGeometry *))(*(uintptr_t *)Pass->m_Geometry + 432i64))(Pass->m_Geometry))
	{
		ShaderSetup(Pass, Pass->m_Shader, AlphaTest, RenderFlags);

		NiSkinPartition::Partition partition;
		partition.m_usBones = 1;

		Pass->m_Shader->SetBoneMatrix(Pass->m_Geometry->QSkinInstance(), &partition, &Pass->m_Geometry->GetWorldTransform());
		Draw(Pass);
	}
	else
	{
		ShaderSetup(Pass, Pass->m_Shader, AlphaTest, RenderFlags);

		SkinRenderData skinData(static_cast<NiBoneMatrixSetterI *>(Pass->m_Shader), Pass->m_Geometry, nullptr, Pass->m_LODMode.SingleLevel, Pass->m_LODMode.Index);

		// Runtime-updated vertices are sent to a GPU vertex buffer directly (non-static objects like trees/characters)
		BSDynamicTriShape *dynamicTri = Pass->m_Geometry->IsDynamicTriShape();

		if (dynamicTri)
		{
			const uint32_t size = dynamicTri->QDynamicDataSize();
			void *vertexBuffer = BSGraphics::Renderer::GetGlobals()->MapDynamicBuffer(size, &skinData.m_VertexBufferOffset);

			memcpy(vertexBuffer, dynamicTri->LockDynamicDataForRead(), size);

			dynamicTri->UnlockDynamicData();
			UnmapDynamicData();
		}

		// Renders multiple skinned instances (SetupTechnique, SetBoneMatrix)
		Pass->m_Geometry->QSkinInstance()->Render(&skinData);
	}

	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::RenderPassImmediately_Custom(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags)
{
	Pass->m_Shader->SetupGeometry(Pass, RenderFlags);
	Pass->m_Shader->SetupGeometryAlphaBlending(Pass->QAlphaProperty(), Pass->m_ShaderProperty, true);

	if (Pass->QAlphaProperty())
		Pass->m_Shader->SetupAlphaTestRef(Pass->QAlphaProperty(), Pass->m_ShaderProperty);

	Draw(Pass);
	Pass->m_Shader->RestoreGeometry(Pass, RenderFlags);
}

void BSBatchRenderer::Draw(BSRenderPass *Pass)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	BSGeometry *geometry = Pass->m_Geometry;

	switch (geometry->QType())
	{
	case GEOMETRY_TYPE_PARTICLES:
	{
		AssertDebug(geometry->IsParticlesGeom());

		int particleCount = (*(unsigned __int16(**)(void))(**(uintptr_t **)((uintptr_t)geometry + 344) + 304i64))();

		AutoFunc(void(__fastcall *)(BSRenderPass *), sub_14131DDF0, 0x131DDF0);
		sub_14131DDF0(Pass);
		return;

		// NiParticles::GetActiveVertexCount
		//int particleCount = (*(unsigned __int16(**)(void))(**(uintptr_t **)((uintptr_t)geometry + 344) + 304i64))();

		particleCount = std::min(particleCount, 2048);

		//AssertMsg(particleCount <= MAX_SHARED_PARTICLES_SIZE,
		//	"This emitter emits more particles than allowed in our rendering buffers. "
		//	"Please investigate emitter or increase MAX_SHARED_PARTICLES_SIZE");

		if (particleCount > 0)
		{
			BSGraphics::DynamicTriShapeDrawData drawData;
			BSGraphics::DynamicTriShape *triShape = renderer->GetParticlesDynamicTriShape();

			void *map = renderer->MapDynamicTriShapeDynamicData(nullptr, triShape, &drawData, particleCount * 4 * geometry->GetDynamicVertexSize());

			if (map)
			{
				//BSGraphics::Utility::PackDynamicParticleData(particleCount, (class BSGraphics::Utility::NiParticles *)geometry, map);
				renderer->UnmapDynamicTriShapeDynamicData(triShape, &drawData);
			}

			renderer->DrawDynamicTriShape(triShape, &drawData, 0, particleCount * 2);
		}
	}
	break;
	
	case GEOMETRY_TYPE_STRIP_PARTICLES:
	{
		// WARNING: Do not enable this function without fixing the input layout lookups first
		// Winterhold
		//Assert(false);
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
		AssertDebug(geometry->IsDynamicTriShape());

		auto dynTriShape = static_cast<BSDynamicTriShape *>(geometry);
		auto rendererData = reinterpret_cast<BSGraphics::DynamicTriShape *>(dynTriShape->QRendererData());

		// Only upload data once per frame
		if (dynTriShape->uiFrameCount != BSGraphics::gState.uiFrameCount)
		{
			dynTriShape->uiFrameCount = BSGraphics::gState.uiFrameCount;

			void *gpuData = renderer->MapDynamicTriShapeDynamicData(dynTriShape, rendererData, &dynTriShape->DrawData, 0);

			const void *dynamicData = dynTriShape->LockDynamicDataForRead();
			memcpy(gpuData, dynamicData, dynTriShape->DynamicDataSize);
			dynTriShape->UnlockDynamicData();

			renderer->UnmapDynamicTriShapeDynamicData(rendererData, &dynTriShape->DrawData);
		}

		renderer->DrawDynamicTriShape(rendererData, &dynTriShape->DrawData, 0, dynTriShape->m_TriangleCount);
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
		// Winterhold
		AutoFunc(void(__fastcall *)(BSRenderPass *), sub_14131DDF0, 0x131DDF0);
		sub_14131DDF0(Pass);
	}
	break;

	case GEOMETRY_TYPE_SUBINDEX_TRISHAPE:
	{
		AssertDebug(geometry->IsSubIndexTriShape());

		AutoFunc(void(__fastcall *)(BSRenderPass *), sub_14131DDF0, 0x131DDF0);
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
		AutoFunc(void(__fastcall *)(BSRenderPass *), sub_14131DDF0, 0x131DDF0);
		sub_14131DDF0(Pass);
	}
	break;

	case GEOMETRY_TYPE_PARTICLE_SHADER_DYNAMIC_TRISHAPE:
	{
		AssertDebug(geometry->IsDynamicTriShape());

		auto dynTriShape = static_cast<BSDynamicTriShape *>(geometry);

		auto dynamicData = dynTriShape->LockDynamicDataForRead();
		renderer->DrawParticleShaderTriShape(dynamicData, dynTriShape->m_VertexCount);
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