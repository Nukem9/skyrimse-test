#include "common.h"
#include "patches/rendering/d3d11_tls.h"

ULONG_PTR g_ModuleBase;
ULONG_PTR g_ModuleSize;

ULONG_PTR g_CodeBase;
ULONG_PTR g_CodeSize;

#define PROCESS_CALLBACK_FILTER_ENABLED 0x1

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
#endif
}

void ApplyPatches()
{
	SetThreadName(GetCurrentThreadId(), "Main Thread");

	char filePath[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(nullptr), filePath, MAX_PATH);

	if (strstr(filePath, "SkyrimSE"))
		g_IsCreationKit = false;
	else if (strstr(filePath, "CreationKit"))
		g_IsCreationKit = true;

    // Called once the exe has been unpacked. Before applying code modifications:
    // ensure that exceptions are not swallowed when dispatching certain Windows
    // messages.
    //
    // http://blog.paulbetts.org/index.php/2010/07/20/the-case-of-the-disappearing-onload-exception-user-mode-callback-exceptions-in-x64/
    // https://support.microsoft.com/en-gb/kb/976038
    BOOL(WINAPI * SetProcessUserModeExceptionPolicyPtr)(DWORD PolicyFlags);
    BOOL(WINAPI * GetProcessUserModeExceptionPolicyPtr)(LPDWORD PolicyFlags);

    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

    if (kernel32)
    {
        *(FARPROC *)&SetProcessUserModeExceptionPolicyPtr = GetProcAddress(kernel32, "SetProcessUserModeExceptionPolicy");
        *(FARPROC *)&GetProcessUserModeExceptionPolicyPtr = GetProcAddress(kernel32, "GetProcessUserModeExceptionPolicy");

        if (SetProcessUserModeExceptionPolicyPtr && GetProcessUserModeExceptionPolicyPtr)
        {
            DWORD flags = 0;

            if (GetProcessUserModeExceptionPolicyPtr(&flags))
                SetProcessUserModeExceptionPolicyPtr(flags & ~PROCESS_CALLBACK_FILTER_ENABLED);
        }
    }

    SetErrorMode(0);
    SetThreadErrorMode(0, nullptr);

	if (g_IsCreationKit)
	{
		Patch_TESVCreationKit();
	}
	else
	{
#if SKYRIM64_USE_VFS
		once();
		DoHook();
#endif

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
