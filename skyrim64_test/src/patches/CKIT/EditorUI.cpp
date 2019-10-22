#include "../../common.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include "../../typeinfo/ms_rtti.h"
#include "../../typeinfo/hk_rtti.h"
#include "EditorUI.h"
#include "LogWindow.h"

#pragma comment(lib, "comctl32.lib")

HWND g_MainHwnd;
HMENU g_ExtensionMenu;
void ExportTest(FILE *File);

WNDPROC OldEditorUI_WndProc;
DLGPROC OldEditorUI_ObjectWindowProc;

void EditorUI_Initialize()
{
	InitCommonControls();

	if (!EditorUI_CreateLogWindow())
		MessageBoxA(nullptr, "Failed to create console log window", "Error", MB_ICONERROR);

	if (g_INI.GetBoolean("CreationKit", "FaceFXDebugOutput", false))
	{
		if (!EditorUI_CreateStdoutListener())
			MessageBoxA(nullptr, "Failed to create output listener for external processes", "Error", MB_ICONERROR);
	}
}

bool EditorUI_CreateExtensionMenu(HWND MainWindow, HMENU MainMenu)
{
	// Create extended menu options
	g_ExtensionMenu = CreateMenu();

	BOOL result = TRUE;
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_SHOWLOG, "Show Log");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_CLEARLOG, "Clear Log");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING | MF_CHECKED, (UINT_PTR)UI_EXTMENU_AUTOSCROLL, "Autoscroll Log");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_SEPARATOR, (UINT_PTR)UI_EXTMENU_SPACER, "");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_DUMPRTTI, "Dump RTTI Data");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_DUMPNIRTTI, "Dump NiRTTI Data");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_DUMPHAVOKRTTI, "Dump Havok RTTI Data");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_LOADEDESPINFO, "Dump Active Forms");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_SEPARATOR, (UINT_PTR)UI_EXTMENU_SPACER, "");
	result = result && InsertMenu(g_ExtensionMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_HARDCODEDFORMS, "Save Hardcoded Forms");

	MENUITEMINFO menuInfo;
	memset(&menuInfo, 0, sizeof(MENUITEMINFO));
	menuInfo.cbSize = sizeof(MENUITEMINFO);
	menuInfo.fMask = MIIM_SUBMENU | MIIM_ID | MIIM_STRING;
	menuInfo.hSubMenu = g_ExtensionMenu;
	menuInfo.wID = UI_EXTMENU_ID;
	menuInfo.dwTypeData = "Extensions";
	menuInfo.cch = (uint32_t)strlen(menuInfo.dwTypeData);
	result = result && InsertMenuItem(MainMenu, -1, TRUE, &menuInfo);

	AssertMsg(result, "Failed to create extension submenu");
	return result != FALSE;
}

HWND EditorUI_GetMainWindow()
{
	return g_MainHwnd;
}

