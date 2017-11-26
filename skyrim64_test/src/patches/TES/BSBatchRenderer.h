#pragma once

class BSBatchRenderer
{
public:
	struct PassInfo
	{
		BSBatchRenderer *m_BatchRenderer;
		uintptr_t UnkPtr2;
		uintptr_t UnkPtr3;
		uintptr_t UnkPtr4;
		char _pad[4];
		uint16_t UnkWord1;
		uint8_t UnkByte1;		// Flags

		void Render(unsigned int a2);
		void Unregister();
	};

	virtual void VDtor() = 0;	// Virtual destructor
	virtual void VFunc01() = 0;	// Registers a pass?
	virtual void VFunc02() = 0;	// Registers a pass?
	virtual void VFunc03() = 0;	// Unknown (render?)

	// BSTScatterTree<>
	char _pad1[0x48];
	uint32_t m_StartingTech;
	uint32_t m_EndingTech;
	char _pad2[0x18];
	// @ 0x68 = uint32_t iGroupingAlphas
	PassInfo *m_Passes[16];
	PassInfo *m_OtherPass;
	void *unk1;
	void *unk2;

	bool HasTechniquePasses(uint32_t StartTech, uint32_t EndTech);

	bool sub_14131E8F0(__int64 a1, unsigned int a2, signed int *a3);
	char sub_14131E700(uint32_t *a2, __int64 a3, __int64 a4);
	char sub_14131ECE0(uint32_t *a2, __int64 a3, __int64 a4);
	bool sub_14131E7B0(__int64 a1, uint32_t *a2, signed int *a3, __int64 *a4);
	char sub_14131E960(unsigned int *a2, unsigned int *a3, __int64 a4, unsigned int a5);
	static void sub_14131D6E0(__int64 a1);
};
static_assert(sizeof(BSBatchRenderer::PassInfo) == 0x28, "");
static_assert(offsetof(BSBatchRenderer::PassInfo, m_BatchRenderer) == 0x0, "");
static_assert(offsetof(BSBatchRenderer::PassInfo, UnkPtr2) == 0x8, "");
static_assert(offsetof(BSBatchRenderer::PassInfo, UnkPtr3) == 0x10, "");
static_assert(offsetof(BSBatchRenderer::PassInfo, UnkPtr4) == 0x18, "");
static_assert(offsetof(BSBatchRenderer::PassInfo, UnkWord1) == 0x24, "");
static_assert(offsetof(BSBatchRenderer::PassInfo, UnkByte1) == 0x26, "");

static_assert(sizeof(BSBatchRenderer) == 0x108, "");
static_assert(offsetof(BSBatchRenderer, _pad1) == 0x8, "");
static_assert(offsetof(BSBatchRenderer, m_StartingTech) == 0x50, "");
static_assert(offsetof(BSBatchRenderer, m_EndingTech) == 0x54, "");
static_assert(offsetof(BSBatchRenderer, m_Passes) == 0x70, "");
static_assert(offsetof(BSBatchRenderer, m_OtherPass) == 0xF0, "");
static_assert(offsetof(BSBatchRenderer, unk1) == 0xF8, "");
static_assert(offsetof(BSBatchRenderer, unk2) == 0x100, "");