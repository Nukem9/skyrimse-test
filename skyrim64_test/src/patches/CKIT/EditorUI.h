#pragma once

#include "../../common.h"

#define UI_EDITOR_TOOLBAR				1
#define UI_EDITOR_STATUSBAR				40139
#define UI_EDITOR_TOGGLEFOG				40937	// "View" menu
#define UI_EDITOR_TOGGLEGRASS_BUTTON	40960	// Main toolbar
#define UI_EDITOR_TOGGLEGRASS			40963	// "View" menu

#define UI_OBJECT_WINDOW_ADD_ITEM		2579
#define UI_OBJECT_WINDOW_CHECKBOX		2580	// See: resource.rc

#define UI_LOG_CMD_ADDTEXT				(WM_APP + 1)
#define UI_LOG_CMD_CLEARTEXT			(WM_APP + 2)
#define UI_LOG_CMD_AUTOSCROLL			(WM_APP + 3)

#define UI_EXTMENU_ID					51001
#define UI_EXTMENU_SHOWLOG				51002
#define UI_EXTMENU_CLEARLOG				51003
#define UI_EXTMENU_AUTOSCROLL			51004
#define UI_EXTMENU_SPACER				51005
#define UI_EXTMENU_DUMPRTTI				51006
#define UI_EXTMENU_DUMPNIRTTI			51007
#define UI_EXTMENU_DUMPHAVOKRTTI		51008
#define UI_EXTMENU_LOADEDESPINFO		51009
#define UI_EXTMENU_HARDCODEDFORMS		51010

void EditorUI_Initialize();
bool EditorUI_CreateExtensionMenu(HWND MainWindow, HMENU MainMenu);
HWND EditorUI_GetMainWindow();

LRESULT CALLBACK EditorUI_WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditorUI_DialogTabProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EditorUI_LipRecordDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EditorUI_ObjectWindowProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT EditorUI_CSScript_PickScriptsToCompileDlgProc(void *This, UINT Message, WPARAM wParam, LPARAM lParam);

BOOL EditorUI_ListViewCustomSetItemState(HWND ListViewHandle, WPARAM Index, UINT Data, UINT Mask);
void EditorUI_ListViewSelectItem(HWND ListViewHandle, int ItemIndex, bool KeepOtherSelections);
void EditorUI_ListViewFindAndSelectItem(HWND ListViewHandle, void *Parameter, bool KeepOtherSelections);
void EditorUI_ListViewDeselectItem(HWND ListViewHandle, void *Parameter);