#define XBYAK_NO_OP_NAMES

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
#include "../TES/BSGraphics/BSGraphicsRenderer.h"
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

	BSGraphics::Renderer::QInstance()->OnNewFrame();

	return hr;
}

uint8_t *FinishAccumulating_Standard_PreResolveDepth;

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

	BSGraphics::Renderer::QInstance()->Initialize();

    // Now hook the render function
	*(uintptr_t *)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t *)*ppSwapChain, &hk_IDXGISwapChain_Present, 8);

	Detours::X64::DetourFunction(g_ModuleBase + 0x131F0D0, (uintptr_t)&BSBatchRenderer::RenderPassImmediately);
	Detours::X64::DetourFunction(g_ModuleBase + 0xD6FC40, (uintptr_t)&BSGraphics::Renderer::SetDirtyStates);
	*(uintptr_t *)&FinishAccumulating_Standard_PreResolveDepth = Detours::X64::DetourFunctionClass(g_ModuleBase + 0x12E1960, &BSShaderAccumulator::FinishAccumulating_Standard_PreResolveDepth);

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
	if (!g_DllDXGI)
		g_DllDXGI = GetModuleHandleA("dxgi.dll");

	if (!g_DllD3D11)
		g_DllD3D11 = GetModuleHandleA("d3d11.dll");

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
