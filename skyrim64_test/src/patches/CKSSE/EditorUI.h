#pragma once

#include "../../common.h"

#define UI_EDITOR_TOOLBAR				1
#define UI_EDITOR_STATUSBAR				40139
#define UI_EDITOR_TOGGLEFOG				40937	// "View" menu
#define UI_EDITOR_TOGGLEGRASS_BUTTON	40960	// Main toolbar
#define UI_EDITOR_TOGGLEGRASS			40963	// "View" menu
#define UI_EDITOR_OPENFORMBYID			52001	// Sent from the LogWindow on double click

#define UI_OBJECT_WINDOW_ADD_ITEM		2579
#define UI_OBJECT_WINDOW_CHECKBOX		2580	// See: resource.rc

#define UI_CELL_VIEW_ADD_CELL_ITEM		2579
#define UI_CELL_VIEW_CHECKBOX			2580	// See: resource.rc

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

namespace EditorUI
{
	extern WNDPROC OldWndProc;
	extern DLGPROC OldObjectWindowProc;
	extern DLGPROC OldCellViewProc;

	HWND GetWindow();

	void Initialize();
	bool CreateExtensionMenu(HWND MainWindow, HMENU MainMenu);

	LRESULT CALLBACK WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK DialogTabProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK LipRecordDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK ObjectWindowProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK CellViewProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT CSScript_PickScriptsToCompileDlgProc(void *This, UINT Message, WPARAM wParam, LPARAM lParam);

	BOOL ListViewCustomSetItemState(HWND ListViewHandle, WPARAM Index, UINT Data, UINT Mask);
	void ListViewSelectItem(HWND ListViewHandle, int ItemIndex, bool KeepOtherSelections);
	void ListViewFindAndSelectItem(HWND ListViewHandle, void *Parameter, bool KeepOtherSelections);
	void ListViewDeselectItem(HWND ListViewHandle, void *Parameter);
}