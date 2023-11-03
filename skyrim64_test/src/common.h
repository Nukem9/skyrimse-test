#pragma once

#include "config.h"

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <d3d11.h>
#include <d3d11_2.h>

#include "..\..\..\..\Dependencies\memex\vmm.h"

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
#include <tracy/Tracy.hpp>
#endif

// Detours
#include <detours/Detours.h>

// ImGui
#include <imgui/imgui.h>

// INIReader
#define INI_ALLOW_MULTILINE 0
#define INI_USE_STACK 0
#define INI_MAX_LINE 4096
#include "INIReader.h"

// Unicode
#include "UtfStr.h"

#include "ui/ui.h"
#include "xutil.h"
#include "dump.h"
#include "profiler.h"
#include "patches/offsets.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

extern INIReader g_INI;
extern INIReader g_INI_ck_conf;
extern INIReader g_INI_ck_User_conf;

extern bool g_OwnArchiveLoader;
extern uint32_t g_crc32_ck;

constexpr static uint32_t CRC32_ORIGINAL_CK1573 = 0x624E8C84;
constexpr static uint32_t CRC32_ORIGINAL_CK1573_PATCHED_51 = 0xB4E5BA2A;
constexpr static uint32_t CRC32_ORIGINAL_CK1573_PATCHED_63 = 0x668F3CB3;
constexpr static uint32_t CRC32_ORIGINAL_CK16438 = 0x3FDB3994;
constexpr static uint32_t CRC32_ORIGINAL_CK16438_NOSTEAM = 0x748A3CC4;

extern uintptr_t g_hModule;
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

enum class GAME_EXECUTABLE_TYPE
{
	UNKNOWN,

	LAUNCHER_SKYRIM,
	GAME_SKYRIM,
	CREATIONKIT_SKYRIM,
};

extern GAME_EXECUTABLE_TYPE g_LoadType;
extern char g_GitVersion[64];

#define PatchIAT(detour, module, procname) Detours::IATHook(g_ModuleBase, (module), (procname), (uintptr_t)(detour));

#ifndef FIXAPI
#define FIXAPI __stdcall
#endif // FIXAPI

#ifndef UI_CUSTOM_MESSAGE
#define UI_CUSTOM_MESSAGE	52000
#endif // UI_CUSTOM_MESSAGE