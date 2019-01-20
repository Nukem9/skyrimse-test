#include "common.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "ws2_32.lib")

#pragma comment(lib, "tbb.lib")					// Thread Building Blocks
#pragma comment(lib, "libzydis.lib")			// Zydis
#pragma comment(lib, "libmoc.lib")				// MaskedOcclusionCulling
#pragma comment(lib, "libmeshoptimizer.lib")	// Meshoptimizer
#pragma comment(lib, "libimgui.lib")			// ImGui

#if SKYRIM64_USE_VTUNE
#pragma comment(lib, "libittnotify.lib")		// Intel Threading Tools

__itt_heap_function ITT_AllocateCallback;
__itt_heap_function ITT_ReallocateCallback;
__itt_heap_function ITT_FreeCallback;
#endif

#pragma comment(lib, "tbbmalloc.lib")			// TBB Allocator

#if SKYRIM64_USE_TRACY
#pragma comment(lib, "libtracy.lib")			// Tracy
#endif

#ifdef _DEBUG
#pragma comment(lib, "detours/detours_debug.lib")
#else
#pragma comment(lib, "detours/detours.lib")
#endif

INIReader g_INI("skyrim64_test.ini");

uintptr_t g_ModuleBase;
uintptr_t g_ModuleSize;

uintptr_t g_CodeBase;	// .text or .textbss
uintptr_t g_CodeEnd;
uintptr_t g_RdataBase;	// .rdata
uintptr_t g_RdataEnd;
uintptr_t g_DataBase;	// .data
uintptr_t g_DataEnd;

HMODULE g_Dll3DAudio;
HMODULE g_DllReshade;
HMODULE g_DllEnb;
HMODULE g_DllSKSE;
HMODULE g_DllVTune;
HMODULE g_DllDXGI;
HMODULE g_DllD3D11;

GAME_EXECUTABLE_TYPE g_LoadType;

char g_GitVersion[64];