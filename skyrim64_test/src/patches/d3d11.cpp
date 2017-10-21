#include "../stdafx.h"
#include "../ui/ui.h"

// BSGraphicsRenderer

IDXGISwapChain *g_SwapChain;
double g_AverageFps;

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

int64_t LastFrame;
int64_t TickSum;
int64_t TickDeltas[32];
int TickDeltaIndex;

HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
	ui::Render();

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

    if (frameTimeMs >= 100.0)
        ui::log::Add("FRAME HITCH WARNING (%g ms)\n", frameTimeMs);

    return (This->*ptrPresent)(SyncInterval, Flags);
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

    // Loop to get the highest available feature level; SkyrimSE originally uses D3D_FL_9_1
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

            g_SwapChain = *ppSwapChain;
            break;
        }
    }

    if (FAILED(hr))
        return hr;

    // Create ImGui globals
    ui::Initialize(pSwapChainDesc->OutputWindow, *ppDevice, *ppImmediateContext);
    ui::log::Add("Created D3D11 device with feature level %X...\n", level);

    // Now hook the render function
    *(PBYTE *)&ptrPresent = Detours::X64::DetourClassVTable(*(PBYTE *)*ppSwapChain, &hk_IDXGISwapChain_Present, 8);
    return S_OK;
}

void logva(const char *Format, va_list va)
{
    char buffer[2048];
    vsnprintf_s(buffer, _TRUNCATE, Format, va);

    ui::log::Add("%s\n", buffer);
}

int MyLogFunc3(__int64 a1, const char *Format, ...)
{
    va_list va;
    va_start(va, Format);

    logva(Format, va);
    return 0;
}

bool StartsWith(const char *Buf, const char *Str)
{
    return strstr(Buf, Str) == Buf;
}

void MyLogFunc2(const char *Format, ...)
{
    va_list va;
    va_start(va, Format);

    if (StartsWith(Format, " %s asking for random") || StartsWith(Format, " %s got a quest back") || StartsWith(Format, " %s failed to get"))
        return;

    logva(Format, va);
}

int MyLogFunc(const char *Format, ...)
{
    va_list va;
    va_start(va, Format);

    logva(Format, va);
    return 0;
}

int hk_sprintf_s(char *DstBuf, size_t SizeInBytes, const char *Format, ...)
{
    va_list va;
    va_start(va, Format);
    int len = vsprintf_s(DstBuf, SizeInBytes, Format, va);
    va_end(va);

    if (strlen(Format) <= 20 || StartsWith(DstBuf, "data\\") || StartsWith(DstBuf, "Data\\") || StartsWith(DstBuf, "mesh\\") || StartsWith(DstBuf, "Meshes\\") || StartsWith(DstBuf, "Textures\\") || StartsWith(DstBuf, "Interface\\") || StartsWith(DstBuf, "SHADERSFX") || StartsWith(Format, "%s (%08X)[%d]/%s") || StartsWith(Format, "alias %s on"))
        return len;

    ui::log::Add("%s\n", DstBuf);
    return len;
}

void PatchD3D11()
{
    Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x1CDF30), (PBYTE)&MyLogFunc);
    Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x166050), (PBYTE)&MyLogFunc2);
    Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x577AA0), (PBYTE)&MyLogFunc2);
    Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x179BC0), (PBYTE)&MyLogFunc3);
    Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x1424D0), (PBYTE)&hk_sprintf_s);

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

    PatchIAT(hk_CreateDXGIFactory, "dxgi.dll", "CreateDXGIFactory");
    PatchIAT(hk_D3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain");
}
