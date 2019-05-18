#pragma once

#include "../../common.h"

void EditorUI_Initialize();
bool EditorUI_CreateLogWindow();
bool EditorUI_CreateExtensionMenu(HWND MainWindow, HMENU MainMenu);
bool EditorUI_CreateStdoutListener();
HANDLE EditorUI_GetStdoutListenerPipe();

LRESULT CALLBACK EditorUI_WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditorUI_LogWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditorUI_DialogTabProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EditorUI_LipRecordDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT EditorUI_CSScript_PickScriptsToCompileDlgProc(void *Thisptr, UINT Message, WPARAM wParam, LPARAM lParam);

void EditorUI_LogVa(const char *Format, va_list Va);
void EditorUI_Log(const char *Format, ...);
void EditorUI_Warning(int Type, const char *Format, ...);
void EditorUI_WarningUnknown1(const char *Format, ...);
void EditorUI_WarningUnknown2(__int64 Unused, const char *Format, ...);
void EditorUI_Assert(const char *File, int Line, const char *Message, ...);

BOOL EditorUI_ListViewCustomSetItemState(HWND ListViewHandle, WPARAM Index, UINT Data, UINT Mask);
void EditorUI_ListViewSelectItem(HWND ListViewHandle, int ItemIndex, bool KeepOtherSelections);
void EditorUI_ListViewFindAndSelectItem(HWND ListViewHandle, void *Parameter, bool KeepOtherSelections);
void EditorUI_ListViewDeselectItem(HWND ListViewHandle, void *Parameter);