LRESULT CALLBACK EditorUI_WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_CREATE)
	{
		const CREATESTRUCT *createInfo = (CREATESTRUCT *)lParam;

		if (!_stricmp(createInfo->lpszName, "Creation Kit") && !_stricmp(createInfo->lpszClass, "Creation Kit"))
		{
			// Initialize the original window before adding anything
			LRESULT status = CallWindowProc(OldEditorUI_WndProc, Hwnd, Message, wParam, lParam);
			g_MainHwnd = Hwnd;

			// Increase status bar spacing
			int spacing[4] =
			{
				200,	// 150
				300,	// 225
				700,	// 500
				-1,		// -1
			};

			SendMessageA(GetDlgItem(Hwnd, UI_EDITOR_STATUSBAR), SB_SETPARTS, ARRAYSIZE(spacing), (LPARAM)& spacing);

			// Grass is always enabled by default, make the UI buttons match
			CheckMenuItem(GetMenu(Hwnd), UI_EDITOR_TOGGLEGRASS, MF_CHECKED);
			SendMessageA(GetDlgItem(Hwnd, UI_EDITOR_TOOLBAR), TB_CHECKBUTTON, UI_EDITOR_TOGGLEGRASS_BUTTON, TRUE);

			// Same for fog
			CheckMenuItem(GetMenu(Hwnd), UI_EDITOR_TOGGLEFOG, *(bool *)(g_ModuleBase + 0x4F05728) ? MF_CHECKED : MF_UNCHECKED);

			// Create custom menu controls
			EditorUI_CreateExtensionMenu(Hwnd, createInfo->hMenu);

			return status;
		}
	}
	else if (Message == WM_COMMAND)
	{
		const uint32_t param = LOWORD(wParam);

		switch (param)
		{
		case UI_EDITOR_TOGGLEFOG:
		{
			// Call the CTRL+F5 hotkey function directly
			((void(__fastcall *)())(g_ModuleBase + 0x1319740))();
		}
		return 0;

		case UI_EXTMENU_SHOWLOG:
		{
			ShowWindow(EditorUI_GetLogWindow(), SW_SHOW);
			SetForegroundWindow(EditorUI_GetLogWindow());
		}
		return 0;

		case UI_EXTMENU_CLEARLOG:
		{
			PostMessageA(EditorUI_GetLogWindow(), UI_LOG_CMD_CLEARTEXT, 0, 0);
		}
		return 0;

		case UI_EXTMENU_AUTOSCROLL:
		{
			MENUITEMINFO info;
			info.cbSize = sizeof(MENUITEMINFO);
			info.fMask = MIIM_STATE;
			GetMenuItemInfo(g_ExtensionMenu, param, FALSE, &info);

			bool check = !((info.fState & MFS_CHECKED) == MFS_CHECKED);

			if (!check)
				info.fState &= ~MFS_CHECKED;
			else
				info.fState |= MFS_CHECKED;

			PostMessageA(EditorUI_GetLogWindow(), UI_LOG_CMD_AUTOSCROLL, (WPARAM)check, 0);
			SetMenuItemInfo(g_ExtensionMenu, param, FALSE, &info);
		}
		return 0;

		case UI_EXTMENU_DUMPRTTI:
		case UI_EXTMENU_DUMPNIRTTI:
		case UI_EXTMENU_DUMPHAVOKRTTI:
		case UI_EXTMENU_LOADEDESPINFO:
		{
			char filePath[MAX_PATH];
			memset(filePath, 0, sizeof(filePath));

			OPENFILENAME ofnData;
			memset(&ofnData, 0, sizeof(OPENFILENAME));
			ofnData.lStructSize = sizeof(OPENFILENAME);
			ofnData.lpstrFilter = "Text Files (*.txt)\0*.txt\0\0";
			ofnData.lpstrFile = filePath;
			ofnData.nMaxFile = ARRAYSIZE(filePath);
			ofnData.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
			ofnData.lpstrDefExt = "txt";

			if (FILE *f; GetSaveFileName(&ofnData) && fopen_s(&f, filePath, "w") == 0)
			{
				if (param == UI_EXTMENU_DUMPRTTI)
					MSRTTI::Dump(f);
				else if (param == UI_EXTMENU_DUMPNIRTTI)
					ExportTest(f);
				else if (param == UI_EXTMENU_DUMPHAVOKRTTI)
				{
					// Convert path to directory
					*strrchr(filePath, '\\') = '\0';
					HKRTTI::DumpReflectionData(filePath);
				}
				else if (param == UI_EXTMENU_LOADEDESPINFO)
				{
					struct VersionControlListItem
					{
						const char *EditorId;
						uint32_t FileOffset;
						char Type[4];
						uint32_t FileLength;
						char GroupType[4];
						uint32_t FormId;
						uint32_t VersionControlId;
						char _pad0[0x8];
					};
					static_assert_offset(VersionControlListItem, EditorId, 0x0);
					static_assert_offset(VersionControlListItem, FileOffset, 0x8);
					static_assert_offset(VersionControlListItem, Type, 0xC);
					static_assert_offset(VersionControlListItem, FileLength, 0x10);
					static_assert_offset(VersionControlListItem, GroupType, 0x14);
					static_assert_offset(VersionControlListItem, FormId, 0x18);
					static_assert_offset(VersionControlListItem, VersionControlId, 0x1C);
					static_assert(sizeof(VersionControlListItem) == 0x28);

					static std::vector<VersionControlListItem> formList;

					// Invoke the dialog, building form list
					void(*callback)(void *, int, VersionControlListItem *) = [](void *, int, VersionControlListItem *Data)
					{
						formList.push_back(*Data);
						formList.back().EditorId = _strdup(Data->EditorId);
					};

					XUtil::DetourCall(g_ModuleBase + 0x13E32B0, callback);
					CallWindowProcA((WNDPROC)(g_ModuleBase + 0x13E6270), Hwnd, WM_COMMAND, 1185, 0);

					// Sort by: form id, then name, then file offset
					std::sort(formList.begin(), formList.end(),
						[](const VersionControlListItem& A, const VersionControlListItem& B) -> bool
					{
						if (A.FormId == B.FormId)
						{
							if (int ret = _stricmp(A.EditorId, B.EditorId); ret != 0)
								return ret < 0;

							return A.FileOffset > B.FileOffset;
						}

						return A.FormId > B.FormId;
					});

					// Dump it to the log
					fprintf(f, "Type, Editor Id, Form Id, File Offset, File Length, Version Control Id\n");

					for (auto& item : formList)
					{
						fprintf(f, "%c%c%c%c,\"%s\",%08X,%u,%u,-%08X-\n",
							item.Type[0], item.Type[1], item.Type[2], item.Type[3],
							item.EditorId,
							item.FormId,
							item.FileOffset,
							item.FileLength,
							item.VersionControlId);

						free((void *)item.EditorId);
					}

					formList.clear();
				}

				fclose(f);
			}
		}
		return 0;

		case UI_EXTMENU_HARDCODEDFORMS:
		{
			auto getFormById = (__int64(__fastcall *)(uint32_t))(g_ModuleBase + 0x16B8780);

			for (uint32_t i = 0; i < 2048; i++)
			{
				__int64 form = getFormById(i);

				if (form)
				{
					(*(void(__fastcall **)(__int64, __int64))(*(__int64 *)form + 360))(form, 1);
					EditorUI_Log("SetFormModified(%08X)", i);
				}
			}

			// Fake the click on "Save"
			PostMessageA(Hwnd, WM_COMMAND, 40127, 0);
		}
		return 0;
		}
	}
	else if (Message == WM_SETTEXT && Hwnd == g_MainHwnd)
	{
		// Continue normal execution but with a custom string
		char customTitle[1024];
		sprintf_s(customTitle, "%s [CK64Fixes Rev. %s]", (const char *)lParam, g_GitVersion);

		return CallWindowProc(OldEditorUI_WndProc, Hwnd, Message, wParam, (LPARAM)customTitle);
	}

	return CallWindowProc(OldEditorUI_WndProc, Hwnd, Message, wParam, lParam);
}

