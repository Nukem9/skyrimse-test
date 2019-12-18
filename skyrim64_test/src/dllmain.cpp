#include "common.h"
#include "version_info.h"
#include "profiler_internal.h"
#include "patches/rendering/d3d11_tls.h"

void Patch_TESV();
void Patch_TESVCreationKit();
void Patch_Fallout4CreationKit();

void ApplyPatches()
{
	// The EXE has been unpacked at this point
	strcpy_s(g_GitVersion, VER_CURRENT_COMMIT_ID);
	XUtil::SetThreadName(GetCurrentThreadId(), "Main Thread");

#if SKYRIM64_USE_VTUNE
	// Check if VTune is already active
	const char *libITTPath = getenv("INTEL_LIBITTNOTIFY64");

	if (!libITTPath)
		libITTPath = "libittnotify.dll";

	g_DllVTune = LoadLibraryA(libITTPath);

	__itt_domain_create("MemoryManager");
	ITT_AllocateCallback = __itt_heap_function_create("Allocate", "MemoryManager");
	ITT_ReallocateCallback = __itt_heap_function_create("Reallocate", "MemoryManager");
	ITT_FreeCallback = __itt_heap_function_create("Free", "MemoryManager");
#endif

	switch (g_LoadType)
	{
#if SKYRIM64
#if !SKYRIM64_CREATIONKIT_ONLY
	case GAME_EXECUTABLE_TYPE::GAME_SKYRIM:
		TLSPatcherInitialize();
		Patch_TESV();
		break;
#endif

	case GAME_EXECUTABLE_TYPE::CREATIONKIT_SKYRIM:
		Patch_TESVCreationKit();
		break;
#endif

#if FALLOUT4
	case GAME_EXECUTABLE_TYPE::CREATIONKIT_FALLOUT4:
		Patch_Fallout4CreationKit();
		break;
#endif
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
		g_LoadType = GAME_EXECUTABLE_TYPE::UNKNOWN;

		char modulePath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandle(nullptr), modulePath, MAX_PATH);

		char executableName[MAX_PATH];
		_strlwr_s(modulePath);
		_splitpath_s(modulePath, nullptr, 0, nullptr, 0, executableName, ARRAYSIZE(executableName), nullptr, 0);

		switch (CRC32(executableName))
		{
		case CRC32("skyrimselauncher"):
			g_LoadType = GAME_EXECUTABLE_TYPE::LAUNCHER_SKYRIM;
			break;

		case CRC32("skyrimse"):
		case CRC32("skyrimse_dump"):
			g_LoadType = GAME_EXECUTABLE_TYPE::GAME_SKYRIM;
			break;

		case CRC32("fallout4launcher"):
			g_LoadType = GAME_EXECUTABLE_TYPE::LAUNCHER_FALLOUT4;
			break;

		case CRC32("fallout4"):
		case CRC32("fallout4_dump"):
			g_LoadType = GAME_EXECUTABLE_TYPE::GAME_FALLOUT4;
			break;

		case CRC32("creationkit"):
			if (strstr(modulePath, "fallout 4"))
				g_LoadType = GAME_EXECUTABLE_TYPE::CREATIONKIT_FALLOUT4;
			else
				g_LoadType = GAME_EXECUTABLE_TYPE::CREATIONKIT_SKYRIM;
			break;
		}

		// For now, skip everything except the game and CK
		switch (g_LoadType)
		{
		case GAME_EXECUTABLE_TYPE::GAME_SKYRIM:
		case GAME_EXECUTABLE_TYPE::CREATIONKIT_SKYRIM:
		case GAME_EXECUTABLE_TYPE::GAME_FALLOUT4:
		case GAME_EXECUTABLE_TYPE::CREATIONKIT_FALLOUT4:
#if SKYRIM64_CREATIONKIT_ONLY
			if (g_LoadType != GAME_EXECUTABLE_TYPE::CREATIONKIT)
				return TRUE;
#endif

			DumpEnableBreakpoint();
			break;
		}
    }

#if SKYRIM64 && !SKYRIM64_CREATIONKIT_ONLY
	if (g_LoadType == GAME_EXECUTABLE_TYPE::GAME_SKYRIM)
		TLSPatcherCallback(hModule, fdwReason, lpReserved);
#endif

    return TRUE;
}
