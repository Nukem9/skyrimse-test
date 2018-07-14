#include "../common.h"
#include "dinput8.h"
#include "TES/TESForm.h"
#include "TES/BSReadWriteLock.h"
#include "TES/BGSDistantTreeBlock.h"
#include "TES/BSGraphicsRenderer.h"
#include "TES/BSShader/BSShaderManager.h"
#include "TES/BSShader/Shaders/BSBloodSplatterShader.h"
#include "TES/BSShader/Shaders/BSDistantTreeShader.h"
#include "TES/BSShader/Shaders/BSSkyShader.h"
#include "TES/BSShader/Shaders/BSGrassShader.h"
#include "TES/BSShader/Shaders/BSParticleShader.h"
#include "TES/NiMain/BSCullingProcess.h"

void PatchAchievements();
void PatchD3D11();
void PatchLogging();
void PatchSettings();
void PatchSteam();
void PatchThreading();
void PatchWindow();
void PatchFileIO();
void ExperimentalPatchEmptyFunctions();
void ExperimentalPatchMemInit();

void PatchBSGraphicsRenderTargetManager();
void PatchBSThread();
void PatchMemory();
void PatchTESForm();
void TestHook5();

bool doCullTest = false;

char __fastcall test1(__int64 a1, __int64(__fastcall ***a2)(__int64, __int64, __int64), unsigned int a3, unsigned int a4)
{
	char result; // al

	if (a3)
		result = (**a2)((__int64)a2, a3, a4);
	else
		result = 1;

	MOC::ForceFlush();
	doCullTest = true;
	return result;
}

void test2()
{
	doCullTest = false;
}

void Patch_TESV()
{
	PatchThreading();
	PatchWindow();
	PatchD3D11();
	PatchSteam();
	PatchAchievements();
	PatchSettings();
	PatchMemory();
	PatchFileIO();
	PatchTESForm();
	PatchBSThread();
	PatchBSGraphicsRenderTargetManager();
	PatchLogging();

	//
	// BGSDistantTreeBlock
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x4A8360), &BGSDistantTreeBlock::UpdateLODAlphaFade);

	//
	// BSCullingProcess
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xD50310), &BSCullingProcess::hk_Process);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12F93B1), &test1, Detours::X64Option::USE_REL32_JUMP);
	PatchMemory(g_ModuleBase + 0x12F93B1, (PBYTE)"\xE8", 1);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12F96C9), &test2, Detours::X64Option::USE_REL32_JUMP);
	PatchMemory(g_ModuleBase + 0x12F96C9, (PBYTE)"\xE8", 1);

	//
	// BSGraphicsRenderer
	//
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::TriShape *)>((PBYTE)(g_ModuleBase + 0x133EC20), &BSGraphics::Renderer::IncRef);
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::TriShape *)>((PBYTE)(g_ModuleBase + 0xD6B9B0), &BSGraphics::Renderer::DecRef);
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::DynamicTriShape *)>((PBYTE)(g_ModuleBase + 0x133ED50), &BSGraphics::Renderer::IncRef);
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::DynamicTriShape *)>((PBYTE)(g_ModuleBase + 0xD6C7D0), &BSGraphics::Renderer::DecRef);

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
	// NiRTTI
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC66F80), &NiRTTI::__ctor__);

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
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1336C80), &BSParticleShader::__ctor__);
	TestHook5();// BSLightingShader

	//
	// Temporary hack to fix array overflow in BSParticleShader::SetupGeometry
	//
	uint32_t test = 0x2000;
	PatchMemory(g_ModuleBase + 0x02493CC + 1, (PBYTE)&test, sizeof(uint32_t));
	PatchMemory(g_ModuleBase + 0x0249374 + 1, (PBYTE)&test, sizeof(uint32_t));

	test = 100;
	PatchMemory(g_ModuleBase + 0x02494A8 + 2, (PBYTE)&test, sizeof(uint32_t));

	//
	// Misc
	//
	ExperimentalPatchEmptyFunctions();
	ExperimentalPatchMemInit();

	// Broken printf statement that triggers invalid_parameter_handler(), "%08" should really be "%08X"
	const char *newFormat = "World object count changed on object '%s' %08X from %i to %i";

	PatchMemory(g_ModuleBase + 0x1696030, (PBYTE)newFormat, strlen(newFormat) + 1);

	//
	// Bug fix for when TAA/FXAA/DOF are disabled and quicksave doesn't work without
	// opening a menu
	//
	PatchMemory(g_ModuleBase + 0x12AE2DD, (PBYTE)"\x90\x90\x90\x90\x90\x90", 6);	// Kill jnz
	PatchMemory(g_ModuleBase + 0x12AE2E8, (PBYTE)"\x80\xBD\x11\x02\x00\x00\x00", 7);// Rewrite to 'cmp byte ptr [rbp+211h], 0'
	PatchMemory(g_ModuleBase + 0x12AE2EF, (PBYTE)"\x90\x90\x90\x90", 4);			// Extra rewrite padding

	*(PBYTE *)&TESObjectCell::CreateRootMultiBoundNode = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x264230), &TESObjectCell::hk_CreateRootMultiBoundNode);
}

void Patch_TESVCreationKit()
{
	PatchThreading();
	PatchWindow();
	PatchSteam();
	PatchFileIO();
}