LRESULT CALLBACK EditorUI_DialogTabProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_INITDIALOG)
	{
		// If it's the weapon sound dialog tab (id 3327), remap the "Unequip Sound" button (id 3682) to
		// a non-conflicting one (id 3683)
		char className[256];

		if (GetClassNameA(Hwnd, className, ARRAYSIZE(className)) > 0)
		{
			if (!strcmp(className, "WeaponClass"))
				SetWindowLongPtr(GetDlgItem(Hwnd, 3682), GWLP_ID, 3683);
		}

		ShowWindow(Hwnd, SW_HIDE);
		return 1;
	}

	return 0;
}

INT_PTR CALLBACK EditorUI_LipRecordDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	// Id's for "Recording..." dialog window
	switch (Message)
	{
	case WM_APP:
		// Don't actually kill the dialog, just hide it. It gets destroyed later when the parent window closes.
		SendMessageA(GetDlgItem(DialogHwnd, 31007), PBM_SETPOS, 0, 0);
		ShowWindow(DialogHwnd, SW_HIDE);
		PostQuitMessage(0);
		return TRUE;

	case 272:
		// OnSaveSoundFile
		SendMessageA(GetDlgItem(DialogHwnd, 31007), PBM_SETRANGE, 0, 32768 * 1000);
		SendMessageA(GetDlgItem(DialogHwnd, 31007), PBM_SETSTEP, 1, 0);
		return TRUE;

	case 273:
		// Stop recording
		if (LOWORD(wParam) != 1)
			return FALSE;

		*(bool *)(g_ModuleBase + 0x3AFAE28) = false;

		if (FAILED(((HRESULT(__fastcall *)(bool))(g_ModuleBase + 0x13D5310))(false)))
			MessageBoxA(DialogHwnd, "Error with DirectSoundCapture buffer.", "DirectSound Error", MB_ICONERROR);

		return EditorUI_LipRecordDialogProc(DialogHwnd, WM_APP, 0, 0);

	case 1046:
		// Start recording
		ShowWindow(DialogHwnd, SW_SHOW);
		*(bool *)(g_ModuleBase + 0x3AFAE28) = true;

		if (FAILED(((HRESULT(__fastcall *)(bool))(g_ModuleBase + 0x13D5310))(true)))
		{
			MessageBoxA(DialogHwnd, "Error with DirectSoundCapture buffer.", "DirectSound Error", MB_ICONERROR);
			return EditorUI_LipRecordDialogProc(DialogHwnd, WM_APP, 0, 0);
		}
		return TRUE;
	}

	return FALSE;
}

INT_PTR CALLBACK EditorUI_ObjectWindowProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_COMMAND)
	{
		const uint32_t param = LOWORD(wParam);

		switch (param)
		{
		case UI_OBJECT_WINDOW_CHECKBOX:
			// Force the list items to update as if it was by timer
			SendMessageA(DialogHwnd, WM_TIMER, 0x4D, 0);
			return TRUE;
		}
	}
	else if (Message == UI_OBJECT_WINDOW_ADD_ITEM)
	{
		const auto insertData = (__int64)wParam;
		const auto form = (__int64)lParam;
		const bool onlyActiveForms = SendMessage(GetDlgItem(DialogHwnd, UI_OBJECT_WINDOW_CHECKBOX), BM_GETCHECK, 0, 0) == BST_CHECKED;

		if (onlyActiveForms)
		{
			if (form && (*(uint32_t *)(form + 0x10) & 2) != 2)
				return TRUE;
		}

		return ((int(__fastcall *)(__int64, __int64))(g_ModuleBase + 0x12D3BD0))(insertData, form);
	}

	return OldEditorUI_ObjectWindowProc(DialogHwnd, Message, wParam, lParam);
}

