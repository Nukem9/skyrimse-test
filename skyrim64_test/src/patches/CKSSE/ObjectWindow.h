#pragma once

#include "../../common.h"

namespace ObjectWindow
{
	extern DLGPROC OldObjectWindowProc;

	INT_PTR CALLBACK ObjectWindowProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void UpdateTreeView(void *Thisptr, HWND ControlHandle);
}