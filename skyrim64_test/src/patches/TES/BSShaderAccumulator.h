#pragma once

class BSBatchRenderer;

class BSShaderAccumulator
{
public:
	virtual void VDtor() = 0;	// Virtual destructor

	char _pad0[0x8];
	void *m_pkCamera;
	char _pad1[0x118];
	BSBatchRenderer *m_MainBatch;
	uint32_t m_CurrentTech;
	char _pad[0x44];

	static void sub_1412E1600(__int64 a1, unsigned int a2, float a3);
	void RenderTechniques(uint32_t StartTechnique, uint32_t EndTechnique, int a4, int PassType);
};
static_assert(sizeof(BSShaderAccumulator) == 0x180, "");
static_assert(offsetof(BSShaderAccumulator, _pad0) == 0x8, "");
static_assert(offsetof(BSShaderAccumulator, m_pkCamera) == 0x10, "");
static_assert(offsetof(BSShaderAccumulator, m_MainBatch) == 0x130, "");
static_assert(offsetof(BSShaderAccumulator, m_CurrentTech) == 0x138, "");