#pragma once

#include "../NiMain/NiObject.h"

class NiCamera;
class NiRenderObject;
class BSBatchRenderer;

class NiAccumulator : public NiObject
{
public:
	virtual void StartAccumulating(NiCamera const *);
	virtual void FinishAccumulating();
	virtual void RegisterObjectArray(/*NiVisibleArray &*/);
	virtual void Unknown0();
	virtual void Unknown1();

	const NiCamera *m_pkCamera;
};
static_assert(sizeof(NiAccumulator) == 0x18, "");
static_assert(offsetof(NiAccumulator, m_pkCamera) == 0x10, "");

class NiBackToFrontAccumulator : public NiAccumulator
{
public:
	virtual ~NiBackToFrontAccumulator();

	char _pad[0x20];
	NiRenderObject **m_ppkItems;
	float *m_pfDepths;
	int m_iCurrItem;						// Guessed
};
static_assert(sizeof(NiBackToFrontAccumulator) == 0x50, "");
static_assert(offsetof(NiBackToFrontAccumulator, m_ppkItems) == 0x38, "");
static_assert(offsetof(NiBackToFrontAccumulator, m_pfDepths) == 0x40, "");
static_assert(offsetof(NiBackToFrontAccumulator, m_iCurrItem) == 0x48, "");

class NiAlphaAccumulator : public NiBackToFrontAccumulator
{
public:
	virtual ~NiAlphaAccumulator();

	bool m_bObserveNoSortHint;				// Guessed
	bool m_bSortByClosestPoint;
	bool m_UnknownByte52;
};
static_assert(sizeof(NiAlphaAccumulator) == 0x58, "");
static_assert(offsetof(NiAlphaAccumulator, m_bObserveNoSortHint) == 0x50, "");
static_assert(offsetof(NiAlphaAccumulator, m_bSortByClosestPoint) == 0x51, "");
static_assert(offsetof(NiAlphaAccumulator, m_UnknownByte52) == 0x52, "");

class BSShaderAccumulator : public NiAlphaAccumulator
{
public:
	virtual ~BSShaderAccumulator();

	virtual void StartAccumulating(NiCamera const *) override;
	virtual void FinishAccumulating() override;
	virtual void Unknown2();
	virtual void Unknown3();
	virtual void Unknown4();

	char _pad1[0xD8];
	BSBatchRenderer *m_MainBatch;
	uint32_t m_CurrentTech;
	uint32_t m_CurrentSubPass;
	char _pad[0x40];

	static void sub_1412E1600(__int64 a1, unsigned int a2, float a3);
	void RenderTechniques(uint32_t StartTechnique, uint32_t EndTechnique, int a4, int PassType);
};
static_assert(sizeof(BSShaderAccumulator) == 0x180, "");
static_assert(offsetof(BSShaderAccumulator, _pad1) == 0x58, "");
static_assert(offsetof(BSShaderAccumulator, m_MainBatch) == 0x130, "");
static_assert(offsetof(BSShaderAccumulator, m_CurrentTech) == 0x138, "");
static_assert(offsetof(BSShaderAccumulator, m_CurrentSubPass) == 0x13C, "");

void ClearShaderAndTechnique();
bool SetupShaderAndTechnique(BSShader *Shader, uint32_t Technique);