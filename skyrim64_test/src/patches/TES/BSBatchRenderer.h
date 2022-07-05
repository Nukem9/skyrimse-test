#pragma once

#include "BSTArray.h"
#include "BSTList.h"
#include "BSTScatterTable.h"
#include "BSShader/BSShaderManager.h"

class __declspec(align(8)) BSBatchRenderer
{
public:
	struct __declspec(align(8)) PersistentPassList
	{
		BSRenderPass *m_Head;
		BSRenderPass *m_Tail;

		void Clear();
	};

	struct __declspec(align(8)) GeometryGroup
	{
		BSBatchRenderer *m_BatchRenderer;
		PersistentPassList m_PassList;
		uintptr_t UnkPtr4;
		float m_Depth;	// Distance from geometry to camera location
		uint16_t m_Count;
		uint8_t m_Flags;	// Flags

		void Render(uint32_t RenderFlags);
		void ClearAndFreePasses();
	};

	struct __declspec(align(8)) PassGroup
	{
		BSRenderPass *m_Passes[5];
		uint32_t m_ValidPassBits;	// OR'd with (1 << PassIndex)

		void Clear(bool ReportNotEmpty, bool FreePasses);// Simply zeros this structure
	};

	virtual ~BSBatchRenderer();
	virtual void VFunc01();															// Registers a pass?
	virtual void VFunc02();															// Registers a pass?
	virtual void VFunc03(uint32_t StartTech, uint32_t EndTech, uint32_t RenderFlags);// Unknown (renders something?)

	BSTArray<PassGroup> m_RenderPass;
	BSTDefaultScatterTable<uint32_t, uint32_t> m_RenderPassMap;// Technique ID -> Index in m_RenderPass
	uint32_t m_CurrentFirstPass;
	uint32_t m_CurrentLastPass;
	BSSimpleList<uint32_t> m_ActivePassIndexList;
	int m_GroupingAlphas;
	bool m_AutoClearPasses;
	GeometryGroup *m_GeometryGroups[16];
	GeometryGroup *m_AlphaGroup;
	void *unk1;
	void *unk2;

	static bool BeginPass(BSShader *Shader, uint32_t Technique);
	static void EndPass();

	bool QPassesWithinRange(uint32_t StartTech, uint32_t EndTech);

	bool sub_14131E8F0(unsigned int a2, uint32_t& GroupIndex);
	bool sub_14131E700(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList);
	bool DiscardBatches(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList);
	bool sub_14131E7B0(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList);
	bool RenderBatches(uint32_t& Technique, uint32_t& GroupIndex, BSSimpleList<uint32_t> *&PassIndexList, uint32_t RenderFlags);
	void ClearRenderPasses();

	static void RenderPersistentPassList(PersistentPassList *PassList, uint32_t RenderFlags);
	static void RenderPassImmediately(BSRenderPass *Pass, uint32_t Technique, bool AlphaTest, uint32_t RenderFlags);
	static void ShaderSetup(BSRenderPass *Pass, BSShader *Shader, bool AlphaTest, uint32_t RenderFlags);
	static void RenderPassImmediately_Standard(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags);
	static void RenderPassImmediately_Skinned(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags);
	static void RenderPassImmediately_Custom(BSRenderPass *Pass, bool AlphaTest, uint32_t RenderFlags);
	static void Draw(BSRenderPass *Pass);
};
static_assert(sizeof(BSBatchRenderer::GeometryGroup) == 0x28);
static_assert_offset(BSBatchRenderer::GeometryGroup, m_BatchRenderer, 0x0);
static_assert_offset(BSBatchRenderer::GeometryGroup, m_PassList, 0x8);
static_assert_offset(BSBatchRenderer::GeometryGroup, UnkPtr4, 0x18);
static_assert_offset(BSBatchRenderer::GeometryGroup, m_Depth, 0x20);
static_assert_offset(BSBatchRenderer::GeometryGroup, m_Count, 0x24);
static_assert_offset(BSBatchRenderer::GeometryGroup, m_Flags, 0x26);

static_assert(sizeof(BSBatchRenderer::PassGroup) == 0x30);

static_assert(sizeof(BSBatchRenderer) == 0x108);
static_assert_offset(BSBatchRenderer, m_RenderPass, 0x8);
static_assert_offset(BSBatchRenderer, m_RenderPassMap, 0x20);
static_assert_offset(BSBatchRenderer, m_CurrentFirstPass, 0x50);
static_assert_offset(BSBatchRenderer, m_CurrentLastPass, 0x54);
static_assert_offset(BSBatchRenderer, m_ActivePassIndexList, 0x58);
static_assert_offset(BSBatchRenderer, m_GroupingAlphas, 0x68);
static_assert_offset(BSBatchRenderer, m_AutoClearPasses, 0x6C);
static_assert_offset(BSBatchRenderer, m_GeometryGroups, 0x70);
static_assert_offset(BSBatchRenderer, m_AlphaGroup, 0xF0);
static_assert_offset(BSBatchRenderer, unk1, 0xF8);
static_assert_offset(BSBatchRenderer, unk2, 0x100);