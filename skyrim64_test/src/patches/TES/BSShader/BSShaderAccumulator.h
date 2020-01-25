#pragma once

#include "../NiMain/NiObject.h"

class NiCamera;
class BSShader;
class BSBatchRenderer;
class BSGeometry;
class BSShaderProperty;

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
static_assert_offset(NiAccumulator, m_pkCamera, 0x10);

class NiBackToFrontAccumulator : public NiAccumulator
{
public:
	virtual ~NiBackToFrontAccumulator();

	char _pad[0x18];// NiTPointerList<BSGeometry *> m_kItems;
	int m_iNumItems;
	int m_iMaxItems;
	BSGeometry **m_ppkItems;
	float *m_pfDepths;
	int m_iCurrItem;
};
static_assert(sizeof(NiBackToFrontAccumulator) == 0x50, "");
//static_assert_offset(NiBackToFrontAccumulator, m_kItems, 0x18);
static_assert_offset(NiBackToFrontAccumulator, m_iNumItems, 0x30);
static_assert_offset(NiBackToFrontAccumulator, m_iMaxItems, 0x34);
static_assert_offset(NiBackToFrontAccumulator, m_ppkItems, 0x38);
static_assert_offset(NiBackToFrontAccumulator, m_pfDepths, 0x40);
static_assert_offset(NiBackToFrontAccumulator, m_iCurrItem, 0x48);

class NiAlphaAccumulator : public NiBackToFrontAccumulator
{
public:
	virtual ~NiAlphaAccumulator();

	bool m_bObserveNoSortHint;
	bool m_bSortByClosestPoint;
	bool m_bInterfaceSort;
};
static_assert(sizeof(NiAlphaAccumulator) == 0x58, "");
static_assert_offset(NiAlphaAccumulator, m_bObserveNoSortHint, 0x50);
static_assert_offset(NiAlphaAccumulator, m_bSortByClosestPoint, 0x51);
static_assert_offset(NiAlphaAccumulator, m_bInterfaceSort, 0x52);

class BSShaderAccumulator : public NiAlphaAccumulator
{
public:
	typedef bool(*REGISTEROBJECTFUNC)(BSShaderAccumulator *Accumulator, BSGeometry *Geometry, BSShaderProperty *Property, void *Unknown);
	typedef void(*FINISHACCUMULATINGFUNC)(BSShaderAccumulator *Accumulator, uint32_t Flags);

	virtual ~BSShaderAccumulator();
	virtual void StartAccumulating(NiCamera const *) override;
	virtual void FinishAccumulatingDispatch(uint32_t RenderFlags);

	char _pad1[0xD0];
	bool m_1stPerson;
	char _pad0[0x3];
	bool m_DrawDecals;
	BSBatchRenderer *m_BatchRenderer;
	uint32_t m_CurrentPass;
	uint32_t m_CurrentBucket;
	bool m_CurrentActive;
	char _pad[0x7];
	class ShadowSceneNode *m_ActiveShadowSceneNode;
	uint32_t m_RenderMode;
	char _pad2[0x18];
	NiPoint3 m_EyePosition;
	char _pad3[0x8];
	// @ 0x68 = Unknown float (1.0f)
	// @ 0x88 = Pointer to array of unknown D3D11 objects
	// @ 0xA0 = Pointer to array of unknown D3D11 objects
	// @ 0xD0 = BSMap<BSFadeNode, uint32_t>
	// @ 0x118 = NiPoint3(0.300, 0.300, 0.300)

	inline static REGISTEROBJECTFUNC RegisterObjectArray[30];
	inline static FINISHACCUMULATINGFUNC FinishAccumulatingArray[30];
	inline static REGISTEROBJECTFUNC RegisterObjectCurrent;
	inline static FINISHACCUMULATINGFUNC FinishAccumulatingCurrent;

	static void InitCallbackTable();
	static void SetRenderMode(uint32_t RenderMode);
	bool hk_RegisterObjectDispatch(BSGeometry *Geometry, void *Unknown);
	void hk_FinishAccumulatingDispatch(uint32_t RenderFlags);

	static bool IsGrassShadowBlacklist(uint32_t Technique);

	static bool RegisterObject_Standard(BSShaderAccumulator *Accumulator, BSGeometry *Geometry, BSShaderProperty *Property, void *Unknown);
	static bool RegisterObject_ShadowMapOrMask(BSShaderAccumulator *Accumulator, BSGeometry *Geometry, BSShaderProperty *Property, void *Unknown);

	static void FinishAccumulating_Standard(BSShaderAccumulator *Accumulator, uint32_t RenderFlags);
	static void FinishAccumulating_Standard_PreResolveDepth(BSShaderAccumulator * Accumulator, uint32_t RenderFlags);
	static void FinishAccumulating_Standard_PostResolveDepth(BSShaderAccumulator * Accumulator, uint32_t RenderFlags);

	static void FinishAccumulating_ShadowMapOrMask(BSShaderAccumulator *Accumulator, uint32_t RenderFlags);
	static void FinishAccumulating_InterfaceElements(BSShaderAccumulator *Accumulator, uint32_t RenderFlags);
	static void FinishAccumulating_FirstPerson(BSShaderAccumulator *Accumulator, uint32_t RenderFlags);
	static void FinishAccumulating_LODOnly(BSShaderAccumulator *Accumulator, uint32_t RenderFlags);
	static void FinishAccumulating_Unknown1(BSShaderAccumulator *Accumulator, uint32_t RenderFlags);

	void RenderGeometryGroup(uint32_t StartTechnique, uint32_t EndTechnique, uint32_t RenderFlags, int GeometryGroup);
	void RenderBatches(uint32_t StartTechnique, uint32_t EndTechnique, uint32_t RenderFlags, int GeometryGroup);
};
static_assert(sizeof(BSShaderAccumulator) == 0x180);
static_assert_offset(BSShaderAccumulator, _pad1, 0x58);
static_assert_offset(BSShaderAccumulator, m_1stPerson, 0x128);
static_assert_offset(BSShaderAccumulator, m_DrawDecals, 0x12C);
static_assert_offset(BSShaderAccumulator, m_BatchRenderer, 0x130);
static_assert_offset(BSShaderAccumulator, m_CurrentPass, 0x138);
static_assert_offset(BSShaderAccumulator, m_CurrentBucket, 0x13C);
static_assert_offset(BSShaderAccumulator, m_CurrentActive, 0x140);
static_assert_offset(BSShaderAccumulator, m_ActiveShadowSceneNode, 0x148);
static_assert_offset(BSShaderAccumulator, m_RenderMode, 0x150);
static_assert_offset(BSShaderAccumulator, m_EyePosition, 0x16C);

STATIC_CONSTRUCTOR(CheckBSShaderAccumulator, []
{
	//assert_vtable_index(&BSShaderAccumulator::~BSShaderAccumulator, 0);
	assert_vtable_index(&BSShaderAccumulator::StartAccumulating, 37);
	assert_vtable_index(&BSShaderAccumulator::FinishAccumulatingDispatch, 42);
});