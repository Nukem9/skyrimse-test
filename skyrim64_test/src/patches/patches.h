#pragma once

extern HWND g_SkyrimWindow;         // window.cpp
extern IDXGISwapChain *g_SwapChain; // d3d11.cpp
extern double g_AverageFps;         // d3d11.cpp

void PatchAchievements();
void PatchD3D11();
void PatchDInput();
void PatchLogging();
void PatchSettings();
void PatchSteam();
void PatchThreading();
void PatchWindow();
void ExperimentalPatchEmptyFunctions();
void ExperimentalPatchMemInit();

// TES patches
void PatchBSGraphicsRenderTargetManager();
void PatchLocks();
void PatchBSThread();
void PatchMemory();
void PatchTESForm();