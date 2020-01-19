#pragma once

#include "../../common.h"
#include <Uxtheme.h>
#include <optional>

void EditorUIDarkMode_Initialize();
BOOL CALLBACK EditorUIDarkMode_EnumWindowsProc(HWND hWnd, LPARAM lParam);
std::optional<INT_PTR> EditorUIDarkMode_ApplyMessageHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditorUIDarkMode_ListViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK EditorUIDarkMode_MDIClientSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

DWORD WINAPI Comctl32GetSysColor(int nIndex);
HBRUSH WINAPI Comctl32GetSysColorBrush(int nIndex);
HRESULT WINAPI Comctl32DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect);
HRESULT WINAPI Comctl32DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect);