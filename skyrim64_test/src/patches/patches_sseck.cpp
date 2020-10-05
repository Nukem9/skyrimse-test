#include "../common.h"
#include <xbyak/xbyak.h>
#include <intrin.h>
#include "../typeinfo/ms_rtti.h"
#include "TES/MemoryManager.h"
#include "TES/bhkThreadMemorySource.h"
#include "TES/NiMain/NiRTTI.h"
#include "CKSSE/Experimental.h"
#include "CKSSE/Editor.h"
#include "CKSSE/TESFile_CK.h"
#include "CKSSE/TESForm_CK.h"
#include "CKSSE/NavMesh.h"
#include "CKSSE/EditorUI.h"
#include "CKSSE/EditorUIDarkMode.h"
#include "CKSSE/LogWindow.h"
#include "CKSSE/BSPointerHandleManager.h"
#include "CKSSE/BSGraphicsRenderTargetManager_CK.h"
#include "CKSSE/BSShaderResourceManager_CK.h"
#include "CKSSE/BSRenderPass_CK.h"

void PatchSteam();
void PatchThreading();
void PatchFileIO();
void PatchMemory();
size_t BNetConvertUnicodeString(char *Destination, size_t DestSize, const wchar_t *Source, size_t SourceSize);

void Patch_TESVCreationKit()
{
	if (!_stricmp((const char *)(g_ModuleBase + 0x3078988), "1.5.3.0"))
	{
		// Released 2018-04-13 / Built Mon Sep 18 18:58:37 2017
		Offsets::BuildTableForCKSSEVersion(1530);
	}
	else if (!_stricmp((const char *)(g_ModuleBase + 0x3062CC8), "1.5.73.0"))
	{
		// Released 2019-03-13 / Built Tue Mar 05 18:25:55 2019
		Offsets::BuildTableForCKSSEVersion(1573);
	}
	else
	{
		char modulePath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandle(nullptr), modulePath, ARRAYSIZE(modulePath));

		char message[1024];
		sprintf_s(message,
			"Unknown Creation Kit version detected. Patches are disabled.\n\n"
			"Required versions:\n"
			"CreationKit.exe 1.5.30 released on 2018-04-13\n"
			"CreationKit.exe 1.5.73 released on 2019-03-13\n"
			"\nExecutable path: %s", modulePath);

		MessageBoxA(nullptr, message, "Version Check", MB_ICONERROR);
		return;
	}

	//
	// Replace broken crash dump functionality
	//
	if (g_INI.GetBoolean("CreationKit", "GenerateCrashdumps", true))
	{
		XUtil::InstallCrashDumpHandler();

		XUtil::PatchMemory(OFFSET(0x247D650, 1530), { 0xC3 });	// StackTrace::MemoryTraceWrite
		XUtil::PatchMemory(OFFSET(0x24801DF, 1530), { 0xC3 });	// SetUnhandledExceptionFilter, BSWin32ExceptionHandler
		XUtil::PatchMemory(OFFSET(0x2E558DB, 1530), { 0xC3 });	// SetUnhandledExceptionFilter, Unknown
		XUtil::PatchMemoryNop(OFFSET(0x24801FB, 1530), 6);		// SetUnhandledExceptionFilter, BSWin32ExceptionHandler

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
	// BSPointerHandle(Manager)
	//
	XUtil::DetourJump(OFFSET(0x141A5C0, 1530), &BSPointerHandleManager<>::InitSDM);
	XUtil::DetourJump(OFFSET(0x1770910, 1530), &HandleManager::KillSDM);
	XUtil::DetourJump(OFFSET(0x1770560, 1530), &HandleManager::WarnForUndestroyedHandles);
	XUtil::DetourJump(OFFSET(0x12E2260, 1530), &BSPointerHandleManagerInterface<>::GetCurrentHandle);
	XUtil::DetourJump(OFFSET(0x12E1BE0, 1530), &BSPointerHandleManagerInterface<>::CreateHandle);
	XUtil::DetourJump(OFFSET(0x1291050, 1530), &BSPointerHandleManagerInterface<>::Destroy1);
	XUtil::DetourJump(OFFSET(0x12E1F70, 1530), &BSPointerHandleManagerInterface<>::Destroy2);
	XUtil::DetourJump(OFFSET(0x1293870, 1530), &BSPointerHandleManagerInterface<>::GetSmartPointer1);
	XUtil::DetourJump(OFFSET(0x12E25B0, 1530), &BSPointerHandleManagerInterface<>::GetSmartPointer2);
	XUtil::DetourJump(OFFSET(0x14C52B0, 1530), &BSPointerHandleManagerInterface<>::IsValid);

	// Stub out the rest of the functions which shouldn't ever be called now
	XUtil::PatchMemory(OFFSET(0x12E38A0, 1530), { 0xCC });// BSUntypedPointerHandle::Clear - 1412E38A0
	XUtil::PatchMemory(OFFSET(0x12E2720, 1530), { 0xCC });// BSUntypedPointerHandle::SetAge - 1412E2720
	XUtil::PatchMemory(OFFSET(0x12E3970, 1530), { 0xCC });// BSUntypedPointerHandle::SetActive - 1412E3970
	XUtil::PatchMemory(OFFSET(0x1294740, 1530), { 0xCC });// BSUntypedPointerHandle::GetAge_0 - 141294740
	XUtil::PatchMemory(OFFSET(0x12E3810, 1530), { 0xCC });// BSUntypedPointerHandle::Set - 1412E3810
	XUtil::PatchMemory(OFFSET(0x12E2FF0, 1530), { 0xCC });// BSUntypedPointerHandle::GetIndex_0 - 1412E2FF0
	XUtil::PatchMemory(OFFSET(0x1294A30, 1530), { 0xCC });// BSUntypedPointerHandle::GetIndex - 141294A30
	XUtil::PatchMemory(OFFSET(0x1294720, 1530), { 0xCC });// BSUntypedPointerHandle::GetAge - 141294720
	XUtil::PatchMemory(OFFSET(0x1297430, 1530), { 0xCC });// BSUntypedPointerHandle::ClearActive - 141297430
	XUtil::PatchMemory(OFFSET(0x12973F0, 1530), { 0xCC });// BSUntypedPointerHandle::SetIndex - 1412973F0
	XUtil::PatchMemory(OFFSET(0x12943B0, 1530), { 0xCC });// BSUntypedPointerHandle::IsBitwiseNull - 1412943B0
	//XUtil::PatchMemory(OFFSET(0x12E0DC0, 1530), { 0xCC });// BSUntypedPointerHandle::BSUntypedPointerHandle - 1412E0DC0
	// sub_14100B0A8 - Unknown operator
	// sub_1412E1300 - Unknown operator
	// sub_1412E1210 - Unknown operator

	XUtil::PatchMemory(OFFSET(0x1294590, 1530), { 0xCC });// BSPointerHandle::AgeMatches - 141294590
	XUtil::PatchMemory(OFFSET(0x128D130, 1530), { 0xCC });// BSPointerHandle::GetPtr - 14128D130
	XUtil::PatchMemory(OFFSET(0x128C8D0, 1530), { 0xCC });// BSPointerHandle::AssignPtr - 14128C8D0
	XUtil::PatchMemory(OFFSET(0x1294570, 1530), { 0xCC });// BSPointerHandle::IsActive - 141294570

	XUtil::PatchMemory(OFFSET(0x12E3900, 1530), { 0xCC });// BSHandleRefObject::AssignHandleIndex - 1412E3900
	XUtil::PatchMemory(OFFSET(0x12949D0, 1530), { 0xCC });// BSHandleRefObject::GetIndex - 1412949D0
	// BSHandleRefObject::QRefCount - 141294CB0

	//
	// FaceGen
	//
	// Disable automatic FaceGen on save
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableAutoFaceGen", false))
		XUtil::PatchMemory(OFFSET(0x18DE530, 1530), { 0xC3 });

	// Don't produce DDS files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportDDS", false))
		XUtil::PatchMemoryNop(OFFSET(0x1904318, 1530), 5);

	// Don't produce TGA files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportTGA", false))
		XUtil::PatchMemoryNop(OFFSET(0x190436B, 1530), 5);

	// Don't produce NIF files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportNIF", false))
		XUtil::PatchMemory(OFFSET(0x1904390, 1530), { 0xC3 });

	// Allow variable tint mask resolution
	uint32_t tintResolution = g_INI.GetInteger("CreationKit_FaceGen", "TintMaskResolution", 512);
	XUtil::PatchMemory(OFFSET(0x2DA588C, 1530), (uint8_t *)&tintResolution, sizeof(uint32_t));
	XUtil::PatchMemory(OFFSET(0x2DA5899, 1530), (uint8_t *)&tintResolution, sizeof(uint32_t));

	// Prevent internal filesystem reloads when exporting FaceGen for many NPCs
	XUtil::DetourJump(OFFSET(0x12D1AC0, 1530), &ExportFaceGenForSelectedNPCs);
	XUtil::PatchMemoryNop(OFFSET(0x18F4D4A, 1530), 5);

	//
	// LipGen
	//
	XUtil::DetourJump(OFFSET(0x20F35E0, 1530), &CreateLipGenProcess);
	XUtil::DetourJump(OFFSET(0x13C4C80, 1530), &IsLipDataPresent);
	XUtil::DetourJump(OFFSET(0x1791240, 1530), &WriteLipData);
	XUtil::DetourCall(OFFSET(0x13D5443, 1530), &IsWavDataPresent);
	XUtil::DetourJump(OFFSET(0x13D29B0, 1530), &EditorUI::LipRecordDialogProc);

	//
	// MemoryManager
	//
	if (g_INI.GetBoolean("CreationKit", "MemoryPatch", false))
	{
		PatchMemory();

		XUtil::PatchMemory(OFFSET(0x1223160, 1530), { 0xC3 });							// [3GB  ] MemoryManager - Default/Static/File heaps
		XUtil::PatchMemory(OFFSET(0x24400E0, 1530), { 0xC3 });							// [1GB  ] BSSmallBlockAllocator
		XUtil::DetourJump(OFFSET(0x257D740, 1530), &bhkThreadMemorySource::__ctor__);	// [512MB] bhkThreadMemorySource
		XUtil::PatchMemory(OFFSET(0x2447D90, 1530), { 0xC3 });							// [64MB ] ScrapHeap init
		XUtil::PatchMemory(OFFSET(0x24488C0, 1530), { 0xC3 });							// [64MB ] ScrapHeap deinit
																						// [128MB] BSScaleformSysMemMapper is untouched due to complexity

		XUtil::DetourJump(OFFSET(0x2440380, 1530), &MemoryManager::Allocate);
		XUtil::DetourJump(OFFSET(0x24407A0, 1530), &MemoryManager::Deallocate);
		XUtil::DetourJump(OFFSET(0x243FBA0, 1530), &MemoryManager::Size);
		XUtil::DetourJump(OFFSET(0x2447FA0, 1530), &ScrapHeap::Allocate);
		XUtil::DetourJump(OFFSET(0x24485F0, 1530), &ScrapHeap::Deallocate);
	}

	//
	// NiRTTI
	//
	XUtil::DetourJump(OFFSET(0x269AD20, 1530), &NiRTTI::__ctor__);

	//
	// NavMesh
	//
	if (g_INI.GetBoolean("CreationKit", "NavMeshPseudoDelete", false))
	{
		*(uintptr_t *)&NavMesh::DeleteTriangle = Detours::X64::DetourFunctionClass(OFFSET(0x1D618E0, 1530), &NavMesh::hk_DeleteTriangle);

		XUtil::DetourCall(OFFSET(0x1D6984F, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		XUtil::DetourCall(OFFSET(0x1D699E6, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		XUtil::DetourCall(OFFSET(0x1D69B80, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);

		XUtil::DetourCall(OFFSET(0x1D6A42B, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);
		XUtil::DetourCall(OFFSET(0x1D6A580, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);

		XUtil::PatchMemory(OFFSET(0x1FF9BAC, 1530), { 0xE9, 0xA1, 0x01, 0x00, 0x00 });// Prevent vertices from being deleted separately
	}

	//
	// TESForm
	//
	XUtil::DetourJump(OFFSET(0x16C0650, 1530), &FormReferenceMap_RemoveAllEntries);
	XUtil::DetourJump(OFFSET(0x16C0A90, 1530), &FormReferenceMap_FindOrCreate);
	XUtil::DetourJump(OFFSET(0x16C0B50, 1530), &FormReferenceMap_RemoveEntry);
	XUtil::DetourJump(OFFSET(0x146C130, 1530), &FormReferenceMap_Get);

	XUtil::PatchMemory(OFFSET(0x16C081E, 1530), { 0xCC });
	XUtil::DetourCall(OFFSET(0x16C09DF, 1530), &TESForm_CK::AlteredFormList_Create);
	XUtil::DetourCall(OFFSET(0x163D738, 1530), &TESForm_CK::AlteredFormList_RemoveAllEntries);
	XUtil::DetourCall(OFFSET(0x16B95E2, 1530), &TESForm_CK::AlteredFormList_Insert);
	XUtil::DetourCall(OFFSET(0x16B8EE6, 1530), &TESForm_CK::AlteredFormList_RemoveEntry);
	XUtil::DetourCall(OFFSET(0x16B9693, 1530), &TESForm_CK::AlteredFormList_RemoveEntry);
	XUtil::DetourCall(OFFSET(0x16B95BD, 1530), &TESForm_CK::AlteredFormList_ElementExists);

	//
	// UI
	//
	PatchIAT(hk_CreateDialogParamA, "USER32.DLL", "CreateDialogParamA");
	PatchIAT(hk_DialogBoxParamA, "USER32.DLL", "DialogBoxParamA");
	PatchIAT(hk_EndDialog, "USER32.DLL", "EndDialog");
	PatchIAT(hk_SendMessageA, "USER32.DLL", "SendMessageA");

	if (g_INI.GetBoolean("CreationKit", "UIDarkTheme", false))
	{
		auto comDll = (uintptr_t)GetModuleHandle("comctl32.dll");
		Assert(comDll);

		EditorUIDarkMode::Initialize();
		Detours::IATHook(comDll, "USER32.dll", "GetSysColor", (uintptr_t)&EditorUIDarkMode::Comctl32GetSysColor);
		Detours::IATHook(comDll, "USER32.dll", "GetSysColorBrush", (uintptr_t)&EditorUIDarkMode::Comctl32GetSysColorBrush);
		Detours::IATDelayedHook(comDll, "UxTheme.dll", "DrawThemeBackground", (uintptr_t)&EditorUIDarkMode::Comctl32DrawThemeBackground);
		Detours::IATDelayedHook(comDll, "UxTheme.dll", "DrawThemeText", (uintptr_t)&EditorUIDarkMode::Comctl32DrawThemeText);
	}

	if (g_INI.GetBoolean("CreationKit", "UIHotkeys", false))
	{
		Detours::X64::DetourFunctionClass(OFFSET(0x1008538, 1530), &EditorUI::RegisterHotkeyFunction);
	}

	if (g_INI.GetBoolean("CreationKit", "UI", false))
	{
		EditorUI::Initialize();
		*(uintptr_t *)&EditorUI::OldWndProc = Detours::X64::DetourFunctionClass(OFFSET(0x13F3770, 1530), &EditorUI::WndProc);
		*(uintptr_t *)&EditorUI::OldObjectWindowProc = Detours::X64::DetourFunctionClass(OFFSET(0x12C3ED0, 1530), &EditorUI::ObjectWindowProc);
		*(uintptr_t *)&EditorUI::OldCellViewProc = Detours::X64::DetourFunctionClass(OFFSET(0x13D8F40, 1530), &EditorUI::CellViewProc);

		XUtil::DetourCall(OFFSET(0x20AD5C9, 1530), &hk_call_1420AD5C9);// Raise the papyrus script editor text limit to 500k characters from 64k
		XUtil::DetourCall(OFFSET(0x1CF03C9, 1530), &hk_call_141CF03C9);// Update the UI options when fog is toggled
		XUtil::DetourCall(OFFSET(0x12D1541, 1530), &hk_call_1412D1541);// Allow forms to be filtered in EditorUI_ObjectWindowProc
		XUtil::DetourCall(OFFSET(0x147FB57, 1530), &hk_call_14147FB57);// Allow forms to be filtered in EditorUI_CellViewProc
		XUtil::DetourCall(OFFSET(0x1C9879C, 1530), &hk_call_14147FB57);// ^
		XUtil::PatchMemoryNop(OFFSET(0x1434473, 1530), 2);				// Force bShowReloadShadersButton to always be enabled
		XUtil::PatchMemoryNop(OFFSET(0x1487B69, 1530), 2);				// Enable push to game button even if version control is disabled
		XUtil::PatchMemory(OFFSET(0x1487B7C, 1530), { 0xEB });			// ^
		XUtil::PatchMemory(OFFSET(0x16179C0, 1530), { 0xC3 });			// Disable "MEM_CATEGORY_X" log spam
		XUtil::PatchMemoryNop(OFFSET(0x2DCE6BC, 1530), 5);				// Disable "utility failed id" log spam
		XUtil::PatchMemoryNop(OFFSET(0x2D270E3, 1530), 5);				// Disable "Should have been converted offline" log spam
		XUtil::PatchMemoryNop(OFFSET(0x1582E18, 1530), 7);				// Prevent setting redundant colors in the condition list view NM_CUSTOMDRAW (breaks dark theme)
		XUtil::PatchMemory(OFFSET(0x1582E85, 1530), { 0x74, 0x20 });	// ^
		XUtil::DetourCall(OFFSET(0x18276C9, 1530), &ArrayQuickSortRecursive<class BGSEntryPointPerkEntry *, true>);// Stable sort for perk entry window

		XUtil::DetourJump(OFFSET(0x1256600, 1530), &LogWindow::LogWarning);
		XUtil::DetourJump(OFFSET(0x243D610, 1530), &LogWindow::LogWarning);
		XUtil::DetourJump(OFFSET(0x1CD29E0, 1530), &LogWindow::LogWarning);
		XUtil::DetourJump(OFFSET(0x122C5F0, 1530), &LogWindow::LogWarningUnknown1);
		XUtil::DetourJump(OFFSET(0x137FC60, 1530), &LogWindow::LogWarningUnknown1);
		XUtil::DetourJump(OFFSET(0x1FCB030, 1530), &LogWindow::LogWarningUnknown1);
		XUtil::DetourJump(OFFSET(0x2452480, 1530), &LogWindow::LogWarningUnknown1);
		XUtil::DetourJump(OFFSET(0x243D5A0, 1530), &LogWindow::LogWarningUnknown1);
		XUtil::DetourJump(OFFSET(0x27A7DA0, 1530), &LogWindow::LogWarningUnknown1);
		XUtil::DetourJump(OFFSET(0x27A6150, 1530), &LogWindow::LogWarningUnknown2);
		XUtil::DetourJump(OFFSET(0x27A6270, 1530), &LogWindow::LogWarningUnknown2);
		XUtil::DetourCall(OFFSET(0x163D3D1, 1530), &LogWindow::LogWarningUnknown2);
		XUtil::DetourJump(OFFSET(0x243D260, 1530), &LogWindow::LogAssert);
	}

	if (g_INI.GetBoolean("CreationKit", "DisableWindowGhosting", false))
	{
		DisableProcessWindowsGhosting();
	}

	// Deferred dialog loading (batched UI updates)
	XUtil::DetourJump(OFFSET(0x13B9AD0, 1530), &InsertComboBoxItem);
	XUtil::DetourJump(OFFSET(0x13BA4D0, 1530), &InsertListViewItem);
	XUtil::DetourJump(OFFSET(0x20A9710, 1530), &EditorUI::CSScript_PickScriptsToCompileDlgProc);
	XUtil::DetourJump(OFFSET(0x1985F20, 1530), &SortDialogueInfo);
	XUtil::DetourCall(OFFSET(0x12C8B63, 1530), &UpdateObjectWindowTreeView);
	XUtil::DetourCall(OFFSET(0x13DAB04, 1530), &UpdateCellViewCellList);
	XUtil::DetourCall(OFFSET(0x13E117C, 1530), &UpdateCellViewObjectList);

	// Disable useless "Processing Topic X..." status bar updates
	XUtil::PatchMemoryNop(OFFSET(0x199DE29, 1530), 5);
	XUtil::PatchMemoryNop(OFFSET(0x199EA9E, 1530), 5);
	XUtil::PatchMemoryNop(OFFSET(0x199DA62, 1530), 5);

	//
	// AllowSaveESM   - Allow saving ESMs directly without version control
	// AllowMasterESP - Allow ESP files to act as master files while saving
	//
	TESFile::AllowSaveESM = g_INI.GetBoolean("CreationKit", "AllowSaveESM", false);
	TESFile::AllowMasterESP = g_INI.GetBoolean("CreationKit", "AllowMasterESP", false);

	if (TESFile::AllowSaveESM || TESFile::AllowMasterESP)
	{
		*(uintptr_t *)&TESFile::LoadTESInfo = Detours::X64::DetourFunctionClass(OFFSET(0x1664CC0, 1530), &TESFile::hk_LoadTESInfo);
		*(uintptr_t *)&TESFile::WriteTESInfo = Detours::X64::DetourFunctionClass(OFFSET(0x1665520, 1530), &TESFile::hk_WriteTESInfo);

		if (TESFile::AllowSaveESM)
		{
			// Also allow non-game ESMs to be set as "Active File"
			XUtil::DetourCall(OFFSET(0x13E2D37, 1530), &TESFile::IsActiveFileBlacklist);
			XUtil::PatchMemoryNop(OFFSET(0x163CA2E, 1530), 2);

			// Disable: "File '%s' is a master file or is in use.\n\nPlease select another file to save to."
			const char *newFormat = "File '%s' is in use.\n\nPlease select another file to save to.";

			XUtil::PatchMemoryNop(OFFSET(0x164020A, 1530), 12);
			XUtil::PatchMemory(OFFSET(0x30B9090, 1530), (uint8_t *)newFormat, strlen(newFormat) + 1);

			XUtil::DetourJump(OFFSET(0x1482DA0, 1530), &OpenPluginSaveDialog);
		}

		if (TESFile::AllowMasterESP)
		{
			// Remove the check for IsMaster()
			XUtil::PatchMemoryNop(OFFSET(0x1657279, 1530), 12);
		}
	}

	//
	// Skip 'Topic Info' validation during load
	//
	if (g_INI.GetBoolean("CreationKit", "SkipTopicInfoValidation", false))
	{
		XUtil::PatchMemory(OFFSET(0x19A83C0, 1530), { 0xC3 });
	}

	//
	// Remove assertion message boxes
	//
	if (g_INI.GetBoolean("CreationKit", "DisableAssertions", false))
	{
		XUtil::PatchMemoryNop(OFFSET(0x243D9FE, 1530), 5);
	}

	//
	// Force render window to draw at 60fps (SetTimer(1ms))
	//
	if (g_INI.GetBoolean("CreationKit", "RenderWindow60FPS", false))
	{
		XUtil::PatchMemory(OFFSET(0x1306978, 1530), { 0x01 });
	}

	//
	// Memory bug fix during BSShadowDirectionalLight calculations (see game patch for more information)
	//
	XUtil::PatchMemory(OFFSET(0x2DC679D, 1530), { 0x4D, 0x89, 0xE1, 0x90, 0x90, 0x90, 0x90 });

	//
	// Re-enable land shadows. Instead of caching the upload once per frame, upload it on every draw call.
	// (BSBatchRenderer::Draw -> GEOMETRY_TYPE_DYNAMIC_TRISHAPE uiFrameCount)
	//
	// Fixes a bug where BSDynamicTriShape dynamic data would be written to 1 of 4 ring buffers in the shadowmap pass and
	// cached. At some point later in the frame sub_140D6BF00 would increment a counter and swap the currently used
	// buffer. In the main render pass DrawDynamicTriShape would use that new buffer instead of the previous one during
	// shadows. The data offset (m_VertexAllocationOffset) was always correct, but the wrong ring buffer was used.
	//
	XUtil::PatchMemoryNop(OFFSET(0x13CECD4, 1530), 6);
	XUtil::PatchMemoryNop(OFFSET(0x2DB6A51, 1530), 2);

	//
	// Re-enable fog rendering in the Render Window by forcing post-process effects (SAO/SAOComposite/SAOFog)
	//
	XUtil::DetourCall(OFFSET(0x13CDC32, 1530), &hk_sub_141032ED7);
	XUtil::DetourCall(OFFSET(0x13CDEDF, 1530), &hk_sub_141032ED7);
	XUtil::DetourCall(OFFSET(0x13CE164, 1530), &hk_sub_141032ED7);
	XUtil::DetourCall(OFFSET(0x13CE36D, 1530), &hk_sub_141032ED7);

	XUtil::PatchMemoryNop(OFFSET(0x2E2EFAF, 1530), 4);		// Pointer always null
	XUtil::PatchMemoryNop(OFFSET(0x2E2F003, 1530), 99);		// Pointer always null
	XUtil::PatchMemoryNop(OFFSET(0x2E2F0AE, 1530), 5);		// Pointer always null (second parameter)
	XUtil::PatchMemoryNop(OFFSET(0x2E2F270, 1530), 5);		// Pointer always null (second parameter)
	XUtil::PatchMemoryNop(OFFSET(0x2E2F275, 1530), 38);		// Assert always triggers
	XUtil::PatchMemoryNop(OFFSET(0x2E2F29B, 1530), 41);		// Assert always triggers
	XUtil::PatchMemoryNop(OFFSET(0x2E2F2C4, 1530), 22);		// Multiple null pointers in call
	XUtil::PatchMemoryNop(OFFSET(0x2E2F2E4, 1530), 546);	// Remove most of the useless stuff in the function

	XUtil::PatchMemory(OFFSET(0x2E2BC50, 1530), { 0xC3 });	// Pointer always null (BSGraphics::State::UpdateTemporalData)
	XUtil::PatchMemory(OFFSET(0x2E2BAF0, 1530), { 0xC3 });	// Pointer always null (BSGraphics::State::UpdateTemporalData)

	XUtil::PatchMemoryNop(OFFSET(0x2DA05C5, 1530), 2);		// Force DEPTH_STENCIL_POST_ZPREPASS_COPY RT to be copied every frame

	//
	// Fix crash while trying to upload BNet mods with existing archives
	//
	XUtil::DetourJump(OFFSET(0x12BE530, 1530), &IsBSAVersionCurrent);

	//
	// Kill broken destructors causing crashes on exit
	//
	XUtil::DetourCall(OFFSET(0x13F3370, 1530), &QuitHandler);
	XUtil::DetourCall(OFFSET(0x2E54B7E, 1530), &QuitHandler);
	XUtil::DetourCall(OFFSET(0x2E54B88, 1530), &QuitHandler);

	//
	// Fix crash when loading new CC ESLs as master files. Update 1.5.73 automatically parses the Skyrim.CCC file.
	//
	if (Offsets::CanResolve(0x2E44890, 1530))
	{
		XUtil::DetourJump(OFFSET(0x2E44890, 1530), &BSGameDataSystemUtility__GetCCFileCount);
		XUtil::DetourJump(OFFSET(0x2E44920, 1530), &BSGameDataSystemUtility__GetCCFile);
		XUtil::DetourJump(OFFSET(0x2E448A0, 1530), &BSGameDataSystemUtility__IsCCFile);
	}

	//
	// Fix for icons not appearing in the script properties dialog (list view) (LVIF_TEXT -> LVIF_IMAGE)
	//
	XUtil::PatchMemory(OFFSET(0x20CD744, 1530), { 0x02 });

	//
	// Rewrite their ray->triangle intersection function. This fixes 3 things:
	//
	// - Being unable to select certain objects in the render view window.
	// - Selections not scaling correctly depending on distance (ex. LOD terrain) and NiObject scale.
	// - The Object Palette window "Conform to slope" option causing broken object angles on placement. SE changed data
	// layouts and geometry vertex data may not include normals.
	//
	Detours::X64::DetourClassVTable(OFFSET(0x345ECD0, 1530), &BSShaderResourceManager_CK::FindIntersectionsTriShapeFastPath, 34);

	//
	// Fix the "Cell View" object list current selection not being synced with the render window
	//
	XUtil::DetourJump(OFFSET(0x13BB650, 1530), &EditorUI::ListViewSelectItem);
	XUtil::DetourJump(OFFSET(0x13BB590, 1530), &EditorUI::ListViewFindAndSelectItem);
	XUtil::DetourJump(OFFSET(0x13BB720, 1530), &EditorUI::ListViewDeselectItem);

	//
	// Fix TESModelTextureSwap being incorrectly loaded (Record typo: 'MODS' -> 'MO5S')
	//
	XUtil::PatchMemory(OFFSET(0x16E55A9, 1530), { 0x4D, 0x4F, 0x35, 0x53 });

	//
	// Correct the "Push-to-game not supported" error when clicking the "UnEquip Sound" button on the weapon editor
	// dialog. 3682 is reserved exclusively for the PTG functionality, so the button id must be changed. Remapped to
	// 3683 instead.
	//
	XUtil::DetourJump(OFFSET(0x13B9900, 1530), &EditorUI::DialogTabProc);

	uint32_t newId = 3683;
	XUtil::PatchMemory(OFFSET(0x1B0CBC4, 1530), (uint8_t *)&newId, sizeof(uint32_t));// SetDlgItemTextA
	XUtil::PatchMemory(OFFSET(0x1B0DCE9, 1530), (uint8_t *)&newId, sizeof(uint32_t));// GetDlgItemTextA
	newId += 1;
	XUtil::PatchMemory(OFFSET(0x1B0AFAA, 1530), (uint8_t *)&newId, sizeof(uint32_t));// Patch if() comparison

	//
	// Fix for crash (recursive sorting function stack overflow) when saving certain ESP files (i.e 3DNPC.esp)
	//
	XUtil::DetourJump(OFFSET(0x1651590, 1530), &ArrayQuickSortRecursive<class TESForm_CK *>);

	//
	// Fix for incorrect NavMesh assertion while saving certain ESP files (i.e 3DNPC.esp). Fixed in 1.5.73.
	//
	if (Offsets::CanResolve(0x159EB48, 1530))
	{
		XUtil::DetourCall(OFFSET(0x159EB48, 1530), &hk_sub_141047AB2);
	}

	//
	// Fix for incorrect pointer truncate assertion while saving certain conditions (i.e 3DNPC.esp). TESParameters/CTDA.
	//
	XUtil::DetourCall(OFFSET(0x158589F, 1530), &hk_call_14158589F);

	//
	// Fix for crash on null BGSPerkRankArray form ids and perk ranks being reset to 1 on save (i.e DianaVampire2017Asherz.esp)
	//
	XUtil::DetourJump(OFFSET(0x168DF70, 1530), &InitItemPerkRankDataVisitor);
	XUtil::DetourCall(OFFSET(0x168D1CA, 1530), &PerkRankData__LoadFrom);

	//
	// Fix use-after-free with a NavMeshInfoMap inserted in the altered forms list during a virtual destructor call. NavMeshInfoMap::Clear.
	//
	XUtil::PatchMemoryNop(OFFSET(0x1DD1D38, 1530), 6);

	//
	// Fix crash when using more than 16 NPC face tint masks during FaceGen
	//
	XUtil::PatchMemory(OFFSET(0x1D3B350, 1530), { 0x48, 0x8B, 0x4C, 0x24, 0x68, 0xE8, 0xCB, 0xFF, 0xFF, 0xFF, 0xE9, 0x7D, 0x01, 0x00, 0x00 });
	XUtil::DetourCall(OFFSET(0x1D3B355, 1530), &FaceGenOverflowWarning);

	//
	// Fix crash when Unicode string conversion fails with bethesda.net http responses
	//
	XUtil::DetourJump(OFFSET(0x2B37750, 1530), &BNetConvertUnicodeString);

	//
	// Fix for "Water Type" window options not updating water in the "Render Window" preview
	//
	XUtil::DetourCall(OFFSET(0x1C68FA6, 1530), &hk_call_141C68FA6);
	XUtil::PatchMemoryNop(OFFSET(0x1C68F93, 1530), 2);
	XUtil::PatchMemory(OFFSET(0x1C62AD8, 1530), { 0xEB });

	//
	// Fix for crash when duplicating worldspaces
	//
	XUtil::DetourCall(OFFSET(0x1C26F3A, 1530), &hk_call_141C26F3A);

	//
	// Fix for broken terrain edit dialog undo functionality (Incorrectly removing elements from a linked list, still contains a memory leak)
	//
	XUtil::PatchMemoryNop(OFFSET(0x143E8CA, 1530), 4);
	XUtil::PatchMemoryNop(OFFSET(0x143EE21, 1530), 4);
	XUtil::PatchMemoryNop(OFFSET(0x143E87E, 1530), 4);

	//
	// Fix for crash when using FlowChartX functionality to grab current topic info id in a dialogue view. The broken code path returns
	// a VARIANT of type VT_UI8 (21) with an invalid 8-byte pointer in the buffer. This code path is never taken in CK32.
	//
	// This hook also fixes broken graph layout where every topic would draw on top of the other.
	//
	XUtil::DetourCall(OFFSET(0x17E42BF, 1530), &hk_call_1417E42BF);

	//
	// Fix for crash (invalid parameter termination) when the "Unable to find variable" warning would exceed the buffer size
	//
	XUtil::PatchMemory(OFFSET(0x31027F8, 1530), (uint8_t *)", Text \"%.240s\"", strlen(", Text \"%.240s\"") + 1);

	//
	// Replace direct crash with an assertion when an incompatible texture format is used in the renderer
	//
	XUtil::DetourCall(OFFSET(0x2D0CCBF, 1530), &DirectX__LoadFromDDSFile);

	//
	// Fix for crash when trying to use "Test Radius" on a reference's "3D Data" dialog tab. This code wasn't correctly ported to
	// BSGeometry from NiGeometry during the LE->SSE transition. Flags & materials need to be fixed as a result.
	//
	XUtil::DetourCall(OFFSET(0x1C410A1, 1530), &hk_call_141C410A1);
	XUtil::DetourCall(OFFSET(0x1C41122, 1530), &hk_call_141C410A1);
	XUtil::PatchMemory(OFFSET(0x1C41094, 1530), { 0x48, 0x8B, 0xC1, 0x90, 0x90 });
	XUtil::PatchMemory(OFFSET(0x1C41115, 1530), { 0x48, 0x8B, 0xC1, 0x90, 0x90 });

	//
	// Fix for "File in use" UI hang after hitting cancel. It tries to use the main window handle as a parent, but it's suspended
	// during the initial data load, resulting in a deadlock. The new BGSThreadedProgressDlg causes this.
	//
	XUtil::PatchMemory(OFFSET(0x16641B1, 1530), { 0x4D, 0x33, 0xC0, 0x90, 0x90, 0x90, 0x90 });

	//
	// Fix for weapon critical effect data (CRDT) being destroyed when upgrading from form version <= 43 to form version 44. The CK
	// reads a structure that has 64bit alignment and is incompatible with old versions.
	//
	XUtil::PatchMemoryNop(OFFSET(0x1B04201, 1530), 98);
	XUtil::PatchMemoryNop(OFFSET(0x1B042C1, 1530), 7);
	XUtil::DetourJump(OFFSET(0x1B08540, 1530), &TESObjectWEAP__Data__ConvertCriticalData);
	XUtil::DetourCall(OFFSET(0x1B037B2, 1530), &TESObjectWEAP__Data__LoadCriticalData);

	//
	// Fix for "Could not select actor value X in LoadDialog for BGSEntryPointFunctionDataTwoValue." Use the editor id instead of perk
	// name for the Perk Entry dialog selection.
	//
	XUtil::DetourCall(OFFSET(0x17F4A04, 1530), &hk_call_1417F4A04);

	//
	// Fix for crash after erasing an iterator and dereferencing it in "InventoryChanges" code
	//
	class changeInventoryHook : public Xbyak::CodeGenerator
	{
	public:
		changeInventoryHook() : Xbyak::CodeGenerator()
		{
			// iterator = iterator->next
			mov(rax, ptr[rsp + 0xD0]);
			mov(rax, ptr[rax + 0x8]);
			mov(ptr[rsp + 0xD0], rax);

			// Continue with code that destroys the now-previous iterator
			mov(rax, ptr[rsp + 0x50]);
			jmp(ptr[rip]);
			dq(OFFSET(0x1776B19, 1530));
		}
	} static inventoryHookInstance;

	XUtil::DetourJump(OFFSET(0x1776B14, 1530), (uintptr_t)inventoryHookInstance.getCode());

	//
	// Fix for crash when editing a spell effect with a large (>= 1'000'000'000) duration. WARNING: Stack padding allows the buffer
	// to be up to 12 bytes, 10 are originally reserved.
	//
	XUtil::PatchMemory(OFFSET(0x1CA2CBD, 1530), { 0xBA, 0x0C, 0x00, 0x00, 0x00 });
	XUtil::PatchMemory(OFFSET(0x1CA2E64, 1530), { 0xBA, 0x0C, 0x00, 0x00, 0x00 });

	//
	// Increase the maximum navmesh autogeneration cell limit to 100,000 and prevent spamming UI updates (0.01% -> 1.00%)
	//
	XUtil::PatchMemory(OFFSET(0x202F0B6, 1530), { 0xA0, 0x86, 0x01, 0x00 });
	XUtil::DetourCall(OFFSET(0x202E0E8, 1530), &hk_call_14202E0E8);

	//
	// Fix for crash when using "Move to topic" in a quest dialogue view. Any unresolved/unused Topic actions default to "Unknown action",
	// but a null pointer is used while trying to get the type.
	//
	XUtil::PatchMemoryNop(OFFSET(0x198C661, 1530), 5);

	//
	// Fix for a package's "Selected Package Data" combo box not having a selected value when using a Topic type. Pointer<->Form ID truncation.
	//
	XUtil::PatchMemoryNop(OFFSET(0x18A0914, 1530), 7);

	//
	// Fix for TESObjectLAND vertex normals appearing corrupted in worldspaces with a parent worldspace. The purpose of this code is unknown
	// and not present in the game itself. TESObjectLAND::LoadVertices.
	//
	XUtil::PatchMemoryNop(OFFSET(0x1B76B17, 1530), 2);

	//
	// Fix for TESObjectLAND vertex normals becoming corrupted when saving worldspaces with a parent worldspace. Invalid memcpy() size supplied.
	//
	XUtil::PatchMemory(OFFSET(0x1B93216, 1530), { 0x41, 0xB8, 0x63, 0x03, 0x00, 0x00 });

	//
	// Fix for the "Object Palette" preview window not working. Window render state has to be set to '2'.
	//
	XUtil::DetourCall(OFFSET(0x12DD706, 1530), &hk_call_1412DD706);

	//
	// Fix for crash when cell references are added/removed during initialization, similar to the broken iterator in InventoryChanges
	//
	XUtil::DetourJump(OFFSET(0x1BBF220, 1530), &sub_141BBF220);
	XUtil::DetourJump(OFFSET(0x1BBF320, 1530), &sub_141BBF320);

	//
	// Fix for memory leak when opening many preview windows or resizing them. D3D11 render targets are recreated each time, but the old ones
	// were never released.
	//
	XUtil::DetourJump(OFFSET(0x2D06B10, 1530), &BSGraphicsRenderTargetManager_CK::CreateRenderTarget);
	XUtil::DetourJump(OFFSET(0x2D06BB0, 1530), &BSGraphicsRenderTargetManager_CK::CreateDepthStencil);
	XUtil::DetourJump(OFFSET(0x2D06C30, 1530), &BSGraphicsRenderTargetManager_CK::CreateCubemapRenderTarget);

	//
	// Fix for crash after the "Multiple masters selected for load" dialog is shown. Missing null pointer check in Sky::UpdateAurora.
	//
	XUtil::DetourCall(OFFSET(0x1CE8269, 1530), &hk_call_141CE8269);

	//
	// Fix for crash when duplicating a form with an empty editor id. Integer underflow when string length is 0. TESForm::MakeUniqueEditorID.
	//
	XUtil::DetourCall(OFFSET(0x16B849E, 1530), &hk_call_1416B849E);
	XUtil::PatchMemoryNop(OFFSET(0x16B84A3, 1530), 1);

	//
	// Fix for "Select Enable State Parent" selecting objects outside of the current cell or worldspace
	//
	//XUtil::DetourCall(OFFSET(0x135CDD3, 1530), &hk_call_14135CDD3);

	//
	// Fix for the "Data" window not listing plugins according to the user's load order. The CK tries to find plugins.txt in the executable
	// directory instead of %localappdata%.
	//
	XUtil::DetourJump(OFFSET(0x27B1720, 1530), &BSUtilities__SetLocalAppDataPath);

	//
	// Fix for the "Actor Flags" or "Actor Behavior" dialogs not showing their column headers. wParam was swapped to lParam for an unknown
	// reason in SE only. Undo that change.
	//
	XUtil::PatchMemory(OFFSET(0x1232001, 1530), { 0x80, 0x12, 0x00, 0x00 });

	//
	// Fix for water not rendering correctly while using the orthographic (top-down) camera view. SSE camera scaling changes cause
	// weird behavior with water shaders.
	//
	XUtil::DetourCall(OFFSET(0x130F9E8, 1530), &hk_call_14130F9E8);

	//
	// Fix for crash when too much geometry is present in the scene (usually with navmesh). The CK runs out of render pass cache entries.
	// Dynamically allocate them instead.
	//
	XUtil::DetourJump(OFFSET(0x2DB3840, 1530), &BSRenderPass_CK::InitSDM);
	XUtil::DetourJump(OFFSET(0x2DB3A10, 1530), &BSRenderPass_CK::KillSDM);
	XUtil::DetourJump(OFFSET(0x2DB3610, 1530), &BSRenderPass_CK::AllocatePass);
	XUtil::DetourJump(OFFSET(0x2DB3750, 1530), &BSRenderPass_CK::DeallocatePass);
	
	//
	// Fix for crash when tab control buttons are deleted. Uninitialized TCITEMA structure variables.
	//
	XUtil::DetourJump(OFFSET(0x13BC5E0, 1530), &EditorUI::TabControlDeleteItem);

	//
	// Fix for crash when saving a plugin with an empty single track file path in a Music Track form. Null pointer dereference.
	//
	XUtil::DetourCall(OFFSET(0x1A0808C, 1530), &hk_call_141A0808C);

	//
	// Fix for the "Class" edit dialog not filling in the "Training" checkbox. Also hide the unused "Recharge" option.
	//
	Detours::X64::DetourClassVTable(OFFSET(0x30F0418, 1530), &TESClass__InitializeEditDialog, 86);

	//
	// Fix for the "Race" dialog not saving "Specials" if the list box was empty and a new spell was added. Inverted comparison logic.
	//
	XUtil::PatchMemory(OFFSET(0x16F5B13, 1530), { 0x74 });

	//
	// Print a warning when a cloned NiCollisionObject has no name specified in its NIF file. This comes from malformed/ported game assets.
	//
	XUtil::DetourCall(OFFSET(0x267B359, 1530), &hk_call_14267B359);

	//
	// Assert if D3D11 FL11 features are not supported
	//
	XUtil::DetourCall(OFFSET(0x2D12196, 1530), &hk_call_142D12196);

	//
	// Fix for a memory leak in BSShadowLight::ClearShadowMapData after opening "Actor" dialogs (~500kb per instance). The code loops over
	// a ShadowMapData array and checks if ShadowMapIndex is NOT -1, freeing the data if true. When opening a dialog this is always -1 and
	// it never gets deallocated. Hacky fix: remove the check.
	//
	XUtil::PatchMemoryNop(OFFSET(0x2DCB709, 1530), 6);

	//
	// Allow the "PlayerKnows" conditional function to accept enchantments as a function parameter
	//
	XUtil::DetourJump(OFFSET(0x1481390, 1530), &hk_sub_141481390);

	//
	// Fix for the "Dialogue Branch" dialog showing corrupted starting topic strings. The address of a variable is provided instead of a string
	// pointer. Change LEA to MOV.
	//
	XUtil::PatchMemory(OFFSET(0x17D9F5C, 1530), { 0x4C, 0x8B });

	//
	// Fix for the "Bright Light Color" option having incorrect colors in the preferences window. The blue and green channels are swapped.
	//
	XUtil::DetourCall(OFFSET(0x1434458, 1530), &hk_call_141434458);

	//
	// Fix for crash when plugins.txt is present in the game root folder. Buffer overflow in ArchiveManager::OpenMasterArchives when appending
	// to a string. Skip the parsing code completely.
	//
	XUtil::PatchMemoryNop(OFFSET(0x2636E9A, 1530), 6);

	//
	// Assert when a NiSkinInstance is missing a skeleton root node in NIF files
	//
	Detours::X64::DetourClassVTable(OFFSET(0x334FBC0, 1530), &NiSkinInstance__LinkObject, 25);
	Detours::X64::DetourClassVTable(OFFSET(0x334FD50, 1530), &NiSkinInstance__LinkObject, 25);

	//
	// FlowChartX needs to be registered as a COM server dll (DllRegisterServer), but it never tells you that administrator permissions are required
	//
	XUtil::DetourCall(OFFSET(0x1299CF5, 1530), &hk_call_141299CF5);

	//
	// Fix for crash when MakeXYZCircles passes an invalid line count to BSShaderResourceManager::CreateLineShape. BGSPrimitiveSphere only creates
	// these debug marker circles in the editor.
	//
	XUtil::DetourCall(OFFSET(0x27D1EC0, 1530), &hk_call_1427D0AC0);

	//
	// Fix for crash when BSLightingShader::SetupMaterial(MULTIINDEXTRISHAPESNOW) incorrectly casts a BSLightingShaderMaterial to
	// BSLightingShaderMaterialSnow. Force the shader sparkle params to zero instead (xor xmm0, xmm1, xmm2, eax).
	//
	XUtil::PatchMemoryNop(OFFSET(0x2DD5460, 1530), 24);
	XUtil::PatchMemory(OFFSET(0x2DD5460, 1530), { 0x0F, 0x57, 0xC0, 0x0F, 0x57, 0xC9, 0x0F, 0x57, 0xD2, 0x33, 0xC0 });

	//
	// Plugin loading optimizations:
	//
	// - TESForm reference map rewrite (above)
	// - Fix an unoptimized function bottleneck (sub_1414974E0) (Large ESP files only)
	// - Fix an unoptimized function bottleneck (sub_1415D5640)
	// - Eliminate millions of calls to update the progress dialog, instead only updating 400 times (0% -> 100%)
	// - Replace old zlib decompression code with optimized libdeflate
	// - Cache results from FindFirstFile when GetFileAttributesExA is called immediately after (BSSystemDir__NextEntry, BSResource__LooseFileLocation__FileExists)
	//
	int cpuinfo[4];
	__cpuid(cpuinfo, 1);

	// Utilize SSE4.1 instructions if available
	if ((cpuinfo[2] & (1 << 19)) != 0)
		XUtil::DetourJump(OFFSET(0x14974E0, 1530), &sub_1414974E0_SSE41);
	else
		XUtil::DetourJump(OFFSET(0x14974E0, 1530), &sub_1414974E0);

	XUtil::DetourJump(OFFSET(0x15D5640, 1530), &sub_1415D5640);
	XUtil::PatchMemory(OFFSET(0x163D56E, 1530), { 0xB9, 0x90, 0x01, 0x00, 0x00, 0x90 });
	XUtil::DetourCall(OFFSET(0x1640FF3, 1530), &UpdateLoadProgressBar);
	XUtil::DetourCall(OFFSET(0x166BB1E, 1530), &hk_inflateInit);
	XUtil::DetourCall(OFFSET(0x166BBB9, 1530), &hk_inflate);
	XUtil::DetourJump(OFFSET(0x2647AC0, 1530), &BSSystemDir__NextEntry);
	XUtil::DetourJump(OFFSET(0x2676020, 1530), &BSResource__LooseFileLocation__FileExists);

	//
	// Experimental. Must be run last to avoid interfering with other hooks and patches.
	//
	ExperimentalPatchOptimizations();
}