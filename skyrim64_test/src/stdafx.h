#pragma once

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <intrin.h>
#include <vector>
#include <algorithm>
#include <d3d11.h>
#include <d3d11_2.h>
#include <DXGI1_3.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include "../detours/Detours.h"
#pragma comment(lib, "detours/detours.lib")

#include "../imgui/imgui.h"
#include "ui/ui.h"

#include <jemalloc/jemalloc.h>

#include "xutil.h"
#include "dump.h"
#include "ansel.h"
#include "profiler.h"

#include "patches/patches.h"
#include "patches/dinput8.h"

#include "patches/TES/BSReadWriteLock.h"
#include "patches/TES/MemoryManager.h"
#include "patches/TES/TESForm.h"
#include "patches/TES/BSThread_Win32.h"
#include "patches/TES/BSGraphicsRenderTargetManager.h"

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