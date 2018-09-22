#include <xbyak/xbyak.h>
#include "../typeinfo/ms_rtti.h"
#include "../common.h"
#include "dinput8.h"
#include "TES/TESForm.h"
#include "TES/BSReadWriteLock.h"
#include "TES/BGSDistantTreeBlock.h"
#include "TES/BSGraphicsRenderer.h"
#include "TES/BSCullingProcess.h"
#include "TES/BSJobs.h"
#include "TES/BSTaskManager.h"
#include "TES/BSShader/BSShaderManager.h"
#include "TES/BSShader/Shaders/BSBloodSplatterShader.h"
#include "TES/BSShader/Shaders/BSDistantTreeShader.h"
#include "TES/BSShader/Shaders/BSSkyShader.h"
#include "TES/BSShader/Shaders/BSGrassShader.h"
#include "TES/BSShader/Shaders/BSParticleShader.h"
#include "TES/MemoryManager.h"
#include "TES/NavMesh.h"

#define INI_ALLOW_MULTILINE 0
#define INI_USE_STACK 0
#define INI_MAX_LINE 4096
#include "INIReader.h"

INIReader INI("skyrim64_test.ini");

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
void ExperimentalPatchEditAndContinue();

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
	MSRTTI::Initialize();
	PatchThreading();
	PatchWindow();
	PatchD3D11();
	PatchSteam();
	PatchAchievements();
	PatchSettings();
	PatchMemory();
	//PatchFileIO();
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
	// BSJobs
	//
	class jobhook : public Xbyak::CodeGenerator
	{
	public:
		jobhook() : Xbyak::CodeGenerator()
		{
			mov(rcx, qword[rax + rdx * 8 + 8]);
			mov(rdx, qword[rax + rdx * 8]);
			mov(r8, (uintptr_t)&BSJobs::DispatchJobCallback);
			call(r8);

			jmp(ptr[rip]);
			dq(g_ModuleBase + 0xC32111);
		}
	} static jobhookInstance;

	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xC32109), (PBYTE)jobhookInstance.getCode());

	//
	// BSTaskManager
	//
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class IOManager")->VTableAddress, &IOManager::QueueTask, 17);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class AddCellGrassTask")->VTableAddress, &BSTask::GetName_AddCellGrassTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class AttachDistant3DTask")->VTableAddress, &BSTask::GetName_AttachDistant3DTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class AudioLoadForPlaybackTask")->VTableAddress, &BSTask::GetName_AudioLoadForPlaybackTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class AudioLoadToCacheTask")->VTableAddress, &BSTask::GetName_AudioLoadToCacheTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class BGSParticleObjectCloneTask")->VTableAddress, &BSTask::GetName_BGSParticleObjectCloneTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class BSScaleformMovieLoadTask")->VTableAddress, &BSTask::GetName_BSScaleformMovieLoadTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class CheckWithinMultiBoundTask")->VTableAddress, &BSTask::GetName_CheckWithinMultiBoundTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class QueuedFile")->VTableAddress, &BSTask::GetName_QueuedFile, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class QueuedPromoteLocationReferencesTask")->VTableAddress, &BSTask::GetName_QueuedPromoteLocationReferencesTask, 4);
	Detours::X64::DetourClassVTable((uint8_t *)MSRTTI::Find("class QueuedPromoteReferencesTask")->VTableAddress, &BSTask::GetName_QueuedPromoteReferencesTask, 4);

	//
	// DirectInput (mouse, keyboard)
	//
	Detours::IATHook((PBYTE)g_ModuleBase, "dinput8.dll", "DirectInput8Create", (PBYTE)hk_DirectInput8Create);

	//
	// MemoryManager
	//
	PatchMemory(g_ModuleBase + 0x59B560, (PBYTE)"\xC3", 1);// [3GB  ] MemoryManager - Default/Static/File heaps
	PatchMemory(g_ModuleBase + 0x59B170, (PBYTE)"\xC3", 1);// [1GB  ] BSSmallBlockAllocator
														   // [512MB] hkMemoryAllocator is untouched due to complexity
														   // [128MB] BSScaleformSysMemMapper is untouched due to complexity
	PatchMemory(g_ModuleBase + 0xC02E60, (PBYTE)"\xC3", 1);// [64MB ] ScrapHeap init
	PatchMemory(g_ModuleBase + 0xC037C0, (PBYTE)"\xC3", 1);// [64MB ] ScrapHeap deinit

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC01DA0), &MemoryManager::Alloc);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC020A0), &MemoryManager::Free);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC02FE0), &ScrapHeap::Alloc);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC03600), &ScrapHeap::Free);

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

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1336860), &BSShader::BeginTechnique);
	*(uint8_t **)&BSShader::Load = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x13364A0), &BSShader::hk_Load);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12EF750), &BSBloodSplatterShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1318050), &BSDistantTreeShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x13113F0), &BSSkyShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E4C10), &BSGrassShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1336C80), &BSParticleShader::__ctor__);
	TestHook5();// BSLightingShader

	//
	// TESForm (Note: The function itself needs to be hooked, not the vtable)
	//
	Detours::X64::DetourFunctionClass(*(uint8_t **)(MSRTTI::Find("class TESForm")->VTableAddress + 0xB0), &TESForm::hk_GetFullTypeName);
	Detours::X64::DetourFunctionClass(*(uint8_t **)(MSRTTI::Find("class TESForm")->VTableAddress + 0x190), &TESForm::hk_GetName);
	Detours::X64::DetourFunctionClass(*(uint8_t **)(MSRTTI::Find("class TESForm")->VTableAddress + 0x198), &TESForm::hk_SetEditorId);

	for (auto ref : MSRTTI::FindAll("class TESObjectREFR"))
	{
		// TESObjectREFR has multiple virtual tables
		if (ref->VFunctionCount == 162)
			Detours::X64::DetourFunctionClass(*(uint8_t **)(ref->VTableAddress + 0x190), &TESObjectREFR::hk_GetName);
	}

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
	//ExperimentalPatchEmptyFunctions();
	ExperimentalPatchMemInit();

	// Broken printf statement that triggers invalid_parameter_handler(), "%08" should really be "%08X"
	const char *newFormat = "World object count changed on object '%s' %08X from %i to %i";

	PatchMemory(g_ModuleBase + 0x1696030, (PBYTE)newFormat, strlen(newFormat) + 1);

	//
	// Bug fix for when TAA/FXAA/DOF are disabled and quicksave doesn't work without
	// opening a menu
	//
	//PatchMemory(g_ModuleBase + 0x12AE2DD, (PBYTE)"\x90\x90\x90\x90\x90\x90", 6);	// Kill jnz
	//PatchMemory(g_ModuleBase + 0x12AE2E8, (PBYTE)"\x80\xBD\x11\x02\x00\x00\x00", 7);// Rewrite to 'cmp byte ptr [rbp+211h], 0'
	//PatchMemory(g_ModuleBase + 0x12AE2EF, (PBYTE)"\x90\x90\x90\x90", 4);			// Extra rewrite padding

	//
	// Memory bug fix during BSShadowDirectionalLight calculations:
	//
	// void *data1 = ScrapHeap::Alloc(32, 8);
	// MessWithData(v40, 8u, v41, &v217, data1);
	// sub_14133E730(..., data1, ...); <- OK
	// ScrapHeap::Free(data1);
	// ....
	// void *data2 = ScrapHeap::Alloc(32, 8);
	// MessWithData(v40, 8u, v41, &v217, data2);
	// sub_14133E730(..., data1, ...); <- USE-AFTER-FREE!! data1 SHOULD BE data2
	// ScrapHeap::Free(data2);
	//
	PatchMemory(g_ModuleBase + 0x133D94D, (PBYTE)"\x4D\x8B\xCF\x90\x90\x90\x90", 7);

	*(PBYTE *)&TESObjectCell::CreateRootMultiBoundNode = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x264230), &TESObjectCell::hk_CreateRootMultiBoundNode);

}

void Patch_TESVCreationKit()
{
	if (_stricmp((const char *)(g_ModuleBase + 0x3078988), "1.5.3.0") != 0)
	{
		MessageBoxA(nullptr, "Incorrect CreationKit version detected. Patches disabled.", "Version Check", MB_ICONERROR);
		return;
	}

	MSRTTI::Initialize();

	if (INI.GetBoolean("CreationKit", "ThreadingPatch", false))	PatchThreading();
	if (INI.GetBoolean("CreationKit", "WindowPatch", false))	PatchWindow();
	if (INI.GetBoolean("CreationKit", "IOPatch", false))		PatchFileIO();
	if (INI.GetBoolean("CreationKit", "SteamPatch", false))		PatchSteam();

	//
	// Experimental
	//
	if (INI.GetBoolean("CreationKit", "ExperimentalOptimization", false))
	{
		ExperimentalPatchEditAndContinue();
		ExperimentalPatchMemInit();
	}

	//
	// MemoryManager
	//
	if (INI.GetBoolean("CreationKit", "MemoryPatch", false))
	{
		PatchMemory();

		PatchMemory(g_ModuleBase + 0x1223160, (PBYTE)"\xC3", 1);// [3GB  ] MemoryManager - Default/Static/File heaps
		PatchMemory(g_ModuleBase + 0x24400E0, (PBYTE)"\xC3", 1);// [1GB  ] BSSmallBlockAllocator
																// [512MB] hkMemoryAllocator is untouched due to complexity
																// [128MB] BSScaleformSysMemMapper is untouched due to complexity
		PatchMemory(g_ModuleBase + 0x2447D90, (PBYTE)"\xC3", 1);// [64MB ] ScrapHeap init
		PatchMemory(g_ModuleBase + 0x24488C0, (PBYTE)"\xC3", 1);// [64MB ] ScrapHeap deinit

		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x2440380), &MemoryManager::Alloc);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x24407A0), &MemoryManager::Free);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x2447FA0), &ScrapHeap::Alloc);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x24485F0), &ScrapHeap::Free);
	}

	//
	// NiRTTI
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x269AD20), &NiRTTI::__ctor__);

	//
	// NavMesh
	//
	if (INI.GetBoolean("CreationKit", "NavMeshPseudoDelete", false))
	{
		*(uint8_t **)&NavMesh::DeleteTriangle = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1D618E0), &NavMesh::hk_DeleteTriangle);

		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1D6984F), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1D699E6), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1D69B80), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);

		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1D6A42B), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1D6A580), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);

		PatchMemory(g_ModuleBase + 0x1D6A42B, (PBYTE)"\xE8", 1);
		PatchMemory(g_ModuleBase + 0x1D6A580, (PBYTE)"\xE8", 1);

		PatchMemory(g_ModuleBase + 0x1D6984F, (PBYTE)"\xE8", 1);
		PatchMemory(g_ModuleBase + 0x1D699E6, (PBYTE)"\xE8", 1);
		PatchMemory(g_ModuleBase + 0x1D69B80, (PBYTE)"\xE8", 1);

		PatchMemory(g_ModuleBase + 0x1FF9BAC, (PBYTE)"\xE9\xA1\x01\x00\x00", 5);// Prevent vertices from being deleted separately
	}

	//
	// Allow saving ESM's directly. "File '%s' is a master file or is in use.\n\nPlease select another file to save to."
	//
	if (INI.GetBoolean("CreationKit", "AllowSaveESM", false))
	{
		const char *newFormat = "File '%s' is in use.\n\nPlease select another file to save to.";

		PatchMemory(g_ModuleBase + 0x164020A, (PBYTE)"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);
		PatchMemory(g_ModuleBase + 0x30B9090, (PBYTE)newFormat, strlen(newFormat) + 1);
	}

	//
	// Allow ESP files to act as master files while saving
	//
	if (INI.GetBoolean("CreationKit", "AllowMasterESP", false))
	{
		PatchMemory(g_ModuleBase + 0x1657279, (PBYTE)"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);
	}

	//
	// Memory bug fix during BSShadowDirectionalLight calculations (see game patch for more information)
	//
	PatchMemory(g_ModuleBase + 0x2DC679D, (PBYTE)"\x4D\x89\xE1\x90\x90\x90\x90", 7);

	//
	// Skip 'Topic Info' validation during load
	//
	if (INI.GetBoolean("CreationKit", "SkipTopicInfoValidation", false))
	{
		PatchMemory(g_ModuleBase + 0x19A83C0, (PBYTE)"\xC3", 1);
	}

	//
	// Remove assertion message boxes
	//
	if (INI.GetBoolean("CreationKit", "DisableAssertions", false))
	{
		PatchMemory(g_ModuleBase + 0x243D9FE, (PBYTE)"\x90\x90\x90\x90\x90", 5);
	}

	// TEMP: Kill broken destructor causing double free
	PatchMemory(g_ModuleBase + 0x1392D90, (PBYTE)"\x90\x90\x90\x90\x90\x90", 6);
}