LRESULT EditorUI_CSScript_PickScriptsToCompileDlgProc(void *This, UINT Message, WPARAM wParam, LPARAM lParam)
{
	thread_local bool disableListViewUpdates;

	auto updateListViewItems = [This]
	{
		if (!disableListViewUpdates)
			((void(__fastcall *)(void *))(g_ModuleBase + 0x20A9870))(This);
	};

	switch (Message)
	{
	case WM_SIZE:
		((void(__fastcall *)(void *))(g_ModuleBase + 0x20A9CF0))(This);
		break;

	case WM_NOTIFY:
	{
		LPNMHDR notification = (LPNMHDR)lParam;

		// "SysListView32" control
		if (notification->idFrom == 5401 && notification->code == LVN_ITEMCHANGED)
		{
			updateListViewItems();
			return 1;
		}
	}
	break;

	case WM_INITDIALOG:
		disableListViewUpdates = true;
		((void(__fastcall *)(void *))(g_ModuleBase + 0x20A99C0))(This);
		disableListViewUpdates = false;

		// Update it ONCE after everything is inserted
		updateListViewItems();
		break;

	case WM_COMMAND:
	{
		const uint32_t param = LOWORD(wParam);

		// "Check All", "Uncheck All", "Check All Checked-Out"
		if (param == 5474 || param == 5475 || param == 5602)
		{
			disableListViewUpdates = true;
			if (param == 5474)
				((void(__fastcall *)(void *))(g_ModuleBase + 0x20AA080))(This);
			else if (param == 5475)
				((void(__fastcall *)(void *))(g_ModuleBase + 0x20AA130))(This);
			else if (param == 5602)
				((void(__fastcall *)(void *))(g_ModuleBase + 0x20AA1E0))(This);
			disableListViewUpdates = false;

			updateListViewItems();
			return 1;
		}
		else if (param == 1)
		{
			// "Compile" button
			((void(__fastcall *)(void *))(g_ModuleBase + 0x20A9F30))(This);
		}
	}
	break;
	}

	return ((LRESULT(__fastcall *)(void *, UINT, WPARAM, LPARAM))(g_ModuleBase + 0x20ABD90))(This, Message, wParam, lParam);
}

BOOL EditorUI_ListViewCustomSetItemState(HWND ListViewHandle, WPARAM Index, UINT Data, UINT Mask)
{
	// Microsoft's implementation of this define is broken (ListView_SetItemState)
	LVITEMA lvi = {};
	lvi.mask = LVIF_STATE;
	lvi.state = Data;
	lvi.stateMask = Mask;

	return (BOOL)SendMessageA(ListViewHandle, LVM_SETITEMSTATE, Index, (LPARAM)&lvi);
}

void EditorUI_ListViewSelectItem(HWND ListViewHandle, int ItemIndex, bool KeepOtherSelections)
{
	if (!KeepOtherSelections)
		EditorUI_ListViewCustomSetItemState(ListViewHandle, -1, 0, LVIS_SELECTED);

	if (ItemIndex != -1)
	{
		ListView_EnsureVisible(ListViewHandle, ItemIndex, FALSE);
		EditorUI_ListViewCustomSetItemState(ListViewHandle, ItemIndex, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void EditorUI_ListViewFindAndSelectItem(HWND ListViewHandle, void *Parameter, bool KeepOtherSelections)
{
	if (!KeepOtherSelections)
		EditorUI_ListViewCustomSetItemState(ListViewHandle, -1, 0, LVIS_SELECTED);

	LVFINDINFOA findInfo;
	memset(&findInfo, 0, sizeof(findInfo));

	findInfo.flags = LVFI_PARAM;
	findInfo.lParam = (LPARAM)Parameter;

	int index = ListView_FindItem(ListViewHandle, -1, &findInfo);

	if (index != -1)
		EditorUI_ListViewSelectItem(ListViewHandle, index, KeepOtherSelections);
}

void EditorUI_ListViewDeselectItem(HWND ListViewHandle, void *Parameter)
{
	LVFINDINFOA findInfo;
	memset(&findInfo, 0, sizeof(findInfo));

	findInfo.flags = LVFI_PARAM;
	findInfo.lParam = (LPARAM)Parameter;

	int index = ListView_FindItem(ListViewHandle, -1, &findInfo);

	if (index != -1)
		EditorUI_ListViewCustomSetItemState(ListViewHandle, index, 0, LVIS_SELECTED);
}