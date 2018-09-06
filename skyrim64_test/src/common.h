#pragma once

#include "config.h"

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <d3d11.h>
#include <d3d11_2.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "ws2_32.lib")

// Intel VTune
#if SKYRIM64_USE_VTUNE
#include <ittnotify.h>
#pragma comment(lib, "libittnotify.lib")

extern __itt_heap_function ITT_AllocateCallback;
extern __itt_heap_function ITT_ReallocateCallback;
extern __itt_heap_function ITT_FreeCallback;
#endif

// Tracy
#if SKYRIM64_USE_TRACY
#define TRACY_ENABLE
#define TRACY_ON_DEMAND
#include "../../tracy_lib/tracy/Tracy.hpp"
#pragma comment(lib, "tracy_lib.lib")
#else
#undef TRACY_ENABLE
#include "../../tracy_lib/tracy/Tracy.hpp"
#endif

// Jemalloc
#include <jemalloc/jemalloc.h>

// Detours
#ifdef _DEBUG
#include "../detours/Detours.h"
#pragma comment(lib, "detours/detours_debug.lib")
#else
#include "../detours/Detours.h"
#pragma comment(lib, "detours/detours.lib")
#endif

// ImGui
#include "../imgui/imgui.h"
#include "ui/ui.h"

#include "xutil.h"
#include "dump.h"
#include "profiler.h"

extern uintptr_t g_ModuleBase;
extern uintptr_t g_ModuleSize;

extern uintptr_t g_CodeBase;	// .text or .textbss
extern uintptr_t g_CodeEnd;
extern uintptr_t g_RdataBase;	// .rdata
extern uintptr_t g_RdataEnd;
extern uintptr_t g_DataBase;	// .data
extern uintptr_t g_DataEnd;

extern HMODULE g_Dll3DAudio;
extern HMODULE g_DllReshade;
extern HMODULE g_DllEnb;
extern HMODULE g_DllSKSE;
extern HMODULE g_DllVTune;
extern HMODULE g_DllDXGI;
extern HMODULE g_DllD3D11;

extern bool g_IsCreationKit;

#define PatchIAT(detour, module, procname) Detours::IATHook((PBYTE)g_ModuleBase, (module), (procname), (PBYTE)(detour));