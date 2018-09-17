#pragma once

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <d3d11.h>
#include <d3d11_2.h>

// Intel VTune
#if SKYRIM64_USE_VTUNE
#include <ittnotify.h>

extern __itt_heap_function ITT_AllocateCallback;
extern __itt_heap_function ITT_ReallocateCallback;
extern __itt_heap_function ITT_FreeCallback;
#endif

// Tracy
#if SKYRIM64_USE_TRACY
#define TRACY_ENABLE
#endif
#include <tracy/Tracy.hpp>
#include <tracy/TracyD3D11.hpp>

// TBBMalloc/Jemalloc
#if SKYRIM64_USE_TBBMALLOC
#include <tbb/scalable_allocator.h>
#else
#include <jemalloc/jemalloc.h>
#endif

// Detours
#ifdef _DEBUG
#include "../detours/Detours.h"
#else
#include "../detours/Detours.h"
#endif

// ImGui
#include <imgui/imgui.h>

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