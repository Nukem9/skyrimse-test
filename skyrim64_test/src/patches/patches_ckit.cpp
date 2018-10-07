#include <xbyak/xbyak.h>
#include "../typeinfo/ms_rtti.h"
#include "../common.h"
#include "TES/MemoryManager.h"
#include "TES/NiMain/NiRTTI.h"
#include "CKIT/Editor.h"
#include "CKIT/TESForm_CK.h"
#include "CKIT/NavMesh.h"
#include "CKIT/EditorUI.h"

#define INI_ALLOW_MULTILINE 0
#define INI_USE_STACK 0
#define INI_MAX_LINE 4096
#include "INIReader.h"

INIReader INI("skyrim64_test.ini");

void PatchSteam();
void PatchThreading();
void PatchFileIO();
void ExperimentalPatchMemInit();
void ExperimentalPatchEditAndContinue();
void PatchMemory();

extern WNDPROC OldEditorUI_WndProc;

void Patch_TESVCreationKit()
{
	if (_stricmp((const char *)(g_ModuleBase + 0x3078988), "1.5.3.0") != 0)
	{
		MessageBoxA(nullptr, "Incorrect CreationKit version detected. Patches disabled.", "Version Check", MB_ICONERROR);
		return;
	}

	MSRTTI::Initialize();

	if (INI.GetBoolean("CreationKit", "ThreadingPatch", false))	PatchThreading();
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
	// FaceGen
	//
	if (INI.GetBoolean("CreationKit", "DisableAutoFacegen", false))
	{
		// Disable automatic FaceGen on save
		PatchMemory(g_ModuleBase + 0x18DE530, (PBYTE)"\xC3", 1);
	}

	// Allow variable tint mask resolution
	uint32_t tintResolution = INI.GetInteger("CreationKit", "TintMaskResolution", 512);
	PatchMemory(g_ModuleBase + 0x2DA588C, (PBYTE)&tintResolution, sizeof(uint32_t));
	PatchMemory(g_ModuleBase + 0x2DA5899, (PBYTE)&tintResolution, sizeof(uint32_t));

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
	// TESForm
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x16C0650), &FormReferenceMap_RemoveAllEntries);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x16C0A90), &FormReferenceMap_FindOrCreate);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x16C0B50), &FormReferenceMap_RemoveEntry);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x146C130), &FormReferenceMap_Get);

	//
	// UI
	//
	if (INI.GetBoolean("CreationKit", "UI", false))
	{
		EditorUI_Initialize();
		*(PBYTE *)&OldEditorUI_WndProc = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x13F3770), &EditorUI_WndProc);

		PatchMemory(g_ModuleBase + 0x1487B69, (PBYTE)"\x90\x90", 2);// Enable push to game button even if version control is disabled
		PatchMemory(g_ModuleBase + 0x1487B7C, (PBYTE)"\xEB", 1);

		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1256600), &EditorUI_Warning);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x243D610), &EditorUI_Warning);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1CD29E0), &EditorUI_Warning);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x122C5F0), &EditorUI_WarningUnknown1);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x137FC60), &EditorUI_WarningUnknown1);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1FCB030), &EditorUI_WarningUnknown1);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x27A6150), &EditorUI_WarningUnknown2);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x27A6270), &EditorUI_WarningUnknown2);
		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x243D260), &EditorUI_Assert);
	}

	//
	// Windows
	//
	if (INI.GetBoolean("CreationKit", "WindowPatch", false))
	{
		PatchIAT(hk_CreateDialogParamA, "USER32.DLL", "CreateDialogParamA");
		PatchIAT(hk_DialogBoxParamA, "USER32.DLL", "DialogBoxParamA");
		PatchIAT(hk_EndDialog, "USER32.DLL", "EndDialog");
		PatchIAT(hk_SendMessageA, "USER32.DLL", "SendMessageA");
	}

	//
	// Allow saving ESM's directly
	//
	if (INI.GetBoolean("CreationKit", "AllowSaveESM", false))
	{
		// Disable: "File '%s' is a master file or is in use.\n\nPlease select another file to save to."
		const char *newFormat = "File '%s' is in use.\n\nPlease select another file to save to.";

		PatchMemory(g_ModuleBase + 0x164020A, (PBYTE)"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);
		PatchMemory(g_ModuleBase + 0x30B9090, (PBYTE)newFormat, strlen(newFormat) + 1);

		// Also allow ESM's to be set as "Active File"
		PatchMemory(g_ModuleBase + 0x13E2D45, (PBYTE)"\x90\x90\x90\x90\x90", 5);

		Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1482DA0), &OpenPluginSaveDialog);
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

	// Force render window to draw at 60fps (SetTimer(1ms))
	if (INI.GetBoolean("CreationKit", "RenderWindow60FPS", false))
	{
		PatchMemory(g_ModuleBase + 0x1306978, (PBYTE)"\x01", 1);
	}

	// TEMP: Kill broken destructor causing double free
	PatchMemory(g_ModuleBase + 0x1392D90, (PBYTE)"\x90\x90\x90\x90\x90\x90", 6);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x166BB1E), &hk_inflateInit);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x166BBB9), &hk_inflate);

	PatchMemory(g_ModuleBase + 0x166BB1E, (PBYTE)"\xE8", 1);
	PatchMemory(g_ModuleBase + 0x166BBB9, (PBYTE)"\xE8", 1);
}