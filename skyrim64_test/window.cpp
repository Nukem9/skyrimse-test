#include "stdafx.h"

WNDPROC g_OriginalWndProc;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP)
	{
		switch (wParam)
		{
		case VK_LWIN:		// Left windows key
		case VK_RWIN:		// Right windows key
		case VK_LSHIFT:		// Left shift
		case VK_RSHIFT:		// Right shift
		case VK_CAPITAL:	// Caps lock
			return DefWindowProcA(hwnd, uMsg, wParam, lParam);
		}
	}
	else if (uMsg == WM_ACTIVATEAPP)
	{
		// Fix alt-tab window activation
		if (wParam == FALSE)
		{
			ShowWindow(hwnd, SW_MINIMIZE);
			ShowCursor(true);
		}
		else
		{
			ShowWindow(hwnd, SW_MAXIMIZE);
			ShowCursor(false);
		}

		return 0;
	}

	return g_OriginalWndProc(hwnd, uMsg, wParam, lParam);
}

HWND WINAPI hk_CreateWindowExA(
	_In_     DWORD     dwExStyle,
	_In_opt_ LPCSTR    lpClassName,
	_In_opt_ LPCSTR    lpWindowName,
	_In_     DWORD     dwStyle,
	_In_     int       x,
	_In_     int       y,
	_In_     int       nWidth,
	_In_     int       nHeight,
	_In_opt_ HWND      hWndParent,
	_In_opt_ HMENU     hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID    lpParam
)
{
	HWND wnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (wnd)
	{
		// The original pointer must be saved BEFORE swapping it out
		g_OriginalWndProc = (WNDPROC)GetWindowLongPtrA(wnd, GWLP_WNDPROC);
		SetWindowLongPtrA(wnd, GWLP_WNDPROC, (LONG_PTR)&WindowProc);
	}

	return wnd;
}

void PatchWindow()
{
	PatchIAT(hk_CreateWindowExA, "user32.dll", "CreateWindowExA");
}