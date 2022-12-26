//////////////////////////////////////////
/*
* Copyright (c) 2020 Nukem9 <email:Nukem@outlook.com>
* Copyright (c) 2022 Perchik71 <email:perchik71@outlook.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this
* software and associated documentation files (the "Software"), to deal in the Software
* without restriction, including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or
* substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
* PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
* FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
//////////////////////////////////////////

#pragma once

#include "../../common.h"

#define UI_SETACTIVEPLUGIN_BUTTON					1121	// See: resource.rc

#define UI_EDITOR_TOOLBAR							1
#define UI_EDITOR_STATUSBAR							40139
#define UI_EDITOR_TOGGLEOBJECTWND					40199
#define UI_EDITOR_TOGGLEFOG							40937	// "View" menu
#define UI_EDITOR_TOGGLEGRASS_BUTTON				40960	// Main toolbar
#define UI_EDITOR_TOGGLEGRASS						40963	// "View" menu
#define UI_EDITOR_OPENFORMBYID						52001	// Sent from the LogWindow on double click

#define UI_OBJECT_WINDOW_ADD_ITEM					2579
#define UI_OBJECT_WINDOW_CHECKBOX					2580	// See: resource.rc

#define UI_CELL_VIEW_ADD_CELL_ITEM					2579
#define UI_CELL_VIEW_ADD_CELL_OBJECT_ITEM			2583
#define UI_CELL_VIEW_ACTIVE_CELLS_CHECKBOX			2580	// See: resource.rc
#define UI_CELL_VIEW_ACTIVE_CELL_OBJECTS_CHECKBOX	2582	// See: resource.rc

#define UI_LIPRECORD_DIALOG_STOPRECORD				1
#define UI_LIPRECORD_DIALOG_PROGRESSBAR				31007
#define UI_LIPRECORD_DIALOG_STARTRECORD				(WM_USER + 22)

#define UI_COMIPLESCRIPT_DIALOG_COMPILE				1		// "Compile"
#define UI_COMIPLESCRIPT_DIALOG_CHECKALL			5474	// "Check All"
#define UI_COMIPLESCRIPT_DIALOG_UNCHECKALL			5475	// "Uncheck All"
#define UI_COMIPLESCRIPT_DIALOG_CHECKALLCHECKEDOUT	5602	// "Check All Checked-Out"
#define UI_COMIPLESCRIPT_DIALOG_LISTVIEW			5401	// Script list view

#define UI_DATA_DIALOG_PLUGINLISTVIEW				1056
#define UI_DATA_DIALOG_FILTERBOX					1003	// See: resource.rc

#define UI_LOG_CMD_ADDTEXT							(WM_APP + 1)
#define UI_LOG_CMD_CLEARTEXT						(WM_APP + 2)
#define UI_LOG_CMD_AUTOSCROLL						(WM_APP + 3)

#define UI_EXTMENU_ID								51001
#define UI_EXTMENU_SHOWLOG							51002
#define UI_EXTMENU_CLEARLOG							51003
#define UI_EXTMENU_AUTOSCROLL						51004
#define UI_EXTMENU_SPACER							51005
#define UI_EXTMENU_DUMPRTTI							51006
#define UI_EXTMENU_DUMPNIRTTI						51007
#define UI_EXTMENU_DUMPHAVOKRTTI					51008
#define UI_EXTMENU_LOADEDESPINFO					51009
#define UI_EXTMENU_HARDCODEDFORMS					51010

namespace EditorUI
{
	void Initialize();

	HWND WINAPI hk_CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
	BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult);
	LRESULT WINAPI hk_SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK DialogFuncOverride(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL WINAPI hk_EnableWindow(HWND hwndDlg, BOOL bEnable);

	void ListViewInsertItemDeferred(HWND ListViewHandle, void *Parameter, bool UseImage, int ItemIndex);
	BOOL ListViewSetItemState(HWND ListViewHandle, WPARAM Index, UINT Data, UINT Mask);
	void ListViewSelectItem(HWND ListViewHandle, int ItemIndex, bool KeepOtherSelections);
	void ListViewFindAndSelectItem(HWND ListViewHandle, void *Parameter, bool KeepOtherSelections);
	void *ListViewGetSelectedItem(HWND ListViewHandle);
	void ListViewDeselectItem(HWND ListViewHandle, void *Parameter);
	void ComboBoxInsertItemDeferred(HWND ComboBoxHandle, const char *DisplayText, void *Value, bool AllowResize);
	void TabControlDeleteItem(HWND TabControlHandle, uint32_t TabIndex);
	
	void* ListViewGetUserData(HWND ListViewHandle, int ItemIndex);

	void RegisterHotkeyFunction(void *Thisptr, void(*Callback)(), const char **HotkeyFunction, const char **DisplayText, char VirtualKey, bool Alt, bool Ctrl, bool Shift);
	void ResetUIDefer();
	void BeginUIDefer();
	void EndUIDefer();
	void SuspendComboBoxUpdates(HWND ComboHandle, bool Suspend);
}