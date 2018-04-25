#include "common.h"
#include "GpuTimer.h"
#include "../../ui/ui.h"
#include "../TES/BSShader/BSPixelShader.h"
#include "../TES/BSShader/BSVertexShader.h"
#include "../TES/BSShader/BSShaderManager.h"
#include "../TES/BSShader/BSShaderRenderTargets.h"
#include "../TES/BSShader/Shaders/BSBloodSplatterShader.h"
#include "../TES/BSShader/Shaders/BSDistantTreeShader.h"
#include "../TES/BSShader/Shaders/BSSkyShader.h"
#include "../TES/BSShader/Shaders/BSGrassShader.h"
#include "../TES/BSGraphicsRenderer.h"
#include "../TES/BSBatchRenderer.h"

IDXGISwapChain *g_SwapChain;
ID3D11DeviceContext2 *g_DeviceContext;
double g_AverageFps;
ID3DUserDefinedAnnotation *annotation;

decltype(&IDXGISwapChain::Present) ptrPresent;
decltype(&CreateDXGIFactory) ptrCreateDXGIFactory;
decltype(&D3D11CreateDeviceAndSwapChain) ptrD3D11CreateDeviceAndSwapChain;

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

LARGE_INTEGER g_FrameStart;
LARGE_INTEGER g_FrameEnd;
LARGE_INTEGER g_FrameDelta;

bool init = false;
HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
	ui::Render();

	if (!g_DeviceContext)
	{
		ID3D11DeviceContext1 *ctx = nullptr;
		ID3D11Device1 *dev;
		This->GetDevice(__uuidof(ID3D11Device1), (void **)&dev);
		dev->GetImmediateContext1(&ctx);

		ctx->QueryInterface<ID3D11DeviceContext2>(&g_DeviceContext);
		ctx->QueryInterface<ID3DUserDefinedAnnotation>(&annotation);
	}

	if (init)
	{
		g_GPUTimers.StopTimer(g_DeviceContext, 0);
		QueryPerformanceCounter(&g_FrameEnd);

		g_FrameDelta.QuadPart = g_FrameEnd.QuadPart - g_FrameStart.QuadPart;
	}

	g_GPUTimers.EndFrame(g_DeviceContext);
    HRESULT hr = (This->*ptrPresent)(SyncInterval, Flags);
	g_GPUTimers.BeginFrame(g_DeviceContext);

	g_GPUTimers.StartTimer(g_DeviceContext, 0);
	QueryPerformanceCounter(&g_FrameStart);
	init = true;

	BSGraphics::Renderer::OnNewFrame();

	using namespace BSShaderRenderTargets;

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

