#pragma once

#include "../../common.h"

void EditorUI_Initialize();
bool EditorUI_CreateLogWindow();
bool EditorUI_CreateExtensionMenu(HWND MainWindow, HMENU MainMenu);

void EditorUI_LogVa(const char *Format, va_list Va);
void EditorUI_Log(const char *Format, ...);
void EditorUI_Warning(int Type, const char *Format, ...);
void EditorUI_WarningUnknown1(const char *Format, ...);
void EditorUI_WarningUnknown2(__int64 Unused, const char *Format, ...);
void EditorUI_Assert(const char *File, int Line, const char *Message);

LRESULT CALLBACK EditorUI_WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditorUI_LogWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);