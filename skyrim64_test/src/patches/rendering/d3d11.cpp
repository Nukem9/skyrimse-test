#include "common.h"
#include <xbyak/xbyak.h>
#include "d3d11_proxy.h"
#include "GpuTimer.h"
#include "../../ui/ui.h"
#include "../TES/BSShader/BSShaderManager.h"
#include "../TES/BSShader/BSShaderRenderTargets.h"
#include "../TES/BSShader/Shaders/BSBloodSplatterShader.h"
#include "../TES/BSShader/Shaders/BSDistantTreeShader.h"
#include "../TES/BSShader/Shaders/BSSkyShader.h"
#include "../TES/BSShader/Shaders/BSGrassShader.h"
#include "../TES/BSGraphicsRenderer.h"
#include "../TES/BSBatchRenderer.h"

ID3D11Texture2D *g_OcclusionTexture;
ID3D11ShaderResourceView *g_OcclusionTextureSRV;

ID3D11Device2 *g_Device;
IDXGISwapChain *g_SwapChain;
ID3D11DeviceContext2 *g_DeviceContext;
double g_AverageFps;

decltype(&IDXGISwapChain::Present) ptrPresent;
decltype(&CreateDXGIFactory) ptrCreateDXGIFactory;
decltype(&D3D11CreateDeviceAndSwapChain) ptrD3D11CreateDeviceAndSwapChain;

void DC_Init(ID3D11Device2 *Device, int DeferredContextCount);

LARGE_INTEGER g_FrameStart;
LARGE_INTEGER g_FrameEnd;
LARGE_INTEGER g_FrameDelta;
extern HWND g_SkyrimWindow;

bool init = false;
HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
	DXGI_SWAP_CHAIN_DESC desc;
	This->GetDesc(&desc);

	if (desc.OutputWindow != g_SkyrimWindow)
	{
		return (This->*ptrPresent)(SyncInterval, Flags);
	}

	if (init)
	{
		g_GPUTimers.StopTimer(g_DeviceContext, 0);
		QueryPerformanceCounter(&g_FrameEnd);

		g_FrameDelta.QuadPart = g_FrameEnd.QuadPart - g_FrameStart.QuadPart;
		g_GPUTimers.EndFrame(g_DeviceContext);
	}

	ui::EndFrame();
	HRESULT hr;
	{
		ZoneScopedNC("Present", tracy::Color::Red);
		hr = (This->*ptrPresent)(SyncInterval, Flags);
	}

	//TracyDx11Collect(g_DeviceContext);
	FrameMark;

	ui::BeginFrame();
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
	g_DeviceContext->BeginEventInt(L"SLI Hacks", 0);
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
	g_DeviceContext->EndEvent();

	return hr;
}

void *sub_140D6BF00(__int64 a1, int AllocationSize, uint32_t *AllocationOffset)
{
	return BSGraphics::Renderer::GetGlobals()->MapDynamicBuffer(AllocationSize, AllocationOffset);

#if 0
	AssertMsg(AllocationSize > 0, "Size must be > 0");

	uint32_t frameDataOffset = globals->m_FrameDataUsedSize;
	uint32_t frameBufferIndex = globals->m_CurrentDynamicBufferIndex;
	uint32_t newFrameDataSzie = globals->m_FrameDataUsedSize + AllocationSize;

	//
	// Check if this request would exceed the allocated buffer size for the currently executing command list. If it does,
	// we end the current query and move on to the next buffer.
	//
	if (newFrameDataSzie > 0x400000)
	{
		AssertMsg(AllocationSize <= 0x400000, "Dynamic geometry buffer overflow.");

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

uint8_t *RenderSceneNormal;

HRESULT WINAPI hk_CreateDXGIFactory(REFIID riid, void **ppFactory)
{
    ui::log::Add("Creating DXGI factory...\n");

    if (SUCCEEDED(ptrCreateDXGIFactory(__uuidof(IDXGIFactory3), ppFactory)))
        return S_OK;

    if (SUCCEEDED(ptrCreateDXGIFactory(__uuidof(IDXGIFactory2), ppFactory)))
        return S_OK;

    return ptrCreateDXGIFactory(__uuidof(IDXGIFactory), ppFactory);
}

const char *NextShaderType;

uint8_t *BuildComputeShaderBundle;
void hk_BuildComputeShaderBundle(__int64 shaderGroupObject, __int64 fileStream)
{
	NextShaderType = (const char *)*(uintptr_t *)(shaderGroupObject + 24);
	((decltype(&hk_BuildComputeShaderBundle))BuildComputeShaderBundle)(shaderGroupObject, fileStream);
	NextShaderType = nullptr;
}

HRESULT WINAPI NsightHack_D3D11CreateDeviceAndSwapChain(
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
	// Nvidia NSight checks the return address of D3D11CreateDeviceAndSwapChain to see if it's
	// a blacklisted directx dll. "d3dx9_42.dll" happens to be in that list. So, now I need to
	// generate code which spoofs the return address to something random.
	//
	// NOTE: Do NOT touch rcx, rdx, r8, r9
	//
	class d3djmp : public Xbyak::CodeGenerator
	{
	public:
		d3djmp() : Xbyak::CodeGenerator()
		{
			push(rbx);
			push(rsi);
			push(rdi);
			sub(rsp, 0x60);
			mov(rax, qword[rsp + 0xD8]);
			mov(r10, qword[rsp + 0xD0]);
			mov(r11, qword[rsp + 0xC8]);
			mov(rbx, qword[rsp + 0xC0]);
			mov(rdi, qword[rsp + 0xB8]);
			mov(rsi, qword[rsp + 0xA0]);
			mov(qword[rsp + 0x58], rax);
			mov(eax, dword[rsp + 0xB0]);
			mov(qword[rsp + 0x50], r10);
			mov(qword[rsp + 0x48], r11);
			mov(qword[rsp + 0x40], rbx);
			mov(qword[rsp + 0x38], rdi);
			mov(dword[rsp + 0x30], eax);
			mov(eax, dword[rsp + 0xA8]);
			mov(dword[rsp + 0x28], eax);
			mov(qword[rsp + 0x20], rsi);

			mov(rax, (uintptr_t)ptrD3D11CreateDeviceAndSwapChain);
			call(rax);

			add(rsp, 0x60);
			pop(rdi);
			pop(rsi);
			pop(rbx);
			ret();
		}
	} hack;

	auto newPtr = hack.getCode<decltype(&D3D11CreateDeviceAndSwapChain)>();
	return newPtr(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
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
    const D3D_FEATURE_LEVEL testFeatureLevels[] =
	{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	//
    // Loop to get the highest available feature level; SkyrimSE originally uses D3D_FL_9_1.
	// SkyrimSE also uses a single render thread (sadface).
	//
    D3D_FEATURE_LEVEL level;
    HRESULT hr;

    for (int i = 0; i < ARRAYSIZE(testFeatureLevels); i++)
    {
        hr = NsightHack_D3D11CreateDeviceAndSwapChain(
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

        if (SUCCEEDED(hr))
        {
            if (pFeatureLevel)
                *pFeatureLevel = level;

            break;
        }
    }

	// Skyrim never checks the return value of D3D11CreateDeviceAndSwapChain, so do it ourselves
	if (FAILED(hr))
	{
		ShowWindow(pSwapChainDesc->OutputWindow, SW_HIDE);

		AssertMsg(false, "DirectX11 device creation failed. Game will now exit.");
		ExitProcess(0);
	}

	// Create ImGui globals BEFORE the device is proxied
	ui::Initialize(pSwapChainDesc->OutputWindow, *ppDevice, *ppImmediateContext);
	ui::log::Add("Created D3D11 device with feature level %X...\n", level);

	// Force DirectX11.2 in case we use features later (11.3+ requires Win10 or higher)
	ID3D11Device2 *proxyDevice = new D3D11DeviceProxy(*ppDevice);
	ID3D11DeviceContext2 *proxyContext = new D3D11DeviceContextProxy(*ppImmediateContext);

	proxyDevice->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR);

	g_Device = proxyDevice;
	*ppDevice = proxyDevice;

	g_DeviceContext = proxyContext;
	*ppImmediateContext = proxyContext;

	g_SwapChain = *ppSwapChain;

	BSGraphics::Renderer::FlushThreadedVars();
	BSGraphics::Renderer::Initialize(g_Device);

    // Now hook the render function
	*(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)*ppSwapChain, &hk_IDXGISwapChain_Present, 8);

	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0x131F0D0, (PBYTE)&BSBatchRenderer::RenderPassImmediately);
	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6FC40, (PBYTE)&BSGraphics::Renderer::SyncD3DState);
	Detours::X64::DetourFunction((PBYTE)g_ModuleBase + 0xD6BF30, (PBYTE)&sub_140D6BF00);
	*(PBYTE *)&RenderSceneNormal = Detours::X64::DetourFunctionClass((PBYTE)g_ModuleBase + 0x12E1960, &BSShaderAccumulator::RenderSceneNormal);

	g_GPUTimers.Create(g_Device, 1);
	//TracyDx11Context(g_Device, g_DeviceContext);
	//DC_Init(g_Device, 0);

	// Culling test buffers
	CD3D11_TEXTURE2D_DESC cpuRenderTargetDescAVX
	(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		1280, // TODO: round up to full tile sizes
		720,
		1, // Array Size
		1, // MIP Levels
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE
	);
	Assert(SUCCEEDED(g_Device->CreateTexture2D(&cpuRenderTargetDescAVX, nullptr, &g_OcclusionTexture)));
	Assert(SUCCEEDED(g_Device->CreateShaderResourceView(g_OcclusionTexture, nullptr, &g_OcclusionTextureSRV)));

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

	AssertMsg(ptrCreateDXGIFactory, "CreateDXGIFactory import not found");
	AssertMsg(ptrD3D11CreateDeviceAndSwapChain, "D3D11CreateDeviceAndSwapChain import not found");

	CreateXbyakCodeBlock();
	CreateXbyakPatches();

    PatchIAT(hk_CreateDXGIFactory, "dxgi.dll", "CreateDXGIFactory");
    PatchIAT(hk_D3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain");
}
