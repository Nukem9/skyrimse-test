#pragma once

#include "../../common.h"
#include "UIBaseWindow.h"

namespace MainWindow
{
	extern WNDPROC OldWndProc;

	HWND GetWindow();
	Core::Classes::UI::CUIMainWindow& FIXAPI GetWindowObj();

	void Initialize();
	void CreateExtensionMenu(HWND MainWindow, HMENU MainMenu);

	LRESULT CALLBACK WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
}