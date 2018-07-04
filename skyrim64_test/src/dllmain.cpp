#include "common.h"
#include "patches/rendering/d3d11_tls.h"

#if SKYRIM64_USE_VTUNE
__itt_heap_function ITT_AllocateCallback;
__itt_heap_function ITT_ReallocateCallback;
__itt_heap_function ITT_FreeCallback;
#endif

ULONG_PTR g_ModuleBase;
ULONG_PTR g_ModuleSize;

ULONG_PTR g_CodeBase;
ULONG_PTR g_CodeSize;

HMODULE g_Dll3DAudio;
HMODULE g_DllReshade;
HMODULE g_DllEnb;
HMODULE g_DllSKSE;
HMODULE g_DllVTune;

HMODULE g_DllDXGI;
HMODULE g_DllD3D11;

bool g_IsCreationKit;

void once();
void DoHook();
void Patch_TESV();
void Patch_TESVCreationKit();

void LoadModules()
{
    // X3DAudio HRTF is a drop-in replacement for x3daudio1_7.dll
    g_Dll3DAudio = LoadLibraryA("test\\x3daudio1_7.dll");

    if (g_Dll3DAudio)
    {
        PatchIAT(GetProcAddress(g_Dll3DAudio, "X3DAudioCalculate"), "x3daudio1_7.dll", "X3DAudioCalculate");
        PatchIAT(GetProcAddress(g_Dll3DAudio, "X3DAudioInitialize"), "x3daudio1_7.dll", "X3DAudioInitialize");
    }

    // Reshade is a drop-in replacement for DXGI.dll
    g_DllReshade = LoadLibraryA("test\\r_dxgi.dll");
    g_DllDXGI    = (g_DllReshade) ? g_DllReshade : GetModuleHandleA("dxgi.dll");

    // ENB is a drop-in replacement for D3D11.dll
    LoadLibraryA("test\\d3dcompiler_46e.dll");

    g_DllEnb   = LoadLibraryA("test\\d3d11.dll");
    g_DllD3D11 = (g_DllEnb) ? g_DllEnb : GetModuleHandleA("d3d11.dll");

    // SKSE64 loads by itself in the root dir
    g_DllSKSE = LoadLibraryA("skse64_1_5_3.dll");

#if SKYRIM64_USE_VTUNE
    // Check if VTune is active
    const char *libttPath = getenv("INTEL_LIBITTNOTIFY64");

    if (!libttPath)
        libttPath = "libittnotify.dll";

    g_DllVTune = LoadLibraryA(libttPath);

	ITT_AllocateCallback = __itt_heap_function_create("Allocate", "MemoryManager");
	ITT_ReallocateCallback = __itt_heap_function_create("Reallocate", "MemoryManager");
	ITT_FreeCallback = __itt_heap_function_create("Free", "MemoryManager");
#endif
}

void ApplyPatches()
{
	SetThreadName(GetCurrentThreadId(), "Main Thread");

#if SKYRIM64_USE_VFS
	once();
	DoHook();
#endif

	char filePath[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(nullptr), filePath, MAX_PATH);

	if (strstr(filePath, "SkyrimSE"))
		g_IsCreationKit = false;
	else if (strstr(filePath, "CreationKit"))
		g_IsCreationKit = true;

	// The EXE has been unpacked at this point
	if (g_IsCreationKit)
	{
		Patch_TESVCreationKit();
	}
	else
	{
		InitializeTLSMain();
		LoadModules();
		Patch_TESV();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        EnableDumpBreakpoint();
        return TRUE;
    }

    return FALSE;
}
