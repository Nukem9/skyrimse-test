#pragma once

#include "../NiMain/NiObject.h"

class NiCamera;
class NiRenderObject;
class BSShader;
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
	typedef void(*RegisterObjFunc)(void *, void *, void *, void *);
	typedef void(*FinishAccumFunc)(BSShaderAccumulator *Accumulator, uint32_t Flags);

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
	bool m_HasPendingDraws;
	char _pad[0x27];
	NiPoint3 m_CurrentViewPos;
	char _pad2[0xC];
	// @ 0x68 = Unknown float (1.0f)
	// @ 0x88 = Pointer to array of unknown D3D11 objects
	// @ 0xA0 = Pointer to array of unknown D3D11 objects
	// @ 0xD0 = BSMap<BSFadeNode, uint32_t>
	// @ 0x118 = NiPoint3(0.300, 0.300, 0.300)
	// @ 0x148 = ShadowSceneNode/NiNode

	inline static RegisterObjFunc RegisterObjectArray[30];
	inline static FinishAccumFunc FinishAccumulatingArray[30];
	inline static RegisterObjFunc RegisterObjectCurrent;
	inline static FinishAccumFunc FinishAccumulatingCurrent;

	static void InitCallbackTable();
	static void SetRenderMode(uint32_t RenderMode);

	void sub_1412E1600(uint32_t RenderFlags);
	void RenderTechniques(uint32_t StartTechnique, uint32_t EndTechnique, uint32_t RenderFlags, int GroupType);
};
static_assert(sizeof(BSShaderAccumulator) == 0x180, "");
static_assert(offsetof(BSShaderAccumulator, _pad1) == 0x58, "");
static_assert(offsetof(BSShaderAccumulator, m_MainBatch) == 0x130, "");
static_assert(offsetof(BSShaderAccumulator, m_CurrentTech) == 0x138, "");
static_assert(offsetof(BSShaderAccumulator, m_CurrentSubPass) == 0x13C, "");
static_assert(offsetof(BSShaderAccumulator, m_HasPendingDraws) == 0x140, "");
static_assert(offsetof(BSShaderAccumulator, m_CurrentViewPos) == 0x168, "");