void *sub_140D6BF00(__int64 a1, int AllocationSize, uint32_t *AllocationOffset)
{
	return BSGraphics::Renderer::GetGlobals()->MapDynamicBuffer(AllocationSize, AllocationOffset);

#if 0
	//if (AllocationSize <= 0)
	//	bAssert((__int64)"Win32\\BSGraphicsRenderer.cpp", 3163i64, (__int64)"Size must be > 0");

	uint32_t frameDataOffset = globals->m_FrameDataUsedSize;
	uint32_t frameBufferIndex = globals->m_CurrentDynamicBufferIndex;
	uint32_t newFrameDataSzie = globals->m_FrameDataUsedSize + AllocationSize;

	//
	// Check if this request would exceed the allocated buffer size for the currently executing command list. If it does,
	// we end the current query and move on to the next buffer.
	//
	if (newFrameDataSzie > 0x400000)
	{
		//if (AllocationSize > 0x400000)
		//	bAssert((__int64)"Win32\\BSGraphicsRenderer.cpp", 3174i64, (__int64)"Dynamic geometry buffer overflow.");

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

uint8_t *sub_1412E1600;

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

void DumpVertexShader(BSGraphics::VertexShader *Shader, const char *Type);
void DumpPixelShader(BSGraphics::PixelShader *Shader, const char *Type, void *Buffer, size_t BufferLen);
void hk_BuildShaderBundle(__int64 shaderGroupObject, __int64 fileStream)
{
	hook();

	NextShaderType = (const char *)*(uintptr_t *)(shaderGroupObject + 136);
	((decltype(&hk_BuildShaderBundle))BuildShaderBundle)(shaderGroupObject, fileStream);
	
	if (shaderGroupObject == (__int64)BSBloodSplatterShader::pInstance)
		BSBloodSplatterShader::pInstance->CreateAllShaders();

	if (shaderGroupObject == (__int64)BSDistantTreeShader::pInstance)
		BSDistantTreeShader::pInstance->CreateAllShaders();

	if (shaderGroupObject == (__int64)BSGrassShader::pInstance)
		BSGrassShader::pInstance->CreateAllShaders();

	//if (shaderGroupObject == (__int64)BSSkyShader::pInstance)
	//	BSSkyShader::pInstance->CreateAllShaders();

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
						DumpVertexShader((BSGraphics::VertexShader *)first->shader, NextShaderType);
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
						auto p = (BSGraphics::PixelShader *)first->shader;
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

void DC_Init(ID3D11Device1 *Device, int DeferredContextCount);
void hook()
{
	//return;
	if (hooked)
		return;

	hooked = true;

	BSGraphics::Renderer::Initialize();

	uintptr_t ptr = *(uintptr_t *)(&BSGraphics::Renderer::GetGlobalsNonThreaded()->qword_14304BF00);
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

	if (!ptrPresent)
		*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)swap, &hk_IDXGISwapChain_Present, 8);

	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x131F0D0, (PBYTE)&BSBatchRenderer::SetupAndDrawPass);
	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6FC40, (PBYTE)&BSGraphics::Renderer::SyncD3DState);
	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6BF30, (PBYTE)&sub_140D6BF00);
	*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1960, (PBYTE)&BSShaderAccumulator::sub_1412E1600);

	DC_Init(newDev, 0);
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
	//
    // From MSDN:
    //
    // If the Direct3D 11.1 runtime is present on the computer and pFeatureLevels is set to NULL,
    // this function won't create a D3D_FEATURE_LEVEL_11_1 device. To create a D3D_FEATURE_LEVEL_11_1
    // device, you must explicitly provide a D3D_FEATURE_LEVEL array that includes
    // D3D_FEATURE_LEVEL_11_1. If you provide a D3D_FEATURE_LEVEL array that contains
    // D3D_FEATURE_LEVEL_11_1 on a computer that doesn't have the Direct3D 11.1 runtime installed,
    // this function immediately fails with E_INVALIDARG.
	//
    const D3D_FEATURE_LEVEL testFeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

	//
    // Loop to get the highest available feature level; SkyrimSE originally uses D3D_FL_9_1.
	// SkyrimSE also uses a single render thread (sadface).
	//
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

	OutputDebugStringA("Created everything\n");

	newDev->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR);
	BSGraphics::Renderer::FlushThreadedVars();

    // Create ImGui globals
    ui::Initialize(pSwapChainDesc->OutputWindow, newDev, newContext);
    ui::log::Add("Created D3D11 device with feature level %X...\n", level);

    // Now hook the render function
	*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)*ppSwapChain, &hk_IDXGISwapChain_Present, 8);
	//CreatePixelShader = Detours::X64::DetourClassVTable(*(PBYTE *)newDev, &hk_CreatePixelShader, 15);

	//Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6FC40, (PBYTE)&CommitShaderChanges);
	//*(PBYTE *)&sub_1412E1600 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1960, (PBYTE)&BSShaderAccumulator::sub_1412E1600);
	//*(PBYTE *)&sub_1412E1C10 = Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x12E1F70, (PBYTE)&hk_sub_1412E1C10);

    return hr;
}

void CreateXbyakCodeBlock();
void CreateXbyakPatches();

__int64 __fastcall sub_141318C10(__int64 a1, int a2, char a3, char a4, char a5)
{
	annotation->BeginEvent(L"BSCubemapCamera: Draw");

	__int64 result = ((__int64(__fastcall *)(__int64 a1, int a2, char a3, char a4, char a5))(g_ModuleBase + 0x1319000))(a1, a2, a3, a4, a5);

	annotation->EndEvent();

	return result;
}

void asdf1(__int64 a1, BSVertexShader *Shader)
{
//	BSGraphics::Renderer::SetVertexShader(Shader);
}

void asdf2(__int64 a1, BSPixelShader *Shader)
{
//	BSGraphics::Renderer::SetPixelShader(Shader);
}

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

	//Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x0D6F040), (PBYTE)&asdf1);
	//Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x0D6F3F0), (PBYTE)&asdf2);
	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x1318C10), (PBYTE)&sub_141318C10);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1336860), &BSShader::BeginTechnique);

	*(PBYTE *)&BuildShaderBundle = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x13364A0), (PBYTE)&hk_BuildShaderBundle);

    PatchIAT(hk_CreateDXGIFactory, "dxgi.dll", "CreateDXGIFactory");
    PatchIAT(hk_D3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain");
}
