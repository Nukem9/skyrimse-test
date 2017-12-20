#include "common.h"
#include "../../ui/ui.h"
#include "../TES/BSShader/BSPixelShader.h"
#include "../TES/BSShader/BSVertexShader.h"
#include "../TES/BSShader/BSShaderManager.h"
#include "../TES/BSShader/BSShaderRenderTargets.h"
#include "../TES/BSSpinLock.h"
#include "../TES/MemoryContextTracker.h"
#include "../TES/BSShader/BSShaderAccumulator.h"
#include "../TES/BSBatchRenderer.h"

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
ID3D11DeviceContext2 *dc3;

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

extern uint32_t lastTechId;
void BSGraphics__Renderer__SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode);

uint32_t opt1;
uint32_t opt2;
uint32_t opt3;

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

	if (lastTechId == 0x5C00002E)
	{
		renderer->dword_14304DEB0 |= 0x80;

		//*(signed int *)&renderer->__zz2[656] = 0;
		*(signed int *)&renderer->__zz0[64] = 0;
		*(signed int *)&renderer->__zz0[68] = 0;
		*(signed int *)&renderer->__zz0[72] = 1;
	}

	//wchar_t buffer[256];
	//_swprintf(buffer, L"SETTINGS [%d][%d][%d][%d]", *(unsigned int *)&renderer->__zz2[656], *(signed int *)&renderer->__zz0[64], *(signed int *)&renderer->__zz0[68], *(signed int *)&renderer->__zz0[72]);
	//annotation->SetMarker(buffer);

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
		if (v1 & (0x4 | 0x8))
		{
			auto test = (ID3D11DepthStencilState *)renderer->m_DepthStates[0][*(signed int *)&renderer->__zz0[40] + 40i64 * *(signed int *)&renderer->__zz0[32]];

			if (test != renderer->m_DepthStates[*(signed int *)&renderer->__zz0[32]][*(signed int *)&renderer->__zz0[40]])
				__debugbreak();

			// OMSetDepthStencilState(m_DepthStates[m_DepthMode][m_StencilMode], m_StencilRef);
			renderer->m_DeviceContext->OMSetDepthStencilState(
				renderer->m_DepthStates[*(signed int *)&renderer->__zz0[32]][*(signed int *)&renderer->__zz0[40]],
				*(UINT *)&renderer->__zz0[44]);

			v1 = renderer->dword_14304DEB0;
		}

		// RSSetState
		if (v1 & (0x1000 | 0x40 | 0x20 | 0x10))
		{
			// Cull mode, depth bias, fill mode, scissor mode, scissor rect (order unknown)
			void *wtf = renderer->qword_14304C930[0][0][0][*(signed int *)&renderer->__zz0[60]
				+ 2
				* (*(signed int *)&renderer->__zz0[56]
					+ 12
					* (*(signed int *)&renderer->__zz0[52]// Cull mode
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
		if (v1 & 0x2)
		{
			renderer->m_DeviceContext->RSSetViewports(1, (D3D11_VIEWPORT *)&renderer->__zz0[8]);

			v1 = renderer->dword_14304DEB0;
		}

		// OMSetBlendState
		if (v1 & 0x80)
		{
			float *blendFactor = (float *)(g_ModuleBase + 0x1E2C168);

			// Mode, write mode, alpha to coverage, blend state (order unknown)
			void *wtf = renderer->qword_14304CDB0[0][0][0][*(unsigned int *)&renderer->__zz2[656]
				+ 2
				* (*(signed int *)&renderer->__zz0[72]
					+ 13
					* (*(signed int *)&renderer->__zz0[68]
						+ 2i64 * *(signed int *)&renderer->__zz0[64]))];// AlphaBlendMode

			renderer->m_DeviceContext->OMSetBlendState((ID3D11BlendState *)wtf, blendFactor, 0xFFFFFFFF);

			v1 = renderer->dword_14304DEB0;
		}

		if (v1 & (0x200 | 0x100))
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

			auto sub_140C06080 = (__int64(__fastcall *)(DWORD *a1, unsigned __int64 a2))(g_ModuleBase + 0xC060B0);
			auto sub_140D705F0 = (__int64(__fastcall *)(unsigned __int64 a1))(g_ModuleBase + 0xD70620);
			auto sub_140D72740 = (char(__fastcall *)(__int64 a1, __int64 a2, int a3, __int64 *a4, int64_t *a5))(g_ModuleBase + 0xD72770);
			auto sub_140D735D0 = (void(__fastcall *)(__int64 a1))(g_ModuleBase + 0xD73600);

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
		if (_bittest((const LONG *)&v1, 11))
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
	//return;
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

	if (FAILED(newDev->CreateDeferredContext2(0, &dc1)))
		__debugbreak();

	if (FAILED(newDev->CreateDeferredContext2(0, &dc2)))
		__debugbreak();

	if (FAILED(newDev->CreateDeferredContext2(0, &dc3)))
		__debugbreak();

	if (!ptrPresent)
		*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)swap, &hk_IDXGISwapChain_Present, 8);

	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6FC40, (PBYTE)&CommitShaderChanges);
	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6BF30, (PBYTE)&sub_140D6BF00);
	*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1960, (PBYTE)&BSShaderAccumulator::sub_1412E1600);

	ID3D11DeviceContext2 *contexts[3];
	contexts[0] = dc1;
	contexts[1] = dc2;
	contexts[2] = dc3;

	DC_Init(g_DeviceContext, contexts, ARRAYSIZE(contexts));

	//*(PBYTE *)&sub_1412E1C10 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1F70, (PBYTE)&hk_sub_1412E1C10);
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
            Flags,
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
	*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)*ppSwapChain, &hk_IDXGISwapChain_Present, 8);
	//*(PBYTE *)&Map = Detours::X64::DetourClassVTable(*(PBYTE *)newContext, &hk_ID3D11DeviceContext_Map, 14);

	//Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6FC40, (PBYTE)&CommitShaderChanges);
	//*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1960, (PBYTE)&BSShaderAccumulator::sub_1412E1600);
	//*(PBYTE *)&sub_1412E1C10 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1F70, (PBYTE)&hk_sub_1412E1C10);

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

	*(PBYTE *)&BuildShaderBundle = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x13364A0), (PBYTE)&hk_BuildShaderBundle);

    PatchIAT(hk_CreateDXGIFactory, "dxgi.dll", "CreateDXGIFactory");
    PatchIAT(hk_D3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain");
}
