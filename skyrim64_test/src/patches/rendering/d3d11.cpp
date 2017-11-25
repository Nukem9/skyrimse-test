#include "common.h"
#include "../../ui/ui.h"
#include "../TES/BSShaderManager.h"
#include "../TES/BSShaderRenderTargets.h"
#include "../TES/BSSpinLock.h"
#include "../TES/MemoryContextTracker.h"

// BSGraphicsRenderer

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
		//g_DeviceContext->DiscardResource(g_DepthStencilTextures[DEPTH_STENCIL_TARGET_SHADOWMAPS_ESRAM]);// Uses 2 4096x4096 slices and both are overwritten. Note: They clear both
																										// slices SEPARATELY (i.e clear s0, render s0, clear s1, render s1) which
																										// may cause dependency issues on slice 1. I hope this fixes it.

		const float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		//g_DeviceContext->ClearRenderTargetView(g_RenderTargets[RENDER_TARGET_RAW_WATER], black);
		//g_DeviceContext->ClearRenderTargetView(g_RenderTargets[RENDER_TARGET_MENUBG], black);		// Fixes flickering in the system menu, but background screen is black
	}
	annotation->EndEvent();

	return hr;
}

extern thread_local bool m_TestBuffer;
bool test = false;
ID3D11Buffer *testbuffer;

void *sub_140D6BF00(__int64 a1, int AllocationSize, uint32_t *AllocationOffset)
{
	auto globals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	if (true)
	{
		if (!test)
		{
			test = true;

			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = 100 * 1024;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			if (FAILED(globals->m_Device->CreateBuffer(&desc, nullptr, &testbuffer)))
				__debugbreak();
		}

		if (AllocationSize > 100 * 1024)
			__debugbreak();

		D3D11_MAPPED_SUBRESOURCE mapping;
		if (FAILED(globals->m_DeviceContext->Map(testbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapping)))
			__debugbreak();

		globals->m_DynamicBuffers[0] = testbuffer;
		globals->m_CurrentDynamicBufferIndex = 0;
		*AllocationOffset = 0;
		globals->m_FrameDataUsedSize = AllocationSize;

		// Invalidate the others for sanity checking
		globals->m_DynamicBuffers[1] = (ID3D11Buffer *)0x101010101;
		globals->m_DynamicBuffers[2] = (ID3D11Buffer *)0x101010101;
		globals->m_CommandListEndEvents[0] = (ID3D11Query *)0x101010101;
		globals->m_CommandListEndEvents[1] = (ID3D11Query *)0x101010101;
		globals->m_CommandListEndEvents[2] = (ID3D11Query *)0x101010101;

		return mapping.pData;
	}

#if 0
	uint32_t frameDataOffset = globals->m_FrameDataUsedSize;
	uint32_t frameBufferIndex = globals->m_CurrentDynamicBufferIndex;
	uint32_t newFrameDataSzie = globals->m_FrameDataUsedSize + AllocationSize;

	//
	// Check if this request would exceed the allocated buffer size for the currently executing command list. If it does,
	// we end the current query and move on to the next buffer.
	//
	if (newFrameDataSzie > 0x400000)
	{
		newFrameDataSzie = AllocationSize;
		frameDataOffset = 0;

		globals->m_EventQueryFinished[globals->m_CurrentDynamicBufferIndex] = false;
		globals->m_DeviceContext->End(globals->m_CommandListEndEvents[globals->m_CurrentDynamicBufferIndex]);

		frameBufferIndex++;

		if (frameBufferIndex >= 3)
			frameBufferIndex = 0;
	}
	
	//
	// This will **suspend execution** until the buffer we want is no longer in use. The query waits on a list of commands
	// using said buffer.
	//
	if (!globals->m_EventQueryFinished[frameBufferIndex])
	{
		ID3D11Query *query = globals->m_CommandListEndEvents[frameBufferIndex];
		BOOL data;

		HRESULT hr = globals->m_DeviceContext->GetData(query, &data, sizeof(data), 0);

		for (; FAILED(hr) || data == FALSE; hr = globals->m_DeviceContext->GetData(query, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH))
			Sleep(1);

		globals->m_EventQueryFinished[frameBufferIndex] = (data == TRUE);
	}
	
	D3D11_MAPPED_SUBRESOURCE resource;
	globals->m_DeviceContext->Map(globals->m_DynamicBuffers[frameBufferIndex], 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource);

	globals->m_CurrentDynamicBufferIndex = frameBufferIndex;
	*AllocationOffset = frameDataOffset;
	globals->m_FrameDataUsedSize = newFrameDataSzie;

	return (void *)((uintptr_t)resource.pData + frameDataOffset);
#endif
}

void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int));
void DC_WaitDeferred();

AutoPtr<uint64_t, 0x31F5490> qword_1431F5490;

AutoPtr<uint64_t, 0x32A8218> qword_1432A8218;
AutoPtr<uint32_t, 0x32A8214> dword_1432A8214;
AutoPtr<uint64_t, 0x34B5220> qword_1434B5220;

AutoPtr<BYTE, 0x31F54CD> byte_1431F54CD;

AutoPtr<DWORD, 0x1E32FDC> dword_141E32FDC;

bool sub_14131E8F0(__int64 a1, unsigned int a2, signed int *a3);
void sub_14131F910(__int64 a1, __int64 a2);
bool sub_14131E7B0(__int64 a1, DWORD *a2, signed int *a3, __int64 *a4);
void sub_14131D6E0(__int64 a1);
__int64 sub_141320350(__int64 a1, __int64 a2);
__int64 sub_1413203B0(__int64 a1, __int64 a2);
unsigned __int64 *sub_141320370(__int64 a1, unsigned __int64 *a2);
signed __int64 *sub_1413203D0(__int64 a1, signed __int64 *a2);
__int64 sub_14131ED70(uint64_t *a1, unsigned int a2, unsigned __int8 a3, unsigned int a4);

void operator_delete(__int64 a1, __int64 a2)
{
	((void(__fastcall *)(void *))(g_ModuleBase + 0x01026F0))((void *)a1);
}

void sub_1401C49C0(__int64 a1, __int64(__fastcall *a2)(uint64_t, __int64), __int64 a3)
{
	__int64 v3; // r9
	__int64(__fastcall *v4)(uint64_t, __int64); // r8
	__int64 v5; // rbx
	__int64 v6; // rax
	__int64 v7; // rsi
	__int64 v8; // rdi
	__int64(__fastcall *v9)(uint64_t, __int64); // [rsp+58h] [rbp+10h]
	__int64 v10; // [rsp+60h] [rbp+18h]

	v10 = a3;
	v9 = a2;
	v3 = a3;
	v4 = a2;
	v5 = a1;
	if (*(uint64_t *)(a1 + 8))
	{
		while (1)
		{
			v6 = *(uint64_t *)(v5 + 8);
			v7 = *(uint64_t *)(v6 + 8);
			*(uint64_t *)(v6 + 8) = 0i64;
			if (v4)
				break;
			v8 = *(uint64_t *)(v5 + 8);
			if (v8)
			{
				sub_1401C49C0(*(uint64_t *)(v5 + 8), 0i64, 0i64);
				operator_delete(v8, 16i64);
				goto LABEL_6;
			}
		LABEL_7:
			*(uint64_t *)(v5 + 8) = v7;
			if (!v7)
			{
				*(DWORD *)v5 = 0;
				return;
			}
		}
		v4(*(uint64_t *)(v5 + 8), v3);
	LABEL_6:
		v3 = v10;
		v4 = v9;
		goto LABEL_7;
	}
	*(DWORD *)a1 = 0;
}

void sub_1412ACFE0(__int64 a1)
{
	qword_1431F5490 = a1;
}

void sub_14131CAE0(uint64_t *a1)
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	a1[1] = 0i64;
	a1[2] = 0i64;

	if (*a1)
		sub_14131D6E0(*a1);

	*((WORD *)a1 + 18) = 0;
}

void sub_14131D6E0(__int64 a1)
{
	__int64 v1; // rbp
	unsigned int v4; // er15
	unsigned __int64 *v5; // rax
	unsigned __int64 v6; // rdi
	unsigned __int64 v7; // r14
	__int64 v8; // rdx
	signed __int64 v9; // rcx
	__int64 v10; // rax
	__int64 v11; // rsi
	__int64 v12; // rdi
	__int64 v13[2]; // [rsp+28h] [rbp-50h]
	char v14[16]; // [rsp+38h] [rbp-40h]

	v1 = a1;

	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	v4 = 0;
	v5 = (unsigned __int64 *)sub_141320350(a1 + 32, (__int64)&v13); // BSTScatterTable::begin()
	v6 = *v5;
	v7 = v5[1];
	v8 = *(uint64_t *)sub_1413203B0(v1 + 32, (__int64)&v14);// BSTScatterTable::end()
	while (v6 != v8)
	{
		// This code uses both BSTScatterTable and BSTArray
		// Also an inlined function. "Pass still has passes"
		if (v6)
			v4 = *(DWORD *)(v6 + 4);
		v9 = *(uint64_t *)(v1 + 8) + 48i64 * v4;
		*(uint64_t *)v9 = 0i64;
		*(uint64_t *)(v9 + 8) = 0i64;
		*(uint64_t *)(v9 + 16) = 0i64;
		*(uint64_t *)(v9 + 24) = 0i64;
		*(uint64_t *)(v9 + 32) = 0i64;
		*(DWORD *)(v9 + 40) = 0;
		do
			v6 += 16i64;
		while (v6 < v7 && !*(uint64_t *)(v6 + 8));
	}

	// This entire block is some inlined function
	if (*(uint64_t *)(v1 + 96))
	{
		do
		{
			v10 = *(uint64_t *)(v1 + 96);
			v11 = *(uint64_t *)(v10 + 8);
			*(uint64_t *)(v10 + 8) = 0i64;
			if (true /*sub_14131F910*/)
			{
				sub_14131F910(*(uint64_t *)(v1 + 96), g_ModuleBase + 0x34B5230);
			}
			else
			{
				v12 = *(uint64_t *)(v1 + 96);
				if (v12)
				{
					sub_1401C49C0(*(uint64_t *)(v1 + 96), 0i64, 0i64);
					operator_delete(v12, 16i64);
				}
			}
			*(uint64_t *)(v1 + 96) = v11;
		} while (v11);
	}

	*(DWORD *)(v1 + 88) = 0;
}

char sub_14131E700(__int64 a1, DWORD *a2, __int64 a3, __int64 a4)
{
	int v4; // er10
	unsigned int v5; // er11
	__int64 v6; // rbp
	__int64 v7; // rdi
	DWORD *v8; // rsi
	__int64 v9; // rbx
	__int64 v10; // rdx
	signed __int64 v11; // rax

	v4 = *a2;
	v5 = 0;
	v6 = a4;
	v7 = a3;
	v8 = a2;
	v9 = a1;
	if (!*a2)
		return sub_14131E7B0(a1, a2, (signed int *)a3, (__int64 *)a4);
	v10 = *(uint64_t *)(a1 + 72);
	if (v10)
	{
		v11 = v10 + 16i64 * (v4 & (unsigned int)(*(DWORD *)(a1 + 44) - 1));
		if (*(uint64_t *)(v11 + 8))
		{
			while (*(DWORD *)v11 != v4)
			{
				v11 = *(uint64_t *)(v11 + 8);
				if (v11 == *(uint64_t *)(a1 + 56))
					goto LABEL_8;
			}
			v5 = *(DWORD *)(v11 + 4);
		}
	}

LABEL_8:
	if (!sub_14131E8F0(a1, v5, (signed int *)a3))
	{
		a4 = v6;
		a3 = v7;
		a2 = v8;
		a1 = v9;
		return sub_14131E7B0(a1, a2, (signed int *)a3, (__int64 *)a4);
	}
	return 1;
}

bool sub_14131E7B0(__int64 a1, DWORD *a2, signed int *a3, __int64 *a4)
{
	signed int *v4; // r14
	DWORD *v5; // rsi
	__int64 v6; // rbx
	__int64 v7; // rax
	unsigned int v8; // ecx
	unsigned int v9; // ebp
	__int64 v10; // rdi
	__int64 v11; // r8
	signed __int64 v12; // rcx
	bool i; // zf

	v4 = a3;
	v5 = a2;
	v6 = a1;
	v7 = *a4;
	if (!*a4 || !*(uint64_t *)(v7 + 8) && !*(DWORD *)v7)
		return 0;
	v8 = *(DWORD *)v7;
	for (*a2 = *(DWORD *)v7; v8 < *(DWORD *)(v6 + 80); *a2 = *(DWORD *)v7)
	{
		v7 = *(uint64_t *)(v7 + 8);
		if (!v7)
			return 0;
		v8 = *(DWORD *)v7;
	}
	if (v8 > *(DWORD *)(v6 + 84))
		return 0;
	v9 = 0;
	if (*(BYTE *)(v6 + 108))
	{
		v10 = *(uint64_t *)(v6 + 96);
		if (v10)
		{
			*(uint64_t *)(v6 + 96) = *(uint64_t *)(v10 + 8);
			*(DWORD *)(v6 + 88) = *(DWORD *)v10;
			*(uint64_t *)(v10 + 8) = 0i64;
			if (true /*sub_14131F910*/)
			{
				sub_14131F910(v10, g_ModuleBase + 0x34B5230);
			}
			else
			{
				sub_1401C49C0(v10, 0i64, 0i64);
				operator_delete(v10, 16i64);
			}
		}
		else
		{
			*(DWORD *)(v6 + 88) = 0;
		}
	}
	else
	{
		*a4 = *(uint64_t *)(*a4 + 8);
	}
	v11 = *(uint64_t *)(v6 + 72);
	if (v11)
	{
		// BSTScatterTable
		v12 = v11 + 16i64 * (*v5 & (unsigned int)(*(DWORD *)(v6 + 44) - 1));
		for (i = *(uint64_t *)(v12 + 8) == 0i64; !i; i = v12 == *(uint64_t *)(v6 + 56))
		{
			if (*(DWORD *)v12 == *v5)
			{
				v9 = *(DWORD *)(v12 + 4);
				return sub_14131E8F0(v6, v9, v4);
			}
			v12 = *(uint64_t *)(v12 + 8);
		}
	}
	return sub_14131E8F0(v6, v9, v4);
}

