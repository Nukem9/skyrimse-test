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
#include "CKSSE/LogWindow.h"
#include "CKSSE/BSPointerHandleManager.h"
#include "CKSSE/BSGraphicsRenderTargetManager_CK.h"
#include "CKSSE/BSShaderResourceManager_CK.h"

void PatchSteam();
void PatchThreading();
void PatchFileIO();
void PatchMemory();
size_t BNetConvertUnicodeString(char *Destination, size_t DestSize, const wchar_t *Source, size_t SourceSize);

extern WNDPROC OldEditorUI_WndProc;
extern DLGPROC OldEditorUI_ObjectWindowProc;
extern DLGPROC OldEditorUI_CellViewProc;

void sub_141BAF3E0(__int64 rcx0, __int64 a2);
//Detours::X64::DetourFunctionClass((PBYTE)(OFFSET(0x1BAF3E0), 1530), &sub_141BAF3E0);

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
		SetUnhandledExceptionFilter(DumpExceptionHandler);

		XUtil::PatchMemory(OFFSET(0x247D650, 1530), (PBYTE)"\xC3", 1);	// StackTrace::MemoryTraceWrite
		XUtil::PatchMemory(OFFSET(0x24801DF, 1530), (PBYTE)"\xC3", 1);	// SetUnhandledExceptionFilter, BSWin32ExceptionHandler
		XUtil::PatchMemory(OFFSET(0x2E558DB, 1530), (PBYTE)"\xC3", 1);	// SetUnhandledExceptionFilter, Unknown
		XUtil::PatchMemoryNop(OFFSET(0x24801FB, 1530), 6);				// SetUnhandledExceptionFilter, BSWin32ExceptionHandler

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
	ExperimentalPatchOptimizations();

	//
	// BSPointerHandle(Manager)
	//
	if (g_INI.GetBoolean("CreationKit", "RefrHandleLimitPatch", false))
	{
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

		//
		// Stub out the rest of the functions which shouldn't ever be called now
		//
		//XUtil::PatchMemory(OFFSET(0x12E0DC0, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::BSUntypedPointerHandle - 1412E0DC0
		XUtil::PatchMemory(OFFSET(0x12E38A0, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::Clear - 1412E38A0
		XUtil::PatchMemory(OFFSET(0x12E2720, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::SetAge - 1412E2720
		XUtil::PatchMemory(OFFSET(0x12E3970, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::SetActive - 1412E3970
		XUtil::PatchMemory(OFFSET(0x1294740, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetAge_0 - 141294740
		XUtil::PatchMemory(OFFSET(0x12E3810, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::Set - 1412E3810
		XUtil::PatchMemory(OFFSET(0x12E2FF0, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetIndex_0 - 1412E2FF0
		XUtil::PatchMemory(OFFSET(0x1294A30, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetIndex - 141294A30
		XUtil::PatchMemory(OFFSET(0x1294720, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::GetAge - 141294720
		XUtil::PatchMemory(OFFSET(0x1297430, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::ClearActive - 141297430
		XUtil::PatchMemory(OFFSET(0x12973F0, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::SetIndex - 1412973F0
		XUtil::PatchMemory(OFFSET(0x12943B0, 1530), (PBYTE)"\xCC", 1);// BSUntypedPointerHandle::IsBitwiseNull - 1412943B0
		// sub_14100B0A8 - Unknown operator
		// sub_1412E1300 - Unknown operator
		// sub_1412E1210 - Unknown operator

		XUtil::PatchMemory(OFFSET(0x1294590, 1530), (PBYTE)"\xCC", 1);// BSPointerHandle::AgeMatches - 141294590
		XUtil::PatchMemory(OFFSET(0x128D130, 1530), (PBYTE)"\xCC", 1);// BSPointerHandle::GetPtr - 14128D130
		XUtil::PatchMemory(OFFSET(0x128C8D0, 1530), (PBYTE)"\xCC", 1);// BSPointerHandle::AssignPtr - 14128C8D0
		XUtil::PatchMemory(OFFSET(0x1294570, 1530), (PBYTE)"\xCC", 1);// BSPointerHandle::IsActive - 141294570

		XUtil::PatchMemory(OFFSET(0x12E3900, 1530), (PBYTE)"\xCC", 1);// BSHandleRefObject::AssignHandleIndex - 1412E3900
		XUtil::PatchMemory(OFFSET(0x12949D0, 1530), (PBYTE)"\xCC", 1);// BSHandleRefObject::GetIndex - 1412949D0
		// BSHandleRefObject::QRefCount - 141294CB0
	}

	//
	// FaceGen
	//

	// Disable automatic FaceGen on save
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableAutoFaceGen", false))
		XUtil::PatchMemory(OFFSET(0x18DE530, 1530), (PBYTE)"\xC3", 1);

	// Don't produce DDS files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportDDS", false))
		XUtil::PatchMemoryNop(OFFSET(0x1904318, 1530), 5);

	// Don't produce TGA files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportTGA", false))
		XUtil::PatchMemoryNop(OFFSET(0x190436B, 1530), 5);

	// Don't produce NIF files
	if (g_INI.GetBoolean("CreationKit_FaceGen", "DisableExportNIF", false))
		XUtil::PatchMemory(OFFSET(0x1904390, 1530), (PBYTE)"\xC3", 1);

	// Allow variable tint mask resolution
	uint32_t tintResolution = g_INI.GetInteger("CreationKit_FaceGen", "TintMaskResolution", 512);
	XUtil::PatchMemory(OFFSET(0x2DA588C, 1530), (PBYTE)&tintResolution, sizeof(uint32_t));
	XUtil::PatchMemory(OFFSET(0x2DA5899, 1530), (PBYTE)&tintResolution, sizeof(uint32_t));

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
	XUtil::DetourJump(OFFSET(0x13D29B0, 1530), &EditorUI_LipRecordDialogProc);

	//
	// MemoryManager
	//
	if (g_INI.GetBoolean("CreationKit", "MemoryPatch", false))
	{
		PatchMemory();

		XUtil::PatchMemory(OFFSET(0x1223160, 1530), (PBYTE)"\xC3", 1);					// [3GB  ] MemoryManager - Default/Static/File heaps
		XUtil::PatchMemory(OFFSET(0x24400E0, 1530), (PBYTE)"\xC3", 1);					// [1GB  ] BSSmallBlockAllocator
		XUtil::DetourJump(OFFSET(0x257D740, 1530), &bhkThreadMemorySource::__ctor__);	// [512MB] bhkThreadMemorySource
		XUtil::PatchMemory(OFFSET(0x2447D90, 1530), (PBYTE)"\xC3", 1);					// [64MB ] ScrapHeap init
		XUtil::PatchMemory(OFFSET(0x24488C0, 1530), (PBYTE)"\xC3", 1);					// [64MB ] ScrapHeap deinit
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
		*(uint8_t **)&NavMesh::DeleteTriangle = Detours::X64::DetourFunctionClass((PBYTE)OFFSET(0x1D618E0, 1530), &NavMesh::hk_DeleteTriangle);

		XUtil::DetourCall(OFFSET(0x1D6984F, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		XUtil::DetourCall(OFFSET(0x1D699E6, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);
		XUtil::DetourCall(OFFSET(0x1D69B80, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_DegenerateCheck);

		XUtil::DetourCall(OFFSET(0x1D6A42B, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);
		XUtil::DetourCall(OFFSET(0x1D6A580, 1530), &BSNavmesh::BSNavmeshTriangle::hk_GetVertexIndex_VertexCheck);

		XUtil::PatchMemory(OFFSET(0x1FF9BAC, 1530), (PBYTE)"\xE9\xA1\x01\x00\x00", 5);// Prevent vertices from being deleted separately
	}

	//
	// TESForm
	//
	XUtil::DetourJump(OFFSET(0x16C0650, 1530), &FormReferenceMap_RemoveAllEntries);
	XUtil::DetourJump(OFFSET(0x16C0A90, 1530), &FormReferenceMap_FindOrCreate);
	XUtil::DetourJump(OFFSET(0x16C0B50, 1530), &FormReferenceMap_RemoveEntry);
	XUtil::DetourJump(OFFSET(0x146C130, 1530), &FormReferenceMap_Get);

	XUtil::PatchMemory(OFFSET(0x16C081E, 1530), (PBYTE)"\xCC", 1);
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

	if (g_INI.GetBoolean("CreationKit", "UI", false))
	{
		EditorUI_Initialize();
		*(uint8_t **)&OldEditorUI_WndProc = Detours::X64::DetourFunctionClass((PBYTE)OFFSET(0x13F3770, 1530), &EditorUI_WndProc);
		*(uint8_t **)&OldEditorUI_ObjectWindowProc = Detours::X64::DetourFunctionClass((PBYTE)OFFSET(0x12C3ED0, 1530), &EditorUI_ObjectWindowProc);
		*(uint8_t **)&OldEditorUI_CellViewProc = Detours::X64::DetourFunctionClass((PBYTE)OFFSET(0x13D8F40, 1530), &EditorUI_CellViewProc);

		XUtil::DetourCall(OFFSET(0x1CF03C9, 1530), &hk_call_141CF03C9);// Update the UI options when fog is toggled
		XUtil::DetourCall(OFFSET(0x12D1541, 1530), &hk_call_1412D1541);// Allow forms to be filtered in EditorUI_ObjectWindowProc
		XUtil::DetourCall(OFFSET(0x147FB57, 1530), &hk_call_14147FB57);// Allow forms to be filtered in EditorUI_CellViewProc
		XUtil::DetourCall(OFFSET(0x1C9879C, 1530), &hk_call_14147FB57);// ^
		XUtil::PatchMemoryNop(OFFSET(0x1434473, 1530), 2);				// Force bShowReloadShadersButton to always be enabled
		XUtil::PatchMemoryNop(OFFSET(0x1487B69, 1530), 2);				// Enable push to game button even if version control is disabled
		XUtil::PatchMemory(OFFSET(0x1487B7C, 1530), (PBYTE)"\xEB", 1);	// ^
		XUtil::PatchMemory(OFFSET(0x16179C0, 1530), (PBYTE)"\xC3", 1);	// Disable "MEM_CATEGORY_X" log spam
		XUtil::PatchMemoryNop(OFFSET(0x2DCE6BC, 1530), 5);				// Disable "utility failed id" log spam
		XUtil::PatchMemoryNop(OFFSET(0x2D270E3, 1530), 5);				// Disable "Should have been converted offline" log spam

		XUtil::DetourJump(OFFSET(0x1256600, 1530), &EditorUI_Warning);
		XUtil::DetourJump(OFFSET(0x243D610, 1530), &EditorUI_Warning);
		XUtil::DetourJump(OFFSET(0x1CD29E0, 1530), &EditorUI_Warning);
		XUtil::DetourJump(OFFSET(0x122C5F0, 1530), &EditorUI_WarningUnknown1);
		XUtil::DetourJump(OFFSET(0x137FC60, 1530), &EditorUI_WarningUnknown1);
		XUtil::DetourJump(OFFSET(0x1FCB030, 1530), &EditorUI_WarningUnknown1);
		XUtil::DetourJump(OFFSET(0x2452480, 1530), &EditorUI_WarningUnknown1);
		XUtil::DetourJump(OFFSET(0x243D5A0, 1530), &EditorUI_WarningUnknown1);
		XUtil::DetourJump(OFFSET(0x27A6150, 1530), &EditorUI_WarningUnknown2);
		XUtil::DetourJump(OFFSET(0x27A6270, 1530), &EditorUI_WarningUnknown2);
		XUtil::DetourCall(OFFSET(0x163D3D1, 1530), &EditorUI_WarningUnknown2);
		XUtil::DetourJump(OFFSET(0x243D260, 1530), &EditorUI_Assert);
	}

	if (g_INI.GetBoolean("CreationKit", "DisableWindowGhosting", false))
	{
		DisableProcessWindowsGhosting();
	}

	// Deferred dialog loading (batched UI updates)
	PatchTemplatedFormIterator();
	XUtil::DetourJump(OFFSET(0x13B9AD0, 1530), &InsertComboBoxItem);
	XUtil::DetourJump(OFFSET(0x13BA4D0, 1530), &InsertListViewItem);
	XUtil::DetourJump(OFFSET(0x20A9710, 1530), &EditorUI_CSScript_PickScriptsToCompileDlgProc);
	XUtil::DetourJump(OFFSET(0x1985F20, 1530), &SortDialogueInfo);
	XUtil::DetourCall(OFFSET(0x12C8B63, 1530), &UpdateObjectWindowTreeView);
	XUtil::DetourCall(OFFSET(0x13DAB04, 1530), &UpdateCellViewCellList);
	XUtil::DetourCall(OFFSET(0x13E117C, 1530), &UpdateCellViewObjectList);

	// Disable useless "Processing Topic X..." status bar updates
	XUtil::PatchMemoryNop(OFFSET(0x199DE29, 1530), 5);
	XUtil::PatchMemoryNop(OFFSET(0x199EA9E, 1530), 5);
	XUtil::PatchMemoryNop(OFFSET(0x199DA62, 1530), 5);

	//
	// Allow saving ESM's directly
	//
	if (g_INI.GetBoolean("CreationKit", "AllowSaveESM", false))
	{
		// Also allow non-game ESMs to be set as "Active File"
		XUtil::DetourCall(OFFSET(0x13E2D37, 1530), &TESFile::IsActiveFileBlacklist);
		XUtil::PatchMemoryNop(OFFSET(0x163CA2E, 1530), 2);

		*(uint8_t **)&TESFile::LoadTESInfo = Detours::X64::DetourFunctionClass((PBYTE)OFFSET(0x1664CC0, 1530), &TESFile::hk_LoadTESInfo);
		*(uint8_t **)&TESFile::WriteTESInfo = Detours::X64::DetourFunctionClass((PBYTE)OFFSET(0x1665520, 1530), &TESFile::hk_WriteTESInfo);

		// Disable: "File '%s' is a master file or is in use.\n\nPlease select another file to save to."
		const char *newFormat = "File '%s' is in use.\n\nPlease select another file to save to.";

		XUtil::PatchMemoryNop(OFFSET(0x164020A, 1530), 12);
		XUtil::PatchMemory(OFFSET(0x30B9090, 1530), (PBYTE)newFormat, strlen(newFormat) + 1);

		XUtil::DetourJump(OFFSET(0x1482DA0, 1530), &OpenPluginSaveDialog);
	}

	//
	// Allow ESP files to act as master files while saving
	//
	if (g_INI.GetBoolean("CreationKit", "AllowMasterESP", false))
	{
		XUtil::PatchMemoryNop(OFFSET(0x1657279, 1530), 12);
	}

	//
	// Skip 'Topic Info' validation during load
	//
	if (g_INI.GetBoolean("CreationKit", "SkipTopicInfoValidation", false))
	{
		XUtil::PatchMemory(OFFSET(0x19A83C0, 1530), (PBYTE)"\xC3", 1);
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
		XUtil::PatchMemory(OFFSET(0x1306978, 1530), (PBYTE)"\x01", 1);
	}

	//
	// Memory bug fix during BSShadowDirectionalLight calculations (see game patch for more information)
	//
	XUtil::PatchMemory(OFFSET(0x2DC679D, 1530), (PBYTE)"\x4D\x89\xE1\x90\x90\x90\x90", 7);

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
	XUtil::PatchMemoryNop(OFFSET(0x2E2F0AE, 1530), 5);		// Pointer always null (second parameter)
	XUtil::PatchMemoryNop(OFFSET(0x2E2F270, 1530), 5);		// Pointer always null (second parameter)
	XUtil::PatchMemoryNop(OFFSET(0x2E2F275, 1530), 38);		// Assert always triggers
	XUtil::PatchMemoryNop(OFFSET(0x2E2F29B, 1530), 41);		// Assert always triggers
	XUtil::PatchMemoryNop(OFFSET(0x2E2F2C4, 1530), 22);		// Multiple null pointers in call
	XUtil::PatchMemoryNop(OFFSET(0x2E2F2E4, 1530), 546);	// Remove most of the useless stuff in the function

	XUtil::PatchMemory(OFFSET(0x2E2BC50, 1530), (PBYTE)"\xC3", 1);// Pointer always null (BSGraphics::State::UpdateTemporalData)
	XUtil::PatchMemory(OFFSET(0x2E2BAF0, 1530), (PBYTE)"\xC3", 1);// Pointer always null (BSGraphics::State::UpdateTemporalData)

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
	XUtil::PatchMemory(OFFSET(0x20CD744, 1530), (PBYTE)"\x02", 1);

	//
	// Rewrite their ray->triangle intersection function. This fixes 3 things:
	//
	// - Being unable to select certain objects in the render view window.
	// - Selections not scaling correctly depending on distance (ex. LOD terrain) and NiObject scale.
	// - The Object Palette window "Conform to slope" option causing broken object angles on placement. SE changed data
	// layouts and geometry vertex data may not include normals.
	//
	Detours::X64::DetourClassVTable((PBYTE)OFFSET(0x345ECD0, 1530), &BSShaderResourceManager_CK::FindIntersectionsTriShapeFastPath, 34);

	//
	// Fix the "Cell View" object list current selection not being synced with the render window
	//
	XUtil::DetourJump(OFFSET(0x13BB650, 1530), &EditorUI_ListViewSelectItem);
	XUtil::DetourJump(OFFSET(0x13BB590, 1530), &EditorUI_ListViewFindAndSelectItem);
	XUtil::DetourJump(OFFSET(0x13BB720, 1530), &EditorUI_ListViewDeselectItem);

	//
	// Fix TESModelTextureSwap being incorrectly loaded (Record typo: 'MODS' -> 'MO5S')
	//
	XUtil::PatchMemory(OFFSET(0x16E55A9, 1530), (PBYTE)"\x4D\x4F\x35\x53", 4);

	//
	// Correct the "Push-to-game not supported" error when clicking the "UnEquip Sound" button on the weapon editor
	// dialog. 3682 is reserved exclusively for the PTG functionality, so the button id must be changed. Remapped to
	// 3683 instead.
	//
	XUtil::DetourJump(OFFSET(0x13B9900, 1530), &EditorUI_DialogTabProc);

	uint32_t newId = 3683;
	XUtil::PatchMemory(OFFSET(0x1B0CBC4, 1530), (PBYTE)&newId, sizeof(uint32_t));// SetDlgItemTextA
	XUtil::PatchMemory(OFFSET(0x1B0DCE9, 1530), (PBYTE)&newId, sizeof(uint32_t));// GetDlgItemTextA
	newId += 1;
	XUtil::PatchMemory(OFFSET(0x1B0AFAA, 1530), (PBYTE)&newId, sizeof(uint32_t));// Patch if() comparison

	//
	// Fix for crash (recursive sorting function stack overflow) when saving certain ESP files (i.e 3DNPC.esp)
	//
	XUtil::DetourJump(OFFSET(0x1651590, 1530), &ArrayQuickSortRecursive_TESForm);

	//
	// Fix for incorrect NavMesh assertion while saving certain ESP files (i.e 3DNPC.esp)
	//
	XUtil::DetourCall(OFFSET(0x159EB48, 1530), &hk_sub_141047AB2);

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
	XUtil::PatchMemory(OFFSET(0x1D3B350, 1530), (PBYTE)"\x48\x8B\x4C\x24\x68\xE8\xCB\xFF\xFF\xFF\xE9\x7D\x01\x00\x00", 15);
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
	XUtil::PatchMemory(OFFSET(0x1C62AD8, 1530), (PBYTE)"\xEB", 1);

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
	XUtil::PatchMemory(OFFSET(0x31027F8, 1530), (PBYTE)", Text \"%.240s\"", strlen(", Text \"%.240s\"") + 1);

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
	XUtil::PatchMemory(OFFSET(0x1C41094, 1530), (PBYTE)"\x48\x8B\xC1\x90\x90", 5);
	XUtil::PatchMemory(OFFSET(0x1C41115, 1530), (PBYTE)"\x48\x8B\xC1\x90\x90", 5);

	//
	// Fix for "File in use" UI hang after hitting cancel. It tries to use the main window handle as a parent, but it's suspended
	// during the initial data load, resulting in a deadlock. The new BGSThreadedProgressDlg causes this.
	//
	XUtil::PatchMemory(OFFSET(0x16641B1, 1530), (PBYTE)"\x4D\x33\xC0\x90\x90\x90\x90", 7);

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
	XUtil::PatchMemory(OFFSET(0x1CA2CBD, 1530), (PBYTE)"\xBA\x0C\x00\x00\x00", 5);
	XUtil::PatchMemory(OFFSET(0x1CA2E64, 1530), (PBYTE)"\xBA\x0C\x00\x00\x00", 5);

	//
	// Increase the maximum navmesh autogeneration cell limit to 100,000 and prevent spamming UI updates (0.01% -> 1.00%)
	//
	XUtil::PatchMemory(OFFSET(0x202F0B6, 1530), (PBYTE)"\xA0\x86\x01\x00", 4);
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
	// Fix for the "Object Palette" preview window not working. Window render state has to be set to '2'.
	//
	XUtil::DetourCall(OFFSET(0x12DD706, 1530), &hk_call_1412DD706);

	//
	// Fix for crash when cell references are added/removed during initialization, similar to the broken iterator in InventoryChanges
	//
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
	// Plugin loading optimizations:
	//
	// - TESForm reference map rewrite (above)
	// - Fix an unoptimized function bottleneck (sub_1414974E0) (Large ESP files only)
	// - Fix an unoptimized function bottleneck (sub_1415D5640)
	// - Eliminate millions of calls to update the progress dialog, instead only updating 400 times (0% -> 100%)
	// - Replace old zlib decompression code with optimized libdeflate
	// - Cache results from FindFirstFile when GetFileAttributesExA is called immediately after (sub_142647AC0, BSResource__LooseFileLocation__FileExists)
	//
	int cpuinfo[4];
	__cpuid(cpuinfo, 1);

	// Utilize SSE4.1 instructions if available
	if ((cpuinfo[2] & (1 << 19)) != 0)
		XUtil::DetourJump(OFFSET(0x14974E0, 1530), &sub_1414974E0_SSE41);
	else
		XUtil::DetourJump(OFFSET(0x14974E0, 1530), &sub_1414974E0);

	XUtil::DetourJump(OFFSET(0x15D5640, 1530), &sub_1415D5640);
	XUtil::PatchMemory(OFFSET(0x163D56E, 1530), (PBYTE)"\xB9\x90\x01\x00\x00\x90", 6);
	XUtil::DetourCall(OFFSET(0x1640FF3, 1530), &UpdateLoadProgressBar);
	XUtil::DetourCall(OFFSET(0x166BB1E, 1530), &hk_inflateInit);
	XUtil::DetourCall(OFFSET(0x166BBB9, 1530), &hk_inflate);
	XUtil::DetourJump(OFFSET(0x2647AC0, 1530), &sub_142647AC0);
	XUtil::DetourJump(OFFSET(0x2676020, 1530), &BSResource__LooseFileLocation__FileExists);

	// Force multiple master loads
	//XUtil::PatchMemory(OFFSET(0x163CDF3, 1530), (PBYTE)"\xEB", 1);
}