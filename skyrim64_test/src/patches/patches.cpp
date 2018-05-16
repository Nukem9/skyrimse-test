#include "../common.h"
#include "dinput8.h"
#include "TES/BSReadWriteLock.h"
#include "TES/BGSDistantTreeBlock.h"
#include "TES/BSShader/BSShaderManager.h"
#include "TES/BSShader/Shaders/BSBloodSplatterShader.h"
#include "TES/BSShader/Shaders/BSDistantTreeShader.h"
#include "TES/BSShader/Shaders/BSSkyShader.h"
#include "TES/BSShader/Shaders/BSGrassShader.h"

void PatchAchievements();
void PatchD3D11();
void PatchLogging();
void PatchSettings();
void PatchSteam();
void PatchThreading();
void PatchWindow();
void ExperimentalPatchEmptyFunctions();
void ExperimentalPatchMemInit();

void PatchBSGraphicsRenderTargetManager();
void PatchBSThread();
void PatchMemory();
void PatchTESForm();
void TestHook5();

void Patch_TESV()
{
	PatchThreading();
	PatchWindow();
	PatchD3D11();
	PatchSteam();
	PatchAchievements();
	PatchSettings();
	PatchMemory();
	PatchTESForm();
	PatchBSThread();
	PatchBSGraphicsRenderTargetManager();
	PatchLogging();

	//
	// DirectInput (mouse, keyboard)
	//
	Detours::IATHook((PBYTE)g_ModuleBase, "dinput8.dll", "DirectInput8Create", (PBYTE)hk_DirectInput8Create);

	//
	// Locking
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06DF0), &BSReadWriteLock::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06E10), &BSReadWriteLock::LockForRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC070D0), &BSReadWriteLock::UnlockRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06E90), &BSReadWriteLock::LockForWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC070E0), &BSReadWriteLock::UnlockWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07080), &BSReadWriteLock::TryLockForWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07110), &BSReadWriteLock::IsWritingThread);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06F90), &BSReadWriteLock::LockForReadAndWrite);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07130), &BSAutoReadAndWriteLock::Initialize);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07180), &BSAutoReadAndWriteLock::Deinitialize);

	//
	// BGSDistantTreeBlock
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x4A8360), &BGSDistantTreeBlock::UpdateLODAlphaFade);

	//
	// Shaders
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12ACB20), &BSShaderManager::SetRenderMode);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12AD340), &BSShaderManager::SetCurrentAccumulator);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12AD330), &BSShaderManager::GetCurrentAccumulator);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E0DA0), &BSShaderAccumulator::hk_RegisterObjectDispatch);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E18B0), &BSShaderAccumulator::hk_FinishAccumulatingDispatch);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12EF750), &BSBloodSplatterShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1318050), &BSDistantTreeShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x13113F0), &BSSkyShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E4C10), &BSGrassShader::__ctor__);
	TestHook5();// BSLightingShader

	//
	// Misc
	//
	ExperimentalPatchEmptyFunctions();
	ExperimentalPatchMemInit();

	// Broken printf statement that triggers invalid_parameter_handler(), "%08" should really be "%08X"
	const char *newFormat = "World object count changed on object '%s' %08X from %i to %i";

	PatchMemory(g_ModuleBase + 0x1696030, (PBYTE)newFormat, strlen(newFormat) + 1);
}