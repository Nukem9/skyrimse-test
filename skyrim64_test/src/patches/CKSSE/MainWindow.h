#pragma once

#include "../../common.h"

namespace MainWindow
{
	extern WNDPROC OldWndProc;

	HWND GetWindow();

	void Initialize();
	void CreateExtensionMenu(HWND MainWindow, HMENU MainMenu);

	LRESULT CALLBACK WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
}