#include "../../common.h"
#include "../../ui/ui.h"
#include "../TES/BSShaderManager.h"
#include "../TES/BSShaderRenderTargets.h"

// BSGraphicsRenderer

#define CHECK_OFFSET(member, actualAddr) static_assert(offsetof(BSGraphicsRendererGlobals, member) == (actualAddr - 0x14304BEF0), "")

struct BSGraphicsRendererGlobals
{
	float			m_Viewport[4];						// CHECK THIS!!

	void *qword_14304BF00;								// Unknown class pointer

	ID3D11Device	*m_Device;
	HWND			m_Window;

	//
	// These are pools for efficient data uploads to the GPU. Each frame can use any buffer as long as there
	// is sufficient space. If there's no space left, delay execution until m_CommandListEndEvents[] says a buffer
	// is no longer in use.
	//
	ID3D11Buffer		*m_DynamicBuffers[3];			// DYNAMIC (VERTEX | INDEX) CPU_ACCESS_WRITE
	uint32_t			m_CurrentDynamicBufferIndex;

	uint32_t			m_FrameDataUsedSize;			// Use in relation with m_CommandListEndEvents[]
	ID3D11Buffer		*m_UnknownIndexBuffer;			// DEFAULT INDEX CPU_ACCESS_NONE
	ID3D11Buffer		*m_UnknownVertexBuffer;			// DEFAULT VERTEX CPU_ACCESS_NONE
	ID3D11InputLayout	*m_UnknownInputLayout;
	ID3D11InputLayout	*m_UnknownInputLayout2;
	uint32_t			m_UnknownCounter;				// 0 to 63
	uint32_t			m_UnknownCounter2;				// No limits
	void				*m_UnknownStaticBuffer[64];
	uint32_t			m_UnknownCounter3;				// 0 to 5
	bool				m_EventQueryFinished[3];
	ID3D11Query			*m_CommandListEndEvents[3];		// D3D11_QUERY_EVENT (Waits for a series of commands to finish execution)

	float m_UnknownFloats1[3][4];						// Probably a matrix

	void *qword_14304C1B0[6][40];						// Wtf? (Probably a weird sampler state setup)
	void *qword_14304C930[2][3][12][2];					// Wtf?
	void *qword_14304CDB0[7][2][13][2];					// Wtf?
	void *qword_14304D910[6][5];						// Wtf?

	//
	// Vertex/Pixel shader constant buffers. Set during load-time (CreateShaderBundle).
	//
	uint32_t		m_NextConstantBufferIndex;
	ID3D11Buffer	*m_ConstantBuffers1[4];				// Sizes: 3840 bytes
	ID3D11Buffer	*m_TempConstantBuffer1;				// 16 bytes
	void			*qword_14304DA30;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers2[19];			// Sizes: 16, 32, 48, ... 304 bytes
	void			*qword_14304DAD0;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers3[9];				// Sizes: 16, 32, 48, ... 144 bytes
	void			*qword_14304DB20;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers4[27];			// Sizes: 16, 32, 48, ... 432 bytes
	void			*qword_14304DC00;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers5[19];			// Sizes: 16, 32, 48, ... 304 bytes
	void			*qword_14304DCA0;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers6[19];			// Sizes: 16, 32, 48, ... 304 bytes
	void			*qword_14304DD40;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers7[39];			// Sizes: 16, 32, 48, ... 624 bytes
	ID3D11Buffer	*m_TempConstantBuffer2;				// 576 bytes
	ID3D11Buffer	*m_TempConstantBuffer3;				// 720 bytes
	ID3D11Buffer	*m_TempConstantBuffer4;				// 16 bytes

	IDXGIOutput *m_DXGIAdapterOutput;
	ID3D11DeviceContext *m_DeviceContext;

	void *m_FrameDurationStringHandle;					// "Frame Duration" but stored in their global string pool

	uint32_t dword_14304DEB0;							// Flags; probably global technique modifiers
	uint32_t m_PSResourceModifiedBits;					// Flags
	uint32_t m_PSSamplerModifiedBits;					// Flags
	uint32_t m_DSSamplerModifiedBits;					// Flags
	uint32_t m_CSSamplerModifiedBits;					// Flags
	uint32_t m_CSUAVModifiedBits;						// Flags

	uint32_t dword_14304DEC8[8];

	uint32_t rshadowState_iDepthStencil;				// Index
	uint32_t rshadowState_iDepthStencilSlice;			// Index
	uint32_t iRenderTargetIndexes[5][2];				// Index[0] = Base target, Index[1] = Slice target

	char __zz0[0x50];
	float float_14304DF68;								// Possibly something to do with diffuse

	uint32_t m_PSSamplerSetting1[16];
	uint32_t m_PSSamplerSetting2[16];
	ID3D11ShaderResourceView *m_PSResources[16];

	uint32_t m_CSSamplerSetting1[16];
	uint32_t m_CSSamplerSetting2[16];

	ID3D11SamplerState *m_DSSamplers[16];
	char __zz1[0x40];
	ID3D11UnorderedAccessView *m_CSUAVResources[8];
	char __zz2[0x2A0];
};
BSGraphicsRendererGlobals *GraphicsGlobals = nullptr;

static_assert(sizeof(BSGraphicsRendererGlobals) == 0x25A0, "");
CHECK_OFFSET(m_Viewport, 0x14304BEF0);
CHECK_OFFSET(qword_14304BF00, 0x14304BF00);
CHECK_OFFSET(m_Device, 0x14304BF08);
CHECK_OFFSET(m_Window, 0x14304BF10);
CHECK_OFFSET(m_DynamicBuffers, 0x14304BF18);
CHECK_OFFSET(m_CurrentDynamicBufferIndex, 0x14304BF30);
CHECK_OFFSET(m_FrameDataUsedSize, 0x14304BF34);
CHECK_OFFSET(m_UnknownIndexBuffer, 0x14304BF38);
CHECK_OFFSET(m_UnknownVertexBuffer, 0x14304BF40);
CHECK_OFFSET(m_UnknownInputLayout, 0x14304BF48);
CHECK_OFFSET(m_UnknownInputLayout2, 0x14304BF50);
CHECK_OFFSET(m_UnknownCounter, 0x14304BF58);
CHECK_OFFSET(m_UnknownCounter2, 0x14304BF5C);
CHECK_OFFSET(m_UnknownStaticBuffer, 0x14304BF60);
CHECK_OFFSET(m_UnknownCounter3, 0x14304C160);
CHECK_OFFSET(m_EventQueryFinished, 0x14304C164);
CHECK_OFFSET(m_CommandListEndEvents, 0x14304C168);
CHECK_OFFSET(m_UnknownFloats1, 0x14304C180);
CHECK_OFFSET(qword_14304C1B0, 0x14304C1B0);
CHECK_OFFSET(qword_14304C930, 0x14304C930);
CHECK_OFFSET(qword_14304CDB0, 0x14304CDB0);
CHECK_OFFSET(qword_14304D910, 0x14304D910);
CHECK_OFFSET(m_NextConstantBufferIndex, 0x14304DA00);
CHECK_OFFSET(m_ConstantBuffers1, 0x14304DA08);
CHECK_OFFSET(m_TempConstantBuffer1, 0x14304DA28);
CHECK_OFFSET(qword_14304DA30, 0x14304DA30);
CHECK_OFFSET(m_ConstantBuffers2, 0x14304DA38);
CHECK_OFFSET(qword_14304DAD0, 0x14304DAD0);
CHECK_OFFSET(m_ConstantBuffers3, 0x14304DAD8);
CHECK_OFFSET(qword_14304DB20, 0x14304DB20);
CHECK_OFFSET(m_ConstantBuffers4, 0x14304DB28);
CHECK_OFFSET(qword_14304DC00, 0x14304DC00);
CHECK_OFFSET(m_ConstantBuffers5, 0x14304DC08);
CHECK_OFFSET(qword_14304DCA0, 0x14304DCA0);
CHECK_OFFSET(m_ConstantBuffers6, 0x14304DCA8);
CHECK_OFFSET(qword_14304DD40, 0x14304DD40);
CHECK_OFFSET(m_ConstantBuffers7, 0x14304DD48);
CHECK_OFFSET(m_TempConstantBuffer2, 0x14304DE80);
CHECK_OFFSET(m_TempConstantBuffer3, 0x14304DE88);
CHECK_OFFSET(m_TempConstantBuffer4, 0x14304DE90);
CHECK_OFFSET(m_DXGIAdapterOutput, 0x14304DE98);
CHECK_OFFSET(m_DeviceContext, 0x14304DEA0);
CHECK_OFFSET(m_FrameDurationStringHandle, 0x14304DEA8);
CHECK_OFFSET(dword_14304DEB0, 0x14304DEB0);
CHECK_OFFSET(m_PSResourceModifiedBits, 0x14304DEB4);
CHECK_OFFSET(m_PSSamplerModifiedBits, 0x14304DEB8);
CHECK_OFFSET(m_DSSamplerModifiedBits, 0x14304DEBC);
CHECK_OFFSET(m_CSSamplerModifiedBits, 0x14304DEC0);
CHECK_OFFSET(m_CSUAVModifiedBits, 0x14304DEC4);
CHECK_OFFSET(dword_14304DEC8, 0x14304DEC8);
CHECK_OFFSET(rshadowState_iDepthStencil, 0x14304DEE8);
CHECK_OFFSET(rshadowState_iDepthStencilSlice, 0x14304DEEC);
CHECK_OFFSET(iRenderTargetIndexes, 0x14304DEF0);
CHECK_OFFSET(__zz0, 0x14304DF18);
CHECK_OFFSET(float_14304DF68, 0x14304DF68);
CHECK_OFFSET(m_PSSamplerSetting1, 0x14304DF6C);
CHECK_OFFSET(m_PSSamplerSetting2, 0x14304DFAC);
CHECK_OFFSET(m_PSResources, 0x14304DFF0);
CHECK_OFFSET(m_CSSamplerSetting1, 0x14304E070);
CHECK_OFFSET(m_CSSamplerSetting2, 0x14304E0B0);
CHECK_OFFSET(m_DSSamplers, 0x14304E0F0);
CHECK_OFFSET(m_CSUAVResources, 0x14304E1B0);

IDXGISwapChain *g_SwapChain;
ID3D11DeviceContext2 *g_DeviceContext;
double g_AverageFps;
ID3DUserDefinedAnnotation *annotation;

decltype(&IDXGISwapChain::Present) ptrPresent;
decltype(&CreateDXGIFactory) ptrCreateDXGIFactory;
decltype(&D3D11CreateDeviceAndSwapChain) ptrD3D11CreateDeviceAndSwapChain;

ID3D11DeviceContext2 *dc1; // deferred context
ID3D11DeviceContext2 *dc2;

void UpdateHavokTimer(int FPS)
{
    static int oldFps;

    // Limit Havok FPS between 30 and 150
    FPS = min(max(FPS, 30), 150);

    // Allow up to 5fps difference
    if (abs(oldFps - FPS) >= 5)
    {
        oldFps = FPS;

        // Round up to nearest 5...and add 5
        int newFPS     = ((FPS + 5 - 1) / 5) * 5;
        float newRatio = 1.0f / (float)(newFPS + 5);

        InterlockedExchange((volatile LONG *)(g_ModuleBase + 0x1DADCA0), *(LONG *)&newRatio); // fMaxTime
        InterlockedExchange((volatile LONG *)(g_ModuleBase + 0x1DADE38), *(LONG *)&newRatio); // fMaxTimeComplex
    }
}

int64_t LastFrame;
int64_t TickSum;
int64_t TickDeltas[32];
int TickDeltaIndex;


HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
    //ui::Render();

    // FPS calculation code
    LARGE_INTEGER ticksPerSecond;
    QueryPerformanceFrequency(&ticksPerSecond);

    LARGE_INTEGER frameEnd;
    QueryPerformanceCounter(&frameEnd);

    if (LastFrame == 0)
        LastFrame = frameEnd.QuadPart;

    int64_t delta = frameEnd.QuadPart - LastFrame;
    LastFrame     = frameEnd.QuadPart;

    TickSum -= TickDeltas[TickDeltaIndex];
    TickSum += delta;
    TickDeltas[TickDeltaIndex++] = delta;

    if (TickDeltaIndex >= ARRAYSIZE(TickDeltas))
        TickDeltaIndex = 0;

    double frameTimeMs      = 1000.0 * (delta / (double)ticksPerSecond.QuadPart);
    double averageFrametime = (TickSum / 32.0) / (double)ticksPerSecond.QuadPart;
    g_AverageFps            = 1.0 / averageFrametime;

    //if (ui::opt::LogHitches && frameTimeMs >= 100.0)
    //    ui::log::Add("FRAME HITCH WARNING (%g ms)\n", frameTimeMs);

    HRESULT hr = (This->*ptrPresent)(SyncInterval, Flags);

	using namespace BSShaderRenderTargets;

	if (!g_DeviceContext)
	{
		ID3D11DeviceContext1 *ctx = nullptr;
		ID3D11Device1 *dev;
		This->GetDevice(__uuidof(ID3D11Device1), (void **)&dev);
		dev->GetImmediateContext1(&ctx);

		ctx->QueryInterface<ID3D11DeviceContext2>(&g_DeviceContext);
		ctx->QueryInterface<ID3DUserDefinedAnnotation>(&annotation);
	}

	//
	// Certain SLI bits emulate this behavior, but for all render targets. If the game uses ClearRenderTargetView(),
	// we probably don't need to discard.
	//
	// These can't be discarded without rewriting engine code:
	// - RENDER_TARGET_MAIN_ONLY_ALPHA
	// - RENDER_TARGET_MENUBG
	// - RENDER_TARGET_WATER_1 (Consume/Write every other frame)
	// - RENDER_TARGET_WATER_2 (Consume/Write every other frame)
	// - DEPTH_STENCIL_TARGET_MAIN_COPY
	//
	// NvAPI_D3D_SetResourceHint() or agsDriverExtensionsDX11_CreateTexture2D(TransferDisable) are better options.
	//
	annotation->BeginEvent(L"SLI Hacks");
	{
		g_DeviceContext->ClearState();

		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_MAIN]);				// Overwrite: ClearRTV()
		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_MAIN_COPY]);			// Overwrite: DrawIndexed()

		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_SHADOW_MASK]);		// Overwrite: ClearRTV()

		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_RAW_WATER]);			// Dirty

		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_SSR]);				// Overwrite: Dispatch()
		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_SSR_RAW]);			// Overwrite: DrawIndexed()
		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_SSR_BLURRED0]);		// Overwrite: Dispatch()

		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_SNOW_SWAP]);			// Overwrite: DrawIndexed()
		g_DeviceContext->DiscardResource(g_RenderTargetTextures[RENDER_TARGET_MENUBG]);				// Dirty 99% of the time

		g_DeviceContext->DiscardResource(g_DepthStencilTextures[DEPTH_STENCIL_TARGET_MAIN]);
		g_DeviceContext->DiscardResource(g_DepthStencilTextures[DEPTH_STENCIL_TARGET_SHADOWMAPS_ESRAM]);// Uses 2 4096x4096 slices and both are overwritten. Note: They clear both
																										// slices SEPARATELY (i.e clear s0, render s0, clear s1, render s1) which
																										// may cause dependency issues on slice 1. I hope this fixes it.

		const float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		g_DeviceContext->ClearRenderTargetView(g_RenderTargets[RENDER_TARGET_RAW_WATER], black);
		g_DeviceContext->ClearRenderTargetView(g_RenderTargets[RENDER_TARGET_MENUBG], black);		// Fixes flickering in the system menu, but background screen is black
	}
	annotation->EndEvent();

	return hr;
}

void *sub_140D6BF00(__int64 a1, int AllocationSize, uint32_t *AllocationOffset)
{
//	GraphicsGlobals = (BSGraphicsRendererGlobals *)tlsGlob;

	uint32_t frameDataOffset = GraphicsGlobals->m_FrameDataUsedSize;
	uint32_t frameBufferIndex = GraphicsGlobals->m_CurrentDynamicBufferIndex;
	uint32_t newFrameDataSzie = GraphicsGlobals->m_FrameDataUsedSize + AllocationSize;

	//
	// Check if this request would exceed the allocated buffer size for the currently executing command list. If it does,
	// we end the current query and move on to the next buffer.
	//
	if (newFrameDataSzie > 0x400000)
	{
		newFrameDataSzie = AllocationSize;
		frameDataOffset = 0;

		GraphicsGlobals->m_EventQueryFinished[GraphicsGlobals->m_CurrentDynamicBufferIndex] = false;
		g_DeviceContext->End(GraphicsGlobals->m_CommandListEndEvents[GraphicsGlobals->m_CurrentDynamicBufferIndex]);

		frameBufferIndex++;

		if (frameBufferIndex >= 3)
			frameBufferIndex = 0;
	}
	
	//
	// This will **suspend execution** until the buffer we want is no longer in use. The query encapsulates a list of commands
	// using said buffer.
	//
	if (!GraphicsGlobals->m_EventQueryFinished[frameBufferIndex])
	{
		ID3D11Query *query = GraphicsGlobals->m_CommandListEndEvents[frameBufferIndex];
		BOOL data;

		HRESULT hr = g_DeviceContext->GetData(query, &data, sizeof(data), 0);

		for (; FAILED(hr) || data == FALSE; hr = g_DeviceContext->GetData(query, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH))
			Sleep(1);

		GraphicsGlobals->m_EventQueryFinished[frameBufferIndex] = (data == TRUE);
	}
	
	D3D11_MAPPED_SUBRESOURCE resource;
	if (FAILED(g_DeviceContext->Map(GraphicsGlobals->m_DynamicBuffers[frameBufferIndex], 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource)))
	{
		//TLS_DeviceContext->Map(GraphicsGlobals->m_DynamicBuffers[frameBufferIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
		//*AllocationOffset = 0;
		//frameDataOffset = 0;
	}

	GraphicsGlobals->m_CurrentDynamicBufferIndex = frameBufferIndex;
	*AllocationOffset = frameDataOffset;
	GraphicsGlobals->m_FrameDataUsedSize = newFrameDataSzie;

	return (void *)((uintptr_t)resource.pData + frameDataOffset);
}

extern thread_local char BSGraphics_TLSGlob[0x4000];

void DC_Init();
void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int));
void DC_WaitDeferred();

void __fastcall hk_sub_1412E1600(__int64 a1, unsigned int a2, float a3)
{
	float v3; // xmm0_4
	uint32_t v6; // ecx
	bool v7; // si
	__int64 v8; // r8
	__int64 v10; // r8
	uint32_t v13; // ecx
	__int64 v16; // r8
	__int64 v17; // rcx
	uint32_t v18; // eax
	int v19; // eax
	int v20; // eax
	uint32_t v21; // eax
	uint32_t v22; // ecx
	float v15; // xmm0_4

	auto graphicsGlobals = (BSGraphicsRendererGlobals *)BSGraphics_TLSGlob;

	if (!annotation)
	{
		DC_Init();
		graphicsGlobals->m_DeviceContext->QueryInterface<ID3DUserDefinedAnnotation>(&annotation);
	}

	auto RenderBatchTechnique1 = (__int64(__fastcall *)(__int64 a1, int a2, int a3, int a4, int a5))(g_ModuleBase + 0x12E3770);
	auto RenderBatchTechnique2 = (void(__fastcall *)(__int64 *a1, unsigned int a2, __int64 a3))(g_ModuleBase + 0x131CB70);

	if (*(uint64_t *)(a1 + 16))
	{
		if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5) && *(DWORD *)&graphicsGlobals->__zz0[32] != 4)
		{
			*(DWORD *)&graphicsGlobals->__zz0[32] = 4;
			v6 = graphicsGlobals->dword_14304DEB0 & 0xFFFFFFFB;
			if (*(DWORD *)&graphicsGlobals->__zz0[36] != 4)
				v6 = graphicsGlobals->dword_14304DEB0 | 4;
			graphicsGlobals->dword_14304DEB0 = v6;
		}
		v7 = (a2 & 0xA) != 0;

		if (!(a2 & 0xA))
			((void(__fastcall *)(float))(g_ModuleBase + 0x12F87B0))(v3);
			//sub_1412F87B0(v3);

		// RenderBatches
		annotation->BeginEvent(L"RenderBatches");
		{
			DC_RenderDeferred(a1, a2, [](__int64 a1, unsigned int a2) {
				auto rbt1 = (__int64(__fastcall *)(__int64 a1, int a2, int a3, int a4, int a5))(g_ModuleBase + 0x12E3770);

				rbt1(a1, 1, 0x5C00002F, a2, -1);
			});

			DC_WaitDeferred();
		}
		annotation->EndEvent();

		// LowAniso
		annotation->BeginEvent(L"LowAniso");
		{
			__int64 v9; // rcx
			v9 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 184i64);
			if (v9)
			{
				if (*(BYTE *)(v9 + 38) & 1)
					RenderBatchTechnique2((__int64 *)v9, a2, v8);
				else
					RenderBatchTechnique1(a1, 1, 0x5C006074, a2, 9);
			}
		}
		annotation->EndEvent();

		// RenderGrass
		annotation->BeginEvent(L"RenderGrass");
		{
			RenderBatchTechnique1(a1, 0x5C000030, 0x5C00005C, a2, -1);
		}
		annotation->EndEvent();

		// RenderNoShadowGroup
		annotation->BeginEvent(L"RenderNoShadowGroup");
		{
			__int64 v11; // rcx
			v11 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 176i64);
			if (v11)
			{
				if (*(BYTE *)(v11 + 38) & 1)
					RenderBatchTechnique2((__int64 *)v11, a2, v10);
				else
					RenderBatchTechnique1(a1, 1, 0x5C006074, a2, 8);
			}
		}
		annotation->EndEvent();

		// RenderLODObjects
		annotation->BeginEvent(L"RenderLODObjects");
		{
			__int64 v12; // rcx
			v12 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 120i64);
			if (v12)
			{
				if (*(BYTE *)(v12 + 38) & 1)
					RenderBatchTechnique2((__int64 *)v12, a2, v10);
				else
					RenderBatchTechnique1(a1, 1, 0x5C006074, a2, 1);
			}
			if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5) && *(DWORD *)&graphicsGlobals->__zz0[32] != 3)
			{
				*(DWORD *)&graphicsGlobals->__zz0[32] = 3;
				v13 = graphicsGlobals->dword_14304DEB0 & 0xFFFFFFFB;
				if (*(DWORD *)&graphicsGlobals->__zz0[36] != 3)
					v13 = graphicsGlobals->dword_14304DEB0 | 4;
				graphicsGlobals->dword_14304DEB0 = v13;
			}
		}
		annotation->EndEvent();

		// RenderLODLand
		annotation->BeginEvent(L"RenderLODLand");
		{
			__int64 v14; // rcx
			v14 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 112i64);
			if (v14)
			{
				if (*(BYTE *)(v14 + 38) & 1)
					RenderBatchTechnique2((__int64 *)v14, a2, v10);
				else
					RenderBatchTechnique1(a1, 1, 0x5C006074, a2, 0);
			}

			if (!v7)
				((void(__fastcall *)())(g_ModuleBase + 0x12F8910))();
			//sub_1412F8910();
		}
		annotation->EndEvent();

		// RenderSky
		annotation->BeginEvent(L"RenderSky");
		{
			if (graphicsGlobals->__zz0[76] != 1)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x100u;
				graphicsGlobals->__zz0[76] = 1;
			}
			v15 = graphicsGlobals->float_14304DF68;
			if (graphicsGlobals->float_14304DF68 != 0.50196081)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x200u;
				graphicsGlobals->float_14304DF68 = 0.50196081;
			}
			RenderBatchTechnique1(a1, 0x5C00005D, 0x5C000064, a2, -1);
		}
		annotation->EndEvent();

		// RenderSkyClouds
		annotation->BeginEvent(L"RenderSkyClouds");
		{
			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 11)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 11;
			}
			v17 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 216i64);
			if (v17)
			{
				if (*(BYTE *)(v17 + 38) & 1)
					RenderBatchTechnique2((__int64 *)v17, a2, v16);
				else
					RenderBatchTechnique1(a1, 1, 0x5C006074, a2, 13);
			}
			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 1)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 1;
			}
		}
		annotation->EndEvent();

		if (!v7)
			((void(__fastcall *)(float))(g_ModuleBase + 0x12F87B0))(v15);
		//sub_1412F87B0(v15);

		if (*(DWORD *)&graphicsGlobals->__zz0[72] != 10)
		{
			graphicsGlobals->dword_14304DEB0 |= 0x80u;
			*(DWORD *)&graphicsGlobals->__zz0[72] = 10;
		}

		((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E2450))(a1, a2);
		//sub_1412E2450(a1, a2);

		// BlendedDecals
		annotation->BeginEvent(L"RenderSkyClouds");
		{
			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 11)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 11;
			}

			((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E25F0))(a1, a2);
			//sub_1412E25F0(a1, a2);
		}
		annotation->EndEvent();

		v18 = graphicsGlobals->dword_14304DEB0;
		if (*(DWORD *)&graphicsGlobals->__zz0[64])
		{
			v18 = graphicsGlobals->dword_14304DEB0 | 0x80;
			*(DWORD *)&graphicsGlobals->__zz0[64] = 0;
			graphicsGlobals->dword_14304DEB0 |= 0x80u;
		}
		if (*(DWORD *)&graphicsGlobals->__zz0[72] != 1)
		{
			*(DWORD *)&graphicsGlobals->__zz0[72] = 1;
			graphicsGlobals->dword_14304DEB0 = v18 | 0x80;
		}

		auto sub_14131F100 = (char(__fastcall *)(__int64 a1, unsigned int a2, unsigned int a3))(g_ModuleBase + 0x131F100);
		auto sub_140D744B0 = (__int64(__fastcall *)())(g_ModuleBase + 0xD744B0);
		auto sub_140D69E70 = (__int64(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0xD69E70);
		auto sub_140D69D30 = (__int64(__fastcall *)(float *a1, float a2, float a3, float a4, int a5))(g_ModuleBase + 0xD69D30);
		auto sub_1412FD120 = (signed __int64(__fastcall *)())(g_ModuleBase + 0x12FD120);
		auto sub_140D74350 = (__int64(__fastcall *)(__int64 a1, unsigned int a2, int a3, int a4, char a5))(g_ModuleBase + 0xD74350);
		auto sub_140D74370 = (void(__fastcall *)(__int64 a1, uint32_t a2, int a3, uint32_t a4))(g_ModuleBase + 0xD74370);
		auto sub_140D69990 = (void(__fastcall *)(__int64 a1, char a2))(g_ModuleBase + 0xD69990);
		auto sub_1412FADA0 = (__int64(__fastcall *)())(g_ModuleBase + 0x12FADA0);
		auto sub_140D69DA0 = (void(__fastcall *)(DWORD *a1))(g_ModuleBase + 0xD69DA0);

		float *flt_14304E490 = (float *)(g_ModuleBase + 0x304E490);

		if ((a2 & 0x80u) != 0 && sub_14131F100(*(uint64_t *)(a1 + 304), 0x5C000071u, 0x5C006071u))
		{
			v19 = sub_140D744B0();
			(*(void(__fastcall **)(ID3D11DeviceContext *, __int64, uint64_t))(*(uint64_t *)graphicsGlobals->m_DeviceContext
				+ 376i64))(
					graphicsGlobals->m_DeviceContext,
					*(__int64*)(g_ModuleBase + 0x3050870),
					*(uint64_t *)&flt_14304E490[38 * v19 + 2030]);
		}

		// RenderWaterStencil
		annotation->BeginEvent(L"RenderWaterStencil");
		{
			if (sub_14131F100(*(uint64_t *)(a1 + 304), 0x5C00006Du, 0x5C000070u))
			{
				sub_140D69E70((__int64)flt_14304E490, 2u);
				sub_140D69D30(flt_14304E490, 0.0, 0.0, 0.0, 0);
				v20 = sub_1412FD120();
				sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 0, v20, 3, 1);
				sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 1u, 7, 3, 1);
				sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 2u, -1, 3, 1);
				sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 3u, -1, 3, 1);
				sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 4u, -1, 3, 1);
				RenderBatchTechnique1(a1, 0x5C00006D, 0x5C000070, a2, -1);
				sub_140D69DA0((DWORD *)flt_14304E490);
				*(DWORD *)(*(uint64_t *)((*(uint64_t*)(g_ModuleBase + 0x31F5810)) + 496) + 44i64) = 1;
			}
		}
		annotation->EndEvent();

		if (a2 & 0x40)
		{
			sub_140D74370((__int64)(g_ModuleBase + 0x3051B20), 0xFFFFFFFF, 3, 0);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 1u, -1, 3, 1);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 2u, -1, 3, 1);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 3u, -1, 3, 1);
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 4u, -1, 3, 1);
			sub_140D69990((__int64)flt_14304E490, 1);
			sub_1412FADA0();
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 0, 1, 3, 1);
			v21 = sub_140D744B0();
			sub_140D74370((__int64)(g_ModuleBase + 0x3051B20), v21, 3, 0);
			if (*(DWORD *)&graphicsGlobals->__zz0[32] != 1)
			{
				*(DWORD *)&graphicsGlobals->__zz0[32] = 1;
				v22 = graphicsGlobals->dword_14304DEB0 & 0xFFFFFFFB;
				if (*(DWORD *)&graphicsGlobals->__zz0[36] != 1)
					v22 = graphicsGlobals->dword_14304DEB0 | 4;
				graphicsGlobals->dword_14304DEB0 = v22;
			}
		}

		if (!v7)
			((void(__fastcall *)())(g_ModuleBase + 0x12F8910))();
		//sub_1412F8910();
	}
}

#include <thread>
uint8_t *sub_1412E1600;
/*
__int64 __fastcall hk_sub_1412E1600(__int64 a1, unsigned int a2, float a3)
{
	if (annotation)
		annotation->BeginEvent(L"sub_1412E1600");

	D3D11_PRIMITIVE_TOPOLOGY topo;
	ID3D11RenderTargetView *rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	ID3D11DepthStencilView *dsv;

	memset(rtvs, 0, sizeof(rtvs));
	dsv = nullptr;

	ID3D11Buffer *psbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *vsbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *csbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11ShaderResourceView *srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *vsrvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11SamplerState *pssamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *vssamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *cssamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11UnorderedAccessView *views[D3D11_1_UAV_SLOT_COUNT];

	g_DeviceContext->CSGetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, views);
	g_DeviceContext->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pssamplers);
	g_DeviceContext->VSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, vssamplers);
	g_DeviceContext->CSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, cssamplers);
	g_DeviceContext->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_DeviceContext->VSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, vsrvs);
	g_DeviceContext->PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, psbuf);
	g_DeviceContext->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vsbuf);
	g_DeviceContext->CSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, csbuf);
	g_DeviceContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtvs, &dsv);
	g_DeviceContext->IAGetPrimitiveTopology(&topo);

	dc1->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
	dc1->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtvs, dsv);
	dc1->PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, psbuf);
	dc1->VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vsbuf);
	dc1->CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, csbuf);
	dc1->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	dc1->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, vsrvs);
	dc1->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pssamplers);
	dc1->VSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, vssamplers);
	dc1->CSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, cssamplers);
	dc1->CSSetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, views, nullptr);

	//SetThisThreadContext(dc1);

	ID3D11CommandList *list;
	__int64 res;
	std::thread t([&list, a1, a2, a3, &res]() {
//		SetThisThreadContext(dc1);
		res = ((decltype(&hk_sub_1412E1600))sub_1412E1600)(a1, a2, a3);
		dc1->FinishCommandList(FALSE, &list);
	});
	t.join();

	//__int64 res = ((decltype(&hk_sub_1412E1600))sub_1412E1600)(a1, a2, a3);

	//SetThisThreadContext(g_DeviceContext);

	g_DeviceContext->ExecuteCommandList(list, TRUE);
	list->Release();

	for (int i = 0; i < 8; i++)
		if (rtvs[i]) rtvs[i]->Release();

	if (dsv) dsv->Release();

	if (annotation)
		annotation->EndEvent();
	return res;
}
*/
uint8_t *sub_1412E1C10;
__int64 __fastcall hk_sub_1412E1C10(__int64 a1, unsigned int a2, float a3)
{
	if (annotation)
		annotation->BeginEvent(L"sub_1412E1C10");

	//SetThisThreadContext(dc2);

	__int64 res = ((decltype(&hk_sub_1412E1C10))sub_1412E1C10)(a1, a2, a3);

	//SetThisThreadContext(g_DeviceContext);

	//ID3D11CommandList *list;
	//dc2->FinishCommandList(TRUE, &list);
	//g_DeviceContext->ExecuteCommandList(list, TRUE);
	//list->Release();

	if (annotation)
		annotation->EndEvent();
	return res;
}

HRESULT WINAPI hk_CreateDXGIFactory(REFIID riid, void **ppFactory)
{
    ui::log::Add("Creating DXGI factory...\n");

    if (SUCCEEDED(ptrCreateDXGIFactory(__uuidof(IDXGIFactory3), ppFactory)))
        return S_OK;

    if (SUCCEEDED(ptrCreateDXGIFactory(__uuidof(IDXGIFactory2), ppFactory)))
        return S_OK;

    return ptrCreateDXGIFactory(__uuidof(IDXGIFactory), ppFactory);
}

bool hooked = false;
void hook();

#include <direct.h>
const char *NextShaderType;

uint8_t *BuildShaderBundle;

std::vector<void *> Doneshaders;

struct ShaderBufferData
{
	void *Buffer;
	size_t BufferLength;
};

std::unordered_map<void *, ShaderBufferData> m_ShaderBuffers;

void DumpVertexShader(BSVertexShader *Shader, const char *Type);
void DumpPixelShader(BSPixelShader *Shader, const char *Type, void *Buffer, size_t BufferLen);
void hk_BuildShaderBundle(__int64 shaderGroupObject, __int64 fileStream)
{
	hook();

	NextShaderType = (const char *)*(uintptr_t *)(shaderGroupObject + 136);
	((decltype(&hk_BuildShaderBundle))BuildShaderBundle)(shaderGroupObject, fileStream);

	return;
	uint32_t vsEntryCount = *(uint32_t *)(shaderGroupObject + 0x34);
	uint32_t psEntryCount = *(uint32_t *)(shaderGroupObject + 0x64);

	struct slink
	{
		__int64 shader;
		slink *next;
	};

	slink *vsEntries = *(slink **)(shaderGroupObject + 0x50);
	__int64 vsLastEntry = *(__int64 *)(shaderGroupObject + 0x40);

	slink *psEntries = *(slink **)(shaderGroupObject + 0x80);
	__int64 psLastEntry = *(__int64 *)(shaderGroupObject + 0x70);

	if (vsEntries)
	{
		for (uint32_t i = 0; i < vsEntryCount; i++)
		{
			slink *first = &vsEntries[i];

			if (!first)
				continue;

			while (true)
			{
				if (first->shader)
				{
					if (std::find(Doneshaders.begin(), Doneshaders.end(), (void *)first->shader) == Doneshaders.end())
					{
						DumpVertexShader((BSVertexShader *)first->shader, NextShaderType);
						Doneshaders.push_back((void *)first->shader);
					}
				}

				if (!first->next || first->next == (slink *)vsLastEntry)
					break;

				first = first->next;
			}
		}
	}

	if (psEntries)
	{
		for (uint32_t i = 0; i < psEntryCount; i++)
		{
			slink *first = &psEntries[i];

			if (!first)
				continue;

			while (true)
			{
				if (first->shader)
				{
					if (std::find(Doneshaders.begin(), Doneshaders.end(), (void *)first->shader) == Doneshaders.end())
					{
						auto p = (BSPixelShader *)first->shader;
						auto& data = m_ShaderBuffers[p->m_Shader];

						DumpPixelShader(p, NextShaderType, data.Buffer, data.BufferLength);
						Doneshaders.push_back((void *)p);
					}
				}

				if (!first->next || first->next == (slink *)psLastEntry)
					break;

				first = first->next;
			}
		}
	}

	for (auto& pair : m_ShaderBuffers)
	{
		if (pair.second.Buffer)
			free(pair.second.Buffer);

		pair.second.Buffer = nullptr;
	}

	NextShaderType = nullptr;
}

uint8_t *BuildComputeShaderBundle;
void hk_BuildComputeShaderBundle(__int64 shaderGroupObject, __int64 fileStream)
{
	hook();

	NextShaderType = (const char *)*(uintptr_t *)(shaderGroupObject + 24);
	((decltype(&hk_BuildComputeShaderBundle))BuildComputeShaderBundle)(shaderGroupObject, fileStream);
	NextShaderType = nullptr;
}

void DumpShader(const char *Prefix, int Index, const void *Bytecode, size_t BytecodeLength, ID3D11DeviceChild *Resource)
{
	if (!Bytecode || !NextShaderType || !Resource)
		return;

	char buffer[2048];
	int len = sprintf_s(buffer, "%s %s %d", Prefix, NextShaderType, Index);
	Resource->SetPrivateData(WKPDID_D3DDebugObjectName, len, buffer);

	sprintf_s(buffer, "C:\\Shaders\\%s\\", NextShaderType);
	_mkdir(buffer);

	sprintf_s(buffer, "C:\\Shaders\\%s\\%s_%d.hlsl", NextShaderType, Prefix, Index);

	FILE *w = fopen(buffer, "wb");
	if ( w)
	{
		fwrite(Bytecode, 1, BytecodeLength, w);
		fflush(w);
		fclose(w);
	}
}

decltype(&ID3D11Device::CreateVertexShader) CreateVertexShader;
HRESULT WINAPI hk_CreateVertexShader(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader)
{
	ProfileCounterInc("Vertex Shaders Created");

	return (This->*CreateVertexShader)(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
}

uint8_t *CreatePixelShader;
HRESULT WINAPI hk_CreatePixelShader(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader)
{
	ProfileCounterInc("Pixel Shaders Created");

	HRESULT hr = ((decltype(&hk_CreatePixelShader))CreatePixelShader)(This, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);

	if (SUCCEEDED(hr))
	{
		void *mem = malloc(BytecodeLength);
		memcpy(mem, pShaderBytecode, BytecodeLength);

		m_ShaderBuffers[(void *)*ppPixelShader] = { mem, BytecodeLength };
	}

	return hr;
}

decltype(&ID3D11Device::CreateComputeShader) CreateComputeShader;
HRESULT WINAPI hk_CreateComputeShader(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11ComputeShader **ppComputeShader)
{
	ProfileCounterInc("Compute Shaders Created");

	return (This->*CreateComputeShader)(pShaderBytecode, BytecodeLength, pClassLinkage, ppComputeShader);
}

void hook()
{
	*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1600, (PBYTE)&hk_sub_1412E1600);

	return;
	if (hooked)
		return;

	hooked = true;

	uintptr_t ptr = *(uintptr_t *)(&BSGraphics_TLSGlob[0x10]);
	//uintptr_t ptr = *(uintptr_t *)(g_ModuleBase + 0x304BF00);
	ID3D11Device *dev = *(ID3D11Device **)(ptr + 56);
	IDXGISwapChain *swap = *(IDXGISwapChain **)(ptr + 96);

	ID3D11Device2 *newDev = nullptr;

	if (FAILED((dev)->QueryInterface<ID3D11Device2>(&newDev)))
		__debugbreak();

	ID3D11DeviceContext1 *ctx;
	newDev->GetImmediateContext1(&ctx);
	ctx->QueryInterface<ID3D11DeviceContext2>(&g_DeviceContext);

	if (FAILED(g_DeviceContext->QueryInterface<ID3DUserDefinedAnnotation>(&annotation)))
		__debugbreak();

	newDev->CreateDeferredContext2(0, &dc1);
	newDev->CreateDeferredContext2(0, &dc2);

	*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)swap, &hk_IDXGISwapChain_Present, 8);

	//Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6BF00, (PBYTE)&sub_140D6BF00);

	*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1600, (PBYTE)&hk_sub_1412E1600);
	//*(PBYTE *)&sub_1412E1C10 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1C10, (PBYTE)&hk_sub_1412E1C10);

	//*(PBYTE *)&CreateVertexShader = Detours::X64::DetourClassVTable(*(PBYTE *)dev, &hk_CreateVertexShader, 12);
	//*(PBYTE *)&CreatePixelShader = Detours::X64::DetourClassVTable(*(PBYTE *)dev, &hk_CreatePixelShader, 15);
	//*(PBYTE *)&CreateComputeShader = Detours::X64::DetourClassVTable(*(PBYTE *)dev, &hk_CreateComputeShader, 18);
}

decltype(&ID3D11DeviceContext::Map) Map;
HRESULT WINAPI hk_ID3D11DeviceContext_Map(ID3D11DeviceContext *This, ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource)
{
	//ProfileCounterInc("ID3D11DeviceContext::Map");
	//ProfileTimer("ID3D11DeviceContext::Map Time");

	if (pResource)
	{
		D3D11_RESOURCE_DIMENSION dimension;
		pResource->GetType(&dimension);

		if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
		{
			D3D11_BUFFER_DESC desc;
			((ID3D11Buffer *)pResource)->GetDesc(&desc);

			if (desc.BindFlags == D3D11_BIND_CONSTANT_BUFFER)
				ProfileCounterAdd("Map Bytes", desc.ByteWidth);
		}
	}

	return (This->*Map)(pResource, Subresource, MapType, MapFlags, pMappedResource);
}

