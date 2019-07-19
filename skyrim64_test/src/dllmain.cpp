#include "common.h"
#include "version_info.h"
#include "patches/rendering/d3d11_tls.h"

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
	// The EXE has been unpacked at this point
	XUtil::SetThreadName(GetCurrentThreadId(), "Main Thread");

#if SKYRIM64_USE_VTUNE
	__itt_domain_create("MemoryManager");
	ITT_AllocateCallback = __itt_heap_function_create("Allocate", "MemoryManager");
	ITT_ReallocateCallback = __itt_heap_function_create("Reallocate", "MemoryManager");
	ITT_FreeCallback = __itt_heap_function_create("Free", "MemoryManager");
#endif

#if SKYRIM64_USE_VFS
	once();
	DoHook();
#endif

	strcpy_s(g_GitVersion, VER_CURRENT_COMMIT_ID);

	switch (g_LoadType)
	{
#if !SKYRIM64_CREATIONKIT_ONLY
	case GAME_EXECUTABLE_TYPE::GAME:
		TLSPatcherInitialize();
		LoadModules();
		Patch_TESV();
		break;
#endif

	case GAME_EXECUTABLE_TYPE::CREATIONKIT:
		Patch_TESVCreationKit();
		break;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
		// Force this dll to be loaded permanently
		HMODULE temp;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)hModule, &temp);

		// Then determine which exe is being loaded
		char modulePath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandle(nullptr), modulePath, MAX_PATH);

		char executableName[MAX_PATH];
		_splitpath_s(modulePath, nullptr, 0, nullptr, 0, executableName, ARRAYSIZE(executableName), nullptr, 0);

		if (!_stricmp(executableName, "SkyrimSELauncher"))
			g_LoadType = GAME_EXECUTABLE_TYPE::LAUNCHER;
		else if (!_stricmp(executableName, "SkyrimSE") || !_stricmp(executableName, "SkyrimSE_dump"))
			g_LoadType = GAME_EXECUTABLE_TYPE::GAME;
		else if (!_stricmp(executableName, "CreationKit"))
			g_LoadType = GAME_EXECUTABLE_TYPE::CREATIONKIT;
		else
			g_LoadType = GAME_EXECUTABLE_TYPE::UNKNOWN;

		// For now, skip everything except the game and CK
		if (g_LoadType == GAME_EXECUTABLE_TYPE::GAME || g_LoadType == GAME_EXECUTABLE_TYPE::CREATIONKIT)
		{
#if SKYRIM64_CREATIONKIT_ONLY
			if (g_LoadType != GAME_EXECUTABLE_TYPE::CREATIONKIT)
				return TRUE;
#endif

			DumpEnableBreakpoint();
		}
    }

#if !SKYRIM64_CREATIONKIT_ONLY
	if (g_LoadType == GAME_EXECUTABLE_TYPE::GAME)
		TLSPatcherCallback(hModule, fdwReason, lpReserved);
#endif

    return TRUE;
}
