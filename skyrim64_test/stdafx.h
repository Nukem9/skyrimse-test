#pragma once

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <intrin.h>
#include <vector>
#include <algorithm>
#include <DirectXMath.h>
#include <d3d11.h>
#include <d3d11_2.h>
#include <DXGI1_3.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include "detours/Detours.h"
#pragma comment(lib, "detours/detours.lib")

#include "util.h"
#include "dump.h"
#include "d3dconv.h"
#include "d3d11.h"
#include "dinput8.h"
#include "window.h"
#include "threading.h"
#include "achievements.h"

extern ULONG_PTR g_ModuleBase;
extern ULONG_PTR g_ModuleSize;

extern ULONG_PTR g_CodeBase;
extern ULONG_PTR g_CodeSize;

#define PatchIAT(detour, module, dll) Detours::X64::DetourIAT((PBYTE)g_ModuleBase, (PBYTE)(detour), module, dll);