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

#include "..\..\common.h"
#include "UITheme\VarCommon.h"
#include <Uxtheme.h>
#include <vssym32.h>
#include <windowsx.h>
#include <dwmapi.h>

namespace UITheme {
	namespace Theme = Core::UI::Theme;

	enum class ThemeType {
		None,
		ScrollBar,
		StatusBar,
		MDIClient,
		Static,
		Edit,
		RichEdit20,
		RichEdit50,
		Button,
		ComboBox,
		Header,
		ListBox,
		ListView,
		TreeView,
		TabControl,
		ToolBar,
		TrackBar,
		ProgressBar,
		PopupMenu,
		Spin
	};

	VOID FIXAPI Initialize(Theme::Theme ThemeID);
	VOID FIXAPI InitializeThread(VOID);
	BOOL FIXAPI IsEnabledMode(VOID);

	// Returns a valid visual theme type, depending on the window class
	ThemeType FIXAPI GetThemeTypeFromWindow(HWND hWindow);
	// Binds the specified class type to the visual theme. hWindow takes only HTHEME
	// Returns TRUE if successful
	BOOL FIXAPI RegisterThemeHandle(HWND hWindow, ThemeType eTheme);
	// Binds the specified class type to the visual theme
	// Returns TRUE if successful
	BOOL FIXAPI RegisterThemeHandle(HTHEME hTheme, ThemeType eTheme);

	HWND FIXAPI Comctl32CreateToolbarEx_1(HWND hwnd, DWORD ws, UINT wID, INT nBitmaps, HINSTANCE hBMInst, UINT_PTR wBMID, LPCTBBUTTON lpButtons,
		INT iNumButtons, INT dxButton, INT dyButton, INT dxBitmap, INT dyBitmap, UINT uStructSize);
	HIMAGELIST FIXAPI Comctl32ImageList_LoadImageA_1(HINSTANCE hi, LPCSTR lpbmp, INT cx, INT cGrow, COLORREF crMask, UINT uType, UINT uFlags);
	HWND FIXAPI Comctl32CreateWindowEx_1(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, INT x, INT y,
		INT nWidth, INT nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	VOID FIXAPI HideOldTimeOfDayComponents(VOID);

	LRESULT CALLBACK CallWndProcCallback(INT nCode, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindowSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK DialogSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	DWORD FIXAPI Comctl32GetSysColor(INT nIndex);
	HBRUSH FIXAPI Comctl32GetSysColorBrush(INT nIndex);
	HRESULT FIXAPI Comctl32DrawThemeText(HTHEME hTheme, HDC hdc, INT iPartId, INT iStateId, LPCWSTR pszText, INT cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect);
	HRESULT FIXAPI Comctl32DrawThemeBackground(HTHEME hTheme, HDC hdc, INT iPartId, INT iStateId, LPCRECT pRect, LPCRECT pClipRect);
}