bool sub_14131E8F0(__int64 a1, unsigned int a2, signed int *a3)
{
	signed int *v3; // r9
	signed int v4; // eax
	bool v5; // zf
	signed __int64 v6; // r8

	v3 = a3;
	if ((unsigned int)*a3 > 4)
		*a3 = 0;
	v4 = *a3;
	v5 = *a3 == 5;
	if (*a3 < 5)
	{
		v6 = *a3;
		do
		{
			// BSTArray
			if (*(uint64_t *)(*(uint64_t *)(a1 + 8) + 8 * (6i64 * a2 + v6)))
			{
				*v3 = v4;
				v4 = 5;
				v6 = 5i64;
			}
			++v4;
			++v6;
			v5 = v4 == 5;
		} while (v4 < 5);
	}
	if (v5)
		*v3 = 0;
	return v4 == 6;
}

#define LODWORD(x) (*(DWORD *)(&x))

char sub_14131E960(__int64 a1, unsigned int *a2, unsigned int *a3, __int64 a4, unsigned int a5)
{
	__int64 v6; // r11
	unsigned int v7; // ebx
	unsigned int *v8; // r15
	unsigned int *v9; // r13
	__int64 v10; // rsi
	signed __int64 v11; // r9
	unsigned __int8 v12; // di
	signed int v13; // eax
	bool v14; // r10
	bool v15; // zf
	__int64 v16; // r8
	char v17; // cl
	signed __int64 v18; // r9
	__int64 v19; // r14
	uint64_t *v20; // rbx
	__int64 v21; // rcx
	signed __int64 v22; // rdx
	__int64 v24; // [rsp+68h] [rbp+20h]

	auto& GraphicsGlobals = *(BSGraphicsRendererGlobals *)GetThreadedGlobals();

	v24 = a4;
	v6 = *(uint64_t *)(a1 + 72);
	v7 = 0;
	v8 = a3;
	v9 = a2;
	v10 = a1;

	// BSTScatterTree
	if (v6)
	{
		v11 = v6 + 16i64 * (*a2 & (*(DWORD *)(a1 + 44) - 1));
		if (*(uint64_t *)(v11 + 8))
		{
			while (*(DWORD *)v11 != *a2)
			{
				v11 = *(uint64_t *)(v11 + 8);
				if (v11 == *(uint64_t *)(a1 + 56))
					goto LABEL_7;
			}
			v7 = *(DWORD *)(v11 + 4);
		}
	}
LABEL_7:
	v12 = 0;
	v13 = *(DWORD *)&GraphicsGlobals.__zz0[52];
	v14 = (a5 & 0x108) != 0;
	v15 = *a3 == 0;
	v16 = GraphicsGlobals.dword_14304DEB0;
	if (v15)
	{
		if (!v14 && *(DWORD *)&GraphicsGlobals.__zz0[52] != 1)
		{
			v13 = 1;
			v16 = GraphicsGlobals.dword_14304DEB0 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 1;
			GraphicsGlobals.dword_14304DEB0 |= 0x20u;
		}
		v17 = GraphicsGlobals.__zz0[76];
		if (GraphicsGlobals.__zz0[76])
		{
			v17 = 0;
			LODWORD(v16) = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
		if (*(DWORD *)&GraphicsGlobals.__zz0[68])
		{
			v18 = 0i64;
			LODWORD(v16) = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
	}
	else
	{
		v17 = GraphicsGlobals.__zz0[76];
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
	}
	if (*v8 == 2)
	{
		if (!v14 && v13)
		{
			v13 = 0;
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17)
		{
			v17 = 0;
			LODWORD(v16) = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if ((DWORD)v18)
		{
			v18 = 0i64;
			LODWORD(v16) = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
	}
	if (*v8 == 3)
	{
		if (!v14 && v13)
		{
			v13 = 0;
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17 != 1)
		{
			v17 = 1;
			LODWORD(v16) = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v12 = 1;
		if (byte_1431F54CD && (DWORD)v18 != 1)
		{
			LODWORD(v16) = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
			v18 = 1i64;
		}
	}
	if (*v8 == 1)
	{
		if (!v14 && v13 != 1)
		{
			v13 = 1;
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17 != 1)
		{
			v17 = 1;
			LODWORD(v16) = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v12 = 1;
		if (byte_1431F54CD && (DWORD)v18 != 1)
		{
			LODWORD(v16) = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
			v18 = 1i64;
		}
	}
	if (*v8 == 4)
	{
		if (!v14 && v13 != 1)
		{
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17 != 1)
		{
			LODWORD(v16) = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v12 = 1;
		if ((DWORD)v18)
		{
			v18 = 0i64;
			LODWORD(v16) = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
	}
	v19 = v7;
	v20 = *(uint64_t **)(*(uint64_t *)(v10 + 8) + 8 * ((signed int)*v8 + 6i64 * v7));
	if (v20)
	{
		do
		{
			sub_14131ED70(v20, *v9, v12, a5);
			v20 = (uint64_t *)v20[6];
		} while (v20);
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
		v16 = GraphicsGlobals.dword_14304DEB0;
	}
	if (*(BYTE *)(v10 + 108))
	{
		v21 = *v8;
		v22 = *(uint64_t *)(v10 + 8) + 48 * v19;
		*(DWORD *)(v22 + 40) &= ~(1 << v21);
		*(uint64_t *)(v22 + 8 * v21) = 0i64;
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
		v16 = GraphicsGlobals.dword_14304DEB0;
	}
	if (qword_1432A8218)
	{
		(*(void(__fastcall **)(__int64, uint64_t, __int64, signed __int64))(*(uint64_t *)qword_1432A8218.get() + 24i64))(
			qword_1432A8218,
			(unsigned int)dword_1432A8214,
			v16,
			v18);
		LODWORD(v18) = *(DWORD *)&GraphicsGlobals.__zz0[68];
		LODWORD(v16) = GraphicsGlobals.dword_14304DEB0;
	}
	qword_1432A8218 = 0i64;
	dword_1432A8214 = 0;
	qword_1434B5220 = 0i64;
	if ((DWORD)v18)
	{
		*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
		GraphicsGlobals.dword_14304DEB0 = v16 | 0x80;
	}
	++*v8;
	return sub_14131E700(v10, (DWORD *)v9, (__int64)v8, v24);
}

char sub_14131ECE0(__int64 a1, DWORD *a2, __int64 a3, __int64 a4)
{
	__int64 v4; // r11
	__int64 v5; // r10
	unsigned int v6; // ebx
	signed __int64 v7; // rax
	signed __int64 v8; // rcx

	v4 = *(uint64_t *)(a1 + 72);
	v5 = a1;
	v6 = 0;
	if (v4)
	{
		// BSTScatterTable
		v7 = v4 + 16i64 * (*a2 & (unsigned int)(*(DWORD *)(a1 + 44) - 1));
		if (*(uint64_t *)(v7 + 8))
		{
			while (*(DWORD *)v7 != *a2)
			{
				v7 = *(uint64_t *)(v7 + 8);
				if (v7 == *(uint64_t *)(a1 + 56))
					goto LABEL_7;
			}
			v6 = *(DWORD *)(v7 + 4);
		}
	}
LABEL_7:
	if (*(BYTE *)(a1 + 108))
	{
		v8 = *(uint64_t *)(a1 + 8) + 48i64 * v6;
		*(uint64_t *)v8 = 0i64;
		*(uint64_t *)(v8 + 8) = 0i64;
		*(uint64_t *)(v8 + 16) = 0i64;
		*(uint64_t *)(v8 + 24) = 0i64;
		*(uint64_t *)(v8 + 32) = 0i64;
		*(DWORD *)(v8 + 40) = 0;
	}
	return sub_14131E700(v5, a2, a3, a4);
}

__int64 sub_14131ED70(uint64_t *a1, unsigned int a2, unsigned __int8 a3, unsigned int a4)
{
	unsigned int v5; // ebp
	__int64 v6; // r14
	unsigned __int8 v7; // r15
	__int64 v8; // rsi
	uint64_t *v9; // rbx
	__int64 result; // rax
	__int64 v11; // rdi
	bool v12; // zf

	auto sub_14131EFF0 = (__int64(__fastcall *)(unsigned int a1, __int64 a2))(g_ModuleBase + 0x131EFF0);
	auto sub_14131F450 = (__int64(__fastcall *)(__int64 *a1, __int64 a2, unsigned int a3))(g_ModuleBase + 0x131F450);
	auto sub_14131F1F0 = (__int64(__fastcall *)(__int64 *a1, char a2, unsigned int a3))(g_ModuleBase + 0x131F1F0);
	auto sub_14131F2A0 = (__int64(__fastcall *)(__int64 a1, unsigned __int8 a2, unsigned int a3))(g_ModuleBase + 0x131F2A0);

	v5 = a4;
	v6 = *a1;
	v7 = a3;
	v8 = a1[2];
	v9 = a1;
	if (dword_1432A8214 == a2 && a2 != 0x5C006076 && v6 == qword_1432A8218
		|| (dword_141E32FDC = a2, result = sub_14131EFF0(a2, v6), (BYTE)result))
	{
		v11 = v9[1];
		if (v11)
			v11 = *(uint64_t *)(v11 + 120);
		if (v11 != qword_1434B5220)
		{
			if (v11)
				(*(void(__fastcall **)(__int64, __int64))(*(uint64_t *)v6 + 32i64))(v6, v11);
			qword_1434B5220 = v11;
		}
		v12 = *(uint64_t *)(v8 + 304) == 0i64;
		*(BYTE *)(v8 + 264) = *((BYTE *)v9 + 30);
		if (v12)
		{
			if (*(BYTE *)(v8 + 265) & 8)
				result = sub_14131F450((__int64 *)v9, v7, v5);
			else
				result = sub_14131F1F0((__int64 *)v9, v7, v5);
		}
		else
		{
			result = sub_14131F2A0((__int64)v9, v7, v5);
		}
	}
	return result;
}

void sub_14131F090()
{
	if (qword_1432A8218)
		(*(void(__fastcall **)(__int64, uint64_t))(*(uint64_t *)qword_1432A8218.get() + 24i64))(
			qword_1432A8218,
			(unsigned int)dword_1432A8214);

	qword_1432A8218 = 0i64;
	dword_1432A8214 = 0;
	qword_1434B5220 = 0i64;
}

void sub_14131F910(__int64 a1, __int64 a2)
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	if (a2)
	{
		BSSpinLock& lock = *(BSSpinLock *)(a2 + 8);

		lock.Acquire();
		*(uint64_t *)(a1 + 8) = *(uint64_t *)a2;
		*(uint64_t *)a2 = a1;
		lock.Release();
	}
	else if (a1)
	{
		sub_1401C49C0(a1, 0i64, 0i64);
		operator_delete(a1, (unsigned int)(a2 + 16));
	}
}

void sub_14131F9F0(__int64 *a1, unsigned int a2, __int64 a3)
{
	unsigned int v4; // ebp
	__int64 *v5; // rdi
	__int64 result; // rax
	__int64 v7; // rbx
	unsigned int v8; // edx
	__int64 v9; // rcx
	bool v10; // r8

	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	v4 = a2;
	v5 = a1;
	if (*a1)
	{
		if (qword_1432A8218)
			result = (*(__int64(__fastcall **)(__int64, uint64_t, __int64))(*(uint64_t *)qword_1432A8218.get() + 24i64))(
				qword_1432A8218,
				(unsigned int)dword_1432A8214,
				a3);
		qword_1432A8218 = 0i64;
		dword_1432A8214 = 0;
		qword_1434B5220 = 0i64;
		v7 = *v5;
		if (*v5)
		{
			while (1)
			{
				v8 = *(DWORD *)(v7 + 24);
				if (*(uint64_t *)(v7 + 16))
					break;
			LABEL_18:
				v7 = *(uint64_t *)(v7 + 48);
				if (!v7)
					goto LABEL_19;
			}
			if ((v4 & 0x108) == 0)
			{
				if (*(uint64_t *)(*(uint64_t *)(v7 + 8) + 56i64) & 0x1000000000i64)
				{
					if (!*(DWORD *)&GraphicsGlobals->__zz0[52])
						goto LABEL_13;
					*(DWORD *)&GraphicsGlobals->__zz0[52] = 0;
				}
				else
				{
					if (*(DWORD *)&GraphicsGlobals->__zz0[52] == 1)
						goto LABEL_13;
					*(DWORD *)&GraphicsGlobals->__zz0[52] = 1;
				}
				GraphicsGlobals->dword_14304DEB0 |= 0x20u;
			}
		LABEL_13:
			v9 = *(uint64_t *)(*(uint64_t *)(v7 + 16) + 288i64);
			v10 = v9 && (*(WORD *)(v9 + 48) >> 9) & 1;
			result = sub_14131ED70((uint64_t *)v7, v8, v10, v4);
			goto LABEL_18;
		}
	LABEL_19:
		if (!(v4 & 0x108) && *(DWORD *)&GraphicsGlobals->__zz0[52] != 1)
		{
			GraphicsGlobals->dword_14304DEB0 |= 0x20u;
			*(DWORD *)&GraphicsGlobals->__zz0[52] = 1;
		}
		*v5 = 0i64;
		v5[1] = 0i64;
		if (qword_1432A8218)
			result = (*(__int64(__fastcall **)(__int64, uint64_t, __int64))(*(uint64_t *)qword_1432A8218.get() + 24i64))(
				qword_1432A8218,
				(unsigned int)dword_1432A8214,
				a3);
		qword_1432A8218 = 0i64;
		dword_1432A8214 = 0;
		qword_1434B5220 = 0i64;
	}
}

__int64 sub_141320350(__int64 a1, __int64 a2)
{
	__int64 v2; // rbx

	v2 = a2;
	sub_141320370(a1 + 8, (unsigned __int64 *)a2);
	return v2;
}

unsigned __int64 *sub_141320370(__int64 a1, unsigned __int64 *a2)
{
	__int64 v2; // r9
	unsigned __int64 v3; // rax
	unsigned __int64 v4; // r8
	unsigned __int64 *result; // rax

	v2 = *(uint64_t *)(a1 + 32);
	v3 = 0i64;
	v4 = 0i64;
	if (v2)
	{
		v3 = *(uint64_t *)(a1 + 32);
		v4 = v2 + 16i64 * *(unsigned int *)(a1 + 4);
		while (v3 < v4 && !*(uint64_t *)(v3 + 8))
			v3 += 16i64;
	}
	*a2 = v3;
	result = a2;
	a2[1] = v4;
	return result;
}

__int64 sub_1413203B0(__int64 a1, __int64 a2)
{
	__int64 v2; // rbx

	v2 = a2;
	sub_1413203D0(a1 + 8, (signed __int64 *)a2);
	return v2;
}

signed __int64 *sub_1413203D0(__int64 a1, signed __int64 *a2)
{
	__int64 v2; // r8
	signed __int64 v3; // rax

	v2 = *(uint64_t *)(a1 + 32);
	if (v2)
	{
		v3 = v2 + 16i64 * *(unsigned int *)(a1 + 4);
		*a2 = v3;
		a2[1] = v3;
	}
	else
	{
		*a2 = 0i64;
		a2[1] = 0i64;
	}
	return a2;
}

void RenderBatchTechnique1(__int64 a1, int a2, int a3, int a4, int a5)
{
	// a1 = BSShaderAccumulator

	__int64 *v10; // r14
	__int64 v11; // rsi
	char v12; // al
	__int64 v14; // [rsp+60h] [rbp+8h]

	sub_1412ACFE0(a1);

	if (a5 <= -1)
	{
		v11 = *(uint64_t *)(a1 + 304);
		v10 = 0i64;
	}
	else
	{
		v10 = *(__int64 **)(*(uint64_t *)(a1 + 304) + 8i64 * a5 + 112);
		v11 = *v10;
	}

	*(DWORD *)(a1 + 312) = 0;

	if (v11)
	{
		*(DWORD *)(v11 + 80) = a2;
		*(DWORD *)(v11 + 84) = a3;
		*(DWORD *)(a1 + 316) = 0;
		v14 = v11 + 88;
		*(BYTE *)(a1 + 320) = sub_14131E700(v11, (DWORD *)(a1 + 312), a1 + 316, (__int64)&v14);
	}
	else
	{
		*(BYTE *)(a1 + 320) = 0;
	}

	if (*(BYTE *)(a1 + 320))
	{
		do
		{
			if ((unsigned int)(*(DWORD *)(a1 + 312) - 0x5C000058) <= 3 && (*(BYTE *)(a1 + 296) || *(BYTE *)(a1 + 297)))
				v12 = sub_14131ECE0(v11, (DWORD *)(a1 + 312), a1 + 316, (__int64)&v14);
			else
				v12 = sub_14131E960(v11, (unsigned int *)(a1 + 312), (unsigned int *)(a1 + 316), (__int64)&v14, a4);

			*(BYTE *)(a1 + 320) = v12;
		} while (v12);
	}

	if (v10)
		sub_14131CAE0((uint64_t *)v10);

	sub_14131F090();
}

void RenderBatchTechnique2(__int64 a1, unsigned int a2, __int64 a3)
{
	// a1 = BSShaderAccumulator

	__int64 v4; // r14
	__int64 v5; // rcx
	__int64 v6; // rbx
	__int64 v7; // rax
	__int64 v8; // rsi
	__int64 v9; // rdi

	v4 = a1;
	if (*(BYTE *)(a1 + 38) & 1)
	{
		sub_14131F9F0((__int64 *)(a1 + 8), a2, a3);
	}
	else
	{
		v5 = *(uint64_t *)a1;
		if (!v5)
			goto LABEL_14;
		(*(void(__fastcall **)(__int64, signed __int64, signed __int64, uint64_t, signed __int64))(*(uint64_t *)v5 + 24i64))(
			v5,
			1i64,
			0x5C006074i64,
			a2,
			-2i64);
	}
	v6 = *(uint64_t *)v4;
	if (*(uint64_t *)v4)
	{
		if (!*(BYTE *)(v6 + 108))
			return;
		if (*(uint64_t *)(v6 + 96))
		{
			do
			{
				v7 = *(uint64_t *)(v6 + 96);
				v8 = *(uint64_t *)(v7 + 8);
				*(uint64_t *)(v7 + 8) = 0i64;
				if (true /*sub_14131F910*/)
				{
					sub_14131F910(*(uint64_t *)(v6 + 96), g_ModuleBase + 0x34B5230);
				}
				else
				{
					v9 = *(uint64_t *)(v6 + 96);
					if (v9)
					{
						sub_1401C49C0(*(uint64_t *)(v6 + 96), 0i64, 0i64);
						operator_delete(v9, 16i64);
					}
				}
				*(uint64_t *)(v6 + 96) = v8;
			} while (v8);
		}
		*(DWORD *)(v6 + 88) = 0;
	}
LABEL_14:
	*(WORD *)(v4 + 36) = 0;
}

void CommitShaderChanges(bool Unknown)
{
	auto renderer = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t v1; // edx
	uint64_t *v3; // r8
	unsigned int v4; // edi
	__int64 v5; // rdx
	uint32_t *v6; // rbx
	ID3D11RenderTargetView **v7; // rsi
	__int64 v8; // rax
	ID3D11RenderTargetView *v9; // rdx
	int v10; // edx
	__int64 v11; // rbx
	signed __int64 v12; // rcx
	float v14; // xmm0_4
	float v15; // xmm0_4
	unsigned __int64 v17; // rbx
	uint64_t v18; // rcx
	__int64 v19; // rdi
	int v20; // ebx
	int v21; // edx
	uint32_t v22; // eax
	int *i; // [rsp+28h] [rbp-80h]
	float *v35; // [rsp+30h] [rbp-78h]
	ID3D11RenderTargetView *v34[8];
	int v37; // [rsp+B8h] [rbp+10h]
	int v38; // [rsp+C0h] [rbp+18h]
	__int64 v39; // [rsp+C8h] [rbp+20h]

	v1 = renderer->dword_14304DEB0;
	if (renderer->dword_14304DEB0)
	{
		if (renderer->dword_14304DEB0 & 1)
		{
			v3 = (uint64_t *)renderer->qword_14304BF00;
			if (renderer->iRenderTargetIndexes[0][0] == -1)
			{
				v6 = renderer->iRenderTargetIndexes[1];
				v7 = v34;
				v4 = 0;
				do
				{
					v8 = (signed int)*(v6 - 12);
					if ((DWORD)v8 == -1)
						break;
					v9 = (ID3D11RenderTargetView *)*((uint64_t *)v3 + 6 * v8 + 0x14B);
					*v7 = v9;
					if (!*v6)
					{
						renderer->m_DeviceContext->ClearRenderTargetView(
							v9,
							(const FLOAT *)v3 + 2522);          // ClearRenderTargetView

						v3 = (uint64_t *)renderer->qword_14304BF00;
						*v6 = 4;
					}
					++v4;
					++v7;
					++v6;
				} while (v4 < 8);
			}
			else
			{
				v4 = 1;
				v5 = *((uint64_t *)renderer->qword_14304BF00
					+ (signed int)renderer->iRenderTargetIndexes[0][1]
					+ 8i64 * (signed int)renderer->iRenderTargetIndexes[0][0]
					+ 1242);
				v34[0] = *((ID3D11RenderTargetView **)renderer->qword_14304BF00
					+ (signed int)renderer->iRenderTargetIndexes[0][1]
					+ 8i64 * (signed int)renderer->iRenderTargetIndexes[0][0]
					+ 1242);
				if (!*(DWORD *)&renderer->__zz0[4])
				{
					renderer->m_DeviceContext->ClearRenderTargetView((ID3D11RenderTargetView *)v5, (float *)(char *)renderer->qword_14304BF00 + 10088);

					v3 = (uint64_t *)renderer->qword_14304BF00;
					*(DWORD *)&renderer->__zz0[4] = 4;
				}
			}
			v10 = *(DWORD *)renderer->__zz0;
			if (*(DWORD *)renderer->__zz0 <= 2u || *(DWORD *)renderer->__zz0 == 6)
			{
				*((BYTE *)v3 + 34) = 0;
				v10 = *(DWORD *)renderer->__zz0;
				v3 = (uint64_t *)renderer->qword_14304BF00;
			}
			if (renderer->rshadowState_iDepthStencil == -1)
			{
				v11 = 0i64;
			LABEL_28:
				renderer->m_DeviceContext->OMSetRenderTargets(// OMSetRenderTargets
					v4,
					v34,
					(ID3D11DepthStencilView *)v11);

				v1 = renderer->dword_14304DEB0;
				goto LABEL_29;
			}
			v12 = renderer->rshadowState_iDepthStencilSlice
				+ 19i64 * (signed int)renderer->rshadowState_iDepthStencil;
			if (*((BYTE *)v3 + 34))
				v11 = v3[v12 + 1022];
			else
				v11 = v3[v12 + 1014];
			if (!v11)
				goto LABEL_28;

			uint32_t clearFlags;
			if (v10 && v10 != 6)
			{
				if (v10 == 2)
				{
					clearFlags = 2i64;
				}
				else
				{
					if (v10 != 1)
						goto LABEL_28;
					clearFlags = 1i64;
				}
			}
			else
			{
				clearFlags = 3i64;
			}

			renderer->m_DeviceContext->ClearDepthStencilView((ID3D11DepthStencilView *)v11, clearFlags, 1.0f, 0);

			*(DWORD *)renderer->__zz0 = 4;
			goto LABEL_28;
		}

	LABEL_29:
		// OMSetDepthStencilState
		if (v1 & 0xC)
		{
			renderer->m_DeviceContext->OMSetDepthStencilState(
				(ID3D11DepthStencilState *)renderer->qword_14304C1B0[0][*(signed int *)&renderer->__zz0[40] + 40i64 * *(signed int *)&renderer->__zz0[32]],
				*(UINT *)&renderer->__zz0[44]);

			v1 = renderer->dword_14304DEB0;
		}

		if (v1 & 0x1070)
		{
			void *wtf = renderer->qword_14304C930[0][0][0][*(signed int *)&renderer->__zz0[60]
				+ 2
				* (*(signed int *)&renderer->__zz0[56]
					+ 12
					* (*(signed int *)&renderer->__zz0[52]
						+ 3i64 * *(signed int *)&renderer->__zz0[48]))];

			renderer->m_DeviceContext->RSSetState((ID3D11RasterizerState *)wtf);

			v1 = renderer->dword_14304DEB0;
			if (renderer->dword_14304DEB0 & 0x40)
			{
				if (*(float *)&renderer->__zz0[24] != *(float *)&renderer->__zz2[640]
					|| (v14 = *(float *)&renderer->__zz0[28],
						*(float *)&renderer->__zz0[28] != *(float *)&renderer->__zz2[644]))
				{
					v14 = *(float *)&renderer->__zz2[644];
					*(DWORD *)&renderer->__zz0[24] = *(DWORD *)&renderer->__zz2[640];
					v1 = renderer->dword_14304DEB0 | 2;
					*(DWORD *)&renderer->__zz0[28] = *(DWORD *)&renderer->__zz2[644];
					renderer->dword_14304DEB0 |= 2u;
				}
				if (*(DWORD *)&renderer->__zz0[56])
				{
					v15 = v14 - renderer->m_UnknownFloats1[0][*(signed int *)&renderer->__zz0[56]];
					v1 |= 2u;
					renderer->dword_14304DEB0 = v1;
					*(float *)&renderer->__zz0[28] = v15;
				}
			}
		}

		// RSSetViewports
		if (v1 & 2)
		{
			renderer->m_DeviceContext->RSSetViewports(1, (D3D11_VIEWPORT *)&renderer->__zz0[8]);

			v1 = renderer->dword_14304DEB0;
		}

		// OMSetBlendState
		if ((v1 & 0x80u) != 0)
		{
			float *blendFactor = (float *)(g_ModuleBase + 0x1E2C168);

			void *wtf = renderer->qword_14304CDB0[0][0][0][*(unsigned int *)&renderer->__zz2[656]
				+ 2
				* (*(signed int *)&renderer->__zz0[72]
					+ 13
					* (*(signed int *)&renderer->__zz0[68]
						+ 2i64 * *(signed int *)&renderer->__zz0[64]))];

			renderer->m_DeviceContext->OMSetBlendState((ID3D11BlendState *)wtf, blendFactor, 0xFFFFFFFF);

			v1 = renderer->dword_14304DEB0;
		}

		if (v1 & 0x300)
		{
			D3D11_MAPPED_SUBRESOURCE resource;
			renderer->m_DeviceContext->Map(renderer->m_TempConstantBuffer1, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

			if (renderer->__zz0[76])
				*(float *)resource.pData = renderer->float_14304DF68;
			else
				*(float *)resource.pData = 0.0f;

			renderer->m_DeviceContext->Unmap(renderer->m_TempConstantBuffer1, 0);

			v1 = renderer->dword_14304DEB0;
		}

		// Shader input layout creation + updates
		if (!Unknown && _bittest((const LONG *)&v1, 0xAu))
		{
			uint32_t& dword_141E2C144 = *(uint32_t *)(g_ModuleBase + 0x1E2C144);
			uint64_t& qword_141E2C160 = *(uint64_t *)(g_ModuleBase + 0x1E2C160);

			uint64_t *off_141E2C150 = *(uint64_t **)(g_ModuleBase + 0x1E2C150);

			auto sub_140C06080 = (__int64(__fastcall *)(DWORD *a1, unsigned __int64 a2))(g_ModuleBase + 0xC06080);
			auto sub_140D705F0 = (__int64(__fastcall *)(unsigned __int64 a1))(g_ModuleBase + 0xD705F0);
			auto sub_140D72740 = (char(__fastcall *)(__int64 a1, __int64 a2, int a3, __int64 *a4, int64_t *a5))(g_ModuleBase + 0xD72740);
			auto sub_140D735D0 = (void(__fastcall *)(__int64 a1))(g_ModuleBase + 0xD735D0);

			v17 = *(uint64_t *)renderer->__zz2 & *(uint64_t *)(*(uint64_t *)&renderer->__zz2[8] + 72i64);
			v35 = (float *)(*(uint64_t *)renderer->__zz2 & *(uint64_t *)(*(uint64_t *)&renderer->__zz2[8] + 72i64));
			sub_140C06080((DWORD *)&v37, (unsigned __int64)v35);
			if (qword_141E2C160
				&& (v18 = qword_141E2C160 + 24i64 * (v37 & (unsigned int)(dword_141E2C144 - 1)),
					*(uint64_t *)(qword_141E2C160 + 24i64 * (v37 & (unsigned int)(dword_141E2C144 - 1)) + 16)))
			{
				while (*(uint64_t *)v18 != v17)
				{
					v18 = *(uint64_t *)(v18 + 16);
					if ((void *)v18 == off_141E2C150)
						goto LABEL_53;
				}
				v19 = *(uint64_t *)(v18 + 8);
			}
			else
			{
			LABEL_53:
				v39 = sub_140D705F0(v17);                 // IACreateInputLayout
				v19 = v39;
				if (v39 || v17 != 0x300000000407i64)
				{
					sub_140C06080((DWORD *)&v38, v17);
					v20 = v38;
					for (i = &v37; !sub_140D72740((__int64)(g_ModuleBase + 0x1E2C140), qword_141E2C160, v20, (__int64 *)&v35, &v39); i = &v37)
						sub_140D735D0((__int64)(g_ModuleBase + 0x1E2C140));
				}
			}

			renderer->m_DeviceContext->IASetInputLayout((ID3D11InputLayout *)v19);
			v1 = renderer->dword_14304DEB0;
		}

		// IASetPrimitiveTopology
		if (_bittest((const LONG *)&v1, 0xBu))
		{
			renderer->m_DeviceContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)*(unsigned int *)&renderer->__zz2[24]);
			v1 = renderer->dword_14304DEB0;
		}

		v21 = v1 & 0x400;
		v22 = 0;

		if (Unknown)
			v22 = v21;

		renderer->dword_14304DEB0 = v22;
	}

	//
	// Resource/state setting code. It's been modified to take 1 of 2 paths for each type:
	//
	// 1: modifiedBits == 0 { Do nothing }
	// 2: modifiedBits > 0  { Build minimal state change [X entries] before submitting it to DX }
	//
#define for_each_bit(itr, bits) for (unsigned long itr; _BitScanForward(&itr, bits); bits &= ~(1 << itr))

	// Compute shader unordered access views (UAVs)
	if (uint32_t bits = renderer->m_CSUAVModifiedBits; bits != 0)
	{
		for_each_bit(i, bits)
			renderer->m_DeviceContext->CSSetUnorderedAccessViews(i, 1, &renderer->m_CSUAVResources[i], nullptr);

		renderer->m_CSUAVModifiedBits = 0;
	}

	// Pixel shader samplers
	if (uint32_t bits = renderer->m_PSSamplerModifiedBits; bits != 0)
	{
		for_each_bit(i, bits)
		{
			char *ptr = (char *)renderer->qword_14304D910 + 8 * ((signed int)renderer->m_PSSamplerSetting2[i] + 5i64 * (signed int)renderer->m_PSSamplerSetting1[i]);

			if (ptr != (char *)&renderer->qword_14304D910[renderer->m_PSSamplerSetting1[i]][renderer->m_PSSamplerSetting2[i]])
				__debugbreak();

			if (i >= 16)
				__debugbreak();

			renderer->m_DeviceContext->PSSetSamplers(i, 1, &renderer->qword_14304D910[renderer->m_PSSamplerSetting1[i]][renderer->m_PSSamplerSetting2[i]]);
		}

		renderer->m_PSSamplerModifiedBits = 0;
	}

	// Pixel shader resources
	if (uint32_t bits = renderer->m_PSResourceModifiedBits; bits != 0)
	{
		for_each_bit(i, bits)
			renderer->m_DeviceContext->PSSetShaderResources(i, 1, &renderer->m_PSResources[i]);

		renderer->m_PSResourceModifiedBits = 0;
	}

	// Compute shader samplers
	if (uint32_t bits = renderer->m_CSSamplerModifiedBits; bits != 0)
	{
		for_each_bit(i, bits)
		{
			char *ptr = (char *)&renderer->qword_14304D910 + 8 * ((signed int)renderer->m_CSSamplerSetting2[i] + 5i64 * (signed int)renderer->m_CSSamplerSetting1[i]);

			if (ptr != (char *)&renderer->qword_14304D910[renderer->m_CSSamplerSetting1[i]][renderer->m_CSSamplerSetting2[i]])
				__debugbreak();

			if (i >= 16)
				__debugbreak();

			renderer->m_DeviceContext->CSSetSamplers(i, 1, &renderer->qword_14304D910[renderer->m_CSSamplerSetting1[i]][renderer->m_CSSamplerSetting2[i]]);
		}

		renderer->m_CSSamplerModifiedBits = 0;
	}

	// Compute shader resources
	if (uint32_t bits = renderer->m_CSResourceModifiedBits; bits != 0)
	{
		for_each_bit(i, bits)
			renderer->m_DeviceContext->CSSetShaderResources(i, 1, &renderer->m_CSResources[i]);

		renderer->m_CSResourceModifiedBits = 0;
	}

#undef for_each_bit
}

void __fastcall hk_sub_1412E1600(__int64 a1, unsigned int a2, float a3)
{
	// a1 = BSShaderAccumulator

	uint32_t v6; // ecx
	bool v7; // si
	uint32_t v13; // ecx
	uint32_t v18; // eax
	int v20; // eax

	auto graphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	auto RenderBatchTechnique1 = (__int64(__fastcall *)(__int64 a1, int a2, int a3, int a4, int a5))(g_ModuleBase + 0x12E3770);
	auto RenderBatchTechnique2 = (void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x131CB70);

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
			((void(__fastcall *)())(g_ModuleBase + 0x12F87B0))();

		// RenderBatches
		annotation->BeginEvent(L"RenderBatches");
		{
			RenderBatchTechnique1(a1, 1, 0x5C00002F, a2, -1);
		}
		annotation->EndEvent();

		// LowAniso
		annotation->BeginEvent(L"LowAniso");
		{
			__int64 v9 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 184i64);

			if (v9)
			{
				if (*(BYTE *)(v9 + 38) & 1)
					RenderBatchTechnique2(v9, a2);
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
			__int64 v11 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 176i64);

			if (v11)
			{
				if (*(BYTE *)(v11 + 38) & 1)
					RenderBatchTechnique2(v11, a2);
				else
					RenderBatchTechnique1(a1, 1, 0x5C006074, a2, 8);
			}
		}
		annotation->EndEvent();

		// RenderLODObjects
		annotation->BeginEvent(L"RenderLODObjects");
		{
			__int64 v12 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 120i64);

			if (v12)
			{
				if (*(BYTE *)(v12 + 38) & 1)
					RenderBatchTechnique2(v12, a2);
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
			__int64 v14 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 112i64);

			if (v14)
			{
				if (*(BYTE *)(v14 + 38) & 1)
					RenderBatchTechnique2(v14, a2);
				else
					RenderBatchTechnique1(a1, 1, 0x5C006074, a2, 0);
			}

			if (!v7)
				((void(__fastcall *)())(g_ModuleBase + 0x12F8910))();
		}
		annotation->EndEvent();

		// RenderSky
		/*
		annotation->BeginEvent(L"RenderSky");
		{
			DC_RenderDeferred(a1, a2, [](__int64 a1, unsigned int a2) {
				auto rbt1 = (__int64(__fastcall *)(__int64 a1, int a2, int a3, int a4, int a5))(g_ModuleBase + 0x12E3770);
				auto graphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

			if (graphicsGlobals->__zz0[76] != 1)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x100u;
				graphicsGlobals->__zz0[76] = 1;
			}
			float v15 = graphicsGlobals->float_14304DF68;
			if (graphicsGlobals->float_14304DF68 != 0.50196081)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x200u;
				graphicsGlobals->float_14304DF68 = 0.50196081f;
			}
			rbt1(a1, 0x5C00005D, 0x5C000064, a2, -1);
			});
		}
		DC_WaitDeferred();
		annotation->EndEvent();*/
		annotation->BeginEvent(L"RenderSky");
		if (graphicsGlobals->__zz0[76] != 1)
		{
			graphicsGlobals->dword_14304DEB0 |= 0x100u;
			graphicsGlobals->__zz0[76] = 1;
		}
		float v15 = graphicsGlobals->float_14304DF68;
		if (graphicsGlobals->float_14304DF68 != 0.50196081f)
		{
			graphicsGlobals->dword_14304DEB0 |= 0x200u;
			graphicsGlobals->float_14304DF68 = 0.50196081f;
		}
		RenderBatchTechnique1(a1, 0x5C00005D, 0x5C000064, a2, -1);
		annotation->EndEvent();

		// RenderSkyClouds
		annotation->BeginEvent(L"RenderSkyClouds");
		{
			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 11)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 11;
			}

			__int64 v17 = *(uint64_t *)(*(uint64_t *)(a1 + 304) + 216i64);

			if (v17)
			{
				if (*(BYTE *)(v17 + 38) & 1)
					RenderBatchTechnique2(v17, a2);
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
			((void(__fastcall *)())(g_ModuleBase + 0x12F87B0))();

		if (*(DWORD *)&graphicsGlobals->__zz0[72] != 10)
		{
			graphicsGlobals->dword_14304DEB0 |= 0x80u;
			*(DWORD *)&graphicsGlobals->__zz0[72] = 10;
		}

		((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E2450))(a1, a2);

		// BlendedDecals
		annotation->BeginEvent(L"BlendedDecals");
		{
			// WARNING: Nvidia NSight causes a bug (?) with the water texture somewhere. It gets drawn here.
			if (*(DWORD *)&graphicsGlobals->__zz0[72] != 11)
			{
				graphicsGlobals->dword_14304DEB0 |= 0x80u;
				*(DWORD *)&graphicsGlobals->__zz0[72] = 11;
			}

			((void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x12E25F0))(a1, a2);
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
		auto sub_140D744B0 = (int(__fastcall *)())(g_ModuleBase + 0xD744B0);
		auto sub_140D69E70 = (__int64(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0xD69E70);
		auto sub_140D69D30 = (__int64(__fastcall *)(float *a1, float a2, float a3, float a4, int a5))(g_ModuleBase + 0xD69D30);
		auto sub_1412FD120 = (signed __int64(__fastcall *)())(g_ModuleBase + 0x12FD120);
		auto sub_140D74350 = (__int64(__fastcall *)(__int64 a1, unsigned int a2, int a3, int a4, char a5))(g_ModuleBase + 0xD74350);
		auto sub_140D74370 = (void(__fastcall *)(__int64 a1, uint32_t a2, int a3, uint32_t a4))(g_ModuleBase + 0xD74370);
		auto sub_140D69990 = (void(__fastcall *)(__int64 a1, char a2))(g_ModuleBase + 0xD69990);
		auto sub_1412FADA0 = (__int64(__fastcall *)())(g_ModuleBase + 0x12FADA0);
		auto sub_140D69DA0 = (void(__fastcall *)(DWORD *a1))(g_ModuleBase + 0xD69DA0);

		DWORD *flt_14304E490 = (DWORD *)(g_ModuleBase + 0x304E490);

		if ((a2 & 0x80u) != 0 && sub_14131F100(*(uint64_t *)(a1 + 304), 0x5C000071u, 0x5C006071u))
		{
			int aiTarget = sub_140D744B0();

// 			bAssert(aiSource != aiTarget &&
// 				aiSource < DEPTH_STENCIL_COUNT &&
// 				aiTarget < DEPTH_STENCIL_COUNT &&
// 				aiSource != DEPTH_STENCIL_TARGET_NONE &&
// 				aiTarget != DEPTH_STENCIL_TARGET_NONE);

			graphicsGlobals->m_DeviceContext->CopyResource(*(ID3D11Resource **)(g_ModuleBase + 0x3050870), *((ID3D11Resource **)flt_14304E490 + 19 * v13 + 1015));
		}

		// RenderWaterStencil
		annotation->BeginEvent(L"RenderWaterStencil");
		{
			if (sub_14131F100(*(uint64_t *)(a1 + 304), 0x5C00006Du, 0x5C000070u))
			{
				sub_140D69E70((__int64)flt_14304E490, 2u);
				sub_140D69D30((float *)flt_14304E490, 0.0, 0.0, 0.0, 0);
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
			int v21 = sub_140D744B0();
			sub_140D74370((__int64)(g_ModuleBase + 0x3051B20), v21, 3, 0);
			if (*(DWORD *)&graphicsGlobals->__zz0[32] != 1)
			{
				*(DWORD *)&graphicsGlobals->__zz0[32] = 1;
				DWORD v22 = graphicsGlobals->dword_14304DEB0 & 0xFFFFFFFB;
				if (*(DWORD *)&graphicsGlobals->__zz0[36] != 1)
					v22 = graphicsGlobals->dword_14304DEB0 | 4;
				graphicsGlobals->dword_14304DEB0 = v22;
			}
		}

		if (!v7)
			((void(__fastcall *)())(g_ModuleBase + 0x12F8910))();
	}
}

#include <thread>
uint8_t *sub_1412E1600;

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

void DC_Init(ID3D11DeviceContext2 *ImmediateContext, ID3D11DeviceContext2 **DeferredContexts, int DeferredContextCount);
void hook()
{
	if (hooked)
		return;

	hooked = true;

	uintptr_t ptr = *(uintptr_t *)(&GetMainGlobals()->qword_14304BF00);
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

	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6FC10, (PBYTE)&CommitShaderChanges);
	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6BF00, (PBYTE)&sub_140D6BF00);
	*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1600, (PBYTE)&hk_sub_1412E1600);

	DC_Init(g_DeviceContext, &dc1, 1);

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
	//newDev->CreateDeferredContext2(0, &dc1);
	//newDev->CreateDeferredContext2(0, &dc2);

	OutputDebugStringA("Created everything\n");

    // Create ImGui globals
    ui::Initialize(pSwapChainDesc->OutputWindow, newDev, newContext);
    ui::log::Add("Created D3D11 device with feature level %X...\n", level);

    // Now hook the render function
	//*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)*ppSwapChain, &hk_IDXGISwapChain_Present, 8);
	//*(PBYTE *)&Map = Detours::X64::DetourClassVTable(*(PBYTE *)newContext, &hk_ID3D11DeviceContext_Map, 14);

	//*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1600, (PBYTE)&hk_sub_1412E1600);
	//*(PBYTE *)&sub_1412E1C10 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1C10, (PBYTE)&hk_sub_1412E1C10);

	//Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6BF00, (PBYTE)&sub_140D6BF00);

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
