#pragma once

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

// Intel VTune
#ifdef SKYRIM64_USE_VTUNE
#include <ittnotify.h>
#pragma comment(lib, "libittnotify.lib")
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
#include "ansel.h"
#include "profiler.h"
#include "patches/patches.h"

extern ULONG_PTR g_ModuleBase;
extern ULONG_PTR g_ModuleSize;

extern ULONG_PTR g_CodeBase;
extern ULONG_PTR g_CodeSize;

extern HMODULE g_Dll3DAudio;
extern HMODULE g_DllReshade;
extern HMODULE g_DllEnb;
extern HMODULE g_DllSKSE;
extern HMODULE g_DllVTune;

extern HMODULE g_DllDXGI;
extern HMODULE g_DllD3D11;

#define PatchIAT(detour, module, procname) Detours::IATHook((PBYTE)g_ModuleBase, (module), (procname), (PBYTE)(detour));