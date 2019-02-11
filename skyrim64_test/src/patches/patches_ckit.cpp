#include <xbyak/xbyak.h>
#include <intrin.h>
#include "../typeinfo/ms_rtti.h"
#include "../common.h"
#include "TES/MemoryManager.h"
#include "TES/bhkThreadMemorySource.h"
#include "TES/NiMain/NiRTTI.h"
#include "CKIT/Editor.h"
#include "CKIT/TESForm_CK.h"
#include "CKIT/NavMesh.h"
#include "CKIT/EditorUI.h"
#include "CKIT/BSPointerHandle.h"

void PatchSteam();
void PatchThreading();
void PatchFileIO();
void ExperimentalPatchMemInit();
void ExperimentalPatchEditAndContinue();
void PatchMemory();
size_t BNetConvertUnicodeString(char *Destination, size_t DestSize, const wchar_t *Source, size_t SourceSize);

extern WNDPROC OldEditorUI_WndProc;

void Patch_TESVCreationKit()
{
	if (_stricmp((const char *)(g_ModuleBase + 0x3078988), "1.5.3.0") != 0)
	{
		char modulePath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandle(nullptr), modulePath, ARRAYSIZE(modulePath));

		char message[1024];
		sprintf_s(message, "Incorrect CreationKit version detected. Version 1.5.3.0 from 2018-04-13 is required. Patches are disabled.\n\nExecutable path: %s", modulePath);

		MessageBoxA(nullptr, message, "Version Check", MB_ICONERROR);
		return;
	}

	//
	// Scare people who are still using the old d3d9.dll
	//
	if (FILE *f; fopen_s(&f, "d3d9.dll", "rb") == 0)
	{
		fseek(f, 0, SEEK_END);
		uint32_t len = ftell(f);
		rewind(f);

		uint8_t *data = new uint8_t[len];
		fread(data, sizeof(uint8_t), len, f);

		const char *pattern = "SkyrimSETest\\x64\\Release\\d3d9.pdb";
		const char *mask = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		uintptr_t found = XUtil::FindPattern((uintptr_t)data, len, (uint8_t *)pattern, mask);

		AssertMsg(!found, "An old version of CKFixes has been detected in your Creation Kit directory. DELETE \"d3d9.dll\" BEFORE RUNNING WITH THIS VERSION.");

		delete[] data;
		fclose(f);
	}

	//
	// Replace broken crash dump functionality
	//
	if (g_INI.GetBoolean("CreationKit", "GenerateCrashdumps", true))
	{
		SetUnhandledExceptionFilter(DumpExceptionHandler);

		XUtil::PatchMemory(g_ModuleBase + 0x247D650, (PBYTE)"\xC3", 1);						// StackTrace::MemoryTraceWrite
		XUtil::PatchMemory(g_ModuleBase + 0x24801FB, (PBYTE)"\x90\x90\x90\x90\x90\x90", 6);	// SetUnhandledExceptionFilter, BSWin32ExceptionHandler
		XUtil::PatchMemory(g_ModuleBase + 0x24801DF, (PBYTE)"\xC3", 1);						// SetUnhandledExceptionFilter, BSWin32ExceptionHandler
		XUtil::PatchMemory(g_ModuleBase + 0x2E558DB, (PBYTE)"\xC3", 1);						// SetUnhandledExceptionFilter, Unknown

		_set_invalid_parameter_handler([](const wchar_t *, const wchar_t *, const wchar_t *, uint32_t, uintptr_t)
		{
			RaiseException('PARM', EXCEPTION_NONCONTINUABLE, 0, nullptr);
		});

		auto purecallHandler = []()
		{
			RaiseException('PURE', EXCEPTION_NONCONTINUABLE, 0, nullptr);
		};

		auto terminateHandler = []()
		{
			RaiseException('TERM', EXCEPTION_NONCONTINUABLE, 0, nullptr);
		};

		PatchIAT((void(*)())terminateHandler, "API-MS-WIN-CRT-RUNTIME-L1-1-0.DLL", "_cexit");
		PatchIAT((void(*)())terminateHandler, "API-MS-WIN-CRT-RUNTIME-L1-1-0.DLL", "_exit");
		PatchIAT((void(*)())terminateHandler, "API-MS-WIN-CRT-RUNTIME-L1-1-0.DLL", "_c_exit");
		PatchIAT((void(*)())terminateHandler, "API-MS-WIN-CRT-RUNTIME-L1-1-0.DLL", "exit");
		PatchIAT((void(*)())terminateHandler, "API-MS-WIN-CRT-RUNTIME-L1-1-0.DLL", "abort");
		PatchIAT((void(*)())terminateHandler, "API-MS-WIN-CRT-RUNTIME-L1-1-0.DLL", "terminate");
		PatchIAT((void(*)())purecallHandler, "VCRUNTIME140.DLL", "_purecall");
	}

	MSRTTI::Initialize();

	if (g_INI.GetBoolean("CreationKit", "ThreadingPatch", false))	PatchThreading();
	if (g_INI.GetBoolean("CreationKit", "IOPatch", false))			PatchFileIO();
	if (g_INI.GetBoolean("CreationKit", "SteamPatch", false))		PatchSteam();

	//
	// Experimental
	//
	ExperimentalPatchEditAndContinue();
	ExperimentalPatchMemInit();

	//
	// BSPointerHandle(Manager)
	//
	if (g_INI.GetBoolean("CreationKit", "RefrHandleLimitPatch", false))
	{
		XUtil::DetourJump(g_ModuleBase + 0x141A5C0, &BSPointerHandleManagerInterface::Initialize);
		XUtil::DetourJump(g_ModuleBase + 0x12E2260, &BSPointerHandleManagerInterface::GetCurrentHandle);
		XUtil::DetourJump(g_ModuleBase + 0x12E1BE0, &BSPointerHandleManagerInterface::CreateHandle);
		XUtil::DetourJump(g_ModuleBase + 0x1291050, &BSPointerHandleManagerInterface::ReleaseHandle);
		XUtil::DetourJump(g_ModuleBase + 0x12E1F70, &BSPointerHandleManagerInterface::ReleaseHandleAndClear);
		XUtil::DetourJump(g_ModuleBase + 0x1770560, &BSPointerHandleManagerInterface::CheckForLeaks);
		XUtil::DetourJump(g_ModuleBase + 0x1770910, &BSPointerHandleManagerInterface::ClearActiveHandles);
		XUtil::DetourJump(g_ModuleBase + 0x1293870, &BSPointerHandleManagerInterface::sub_141293870);
		XUtil::DetourJump(g_ModuleBase + 0x12E25B0, &BSPointerHandleManagerInterface::sub_1412E25B0);
		XUtil::DetourJump(g_ModuleBase + 0x14C52B0, &BSPointerHandleManagerInterface::sub_1414C52B0);

		//
		// Stub out the rest of the functions which shouldn't ever be called now
		//
		//XUtil::PatchMemory(g_ModuleBase + 0x12E0DC0, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::BSUntypedPointerHandle - 1412E0DC0
		XUtil::PatchMemory(g_ModuleBase + 0x12E38A0, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::Clear - 1412E38A0
		XUtil::PatchMemory(g_ModuleBase + 0x12E2720, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::SetAge - 1412E2720
		XUtil::PatchMemory(g_ModuleBase + 0x12E3970, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::SetActive - 1412E3970
		XUtil::PatchMemory(g_ModuleBase + 0x1294740, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetAge_0 - 141294740
		XUtil::PatchMemory(g_ModuleBase + 0x12E3810, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::Set - 1412E3810
		XUtil::PatchMemory(g_ModuleBase + 0x12E2FF0, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetIndex_0 - 1412E2FF0
		XUtil::PatchMemory(g_ModuleBase + 0x1294A30, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetIndex - 141294A30
		XUtil::PatchMemory(g_ModuleBase + 0x1294720, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetAge - 141294720
		XUtil::PatchMemory(g_ModuleBase + 0x1297430, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::ClearActive - 141297430
		XUtil::PatchMemory(g_ModuleBase + 0x12973F0, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::SetIndex - 1412973F0
		XUtil::PatchMemory(g_ModuleBase + 0x12943B0, (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::IsEmpty - 1412943B0
		// sub_14100B0A8 - Unknown operator
		// sub_1412E1300 - Unknown operator
		// sub_1412E1210 - Unknown operator

		XUtil::PatchMemory(g_ModuleBase + 0x1294590, (PBYTE)"\xCC", 1);// BSPointerHandle::AgeMatches - 141294590
		XUtil::PatchMemory(g_ModuleBase + 0x128D130, (PBYTE)"\xCC", 1);// BSPointerHandle::GetPtr - 14128D130
		XUtil::PatchMemory(g_ModuleBase + 0x128C8D0, (PBYTE)"\xCC", 1);// BSPointerHandle::AssignPtr - 14128C8D0
		XUtil::PatchMemory(g_ModuleBase + 0x1294570, (PBYTE)"\xCC", 1);// BSPointerHandle::IsActive - 141294570

		XUtil::PatchMemory(g_ModuleBase + 0x12E3900, (PBYTE)"\xCC", 1);// BSHandleRefObject::AssignHandleIndex - 1412E3900
		XUtil::PatchMemory(g_ModuleBase + 0x12949D0, (PBYTE)"\xCC", 1);// BSHandleRefObject::GetIndex - 1412949D0
		// BSHandleRefObject::GetRefCount - 141294CB0
	}

	//
	// FaceGen
	//

	// Disable automatic FaceGen on save
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableAutoFaceGen", false))
		XUtil::PatchMemory(g_ModuleBase + 0x18DE530, (PBYTE)"\xC3", 1);

	// Don't produce DDS files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportDDS", false))
		XUtil::PatchMemory(g_ModuleBase + 0x1904318, (PBYTE)"\x90\x90\x90\x90\x90", 5);

	// Don't produce TGA files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportTGA", false))
		XUtil::PatchMemory(g_ModuleBase + 0x190436B, (PBYTE)"\x90\x90\x90\x90\x90", 5);

	// Don't produce NIF files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportNIF", false))
		XUtil::PatchMemory(g_ModuleBase + 0x1904390, (PBYTE)"\xC3", 1);

	// Allow variable tint mask resolution
	uint32_t tintResolution = g_INI.GetInteger("CreationKit_FaceGen", "TintMaskResolution", 512);
	XUtil::PatchMemory(g_ModuleBase + 0x2DA588C, (PBYTE)&tintResolution, sizeof(uint32_t));
	XUtil::PatchMemory(g_ModuleBase + 0x2DA5899, (PBYTE)&tintResolution, sizeof(uint32_t));

	if (g_INI.GetBoolean("CreationKit", "FaceGenBatchSpeedup", false))
		XUtil::DetourJump(g_ModuleBase + 0x12D1AC0, &ExportFaceGenForSelectedNPCs);

	//
	// LipGen
	//
	XUtil::DetourJump(g_ModuleBase + 0x13C4C80, &IsLipDataPresent);
	XUtil::DetourJump(g_ModuleBase + 0x1791240, &WriteLipData);
	XUtil::DetourCall(g_ModuleBase + 0x13D5443, &IsWavDataPresent);
	XUtil::DetourJump(g_ModuleBase + 0x13D29B0, &LipRecordDialogProc);

	//
	// MemoryManager
	//
	if (g_INI.GetBoolean("CreationKit", "MemoryPatch", false))
	{
		PatchMemory();

		XUtil::PatchMemory(g_ModuleBase + 0x1223160, (PBYTE)"\xC3", 1);					// [3GB  ] MemoryManager - Default/Static/File heaps
		XUtil::PatchMemory(g_ModuleBase + 0x24400E0, (PBYTE)"\xC3", 1);					// [1GB  ] BSSmallBlockAllocator
		XUtil::DetourJump(g_ModuleBase + 0x257D740, &bhkThreadMemorySource::__ctor__);	// [512MB] bhkThreadMemorySource
		XUtil::PatchMemory(g_ModuleBase + 0x2447D90, (PBYTE)"\xC3", 1);					// [64MB ] ScrapHeap init
		XUtil::PatchMemory(g_ModuleBase + 0x24488C0, (PBYTE)"\xC3", 1);					// [64MB ] ScrapHeap deinit
																						// [128MB] BSScaleformSysMemMapper is untouched due to complexity

		XUtil::DetourJump(g_ModuleBase + 0x2440380, &MemoryManager::Alloc);
		XUtil::DetourJump(g_ModuleBase + 0x24407A0, &MemoryManager::Free);
		XUtil::DetourJump(g_ModuleBase + 0x2447FA0, &ScrapHeap::Alloc);
		XUtil::DetourJump(g_ModuleBase + 0x24485F0, &ScrapHeap::Free);
	}

	//
	// NiRTTI
	//
	XUtil::DetourJump(g_ModuleBase + 0x269AD20, &NiRTTI::__ctor__);

	//
	// NavMesh
	//
	if (g_INI.GetBoolean("CreationKit", "NavMeshPseudoDelete", false))
	{
		*(uint8_t **)&NavMesh::DeleteTriangle = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1D618E0), &NavMesh::hk_DeleteTriangle);

		XUtil::DetourCall(g_ModuleBase + 0x1D6984F, &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		XUtil::DetourCall(g_ModuleBase + 0x1D699E6, &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		XUtil::DetourCall(g_ModuleBase + 0x1D69B80, &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);

		XUtil::DetourCall(g_ModuleBase + 0x1D6A42B, &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);
		XUtil::DetourCall(g_ModuleBase + 0x1D6A580, &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);

		XUtil::PatchMemory(g_ModuleBase + 0x1FF9BAC, (PBYTE)"\xE9\xA1\x01\x00\x00", 5);// Prevent vertices from being deleted separately
	}

	//
	// TESForm
	//
	XUtil::DetourJump(g_ModuleBase + 0x16C0650, &FormReferenceMap_RemoveAllEntries);
	XUtil::DetourJump(g_ModuleBase + 0x16C0A90, &FormReferenceMap_FindOrCreate);
	XUtil::DetourJump(g_ModuleBase + 0x16C0B50, &FormReferenceMap_RemoveEntry);
	XUtil::DetourJump(g_ModuleBase + 0x146C130, &FormReferenceMap_Get);

	//
	// UI
	//
	PatchIAT(hk_CreateDialogParamA, "USER32.DLL", "CreateDialogParamA");
	PatchIAT(hk_DialogBoxParamA, "USER32.DLL", "DialogBoxParamA");
	PatchIAT(hk_EndDialog, "USER32.DLL", "EndDialog");
	PatchIAT(hk_SendMessageA, "USER32.DLL", "SendMessageA");

	if (g_INI.GetBoolean("CreationKit", "UI", false))
	{
		EditorUI_Initialize();
		*(PBYTE *)&OldEditorUI_WndProc = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x13F3770), &EditorUI_WndProc);

		XUtil::PatchMemory(g_ModuleBase + 0x1434473, (PBYTE)"\x90\x90", 2);	// Force bShowReloadShadersButton to always be enabled
		XUtil::PatchMemory(g_ModuleBase + 0x1487B69, (PBYTE)"\x90\x90", 2);	// Enable push to game button even if version control is disabled
		XUtil::PatchMemory(g_ModuleBase + 0x1487B7C, (PBYTE)"\xEB", 1);
		XUtil::PatchMemory(g_ModuleBase + 0x16179C0, (PBYTE)"\xC3", 1);		// Disable "MEM_CATEGORY_X" log spam

		XUtil::DetourJump(g_ModuleBase + 0x1256600, &EditorUI_Warning);
		XUtil::DetourJump(g_ModuleBase + 0x243D610, &EditorUI_Warning);
		XUtil::DetourJump(g_ModuleBase + 0x1CD29E0, &EditorUI_Warning);
		XUtil::DetourJump(g_ModuleBase + 0x122C5F0, &EditorUI_WarningUnknown1);
		XUtil::DetourJump(g_ModuleBase + 0x137FC60, &EditorUI_WarningUnknown1);
		XUtil::DetourJump(g_ModuleBase + 0x1FCB030, &EditorUI_WarningUnknown1);
		XUtil::DetourJump(g_ModuleBase + 0x2452480, &EditorUI_WarningUnknown1);
		XUtil::DetourJump(g_ModuleBase + 0x243D5A0, &EditorUI_WarningUnknown1);
		XUtil::DetourJump(g_ModuleBase + 0x27A6150, &EditorUI_WarningUnknown2);
		XUtil::DetourJump(g_ModuleBase + 0x27A6270, &EditorUI_WarningUnknown2);
		XUtil::DetourCall(g_ModuleBase + 0x163D3D1, &EditorUI_WarningUnknown2);
		XUtil::DetourJump(g_ModuleBase + 0x243D260, &EditorUI_Assert);
	}

	if (g_INI.GetBoolean("CreationKit", "DeferredDialogLoad", false))
	{
		PatchTemplatedFormIterator();
		XUtil::DetourJump(g_ModuleBase + 0x13B9AD0, &InsertComboBoxItem);
		XUtil::DetourJump(g_ModuleBase + 0x13BA4D0, &InsertListViewItem);
		XUtil::DetourJump(g_ModuleBase + 0x20A9710, &CSScript_PickScriptsToCompileDlg_WindowMessage);
		XUtil::DetourJump(g_ModuleBase + 0x1985F20, &SortDialogueInfo);
		XUtil::DetourCall(g_ModuleBase + 0x12C8B63, &UpdateObjectWindowTreeView);
		XUtil::DetourCall(g_ModuleBase + 0x13E117C, &UpdateCellViewListView);

		// Disable useless "Processing Topic X..." status bar updates
		XUtil::PatchMemory(g_ModuleBase + 0x199DE29, (PBYTE)"\x90\x90\x90\x90\x90", 5);
		XUtil::PatchMemory(g_ModuleBase + 0x199EA9E, (PBYTE)"\x90\x90\x90\x90\x90", 5);
		XUtil::PatchMemory(g_ModuleBase + 0x199DA62, (PBYTE)"\x90\x90\x90\x90\x90", 5);
	}

	//
	// Allow saving ESM's directly
	//
	if (g_INI.GetBoolean("CreationKit", "AllowSaveESM", false))
	{
		// Disable: "File '%s' is a master file or is in use.\n\nPlease select another file to save to."
		const char *newFormat = "File '%s' is in use.\n\nPlease select another file to save to.";

		XUtil::PatchMemory(g_ModuleBase + 0x164020A, (PBYTE)"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);
		XUtil::PatchMemory(g_ModuleBase + 0x30B9090, (PBYTE)newFormat, strlen(newFormat) + 1);

		// Also allow ESM's to be set as "Active File"
		XUtil::PatchMemory(g_ModuleBase + 0x13E2D45, (PBYTE)"\x90\x90\x90\x90\x90", 5);

		XUtil::DetourJump(g_ModuleBase + 0x1482DA0, &OpenPluginSaveDialog);
	}

	//
	// Allow ESP files to act as master files while saving
	//
	if (g_INI.GetBoolean("CreationKit", "AllowMasterESP", false))
	{
		XUtil::PatchMemory(g_ModuleBase + 0x1657279, (PBYTE)"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);
	}

	//
	// Memory bug fix during BSShadowDirectionalLight calculations (see game patch for more information)
	//
	XUtil::PatchMemory(g_ModuleBase + 0x2DC679D, (PBYTE)"\x4D\x89\xE1\x90\x90\x90\x90", 7);

	//
	// Skip 'Topic Info' validation during load
	//
	if (g_INI.GetBoolean("CreationKit", "SkipTopicInfoValidation", false))
	{
		XUtil::PatchMemory(g_ModuleBase + 0x19A83C0, (PBYTE)"\xC3", 1);
	}

	//
	// Remove assertion message boxes
	//
	if (g_INI.GetBoolean("CreationKit", "DisableAssertions", false))
	{
		XUtil::PatchMemory(g_ModuleBase + 0x243D9FE, (PBYTE)"\x90\x90\x90\x90\x90", 5);
	}

	//
	// Force render window to draw at 60fps (SetTimer(1ms))
	//
	if (g_INI.GetBoolean("CreationKit", "RenderWindow60FPS", false))
	{
		XUtil::PatchMemory(g_ModuleBase + 0x1306978, (PBYTE)"\x01", 1);
	}

	//
	// Fix crash while trying to upload BNet mods with existing archives
	//
	XUtil::DetourJump(g_ModuleBase + 0x12BE530, &IsBSAVersionCurrent);

	//
	// Kill broken destructors causing crashes on exit
	//
	XUtil::DetourCall(g_ModuleBase + 0x13F3370, &QuitHandler);
	XUtil::DetourCall(g_ModuleBase + 0x2E54B7E, &QuitHandler);
	XUtil::DetourCall(g_ModuleBase + 0x2E54B88, &QuitHandler);

	//
	// Fix crash when loading new CC ESLs as master files
	//
	XUtil::DetourJump(g_ModuleBase + 0x2E44890, &GetESLMasterCount);
	XUtil::DetourJump(g_ModuleBase + 0x2E44920, &GetESLMasterName);
	XUtil::DetourJump(g_ModuleBase + 0x2E448A0, &IsESLMaster);

	//
	// Fix for icons not appearing in the script properties dialog (list view) (LVIF_TEXT -> LVIF_IMAGE)
	//
	XUtil::PatchMemory(g_ModuleBase + 0x20CD744, (PBYTE)"\x02", 1);

	//
	// Fix for being unable to select certain objects in the render view window
	//
	Detours::X64::DetourClassVTable((PBYTE)(g_ModuleBase + 0x345ECD0), &BSShaderResourceManager::FindIntersectionsTriShapeFastPath, 34);

	//
	// Fix the "Cell View" object list current selection not being synced with the render window
	//
	XUtil::DetourJump(g_ModuleBase + 0x13BB720, &ListViewUnselectItem);
	XUtil::DetourJump(g_ModuleBase + 0x13BB650, &ListViewSelectItem);
	XUtil::DetourJump(g_ModuleBase + 0x13BB590, &ListViewFindAndSelectItem);

	//
	// Fix TESModelTextureSwap being incorrectly loaded (Record typo: 'MODS' -> 'MO5S')
	//
	XUtil::PatchMemory(g_ModuleBase + 0x16E55A9, (PBYTE)"\x4D\x4F\x35\x53", 4);

	//
	// Correct the "Push-to-game not supported" error when clicking the "UnEquip Sound" button on the weapon editor
	// dialog. 3682 is reserved exclusively for the PTG functionality, so the button id must be changed. Remapped to
	// 3683 instead.
	//
	XUtil::DetourJump(g_ModuleBase + 0x13B9900, &EditorUI_DialogTabProc);

	uint32_t newId = 3683;
	XUtil::PatchMemory(g_ModuleBase + 0x1B0CBC4, (PBYTE)&newId, sizeof(uint32_t));// SetDlgItemTextA
	XUtil::PatchMemory(g_ModuleBase + 0x1B0DCE9, (PBYTE)&newId, sizeof(uint32_t));// GetDlgItemTextA
	newId += 1;
	XUtil::PatchMemory(g_ModuleBase + 0x1B0AFAA, (PBYTE)&newId, sizeof(uint32_t));// Patch if() comparison

	//
	// Fix for crash when saving certain ESP files (i.e 3DNPC.esp)
	//
	XUtil::DetourJump(g_ModuleBase + 0x1651590, &SortFormArray);

	//
	// Fix for incorrect NavMesh assertion while saving certain ESP files (i.e 3DNPC.esp)
	//
	XUtil::DetourCall(g_ModuleBase + 0x159EB48, &hk_sub_141047AB2);

	//
	// Fix for crash on null BGSPerkRankArray form ids and perk ranks being reset to 1 on save (i.e DianaVampire2017Asherz.esp)
	//
	XUtil::DetourJump(g_ModuleBase + 0x168DF70, &hk_BGSPerkRankArray_sub_14168DF70);
	XUtil::DetourCall(g_ModuleBase + 0x168D1CA, &hk_BGSPerkRankArray_sub_14168EAE0);

	//
	// Fix use-after-free with a NavMeshInfoMap inserted in the altered forms list during a virtual destructor call
	//
	XUtil::PatchMemory(g_ModuleBase + 0x1DD1D38, (PBYTE)"\x90\x90\x90\x90\x90\x90", 6);

	//
	// Fix crash when using more than 16 NPC face tint masks during FaceGen
	//
	XUtil::PatchMemory(g_ModuleBase + 0x1D3B350, (PBYTE)"\x48\x8B\x4C\x24\x68\xE8\xCB\xFF\xFF\xFF\xE9\x7D\x01\x00\x00", 15);
	XUtil::DetourCall(g_ModuleBase + 0x1D3B355, &FaceGenOverflowWarning);

	//
	// Fix crash when Unicode string conversion fails with bethesda.net http responses
	//
	XUtil::DetourJump(g_ModuleBase + 0x2B37750, &BNetConvertUnicodeString);

	//
	// Fix for "Water Type" window options not updating water in the "Render Window" preview
	//
	XUtil::DetourCall(g_ModuleBase + 0x1C68FA6, &hk_call_141C68FA6);
	XUtil::PatchMemory(g_ModuleBase + 0x1C68F93, (PBYTE)"\x90\x90", 2);
	XUtil::PatchMemory(g_ModuleBase + 0x1C62AD8, (PBYTE)"\xEB", 1);

	//
	// Fix for crash when duplicating worldspaces
	//
	XUtil::DetourCall(g_ModuleBase + 0x1C26F3A, &hk_call_141C26F3A);

	//
	// Fix for broken terrain edit dialog undo functionality (Incorrectly removing elements from a linked list, still contains a memory leak)
	//
	XUtil::PatchMemory(g_ModuleBase + 0x143E8CA, (PBYTE)"\x90\x90\x90\x90", 4);
	XUtil::PatchMemory(g_ModuleBase + 0x143EE21, (PBYTE)"\x90\x90\x90\x90", 4);
	XUtil::PatchMemory(g_ModuleBase + 0x143E87E, (PBYTE)"\x90\x90\x90\x90", 4);

	//
	// Plugin loading optimizations:
	//
	// - TESForm reference map rewrite (above)
	// - Fix an unoptimized function bottleneck (sub_141477DA0)
	// - Fix an unoptimized function bottleneck (sub_1414974E0) (Large ESP files only)
	// - Eliminate millions of calls to update the progress dialog, instead only updating 400 times (0% -> 100%)
	// - Replace old zlib decompression code with optimized libdeflate
	//
	int cpuinfo[4];
	__cpuid(cpuinfo, 1);

	// Fall back to non-SSE 4.1 code path when not available
	if ((cpuinfo[2] & (1 << 19)) != 0)
		XUtil::DetourJump(g_ModuleBase + 0x1477DA0, &sub_141477DA0_SSE41);
	else
		XUtil::DetourJump(g_ModuleBase + 0x1477DA0, &sub_141477DA0);

	XUtil::DetourJump(g_ModuleBase + 0x14974E0, &sub_1414974E0);
	XUtil::PatchMemory(g_ModuleBase + 0x163D56E, (PBYTE)"\xB9\x90\x01\x00\x00\x90", 6);
	XUtil::DetourCall(g_ModuleBase + 0x1640FF3, &UpdateLoadProgressBar);
	XUtil::DetourCall(g_ModuleBase + 0x166BB1E, &hk_inflateInit);
	XUtil::DetourCall(g_ModuleBase + 0x166BBB9, &hk_inflate);

}