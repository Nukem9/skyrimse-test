#pragma once

#include "../../common.h"

bool EditorUI_CreateLogWindow();
bool EditorUI_CreateStdoutListener();
void EditorUI_GenerateWarningBlacklist();
HWND EditorUI_GetLogWindow();
HANDLE EditorUI_GetStdoutListenerPipe();

LRESULT CALLBACK EditorUI_LogWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

void EditorUI_LogVa(const char *Format, va_list Va);
void EditorUI_Log(const char *Format, ...);
void EditorUI_Warning(int Type, const char *Format, ...);
void EditorUI_WarningUnknown1(const char *Format, ...);
void EditorUI_WarningUnknown2(__int64 Unused, const char *Format, ...);
void EditorUI_Assert(const char *File, int Line, const char *Message, ...);