HRESULT WINAPI hk_D3D11CreateDeviceAndSwapChain(
    IDXGIAdapter *pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    const D3D_FEATURE_LEVEL *pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain **ppSwapChain,
    ID3D11Device **ppDevice,
    D3D_FEATURE_LEVEL *pFeatureLevel,
    ID3D11DeviceContext **ppImmediateContext)
{
    // From MSDN:
    //
    // If the Direct3D 11.1 runtime is present on the computer and pFeatureLevels is set to NULL,
    // this function won't create a D3D_FEATURE_LEVEL_11_1 device. To create a D3D_FEATURE_LEVEL_11_1
    // device, you must explicitly provide a D3D_FEATURE_LEVEL array that includes
    // D3D_FEATURE_LEVEL_11_1. If you provide a D3D_FEATURE_LEVEL array that contains
    // D3D_FEATURE_LEVEL_11_1 on a computer that doesn't have the Direct3D 11.1 runtime installed,
    // this function immediately fails with E_INVALIDARG.
    const D3D_FEATURE_LEVEL testFeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // Loop to get the highest available feature level; SkyrimSE originally uses D3D_FL_9_1.
	// SkyrimSE also uses a single render thread (sadface).
    D3D_FEATURE_LEVEL level;
    HRESULT hr;

    for (int i = 0; i < ARRAYSIZE(testFeatureLevels); i++)
    {
        hr = ptrD3D11CreateDeviceAndSwapChain(
            pAdapter,
            DriverType,
            Software,
            Flags | D3D11_CREATE_DEVICE_DEBUG,
            &testFeatureLevels[i],
            1,
            SDKVersion,
            pSwapChainDesc,
            ppSwapChain,
            ppDevice,
            &level,
            ppImmediateContext);

        // Exit if device was created
        if (SUCCEEDED(hr))
        {
            if (pFeatureLevel)
                *pFeatureLevel = level;

            break;
        }
    }

    if (FAILED(hr))
        return hr;

	// Force DirectX11.2 in case we use features later (11.3+ requires Win10 or higher)
	ID3D11Device2 *newDev = nullptr;
	ID3D11DeviceContext2 *newContext = nullptr;

	if (FAILED((*ppDevice)->QueryInterface<ID3D11Device2>(&newDev)))
		return E_FAIL;

	if (FAILED((*ppImmediateContext)->QueryInterface<ID3D11DeviceContext2>(&newContext)))
		return E_FAIL;

	if (FAILED(newContext->QueryInterface<ID3DUserDefinedAnnotation>(&annotation)))
		return E_FAIL;

	*ppDevice = newDev;
	*ppImmediateContext = newContext;

	g_DeviceContext = newContext;
	g_SwapChain = *ppSwapChain;

	// Create deferred contexts for later frame use
	newDev->CreateDeferredContext2(0, &dc1);
	newDev->CreateDeferredContext2(0, &dc2);

	OutputDebugStringA("Created everything\n");

    // Create ImGui globals
    ui::Initialize(pSwapChainDesc->OutputWindow, newDev, newContext);
    ui::log::Add("Created D3D11 device with feature level %X...\n", level);

    // Now hook the render function
	*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)*ppSwapChain, &hk_IDXGISwapChain_Present, 8);
	*(PBYTE *)&Map = Detours::X64::DetourClassVTable(*(PBYTE *)newContext, &hk_ID3D11DeviceContext_Map, 14);

	//*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1600, (PBYTE)&hk_sub_1412E1600);
	//*(PBYTE *)&sub_1412E1C10 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1C10, (PBYTE)&hk_sub_1412E1C10);

	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6BF00, (PBYTE)&sub_140D6BF00);

	//*(PBYTE *)&CreateVertexShader = Detours::X64::DetourClassVTable(*(PBYTE *)*ppDevice, &hk_CreateVertexShader, 12);
	//*(PBYTE *)&CreatePixelShader = Detours::X64::DetourClassVTable(*(PBYTE *)*ppDevice, &hk_CreatePixelShader, 15);
	//*(PBYTE *)&CreateComputeShader = Detours::X64::DetourClassVTable(*(PBYTE *)*ppDevice, &hk_CreateComputeShader, 18);

	//*(PBYTE *)&BuildShaderBundle = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x1336140), (PBYTE)&hk_BuildShaderBundle);
	//*(PBYTE *)&BuildComputeShaderBundle = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x133A450), (PBYTE)&hk_BuildComputeShaderBundle); todo fix offset
    return hr;
}

void CreateXbyakCodeBlock();
void CreateXbyakPatches();

void PatchD3D11()
{
    // Grab the original function pointers
    *(FARPROC *)&ptrCreateDXGIFactory = GetProcAddress(g_DllDXGI, "CreateDXGIFactory1");

    if (!ptrCreateDXGIFactory)
        *(FARPROC *)&ptrCreateDXGIFactory = GetProcAddress(g_DllDXGI, "CreateDXGIFactory");

    *(FARPROC *)&ptrD3D11CreateDeviceAndSwapChain = GetProcAddress(g_DllD3D11, "D3D11CreateDeviceAndSwapChain");

    if (!ptrCreateDXGIFactory || !ptrD3D11CreateDeviceAndSwapChain)
    {
        // Couldn't find one of the exports
        __debugbreak();
    }

	CreateXbyakCodeBlock();
	CreateXbyakPatches();

	*(PBYTE *)&BuildShaderBundle = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x1336140), (PBYTE)&hk_BuildShaderBundle);

    //PatchIAT(hk_CreateDXGIFactory, "dxgi.dll", "CreateDXGIFactory");
    //PatchIAT(hk_D3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain");
}
