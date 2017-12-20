#pragma once

#include "../NiObject.h"

class NiCamera;
class BSBatchRenderer;
class NiRenderObject;

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
	char _pad[0x44];

	static void sub_1412E1600(__int64 a1, unsigned int a2, float a3);
	void RenderTechniques(uint32_t StartTechnique, uint32_t EndTechnique, int a4, int PassType);
};
static_assert(sizeof(BSShaderAccumulator) == 0x180, "");
static_assert(offsetof(BSShaderAccumulator, _pad1) == 0x58, "");
static_assert(offsetof(BSShaderAccumulator, m_MainBatch) == 0x130, "");
static_assert(offsetof(BSShaderAccumulator, m_CurrentTech) == 0x138, "");

void sub_14131F090();
void BSGraphics__Renderer__RasterStateSetCullMode(uint32_t CullMode);
void BSGraphics__Renderer__AlphaBlendStateSetMode(uint32_t Mode);
void BSGraphics__Renderer__AlphaBlendStateSetUnknown1(uint32_t Value);
void BSGraphics__Renderer__AlphaBlendStateSetUnknown2(uint32_t Value);
void BSGraphics__Renderer__DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef);
void BSGraphics__Renderer__DepthStencilStateSetDepthMode(uint32_t Mode);
void BSGraphics__Renderer__SetTextureFilterMode(uint32_t Index, uint32_t Mode);
void BSGraphics__Renderer__SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode);
void BSGraphics__Renderer__SetUseScrapConstantValue(bool UseStoredValue);
void BSGraphics__Renderer__SetUseScrapConstantValue(bool UseStoredValue, float Value);

struct RenderCommand
{
	int m_Type;
	int m_Size;

	RenderCommand(int Type, int Size)
	{
		m_Type = Type;
		m_Size = Size;
	}
};

struct EndListRenderCommand : RenderCommand
{
	// Terminates execution sequence
	EndListRenderCommand()
		: RenderCommand(0, sizeof(EndListRenderCommand))
	{
	}
};

struct ClearStateRenderCommand : RenderCommand
{
	// Equivalent to calling sub_14131F090();
	ClearStateRenderCommand()
		: RenderCommand(1, sizeof(ClearStateRenderCommand))
	{
	}

	void Run()
	{
		sub_14131F090();
	}
};

struct SetStateRenderCommand : RenderCommand
{
	// Equivalent to BSGraphics::Renderer::SetXYZ functions
	enum StateVar
	{
		RasterStateCullMode,
		AlphaBlendStateMode,
		AlphaBlendStateUnknown1,
		AlphaBlendStateUnknown2,
		DepthStencilStateStencilMode,
		DepthStencilStateDepthMode,
		UseScrapConstantValue_1,
		UseScrapConstantValue_2,
	};

	union
	{
		struct
		{
			uint32_t part1;
			uint32_t part2;
		};

		__int64 all;
	} Data;

	StateVar m_StateType;

	SetStateRenderCommand(StateVar Type, uint32_t Arg1 = 0, uint32_t Arg2 = 0)
		: RenderCommand(2, sizeof(SetStateRenderCommand))
	{
		m_StateType = Type;
		Data.part1 = Arg1;
		Data.part2 = Arg2;

		switch (m_StateType)
		{
		case RasterStateCullMode:
		case AlphaBlendStateMode:
		case AlphaBlendStateUnknown1:
		case AlphaBlendStateUnknown2:
		case DepthStencilStateStencilMode:
		case DepthStencilStateDepthMode:
		case UseScrapConstantValue_1:
		case UseScrapConstantValue_2:
			break;

		default:
			__debugbreak();
		}
	}

	void Run()
	{
		switch (m_StateType)
		{
		case RasterStateCullMode:
			BSGraphics__Renderer__RasterStateSetCullMode(Data.part1);
			break;

		case AlphaBlendStateMode:
			BSGraphics__Renderer__AlphaBlendStateSetMode(Data.part1);
			break;

		case AlphaBlendStateUnknown1:
			BSGraphics__Renderer__AlphaBlendStateSetUnknown1(Data.part1);
			break;

		case AlphaBlendStateUnknown2:
			BSGraphics__Renderer__AlphaBlendStateSetUnknown2(Data.part1);
			break;

		case DepthStencilStateStencilMode:
			BSGraphics__Renderer__DepthStencilStateSetStencilMode(Data.part1, Data.part2);
			break;

		case DepthStencilStateDepthMode:
			BSGraphics__Renderer__DepthStencilStateSetDepthMode(Data.part1);
			break;

		case UseScrapConstantValue_1:
			BSGraphics__Renderer__SetUseScrapConstantValue((bool)Data.part1);
			break;

		case UseScrapConstantValue_2:
			BSGraphics__Renderer__SetUseScrapConstantValue((bool)Data.part1, *(float *)&Data.part2);
			break;

		default:
			__debugbreak();
		}
	}
};

void sub_14131ED70(BSRenderPass *a1, uint32_t Technique, unsigned __int8 a3, unsigned int a4);
struct DrawGeometryRenderCommand : RenderCommand
{
	// Equivalent to calling sub_14131ED70();
	BSRenderPass Pass;
	uint32_t Technique;
	unsigned __int8 a3;
	unsigned int a4;

	DrawGeometryRenderCommand(BSRenderPass *Arg1, uint32_t Arg2, unsigned __int8 Arg3, uint32_t Arg4)
		: RenderCommand(3, sizeof(DrawGeometryRenderCommand))
	{
		memcpy(&Pass, Arg1, sizeof(BSRenderPass));
		Technique = Arg2;
		a3 = Arg3;
		a4 = Arg4;

		//
		// **** WARNING ****
		// The game code shouldn't really be using this pointer after the initial batching
		// stage, but just to make sure...
		//
		if (Arg1->m_Next)
			Pass.m_Next = (BSRenderPass *)0xCCCCCCCCDEADBEEF;
	}

	void Run()
	{
		sub_14131ED70(&Pass, Technique, a3, a4);
	}
};

struct LockShaderTypeRenderCommand : RenderCommand
{
	int m_LockIndex;
	bool m_Lock;

	LockShaderTypeRenderCommand(int LockIndex, bool Lock)
		: RenderCommand(4, sizeof(LockShaderTypeRenderCommand))
	{
		m_LockIndex = LockIndex;
		m_Lock = Lock;
	}
};

extern uintptr_t commandData[3];
extern thread_local int ThreadUsingCommandList;

template<typename T, class ... Types>
bool InsertRenderCommand(Types&& ...args)
{
	static_assert(sizeof(T) % 8 == 0);

	if (!ThreadUsingCommandList)
		return false;

	// Utilize placement new, then increment to next command slot
	new ((void *)commandData[ThreadUsingCommandList - 1]) T(args...);
	commandData[ThreadUsingCommandList - 1] += sizeof(T);

	return true;
}